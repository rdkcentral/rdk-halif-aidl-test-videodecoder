/**
*  If not stated otherwise in this file or this component's LICENSE
*  file the following copyright and licenses apply:
*
*  Copyright 2025 RDK Management
*
*  Licensed under the Apache License, Version 2.0 (the License);
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an AS IS BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/

/** @brief
 * VideoDecoder.cpp
 *
 * Implementation of VideoDecoder service
 */

#include "VideoDecoder.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <android-base/logging.h>

namespace com {
namespace rdk {
namespace hal {
namespace videodecoder {

VideoDecoderManager::VideoDecoderManager() {
    printf("VideoDecoderManager service initialized\n");
}

VideoDecoderManager::~VideoDecoderManager() {
    printf("VideoDecoderManager service destroyed\n");
}

android::binder::Status VideoDecoderManager::getVideoDecoderIds(std::vector<IVideoDecoder::Id>* _aidl_return) {
    for (int32_t i = 0; i < NUM_VIDEO_DECODER; i++) {
        IVideoDecoder::Id id;
        id.value = i;
        _aidl_return->push_back(id);
    }
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderManager::getSupportedOperationalModes(std::vector<OperationalMode>* _aidl_return) {
// _aidl_return->push_back(OperationalMode::TUNNELLED);
    _aidl_return->push_back(OperationalMode::NON_TUNNELLED);
// _aidl_return->push_back(OperationalMode::GRAPHICS_TEXTURE);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderManager::getVideoDecoder(const IVideoDecoder::Id& decoderResourceId, android::sp<IVideoDecoder>* _aidl_return) {
    if (decoderResourceId.value < 0 || decoderResourceId.value >= NUM_VIDEO_DECODER) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    android::sp<IVideoDecoder> videoDecoder = new VideoDecoder(decoderResourceId);
    *_aidl_return = videoDecoder;
    return android::binder::Status::ok();
}

VideoDecoder::VideoDecoder(IVideoDecoder::Id id) {
    this->mId = id;
    this->mEventListener = false;
    this->mVideoDecoderEventListener = nullptr;
    this->mLowLatency = 0;
    this->mAVSource = AVSource::UNKNOWN;
    this->mDecodeErrorPolicy = 0;
    this->mSha1Calc = 0;
    this->mOperationalMode = static_cast<int32_t>(OperationalMode::NON_TUNNELLED);
    this->mSupportsSecure = 1;
    this->mMetricsFrameDecoded = 0;
    this->mMetricsDecodeErrors = 0;
    this->mMetricsFramesDropped = 0;
    this->mInputQueueDepth = 96000;
    this->mOutputFramePoolSize = 5;
    this->mSupportedCodecs = {
        Codec::MPEG, Codec::H264, Codec::H265, Codec::VP9, Codec::AV1
    };
    this->setState(State::CLOSED);
    printf("VideoDecoder service initialized for ID: %d\n", id.value);
}

VideoDecoder::~VideoDecoder() {
    printf("VideoDecoder service destroyed for ID: %d\n", mId.value);
}

void VideoDecoder::setState(State state) {
    std::lock_guard<std::mutex> lock(mStateMutex);
    mVideoDecoderState = state;
}

State VideoDecoder::getState() {
    std::lock_guard<std::mutex> lock(mStateMutex);
    return mVideoDecoderState;
}

android::binder::Status VideoDecoder::getCapabilities(videodecoder::Capabilities* _aidl_return) {
    Capabilities capabilities;
    CodecCapabilities mpeg;
    mpeg.codec = Codec::MPEG;
    mpeg.maxFrameRate = 60;
    mpeg.maxFrameWidth = 1920;
    mpeg.maxFrameHeight = 1080;

    CodecCapabilities h264;
    h264.codec = Codec::H264;
    h264.maxFrameRate = 60;
    h264.maxFrameWidth = 1920;
    h264.maxFrameHeight = 1080;

    CodecCapabilities h265;
    h265.codec = Codec::H265;
    h265.maxFrameRate = 60;

    CodecCapabilities vp9;
    vp9.codec = Codec::VP9;
    vp9.maxFrameRate = 60;

    CodecCapabilities av1;
    av1.codec = Codec::AV1;
    av1.maxFrameRate = 60;

    if (mId.value == 0) {
        h265.maxFrameWidth = 3840;
        h265.maxFrameHeight = 2160;
        vp9.maxFrameWidth = 3840;
        vp9.maxFrameHeight = 2160;
        av1.maxFrameWidth = 3840;
        av1.maxFrameHeight = 2160;
        capabilities.supportedDynamicRanges = {
            DynamicRange::SDR,
            DynamicRange::HLG,
            DynamicRange::HDR10,
            DynamicRange::DOLBY_VISION
        };
    } else if (mId.value == 1){
        h265.maxFrameWidth = 1920;
        h265.maxFrameHeight = 1080;
        vp9.maxFrameWidth = 1920;
        vp9.maxFrameHeight = 1080;
        av1.maxFrameWidth = 1920;
        av1.maxFrameHeight = 1080;
        capabilities.supportedDynamicRanges = {
            DynamicRange::SDR
        };
    }

    capabilities.supportedCodecs = {mpeg, h264, h265, vp9, av1};
    capabilities.supportsSecure = mSupportsSecure;
    *_aidl_return = capabilities;
    return android::binder::Status::ok();
}


android::binder::Status VideoDecoder::getProperty(videodecoder::Property property, std::optional<PropertyValue>* _aidl_return) {
    if (!_aidl_return) {
        LOG(ERROR) << "getProperty: null _aidl_return";
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }
    PropertyValue value;
    switch (property) {
        case Property::RESOURCE_ID:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mId.value);
            LOG(DEBUG) << "getProperty: RESOURCE_ID = " << mId.value;
            break;
        case Property::LOW_LATENCY_MODE:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mLowLatency);
            LOG(DEBUG) << "getProperty: LOW_LATENCY_MODE = " << mLowLatency;
            break;
        case Property::AV_SOURCE:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(static_cast<int32_t>(mAVSource));
            LOG(DEBUG) << "getProperty: AV_SOURCE = " << static_cast<int32_t>(mAVSource);
            break;
        case Property::DECODE_ERROR_POLICY:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mDecodeErrorPolicy);
            LOG(DEBUG) << "getProperty: DECODE_ERROR_POLICY = " << mDecodeErrorPolicy;
            break;
        case Property::SHA1_CALC:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mSha1Calc);
            LOG(DEBUG) << "getProperty: SHA1_CALC = " << mSha1Calc;
            break;
        case Property::OPERATIONAL_MODE:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mOperationalMode);
            LOG(DEBUG) << "getProperty: OPERATIONAL_MODE = " << mOperationalMode;
            break;
        case Property::SECURE_VIDEO:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mSupportsSecure);
            LOG(DEBUG) << "getProperty: SECURE_VIDEO = " << mSupportsSecure;
            break;
        case Property::METRIC_FRAMES_DECODED:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mMetricsFrameDecoded);
            LOG(DEBUG) << "getProperty: METRIC_FRAMES_DECODED = " << mMetricsFrameDecoded;
            break;
        case Property::METRIC_DECODE_ERRORS:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mMetricsDecodeErrors);
            LOG(DEBUG) << "getProperty: METRIC_DECODE_ERRORS = " << mMetricsDecodeErrors;
            break;
        case Property::METRIC_FRAMES_DROPPED:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mMetricsFramesDropped);
            LOG(DEBUG) << "getProperty: METRIC_FRAMES_DROPPED = " << mMetricsFramesDropped;
            break;
        case Property::INPUT_QUEUE_DEPTH:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mInputQueueDepth);
            LOG(DEBUG) << "getProperty: INPUT_QUEUE_DEPTH = " << mInputQueueDepth;
            break;
        case Property::OUTPUT_FRAME_POOL_SIZE:
            value.value = PropertyValue::Value::make<PropertyValue::Value::intValue>(mOutputFramePoolSize);
            LOG(DEBUG) << "getProperty: OUTPUT_FRAME_POOL_SIZE = " << mOutputFramePoolSize;
            break;
        default:
            LOG(ERROR) << "getProperty: invalid property value: " << static_cast<int32_t>(property);
            *_aidl_return = std::nullopt;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    *_aidl_return = value;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::getPropertyMulti(const std::vector<videodecoder::Property>& properties, std::vector<videodecoder::PropertyKVPair>* propertyKVList, bool* _aidl_return) {
    // Check _aidl_return first before dereferencing it anywhere
    if (!_aidl_return) {
        LOG(ERROR) << "getPropertyMulti: null _aidl_return output parameter";
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    if (!propertyKVList) {
        LOG(ERROR) << "getPropertyMulti: null propertyKVList output parameter";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    if (properties.empty()) {
        LOG(ERROR) << "getPropertyMulti: empty properties input";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    propertyKVList->clear();

    for (const auto& property : properties) {
        LOG(DEBUG) << "getPropertyMulti: processing property " << static_cast<int32_t>(property);
        std::optional<PropertyValue> value;
        android::binder::Status status = getProperty(property, &value);
        if (!status.isOk()) {
            LOG(ERROR) << "getPropertyMulti: failed for property " << static_cast<int32_t>(property) << ", status: " << status.exceptionCode();
            *_aidl_return = false;
            propertyKVList->clear();
            return status;
        }

        videodecoder::PropertyKVPair kvPair;
        kvPair.property = property;
        kvPair.propertyValue = value.value();
        propertyKVList->push_back(kvPair);
    }

    LOG(DEBUG) << "getPropertyMulti: all properties processed successfully";
    *_aidl_return = true;
    return android::binder::Status::ok();
}


android::binder::Status VideoDecoder::getState(State* _aidl_return) {
    *_aidl_return = getState();
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::open(videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, android::sp<videodecoder::IVideoDecoderController>* _aidl_return) {
    if (std::find(mSupportedCodecs.begin(), mSupportedCodecs.end(), codec) == mSupportedCodecs.end()) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    if (codec == Codec::MPEG && !secure) { // Fail for MPEG with secure:false
        LOG(ERROR) << "open: MPEG codec not supported with secure:false";
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    if (videoDecoderControllerListener == nullptr) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    if (getState() != State::CLOSED) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (mEventListener) {
        mVideoDecoderEventListener->onStateChanged(getState(), State::OPENING);
    }
    setState(State::OPENING);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (mEventListener) {
        mVideoDecoderEventListener->onStateChanged(State::OPENING, State::READY);
    }
    setState(State::READY);
    *_aidl_return = new VideoDecoderController(mId, this);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::close(const android::sp<videodecoder::IVideoDecoderController>& videoDecoderController, bool* _aidl_return) {
    if (getState() != State::READY) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (mEventListener) {
        mVideoDecoderEventListener->onStateChanged(getState(), State::CLOSING);
    }
    setState(State::CLOSING);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (mEventListener) {
        mVideoDecoderEventListener->onStateChanged(State::CLOSING, State::CLOSED);
    }
    setState(State::CLOSED);
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::registerEventListener(const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return) {
    if (mEventListener) {
        *_aidl_return = false;
        return android::binder::Status::ok();
    }
    *_aidl_return = true;
    mEventListener = true;
    mVideoDecoderEventListener = videoDecoderEventListener;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::unregisterEventListener(const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return) {
    if (!mEventListener) {
        *_aidl_return = false;
        return android::binder::Status::ok();
    }
    *_aidl_return = true;
    mEventListener = false;
    mVideoDecoderEventListener = nullptr;
    return android::binder::Status::ok();
}

VideoDecoderController::VideoDecoderController(IVideoDecoder::Id id, VideoDecoder* parentDecoder)
    : mId(id), mParentDecoder(parentDecoder) {
    printf("VideoDecoderController service initialized for ID: %d\n", id.value);
}

VideoDecoderController::~VideoDecoderController() {
    printf("VideoDecoderController service destroyed for ID: %d\n", mId.value);
}

android::binder::Status VideoDecoderController::start() {
    if (mParentDecoder->getState() != State::READY) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (mParentDecoder->mEventListener) {
        mParentDecoder->mVideoDecoderEventListener->onStateChanged(mParentDecoder->getState(), State::STARTING);
    }
    mParentDecoder->setState(State::STARTING);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (mParentDecoder->mEventListener) {
        mParentDecoder->mVideoDecoderEventListener->onStateChanged(State::STARTING, State::STARTED);
    }
    mParentDecoder->setState(State::STARTED);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::stop() {
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (mParentDecoder->mEventListener) {
        mParentDecoder->mVideoDecoderEventListener->onStateChanged(mParentDecoder->getState(), State::STOPPING);
    }
    mParentDecoder->setState(State::STOPPING);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (mParentDecoder->mEventListener) {
        mParentDecoder->mVideoDecoderEventListener->onStateChanged(State::STOPPING, State::READY);
    }
    mParentDecoder->setState(State::READY);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::decodeBuffer(int64_t nsPresentationTime, int64_t bufferHandle, bool* _aidl_return) {
    if (mParentDecoder->getState() != State::STARTED) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    if (nsPresentationTime < 0) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::flush(bool reset) {
    if (!mParentDecoder) {
        LOG(ERROR) << "flush: mParentDecoder is null";
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (mParentDecoder->mEventListener) {
        mParentDecoder->mVideoDecoderEventListener->onStateChanged(mParentDecoder->getState(), State::FLUSHING);
    }
    mParentDecoder->setState(State::FLUSHING);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    State targetState = reset ? State::READY : State::STARTED;
    if (mParentDecoder->mEventListener) {
        mParentDecoder->mVideoDecoderEventListener->onStateChanged(State::FLUSHING, targetState);
    }
    mParentDecoder->setState(targetState);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::signalDiscontinuity() {
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::signalEOS() {
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::parseCodecSpecificData(rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return) {
    // Check if mParentDecoder is null
    if (!mParentDecoder) {
        LOG(ERROR) << "parseCodecSpecificData: mParentDecoder is null";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    // Check the state (require STARTED per test expectation)
    State currentState = mParentDecoder->getState();
    if (currentState != State::STARTED) {
        LOG(ERROR) << "parseCodecSpecificData: Invalid state: " << static_cast<int32_t>(currentState) << ", expected STARTED";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    // Check if codecData is empty (test expects failure for empty data)
    if (codecData.empty()) {
        LOG(ERROR) << "parseCodecSpecificData: codecData is empty";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    // Validate csdVideoFormat against the codec
    bool csdFormatValid = false;
    if (csdVideoFormat == rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD) {
        csdFormatValid = true;
    } else if (csdVideoFormat == rdk::hal::videodecoder::CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD) {
        csdFormatValid = true;
    } else if (csdVideoFormat == rdk::hal::videodecoder::CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD) {
        csdFormatValid = true;
    }

    if (!csdFormatValid) {
        LOG(ERROR) << "parseCodecSpecificData: Invalid csdVideoFormat " << static_cast<int32_t>(csdVideoFormat);
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    // Validate codec-specific data based on codec
    if (csdVideoFormat == rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD) {
        // Minimum size check for AVCDecoderConfigurationRecord (H.264)
        // - Version (1 byte), Profile (1 byte), Compatibility (1 byte), Level (1 byte), Length size (1 byte),
        // - Number of SPS (1 byte), SPS length (2 bytes), SPS data (at least 1 byte),
        // - Number of PPS (1 byte), PPS length (2 bytes), PPS data (at least 1 byte)
        // Minimum size = 13 bytes
        if (codecData.size() < 13) {
            LOG(ERROR) << "parseCodecSpecificData: codecData too small for H.264 (" << codecData.size() << " bytes, expected at least 13)";
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }

        // Basic structure validation
        if (codecData[0] != 1) {
            LOG(ERROR) << "parseCodecSpecificData: Invalid AVCDecoderConfigurationRecord version: " << static_cast<int>(codecData[0]);
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
    } else if (csdVideoFormat == rdk::hal::videodecoder::CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD) {
        // Minimum size check for HEVCDecoderConfigurationRecord (H.265)
        // - Version (1 byte), General profile space/tier/profile (5 bytes), Constraint flags (6 bytes),
        // - Level IDC (1 byte), Min spatial segmentation IDC (2 bytes), Chroma format IDC (1 byte)
        // Minimum size = 14 bytes (without VPS/SPS/PPS NAL units)
        if (codecData.size() < 14) {
            LOG(ERROR) << "parseCodecSpecificData: codecData too small for H.265 (" << codecData.size() << " bytes, expected at least 14)";
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }

        // Basic structure validation
        if (codecData[0] != 1) {
            LOG(ERROR) << "parseCodecSpecificData: Invalid HEVCDecoderConfigurationRecord version: " << static_cast<int>(codecData[0]);
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
    } else if (csdVideoFormat == rdk::hal::videodecoder::CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD) {
        // Minimum size check for AV1DecoderConfigurationRecord
        // - Marker/version/seq profile (1 byte), Seq level idx 0 (1 byte), Seq tier 0 (1 byte), Reserved (1 byte)
        // Minimum size = 4 bytes
        if (codecData.size() < 4) {
            LOG(ERROR) << "parseCodecSpecificData: codecData too small for AV1 (" << codecData.size() << " bytes, expected at least 4)";
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }

        // Basic structure validation
        if ((codecData[0] & 0x80) != 0x80) {  // Check marker bit
            LOG(ERROR) << "parseCodecSpecificData: Invalid AV1DecoderConfigurationRecord marker: " << static_cast<int>(codecData[0]);
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
    }

    LOG(DEBUG) << "parseCodecSpecificData: Successfully parsed codecData, size: " << codecData.size();
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::setProperty(videodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return) {
    if (!propertyValue.value.has_value()) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    int32_t intvalue;
    *_aidl_return = true;
    switch (property) {
        case Property::LOW_LATENCY_MODE:
            if (mParentDecoder->getState() != State::READY) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
            }
            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
            if (intvalue < 0 || intvalue > 1) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
            }
            mParentDecoder->mLowLatency = intvalue;
            break;
        case Property::AV_SOURCE:
            if (mParentDecoder->getState() != State::READY) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
            }
            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
            if (intvalue < static_cast<int32_t>(AVSource::AUTO) || intvalue > static_cast<int32_t>(AVSource::COMPOSITE_5)) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
            }
            mParentDecoder->mAVSource = static_cast<AVSource>(intvalue);
            break;
        case Property::DECODE_ERROR_POLICY:
            if (mParentDecoder->getState() != State::READY) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
            }
            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
            if (intvalue < 0 || intvalue > 1) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
            }
            mParentDecoder->mDecodeErrorPolicy = intvalue;
            break;
        case Property::SHA1_CALC:
            if (mParentDecoder->getState() != State::READY && mParentDecoder->getState() != State::STARTED) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
            }
            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
            if (intvalue < 0 || intvalue > 1) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
            }
            mParentDecoder->mSha1Calc = intvalue;
            break;
        case Property::OPERATIONAL_MODE:
            if (mParentDecoder->getState() != State::READY && mParentDecoder->getState() != State::STARTED) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
            }
            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
            if (intvalue < static_cast<int32_t>(OperationalMode::TUNNELLED) || intvalue > static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE)) {
                *_aidl_return = false;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
            }
            mParentDecoder->mOperationalMode = intvalue;
            break;
        default:
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    return android::binder::Status::ok();
}

} // namespace videodecoder
} // namespace hal
} // namespace rdk
} // namespace com