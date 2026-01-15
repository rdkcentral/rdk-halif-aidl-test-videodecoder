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
 * @defgroup Video_Decoder_Halif_Test_Util Video Decoder HALIF Tests util File
 * @{
 * @parblock
 *
 * ### utility functions for Video Decoder VTS HALIF AIDL tests :
 *
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
 * @file vts_videodecoder_util.cpp
 *
 */

#include "vts_videodecoder.h"

using namespace std;

sp<IVideoDecoderManager> gVideoDecoderManager;
std::vector<decoder_t> gVideodecoders_hfp;

UT_status_t getVideoDecoderService() {
    gVideoDecoderManager = android::interface_cast<IVideoDecoderManager>(
       defaultServiceManager()->getService(String16(IVideoDecoderManager::serviceName().c_str())));
   if (gVideoDecoderManager == nullptr) {
       UT_LOG_ERROR("Video Decoder Service is not available");
       return UT_STATUS_FAILURE;
   }
   return UT_STATUS_OK;
}


DynamicRange videoDecoderStringToDynamicRange(const std::string& rangeStr) {
    static const std::unordered_map<std::string, DynamicRange> dynamicRangeMap = {
        {"UNKNOWN", DynamicRange::UNKNOWN},
        {"SDR", DynamicRange::SDR},
        {"HLG", DynamicRange::HLG},
        {"HDR10", DynamicRange::HDR10},
        {"HDR10PLUS", DynamicRange::HDR10PLUS},
        {"DOLBY_VISION", DynamicRange::DOLBY_VISION}
    };
    auto it = dynamicRangeMap.find(rangeStr);
    return (it != dynamicRangeMap.end()) ? it->second : DynamicRange::UNKNOWN;
}

ScanType videoDecoderStringToScanType(const std::string& scanTypeStr) {
    static const std::unordered_map<std::string, ScanType> scanTypeMap = {
        {"PROGRESSIVE_FRAME", ScanType::PROGRESSIVE_FRAME},
        {"INTERLACED_FRAME", ScanType::INTERLACED_FRAME}
    };
    auto it = scanTypeMap.find(scanTypeStr);
    return (it != scanTypeMap.end()) ? it->second : ScanType::PROGRESSIVE_FRAME; // Default fallback
}

OperationalMode videoDecoderStringToOperationalMode(const std::string& modeStr) {
    static const std::unordered_map<std::string, OperationalMode> modeMap = {
        {"TUNNELLED", OperationalMode::TUNNELLED},
        {"NON_TUNNELLED", OperationalMode::NON_TUNNELLED},
        {"GRAPHICS_TEXTURE", OperationalMode::GRAPHICS_TEXTURE}
    };

    auto it = modeMap.find(modeStr);
    if (it != modeMap.end()) {
        return it->second;  // Return the integer value instead of enum
    } else {
        return OperationalMode::TUNNELLED;  // Return TUNNELLED for unknown/default modes
    }
}

videodecoder::CodecProfile videoDecoderStringToCodecProfile(const std::string& profileStr) {
    static const std::unordered_map<std::string, videodecoder::CodecProfile> profileMap = {
        // MPEG-2 Profiles
        {"MPEG2_SIMPLE", videodecoder::CodecProfile::MPEG2_SIMPLE},
        {"MPEG2_MAIN", videodecoder::CodecProfile::MPEG2_MAIN},
        // H.264/AVC Profiles
        {"H264_BASELINE", videodecoder::CodecProfile::H264_BASELINE},
        {"H264_MAIN", videodecoder::CodecProfile::H264_MAIN},
        {"H264_HIGH", videodecoder::CodecProfile::H264_HIGH},
        // H.265/HEVC Profiles
        {"H265_MAIN", videodecoder::CodecProfile::H265_MAIN},
        {"H265_MAIN_10", videodecoder::CodecProfile::H265_MAIN_10},
        {"H265_MAIN_10_HDR10", videodecoder::CodecProfile::H265_MAIN_10_HDR10},
        // VP9 Profiles
        {"VP9_PROFILE_0", videodecoder::CodecProfile::VP9_PROFILE_0},
        {"VP9_PROFILE_1", videodecoder::CodecProfile::VP9_PROFILE_1},
        {"VP9_PROFILE_2", videodecoder::CodecProfile::VP9_PROFILE_2},
        {"VP9_PROFILE_3", videodecoder::CodecProfile::VP9_PROFILE_3},
        // AV1 Profiles
        {"AV1_MAIN", videodecoder::CodecProfile::AV1_MAIN},
        {"AV1_HIGH", videodecoder::CodecProfile::AV1_HIGH}
    };
    auto it = profileMap.find(profileStr);
    if (it != profileMap.end()) {
        return it->second;
    } else {
        UT_LOG_WARNING("Unknown codec profile: %s, using default", profileStr.c_str());
        return static_cast<videodecoder::CodecProfile>(0);
    }
}

videodecoder::CodecLevel videoDecoderStringToCodecLevel(const std::string& levelStr) {
    static const std::unordered_map<std::string, videodecoder::CodecLevel> levelMap = {
        // MPEG-2 Levels
        {"MPEG2_LEVEL_LOW", videodecoder::CodecLevel::MPEG2_LEVEL_LOW},
        {"MPEG2_LEVEL_MAIN", videodecoder::CodecLevel::MPEG2_LEVEL_MAIN},
        {"MPEG2_LEVEL_HIGH", videodecoder::CodecLevel::MPEG2_LEVEL_HIGH},
        // H.264 Levels
        {"H264_LEVEL_3", videodecoder::CodecLevel::H264_LEVEL_3},
        {"H264_LEVEL_3_1", videodecoder::CodecLevel::H264_LEVEL_3_1},
        {"H264_LEVEL_4", videodecoder::CodecLevel::H264_LEVEL_4},
        {"H264_LEVEL_4_1", videodecoder::CodecLevel::H264_LEVEL_4_1},
        {"H264_LEVEL_5", videodecoder::CodecLevel::H264_LEVEL_5},
        {"H264_LEVEL_5_1", videodecoder::CodecLevel::H264_LEVEL_5_1},
        {"H264_LEVEL_5_2", videodecoder::CodecLevel::H264_LEVEL_5_2},
        // H.265 Levels
        {"H265_LEVEL_4", videodecoder::CodecLevel::H265_LEVEL_4},
        {"H265_LEVEL_4_1", videodecoder::CodecLevel::H265_LEVEL_4_1},
        {"H265_LEVEL_5", videodecoder::CodecLevel::H265_LEVEL_5},
        {"H265_LEVEL_5_1", videodecoder::CodecLevel::H265_LEVEL_5_1},
        {"H265_LEVEL_5_2", videodecoder::CodecLevel::H265_LEVEL_5_2},
        {"H265_LEVEL_6", videodecoder::CodecLevel::H265_LEVEL_6},
        {"H265_LEVEL_6_1", videodecoder::CodecLevel::H265_LEVEL_6_1},
        {"H265_LEVEL_6_2", videodecoder::CodecLevel::H265_LEVEL_6_2},
        // VP9 Levels
        {"VP9_LEVEL_1", videodecoder::CodecLevel::VP9_LEVEL_1},
        {"VP9_LEVEL_1_1", videodecoder::CodecLevel::VP9_LEVEL_1_1},
        {"VP9_LEVEL_2", videodecoder::CodecLevel::VP9_LEVEL_2},
        {"VP9_LEVEL_2_1", videodecoder::CodecLevel::VP9_LEVEL_2_1},
        {"VP9_LEVEL_3", videodecoder::CodecLevel::VP9_LEVEL_3},
        {"VP9_LEVEL_3_1", videodecoder::CodecLevel::VP9_LEVEL_3_1},
        {"VP9_LEVEL_4", videodecoder::CodecLevel::VP9_LEVEL_4},
        {"VP9_LEVEL_4_1", videodecoder::CodecLevel::VP9_LEVEL_4_1},
        {"VP9_LEVEL_5", videodecoder::CodecLevel::VP9_LEVEL_5},
        {"VP9_LEVEL_5_1", videodecoder::CodecLevel::VP9_LEVEL_5_1},
        {"VP9_LEVEL_5_2", videodecoder::CodecLevel::VP9_LEVEL_5_2},
        {"VP9_LEVEL_6", videodecoder::CodecLevel::VP9_LEVEL_6},
        {"VP9_LEVEL_6_1", videodecoder::CodecLevel::VP9_LEVEL_6_1},
        {"VP9_LEVEL_6_2", videodecoder::CodecLevel::VP9_LEVEL_6_2},
        // AV1 Levels
        {"AV1_LEVEL_4_0", videodecoder::CodecLevel::AV1_LEVEL_4_0},
        {"AV1_LEVEL_4_1", videodecoder::CodecLevel::AV1_LEVEL_4_1},
        {"AV1_LEVEL_5_0", videodecoder::CodecLevel::AV1_LEVEL_5_0},
        {"AV1_LEVEL_5_1", videodecoder::CodecLevel::AV1_LEVEL_5_1},
        {"AV1_LEVEL_6_0", videodecoder::CodecLevel::AV1_LEVEL_6_0},
        {"AV1_LEVEL_6_1", videodecoder::CodecLevel::AV1_LEVEL_6_1}
    };
    auto it = levelMap.find(levelStr);
    return (it != levelMap.end()) ? it->second : static_cast<videodecoder::CodecLevel>(0);
}

PixelFormat videoDecoderStringToPixelFormat(const std::string& formatStr) {
    static const std::unordered_map<std::string, PixelFormat> pixelFormatMap = {
        {"YCBCR422", PixelFormat::YCBCR422},
        {"YCBCR444", PixelFormat::YCBCR444},
        {"YCBCR420", PixelFormat::YCBCR420},
        {"RGB", PixelFormat::RGB},
        {"NATIVE", PixelFormat::NATIVE}
    };
    auto it = pixelFormatMap.find(formatStr);
    if (it != pixelFormatMap.end()) {
        return it->second;
    }
    return static_cast<PixelFormat>(0); // Unknown format
}

bool vts_videodecoder_helper_profileFieldPresent(const std::string& key) {
    // Equivalent to ut_kvp_fieldPresent but for profile instance
    // Try to get string field and check status - this is more reliable than checking return values
    return ut_kvp_fieldPresent(ut_kvp_profile_getInstance(), key.c_str());
}

bool vts_videodecoder_helper_checkCodecAttributeExists(const std::string& attrpath, const std::string& codec_name) {
    // Check all required fields exist (we already know profile exists from detection phase)
    const std::vector<std::string> requiredFields = {
        ".maxFrameRate",
        ".maxFrameWidth",
        ".maxFrameHeight",
        ".profile",
        ".level"
    };

    // Check if all required fields exist
    for (const auto& field : requiredFields) {
        if (!vts_videodecoder_helper_profileFieldPresent(attrpath + field)) {
            UT_LOG_DEBUG("Codec %s missing required field: %s", codec_name.c_str(), field.c_str());
            return false;
        }
    }

    // All fields exist - now validate their values are reasonable
    uint32_t maxFrameRate = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameRate").c_str());
    uint32_t maxFrameWidth = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameWidth").c_str());
    uint32_t maxFrameHeight = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameHeight").c_str());

    char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
    UT_KVP_PROFILE_GET_STRING((attrpath + ".profile").c_str(), result_kvp);
    std::string profileStr = std::string(result_kvp);

    memset(result_kvp, 0, UT_KVP_MAX_ELEMENT_SIZE);
    UT_KVP_PROFILE_GET_STRING((attrpath + ".level").c_str(), result_kvp);
    std::string levelStr = std::string(result_kvp);

    // Validate values are reasonable
    // Note: Not requiring frameRate > 0 since 0 could be valid for special cases
    bool isValid = (maxFrameWidth > 0 && maxFrameHeight > 0 &&
                   !profileStr.empty() && !levelStr.empty());

    if (isValid) {
        UT_LOG_DEBUG("Codec %s has valid attributes: rate=%u, width=%u, height=%u, profile=%s, level=%s",
                     codec_name.c_str(), maxFrameRate, maxFrameWidth, maxFrameHeight,
                     profileStr.c_str(), levelStr.c_str());
    } else {
        UT_LOG_DEBUG("Codec %s has invalid attribute values: rate=%u, width=%u, height=%u, profile=%s, level=%s",
                     codec_name.c_str(), maxFrameRate, maxFrameWidth, maxFrameHeight,
                     profileStr.c_str(), levelStr.c_str());
    }

    return isValid;
}

bool vts_videodecoder_helper_validatePropertyValue(int decoder_id,const std::optional<PropertyValue>& propertyValue, const videodecoder::Property property) {
    // Check if the property value is present and contains a valid value
    if (!propertyValue.has_value() || !propertyValue->value.has_value()) {
        return false;
    }

    // Extract the value into a variant (only int32_t is expected per Property.aidl)
    std::variant<int32_t> val;
    const auto& propVal = propertyValue->value.value();
    switch (propVal.getTag()) {
        case PropertyValue::Value::Tag::intValue:
            val = propVal.get<PropertyValue::Value::Tag::intValue>();
            break;
        default:
            std::cerr << "Unsupported type in PropertyValue::Value for video decoder property" << std::endl;
            return false;  // All properties should be intValue per AIDL
    }

    // Validate based on the specific property
    switch (property) {
        case videodecoder::Property::RESOURCE_ID:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) == decoder_id;
        case videodecoder::Property::INPUT_QUEUE_DEPTH:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;  // Non-negative size
        case videodecoder::Property::OUTPUT_FRAME_POOL_SIZE:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;  // Non-negative size
        case videodecoder::Property::OPERATIONAL_MODE:
            return std::holds_alternative<int32_t>(val) &&(std::get<int32_t>(val) == static_cast<int32_t>(OperationalMode::TUNNELLED) ||
                std::get<int32_t>(val) == static_cast<int32_t>(OperationalMode::NON_TUNNELLED) ||
                std::get<int32_t>(val) == static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE) ||
                // Check for valid OR combinations (GRAPHICS_TEXTURE with TUNNELLED or NON_TUNNELLED)
                std::get<int32_t>(val) == (static_cast<int32_t>(OperationalMode::TUNNELLED) |
                static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE)) ||
                std::get<int32_t>(val) == (static_cast<int32_t>(OperationalMode::NON_TUNNELLED) |
                static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE)));
        case videodecoder::Property::LOW_LATENCY_MODE:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case videodecoder::Property::DECODE_ERROR_POLICY:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case videodecoder::Property::AV_SOURCE:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;  // Assuming AVSource enum starts at 0
        case videodecoder::Property::SHA1_CALC:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case videodecoder::Property::SECURE_VIDEO:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case videodecoder::Property::METRIC_FRAMES_DECODED:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;  // -1 for not implemented
        case videodecoder::Property::METRIC_DECODE_ERRORS:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;  // -1 for not implemented
        case videodecoder::Property::METRIC_FRAMES_DROPPED:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;  // -1 for not implemented
        default:
            return true;  // Default to true for unhandled properties (future-proofing)
    }
}


// Maximum decoder ID to probe for in YAML files (supports IDs 0-10)
constexpr int32_t MAX_DECODER_ID_PROBE = 10;

UT_status_t vts_videodecoder_parseVideoDecoderHFP()
{
    std::string videodecoderpath = VIDEODECODERMGR_IVIDEODECODER;
    std::string opmodepath = VIDEODECODERMGR_SUPPORTED_OPERATIONAL_MODES;
    int32_t number_of_videodecoders;
    int32_t supportedopmode_count;
    decoder_t temporary = {};

    UT_LOG_INFO("Parsing HFP from: %s", videodecoderpath.c_str());

    number_of_videodecoders = UT_KVP_PROFILE_GET_LIST_COUNT(videodecoderpath.c_str());

    if (number_of_videodecoders <= 0)
    {
        UT_LOG_ERROR("No videodecoders found in HFP at path: %s", videodecoderpath.c_str());
        return UT_STATUS_FAILURE;
    }

    supportedopmode_count = UT_KVP_PROFILE_GET_LIST_COUNT(opmodepath.c_str());
    UT_LOG_INFO("Operational modes count: %d", supportedopmode_count);
    if (supportedopmode_count <= 0)
    {
        UT_LOG_ERROR("No operational modes found at path: %s", opmodepath.c_str());
        return UT_STATUS_FAILURE;
    }

    temporary.supportedOperationalModes.clear();
    for (int32_t j = 0; j < supportedopmode_count; ++j)
    {
        char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
        std::string modepath = opmodepath + "." + std::to_string(j);
        UT_KVP_PROFILE_GET_STRING(modepath.c_str(), result_kvp);
        std::string modeStr = std::string(result_kvp);
        UT_LOG_INFO("Reading opmode path: %s, value: %s", modepath.c_str(), modeStr.c_str());
        if (!modeStr.empty()) {
            try {
                temporary.supportedOperationalModes.push_back(videoDecoderStringToOperationalMode(modeStr));
            } catch (const std::invalid_argument& e) {
                UT_LOG_WARNING("Invalid opmode at path: %s, value: %s, error: %s", modepath.c_str(), modeStr.c_str(), e.what());
            }
        } else {
            UT_LOG_WARNING("Empty opmode at path: %s", modepath.c_str());
        }
    }
    UT_LOG_INFO("Parsed %zu operational modes", temporary.supportedOperationalModes.size());
    if (temporary.supportedOperationalModes.empty()) {
        UT_LOG_WARNING("No valid operational modes parsed for decoder");
    }

    // Get all known codec names from the centralized mapping
    const std::vector<std::string> known_codecs = videoDecoderGetAllCodecNames();

    gVideodecoders_hfp.clear();
    UT_LOG_INFO("Cleared gVideodecoders_hfp, capacity: %zu", gVideodecoders_hfp.capacity());
    UT_LOG_INFO("Will search for these codec types: %s",
                [&known_codecs]() -> std::string {
                    std::string result;
                    for (size_t i = 0; i < known_codecs.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += known_codecs[i];
                    }
                    return result;
                }().c_str());

    for (int32_t i = 0; i < number_of_videodecoders; ++i)
    {
        std::string decoderbasepath = videodecoderpath + "." + std::to_string(i);
        
        // The YAML structure uses integer keys (e.g., "- 0:", "- 1:", "- 2:") to identify decoders
        // These keys are stored as dictionary entries alongside other fields like supportedCodecs
        // We need to probe for which numeric key exists to determine the actual decoder ID
        int32_t decoder_id = -1;
        for (int32_t probe_id = 0; probe_id <= MAX_DECODER_ID_PROBE; ++probe_id) {
            std::string id_probe_path = decoderbasepath + "." + std::to_string(probe_id);
            if (vts_videodecoder_helper_profileFieldPresent(id_probe_path)) {
                decoder_id = probe_id;
                UT_LOG_INFO("Found decoder ID %d at array index %d (path: %s)", decoder_id, i, id_probe_path.c_str());
                break;
            }
        }
        
        // If no numeric key was found (0-10), fall back to using array index as ID
        if (decoder_id < 0) {
            decoder_id = i;
            UT_LOG_WARNING("No numeric key found for decoder at index %d, using index as ID", i);
        }
        
        std::string supportedcodecpath = decoderbasepath + ".supportedCodecs";
        std::string supporteddrpath = decoderbasepath + ".supportedDynamicRanges";
        std::string supportsSecurepath = decoderbasepath + ".supportsSecure";

        temporary.id = decoder_id;
        temporary.supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
        temporary.supportedCodecs.clear();
        temporary.supportedDynamicRanges.clear();

        UT_LOG_INFO("Parsing Decoder ID: %d (array index: %d), supportsSecure: %d", temporary.id, i, temporary.supportsSecure);

        int supportedcodec_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodecpath.c_str());
        UT_LOG_INFO("Decoder %d (array index %d): codec count=%d", temporary.id, i, supportedcodec_count);
        for (int32_t j = 0; j < supportedcodec_count; ++j)
        {
            std::string codecpath = supportedcodecpath + "." + std::to_string(j);

            UT_LOG_DEBUG("Checking codec slot %d at path: %s", j, codecpath.c_str());

            // First, efficiently detect which codec exists at this slot by checking for any codec-specific path
            std::string detected_codec;
            for (const auto& codec_name : known_codecs)
            {
                std::string test_path = codecpath + "." + codec_name + ".profile";
                if (vts_videodecoder_helper_profileFieldPresent(test_path))
                {
                    detected_codec = codec_name;
                    break; // Found the codec type, no need to check others
                }
            }

            if (detected_codec.empty())
            {
                UT_LOG_DEBUG("No recognized codec found at slot %d, skipping", j);
                continue;
            }

            UT_LOG_DEBUG("Detected codec %s at slot %d", detected_codec.c_str(), j);

            // Now parse the detected codec's attributes
            std::string attrpath = codecpath + "." + detected_codec;

            // Validate all required attributes exist before parsing
            if (!vts_videodecoder_helper_checkCodecAttributeExists(attrpath, detected_codec))
            {
                UT_LOG_WARNING("Codec %s at slot %d missing required attributes, skipping", detected_codec.c_str(), j);
                continue;
            }

            UT_LOG_INFO("Found valid codec at path: %s, codec: %s", codecpath.c_str(), detected_codec.c_str());

            CodecCapability cap;
            char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};

            try {
                cap.codec = videoDecoderStringToCodec(detected_codec);
            } catch (const std::invalid_argument& e) {
                UT_LOG_WARNING("Invalid codec: %s, error: %s, skipping", detected_codec.c_str(), e.what());
                continue;
            }

            // Profile
            std::string profilepath = attrpath + ".profile";
            UT_KVP_PROFILE_GET_STRING(profilepath.c_str(), result_kvp);
            std::string profileStr = std::string(result_kvp);
            if (!profileStr.empty()) {
                cap.profile = videoDecoderStringToCodecProfile(profileStr);
            } else {
                UT_LOG_WARNING("Empty profile for codec %s", detected_codec.c_str());
            }

            // Level
            std::string levelpath = attrpath + ".level";
            memset(result_kvp, 0, UT_KVP_MAX_ELEMENT_SIZE);
            UT_KVP_PROFILE_GET_STRING(levelpath.c_str(), result_kvp);
            std::string levelStr = std::string(result_kvp);
            if (!levelStr.empty()) {
                cap.level = videoDecoderStringToCodecLevel(levelStr);
            } else {
                UT_LOG_WARNING("Empty level for codec %s", detected_codec.c_str());
            }

            cap.maxFrameRate = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameRate").c_str());
            cap.maxFrameWidth = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameWidth").c_str());
            cap.maxFrameHeight = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameHeight").c_str());

            UT_LOG_INFO("Codec %s: profile=%d, level=%d, rate=%d, width=%d, height=%d",
                        detected_codec.c_str(), static_cast<int32_t>(cap.profile), static_cast<int32_t>(cap.level),
                        cap.maxFrameRate, cap.maxFrameWidth, cap.maxFrameHeight);

            temporary.supportedCodecs.push_back(cap);
        }

        int supporteddr_count = UT_KVP_PROFILE_GET_LIST_COUNT(supporteddrpath.c_str());
        UT_LOG_INFO("Decoder %d: dynamic range count=%d", i, supporteddr_count);
        for (int32_t j = 0; j < supporteddr_count; ++j)
        {
            char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
            std::string drpath = supporteddrpath + "." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(drpath.c_str(), result_kvp);
            std::string drStr = std::string(result_kvp);
            UT_LOG_INFO("Reading dynamic range path: %s, value: %s", drpath.c_str(), drStr.c_str());
            if (!drStr.empty()) {
                try {
                    temporary.supportedDynamicRanges.push_back(videoDecoderStringToDynamicRange(drStr));
                } catch (const std::invalid_argument& e) {
                    UT_LOG_WARNING("Invalid dynamic range at path: %s, value: %s, error: %s",
                                  drpath.c_str(), drStr.c_str(), e.what());
                }
            } else {
                UT_LOG_WARNING("Empty dynamic range at path: %s", drpath.c_str());
            }
        }

        UT_LOG_INFO("Decoder %d: id=%d, %zu codecs, %zu dynamic ranges, %zu opmodes",
                    i, temporary.id, temporary.supportedCodecs.size(),
                    temporary.supportedDynamicRanges.size(), temporary.supportedOperationalModes.size());

        if (temporary.supportedCodecs.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid codecs, skipping", i);
            continue;
        }
        if (temporary.supportedDynamicRanges.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid dynamic ranges, skipping", i);
            continue;
        }

        gVideodecoders_hfp.push_back(temporary);
    }

    if (gVideodecoders_hfp.empty()) {
        UT_LOG_ERROR("No valid decoders parsed from HFP");
        return UT_STATUS_FAILURE;
    }

    UT_LOG_INFO("Successfully parsed %zu decoders", gVideodecoders_hfp.size());
    for (const auto& dec : gVideodecoders_hfp) {
        UT_LOG_INFO("HFP Decoder ID: %d, codecs: %zu, dynamic ranges: %zu, opmodes: %zu",
                    dec.id, dec.supportedCodecs.size(), dec.supportedDynamicRanges.size(),
                    dec.supportedOperationalModes.size());
    }
    return UT_STATUS_OK;
}

size_t vts_bytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::YCBCR420: return 1.5; // Y + 0.25*Cb + 0.25*Cr
        case PixelFormat::YCBCR422: return 2;   // Y + 0.5*Cb + 0.5*Cr
        case PixelFormat::YCBCR444: return 3;   // Y + Cb + Cr
        case PixelFormat::RGB: return 3;        // R + G + B
        case PixelFormat::NATIVE: return 3;     // Assume 3 bytes
        default: return 1;
    }
}

double vts_computeVideoRMSDiff(const uint8_t* ref, const uint8_t* dec, size_t width, size_t height, PixelFormat format) {
    double sum = 0.0;
    size_t componentCount = 0;

    if (format == PixelFormat::RGB || format == PixelFormat::NATIVE) {
        // RGB or NATIVE: 3 bytes per pixel (R, G, B)
        size_t pixelCount = width * height;
        size_t byteCount = pixelCount * 3;
        for (size_t i = 0; i < byteCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
    } else if (format == PixelFormat::YCBCR444) {
        // YCBCR444: 3 bytes per pixel (Y, Cb, Cr)
        size_t pixelCount = width * height;
        size_t byteCount = pixelCount * 3;
        for (size_t i = 0; i < byteCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
    } else if (format == PixelFormat::YCBCR422) {
        // YCBCR422: 4:2:2, 2 bytes per pixel (Y0, Y1, Cb, Cr for 2 pixels)
        size_t byteOffset = 0;
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; x += 2) {
                // Y0
                double diffY0 = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffY0 * diffY0;
                componentCount++;
                byteOffset++;
                // Cb
                double diffCb = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffCb * diffCb;
                componentCount++;
                byteOffset++;
                // Y1
                double diffY1 = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffY1 * diffY1;
                componentCount++;
                byteOffset++;
                // Cr
                double diffCr = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffCr * diffCr;
                componentCount++;
                byteOffset++;
            }
        }
    } else if (format == PixelFormat::YCBCR420) {
        // YCBCR420: 4:2:0, 1.5 bytes per pixel (Y for each pixel, Cb/Cr for 2x2 block)
        size_t pixelCount = width * height;
        size_t yCount = pixelCount;
        size_t chromaCount = pixelCount / 4; // One Cb/Cr per 4 Y
        // Y plane
        for (size_t i = 0; i < yCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
        // Cb plane
        size_t cbOffset = yCount;
        for (size_t i = 0; i < chromaCount; ++i) {
            double diff = static_cast<double>(ref[cbOffset + i]) - static_cast<double>(dec[cbOffset + i]);
            sum += diff * diff;
            componentCount++;
        }
        // Cr plane
        size_t crOffset = yCount + chromaCount;
        for (size_t i = 0; i < chromaCount; ++i) {
            double diff = static_cast<double>(ref[crOffset + i]) - static_cast<double>(dec[crOffset + i]);
            sum += diff * diff;
            componentCount++;
        }
    } else {
        // Unknown format: treat as single-component
        size_t byteCount = width * height;
        for (size_t i = 0; i < byteCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
    }

    if (componentCount == 0) {
        return 0.0; // Avoid division by zero
    }
    return std::sqrt(sum / componentCount);
}

void vts_videodecoder_helper_validateCapabilities(
    int32_t decoderId,
    const videodecoder::Capabilities& capabilities)
{
    UT_LOG_INFO("Validating capabilities for decoder ID: %d", decoderId);

    // Basic validation: non-empty codecs and dynamic ranges
    if (capabilities.supportedCodecs.empty()) {
        UT_LOG_ERROR("No codecs in capabilities for decoder ID: %d", decoderId);
        return; // Avoid crash
    }
    UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());
    UT_LOG_INFO("Capabilities has %zu codecs", capabilities.supportedCodecs.size());

    // Validate supportedCodecs
    for (const auto& codecCap : capabilities.supportedCodecs)
    {
        if (codecCap.maxFrameRate <= 0 || codecCap.maxFrameWidth <= 0 || codecCap.maxFrameHeight <= 0) {
            UT_LOG_ERROR("Invalid codec attributes: rate=%d, width=%d, height=%d",
                         codecCap.maxFrameRate, codecCap.maxFrameWidth, codecCap.maxFrameHeight);
            continue;
        }
        UT_ASSERT_TRUE(codecCap.maxFrameRate > 0);
        UT_ASSERT_TRUE(codecCap.maxFrameWidth > 0);
        UT_ASSERT_TRUE(codecCap.maxFrameHeight > 0);
        UT_LOG_INFO("Codec %d: profile=%d, level=%d, rate=%d, width=%d, height=%d",
                    static_cast<int32_t>(codecCap.codec),
                    static_cast<int32_t>(codecCap.profile),
                    static_cast<int32_t>(codecCap.level),
                    codecCap.maxFrameRate,
                    codecCap.maxFrameWidth,
                    codecCap.maxFrameHeight);
    }

    // Validate supportedDynamicRanges
    if (capabilities.supportedDynamicRanges.empty()) {
        UT_LOG_ERROR("No dynamic ranges in capabilities for decoder ID: %d", decoderId);
        return; // Avoid crash
    }
    UT_ASSERT_TRUE(!capabilities.supportedDynamicRanges.empty());
    UT_LOG_INFO("Capabilities has %zu dynamic ranges", capabilities.supportedDynamicRanges.size());

    // Optional: Validate against gVideodecoders_hfp (if populated)
    if (gVideodecoders_hfp.empty()) {
        UT_LOG_ERROR("gVideodecoders_hfp is empty for decoder ID: %d", decoderId);
        return; // Avoid crash
    }
    for (const auto& hfpDecoder : gVideodecoders_hfp)
    {
        if (hfpDecoder.id == decoderId)
        {
            UT_LOG_INFO("Found HFP entry for decoder ID: %d", decoderId);
            if (capabilities.supportedCodecs.size() != hfpDecoder.supportedCodecs.size()) {
                UT_LOG_ERROR("Codec size mismatch: capabilities=%zu, hfpDecoder=%zu",
                             capabilities.supportedCodecs.size(), hfpDecoder.supportedCodecs.size());
                return; // Avoid out-of-bounds
            }
            UT_ASSERT_EQUAL(capabilities.supportedCodecs.size(), hfpDecoder.supportedCodecs.size());

            if (capabilities.supportedDynamicRanges.size() != hfpDecoder.supportedDynamicRanges.size()) {
                UT_LOG_ERROR("Dynamic range size mismatch: capabilities=%zu, hfpDecoder=%zu",
                             capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());
                return; // Avoid out-of-bounds
            }
            UT_ASSERT_EQUAL(capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());

            UT_ASSERT_EQUAL(capabilities.supportsSecure, hfpDecoder.supportsSecure);

            // Validate codecs
            for (size_t i = 0; i < capabilities.supportedCodecs.size(); ++i)
            {
                UT_LOG_INFO("Validating codec index %zu", i);
                const auto& cap = capabilities.supportedCodecs[i];
                const auto& hfpCap = hfpDecoder.supportedCodecs[i];
                if (static_cast<int32_t>(cap.codec) != static_cast<int32_t>(hfpCap.codec)) {
                    UT_LOG_ERROR("Codec mismatch at index %zu: capabilities=%d, hfpDecoder=%d",
                                 i, static_cast<int32_t>(cap.codec), static_cast<int32_t>(hfpCap.codec));
                    continue;
                }
                UT_ASSERT_EQUAL(static_cast<int32_t>(cap.codec), static_cast<int32_t>(hfpCap.codec));
                UT_ASSERT_EQUAL(static_cast<int32_t>(cap.profile), static_cast<int32_t>(hfpCap.profile));
                UT_ASSERT_EQUAL(static_cast<int32_t>(cap.level), static_cast<int32_t>(hfpCap.level));
                UT_ASSERT_EQUAL(cap.maxFrameRate, hfpCap.maxFrameRate);
                UT_ASSERT_EQUAL(cap.maxFrameWidth, hfpCap.maxFrameWidth);
                UT_ASSERT_EQUAL(cap.maxFrameHeight, hfpCap.maxFrameHeight);
            }

            // Validate dynamic ranges
            for (size_t i = 0; i < capabilities.supportedDynamicRanges.size(); ++i)
            {
                UT_LOG_INFO("Validating dynamic range index %zu", i);
                if (static_cast<int32_t>(capabilities.supportedDynamicRanges[i]) !=
                    static_cast<int32_t>(hfpDecoder.supportedDynamicRanges[i])) {
                    UT_LOG_ERROR("Dynamic range mismatch at index %zu: capabilities=%d, hfpDecoder=%d",
                                 i, static_cast<int32_t>(capabilities.supportedDynamicRanges[i]),
                                 static_cast<int32_t>(hfpDecoder.supportedDynamicRanges[i]));
                    continue;
                }
                UT_ASSERT_EQUAL(static_cast<int32_t>(capabilities.supportedDynamicRanges[i]),
                                static_cast<int32_t>(hfpDecoder.supportedDynamicRanges[i]));
            }
            break;
        }
    }
    UT_LOG_INFO("Completed validation for decoder ID: %d", decoderId);
}

UT_status_t vts_videodecoder_parse_testconfig(void* pTestInstance, const char *module, const char *testGroup, const char *testName, vector<videoDecoderTestConfiguration_t>& testConfiguration)
{
    ut_kvp_status_t status;
    ut_kvp_instance_t* kvpTestInstance = (ut_kvp_instance_t*)pTestInstance;
    std::string streamInputPath;
    std::string streamRefPath;
    int32_t streamCount = 0;
    filesystem::path currentPath = filesystem::current_path();
    filesystem::path fullPath;
    string testList = string(module) + "." + string(testGroup) + "." + string(testName);
    string controlList = testList + "." + "Controls";
    string streamList = testList + "." + "Inputs" + "." + "Streams";
    string streamsDetails = string("VideoStreams");

    UT_LOG_INFO("Parsing test config for %s", testList.c_str());

    // Get input and reference paths
    char tempBuffer[UT_KVP_MAX_ELEMENT_SIZE] = {0};
    status = ut_kvp_getStringField(kvpTestInstance, "VIDEO_DECODER_STREAM_INPUT_PATH", tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
    if (status != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to get input_streams_path, status: %d", status);
        return UT_STATUS_FAILURE;
    }
    streamInputPath = tempBuffer;
    UT_LOG_INFO("streamInputPath: %s", streamInputPath.c_str());

    status = ut_kvp_getStringField(kvpTestInstance, "VIDEO_DECODER_STREAM_REF_PATH", tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
    if (status != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to get reference_streams_path, status: %d", status);
        return UT_STATUS_FAILURE;
    }
    streamRefPath = tempBuffer;
    UT_LOG_INFO("streamRefPath: %s", streamRefPath.c_str());

    streamCount = ut_kvp_getListCount(kvpTestInstance, streamList.c_str());
    if (streamCount <= 0)
    {
        UT_LOG_ERROR("No streams found in %s", streamList.c_str());
        return UT_STATUS_FAILURE;
    }
    UT_LOG_INFO("Found %d streams", streamCount);

    for (int32_t i = 0; i < streamCount; i++)
    {
        videoDecoderTestConfiguration_t testDetails = {};
        std::string resultKVP;
        vector<uint32_t> errorFrameNum;

        string streamName = streamList + "." + to_string(i) + ".Name";
        status = ut_kvp_getStringField(kvpTestInstance, streamName.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status != UT_KVP_STATUS_SUCCESS)
        {
            UT_LOG_ERROR("Failed to get stream name for index %d, status: %d", i, status);
            continue;
        }
        resultKVP = tempBuffer;
        UT_LOG_INFO("Stream name: %s", resultKVP.c_str());
        testDetails.streamName = resultKVP.c_str();

        string prefix = streamsDetails + "." + resultKVP + ".";
        string input = prefix + "input";
        string reference = prefix + "reference";
        string codec = prefix + "codec";
        string codedWidth = prefix + "codedWidth";
        string codedHeight = prefix + "codedHeight";
        string frameRateNumerator = prefix + "frameRateNumerator";
        string frameRateDenominator = prefix + "frameRateDenominator";
        string pixelFormat = prefix + "pixelFormat";
        string dynamicRange = prefix + "dynamic_range";
        string scanType = prefix + "dynamic_range";
        string colorDepth = prefix + "colorDepth";
        string bit_rate = prefix + "bit_rate";
        string noOfFrames = prefix + "no_of_frames";
        string decodeErrors = prefix + "decodeErrors";
        string framesDropped = prefix + "framesDropped";
        string frameSize = prefix + "frame_size";
        string errorFrames = prefix + "error_frames";
        string framePTS = prefix  + "PTS";
        string rmsDiff = streamList + "." + to_string(i) + ".rms_diff_threshold";
        string decodeTime = streamList + "." + to_string(i) + ".decode_time_threshold";
        string decoderDelay = controlList + ".delay_between_decode_calls";
        string stopPts = prefix + "stop_start_pts";
        string discontinuityPts = prefix + "signal_discontinuity_pts";
        string flushPts = prefix + "flush_pts";
        string flushResetPts = prefix + "flush_reset_pts";
        string eosPts = prefix + "eos_pts";
        string endOfStream = prefix + "endOfStream";
        string discontinuity = prefix + "discontinuity";
        string lowLatency = prefix + "lowLatency";

        // Input stream path
        status = ut_kvp_getStringField(kvpTestInstance, input.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            resultKVP = tempBuffer;
            fullPath = filesystem::path(streamInputPath) / resultKVP;
            UT_LOG_INFO("Full input file path: %s", fullPath.string().c_str());

            if (!filesystem::exists(fullPath))
            {
                UT_LOG_ERROR("Input file does not exist: %s", fullPath.string().c_str());
                continue;
            }

            testDetails.input = fullPath.string();
            UT_LOG_INFO("Input path set: %s", testDetails.input.c_str());
        }
        else
        {
            UT_LOG_ERROR("Failed to get input path for stream %s, status: %d", resultKVP.c_str(), status);
            continue;
        }

        // Codec
        status = ut_kvp_getStringField(kvpTestInstance, codec.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.codec = videoDecoderStringToCodec(string(tempBuffer));
        }

        // Resolution
        testDetails.codedWidth = ut_kvp_getUInt32Field(kvpTestInstance, codedWidth.c_str());
        testDetails.codedHeight = ut_kvp_getUInt32Field(kvpTestInstance, codedHeight.c_str());

        // Frame rate
        testDetails.frameRateNumerator = ut_kvp_getUInt32Field(kvpTestInstance, frameRateNumerator.c_str());
        testDetails.frameRateDenominator = ut_kvp_getUInt32Field(kvpTestInstance, frameRateDenominator.c_str());

        // Pixel format
        status = ut_kvp_getStringField(kvpTestInstance, pixelFormat.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.pixelFormat = videoDecoderStringToPixelFormat(string(tempBuffer));
        }

        // Dynamic range
        status = ut_kvp_getStringField(kvpTestInstance, dynamicRange.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.dynamicRange = videoDecoderStringToDynamicRange(string(tempBuffer));
        }

        // scanType
        status = ut_kvp_getStringField(kvpTestInstance, scanType.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.scanType = videoDecoderStringToScanType(string(tempBuffer));
        }

        // Color depth
        testDetails.colorDepth = ut_kvp_getUInt32Field(kvpTestInstance, colorDepth.c_str());

        // Bit rate
        testDetails.bit_rate = ut_kvp_getUInt32Field(kvpTestInstance, bit_rate.c_str());

        // Number of frames
        testDetails.no_of_frames = ut_kvp_getUInt32Field(kvpTestInstance, noOfFrames.c_str());

        // Number of Error frames
        testDetails.decodeErrors = ut_kvp_getUInt32Field(kvpTestInstance, decodeErrors.c_str());

        // Number of Dropped frames
        testDetails.framesDropped = ut_kvp_getUInt32Field(kvpTestInstance, framesDropped.c_str());

        // Error frames
        if (ut_kvp_fieldPresent(kvpTestInstance, errorFrames.c_str()))
        {
            int32_t count = ut_kvp_getListCount(kvpTestInstance, errorFrames.c_str());
            for (int32_t j = 0; j < count; j++)
            {
                string error = errorFrames + "." + to_string(j);
                uint32_t frameNum = ut_kvp_getUInt32Field(kvpTestInstance, error.c_str());
                if (frameNum >= 1 && frameNum <= testDetails.no_of_frames)
                {
                    errorFrameNum.push_back(frameNum);
                }
                else
                {
                    UT_LOG_WARNING("Invalid error frame index: %d, max: %d", frameNum, testDetails.no_of_frames);
                }
            }
        }

        // Frame details
        int32_t frameCount = ut_kvp_getListCount(kvpTestInstance, frameSize.c_str());
        UT_LOG_INFO("frameCount %d ", frameCount);

        for (int32_t j = 0; j < frameCount; j++)
        {
            string eachFrame = frameSize + "." + to_string(j);
            string eachPTS = framePTS + "." + to_string(j);
            int32_t size = ut_kvp_getUInt32Field(kvpTestInstance, eachFrame.c_str());
            int64_t PTS = ut_kvp_getUInt64Field(kvpTestInstance, eachPTS.c_str());
            bool errorFrame = (find(errorFrameNum.begin(), errorFrameNum.end(), j + 1) != errorFrameNum.end());
            testDetails.frameDetails.push_back({size, PTS, errorFrame});
            testDetails.expectedPts.push_back(PTS);
            UT_LOG_INFO("Frame %d: size=%d, PTS=%lld, errorFrame=%d", j, size, PTS, errorFrame);
        }

        // RMS difference threshold
        testDetails.rms_diff_threshold = ut_kvp_getDoubleField(kvpTestInstance, rmsDiff.c_str());

        // Decode time threshold
        testDetails.decode_time_threshold = ut_kvp_getUInt64Field(kvpTestInstance, decodeTime.c_str());

        // Decoder delay
        testDetails.decoderDelay = ut_kvp_getUInt32Field(kvpTestInstance, decoderDelay.c_str());

        // Control points
        testDetails.stopPts = ut_kvp_getUInt64Field(kvpTestInstance, stopPts.c_str());
        testDetails.discontinuityPts = ut_kvp_getUInt64Field(kvpTestInstance, discontinuityPts.c_str());
        testDetails.flushPts = ut_kvp_getUInt64Field(kvpTestInstance, flushPts.c_str());
        testDetails.flushResetPts = ut_kvp_getUInt64Field(kvpTestInstance, flushResetPts.c_str());
        testDetails.eosPts = ut_kvp_getUInt64Field(kvpTestInstance, eosPts.c_str());

        testDetails.endOfStream = ut_kvp_getUInt32Field(kvpTestInstance, endOfStream.c_str());
        testDetails.discontinuity = ut_kvp_getUInt32Field(kvpTestInstance, discontinuity.c_str());
        testDetails.lowLatency = ut_kvp_getUInt32Field(kvpTestInstance, lowLatency.c_str());

        testConfiguration.push_back(testDetails);
        UT_LOG_INFO("Added test configuration for stream %d, frameDetails size: %zu, expectedPts size: %zu",
                    i, testDetails.frameDetails.size(), testDetails.expectedPts.size());
    }

    if (testConfiguration.empty()) {
        UT_LOG_ERROR("No valid test configurations parsed");
        return UT_STATUS_FAILURE;
    }

    UT_LOG_INFO("Parsed %zu test configurations", testConfiguration.size());
    return UT_STATUS_OK;
}

/** @} */ // End of Video_Decoder_Halif_Test_Util
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK
