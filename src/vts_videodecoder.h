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

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 * @par The Hardware Porting Kit
 * HPK is the next evolution of the well-defined Hardware Abstraction Layer
 * (HAL), but augmented with more comprehensive documentation and test suites
 * that OEM or SOC vendors can use to self-certify their ports before taking
 * them to RDKM for validation or to an operator for final integration and
 * deployment. The Hardware Porting Kit effectively enables an OEM and/or SOC
 * vendor to self-certify their own Video Accelerator devices, with minimal RDKM
 * assistance.
 *
 */

/**
 * @addtogroup Video_Decoder Video Decoder Module
 * @{
 */

/**
 * @addtogroup Video_Decoder_Halif_Test Video Decoder HALIF Tests
 * @{
 */

/**
 * @defgroup Video_Decoder_Halif_Test_Header Video Decoder HALIF Tests Header File
 * @{
 * @parblock
 *
 *
 * **Pre-Conditions:**  None@n
 * **Dependencies:** None@n
 *
 * Ref to API Definition specification documentation :
 *
 * @endparblock
 */


/**
 * @file vts_videodecoder.h
 *
 */

#ifndef __VTS_VIDEODECODER_H__
#define __VTS_VIDEODECODER_H__

#include <ut.h>
#include <ut_kvp_profile.h>
#include <ut_log.h>
#include <ut_control_plane.h>
#include <deque>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <deque>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <unistd.h>
#include <cstdlib>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory> // For std::unique_ptr
#include <inttypes.h>

#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"
//#include "com/rdk/hal/avbufferhelper.h"
#include "avbufferhelper.h"

#include "vts_avbuffer.h"
#include "vts_helper_functions.h"

#include "com/rdk/hal/videodecoder/IVideoDecoderManager.h"
#include "com/rdk/hal/videodecoder/IVideoDecoderController.h"
#include "com/rdk/hal/videodecoder/IVideoDecoderEventListener.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderControllerListener.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderEventListener.h"
#include "com/rdk/hal/videodecoder/DynamicRange.h"
#include "com/rdk/hal/videodecoder/Codec.h"
#include "com/rdk/hal/videodecoder/CodecCapabilities.h"
#include "com/rdk/hal/videodecoder/CodecProfile.h"
#include "com/rdk/hal/videodecoder/CodecLevel.h"
#include "com/rdk/hal/videodecoder/Profile.h"
#include "com/rdk/hal/videodecoder/ColorRange.h"
#include "com/rdk/hal/videodecoder/ColorMatrix.h"
#include "com/rdk/hal/videodecoder/TransferCharacteristics.h"
#include "com/rdk/hal/videodecoder/ColorPrimaries.h"
#include "com/rdk/hal/OperationalMode.h"   /* moved to common in rdk-halif-aidl 0.23.0 */
#include "com/rdk/hal/videodecoder/Colorimetry.h"
#include "com/rdk/hal/videodecoder/MasteringDisplayInfo.h"
#include "com/rdk/hal/videodecoder/ContentLightLevel.h"
#include "com/rdk/hal/videodecoder/InputBufferMetadata.h"
#include "com/rdk/hal/videodecoder/VideoDecoderStreamConfig.h"

#include "vts_helper_functions.h"

#define VIDEODECODERMGR "videodecoder"
#define VIDEODECODERMGR_IVIDEODECODER "videodecoder.IVideoDecoder"

using namespace com::rdk::hal;
using namespace com::rdk::hal::videodecoder;
using namespace com::rdk::hal::avbuffer;
using ::com::rdk::hal::videodecoder::State;

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

static constexpr int VIDEO_MAX_TRANSITORY_STATE_WAIT_MS = 10;
static constexpr int VIDEO_MAX_FINAL_STATE_WAIT_MS = 500;

// Global test data (consider moving to a singleton or test fixture if shared state is needed)

extern sp<IVideoDecoderManager>                  gVideoDecoderManager;
extern std::vector<IVideoDecoder::Id>            decoderIds;
extern std::vector<sp<IVideoDecoderController>>  controllers;


// Detects MockVideoDecoderService via pgrep (like videodecoder/run.sh), since the
// shared top-level run.sh never sets VIDEO_DECODER_L3_CONFIG_PATH for any HAL component.
inline bool isMockVideoDecoderServiceRunning()
{
    // Bracket trick avoids self-matching the "sh -c <this pgrep>" wrapper std::system() spawns.
    int status = std::system("pgrep -f [M]ockVideoDecoderService > /dev/null 2>&1");
    if (status == -1)
    {
        // Child process could not be created/waited on; status is not a wait status here.
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// Lets run.sh point L3 tests at a mock-only config; falls back to pgrep auto-detection if unset.
inline const char* getVideoDecoderTestConfigPath()
{
    const char* envPath = getenv("VIDEO_DECODER_L3_CONFIG_PATH");
    if (envPath != nullptr)
    {
        return envPath;
    }
    if (isMockVideoDecoderServiceRunning())
    {
        // Consolidated mock config (test defs + all per-codec stream data in one file);
        // source lives at mockservice/config/L3_stream_mock.yaml, installed alongside the
        // real config by install_test() so this relative path resolves the same way.
        return "./videodecoder_test_configuration/L3_stream_mock.yaml";
    }
    return "./videodecoder_test_configuration/vts_l3_VideoDecoder_test_config.yaml";
}
// vts_kvp_create_instance() takes char* (not const char*); a raw string literal
// gets an implicit (deprecated) conversion to char*, but a function-returned
// const char* does not, so cast explicitly here instead of relaxing the callee.
#define VIDEO_DECODER_TEST_CONFIG_PATH const_cast<char*>(getVideoDecoderTestConfigPath())
#define VIDEO_DECODER_STREAM_CONFIG_PATH "./videodecoder_test_configuration/vts_stream_list.yaml"
#define VIDEO_DECODER_STREAM_DOWNLOAD_PATH "videodecoder.stream_download_path"

extern void *gVideoDecoderTestConfiguration;

typedef struct InputVideoFrame
{
    int64_t framePTS;
    std::string reference;
    double rms_threshold;
    bool isErrorFrame;
    std::chrono::steady_clock::time_point sendTime;
}InputVideoFrame_t;

struct CodecCapability {
    videodecoder::Codec codec;
    std::vector<videodecoder::Profile> profiles;
    int32_t maxFrameRate;
    int32_t maxFrameWidth;
    int32_t maxFrameHeight;
    std::vector<PixelFormat> supportedOutputPixelFormats;
};

typedef struct decoder {
    int32_t id;
    std::vector<CodecCapability> supportedCodecs;
    std::vector<DynamicRange> supportedDynamicRanges;
    std::vector<Colorimetry> supportedColorimetries;
    // DEPRECATED (rdk-halif-aidl 0.23.0): the HFP no longer declares operational modes and this list is
    // no longer parsed (always empty). Kept only so modules not yet migrated still compile; read the
    // mode after start() instead (getCurrentOperationalMode() / videoDecoderTest::waitForOperationalMode()).
    // Remove once all modules are migrated.
    std::vector<OperationalMode> supportedOperationalModes;
    bool supportsSecure;
}decoder_t;

extern std::vector<decoder_t> gVideodecoders_hfp;

struct videoFrameMetadata {
        PixelFormat pixelFormat;
        int32_t codedWidth;
        int32_t codedHeight;
        int colorDepth;
        videodecoder::Codec sourceCodec;
        int frameRateNumerator;
	    int frameRateDenominator;
        DynamicRange dynamicRange;
        Colorimetry colorimetry;
        std::optional<MasteringDisplayInfo> masteringDisplayInfo;
        std::optional<ContentLightLevel> contentLightLevel;
        int bit_rate;
        bool lowLatency;
        bool endOfStream;
        bool discontinuity;
        ScanType scanType;
        AVSource source;
};

struct DecodedVideoFrame_t {
    std::vector<uint8_t> data;
    int64_t framePTS;
    std::chrono::steady_clock::time_point receiveTime;
    bool isMetaAvailable;
    std::unique_ptr<com::rdk::hal::videodecoder::FrameMetadata> metadata;
    bool validationStatus; // Added for validation result
    int64_t frameAVBufferHandle; // Added for output buffer handle
};

// One in-band resolution/bitrate ladder breakpoint: expected width/height/bitRate
// take effect from `pts` onward, until the next entry's pts (or end of stream).
struct ResolutionChange_t {
    int64_t pts;
    int32_t width;
    int32_t height;
    uint32_t bitRate;
};

struct videoDecoderTestConfiguration_t {
    std::string input; // Path to input .es file
    std::string streamName; //
    videodecoder::Codec codec; // H264_AVC, HEVC, etc.
    int32_t codedWidth; // Video width
    int32_t codedHeight; // Video height
    uint32_t frameRateNumerator; // Frame rate numerator
    uint32_t frameRateDenominator; // Frame rate denominator
    videodecoder::PixelFormat pixelFormat; // YUV, etc.
    videodecoder::DynamicRange dynamicRange; // SDR, HDR, etc.
    videodecoder::Colorimetry colorimetry;
    std::optional<MasteringDisplayInfo> masteringDisplayInfo;
    std::optional<ContentLightLevel> contentLightLevel;
    std::optional<videodecoder::Fraction> pixelAspectRatio; // set only if bitstream signals PAR
    videodecoder::ScanType scanType;
    uint32_t colorDepth; // Bit depth
    uint32_t bit_rate; // Bit rate
    uint32_t no_of_frames; // Total frames
    uint32_t decodeErrors;
    uint32_t frames_to_discard;
    uint32_t framesDropped;
    std::vector<std::tuple<int32_t, int64_t, bool>> frameDetails; // {size, PTS, isErrorFrame}
    std::vector<int64_t> expectedPts; // Added for PTS validation
    double rms_diff_threshold; // RMS difference threshold
    uint32_t decoderDelay; // Delay between decode calls (ms)
    int64_t stopPts; // Stop/restart PTS
    int64_t discontinuityPts; // Discontinuity PTS
    int64_t flushPts; // Flush PTS
    int64_t flushResetPts; // Flush with reset PTS
    int64_t eosPts; // EOS PTS
    int64_t decode_time_threshold; // Decode time budget per frame (ns)
    double rms_diff_threashold; // RMS difference threshold
    bool endOfStream;
    bool discontinuity;
    bool lowLatency;
    bool blPresent;
    bool elPresent;

    // Mid-stream metadata override via InputBufferMetadata nullable fields (-1 = disabled)
    int64_t metadataOverridePts;
    std::optional<videodecoder::Colorimetry> overrideColorimetry;
    std::optional<MasteringDisplayInfo> overrideMasteringDisplayInfo;
    std::optional<ContentLightLevel> overrideContentLightLevel;
    std::optional<DolbyVisionLayerFlags> overrideDolbyVisionLayerFlags;
    std::optional<videodecoder::Fraction> overridePixelAspectRatio;

    // In-band resolution/bitrate ladder (AdaptiveBitrate streams); non-empty makes
    // decode_time_threshold a hard failure instead of a warning.
    std::vector<ResolutionChange_t> resolutionChanges;

    // When != -1, open() is replaced by openWithResolution(codec, secure, listener, openWithResolutionWidth, openWithResolutionHeight)
    int32_t openWithResolutionWidth;  // -1 = use open(), >0 = use openWithResolution()
    int32_t openWithResolutionHeight; // -1 = use open(), >0 = use openWithResolution()

    // Sink control fields
    bool    sink_backpressure;
    int64_t sink_discardFramesUntilPts;
    int64_t sink_underflow_pts; //sink specific
};


UT_status_t getVideoDecoderService();

DynamicRange videoDecoderStringToDynamicRange(const std::string& rangeStr);
PixelFormat videoDecoderStringToPixelFormat(const std::string& formatStr);
videodecoder::CodecProfile videoDecoderStringToCodecProfile(const std::string& profileStr);
videodecoder::CodecLevel videoDecoderStringToCodecLevel(const std::string& levelStr);
videodecoder::Profile videoDecoderStringToProfile(const std::string& profileStr, const std::string& levelStr, int64_t maxBitrateInBps = 0);
videodecoder::Colorimetry videoDecoderStringToColorimetry(const std::string& rangeStr, const std::string& matrixStr, const std::string& transferStr, const std::string& primariesStr);
videodecoder::Colorimetry videoDecoderColorimetryPresetToColorimetry(const std::string& presetStr);
bool vts_videodecoder_helper_validatePropertyValue(int decoder_id,const std::optional<PropertyValue>& propertyValue, const videodecoder::Property property);
void vts_videodecoder_helper_validateCapabilities(int32_t decoderId,const videodecoder::Capabilities& capabilities);
bool vts_videodecoder_helper_checkCodecAttributeExists(const std::string& attrpath, const std::string& codec_name);
bool vts_videodecoder_helper_profileFieldPresent(const std::string& key);
double vts_computeVideoRMSDiff(const uint8_t* ref, const uint8_t* dec, size_t width, size_t height, PixelFormat format);

UT_status_t vts_videodecoder_parse_testconfig(void* pTestInstance, const char *module,
                                              const char *testGroup, const char *testName,
                                              std::vector<videoDecoderTestConfiguration_t>& testConfiguration);

void vts_videodecoder_get_available_id(std::vector<int32_t> &availableIds);
UT_status_t vts_videodecoder_parseVideoDecoderHFP();

size_t vts_bytesPerPixel(PixelFormat format);

bool checkDecoderSupportsConfig(const IVideoDecoder::Id& decoderId, const videodecoder::Capabilities* capabilities,
                                const videoDecoderTestConfiguration_t& testConfig, bool secureDecode);


#define VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoderIds(OUT:decoderIds:[])"); \
        status = gVideoDecoderManager->getVideoDecoderIds(&decoderIds); \
        UT_LOG_INFO("Result getVideoDecoderIds(OUT:decoderIds:[size: %d]), status:[isOK:%s, exception:%s]", \
                                decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_FALSE_FATAL(decoderIds.empty()); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[])", _decoderId.value); \
        status = gVideoDecoderManager->getVideoDecoder(_decoderId, &videoDecoder); \
        UT_LOG_INFO("Result getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(videoDecoder.get() != nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

/**
 * @brief Calls getVideoDecoder with a valid ID and verifies success.
 * @param[in] gVideoDecoderManager The IVideoDecoderManager instance (assumed available in scope).
 * @param[in] id The IVideoDecoder::Id to query.
 * @param[out] decoder The returned IVideoDecoder instance.
 */
#define VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoder(IN:id:[%d], OUT:videoDecoder[])", (_decoderId).value); \
        status = gVideoDecoderManager->getVideoDecoder(_decoderId, &videoDecoder); \
        UT_LOG_INFO("Result getVideoDecoder(IN:id:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                    (_decoderId).value, (videoDecoder).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "Failed to call getVideoDecoder for ID " << (_decoderId).value; \
        UT_ASSERT_TRUE_FATAL((videoDecoder) != nullptr) << "Returned videoDecoder is null for ID " << (_decoderId).value; \
    } while (0)

#define VIDEODECODER_MANAGER_GETVIDEODECODER_FAILURE(_decoderId, videoDecoder) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[])", _decoderId.value); \
        status = gVideoDecoderManager->getVideoDecoder(_decoderId, &videoDecoder); \
        UT_LOG_INFO("Result getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(videoDecoder.get() == nullptr); \
    } while (0)

/**
 * @brief Calls getSupportedOperationalModes and verifies a non-empty list is returned.
 * @param[in]  videoDecoder The IVideoDecoder instance.
 * @param[out] mode Operational mode currently selected by the vendor.
 * @pre The decoder must be in State::STARTED (otherwise EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_GET_OPERATIONALMODE_SUCCESS(videoDecoder, operationalMode) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getCurrentOperationalMode(OUT:operationalMode[])"); \
        status = (videoDecoder)->getCurrentOperationalMode(&(operationalMode)); \
        UT_LOG_INFO("Result getCurrentOperationalMode(OUT:operationalMode[%s]), status:[isOK:%s, exception:%s]", \
            (status.isOk() ? UT_Control_GetMapString(decoder_operationalMode_mapTable, static_cast<int32_t>(operationalMode)) : "N/A"), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define VIDEODECODER_GET_OPERATIONALMODE_FAILURE(videoDecoder, operationalMode, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getCurrentOperationalMode(OUT:operationalMode[])"); \
        status = (videoDecoder)->getCurrentOperationalMode(&(operationalMode)); \
        UT_LOG_INFO("Result getCurrentOperationalMode(OUT:operationalMode[%s]), status:[isOK:%s, exception:%s]", \
            (status.isOk() ? UT_Control_GetMapString(decoder_operationalMode_mapTable, static_cast<int32_t>(operationalMode)) : "N/A"), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) \
        { \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)); \
        } \
    } while (0)

/**
 * @brief Calls getCapabilities and verifies the result.
 * @param[in] decoder The IVideoDecoder instance.
 * @param[out] capabilities Pointer to the Capabilities struct to be filled.
 */
#define VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities) \
     do { \
         Status status; \
         UT_LOG_INFO("Calling getCapabilities(OUT:capabilities)"); \
         status = videoDecoder->getCapabilities(capabilities); \
         UT_LOG_INFO("Result getCapabilities(OUT:capabilities), status:[isOK:%s, exception:%s]", \
                     UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                     UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
         UT_ASSERT_TRUE(status.isOk()); \
         UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
     } while (0)

#define VIDEODECODER_GET_PROPERTY_FAILURE(videoDecoder, property, propertyValue, exception) \
    do { \
        Status status; \
        State state; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", videoDecoder::toString(property).c_str()); \
        status = (videoDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", videoDecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

/**
 * @brief Calls getProperty, verifies the result, and checks for intValue tag.
 * @param[in] decoder The IVideoDecoder instance.
 * @param[in] property The property to query.
 * @param[out] aidl_return The optional PropertyValue result.
 */
#define VIDEODECODER_GETPROPERTY(videoDecoder, property, aidl_return) \
do { \
    Status status; \
    UT_LOG_INFO("Calling getProperty(IN:property:[%d], OUT:_aidl_return[]) ", static_cast<int32_t>(property)); \
    status = (videoDecoder)->getProperty((property), &(aidl_return)); \
    if(aidl_return.has_value()) \
    { \
        switch (aidl_return->value.value().getTag()) \
        { \
            case PropertyValue::Value::Tag::intValue: \
                UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                aidl_return->value->get<PropertyValue::Value::Tag::intValue>(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                break; \
            case PropertyValue::Value::Tag::stringValue: \
                UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[%s]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                aidl_return->value->get<PropertyValue::Value::Tag::stringValue>().c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                break; \
            default: \
                break; \
        } \
    } \
    else \
    { \
        UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(false); \
    } \
    UT_ASSERT_TRUE(status.isOk()) << "getProperty failed for property " << static_cast<int32_t>(property); \
    UT_ASSERT_TRUE((aidl_return).has_value()) << "No value returned for property " << static_cast<int32_t>(property); \
} while (0)

#define VIDEODECODER_GET_PROPERTY_SUCCESS(videoDecoder, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", videodecoder::toString(property).c_str()); \
        status = (videoDecoder)->getProperty(property, &propertyValue); \
        if (propertyValue.has_value() && propertyValue->value.has_value()) \
        { \
            switch (propertyValue->value.value().getTag()) \
            { \
                case PropertyValue::Value::Tag::intValue: \
                    UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                    propertyValue->value->get<PropertyValue::Value::Tag::intValue>(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                    break; \
                case PropertyValue::Value::Tag::stringValue: \
                    UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%s]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                    propertyValue->value->get<PropertyValue::Value::Tag::stringValue>().c_str(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                    break; \
                default: \
                    break; \
            } \
        } \
        else \
        { \
            UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        } \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

/**
 * @brief Calls getPropertyMulti and verifies the result.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] properties The list of Property enum values to query.
 * @param[out] propertyKVList The list of PropertyKVPair to populate.
 * @param[out] aidl_return The boolean result of the operation.
 */
#define VIDEODECODER_GETPROPERTYMULTI(videoDecoder, properties, propertyKVList, aidl_return) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getPropertyMulti(IN:properties.size:[%zu], OUT:propertyKVList[], OUT:_aidl_return)", (properties).size()); \
        status = (videoDecoder)->getPropertyMulti((properties), &(propertyKVList), &(aidl_return)); \
        UT_LOG_INFO("Result getPropertyMulti(OUT:propertyKVList.size:[%zu], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    (propertyKVList).size(), \
                    UT_Control_GetMapString(bool_mapTable, (aidl_return)), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "Binder call failed: " << status.exceptionMessage().c_str(); \
        UT_ASSERT_TRUE(aidl_return) << "getPropertyMulti returned false."; \
    } while (0)

/**
 * @brief Adds a Property enum value to a properties vector for getPropertyMulti input.
 * @param[in,out] properties The vector to append to.
 * @param[in] prop The Property enum value to add.
 */
#define VIDEODECODER_ADD_PROPERTY(properties, prop) \
    do { \
        (properties).push_back(videodecoder::Property::prop); \
    } while (0)

/**
 * @brief Adds a PropertyKVPair to a vector with the specified property.
 * @param[in,out] propertyKVList The vector to append to.
 * @param[in] prop The Property enum value to set.
 */
#define VIDEODECODER_ADD_PROPERTYKVPAIR(propertyKVList, prop) \
    do { \
        PropertyKVPair kvPair; \
        kvPair.property = videodecoder::Property::prop; \
        kvPair.propertyValue.value = PropertyValue::Value(); \
        (propertyKVList).push_back(kvPair); \
    } while (0)

    /**
 * @brief Calls getPropertyMulti for a negative test and verifies failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] properties The list of Property enum values to query (may be invalid or empty).
 * @param[in,out] propertyKVList The list of PropertyKVPair output.
 * @param[out] aidl_return The boolean result, expected to be false.
 * @param[in] exception The expected exception code.
 */
#define VIDEODECODER_GETPROPERTYMULTI_FAILURE(videoDecoder, properties, propertyKVList, aidl_return, exception) \
do { \
    Status status; \
    std::vector<videodecoder::PropertyKVPair>* _propertyKVList = &propertyKVList; \
    if(exception == Status::EX_NULL_POINTER) \
    { \
        UT_LOG_INFO("Calling getPropertyMulti(IN:properties.size:[%zu], OUT:propertyKVList:NULL, OUT:_aidl_return)", (properties).size()); \
        _propertyKVList = nullptr; \
    } \
    else { \
        UT_LOG_INFO("Calling getPropertyMulti(IN:properties.size:[%zu], OUT:propertyKVList[], OUT:_aidl_return)", (properties).size()); \
    } \
    status = (videoDecoder)->getPropertyMulti((properties), _propertyKVList, &(aidl_return)); \
    UT_LOG_INFO("Result getPropertyMulti(), status:[isOK:%s, exception:%s]", \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
    UT_ASSERT_FALSE(status.isOk()); \
    UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
} while (0)

/**
 * @brief Calls getState and verifies the result.
 * @param[in] videoDecoder The IVideoDecoder instance (not IVideoDecoderController).
 * @param[out] state The returned State enum value.
 */
#define VIDEODECODER_GETSTATE(videoDecoder, state) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getState(OUT:_aidl_return:[state])"); \
        status = (videoDecoder)->getState(&state); \
        UT_LOG_INFO("Result getState(OUT:_aidl_return:[state:%d]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(state), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getState binder call failed"; \
        UT_ASSERT_TRUE((state) == ::com::rdk::hal::videodecoder::State::UNKNOWN || \
                       (state) == ::com::rdk::hal::videodecoder::State::CLOSED || \
                       (state) == ::com::rdk::hal::videodecoder::State::OPENING || \
                       (state) == ::com::rdk::hal::videodecoder::State::READY || \
                       (state) == ::com::rdk::hal::videodecoder::State::STARTING || \
                       (state) == ::com::rdk::hal::videodecoder::State::STARTED || \
                       (state) == ::com::rdk::hal::videodecoder::State::FLUSHING || \
                       (state) == ::com::rdk::hal::videodecoder::State::STOPPING || \
                       (state) == ::com::rdk::hal::videodecoder::State::CLOSING) \
            << "Invalid state returned: " << static_cast<int32_t>(state); \
    } while (0)

#define VIDEODECODER_CHECK_STATE(videoDecoder, expected_state) \
    do { \
        ::com::rdk::hal::videodecoder::State currentState; \
        Status status = (videoDecoder)->getState(&currentState); \
        UT_LOG_INFO("Calling getState(OUT:_aidl_return:[state]) for decoder"); \
        UT_LOG_INFO("Result getState(OUT:_aidl_return:[state:%d]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(currentState), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getState binder call failed"; \
        UT_ASSERT_EQUAL(currentState, expected_state) << "Expected state " << static_cast<int32_t>(expected_state) \
                                                  << ", got " << static_cast<int32_t>(currentState); \
    } while (0)
/**
 * @brief Waits for the video decoder to reach a target state.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] targetState The expected state to wait for.
 * @param[out] lastState The final state observed.
 */
#define VIDEODECODER_WAIT_FOR_STATE(videoDecoder, targetState, lastState) \
    do { \
        int32_t sleep_time = 0; \
        do \
        { \
            std::this_thread::sleep_for(std::chrono::milliseconds(VIDEO_MAX_TRANSITORY_STATE_WAIT_MS)); \
            sleep_time += VIDEO_MAX_TRANSITORY_STATE_WAIT_MS; \
            /* Check the state of the decoder */ \
            VIDEODECODER_GETSTATE(videoDecoder, lastState); \
        } while (sleep_time < VIDEO_MAX_FINAL_STATE_WAIT_MS && lastState != targetState); \
    } while (0)

/**
 * @brief Opens the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] codec The Codec enum value to open.
 * @param[in] secure Whether to open in secure mode.
 * @param[in] listener The controller listener.
 * @param[out] controller The returned IVideoDecoderController.
 * @param[in] stateCheck Whether to validate the state after opening.
 */
#define VIDEODECODER_OPEN_SUCCESS(videoDecoder, codec, secure, listener, controller, stateCheck) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller[])", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), (listener).get()); \
        status = (videoDecoder)->open((codec), (secure), (listener), &(controller)); \
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller:[0x%X]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), (listener).get(), (controller).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "open binder call failed"; \
        UT_ASSERT_NOT_EQUAL_FATAL((controller).get(), nullptr) << "Controller is null; codec or secure mode may not be supported"; \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE) << "open returned an exception"; \
        if (stateCheck) { \
            ::com::rdk::hal::videodecoder::State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::READY) << "Decoder did not reach READY state after open"; \
        } \
    } while (0)

/**
 * @brief Tests opening the video decoder with expected failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] codec The Codec enum value to test.
 * @param[in] secure The secure mode to test.
 * @param[in] listener The controller listener (or nullptr).
 * @param[out] controller The returned IVideoDecoderController.
 * @param[in] exception The expected exception code (or EX_NONE for unsupported cases).
 */
#define VIDEODECODER_OPEN_FAILURE(videoDecoder, codec, secure, listener, controller, exception) \
    do { \
        Status status; \
        sp<IVideoDecoderControllerListener> tempListener = (listener); \
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller[])", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), tempListener ? tempListener.get() : nullptr); \
        status = (videoDecoder)->open((codec), (secure), (listener), &(controller)); \
        if ((exception) == Status::EX_NONE) { \
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller:[0x%X]), status:[isOK:%s, exception:%s]", \
                        static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), tempListener ? tempListener.get() : nullptr, \
                        (controller).get(), \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_TRUE(status.isOk()) << "Binder call failed unexpectedly"; \
            UT_ASSERT_EQUAL((controller).get(), nullptr) << "Expected null controller for unsupported parameters"; \
        } else { \
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p]), status:[isOK:%s, exception:%s]", \
                        static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), tempListener ? tempListener.get() : nullptr, \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_FALSE(status.isOk()) << "Expected failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)


/**
 * @brief Closes the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] controller The IVideoDecoderController instance.
 * @param[in] stateCheck Whether to validate the state after closing.
 */
#define VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, stateCheck) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderController> tempController = (controller); \
        UT_LOG_INFO("Calling close(IN:videoDecoderController:[0x%p], OUT:_aidl_return[])", tempController ? tempController.get() : nullptr); \
        status = (videoDecoder)->close((controller), &aidl_return); \
        UT_LOG_INFO("Result close(IN:videoDecoderController:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    tempController ? tempController.get() : nullptr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "close binder call failed"; \
        UT_ASSERT_TRUE_FATAL(aidl_return) << "close returned false"; \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE) << "close returned an exception"; \
        if (stateCheck) { \
            ::com::rdk::hal::videodecoder::State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::CLOSED, lastState); \
            UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::CLOSED) << "Decoder did not reach CLOSED state after close"; \
        } \
    } while (0)

/**
 * @brief Tests closing the video decoder with expected failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] controller The IVideoDecoderController instance (or nullptr).
 * @param[in] exception The expected exception code (or EX_NONE for false return).
 */
#define VIDEODECODER_CLOSE_FAILURE(videoDecoder, controller, exception) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderController> tempController = (controller); \
        UT_LOG_INFO("Calling close(IN:videoDecoderController:[0x%p], OUT:_aidl_return[])", tempController ? tempController.get() : nullptr); \
        status = (videoDecoder)->close((controller), &aidl_return); \
        if ((exception) == Status::EX_NONE) { \
            UT_LOG_INFO("Result close(IN:videoDecoderController:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                         tempController ? tempController.get() : nullptr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_TRUE(status.isOk()) << "Binder call failed unexpectedly"; \
            UT_ASSERT_FALSE(aidl_return) << "Expected close to return false"; \
        } else { \
            UT_LOG_INFO("Result close(IN:videoDecoderController:[0x%p]), status:[isOK:%s, exception:%s]", \
                         tempController ? tempController.get() : nullptr, \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_FALSE(status.isOk()) << "Expected failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)

/**
 * @brief Calls open and verifies a valid controller is returned.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] codec The codec to use.
 * @param[in] secure The secure mode flag.
 * @param[in] listener The controller listener.
 * @param[out] controller The returned IVideoDecoderController.
 */
#define VIDEODECODER_OPEN(videoDecoder, codec, secure, listener, controller) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[])", \
                    static_cast<int32_t>(codec), (secure), (listener).get()); \
        status = (videoDecoder)->open((codec), (secure), (listener), &(controller)); \
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(codec), (secure), (listener).get(), (controller).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE((controller) != nullptr); \
    } while (0)

#define VIDEODECODER_OPEN_WITH_RESOLUTION_SUCCESS(videoDecoder, codec, secure, listener, maxWidth, maxHeight, controller, stateCheck) \
    do { \
        Status status; \
        sp<IVideoDecoderControllerListener> tempListener = (listener); \
        UT_LOG_INFO("Calling openWithResolution(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], IN:maxWidth:[%d], IN:maxHeight:[%d], OUT:controller[])", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, secure), tempListener ? tempListener.get() : nullptr, \
                    static_cast<int32_t>(maxWidth), static_cast<int32_t>(maxHeight)); \
        status = (videoDecoder)->openWithResolution((codec), (secure), (listener), (maxWidth), (maxHeight), &(controller)); \
        UT_LOG_INFO("Result openWithResolution(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], IN:maxWidth:[%d], IN:maxHeight:[%d], OUT:controller:[0x%p]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, secure), tempListener ? tempListener.get() : nullptr, \
                static_cast<int32_t>(maxWidth), static_cast<int32_t>(maxHeight), static_cast<void*>((controller).get()), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "openWithResolution binder call failed"; \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE_FATAL((controller) != nullptr) << "openWithResolution returned null controller"; \
        if (stateCheck) { \
            ::com::rdk::hal::videodecoder::State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::READY) << "Decoder did not reach READY state"; \
        } \
    } while (0)

#define VIDEODECODER_OPEN_WITH_RESOLUTION_FAILURE(videoDecoder, codec, secure, listener, maxWidth, maxHeight, controller, exception) \
    do { \
        Status status; \
        sp<IVideoDecoderControllerListener> tempListener = (listener); \
        UT_LOG_INFO("Calling openWithResolution(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], IN:maxWidth:[%d], IN:maxHeight:[%d], OUT:controller[])", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, secure), tempListener ? tempListener.get() : nullptr, \
                    static_cast<int32_t>(maxWidth), static_cast<int32_t>(maxHeight)); \
        status = (videoDecoder)->openWithResolution((codec), (secure), (listener), (maxWidth), (maxHeight), &(controller)); \
        if ((exception) == Status::EX_NONE) { \
            UT_LOG_INFO("Result openWithResolution(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], IN:maxWidth:[%d], IN:maxHeight:[%d], OUT:controller:[0x%p]), status:[isOK:%s, exception:%s]", \
                        static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, secure), tempListener ? tempListener.get() : nullptr, \
                        static_cast<int32_t>(maxWidth), static_cast<int32_t>(maxHeight), static_cast<void*>((controller).get()), \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_TRUE(status.isOk()) << "openWithResolution binder call failed unexpectedly"; \
            UT_ASSERT_TRUE((controller) == nullptr) << "Expected openWithResolution to return null controller"; \
        } else { \
            UT_LOG_INFO("Result openWithResolution(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], IN:maxWidth:[%d], IN:maxHeight:[%d]), status:[isOK:%s, exception:%s]", \
                        static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, secure), tempListener ? tempListener.get() : nullptr, \
                        static_cast<int32_t>(maxWidth), static_cast<int32_t>(maxHeight), \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_FALSE(status.isOk()) << "Expected openWithResolution failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)); \
        } \
    } while (0)

/**
 * @brief Registers an event listener with the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] listener The IVideoDecoderEventListener to register.
 */
#define VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, listener) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderEventListener> tempListener = (listener); \
        UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return[])", tempListener ? tempListener.get() : nullptr); \
        status = (videoDecoder)->registerEventListener((listener), &aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    tempListener ? tempListener.get() : nullptr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "registerEventListener binder call failed"; \
        UT_ASSERT_TRUE(aidl_return) << "registerEventListener returned false"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "registerEventListener returned an exception"; \
    } while (0)

/**
 * @brief Unregisters an event listener from the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] listener The IVideoDecoderEventListener to unregister.
 */
#define VIDEODECODER_UNREGISTER_EVENT_SUCCESS(videoDecoder, listener) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderEventListener> tempListener = (listener); \
        UT_LOG_INFO("Calling unregisterEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return[])", tempListener ? tempListener.get() : nullptr); \
        status = (videoDecoder)->unregisterEventListener((listener), &aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    tempListener ? tempListener.get() : nullptr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "unregisterEventListener binder call failed"; \
        UT_ASSERT_TRUE(aidl_return) << "unregisterEventListener returned false"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "unregisterEventListener returned an exception"; \
    } while (0)

/**
 * @brief Tests registering an event listener with expected failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] listener The IVideoDecoderEventListener (or nullptr).
 * @param[in] exception The expected exception code (or EX_NONE for false return).
 */
#define VIDEODECODER_REGISTER_EVENT_FAILURE(videoDecoder, listener, exception) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderEventListener> tempListener = (listener); \
        UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return[])", tempListener ? tempListener.get() : nullptr); \
        status = (videoDecoder)->registerEventListener((listener), &aidl_return); \
        if ((exception) == Status::EX_NONE) { \
            UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                        tempListener ? tempListener.get() : nullptr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_TRUE(status.isOk()) << "Binder call failed unexpectedly"; \
            UT_ASSERT_FALSE(aidl_return) << "Expected registerEventListener to return false"; \
        } else { \
            UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%p]), status:[isOK:%s, exception:%s]", \
                        tempListener ? tempListener.get() : nullptr, \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_FALSE(status.isOk()) << "Expected failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)

#define VIDEODECODER_UNREGISTER_EVENT_FAILURE(videoDecoder, eventListener, exception) \
    do { \
        Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[])", (eventListener) ? (eventListener).get() : nullptr); \
        status = (videoDecoder)->unregisterEventListener((eventListener), &_aidl_return); \
        if ((exception) == Status::EX_NONE) { \
            UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
                        (eventListener) ? (eventListener).get() : nullptr, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_FALSE(_aidl_return) << "Expected unregisterEventListener to return false"; \
        } else { \
            UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%p]), status:[isOK:%s, exception:%s]", \
                        (eventListener) ? (eventListener).get() : nullptr, \
                        UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_FALSE(status.isOk()); \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)

#define VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, stateCheck) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = (videoDecoderController)->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "start binder call failed"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "start returned an exception"; \
        if (stateCheck) { \
            ::com::rdk::hal::videodecoder::State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::STARTED) << "Decoder did not reach STARTED state"; \
        } \
    } while (0)

#define VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, stateCheck) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = videoDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) { \
            ::com::rdk::hal::videodecoder::State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::READY) << "Decoder did not reach READY state"; \
        } \
    } while (0)

#define VIDEODECODER_START_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling start() expecting failure"); \
        status = videoDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), expectedException); \
    } while (0)

#define VIDEODECODER_STOP_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling stop() expecting failure"); \
        status = videoDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), expectedException); \
    } while (0)

/**
 * @brief Calls setProperty on an IVideoDecoder instance to set an integer property value.
 * @param[in] videoDecoder The IVideoDecoder instance (not IVideoDecoderController).
 * @param[in] property The Property enum key (must be read-write, e.g., OPERATIONAL_MODE).
 * @param[in] value The integer value to set.
 * @param[in] value_desc String description of the value for logging.
 */

#define VIDEODECODER_SETPROPERTY_SUCCESS(videoDecoderController, property, propertyValue) \
    do { \
        Status status; \
        bool result; \
        int32_t logValue = 0; \
        if (propertyValue.value.has_value()) { \
            switch (propertyValue.value.value().getTag()) { \
                case PropertyValue::Value::Tag::intValue: \
                    logValue = propertyValue.value.value().get<PropertyValue::Value::Tag::intValue>(); \
                    break; \
                default: \
                    logValue = -1; \
                    break; \
            } \
        } \
        UT_LOG_INFO("Calling setProperty(IN:property[%d], IN:propertyValue=%d)", \
                    static_cast<int32_t>(property), logValue); \
        status = videoDecoderController->setProperty(property, propertyValue, &result); \
        UT_LOG_INFO("Result setProperty(IN:property[%d], OUT:result=%s), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(property), UT_Control_GetMapString(bool_mapTable, result), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(result); \
    } while (0)

#define VIDEODECODER_SETPROPERTY_FAILURE(videoDecoderController, property, propertyValue, expectedException) \
    do { \
        Status status; \
        bool result; \
        int32_t logValue = 0; \
        if (propertyValue.value.has_value()) { \
            switch (propertyValue.value.value().getTag()) { \
                case PropertyValue::Value::Tag::intValue: \
                    logValue = propertyValue.value.value().get<PropertyValue::Value::Tag::intValue>(); \
                    break; \
                default: \
                    logValue = -1; \
                    break; \
            } \
        } \
        UT_LOG_INFO("Calling setProperty(IN:property[%d], IN:propertyValue=%d) expecting failure", \
                    static_cast<int32_t>(property), logValue); \
        status = videoDecoderController->setProperty(property, propertyValue, &result); \
        UT_LOG_INFO("Result setProperty(IN:property[%d]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(property), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), expectedException); \
    } while (0)

    #define VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, propertyList, type) \
    do { \
        for (const auto& [property, value] : propertyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            VIDEODECODER_SETPROPERTY_SUCCESS(videoDecoderController, property, propertyValue); \
        } \
    } while (0)

#define VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, propertyList, type, exception) \
    do { \
        for (const auto& [property, value] : propertyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            VIDEODECODER_SETPROPERTY_FAILURE(videoDecoderController, property, propertyValue, exception); \
        } \
    } while (0)

#define VIDEODECODER_CONTROLLER_SET_PROPERTY(videoDecoderController, property, propertyValue, status, result) \
    do { \
        int32_t logValue = 0; \
        if (propertyValue.value.has_value()) { \
            switch (propertyValue.value.value().getTag()) { \
                case PropertyValue::Value::Tag::intValue: \
                    logValue = propertyValue.value.value().get<PropertyValue::Value::Tag::intValue>(); \
                    break; \
                default: \
                    logValue = -1; \
                    break; \
            } \
        } \
        UT_LOG_INFO("Calling setProperty(IN:property[%d], IN:propertyValue=%d)", \
                    static_cast<int32_t>(property), logValue); \
        status = videoDecoderController->setProperty(property, propertyValue, &result); \
        UT_LOG_INFO("Result setProperty(IN:property[%d], OUT:result=%d), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(property), result, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
    } while (0)

/**
 * @brief Tests that flush fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] reset The reset flag for flush.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_FLUSH_FAILURE(videoDecoderController, reset, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling flush(IN:reset:[%s])", (reset) ? "true" : "false"); \
        status = (videoDecoderController)->flush(reset); \
        UT_LOG_INFO("Result flush(IN:reset:[%s]), status:[isOK:%s, exception:%s]", \
                    (reset) ? "true" : "false", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "flush succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for flush"; \
    } while (0)
/**
 * @brief Tests that flush fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] reset The reset flag for flush.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */

#define VIDEODECODER_FLUSH_SUCCESS(videoDecoder, videoDecoderController, reset, stateCheck) \
    do { \
        Status status = (videoDecoderController)->flush(reset); \
        UT_LOG_INFO("flush(%s) status=%s", reset ? "true" : "false", UT_Control_GetMapString(bool_mapTable, status.isOk())); \
        UT_ASSERT_TRUE(status.isOk()) << "flush(" << reset << ") failed"; \
        if (stateCheck) { \
            ::com::rdk::hal::videodecoder::State lastState; \
            if (reset){ \
                VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::READY, lastState); \
                UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::READY) << "Decoder did not reach READY state"; \
            } \
            else { \
                VIDEODECODER_WAIT_FOR_STATE(videoDecoder, ::com::rdk::hal::videodecoder::State::STARTED, lastState); \
                UT_ASSERT_EQUAL(lastState, ::com::rdk::hal::videodecoder::State::STARTED) << "Decoder did not reach STARTED state"; \
            } \
        } \
    } while (0)
/**
 * @brief Tests that signalDiscontinuity succeeds in the STARTED state.
 * @param[in] videoDecoder The IVideoDecoder instance (for state checking).
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] expectSuccess Expect the call to succeed (true for positive test).
 */
#define VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(videoDecoder, videoDecoderController, expectSuccess) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = (videoDecoderController)->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk() == (expectSuccess)) << "signalDiscontinuity " \
            << ((expectSuccess) ? "failed unexpectedly" : "succeeded unexpectedly"); \
    } while (0)

/**
 * @brief Tests that signalDiscontinuity fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_SIGNAL_DISCONTINUITY_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = (videoDecoderController)->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "signalDiscontinuity succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for signalDiscontinuity"; \
    } while (0)

#define VIDEODECODER_SIGNAL_EOS_SUCCESS(videoDecoderController) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalEndOfStream()"); \
        status = (videoDecoderController)->signalEndOfStream(); \
        UT_LOG_INFO("Result signalEndOfStream(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define VIDEODECODER_SIGNAL_EOS_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalEndOfStream()"); \
        status = (videoDecoderController)->signalEndOfStream(); \
        UT_LOG_INFO("Result signalEndOfStream(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)); \
    } while (0)

/**
 * @brief Tests that parseCodecSpecificData succeeds in STARTED with valid data.
 * @param[in] videoDecoder The IVideoDecoder instance (for state checking).
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] csdVideoFormat The codec-specific data format.
 * @param[in] codecData The codec data byte array.
 * @param[in] expectSuccess Expect the call to succeed (true for positive test).
 */

#define VIDEODECODER_PARSE_CSD_SUCCESS(videoDecoder, videoDecoderController, csdVideoFormat, codecData, expectSuccess) \
    do { \
        bool result = false; \
        Status status; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[%d], IN:codecData:size:[%zu])", \
                    static_cast<int32_t>(csdVideoFormat), codecData.size()); \
        status = (videoDecoderController)->parseCodecSpecificData(csdVideoFormat, codecData, &result); \
        UT_LOG_INFO("Result parseCodecSpecificData(OUT:result:[%s]), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, result), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "parseCodecSpecificData binder call failed"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "parseCodecSpecificData returned exception"; \
        if (expectSuccess) { \
            UT_ASSERT_TRUE(result) << "parseCodecSpecificData returned false"; \
        } \
    } while (0)

/**
 * @brief Tests that parseCodecSpecificData fails with EX_ILLEGAL_STATE in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] csdVideoFormat The codec-specific data format.
 * @param[in] codecData The codec data byte array.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_PARSE_CSD_STATE_FAILURE(videoDecoderController, csdVideoFormat, codecData, expectedException) \
    do { \
        bool result = false; \
        Status status; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[%d], IN:codecData:size:[%zu])", \
                    static_cast<int32_t>(csdVideoFormat), codecData.size()); \
        status = (videoDecoderController)->parseCodecSpecificData(csdVideoFormat, codecData, &result); \
        UT_LOG_INFO("Result parseCodecSpecificData(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "parseCodecSpecificData succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for parseCodecSpecificData"; \
    } while (0)

/**
 * @brief Tests that parseCodecSpecificData fails with false return for invalid data.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] csdVideoFormat The codec-specific data format.
 * @param[in] codecData The invalid codec data byte array.
 */
#define VIDEODECODER_PARSE_CSD_DATA_FAILURE(videoDecoderController, csdVideoFormat, codecData, exception) \
    do { \
        ::android::binder::Status status; \
        bool result; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[%d], IN:codecData:size:[%zu])", \
                    static_cast<int32_t>(csdVideoFormat), codecData.size()); \
        status = videoDecoderController->parseCodecSpecificData(csdVideoFormat, codecData, &result); \
        UT_LOG_INFO("Result parseCodecSpecificData(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "parseCodecSpecificData should fail"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception) << "Unexpected exception for parseCodecSpecificData"; \
    } while (0)

#define VIDEODECODER_DECODEBUFFER_WITH_METADATA_SUCCESS(controller, frameBufferHandle, frameMetadata) \
    do { \
        bool decodeResult = false; \
        UT_LOG_INFO("Calling decodeBufferWithMetadata(IN:bufferHandle=[0x%016" PRIx64 "], IN:metadata:[pts=%lld])", \
                    static_cast<uint64_t>(frameBufferHandle), static_cast<long long>((frameMetadata).nsPresentationTime)); \
        Status status = (controller)->decodeBufferWithMetadata((frameBufferHandle), (frameMetadata), &decodeResult); \
        UT_LOG_INFO("Result decodeBufferWithMetadata(IN:bufferHandle=[0x%016" PRIx64 "], IN:metadata:[pts=%lld]) status=[isOK:%s, exception:%s], result=%s", \
                    static_cast<uint64_t>(frameBufferHandle), static_cast<long long>((frameMetadata).nsPresentationTime), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), \
                    decodeResult ? "true" : "false"); \
        UT_ASSERT_TRUE(status.isOk()) << "decodeBufferWithMetadata failed"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(decodeResult) << "decodeBufferWithMetadata returned false"; \
        if(decodeResult == false || status.isOk() == false || status.exceptionCode() != Status::EX_NONE) \
        { \
            if(frameBufferHandle != -1) \
            { \
                AVBUFFER_FREE_SUCCESS(gAVBufferManager, frameBufferHandle);\
            } \
        } \
    } while (0)

#define VIDEODECODER_DECODEBUFFER_WITH_METADATA_FAILURE(controller, framePts, frameBufferHandle, expectedException) \
    do { \
        bool decodeResult = false; \
        ::com::rdk::hal::videodecoder::InputBufferMetadata inputMetadata = {}; \
        inputMetadata.nsPresentationTime = static_cast<int64_t>(framePts); \
        UT_LOG_INFO("Calling decodeBufferWithMetadata(IN:bufferHandle=[0x%016" PRIx64 "], IN:metadata:[pts=%lld])", \
                    static_cast<uint64_t>(frameBufferHandle), static_cast<long long>(inputMetadata.nsPresentationTime)); \
        Status status = (controller)->decodeBufferWithMetadata((frameBufferHandle), inputMetadata, &decodeResult); \
        UT_LOG_INFO("Result decodeBufferWithMetadata(IN:bufferHandle=[0x%016" PRIx64 "], IN:metadata:[pts=%lld]) status=[isOK:%s, exception:%s], result=%s", \
                    static_cast<uint64_t>(frameBufferHandle), static_cast<long long>(inputMetadata.nsPresentationTime), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), \
                    decodeResult ? "true" : "false"); \
        if (expectedException != Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.exceptionCode() == expectedException) \
                << "decodeBufferWithMetadata returned unexpected exception"; \
        } \
        else { \
            UT_ASSERT_TRUE(status.isOk()) << "decodeBufferWithMetadata failed unexpectedly"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
            UT_ASSERT_FALSE(decodeResult) << "decodeBufferWithMetadata returned true unexpectedly"; \
        } \
    } while (0)

#define VIDEODECODER_SETSTREAMCONFIG_SUCCESS(videoDecoderController, streamConfig) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling setStreamConfig(IN:config=[%s])", (streamConfig).toString().c_str()); \
        status = (videoDecoderController)->setStreamConfig((streamConfig)); \
        UT_LOG_INFO("Result setStreamConfig(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define VIDEODECODER_SETSTREAMCONFIG_FAILURE(videoDecoderController, streamConfig, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling setStreamConfig(IN:config=[%s]) expecting failure", (streamConfig).toString().c_str()); \
        status = (videoDecoderController)->setStreamConfig((streamConfig)); \
        UT_LOG_INFO("Result setStreamConfig(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)); \
    } while (0)

#define VIDEODECODER_SETSTREAMRESOLUTION_SUCCESS(ctrl, w, h) \
    do { \
        ::com::rdk::hal::videodecoder::VideoDecoderStreamConfig _scfg; \
        ::com::rdk::hal::videodecoder::Resolution _res; \
        _res.width = static_cast<int32_t>(w); \
        _res.height = static_cast<int32_t>(h); \
        _scfg.resolution = _res; \
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(ctrl, _scfg); \
    } while (0)

#define VIDEODECODER_SETFRAMERATE_SUCCESS(ctrl, num, den) \
    do { \
        ::com::rdk::hal::videodecoder::VideoDecoderStreamConfig _scfg; \
        ::com::rdk::hal::videodecoder::Fraction _fr; \
        _fr.numerator = static_cast<int32_t>(num); \
        _fr.denominator = static_cast<int32_t>(den); \
        _scfg.frameRate = _fr; \
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(ctrl, _scfg); \
    } while (0)

#define VIDEODECODER_SETCOLORIMETRY_SUCCESS(ctrl, colorimetry) \
    do { \
        ::com::rdk::hal::videodecoder::VideoDecoderStreamConfig _scfg; \
        _scfg.colorimetry = (colorimetry); \
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(ctrl, _scfg); \
    } while (0)

#define VIDEODECODER_SETMASTERINGDISPLAYINFO_SUCCESS(ctrl, info) \
    do { \
        ::com::rdk::hal::videodecoder::VideoDecoderStreamConfig _scfg; \
        _scfg.masteringDisplayInfo = (info); \
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(ctrl, _scfg); \
    } while (0)

#define VIDEODECODER_SETCONTENTLIGHTLEVEL_SUCCESS(ctrl, lightLevel) \
    do { \
        ::com::rdk::hal::videodecoder::VideoDecoderStreamConfig _scfg; \
        _scfg.contentLightLevel = (lightLevel); \
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(ctrl, _scfg); \
    } while (0)

#define VIDEODECODER_SETDOLBYVISIONLAYERFLAGS_SUCCESS(ctrl, blPresent, elPresent) \
    do { \
        ::com::rdk::hal::videodecoder::VideoDecoderStreamConfig _scfg; \
        ::com::rdk::hal::videodecoder::DolbyVisionLayerFlags _dvf; \
        _dvf.blPresent = static_cast<bool>(blPresent); \
        _dvf.elPresent = static_cast<bool>(elPresent); \
        _scfg.dolbyVisionLayerFlags = _dvf; \
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(ctrl, _scfg); \
    } while (0)

/**
 * @brief Test fixture for Level 1 Video Decoder HALIF tests.
 * Inherits from UTCore to leverage ut-core's test utilities and lifecycle methods.
 */
class VTS_L1_VideoDecoder : public UTCore
{
    public:
        /**
         * @brief Default constructor.
         * Relies on UTCore's default initialization; no additional setup required.
         */
        VTS_L1_VideoDecoder() = default;

        /**
         * @brief Virtual destructor.
         * Ensures proper cleanup; defaults to UTCore's implementation.
         */
        ~VTS_L1_VideoDecoder() override = default;

        /**
         * @brief Prepares the test environment before each test case.
         * Calls UTCore::SetUp() to ensure base class initialization.
         * Add custom setup logic here if needed.
         */
        void SetUp() override {
            if (gVideoDecoderManager == nullptr) {
                UT_FAIL_FATAL("VideoDecoder Service not available");
            }

            UTCore::SetUp();  // Call base class setup (if it exists)
            // Custom setup code here, e.g., initialize decoder mocks or profiles
        }

        /**
         * @brief Cleans up the test environment after each test case.
         * Calls UTCore::TearDown() to ensure base class cleanup.
         * Add custom cleanup logic here if needed.
         */
        void TearDown() override {
            // Custom cleanup code here, e.g., release resources
            UTCore::TearDown();  // Call base class teardown (if it exists)
        }
};

class VideoDecoderControllerListener : public BnVideoDecoderControllerListener
{
public:
    VideoDecoderControllerListener() {}
    virtual ~VideoDecoderControllerListener() = default;

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameAVBufferHandle, const ::std::optional<::com::rdk::hal::videodecoder::FrameMetadata>& metadata) override
    {
        DecodedVideoFrame_t frame;
        frame.receiveTime = std::chrono::steady_clock::now();

        UT_LOG_INFO("[Video DecoderID: %d] onFrameOutput received: nsPresentationTime=%lld, frameAVBufferHandle=0x%016" PRIx64 "", mDecoderID, nsPresentationTime, static_cast<uint64_t>(frameAVBufferHandle));

        frame.framePTS = nsPresentationTime;
        frame.isMetaAvailable = metadata.has_value();
        frame.frameAVBufferHandle = frameAVBufferHandle; // Store handle
        if (metadata.has_value())
        {
            frame.metadata = std::make_unique<com::rdk::hal::videodecoder::FrameMetadata>(metadata.value());

            UT_LOG_INFO("FrameMetadata: codedWidth=%d, codedHeight=%d, activeX=%d, activeY=%d, activeWidth=%d, activeHeight=%d",
                        frame.metadata->codedWidth, frame.metadata->codedHeight,
                        frame.metadata->activeX, frame.metadata->activeY,
                        frame.metadata->activeWidth, frame.metadata->activeHeight);

            UT_LOG_INFO("AspectRatios: PAR=%d:%d, SAR=%d:%d",
                        frame.metadata->parX, frame.metadata->parY,
                        frame.metadata->sarX, frame.metadata->sarY);

            UT_LOG_INFO("FrameMetadata: colorDepth=%d, pixelFormat=%d, dynamicRange=%d, scanType=%d, afd=%d",
                        frame.metadata->colorDepth,
                        static_cast<int>(frame.metadata->pixelFormat),
                        static_cast<int>(frame.metadata->dynamicRange),
                        static_cast<int>(frame.metadata->scanType),
                        frame.metadata->afd);

            UT_LOG_INFO("FrameRate: %d/%d",
                        frame.metadata->frameRateNumerator,
                        frame.metadata->frameRateDenominator);

            UT_LOG_INFO("Flags: bitstreamEOS=%d, discontinuity=%d, lowLatency=%d",
                        frame.metadata->bitstreamEOS,
                        frame.metadata->discontinuity,
                        frame.metadata->lowLatency);

            if (frame.metadata->bitstreamEOS)
            {
                UT_LOG_INFO("[Video DecoderID: %d] EOS received on final onFrameOutput callback: PTS=%lld, frameAVBufferHandle=0x%016" PRIx64,
                            mDecoderID, nsPresentationTime, static_cast<uint64_t>(frameAVBufferHandle));
            }

            UT_LOG_INFO("Source: %d", static_cast<int>(frame.metadata->source));

        } else {
            UT_LOG_INFO("No FrameMetadata provided");
            // Contract: metadata is GUARANTEED non-null on the EOS callback.
            // A null metadata callback is only valid for routine (non-EOS) frames.
        }

        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDecodedFrames.push_back(std::move(frame)); // Move is now possible
        }
        mCV.notify_all();
        return Status::ok();
    }

    Status onDecodeBufferAvailable() override
    {
        {
            std::lock_guard<std::mutex> lock(mDecodeBufferAvailableMutex);
            ++mDecodeBufferAvailableCount;
        }
        mDecodeBufferAvailableCV.notify_all();
        UT_LOG_INFO("[Video DecoderID: %d] onDecodeBufferAvailable received; count=%d", mDecoderID, getDecodeBufferAvailableCount());
        return Status::ok();
    }

    Status onEndOfStream() override
    {
        {
            std::lock_guard<std::mutex> lock(mEndOfStreamMutex);
            ++mEndOfStreamCount;
        }
        mEndOfStreamCV.notify_all();
        UT_LOG_INFO("[Video DecoderID: %d] onEndOfStream received; count=%d", mDecoderID, getEndOfStreamCount(false));
        return Status::ok();
    }

    int32_t getInterfaceVersion() override { return 1; }
    std::string getInterfaceHash() override { return "notfrozen"; }

    void setDecoderID(int32_t id)
    {
        mDecoderID = id;
    }
    Status onUserDataOutput(int64_t nsPresentationTime, const ::std::vector<uint8_t>& userData) override
    {
        UT_LOG_INFO("[Video DecoderID: %d] onUserDataOutput received", mDecoderID);
        return Status::ok();
    }

    bool getNextFrame(DecodedVideoFrame_t& frame, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mCV.wait_for(lock, timeout, [this]() { return !mDecodedFrames.empty(); })) {
            frame = std::move(mDecodedFrames.front()); // Move is now possible
            mDecodedFrames.pop_front();
            return true;
        }
        return false;
    }

    void resetQueues()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mDecodedFrames.clear();
        resetDecodeBufferAvailableCount();
        resetEndOfStreamCount();
    }

    bool waitForDecodeBufferAvailable(int expectedCount, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mDecodeBufferAvailableMutex);
        return mDecodeBufferAvailableCV.wait_for(lock, timeout, [this, expectedCount]() {
            return mDecodeBufferAvailableCount >= expectedCount;
        });
    }

    int getDecodeBufferAvailableCount() const
    {
        std::lock_guard<std::mutex> lock(mDecodeBufferAvailableMutex);
        return mDecodeBufferAvailableCount;
    }

    void resetDecodeBufferAvailableCount()
    {
        std::lock_guard<std::mutex> lock(mDecodeBufferAvailableMutex);
        mDecodeBufferAvailableCount = 0;
    }

    bool waitForEndOfStream(int expectedCount, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mEndOfStreamMutex);
        return mEndOfStreamCV.wait_for(lock, timeout, [this, expectedCount]() {
            return mEndOfStreamCount >= expectedCount;
        });
    }

    int getEndOfStreamCount(bool reset)
    {
        std::lock_guard<std::mutex> lock(mEndOfStreamMutex);
        int count = mEndOfStreamCount;
        if (reset)
        {
            mEndOfStreamCount = 0;
        }
        return count;
    }

    void resetEndOfStreamCount()
    {
        std::lock_guard<std::mutex> lock(mEndOfStreamMutex);
        mEndOfStreamCount = 0;
    }

private:
    int32_t mDecoderID = -1;
    std::mutex mMutex;
    std::condition_variable mCV;
    std::deque<DecodedVideoFrame_t> mDecodedFrames;
    mutable std::mutex mDecodeBufferAvailableMutex;
    std::condition_variable mDecodeBufferAvailableCV;
    int mDecodeBufferAvailableCount = 0;
    mutable std::mutex mEndOfStreamMutex;
    std::condition_variable mEndOfStreamCV;
    int mEndOfStreamCount = 0;
};

class VideoDecoderEventListener : public BnVideoDecoderEventListener
{
public:
    static int32_t mUsedDecoderID;
    VideoDecoderEventListener() {};
    virtual ~VideoDecoderEventListener() = default;

    Status onDecodeError(videodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override
    {
        UT_LOG_ERROR("[Video DecoderID: %d] onDecodeError: errorCode=%d, vendorErrorCode=%d", mDecoderID, static_cast<int32_t>(errorCode), vendorErrorCode);
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mErrors.push_back(static_cast<int32_t>(errorCode));
        }
        mCV.notify_all();
        return Status::ok();
    }
    Status onStateChanged(::com::rdk::hal::videodecoder::State oldState, ::com::rdk::hal::videodecoder::State newState) override
    {
        UT_LOG_INFO("[Video DecoderID: %d] onStateChanged: %s -> %s", mDecoderID, ::com::rdk::hal::videodecoder::toString(oldState).c_str(), ::com::rdk::hal::videodecoder::toString(newState).c_str());
        std::lock_guard<std::mutex> lock(mMutex);
        mStateChanges.push_back({oldState, newState});
        mCond.notify_one();
        return Status::ok();
    }

    int32_t getInterfaceVersion() override { return 1; }
    std::string getInterfaceHash() override { return "notfrozen"; }

    bool checkStateTransistion(::com::rdk::hal::videodecoder::State oldState, ::com::rdk::hal::videodecoder::State newState)
    {
        for(const auto& statePair : mStateChanges)
        {
            if(statePair.first == oldState && statePair.second == newState)
                return true;
        }
        return false;
    }

    std::vector<int32_t> getErrors() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mErrors;
    }

    int32_t getErrorCount() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mErrors.size();
    }

    void setDecoderID(int32_t id)
    {
        mDecoderID = id;
    }

    static int32_t getUsedDecoder()
    {
        return VideoDecoderEventListener::mUsedDecoderID;
    }

    void resetQueues()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStateChanges.clear();
        mErrors.clear();
    }

private:
    mutable std::mutex mMutex;
    std::condition_variable mCV;
    std::condition_variable mCond;

    ::com::rdk::hal::videodecoder::State mCurrentState;
    std::vector<std::pair<::com::rdk::hal::videodecoder::State, ::com::rdk::hal::videodecoder::State>> mStateChanges;
    std::vector<int32_t> mErrors;
    int32_t mDecoderID = -1;
};

// AVBufferSpaceListener declaration
class AVBufferSpaceListener : public BnAVBufferSpaceListener {
public:
    AVBufferSpaceListener() {}
    virtual ~AVBufferSpaceListener() = default;
    Status onSpaceAvailable() override;
};

// DecoderTest declaration
class videoDecoderTest {
public:
    int32_t mEOSignaledCount = 0;
    videoDecoderTest(IVideoDecoder::Id decoderId, sp<IVideoDecoder> decoderHandle,
                sp<VideoDecoderControllerListener> controlListener,
                sp<VideoDecoderEventListener> eventListener,
                sp<IAVBuffer> AVBufferManager,
                bool secureDecode, bool tunnelMode, const videoDecoderTestConfiguration_t& config,
                IAVBufferHelper* avbufferHelper, bool isSinkAttached)
    {
        mDecoderId = decoderId;
        mDecoderHandle = std::move(decoderHandle);
        mControlListener = std::move(controlListener);
        mSecureDecode = secureDecode;
        mEventListener = std::move(eventListener);
        mTestConfiguration = config;
        mAVBufferManager = std::move(AVBufferManager);
        mAVbufferHelper = avbufferHelper;
        mIsSinkAttached = isSinkAttached;
        mTunnelMode = tunnelMode;
        mEventListener->setDecoderID(decoderId.value);
        mControlListener->setDecoderID(decoderId.value);
    }
    void run(bool concurrentDecodeConsume);
    /* rdk-halif-aidl 0.23.0: wait until the decoder has started and the vendor-selected operational
     * mode has been read (getCurrentOperationalMode()). Returns false if the decoder did not reach
     * STARTED (setup failed) or the timeout expired. Used by sink tests to follow the vendor's mode. */
    bool waitForOperationalMode(OperationalMode& mode, std::chrono::milliseconds timeout);
    bool runBackPressureTest();
    void setTestConfiguration(const videoDecoderTestConfiguration_t& config);
    void closeDecoderSession();
    void getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped);
    std::vector<std::tuple<uint64_t, uint32_t, int64_t>> parseFrames(
        const std::vector<uint8_t>& inputBuffer, uint64_t fileSize, videodecoder::Codec codec);

private:
    // Configuration parameters
    IVideoDecoder::Id mDecoderId;
    bool mSecureDecode;
    bool mTunnelMode;
    // Operational mode selected by the vendor (read after start(); rdk-halif-aidl 0.23.0)
    OperationalMode mCurrentMode = OperationalMode::NON_TUNNELLED;
    enum class ModeState { PENDING, KNOWN, UNAVAILABLE };
    std::mutex mModeMutex;
    std::condition_variable mModeCv;
    ModeState mModeState = ModeState::PENDING;
    void publishOperationalMode(ModeState state);
    bool mConcurrentDecodeConsume = false;
    bool mIsSinkAttached;
    videoDecoderTestConfiguration_t mTestConfiguration;
    sp<IVideoDecoder> mDecoderHandle;
    sp<VideoDecoderControllerListener> mControlListener;
    sp<VideoDecoderEventListener> mEventListener;
    sp<IAVBuffer> mAVBufferManager;
    IAVBufferHelper* mAVbufferHelper;

    // test Execution parameters
    deque<InputVideoFrame_t> mInputFrame;
    std::vector<std::tuple<uint64_t, uint32_t, int64_t, bool>> mFrameData; // {offset, size, pts, isKeyFrame}
    std::vector<uint8_t> mInputBuffer;
    sp<IVideoDecoderController> mVideoDecoderController;
    Pool mVideoPool;
    sp<IAVBufferSpaceListener> mBufferListener;

    std::mutex mInputFrameMutex;

private:
    bool makeFrame();
    void setupVideoDecoder();
    // Helpers for setupVideoDecoder(), one per setup concern:
    void openOrReuseVideoDecoder();
    void checkOperationalModeIfNeeded();
    void buildAndApplyStreamConfig();
    void applyDecodeErrorPolicyIfNeeded();
    void applyLowLatencyModeIfNeeded();
    void startDecoding();
    bool validateDecodedFrame();
    // Helpers for validateDecodedFrame(), one per validation concern:
    bool validateFrameLatencyAndBuffer(const DecodedVideoFrame_t& decodedFrame, std::set<int64_t>& receivedPts,
                                        int32_t& totalOnFramesDecoded);
    void validateFrameMetadata(const DecodedVideoFrame_t& decodedFrame, int32_t& discontinuityFramesReceived);
    void validateDiscontinuityAndEndOfStream(int32_t discontinuityFramesReceived, int32_t expectedFrames,
                                              int32_t expectedEOSFrames, bool clientFramesReturned = true);
    void validateFinalMetrics(int32_t totalOnFramesDecoded, int32_t totalOnErrorsReceived, int32_t expectedFrames,
                              bool clientFramesReturned = true);
    void validateTunnelledSession();
    void releaseDecoderSession(bool closeController);
    void teardownDecoder(bool closeDecoderSession = false);
    void decodeAndConsumeVideoFrames();
};

#endif //__VTS_VIDEODECODER_H__

/** @} */ // End of Video_Decoder_Halif_Test_Header
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK
