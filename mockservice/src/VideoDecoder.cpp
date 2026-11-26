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
#include <vector>
#include <string>
#include <cstdint>
#include <android-base/logging.h>
#include <binder/IServiceManager.h>

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

android::binder::Status VideoDecoderManager::getVideoDecoder(const IVideoDecoder::Id& decoderResourceId, android::sp<IVideoDecoder>* _aidl_return) {
    // Per IVideoDecoderManager.aidl: invalid ID returns null with EX_NONE, not an exception.
    if (decoderResourceId.value < 0 || decoderResourceId.value >= NUM_VIDEO_DECODER) {
        *_aidl_return = nullptr;
        return android::binder::Status::ok();
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
    this->mDecodeErrorPolicySet = false;
    this->mSha1Calc = 0;
    this->mOperationalMode = static_cast<int32_t>(OperationalMode::NON_TUNNELLED);
    this->mSupportsSecure = 1;
    this->mMetricsFrameDecoded = 0;
    this->mMetricsDecodeErrors = 0;
    this->mMetricsFramesDropped = 0;
    this->mInputQueueDepth = 96000;
    this->mOutputFramePoolSize = 5;
    this->mSupportedCodecs = {
        Codec::MPEG2_VIDEO, Codec::H264_AVC, Codec::H265_HEVC, Codec::VP9, Codec::AV1
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
    /* maxFrameHeight uses 1088, not 1080: coded height is padded up to the next
     * 16-pixel macroblock boundary, and 1080p test streams declare codedHeight=1088. */
    CodecCapabilities mpeg;
    mpeg.codec = Codec::MPEG2_VIDEO;
    mpeg.maxFrameRate = 60;
    mpeg.maxFrameWidth = 1920;
    mpeg.maxFrameHeight = 1088;
    mpeg.supportedOutputPixelFormats = {PixelFormat::YCBCR420};

    CodecCapabilities h264;
    h264.codec = Codec::H264_AVC;
    h264.maxFrameRate = 60;
    h264.maxFrameWidth = 1920;
    h264.maxFrameHeight = 1088;
    h264.supportedOutputPixelFormats = {PixelFormat::YCBCR420};

    CodecCapabilities h265;
    h265.codec = Codec::H265_HEVC;
    h265.maxFrameRate = 60;
    h265.supportedOutputPixelFormats = {PixelFormat::YCBCR420};

    CodecCapabilities vp9;
    vp9.codec = Codec::VP9;
    vp9.maxFrameRate = 60;
    vp9.supportedOutputPixelFormats = {PixelFormat::YCBCR420};

    CodecCapabilities av1;
    av1.codec = Codec::AV1;
    av1.maxFrameRate = 60;
    av1.supportedOutputPixelFormats = {PixelFormat::YCBCR420};

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
        Colorimetry bt709Sdr;
        bt709Sdr.range = ColorRange::LIMITED;
        bt709Sdr.matrix = ColorMatrix::BT709;
        bt709Sdr.transfer = TransferCharacteristics::BT709;
        bt709Sdr.primaries = ColorPrimaries::BT709;
        Colorimetry hdr10;
        hdr10.range = ColorRange::LIMITED;
        hdr10.matrix = ColorMatrix::BT2020_NCL;
        hdr10.transfer = TransferCharacteristics::SMPTE2084_PQ;
        hdr10.primaries = ColorPrimaries::BT2020;
        capabilities.supportedColorimetries = {bt709Sdr, hdr10};
    } else if (mId.value == 1){
        h265.maxFrameWidth = 1920;
        h265.maxFrameHeight = 1088;
        vp9.maxFrameWidth = 1920;
        vp9.maxFrameHeight = 1088;
        av1.maxFrameWidth = 1920;
        av1.maxFrameHeight = 1088;
        capabilities.supportedDynamicRanges = {
            DynamicRange::SDR
        };
        Colorimetry bt709Sdr;
        bt709Sdr.range = ColorRange::LIMITED;
        bt709Sdr.matrix = ColorMatrix::BT709;
        bt709Sdr.transfer = TransferCharacteristics::BT709;
        bt709Sdr.primaries = ColorPrimaries::BT709;
        capabilities.supportedColorimetries = {bt709Sdr};
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

/* rdk-halif-aidl 0.23.0: the operational mode is selected by the vendor (this mock always uses
 * NON_TUNNELLED) and is read-only. It can only be queried in the STARTED state. */
android::binder::Status VideoDecoder::getCurrentOperationalMode(OperationalMode* _aidl_return) {
    if (getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    *_aidl_return = static_cast<OperationalMode>(mOperationalMode);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::open(videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, android::sp<videodecoder::IVideoDecoderController>* _aidl_return) {
    if (std::find(mSupportedCodecs.begin(), mSupportedCodecs.end(), codec) == mSupportedCodecs.end()) {
        /* Recognized-but-unsupported codec: gracefully decline per spec (null controller, EX_NONE), not EX_ILLEGAL_ARGUMENT */
        *_aidl_return = nullptr;
        return android::binder::Status::ok();
    }
    if (codec == Codec::MPEG2_VIDEO && !secure) { // MPEG2 with secure:false is an unsupported combination, not an invalid argument
        LOG(DEBUG) << "open: MPEG codec + secure:false is an unsupported combination, declining gracefully";
        *_aidl_return = nullptr;
        return android::binder::Status::ok();
    }
    if (videoDecoderControllerListener == nullptr) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
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
    /* Metrics reset on open() per Property.aidl */
    mMetricsFrameDecoded = 0;
    mMetricsDecodeErrors = 0;
    mMetricsFramesDropped = 0;
    /* DECODE_ERROR_POLICY defaults to 0 on open() per Property.aidl */
    mDecodeErrorPolicy = 0;
    mDecodeErrorPolicySet = false;
    *_aidl_return = new VideoDecoderController(mId, this, videoDecoderControllerListener, secure);
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::openWithResolution(videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, int32_t maxWidth, int32_t maxHeight, android::sp<videodecoder::IVideoDecoderController>* _aidl_return) {
    if (std::find(mSupportedCodecs.begin(), mSupportedCodecs.end(), codec) == mSupportedCodecs.end()) {
        /* Recognized-but-unsupported codec: gracefully decline per spec (null controller, EX_NONE), not EX_ILLEGAL_ARGUMENT */
        *_aidl_return = nullptr;
        return android::binder::Status::ok();
    }
    if (codec == Codec::MPEG2_VIDEO && !secure) { // MPEG2 with secure:false is an unsupported combination, not an invalid argument
        LOG(DEBUG) << "openWithResolution: MPEG codec + secure:false is an unsupported combination, declining gracefully";
        *_aidl_return = nullptr;
        return android::binder::Status::ok();
    }
    if (videoDecoderControllerListener == nullptr) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }
    if (maxWidth <= 0 || maxHeight <= 0) {
        *_aidl_return = nullptr;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    /* Check maxWidth/maxHeight against per-codec capability limits (codec support already confirmed above) */
    Capabilities capabilities;
    getCapabilities(&capabilities);
    for (const auto& codecCap : capabilities.supportedCodecs) {
        if (codecCap.codec == codec) {
            if (maxWidth > codecCap.maxFrameWidth || maxHeight > codecCap.maxFrameHeight) {
                *_aidl_return = nullptr;
                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
            }
            break;
        }
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
    /* Metrics reset on open() per Property.aidl */
    mMetricsFrameDecoded = 0;
    mMetricsDecodeErrors = 0;
    mMetricsFramesDropped = 0;
    /* DECODE_ERROR_POLICY defaults to 0 on open() per Property.aidl */
    mDecodeErrorPolicy = 0;
    mDecodeErrorPolicySet = false;
    *_aidl_return = new VideoDecoderController(mId, this, videoDecoderControllerListener, secure);
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
    /* Destroy the output pool synchronously - ~VideoDecoderController() alone would race
     * the client's post-close() heap-metrics check (its timing depends on remote binder release). */
    VideoDecoderController* controller = dynamic_cast<VideoDecoderController*>(videoDecoderController.get());
    if (controller != nullptr) {
        controller->releaseOutputPool();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (mEventListener) {
        mVideoDecoderEventListener->onStateChanged(State::CLOSING, State::CLOSED);
    }
    setState(State::CLOSED);
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoder::registerEventListener(const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return) {
    if (videoDecoderEventListener == nullptr) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }
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
    if (videoDecoderEventListener == nullptr) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }
    if (!mEventListener) {
        *_aidl_return = false;
        return android::binder::Status::ok();
    }
    *_aidl_return = true;
    mEventListener = false;
    mVideoDecoderEventListener = nullptr;
    return android::binder::Status::ok();
}

VideoDecoderController::VideoDecoderController(IVideoDecoder::Id id, VideoDecoder* parentDecoder,
                                               const android::sp<videodecoder::IVideoDecoderControllerListener>& controllerListener,
                                               bool secure)
    : mId(id), mParentDecoder(parentDecoder), mControllerListener(controllerListener), mSecure(secure),
      mEosSignaled(false), mBuffersSubmitted(0),
      mDiscontinuityPending(false), mMetadataChanged(false), mFirstFrameAfterStart(false),
      mColorimetry(videodecoder::Colorimetry{}),
      mColorDepth(8), mDynamicRange(DynamicRange::SDR),
      mQueueCount(0), mFrameCounter(0), mOutputPoolValid(false) {
    mOutputPoolSpaceListener = new OutputPoolSpaceListener();
    android::sp<android::IBinder> binder = android::defaultServiceManager()->getService(
        android::String16(::com::rdk::hal::avbuffer::IAVBuffer::serviceName().c_str()));
    mAVBuffer = android::interface_cast<::com::rdk::hal::avbuffer::IAVBuffer>(binder);
    printf("VideoDecoderController service initialized for ID: %d\n", id.value);
}

VideoDecoderController::~VideoDecoderController() {
    releaseOutputPool();
    printf("VideoDecoderController service destroyed for ID: %d\n", mId.value);
}

void VideoDecoderController::releaseOutputPool() {
    if (mAVBuffer != nullptr && mOutputPoolValid) {
        bool destroyResult = false;
        android::binder::Status destroyStatus = mAVBuffer->destroyPool(mOutputPool, &destroyResult);
        if (!destroyStatus.isOk() || !destroyResult) {
            LOG(ERROR) << "destroyPool for internal output pool failed at session end: "
                       << "exceptionCode=" << destroyStatus.exceptionCode()
                       << " destroyResult=" << destroyResult;
        }
        mOutputPoolValid = false;
    }
}

void VideoDecoderController::ensureOutputPool() {
    if (mAVBuffer != nullptr && !mOutputPoolValid) {
        ::com::rdk::hal::avbuffer::Pool pool;
        android::binder::Status poolStatus = mAVBuffer->createVideoPool(mSecure, mId, mOutputPoolSpaceListener, &pool);
        if (poolStatus.isOk()) {
            mOutputPool = pool;
            mOutputPoolValid = true;
        } else {
            LOG(ERROR) << "createVideoPool for internal output pool failed: exceptionCode=" << poolStatus.exceptionCode();
        }
    }
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

    mFirstFrameAfterStart = true;
    mMetadataChanged = true;
    mQueueCount = 0;

    ensureOutputPool();
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
    {
        std::lock_guard<std::mutex> lock(mEosMutex);
        mEosSignaled = false;
        mBuffersSubmitted = 0;
    }
    mQueueCount = 0;
    /* Metrics reset on stop() per Property.aidl. mFrameCounter deliberately NOT reset -
     * keeping it monotonic keeps the error-injection pattern independent of start/stop/flush counts. */
    mParentDecoder->mMetricsFrameDecoded = 0;
    mParentDecoder->mMetricsDecodeErrors = 0;
    mParentDecoder->mMetricsFramesDropped = 0;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::validateDecodeBufferArgs(int64_t bufferHandle,
        const ::com::rdk::hal::videodecoder::InputBufferMetadata& metadata) {
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    /* Per spec, decodeBufferWithMetadata() throws EX_ILLEGAL_STATE if called after signalEndOfStream() */
    {
        std::lock_guard<std::mutex> lock(mEosMutex);
        if (mEosSignaled) {
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
        }
    }
    if (bufferHandle < 0) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    if (metadata.nsPresentationTime < 0) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    return android::binder::Status::ok();
}

bool VideoDecoderController::tryAcceptBufferIntoQueue() {
    /* Backpressure: bounded input queue, matching the audio mock's pattern */
    if (mQueueCount >= kQueueDepth) {
        android::sp<videodecoder::IVideoDecoderControllerListener> listener = mControllerListener;
        std::thread([listener]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (listener) {
                listener->onDecodeBufferAvailable();
            }
        }).detach();
        return false;
    }
    mQueueCount++;
    {
        std::lock_guard<std::mutex> lock(mEosMutex);
        mBuffersSubmitted++;
    }
    return true;
}

void VideoDecoderController::applyMetadataOverrides(const ::com::rdk::hal::videodecoder::InputBufferMetadata& metadata) {
    /* Per-buffer overrides; a field already locked via setStreamConfig() wins (bitstream-derived). */
    if (metadata.colorimetry.has_value() && mColorimetry == videodecoder::Colorimetry{}) {
        mColorimetry = metadata.colorimetry.value();
        mMetadataChanged = true;
    }
    if (metadata.masteringDisplayInfo.has_value() && !mMasteringDisplayInfo.has_value()) {
        mMasteringDisplayInfo = metadata.masteringDisplayInfo;
        mMetadataChanged = true;
    }
    if (metadata.contentLightLevel.has_value() && !mContentLightLevel.has_value()) {
        mContentLightLevel = metadata.contentLightLevel;
        mMetadataChanged = true;
    }
    if (metadata.dolbyVisionLayerFlags.has_value() && !mDolbyVisionLayerFlags.has_value()) {
        mDolbyVisionLayerFlags = metadata.dolbyVisionLayerFlags;
        mMetadataChanged = true;
    }
    if (metadata.pixelAspectRatio.has_value() && !mPixelAspectRatio.has_value()) {
        mPixelAspectRatio = metadata.pixelAspectRatio;
        mMetadataChanged = true;
    }
}

bool VideoDecoderController::injectDecodeErrorIfDue() {
    /* Only simulate errors for sessions that opted in via setProperty(DECODE_ERROR_POLICY, ...).
     * Without this gate, every session (including ones with no configured errors) would hit
     * the periodic injection below. */
    if (!mParentDecoder->mDecodeErrorPolicySet) {
        return false;
    }
    /* Deterministic decode-error simulation: every Kth accepted frame this session is corrupt.
     * No bitstream parsing - content-based detection can't reliably tell genuinely truncated
     * frames apart from legitimately tiny NALs (see plan doc for rationale). */
    mFrameCounter++;
    if (mFrameCounter % kErrorEveryNth != 0) {
        return false;
    }

    mParentDecoder->mMetricsDecodeErrors++;
    android::sp<videodecoder::IVideoDecoderEventListener> eventListener = mParentDecoder->mVideoDecoderEventListener;
    bool hasEventListener = mParentDecoder->mEventListener;
    std::thread([eventListener, hasEventListener]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (hasEventListener && eventListener) {
            eventListener->onDecodeError(ErrorCode::xxxx, 0);
        }
    }).detach();

    /* Policy 1 conceals (caller returns early without emitting a frame); default policy 0
     * falls through and outputs the corrupt frame anyway. */
    return (mParentDecoder->mDecodeErrorPolicy == 1);
}

int64_t VideoDecoderController::allocateOutputBufferIfNeeded() {
    /* Tunnelled mode always reports -1 per spec */
    bool tunnelled = (mParentDecoder->mOperationalMode == static_cast<int32_t>(OperationalMode::TUNNELLED));
    if (tunnelled) {
        return -1;
    }
    ensureOutputPool();
    if (!mOutputPoolValid) {
        return -1;
    }
    int32_t width = (mResolution.has_value() && mResolution->width > 0) ? mResolution->width : 1920;
    int32_t height = (mResolution.has_value() && mResolution->height > 0) ? mResolution->height : 1080;
    int32_t bytes = width * height * 3 / 2; /* approx YUV 4:2:0 */
    int64_t handle = -1;
    android::binder::Status allocStatus = mAVBuffer->alloc(mOutputPool, bytes, &handle);
    if (allocStatus.isOk() && handle >= 0) {
        return handle;
    }
    return -1;
}

android::binder::Status VideoDecoderController::decodeBufferWithMetadata(int64_t bufferHandle, const ::com::rdk::hal::videodecoder::InputBufferMetadata& metadata, bool* _aidl_return) {
    android::binder::Status validation = validateDecodeBufferArgs(bufferHandle, metadata);
    if (!validation.isOk()) {
        *_aidl_return = false;
        return validation;
    }

    if (!tryAcceptBufferIntoQueue()) {
        *_aidl_return = false;
        return android::binder::Status::ok();
    }
    *_aidl_return = true;

    applyMetadataOverrides(metadata);

    /* Input buffer ownership transfers to the decoder; free it here */
    if (mAVBuffer != nullptr) {
        bool freeResult = false;
        mAVBuffer->free(bufferHandle, &freeResult);
    }

    int64_t pts = metadata.nsPresentationTime;

    if (injectDecodeErrorIfDue()) {
        /* Concealed: no output. Don't consume mDiscontinuityPending here - leave it pending
         * so it carries forward to the next frame that's actually emitted. */
        mQueueCount--;
        return android::binder::Status::ok();
    }

    /* Consume the pending discontinuity flag - only once we know this frame will actually
     * be emitted (a concealed frame above returns early without reaching here). */
    bool isDiscontinuous = mDiscontinuityPending;
    if (isDiscontinuous) {
        mDiscontinuityPending = false;
        mMetadataChanged = true;
    }

    mParentDecoder->mMetricsFrameDecoded++;

    int64_t outputBufferHandle = allocateOutputBufferIfNeeded();

    bool sendFullMetadata = mMetadataChanged || mFirstFrameAfterStart;
    fireOnFrameOutputCallback(pts, outputBufferHandle, sendFullMetadata, isDiscontinuous);
    mFirstFrameAfterStart = false;
    mMetadataChanged = false;

    return android::binder::Status::ok();
}

namespace {

// Minimal self-contained SHA-1 (RFC 3174) - avoids adding a new build dependency (e.g. OpenSSL)
// to the mockservice just to produce a well-formed 20-byte digest for SHA1_CALC. The mock never
// renders real pixel content, so this hashes a deterministic per-frame byte sequence instead of
// actual frame data.
std::vector<uint8_t> computeSha1(const uint8_t* data, size_t len) {
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;

    std::vector<uint8_t> msg(data, data + len);
    uint64_t bitLen = static_cast<uint64_t>(len) * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 80; ++i) {
            uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (v << 1) | (v >> 31);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::vector<uint8_t> digest(20);
    uint32_t hs[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        digest[i * 4]     = static_cast<uint8_t>((hs[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<uint8_t>((hs[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<uint8_t>((hs[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<uint8_t>(hs[i] & 0xFF);
    }
    return digest;
}

}  // namespace

void VideoDecoderController::fireOnFrameOutputCallback(int64_t nsPresentationTime, int64_t outputBufferHandle,
                                                       bool sendFullMetadata, bool discontinuity) {
    android::sp<videodecoder::IVideoDecoderControllerListener> listener = mControllerListener;

    std::optional<FrameMetadata> metadataOpt;
    if (sendFullMetadata) {
        FrameMetadata fm;
        fm.codedWidth = mResolution.has_value() ? mResolution->width : 0;
        fm.codedHeight = mResolution.has_value() ? mResolution->height : 0;
        fm.activeWidth = fm.codedWidth;
        fm.activeHeight = fm.codedHeight;
        /* Bitstream-derived only per FrameMetadata.aidl - neither InputBufferMetadata nor
         * VideoDecoderStreamConfig has an override field for colorDepth/dynamicRange, so these
         * must stay fixed regardless of any colorimetry override applied via decodeBufferWithMetadata(). */
        fm.colorDepth = mColorDepth;
        fm.dynamicRange = mDynamicRange;
        fm.pixelFormat = PixelFormat::YCBCR420;
        fm.scanType = ScanType::PROGRESSIVE_FRAME;
        fm.frameRateNumerator = mFrameRate.has_value() ? mFrameRate->numerator : 0;
        fm.frameRateDenominator = mFrameRate.has_value() ? mFrameRate->denominator : 0;
        fm.bitstreamEOS = false;
        fm.discontinuity = discontinuity;
        fm.lowLatency = (mParentDecoder->mLowLatency == 1);
        fm.colorimetry = mColorimetry;
        fm.masteringDisplayInfo = mMasteringDisplayInfo;
        fm.contentLightLevel = mContentLightLevel;
        fm.source = mParentDecoder->mAVSource;
        if (mPixelAspectRatio.has_value()) {
            fm.parX = mPixelAspectRatio->numerator;
            fm.parY = mPixelAspectRatio->denominator;
        }
        if (mParentDecoder->mSha1Calc == 1) {
            std::string seed = std::to_string(nsPresentationTime) + ":" +
                                std::to_string(fm.codedWidth) + "x" + std::to_string(fm.codedHeight);
            fm.sha1 = computeSha1(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
        }
        metadataOpt.emplace(std::move(fm));
    }

    // Keep controller alive until the detached thread below finishes.
    android::sp<VideoDecoderController> self(this);
    std::thread([self, listener, nsPresentationTime, outputBufferHandle, metadataOpt]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        /* Frame processing complete - release its queue slot */
        self->mQueueCount--;
        if (listener) {
            listener->onFrameOutput(nsPresentationTime, outputBufferHandle, metadataOpt);
        }
    }).detach();
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
    {
        std::lock_guard<std::mutex> lock(mEosMutex);
        mEosSignaled = false;
        mBuffersSubmitted = 0;
    }
    mQueueCount = 0;
    /* Metrics reset on flush() per Property.aidl, regardless of the reset flag.
     * mFrameCounter deliberately NOT reset here - see stop() for rationale. */
    mParentDecoder->mMetricsFrameDecoded = 0;
    mParentDecoder->mMetricsDecodeErrors = 0;
    mParentDecoder->mMetricsFramesDropped = 0;
    if (!reset) {
        /* Still STARTED: per spec, metadata must be non-null again on the first frame after flush() */
        mFirstFrameAfterStart = true;
        mMetadataChanged = true;
    }
    /* Note: colorimetry/masteringDisplayInfo/contentLightLevel/dolbyVisionLayerFlags/pixelAspectRatio
     * hints deliberately persist across flush() per VideoDecoderStreamConfig's documented semantics -
     * cleared only on close() (i.e. when a fresh VideoDecoderController is constructed on next open()). */
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::signalEndOfStream() {
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    {
        std::lock_guard<std::mutex> lock(mEosMutex);
        /* Reject if no buffers have ever been submitted on this session */
        if (mBuffersSubmitted == 0) {
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
        }
        /* Second call is a no-op */
        if (mEosSignaled) {
            return android::binder::Status::ok();
        }
        mEosSignaled = true;
    }
    android::sp<videodecoder::IVideoDecoderControllerListener> listener = mControllerListener;
    std::thread([listener]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (listener) {
            listener->onEndOfStream();
        }
    }).detach();
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::signalDiscontinuity() {
    if (mParentDecoder->getState() != State::STARTED) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    mDiscontinuityPending = true;
    return android::binder::Status::ok();
}

namespace {

// Per-format minimum-size and structure check for parseCodecSpecificData(). Returns false
// (having logged the reason) for an unrecognised format or a payload that fails its check.
bool isValidVideoCodecSpecificData(rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat,
                                    const std::vector<uint8_t>& codecData) {
    switch (csdVideoFormat) {
        case rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD:
            // AVCDecoderConfigurationRecord (H.264): version/profile/compat/level/length-size (5 bytes)
            // + numSPS/SPSlen/SPSdata (>=4) + numPPS/PPSlen/PPSdata (>=4) = 13 bytes minimum.
            if (codecData.size() < 13) {
                LOG(ERROR) << "parseCodecSpecificData: codecData too small for H.264 (" << codecData.size() << " bytes, expected at least 13)";
                return false;
            }
            if (codecData[0] != 1) {
                LOG(ERROR) << "parseCodecSpecificData: Invalid AVCDecoderConfigurationRecord version: " << static_cast<int>(codecData[0]);
                return false;
            }
            return true;

        case rdk::hal::videodecoder::CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD:
            // HEVCDecoderConfigurationRecord (H.265): version(1)/profile-space-tier-profile(5)/
            // constraint-flags(6)/level(1)/min-spatial-seg(2)/chroma(1) = 14 bytes minimum
            // (without VPS/SPS/PPS NAL units).
            if (codecData.size() < 14) {
                LOG(ERROR) << "parseCodecSpecificData: codecData too small for H.265 (" << codecData.size() << " bytes, expected at least 14)";
                return false;
            }
            if (codecData[0] != 1) {
                LOG(ERROR) << "parseCodecSpecificData: Invalid HEVCDecoderConfigurationRecord version: " << static_cast<int>(codecData[0]);
                return false;
            }
            return true;

        case rdk::hal::videodecoder::CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD:
            // AV1DecoderConfigurationRecord: marker/version/seq-profile(1)/seq-level-idx0(1)/
            // seq-tier0(1)/reserved(1) = 4 bytes minimum.
            if (codecData.size() < 4) {
                LOG(ERROR) << "parseCodecSpecificData: codecData too small for AV1 (" << codecData.size() << " bytes, expected at least 4)";
                return false;
            }
            if ((codecData[0] & 0x80) != 0x80) {  // marker bit
                LOG(ERROR) << "parseCodecSpecificData: Invalid AV1DecoderConfigurationRecord marker: " << static_cast<int>(codecData[0]);
                return false;
            }
            return true;

        default:
            LOG(ERROR) << "parseCodecSpecificData: Invalid csdVideoFormat " << static_cast<int32_t>(csdVideoFormat);
            return false;
    }
}

}  // namespace

android::binder::Status VideoDecoderController::parseCodecSpecificData(rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return) {
    if (!mParentDecoder) {
        LOG(ERROR) << "parseCodecSpecificData: mParentDecoder is null";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    // Require STARTED per test expectation.
    State currentState = mParentDecoder->getState();
    if (currentState != State::STARTED) {
        LOG(ERROR) << "parseCodecSpecificData: Invalid state: " << static_cast<int32_t>(currentState) << ", expected STARTED";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (codecData.empty()) {
        LOG(ERROR) << "parseCodecSpecificData: codecData is empty";
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    if (!isValidVideoCodecSpecificData(csdVideoFormat, codecData)) {
        *_aidl_return = false;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    LOG(DEBUG) << "parseCodecSpecificData: Successfully parsed codecData, size: " << codecData.size();
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status VideoDecoderController::setStreamConfig(const videodecoder::VideoDecoderStreamConfig& config) {
    if (mParentDecoder->getState() != State::READY) {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    // Validate every non-null field before applying any of them: per spec, setStreamConfig()
    // is all-or-nothing, so a later field's failure must not leave an earlier field's
    // mutation already applied.
    if (config.resolution.has_value() &&
        (config.resolution->width <= 0 || config.resolution->height <= 0)) {
        LOG(ERROR) << "setStreamConfig: invalid resolution " << config.resolution->width << "x" << config.resolution->height;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    /* denominator must be > 0 unless numerator is also 0 (encodes unknown) */
    if (config.frameRate.has_value() &&
        config.frameRate->denominator <= 0 && config.frameRate->numerator != 0) {
        LOG(ERROR) << "setStreamConfig: invalid frameRate " << config.frameRate->numerator << "/" << config.frameRate->denominator;
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }
    if (config.pixelAspectRatio.has_value()) {
        const int32_t parNum = config.pixelAspectRatio->numerator;
        const int32_t parDen = config.pixelAspectRatio->denominator;
        if (parNum < 0 || parDen < 0) {
            LOG(ERROR) << "setStreamConfig: invalid pixelAspectRatio " << parNum << "/" << parDen;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
        /* The only valid "unknown" encoding is 0/0; exactly one field being 0 is invalid */
        if ((parNum == 0) != (parDen == 0)) {
            LOG(ERROR) << "setStreamConfig: pixelAspectRatio " << parNum << "/" << parDen
                       << " has exactly one zero field; only 0/0 (unknown) is valid";
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
    }

    // All non-null fields validated - apply them as a single atomic update.
    if (config.resolution.has_value()) {
        mResolution = config.resolution;
        mMetadataChanged = true;
    }
    if (config.frameRate.has_value()) {
        mFrameRate = config.frameRate;
        mMetadataChanged = true;
    }
    if (config.pixelAspectRatio.has_value()) {
        mPixelAspectRatio = config.pixelAspectRatio;
        mMetadataChanged = true;
    }
    if (config.colorimetry.has_value()) {
        mColorimetry = config.colorimetry.value();
        /* colorDepth/dynamicRange are bitstream-derived per FrameMetadata.aidl - the mock has
         * no real content analysis, so approximate this once from the session's configured
         * colorimetry (a reasonable proxy: real HDR10/HLG content is what actually carries
         * these transfer characteristics). Deliberately done only here, not on every frame or
         * on a later per-buffer override in applyMetadataOverrides() - colorDepth/dynamicRange
         * have no override field in InputBufferMetadata, so they must stay fixed for the rest
         * of the session once derived, regardless of any mid-stream colorimetry override. */
        switch (mColorimetry.transfer) {
            case TransferCharacteristics::SMPTE2084_PQ:
                mColorDepth = 10;
                mDynamicRange = DynamicRange::HDR10;
                break;
            case TransferCharacteristics::ARIB_STD_B67_HLG:
                mColorDepth = 10;
                mDynamicRange = DynamicRange::HLG;
                break;
            default:
                mColorDepth = 8;
                mDynamicRange = DynamicRange::SDR;
                break;
        }
        mMetadataChanged = true;
    }
    if (config.masteringDisplayInfo.has_value()) {
        mMasteringDisplayInfo = config.masteringDisplayInfo;
        mMetadataChanged = true;
    }
    if (config.contentLightLevel.has_value()) {
        mContentLightLevel = config.contentLightLevel;
        mMetadataChanged = true;
    }
    if (config.dolbyVisionLayerFlags.has_value()) {
        mDolbyVisionLayerFlags = config.dolbyVisionLayerFlags;
        mMetadataChanged = true;
    }
    LOG(DEBUG) << "setStreamConfig: accepted and stored stream configuration hints";
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
        case Property::RESOURCE_ID:
        case Property::INPUT_QUEUE_DEPTH:
        case Property::OUTPUT_FRAME_POOL_SIZE:
        case Property::SECURE_VIDEO:
        case Property::METRIC_FRAMES_DECODED:
        case Property::METRIC_DECODE_ERRORS:
        case Property::METRIC_FRAMES_DROPPED:
            *_aidl_return = false;
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_UNSUPPORTED_OPERATION);
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
            mParentDecoder->mDecodeErrorPolicySet = true;
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
