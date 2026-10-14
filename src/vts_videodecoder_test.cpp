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
 * @file vts_videodecoder_test.cpp
 *
 * @brief Contains the videoDecoderTest class implementation and frame parsing logic for video decoder testing.
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
#include <unistd.h> // For getpid()
#include <sys/syscall.h> // For syscall(SYS_gettid)

using namespace std::chrono;
using namespace std;

// AVBufferSpaceListener implementation
Status AVBufferSpaceListener::onSpaceAvailable()
{
    return Status::ok();
}

// videoDecoderTest method implementations
void videoDecoderTest::run(bool concurrentDecodeConsume)
{
    thread consumerThread;

    if (!mAVbufferHelper)
    {
        UT_ASSERT_TRUE(false) << "AVBufferHelper is not initialized";
        return;
    }

    mFrameData.clear();
    mInputBuffer.clear();
    mInputFrame.clear();

    bool status = makeFrame();
    if (!status)
    {
        UT_ASSERT_TRUE(false) << "Failed to prepare frame data for decoding";
        return;
    }

    if (concurrentDecodeConsume)
    {
        if(!mIsSinkAttached)
        {
            // If sink is not attached, we need to validate decoded frames in a separate thread
            consumerThread = thread(&videoDecoderTest::validateDecodedFrame, this);
        }
        setupVideoDecoder();

        if(mVideoDecoderController.get() == nullptr)
        {
            UT_ASSERT_TRUE(false) << "Decoder session is not available";
            if(!mIsSinkAttached && consumerThread.joinable())
            {
                consumerThread.join();
            }
            return;
        }

        startDecoding();

        if(!mIsSinkAttached && consumerThread.joinable())
        {
            // If sink is not attached, we need to validate decoded frames here
            consumerThread.join();
        }

        // Keep decoder running while sink consumes output frames.
        // Session close/cleanup is handled explicitly by the caller.
        if (!mIsSinkAttached)
        {
            teardownDecoder(false);
        }
    }
    else
    {
        //TODO: Not used currently, update implementation if required in future
    }
}

void videoDecoderTest::setTestConfiguration(const videoDecoderTestConfiguration_t& config)
{
    mTestConfiguration = config;
}

void videoDecoderTest::closeDecoderSession()
{
    releaseDecoderSession(true);
}

void videoDecoderTest::getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    framesDecoded = decodeErrors = framesDropped = -1;

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }
}

void videoDecoderTest::openOrReuseVideoDecoder()
{
    if (mVideoDecoderController.get() == nullptr)
    {
        mBufferListener = new AVBufferSpaceListener();

        // Create Video Pool
        AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, mBufferListener, mVideoPool);

        // Open Decoder — use openWithResolution() when hint dimensions are set in the test config
        if (mTestConfiguration.openWithResolutionWidth != -1 && mTestConfiguration.openWithResolutionHeight != -1)
        {
            UT_LOG_INFO("Opening decoder via openWithResolution(%dx%d)",
                        mTestConfiguration.openWithResolutionWidth, mTestConfiguration.openWithResolutionHeight);
            VIDEODECODER_OPEN_WITH_RESOLUTION_SUCCESS(
                mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener,
                mTestConfiguration.openWithResolutionWidth, mTestConfiguration.openWithResolutionHeight,
                mVideoDecoderController, true);
        }
        else
        {
            VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode,
                                      mControlListener, mVideoDecoderController, true);
        }
    }
    else
    {
        UT_LOG_INFO("Reusing open decoder for stream %s with codec %s",
                    mTestConfiguration.streamName.c_str(),
                    videodecoder::toString(mTestConfiguration.codec).c_str());
    }
}

void videoDecoderTest::applyTunnelModePropertyIfNeeded()
{
    if (mTunnelMode)
    {
        std::vector<std::pair<videodecoder::Property, int32_t>> startedIntProps = {
            {videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(OperationalMode::TUNNELLED)}
        };
        UT_LOG_DEBUG("Set properties in TUNNELLED Mode");
        VIDEODECODER_SETPROPERTY_LIST_SUCCESS(mVideoDecoderController, startedIntProps, intValue);
    }
}

void videoDecoderTest::buildAndApplyStreamConfig()
{
    videodecoder::VideoDecoderStreamConfig streamConfig;

    // AdaptiveBitrate streams: codedWidth/codedHeight is the ladder peak (used for
    // capability checks), not the resolution the stream actually starts at. Seed the
    // hint with the first rung instead, so it doesn't contradict the bitstream-signalled
    // resolution changes FrameMetadata is expected to report as the ladder progresses.
    if (!mTestConfiguration.resolutionChanges.empty())
    {
        const auto& firstRung = mTestConfiguration.resolutionChanges.front();
        videodecoder::Resolution res;
        res.width  = firstRung.width;
        res.height = firstRung.height;
        streamConfig.resolution = res;
    }
    else if (mTestConfiguration.codedWidth > 0 && mTestConfiguration.codedHeight > 0)
    {
        videodecoder::Resolution res;
        res.width  = mTestConfiguration.codedWidth;
        res.height = mTestConfiguration.codedHeight;
        streamConfig.resolution = res;
    }
    if (mTestConfiguration.frameRateDenominator > 0)
    {
        videodecoder::Fraction fr;
        fr.numerator   = mTestConfiguration.frameRateNumerator;
        fr.denominator = mTestConfiguration.frameRateDenominator;
        streamConfig.frameRate = fr;
    }
    Colorimetry unknownColorimetry{};
    if (mTestConfiguration.colorimetry != unknownColorimetry)
    {
        streamConfig.colorimetry = mTestConfiguration.colorimetry;
    }
    if (mTestConfiguration.masteringDisplayInfo.has_value())
    {
        streamConfig.masteringDisplayInfo = mTestConfiguration.masteringDisplayInfo;
    }
    if (mTestConfiguration.contentLightLevel.has_value())
    {
        streamConfig.contentLightLevel = mTestConfiguration.contentLightLevel;
    }
    if (mTestConfiguration.pixelAspectRatio.has_value())
    {
        streamConfig.pixelAspectRatio = mTestConfiguration.pixelAspectRatio;
    }
    UT_LOG_INFO("value of the dynamicRange %d", static_cast<int>(mTestConfiguration.dynamicRange));
    if (mTestConfiguration.dynamicRange == videodecoder::DynamicRange::DOLBY_VISION)
    {
        videodecoder::DolbyVisionLayerFlags dvFlags;
        dvFlags.blPresent = mTestConfiguration.blPresent;
        dvFlags.elPresent = mTestConfiguration.elPresent;
        streamConfig.dolbyVisionLayerFlags = dvFlags;
    }
    VIDEODECODER_SETSTREAMCONFIG_SUCCESS(mVideoDecoderController, streamConfig);
}

void videoDecoderTest::applyDecodeErrorPolicyIfNeeded()
{
    // Streams with expected decode errors need conceal mode, otherwise the HAL's
    // default (output corrupt frames) makes expectedFrames/error-count checks below fail.
    if (mTestConfiguration.decodeErrors > 0)
    {
        PropertyValue concealValue;
        concealValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
        VIDEODECODER_SETPROPERTY_SUCCESS(mVideoDecoderController, videodecoder::Property::DECODE_ERROR_POLICY, concealValue);
    }
}

void videoDecoderTest::applyLowLatencyModeIfNeeded()
{
    // Streams flagged lowLatency:1 request LOW_LATENCY_MODE so the decoder skips/
    // discards any B-frames and outputs decoded frames as soon as possible; the
    // resulting FrameMetadata.lowLatency is verified per-frame further below.
    if (mTestConfiguration.lowLatency)
    {
        PropertyValue lowLatencyValue;
        lowLatencyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
        VIDEODECODER_SETPROPERTY_SUCCESS(mVideoDecoderController, videodecoder::Property::LOW_LATENCY_MODE, lowLatencyValue);
    }
}

void videoDecoderTest::setupVideoDecoder()
{
    openOrReuseVideoDecoder();

    if (mVideoDecoderController.get() == nullptr)
    {
        UT_LOG_ERROR("VideoDecoderController is null after open");
        UT_ASSERT_TRUE(false) << "Failed to get VideoDecoderController after opening decoder";
        return;
    }

    applyTunnelModePropertyIfNeeded();
    buildAndApplyStreamConfig();
    applyDecodeErrorPolicyIfNeeded();
    applyLowLatencyModeIfNeeded();

    // Start Decoder
    VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    UT_LOG_DEBUG("Decoder started");
}

void videoDecoderTest::startDecoding()
{
    int64_t videoBufferHandle = 0;
    mEOSignaledCount= 0; //before start decode reset the count to 0

    UT_LOG_DEBUG("[PID: %d, TID: %d] startDecoding Start", getpid(), syscall(SYS_gettid));

    // Process Frames
    for (size_t i = 0; i < mFrameData.size(); ++i)
    {
        auto [frameOffset, frameSize, framePTS, isErrorFrame] = mFrameData[i];

        UT_LOG_INFO("Processing frame %zu: offset=%llu, size=%u, PTS=%lldns, errorFrame=%d",
                    i, frameOffset, frameSize, framePTS, isErrorFrame);

        // Allocate Buffer
        videoBufferHandle = 0;
        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mVideoPool, frameSize, videoBufferHandle);

        if (videoBufferHandle == 0)
        {
            UT_LOG_ERROR("Alloc failed for frame %zu, skipping", i);
            continue;
        }

        // Write Frame Data
        void* mapBuffer = nullptr;
        uint32_t mapSize = 0;
        if (mSecureDecode)
        {
            UT_LOG_INFO("Calling writeSecureHandle(handle:0x%ld, size=%u, mAVbufferHelper:%p)",
                        videoBufferHandle, frameSize, mAVbufferHelper);

            if (!mAVbufferHelper->writeSecureHandle(videoBufferHandle, mInputBuffer.data() + frameOffset, frameSize))
            {
                UT_LOG_ERROR("Failed to copy frame data to secure buffer");
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
        }
        else
        {
            UT_LOG_INFO("Calling mapHandle(handle:0x%ld, mAVbufferHelper:%p)", videoBufferHandle, mAVbufferHelper);

            mapBuffer = mAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
            if (mapBuffer && mapSize >= frameSize)
            {
                UT_LOG_DEBUG("Copying frame data to mapped buffer, size=%u", frameSize);
                memcpy(mapBuffer, mInputBuffer.data() + frameOffset, frameSize);
                mAVbufferHelper->unmapHandle(videoBufferHandle);
            }
            else
            {
                UT_LOG_ERROR("Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                            mapBuffer, mapSize, frameSize);
                if (mapBuffer)
                {
                    mAVbufferHelper->unmapHandle(videoBufferHandle);
                }
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
        }

        // Decoder Operations
        if (mTestConfiguration.stopPts == framePTS)
        {
            UT_LOG_INFO("Stopping decoder at PTS:%lld", framePTS);
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
        if (mTestConfiguration.discontinuityPts == framePTS)
        {
            UT_LOG_INFO("Signaling discontinuity at PTS:%lld", framePTS);
            VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
        // Several NAL units of one picture share its PTS (e.g. SEI, SPS, PPS and the IDR slice).
        // Flush only before the first of them, so the headers sent after the flush are kept.
        const bool isFirstEntryOfPts = (i == 0) || (std::get<2>(mFrameData[i - 1]) != framePTS);
        if (mTestConfiguration.flushPts == framePTS && isFirstEntryOfPts)
        {
            UT_LOG_INFO("Flushing decoder at PTS:%lld", framePTS);
            VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, mVideoDecoderController, false, true);
            VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
        if (mTestConfiguration.flushResetPts == framePTS && isFirstEntryOfPts)
        {
            UT_LOG_INFO("Flush reset at PTS:%lld", framePTS);
            VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, mVideoDecoderController, true, true);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }

        UT_LOG_DEBUG("Adding frame to mInputFrame, PTS:%lld, errorFrame=%d", framePTS, isErrorFrame);

        // Decode Frame
        InputVideoFrame_t inputVideo;
        inputVideo.framePTS = framePTS;
        inputVideo.rms_threshold = mTestConfiguration.rms_diff_threshold;
        inputVideo.isErrorFrame = isErrorFrame;
        inputVideo.sendTime = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(mInputFrameMutex);
            mInputFrame.push_back(std::move(inputVideo));
        }

        videodecoder::InputBufferMetadata metadata = {};
        metadata.nsPresentationTime = framePTS;
        metadata.discontinuity = false;
        if (mTestConfiguration.metadataOverridePts != -1 && mTestConfiguration.metadataOverridePts == framePTS)
        {
            UT_LOG_INFO("Applying metadata override at PTS:%lld", framePTS);
            metadata.colorimetry = mTestConfiguration.overrideColorimetry;
            metadata.masteringDisplayInfo = mTestConfiguration.overrideMasteringDisplayInfo;
            metadata.contentLightLevel = mTestConfiguration.overrideContentLightLevel;
            metadata.dolbyVisionLayerFlags = mTestConfiguration.overrideDolbyVisionLayerFlags;
            metadata.pixelAspectRatio = mTestConfiguration.overridePixelAspectRatio;
        }
        const bool isEosControlPoint = (mTestConfiguration.eosPts == framePTS);

        VIDEODECODER_DECODEBUFFER_WITH_METADATA_SUCCESS(mVideoDecoderController, videoBufferHandle, metadata);

        const bool isFinalFrame = (i == (mFrameData.size() - 1));
        if (isEosControlPoint || isFinalFrame)
        {
            UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
            VIDEODECODER_SIGNAL_EOS_SUCCESS(mVideoDecoderController);
            mEOSignaledCount++;
        }

        if (isEosControlPoint)
        {
            UT_LOG_INFO("Verifying post-EOS decode reject at PTS:%lld", framePTS);
            // Use a fresh handle: videoBufferHandle was already handed to the HAL above, so reusing it here could fail as an invalid handle instead of testing the EOS rejection.
            int64_t postEosBufferHandle = 0;
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mVideoPool, frameSize, postEosBufferHandle);
            // Per spec, decodeBufferWithMetadata() must throw EX_ILLEGAL_STATE once called after signalEndOfStream().
            VIDEODECODER_DECODEBUFFER_WITH_METADATA_FAILURE(mVideoDecoderController, framePTS, postEosBufferHandle,
                                                            false, Status::EX_ILLEGAL_STATE);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, postEosBufferHandle);
            // Let the drain finish before stop(), which would otherwise discard it along with its onEndOfStream().
            UT_ASSERT_TRUE(mControlListener->waitForEndOfStream(mEOSignaledCount, std::chrono::milliseconds(5000)))
                << "onEndOfStream not received for EOS at PTS " << framePTS;
            // Restart only for a mid-stream EOS: the decoder resumes on the next keyframe and the
            // final frame signals a 2nd EOS. At the last frame there is nothing left to send, and
            // stop() would reset the metrics before they are checked.
            if (!isFinalFrame)
            {
                VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(mTestConfiguration.decoderDelay));

        // EOS is sent via signalEndOfStream().
    }

    UT_LOG_DEBUG("[PID: %d, TID: %d] startDecoding End", getpid(), syscall(SYS_gettid));
}

bool videoDecoderTest::validateFrameLatencyAndBuffer(const DecodedVideoFrame_t& decodedFrame,
                                                      std::set<int64_t>& receivedPts,
                                                      int32_t& totalOnFramesDecoded)
{
    // Free the output buffer first, so a frame rejected below does not leak.
    if(decodedFrame.frameAVBufferHandle != -1)
    {
        //TODO: Need to validate buffer content as well. Currently just checking if buffer handle is valid and freeing it.
        bool isBufferValid = false;
        AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameAVBufferHandle, isBufferValid);
        if (isBufferValid)
        {
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameAVBufferHandle);
        }
    }

    auto it = receivedPts.find(decodedFrame.framePTS);
    if (it == receivedPts.end())
    {
        UT_LOG_DEBUG("Unexpected PTS received: %lld", decodedFrame.framePTS);
        UT_ASSERT_TRUE(false);
        return false;
    }
    UT_LOG_DEBUG("Validated PTS: %lld", decodedFrame.framePTS);
    receivedPts.erase(it); // Remove to track received PTS

    {
        std::lock_guard<std::mutex> lock(mInputFrameMutex);
        // Latency calculation: find matching input frame by PTS
        auto inputIt = std::find_if(mInputFrame.begin(), mInputFrame.end(),
            [&decodedFrame](const InputVideoFrame_t& inputFrame) {
                return inputFrame.framePTS == decodedFrame.framePTS;
            });
        if (inputIt != mInputFrame.end())
        {
            auto sendTime = inputIt->sendTime;
            auto receiveTime = decodedFrame.receiveTime;
            auto decodeTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(receiveTime - sendTime).count();
            if (decodeTimeNs > mTestConfiguration.decode_time_threshold)
            {
                UT_LOG_WARNING("Frame PTS:%lld decode time: %lld ns exceeds threshold %lld ns",
                            decodedFrame.framePTS, decodeTimeNs, mTestConfiguration.decode_time_threshold);
            }
            mInputFrame.erase(inputIt);
        }
    }

    totalOnFramesDecoded++;
    return true;
}

namespace {

/* InputBufferMetadata.aidl precedence: bitstream wins, override is a silent-bitstream fallback. */
template <typename T>
void assertOverridableField(const T& actual, bool bitstreamProvidesField, const T& bitstreamValue,
                             bool afterOverride, const std::optional<T>& overrideValue,
                             const char* fieldName, const char* codecName)
{
    T expected;
    if (bitstreamProvidesField)
    {
        expected = bitstreamValue;
    }
    else if (afterOverride && overrideValue.has_value())
    {
        expected = overrideValue.value();
    }
    else
    {
        expected = T{};
    }
    UT_ASSERT_EQUAL(actual, expected) << fieldName << " mismatch, for Codec " << codecName;
}

/* Same rule, for @nullable output fields instead of a sentinel value. */
template <typename T>
void assertOverridableNullableField(const std::optional<T>& actual, const std::optional<T>& bitstreamValue,
                                     bool afterOverride, const std::optional<T>& overrideValue,
                                     const char* fieldName, const char* codecName)
{
    std::optional<T> expected;
    if (bitstreamValue.has_value())
    {
        expected = bitstreamValue;
    }
    else if (afterOverride && overrideValue.has_value())
    {
        expected = overrideValue;
    }
    else
    {
        expected = std::nullopt;
    }
    UT_ASSERT_EQUAL(actual, expected) << fieldName << " mismatch, for Codec " << codecName;
}

}  // namespace

void videoDecoderTest::validateFrameMetadata(const DecodedVideoFrame_t& decodedFrame, int32_t& discontinuityFramesReceived)
{
    UT_LOG_DEBUG("Validating metadata for PTS=%lld", decodedFrame.framePTS);
    // AdaptiveBitrate streams: expected resolution is the ladder segment active at this
    // frame's PTS, not the stream-level codedWidth/codedHeight (which holds the ladder's peak).
    int32_t expectedResWidth = mTestConfiguration.codedWidth;
    int32_t expectedResHeight = mTestConfiguration.codedHeight;
    bool isAdaptiveBitrate = !mTestConfiguration.resolutionChanges.empty();
    if (isAdaptiveBitrate)
    {
        for (const auto& change : mTestConfiguration.resolutionChanges)
        {
            if (change.pts <= decodedFrame.framePTS)
            {
                expectedResWidth = change.width;
                expectedResHeight = change.height;
            }
        }
    }
    // ABR ladder resolutions are nominal/display size, so compare against active dims, not the macroblock-padded coded dims
    int32_t actualResWidth = isAdaptiveBitrate ? decodedFrame.metadata->activeWidth : decodedFrame.metadata->codedWidth;
    int32_t actualResHeight = isAdaptiveBitrate ? decodedFrame.metadata->activeHeight : decodedFrame.metadata->codedHeight;
    UT_ASSERT_EQUAL(actualResWidth, expectedResWidth)
        << "Width mismatch: expected=" << expectedResWidth
        << ", actual=" << actualResWidth
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
    UT_ASSERT_EQUAL(actualResHeight, expectedResHeight)
        << "Height mismatch: expected=" << expectedResHeight
        << ", actual=" << actualResHeight
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

    // colorDepth and dynamicRange are bitstream-derived only (see FrameMetadata.aidl);
    // InputBufferMetadata has no override field for either, so a colorimetry override
    // does not change them regardless of the overridden TransferCharacteristics.
    const bool afterOverride = (mTestConfiguration.metadataOverridePts != -1 &&
                                decodedFrame.framePTS >= mTestConfiguration.metadataOverridePts);
    int32_t expectedColorDepth = mTestConfiguration.colorDepth;
    videodecoder::DynamicRange expectedDynamicRange = mTestConfiguration.dynamicRange;

    UT_ASSERT_EQUAL(decodedFrame.metadata->colorDepth, expectedColorDepth)
        << "Color depth mismatch: expected=" << expectedColorDepth
        << ", actual=" << decodedFrame.metadata->colorDepth
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

    // Validate dynamic range
    UT_ASSERT_EQUAL(decodedFrame.metadata->dynamicRange, expectedDynamicRange)
        << "Dynamic range mismatch: expected=" << static_cast<int>(expectedDynamicRange)
        << ", actual=" << static_cast<int>(decodedFrame.metadata->dynamicRange)
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

    {
        const bool bitstreamProvidesColorimetry = !(mTestConfiguration.colorimetry == videodecoder::Colorimetry{});
        assertOverridableField(decodedFrame.metadata->colorimetry, bitstreamProvidesColorimetry,
                                mTestConfiguration.colorimetry, afterOverride, mTestConfiguration.overrideColorimetry,
                                "Colorimetry", videodecoder::toString(mTestConfiguration.codec).c_str());
    }

    // Validate frame rate
    UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateNumerator, mTestConfiguration.frameRateNumerator)
        << "Frame rate numerator mismatch: expected=" << mTestConfiguration.frameRateNumerator
        << ", actual=" << decodedFrame.metadata->frameRateNumerator
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
    UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateDenominator, mTestConfiguration.frameRateDenominator)
        << "Frame rate denominator mismatch: expected=" << mTestConfiguration.frameRateDenominator
        << ", actual=" << decodedFrame.metadata->frameRateDenominator
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

    // Validate scan type
    UT_ASSERT_EQUAL(decodedFrame.metadata->scanType, mTestConfiguration.scanType)
        << "Scan type mismatch: expected=" << static_cast<int>(mTestConfiguration.scanType)
        << ", actual=" << static_cast<int>(decodedFrame.metadata->scanType)
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

    assertOverridableNullableField(decodedFrame.metadata->masteringDisplayInfo, mTestConfiguration.masteringDisplayInfo,
                                    afterOverride, mTestConfiguration.overrideMasteringDisplayInfo,
                                    "MasteringDisplayInfo", videodecoder::toString(mTestConfiguration.codec).c_str());

    assertOverridableNullableField(decodedFrame.metadata->contentLightLevel, mTestConfiguration.contentLightLevel,
                                    afterOverride, mTestConfiguration.overrideContentLightLevel,
                                    "ContentLightLevel", videodecoder::toString(mTestConfiguration.codec).c_str());

    // FrameMetadata.aidl has no "unknown" value for parX/parY, so skip when we have no ground truth to check against.
    if (mTestConfiguration.pixelAspectRatio.has_value() ||
        (afterOverride && mTestConfiguration.overridePixelAspectRatio.has_value()))
    {
        videodecoder::Fraction actualPar;
        actualPar.numerator = decodedFrame.metadata->parX;
        actualPar.denominator = decodedFrame.metadata->parY;
        assertOverridableField(actualPar, mTestConfiguration.pixelAspectRatio.has_value(),
                                mTestConfiguration.pixelAspectRatio.value_or(videodecoder::Fraction{}),
                                afterOverride, mTestConfiguration.overridePixelAspectRatio,
                                "PixelAspectRatio", videodecoder::toString(mTestConfiguration.codec).c_str());
    }

    if(decodedFrame.metadata->discontinuity)
    {
        discontinuityFramesReceived++;
    }

    UT_ASSERT_EQUAL(decodedFrame.metadata->lowLatency, mTestConfiguration.lowLatency)
        << "lowLatency mismatch: expected=" << mTestConfiguration.lowLatency
        << ", actual=" << decodedFrame.metadata->lowLatency
        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
}

void videoDecoderTest::validateDiscontinuityAndEndOfStream(int32_t discontinuityFramesReceived, int32_t expectedFrames,
                                                            int32_t expectedEOSFrames)
{
    if((mTestConfiguration.discontinuityPts != -1) || (mTestConfiguration.flushPts !=-1))
    {
        UT_ASSERT_TRUE((discontinuityFramesReceived > 0 && discontinuityFramesReceived < expectedFrames));
    }

    UT_ASSERT_TRUE(mControlListener->waitForEndOfStream(expectedEOSFrames, std::chrono::milliseconds(5000)));
    int32_t eosCallbacksReceived = mControlListener->getEndOfStreamCount(false);
    UT_ASSERT_EQUAL(eosCallbacksReceived, expectedEOSFrames);
}

void videoDecoderTest::validateFinalMetrics(int32_t totalOnFramesDecoded, int32_t totalOnErrorsReceived, int32_t expectedFrames)
{
    int32_t metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped;
    getDecoderMetrics(metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped);
    UT_LOG_DEBUG("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec, metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped);
    UT_LOG_DEBUG("Total Callbacks Received: Frames Decoded=%d, Decode Errors=%d, expectedFrames=%d",
                    totalOnFramesDecoded, totalOnErrorsReceived, expectedFrames);

    // eosPts triggers a mid-session stop()+start() (see the isEosControlPoint block above),
    // and Property.aidl documents METRIC_FRAMES_DECODED/METRIC_DECODE_ERRORS as reset on stop() -
    // so exact-total assertions don't hold for it, same as the existing flush cases.
    const bool isResetAfterFlushOrEos =
        (mTestConfiguration.flushResetPts != -1) ||
        (mTestConfiguration.flushPts != -1) ||
        (mTestConfiguration.eosPts != -1);

    // How a decoder counts/conceals errored frames in METRIC_FRAMES_DECODED is implementation-defined,
    // so streams with expected decode errors can only be bounded (<=), not held to an exact total.
    const bool allowBoundedFrameCount = isResetAfterFlushOrEos || (mTestConfiguration.decodeErrors > 0) || (mTestConfiguration.framesDropped > 0);

    // Asserts value == target, or value <= target when the total isn't guaranteed to be exact.
    auto assertExactOrBounded = [allowBoundedFrameCount](int32_t value, int32_t target)
    {
        if (allowBoundedFrameCount)
        {
            UT_ASSERT_TRUE(value <= target);
        }
        else
        {
            UT_ASSERT_EQUAL(value, target);
        }
    };
    // Asserts value > 0 when expected, or value == 0 when not expected.
    auto assertPresentOrZero = [](int32_t value, bool expected)
    {
        if (expected)
        {
            UT_ASSERT_TRUE(value > 0);
        }
        else
        {
            UT_ASSERT_EQUAL(value, 0);
        }
    };

    assertExactOrBounded(totalOnFramesDecoded, expectedFrames);
    if (metricsFramesDecoded != -1)
    {
        assertExactOrBounded(metricsFramesDecoded, expectedFrames);
    }

    assertPresentOrZero(totalOnErrorsReceived, mTestConfiguration.decodeErrors > 0);
    if (metricsDecodeErrors != -1)
    {
        assertPresentOrZero(metricsDecodeErrors, mTestConfiguration.decodeErrors > 0);
    }

    if (metricsFramesDropped != -1)
    {
        assertPresentOrZero(metricsFramesDropped, mTestConfiguration.framesDropped > 0);
    }
}

bool videoDecoderTest::validateDecodedFrame()
{
    DecodedVideoFrame_t decodedFrame;
    // Initialize PTS tracking
    std::set<int64_t> receivedPts(mTestConfiguration.expectedPts.begin(), mTestConfiguration.expectedPts.end());
    int32_t totalOnFramesDecoded = 0;
    int32_t discontinuityFramesReceived = 0;


    while (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(5000)))
    {
        UT_LOG_DEBUG("Retrieved remaining output frame: PTS=%lld, handle=0x%llx, isMetaAvailable=%d",
                decodedFrame.framePTS, decodedFrame.frameAVBufferHandle, decodedFrame.isMetaAvailable);

        if(decodedFrame.framePTS != -1)
        {
            if (!validateFrameLatencyAndBuffer(decodedFrame, receivedPts, totalOnFramesDecoded))
            {
                continue;
            }
        }
        if (decodedFrame.metadata)
        {
            validateFrameMetadata(decodedFrame, discontinuityFramesReceived);
        }
        else
        {
            UT_LOG_INFO("No metadata available for PTS=%lld", decodedFrame.framePTS);
        }
    }

    int32_t expectedFrames = mTestConfiguration.no_of_frames - mTestConfiguration.decodeErrors;
    validateDiscontinuityAndEndOfStream(discontinuityFramesReceived, expectedFrames, mEOSignaledCount);

    int32_t totalOnErrorsReceived = mEventListener->getErrorCount();
    validateFinalMetrics(totalOnFramesDecoded, totalOnErrorsReceived, expectedFrames);

    return true;
}

bool videoDecoderTest::runBackPressureTest()
{
    mControlListener->resetQueues();
    mControlListener->resetDecodeBufferAvailableCount();

    mFrameData.clear();
    mInputBuffer.clear();
    if (!makeFrame())
    {
        UT_ASSERT_TRUE(false) << "Failed to prepare frame data for back-pressure test";
        return false;
    }

    setupVideoDecoder();
    if (mVideoDecoderController.get() == nullptr)
    {
        // open() failed, so only the pool needs to be released.
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mVideoPool);
        return false;
    }

    bool backPressureObserved = false;
    bool callbackObserved = false;
    // Callback count just before the rejected send, so only a new callback counts.
    int callbackBaseline = 0;

    auto freeIfRejected = [&](int64_t bufferHandle, const Status& decodeStatus, bool decodeAccepted)
    {
        if (!decodeAccepted || !decodeStatus.isOk() || decodeStatus.exceptionCode() != Status::EX_NONE)
        {
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, bufferHandle);
        }
    };

    // Output frames are not freed while feeding, so the decoder stalls and its input queue fills up.
    auto freeOutputFrames = [&]()
    {
        DecodedVideoFrame_t decodedFrame;
        while (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(0)))
        {
            if (decodedFrame.frameAVBufferHandle == -1)
            {
                continue;
            }
            bool isBufferValid = false;
            AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameAVBufferHandle, isBufferValid);
            if (isBufferValid)
            {
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameAVBufferHandle);
            }
        }
    };

    // The stream is short, so send it again in a loop with later PTS values until the decoder is full.
    static constexpr size_t kMaxProbeSubmissions = 512;
    int64_t maxPts = 0;
    for (const auto& frame : mFrameData)
    {
        maxPts = std::max(maxPts, std::get<2>(frame));
    }
    const int64_t loopPtsStep = maxPts + 1000000000; // 1 second gap between loops

    for (size_t i = 0; i < kMaxProbeSubmissions; ++i)
    {
        const auto& [frameOffset, frameSize, streamPTS, isErrorFrame] = mFrameData[i % mFrameData.size()];
        const int64_t framePTS = streamPTS + static_cast<int64_t>(i / mFrameData.size()) * loopPtsStep;

        int64_t videoBufferHandle = 0;
        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mVideoPool, frameSize, videoBufferHandle);

        void* mapBuffer = nullptr;
        uint32_t mapSize = 0;
        mapBuffer = mAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
        if (mapBuffer && mapSize >= frameSize)
        {
            memcpy(mapBuffer, mInputBuffer.data() + frameOffset, frameSize);
            mAVbufferHelper->unmapHandle(videoBufferHandle);
        }
        else
        {
            if (mapBuffer) mAVbufferHelper->unmapHandle(videoBufferHandle);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
            UT_ASSERT_TRUE(false) << "Failed to map input buffer for back-pressure test";
            break;
        }

        videodecoder::InputBufferMetadata metadata = {};
        metadata.nsPresentationTime = framePTS;
        metadata.discontinuity = false;

        bool decodeResult = false;
        callbackBaseline = mControlListener->getDecodeBufferAvailableCount();
        Status status = mVideoDecoderController->decodeBufferWithMetadata(videoBufferHandle, metadata, &decodeResult);
        freeIfRejected(videoBufferHandle, status, decodeResult);
        UT_ASSERT_TRUE(status.isOk()) << "decodeBufferWithMetadata binder call failed while probing back-pressure";
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE);

        if (!decodeResult)
        {
            backPressureObserved = true;
            break;
        }
    }

    if (backPressureObserved)
    {
        // Per spec, onDecodeBufferAvailable() is only fired after a refused buffer, so none may arrive
        // while every earlier buffer was accepted.
        UT_ASSERT_EQUAL(callbackBaseline, 0) << "onDecodeBufferAvailable fired before any buffer was refused";

        // Release the held output frames so the decoder can drain its input queue.
        freeOutputFrames();
        callbackObserved = mControlListener->waitForDecodeBufferAvailable(callbackBaseline + 1, std::chrono::milliseconds(5000));
        UT_ASSERT_TRUE(callbackObserved) << "onDecodeBufferAvailable was not received after back-pressure";

        if (callbackObserved)
        {
            // Per spec, the callback fires exactly once per back-pressure episode. No buffer is sent after
            // the refusal, so give any extra callback time to arrive, then check only one was delivered.
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            UT_ASSERT_EQUAL(mControlListener->getDecodeBufferAvailableCount(), callbackBaseline + 1)
                << "onDecodeBufferAvailable fired more than once for one back-pressure episode";
        }
    }

    teardownDecoder();
    freeOutputFrames();
    closeDecoderSession();

    if (backPressureObserved)
    {
        return true;
    }

    UT_LOG_WARNING("[DecoderID: %d] Back-pressure not observed within %zu submissions; callback semantics not asserted on this decoder",
                   mDecoderId.value, kMaxProbeSubmissions);
    return false;
}

void videoDecoderTest:: teardownDecoder(bool closeDecoderSession)
{
    releaseDecoderSession(closeDecoderSession);
}

void videoDecoderTest::releaseDecoderSession(bool closeController)
{
    if (mVideoDecoderController.get() == nullptr)
    {
        return;
    }

    ::com::rdk::hal::videodecoder::State currentState = ::com::rdk::hal::videodecoder::State::UNKNOWN;
    Status getStateStatus = mDecoderHandle->getState(&currentState);
    if (getStateStatus.isOk() && getStateStatus.exceptionCode() == Status::EX_NONE)
    {
        if (currentState == ::com::rdk::hal::videodecoder::State::STARTED ||
            currentState == ::com::rdk::hal::videodecoder::State::STARTING ||
            currentState == ::com::rdk::hal::videodecoder::State::FLUSHING ||
            currentState == ::com::rdk::hal::videodecoder::State::STOPPING)
        {
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
    }
    else
    {
        UT_LOG_WARNING("Skipping stop during releaseDecoderSession: failed to query decoder state");
    }

    if (closeController)
    {
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mVideoPool);
        mVideoDecoderController = nullptr;
        mBufferListener = nullptr;
    }
}

/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
