/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 */

/**
 * @addtogroup VIDEO_DECODER Video Decoder Module
 * @{
 */

/**
 * @addtogroup VIDEO_DECODER_HALTEST Video Decoder HAL Tests
 * @{
 */

/**
 * @file vts_videodecoder_test_utils.cpp
 *
 * @brief Contains frame parsing and frame data preparation utilities for video decoder testing.
 */

#include "vts_videodecoder.h"
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <chrono>
#include <filesystem>
#include <limits>
#include <cstring> // For memcmp()
#include <unistd.h> // For getpid()
#include <sys/syscall.h> // For syscall(SYS_gettid)

using namespace std::chrono;
using namespace std;

/**
 * @brief Returns the offset of the next start code after @p offset, or @p fileSize if none.
 */
static uint64_t findNextStartCode(const std::vector<uint8_t>& inputBuffer, uint64_t fileSize, uint64_t offset)
{
    for (uint64_t pos = offset + 3; pos + 3 <= fileSize; ++pos)
    {
        if (inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 && inputBuffer[pos + 2] == 1)
        {
            return (inputBuffer[pos - 1] == 0 && pos - 1 > offset) ? pos - 1 : pos;
        }
    }
    return fileSize;
}

std::vector<std::tuple<uint64_t, uint32_t, int64_t>> videoDecoderTest::parseFrames(const std::vector<uint8_t>& inputBuffer, uint64_t fileSize, com::rdk::hal::videodecoder::Codec codec)
{
    std::vector<std::tuple<uint64_t, uint32_t, int64_t>> frames; // {offset, size, pts}

    if (mTestConfiguration.frameDetails.empty()) {
        UT_LOG_ERROR("frameDetails is empty");
        return frames;
    }

    switch (codec) {
        case com::rdk::hal::videodecoder::Codec::H264_AVC:
        case com::rdk::hal::videodecoder::Codec::H265_HEVC: {
            uint64_t pos = 0;
            size_t frameIndex = 0;
            while (pos < fileSize - 4) {
                bool isStartCode = false;
                uint32_t startCodeLen = 0;
                if (pos < fileSize - 3 && inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 && inputBuffer[pos + 2] == 1) {
                    isStartCode = true;
                    startCodeLen = 3;
                } else if (pos < fileSize - 4 && inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 &&
                           inputBuffer[pos + 2] == 0 && inputBuffer[pos + 3] == 1) {
                    isStartCode = true;
                    startCodeLen = 4;
                }

                if (isStartCode) {
                    uint64_t start = pos;
                    pos += startCodeLen;
                    uint64_t nextPos = pos;
                    while (nextPos < fileSize - 3) {
                        if ((inputBuffer[nextPos] == 0 && inputBuffer[nextPos + 1] == 0 && inputBuffer[nextPos + 2] == 1) ||
                            (nextPos < fileSize - 4 && inputBuffer[nextPos] == 0 && inputBuffer[nextPos + 1] == 0 &&
                             inputBuffer[nextPos + 2] == 0 && inputBuffer[nextPos + 3] == 1)) {
                            break;
                        }
                        nextPos++;
                    }
                    uint32_t frameSize = nextPos - start;
                    if (frameSize > startCodeLen) {
                        int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                      std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                      frameIndex * 33333333; // Default 30fps
                        frames.emplace_back(start, frameSize, pts);
                        UT_LOG_INFO("Frame detected: offset=%llu, size=%u, pts=%lld", start, frameSize, pts);
                    }
                    pos = nextPos;
                    frameIndex++;
                } else {
                    pos++;
                }
            }
            break;
        }
        case com::rdk::hal::videodecoder::Codec::MPEG2_VIDEO: {
            uint64_t pos = 0;
            size_t frameIndex = 0;
            while (pos < fileSize - 4) {
                if (inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 && inputBuffer[pos + 2] == 1 &&
                    inputBuffer[pos + 3] >= 0xE0 && inputBuffer[pos + 3] <= 0xEF) { // Video stream ID
                    uint64_t start = pos;
                    pos += 4;
                    if (pos + 2 >= fileSize) break;
                    uint32_t pesLength = (inputBuffer[pos] << 8) | inputBuffer[pos + 1];
                    pos += 2;
                    uint32_t frameSize = pesLength + 6; // Include start code and length
                    if (start + frameSize > fileSize) break;
                    int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                  std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                  frameIndex * 33333333; // Default 30fps
                    frames.emplace_back(start, frameSize, pts);
                    UT_LOG_INFO("MPEG PES packet detected: offset=%llu, size=%u, pts=%lld", start, frameSize, pts);
                    pos = start + frameSize;
                    frameIndex++;
                } else {
                    pos++;
                }
            }
            break;
        }
        case com::rdk::hal::videodecoder::Codec::VP9:
        case com::rdk::hal::videodecoder::Codec::AV1: {
            const bool isVp9 = (codec == com::rdk::hal::videodecoder::Codec::VP9);
            const char* codecName = isVp9 ? "VP9" : "AV1";
            // VP9 streams are always IVF; AV1 streams may be IVF ("DKIF" signature) or raw OBUs.
            const bool isIvf = isVp9 || (fileSize >= 32 && memcmp(inputBuffer.data(), "DKIF", 4) == 0);
            if (isIvf) {
                uint64_t pos = 32; // Skip IVF header (fixed 32 bytes)
                size_t frameIndex = 0;
                while (pos + 12 <= fileSize) {
                    uint32_t frameSize = (uint32_t)inputBuffer[pos] |
                                         ((uint32_t)inputBuffer[pos + 1] << 8) |
                                         ((uint32_t)inputBuffer[pos + 2] << 16) |
                                         ((uint32_t)inputBuffer[pos + 3] << 24);
                    uint64_t timestamp = static_cast<uint64_t>(inputBuffer[pos + 4]) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 5]) << 8) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 6]) << 16) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 7]) << 24) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 8]) << 32) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 9]) << 40) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 10]) << 48) |
                                         (static_cast<uint64_t>(inputBuffer[pos + 11]) << 56);

                    uint64_t frameStart = pos + 12;
                    if (frameStart + frameSize > fileSize) break;

                    int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                  std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                  static_cast<int64_t>(timestamp); // Use IVF timestamp

                    // Error frame: send the smaller YAML size; the next frame still starts at the real size.
                    // VP9 only: the AV1 YAML frame sizes do not match the IVF frames, so AV1 frames are sent whole.
                    uint32_t sendSize = frameSize;
                    if (isVp9 && frameIndex < mTestConfiguration.frameDetails.size())
                    {
                        int32_t yamlSize = std::get<0>(mTestConfiguration.frameDetails[frameIndex]);
                        if (yamlSize > 0 && static_cast<uint32_t>(yamlSize) < frameSize)
                        {
                            UT_LOG_INFO("VP9 truncated frame %zu: sending %d of %u bytes", frameIndex, yamlSize, frameSize);
                            sendSize = static_cast<uint32_t>(yamlSize);
                        }
                    }
                    frames.emplace_back(frameStart, sendSize, pts);
                    UT_LOG_INFO("%s IVF frame: offset=%llu, size=%u, pts=%lld", codecName, frameStart, sendSize, pts);

                    pos = frameStart + frameSize;
                    frameIndex++;
                }
                break;
            }

            // Raw AV1 OBU stream (no IVF container).
            uint64_t pos = 0;
            size_t frameIndex = 0;
            while (pos < fileSize) {
                if (pos + 1 >= fileSize) break;
                uint8_t obuHeader = inputBuffer[pos];
                uint32_t obuType = (obuHeader >> 3) & 0x1F;
                bool hasSize = (obuHeader & 0x02) == 0;
                uint64_t start = pos;
                pos++;
                uint32_t obuSize = 0;
                if (hasSize) {
                    for (int i = 0; i < 8 && pos < fileSize; i++) {
                        uint8_t byte = inputBuffer[pos++];
                        obuSize |= ((uint32_t)(byte & 0x7F)) << (i * 7);
                        if (!(byte & 0x80)) break;
                    }
                }
                if (obuType == 1 || obuType == 3 || obuType == 6) { // Frame or Temporal Unit OBUs
                    uint32_t frameSize = obuSize + (pos - start);
                    if (start + frameSize > fileSize) break;
                    int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                  std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                  frameIndex * 33333333; // Default 30fps
                    frames.emplace_back(start, frameSize, pts);
                    UT_LOG_INFO("AV1 OBU detected: offset=%llu, size=%u, pts=%lld", start, frameSize, pts);
                    pos += obuSize;
                    frameIndex++;
                } else {
                    pos += obuSize;
                }
            }
            break;
        }
        default:
            UT_LOG_ERROR("Unsupported codec: %d", static_cast<int>(codec));
            return frames;
    }

    if (!frames.empty() && frames.size() != mTestConfiguration.frameDetails.size()) {
        UT_LOG_WARNING("Frame count mismatch: parsed=%zu, expected=%zu", frames.size(), mTestConfiguration.frameDetails.size());
    }
    return frames;
}

bool videoDecoderTest::makeFrame()
{
    if (!std::filesystem::exists(mTestConfiguration.input))
    {
        UT_LOG_ERROR("Input file does not exist: %s", mTestConfiguration.input.c_str());
        return false;
    }

    std::ifstream inputFile(mTestConfiguration.input, std::ios::binary);
    if (!inputFile.is_open())
    {
        UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
        return false;
    }

    inputFile.seekg(0, std::ios::end);
    auto fileSizePos = inputFile.tellg();
    if (fileSizePos < 0)
    {
        UT_LOG_ERROR("Failed to get file size for %s: tellg returned %lld",
                    mTestConfiguration.input.c_str(), static_cast<int64_t>(fileSizePos));
        return false;
    }
    uint64_t fileSize = static_cast<uint64_t>(fileSizePos);
    if (fileSize == 0 || fileSize > std::numeric_limits<uint32_t>::max())
    {
        UT_LOG_ERROR("File size too large or empty: %llu bytes, max allowed: %u bytes",
                    fileSize, std::numeric_limits<uint32_t>::max());
        return false;
    }
    UT_LOG_DEBUG("Input file size: %llu bytes", fileSize);

    mInputBuffer.clear();
    mInputBuffer.resize(fileSize);

    try
    {
        inputFile.seekg(0, std::ios::beg);
        inputFile.read(reinterpret_cast<char*>(mInputBuffer.data()), fileSize);
        if (!inputFile)
        {
            UT_LOG_ERROR("Failed to read file data: only %zu bytes read", inputFile.gcount());
            return false;
        }
        uint64_t actualSize = static_cast<uint64_t>(inputFile.gcount());
        if (actualSize != fileSize)
        {
            UT_LOG_WARNING("Read %llu bytes, expected %llu bytes", actualSize, fileSize);
            fileSize = actualSize;
        }
    }
    catch (const std::exception& e)
    {
        UT_LOG_ERROR("Exception reading file: %s", e.what());
        return false;
    }

    // Determine frame data based on codec
    if (mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H264_AVC ||
        mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H265_HEVC)
    {
        if (mTestConfiguration.frameDetails.empty())
        {
            UT_LOG_ERROR("frameDetails is empty for H.264/H.265");
            return false;
        }
        uint64_t offset = 0;
        for (const auto& [size, pts, isErrorFrame] : mTestConfiguration.frameDetails)
        {
            if (offset + size > fileSize)
            {
                UT_LOG_ERROR("Frame size at offset %llu exceeds file size %llu", offset + size, fileSize);
                return false;
            }
            mFrameData.emplace_back(offset, size, pts, isErrorFrame);
            UT_LOG_DEBUG("Codec:%d Frame from frameDetails: offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                        mTestConfiguration.codec, offset, size, pts, isErrorFrame);

            // Error frame: skip to the real start of the next frame, so later frames stay aligned.
            uint64_t nextStart = findNextStartCode(mInputBuffer, fileSize, offset);
            uint64_t realSize = nextStart - offset;
            if (nextStart < fileSize && static_cast<uint64_t>(size) < realSize)
            {
                UT_LOG_INFO("Codec:%d Truncated frame at offset=%llu: sending %u of %llu bytes",
                            mTestConfiguration.codec, offset, size, realSize);
                offset += realSize;
            }
            else
            {
                offset += size;
            }
        }
    }
    else
    {
        auto parsedFrames = parseFrames(mInputBuffer, fileSize, mTestConfiguration.codec);
        if (parsedFrames.empty())
        {
            UT_LOG_ERROR("No frames parsed for codec: %d", static_cast<int>(mTestConfiguration.codec));
            return false;
        }
        for (size_t i = 0; i < parsedFrames.size(); ++i)
        {
            auto [offset, size, pts] = parsedFrames[i];
            bool isErrorFrame = (i < mTestConfiguration.frameDetails.size()) ?
                                std::get<2>(mTestConfiguration.frameDetails[i]) : false;
            mFrameData.emplace_back(offset, size, pts, isErrorFrame);
            UT_LOG_DEBUG("Parsed frame: Codec:%d offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                          mTestConfiguration.codec, offset, size, pts, isErrorFrame);
        }
    }

    return true;
}

/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
