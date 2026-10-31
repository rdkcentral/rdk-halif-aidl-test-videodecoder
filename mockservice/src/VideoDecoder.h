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
 * VideoDecoder.h
 *
 * Header file for the VideoDecoder service implementation
 */

#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <com/rdk/hal/PropertyValue.h>
#include <com/rdk/hal/videodecoder/BnVideoDecoderManager.h>
#include <com/rdk/hal/videodecoder/IVideoDecoderManager.h>
#include <com/rdk/hal/videodecoder/BnVideoDecoder.h>
#include <com/rdk/hal/videodecoder/BnVideoDecoderController.h>
#include <com/rdk/hal/videodecoder/InputBufferMetadata.h>
#include <com/rdk/hal/videodecoder/VideoDecoderStreamConfig.h>
#include <com/rdk/hal/videodecoder/FrameMetadata.h>
#include <com/rdk/hal/videodecoder/Resolution.h>
#include <com/rdk/hal/videodecoder/Fraction.h>
#include <com/rdk/hal/videodecoder/Colorimetry.h>
#include <com/rdk/hal/videodecoder/TransferCharacteristics.h>
#include <com/rdk/hal/videodecoder/MasteringDisplayInfo.h>
#include <com/rdk/hal/videodecoder/ContentLightLevel.h>
#include <com/rdk/hal/videodecoder/DolbyVisionLayerFlags.h>
#include <com/rdk/hal/videodecoder/ErrorCode.h>
#include <com/rdk/hal/avbuffer/IAVBuffer.h>
#include <com/rdk/hal/avbuffer/Pool.h>
#include <com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h>
#include <binder/BinderService.h>
#include <optional>
#include <ostream>
#include <string>
#include <utils/StrongPointer.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>

namespace com {
namespace rdk {
namespace hal {
inline std::ostream& operator<<(std::ostream& os, const PropertyValue& value)
{
    os << value.toString();
    return os;
}
} // namespace hal
} // namespace rdk
} // namespace com

namespace com {
namespace rdk {
namespace hal {
namespace videodecoder {

#define NUM_VIDEO_DECODER 2

class VideoDecoderManager : public android::BinderService<VideoDecoderManager>,
                            public BnVideoDecoderManager {
public:
    VideoDecoderManager();
    virtual ~VideoDecoderManager();

    static char const* getServiceName() { return IVideoDecoderManager::serviceName().c_str(); }

    android::binder::Status getVideoDecoderIds(std::vector<IVideoDecoder::Id>* _aidl_return) override;
    android::binder::Status getVideoDecoder(const IVideoDecoder::Id& videoDecoderId, android::sp<IVideoDecoder>* _aidl_return) override;
};

class VideoDecoder : public android::BinderService<VideoDecoder>,
                    public BnVideoDecoder {
public:
    /* Decoder Read-Only Properties */
    IVideoDecoder::Id mId;
    int32_t mSupportsSecure;
    int32_t mMetricsFrameDecoded;
    int32_t mMetricsDecodeErrors;
    int32_t mMetricsFramesDropped;
    int32_t mInputQueueDepth;
    int32_t mOutputFramePoolSize;

    /* Read-Write Properties */
    int32_t mLowLatency;
    AVSource mAVSource;
    int32_t mDecodeErrorPolicy;
    bool mDecodeErrorPolicySet;
    int32_t mSha1Calc;
    int32_t mOperationalMode;

    /* State and Codecs */
    State mVideoDecoderState;
    std::mutex mStateMutex;
    std::vector<Codec> mSupportedCodecs;

    /* Event Listener */
    bool mEventListener;
    android::sp<videodecoder::IVideoDecoderEventListener> mVideoDecoderEventListener;

public:
    VideoDecoder(IVideoDecoder::Id id);
    virtual ~VideoDecoder();

    void setState(State state);
    State getState();

    android::binder::Status getCapabilities(videodecoder::Capabilities* _aidl_return) override;
    android::binder::Status getProperty(videodecoder::Property property, std::optional<PropertyValue>* _aidl_return) override;
    android::binder::Status getPropertyMulti(const std::vector<videodecoder::Property>& properties, std::vector<videodecoder::PropertyKVPair>* propertyKVList, bool* _aidl_return) override;
    android::binder::Status getState(State* _aidl_return) override;
    android::binder::Status getCurrentOperationalMode(::com::rdk::hal::OperationalMode* _aidl_return) override;
    android::binder::Status open(videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, android::sp<videodecoder::IVideoDecoderController>* _aidl_return) override;
    android::binder::Status openWithResolution(videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, int32_t maxWidth, int32_t maxHeight, android::sp<videodecoder::IVideoDecoderController>* _aidl_return) override;
    android::binder::Status close(const android::sp<videodecoder::IVideoDecoderController>& videoDecoderController, bool* _aidl_return) override;
    android::binder::Status registerEventListener(const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return) override;
    android::binder::Status unregisterEventListener(const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return) override;
};

// No-op listener to satisfy createVideoPool()'s non-null requirement
class OutputPoolSpaceListener : public ::com::rdk::hal::avbuffer::BnAVBufferSpaceListener {
public:
    android::binder::Status onSpaceAvailable() override { return android::binder::Status::ok(); }
};

class VideoDecoderController : public android::BinderService<VideoDecoderController>,
                            public BnVideoDecoderController {
private:
    IVideoDecoder::Id mId;
    VideoDecoder* mParentDecoder;
    android::sp<videodecoder::IVideoDecoderControllerListener> mControllerListener;
    bool mSecure;

public:
    VideoDecoderController(IVideoDecoder::Id id, VideoDecoder* parentDecoder,
                           const android::sp<videodecoder::IVideoDecoderControllerListener>& controllerListener = nullptr,
                           bool secure = false);
    virtual ~VideoDecoderController();

    void fireOnFrameOutputCallback(int64_t nsPresentationTime, int64_t outputBufferHandle,
                                   bool sendFullMetadata, bool discontinuity);
    void fireOnEndOfStreamCallback();
    void ensureOutputPool();
    // Synchronously destroys the output pool; called from close() and, as a fallback,
    // from the destructor for sessions that never call close() explicitly. Idempotent.
    void releaseOutputPool();

    android::binder::Status start() override;
    android::binder::Status stop() override;
    android::binder::Status decodeBufferWithMetadata(int64_t bufferHandle, const ::com::rdk::hal::videodecoder::InputBufferMetadata& metadata, bool* _aidl_return) override;
    android::binder::Status flush(bool reset) override;
    android::binder::Status signalDiscontinuity() override;
    android::binder::Status signalEndOfStream() override;
    android::binder::Status parseCodecSpecificData(rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return) override;
    android::binder::Status setProperty(videodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return) override;
    android::binder::Status setStreamConfig(const videodecoder::VideoDecoderStreamConfig& config) override;

private:
    // Helpers for decodeBufferWithMetadata(), one per concern:
    android::binder::Status validateDecodeBufferArgs(int64_t bufferHandle,
        const ::com::rdk::hal::videodecoder::InputBufferMetadata& metadata);
    bool tryAcceptBufferIntoQueue();
    void applyMetadataOverrides(const ::com::rdk::hal::videodecoder::InputBufferMetadata& metadata);
    bool injectDecodeErrorIfDue();
    int64_t allocateOutputBufferIfNeeded();

    bool mEosSignaled;
    std::mutex mEosMutex;
    int32_t mBuffersSubmitted;

    /* Pending discontinuity flag, applied to the next output frame */
    bool mDiscontinuityPending;
    /* Forces a full (non-null) FrameMetadata on the next onFrameOutput() call */
    bool mMetadataChanged;
    bool mFirstFrameAfterStart;

    /* Effective stream-config state: setStreamConfig() hints, overridable per-buffer via InputBufferMetadata */
    std::optional<videodecoder::Resolution> mResolution;
    std::optional<videodecoder::Fraction> mFrameRate;
    videodecoder::Colorimetry mColorimetry;
    /* Bitstream-derived only per FrameMetadata.aidl - neither InputBufferMetadata nor
     * VideoDecoderStreamConfig has an override field for either, so these are fixed for the
     * lifetime of the session and must never be derived from mColorimetry. */
    int32_t mColorDepth;
    videodecoder::DynamicRange mDynamicRange;
    std::optional<videodecoder::MasteringDisplayInfo> mMasteringDisplayInfo;
    std::optional<videodecoder::ContentLightLevel> mContentLightLevel;
    std::optional<videodecoder::DolbyVisionLayerFlags> mDolbyVisionLayerFlags;
    std::optional<videodecoder::Fraction> mPixelAspectRatio;

    /* Outstanding-frame count (atomic: decremented on a background thread) */
    std::atomic<int32_t> mQueueCount;
    static constexpr int32_t kQueueDepth = 4;

    /* Deterministic decode-error simulation: every Kth accepted frame is treated as corrupt */
    int32_t mFrameCounter;
    static constexpr int32_t kErrorEveryNth = 4;

    /* Connection to MockAVBufferService for allocating output frame buffers */
    android::sp<::com::rdk::hal::avbuffer::IAVBuffer> mAVBuffer;
    ::com::rdk::hal::avbuffer::Pool mOutputPool;
    bool mOutputPoolValid;
    android::sp<OutputPoolSpaceListener> mOutputPoolSpaceListener;
};

} // namespace videodecoder
} // namespace hal
} // namespace rdk
} // namespace com

 #endif // VIDEODECODER_H
