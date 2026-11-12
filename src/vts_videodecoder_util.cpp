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
        {"HDR10_PLUS", DynamicRange::HDR10_PLUS},
        {"DOLBY_VISION", DynamicRange::DOLBY_VISION}
    };
    auto it = dynamicRangeMap.find(rangeStr);
    return (it != dynamicRangeMap.end()) ? it->second : DynamicRange::UNKNOWN;
}

ColorRange videoDecoderStringToColorRange(const std::string& s) {
    static const std::unordered_map<std::string, ColorRange> m = {
        {"UNKNOWN", ColorRange::UNKNOWN},
        {"LIMITED", ColorRange::LIMITED},
        {"FULL",    ColorRange::FULL}
    };
    auto it = m.find(s);
    return (it != m.end()) ? it->second : ColorRange::UNKNOWN;
}

ColorMatrix videoDecoderStringToColorMatrix(const std::string& s) {
    static const std::unordered_map<std::string, ColorMatrix> m = {
        {"UNKNOWN",    ColorMatrix::UNKNOWN},
        {"IDENTITY",   ColorMatrix::IDENTITY},
        {"BT709",      ColorMatrix::BT709},
        {"UNSPECIFIED",ColorMatrix::UNSPECIFIED},
        {"BT601_625",  ColorMatrix::BT601_625},
        {"BT601_525",  ColorMatrix::BT601_525},
        {"SMPTE240M",  ColorMatrix::SMPTE240M},
        {"BT2020_NCL", ColorMatrix::BT2020_NCL},
        {"BT2020_CL",  ColorMatrix::BT2020_CL}
    };
    auto it = m.find(s);
    return (it != m.end()) ? it->second : ColorMatrix::UNKNOWN;
}

TransferCharacteristics videoDecoderStringToTransferCharacteristics(const std::string& s) {
    static const std::unordered_map<std::string, TransferCharacteristics> m = {
        {"UNKNOWN",          TransferCharacteristics::UNKNOWN},
        {"BT709",            TransferCharacteristics::BT709},
        {"UNSPECIFIED",      TransferCharacteristics::UNSPECIFIED},
        {"BT470M_GAMMA22",   TransferCharacteristics::BT470M_GAMMA22},
        {"BT470BG_GAMMA28",  TransferCharacteristics::BT470BG_GAMMA28},
        {"SMPTE170M",        TransferCharacteristics::SMPTE170M},
        {"SMPTE240M",        TransferCharacteristics::SMPTE240M},
        {"LINEAR",           TransferCharacteristics::LINEAR},
        {"LOG_100",          TransferCharacteristics::LOG_100},
        {"LOG_316",          TransferCharacteristics::LOG_316},
        {"IEC61966_2_4",     TransferCharacteristics::IEC61966_2_4},
        {"BT1361",           TransferCharacteristics::BT1361},
        {"SRGB",             TransferCharacteristics::SRGB},
        {"BT2020_10",        TransferCharacteristics::BT2020_10},
        {"BT2020_12",        TransferCharacteristics::BT2020_12},
        {"SMPTE2084_PQ",     TransferCharacteristics::SMPTE2084_PQ},
        {"SMPTE428",         TransferCharacteristics::SMPTE428},
        {"ARIB_STD_B67_HLG", TransferCharacteristics::ARIB_STD_B67_HLG}
    };
    auto it = m.find(s);
    return (it != m.end()) ? it->second : TransferCharacteristics::UNKNOWN;
}

ColorPrimaries videoDecoderStringToColorPrimaries(const std::string& s) {
    static const std::unordered_map<std::string, ColorPrimaries> m = {
        {"UNKNOWN",    ColorPrimaries::UNKNOWN},
        {"BT709",      ColorPrimaries::BT709},
        {"UNSPECIFIED",ColorPrimaries::UNSPECIFIED},
        {"BT470M",     ColorPrimaries::BT470M},
        {"BT470BG",    ColorPrimaries::BT470BG},
        {"BT601_525",  ColorPrimaries::BT601_525},
        {"SMPTE240M",  ColorPrimaries::SMPTE240M},
        {"FILM",       ColorPrimaries::FILM},
        {"BT2020",     ColorPrimaries::BT2020},
        {"SMPTE428",   ColorPrimaries::SMPTE428},
        {"DCI_P3_D60", ColorPrimaries::DCI_P3_D60},
        {"DISPLAY_P3", ColorPrimaries::DISPLAY_P3},
        {"EBU3213",    ColorPrimaries::EBU3213}
    };
    auto it = m.find(s);
    return (it != m.end()) ? it->second : ColorPrimaries::UNKNOWN;
}

Colorimetry videoDecoderStringToColorimetry(const std::string& rangeStr, const std::string& matrixStr,
                                             const std::string& transferStr, const std::string& primariesStr) {
    Colorimetry c;
    c.range    = videoDecoderStringToColorRange(rangeStr);
    c.matrix   = videoDecoderStringToColorMatrix(matrixStr);
    c.transfer = videoDecoderStringToTransferCharacteristics(transferStr);
    c.primaries= videoDecoderStringToColorPrimaries(primariesStr);
    return c;
}

// Some HFP yaml variants declare supportedColorimetries as scalar presets
// (e.g. BT601_525, BT601_625, BT709, BT2020) instead of the 4-field
// range/matrix/transfer/primaries map; map those presets to their canonical Colorimetry.
Colorimetry videoDecoderColorimetryPresetToColorimetry(const std::string& presetStr) {
    auto makeColorimetry = [](ColorRange range, ColorMatrix matrix, TransferCharacteristics transfer, ColorPrimaries primaries) {
        Colorimetry c;
        c.range = range;
        c.matrix = matrix;
        c.transfer = transfer;
        c.primaries = primaries;
        return c;
    };
    static const std::unordered_map<std::string, Colorimetry> presetMap = {
        {"BT601_525", makeColorimetry(ColorRange::LIMITED, ColorMatrix::BT601_525,  TransferCharacteristics::SMPTE170M, ColorPrimaries::BT601_525)},
        {"BT601_625", makeColorimetry(ColorRange::LIMITED, ColorMatrix::BT601_625,  TransferCharacteristics::BT470BG_GAMMA28, ColorPrimaries::BT470BG)},
        {"BT709",     makeColorimetry(ColorRange::LIMITED, ColorMatrix::BT709,      TransferCharacteristics::BT709,     ColorPrimaries::BT709)},
        {"BT2020",    makeColorimetry(ColorRange::LIMITED, ColorMatrix::BT2020_NCL, TransferCharacteristics::BT2020_10, ColorPrimaries::BT2020)}
    };
    auto it = presetMap.find(presetStr);
    if (it != presetMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown colorimetry preset: " + presetStr);
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

videodecoder::Profile videoDecoderStringToProfile(const std::string& profileStr, const std::string& levelStr, int64_t maxBitrateInBps) {
    videodecoder::Profile prof;
    prof.profile = videoDecoderStringToCodecProfile(profileStr);
    prof.maxLevel = videoDecoderStringToCodecLevel(levelStr);
    prof.maxBitrateInBps = maxBitrateInBps;
    return prof;
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

bool vts_videodecoder_helper_isKnownPixelFormatString(const std::string& formatStr) {
    static const std::unordered_set<std::string> knownPixelFormats = {
        "YCBCR422",
        "YCBCR444",
        "YCBCR420",
        "RGB",
        "NATIVE"
    };

    return knownPixelFormats.find(formatStr) != knownPixelFormats.end();
}

bool vts_videodecoder_helper_profileFieldPresent(const std::string& key) {
    // Equivalent to ut_kvp_fieldPresent but for profile instance
    // Try to get string field and check status - this is more reliable than checking return values
    return ut_kvp_fieldPresent(ut_kvp_profile_getInstance(), key.c_str());
}

bool vts_videodecoder_helper_checkCodecAttributeExists(const std::string& attrpath, const std::string& codec_name) {
    // Check all required fields exist
    const std::vector<std::string> requiredFields = {
        ".maxFrameRate",
        ".maxFrameWidth",
        ".maxFrameHeight",
        ".profiles"
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

    // Validate values are reasonable
    // Note: Not requiring frameRate > 0 since 0 could be valid for special cases
    bool isValid = (maxFrameWidth > 0 && maxFrameHeight > 0);

    if (isValid) {
        UT_LOG_DEBUG("Codec %s has valid attributes: rate=%u, width=%u, height=%u",
                     codec_name.c_str(), maxFrameRate, maxFrameWidth, maxFrameHeight);
    } else {
        UT_LOG_DEBUG("Codec %s has invalid attribute values: rate=%u, width=%u, height=%u",
                     codec_name.c_str(), maxFrameRate, maxFrameWidth, maxFrameHeight);
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

void vts_videodecoder_get_available_id(std::vector<int32_t> &availableIds)
{
    string videodecoderpath = VIDEODECODERMGR_IVIDEODECODER;
    int32_t number_of_videodecoders = UT_KVP_PROFILE_GET_LIST_COUNT(videodecoderpath.c_str());
    availableIds.clear();
    for (int32_t i = 0; i < number_of_videodecoders; ++i)
    {
        availableIds.push_back(i);
    }
}

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

    auto savedOpmodes = std::move(temporary.supportedOperationalModes);

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
        std::string supportedcodecpath = decoderbasepath + ".supportedCodecs";
        std::string supporteddrpath = decoderbasepath + ".supportedDynamicRanges";
        std::string supportedcolorpath = decoderbasepath + ".supportedColorimetries";
        std::string supportsSecurepath = decoderbasepath + ".supportsSecure";

        temporary.id = i; // The ID is the index in the list.
        temporary.supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
        temporary.supportedCodecs.clear();
        temporary.supportedDynamicRanges.clear();
        temporary.supportedColorimetries.clear();
        temporary.supportedOperationalModes = savedOpmodes;

        UT_LOG_INFO("Parsing Decoder ID: %d, supportsSecure: %d", temporary.id, temporary.supportsSecure);

        int supportedcodec_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodecpath.c_str());
        UT_LOG_INFO("Decoder %d: codec count=%d", i, supportedcodec_count);
        for (int32_t j = 0; j < supportedcodec_count; ++j)
        {
            std::string codecpath = supportedcodecpath + "." + std::to_string(j);

            UT_LOG_DEBUG("Checking codec slot %d at path: %s", j, codecpath.c_str());

            // First, efficiently detect which codec exists at this slot by checking for any codec-specific path
            std::string detected_codec;
            for (const auto& codec_name : known_codecs)
            {
                std::string test_path = codecpath + "." + codec_name + ".profiles";
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

            // Profiles (array of {profile, maxLevel} entries)
            int profileCount = UT_KVP_PROFILE_GET_LIST_COUNT((attrpath + ".profiles").c_str());
            for (int32_t p = 0; p < profileCount; ++p)
            {
                std::string profileEntryPath = attrpath + ".profiles." + std::to_string(p);
                char profile_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
                char level_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};

                UT_KVP_PROFILE_GET_STRING((profileEntryPath + ".profile").c_str(), profile_kvp);
                UT_KVP_PROFILE_GET_STRING((profileEntryPath + ".maxLevel").c_str(), level_kvp);
                int64_t maxBitrateInBps = static_cast<int64_t>(UT_KVP_PROFILE_GET_UINT64((profileEntryPath + ".maxBitrateInBps").c_str()));

                std::string profileStr = std::string(profile_kvp);
                std::string levelStr   = std::string(level_kvp);

                if (!profileStr.empty()) {
                    cap.profiles.push_back(videoDecoderStringToProfile(profileStr, levelStr, maxBitrateInBps));
                } else {
                    UT_LOG_WARNING("Empty profile entry %d for codec %s", p, detected_codec.c_str());
                }
            }
            if (cap.profiles.empty()) {
                UT_LOG_WARNING("No valid profiles for codec %s", detected_codec.c_str());
            }

            cap.maxFrameRate = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameRate").c_str());
            cap.maxFrameWidth = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameWidth").c_str());
            cap.maxFrameHeight = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameHeight").c_str());

            int supportedPixFmtCount = UT_KVP_PROFILE_GET_LIST_COUNT((attrpath + ".supportedOutputPixelFormats").c_str());
            for (int32_t k = 0; k < supportedPixFmtCount; ++k)
            {
                char pixFmtKvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
                std::string pixFmtPath = attrpath + ".supportedOutputPixelFormats." + std::to_string(k);
                UT_KVP_PROFILE_GET_STRING(pixFmtPath.c_str(), pixFmtKvp);
                std::string pixFmtStr = std::string(pixFmtKvp);
                if (!pixFmtStr.empty()) {
                    if (!vts_videodecoder_helper_isKnownPixelFormatString(pixFmtStr)) {
                        UT_LOG_WARNING("Invalid supportedOutputPixelFormats entry at %s (%s): skipping",
                                       pixFmtPath.c_str(), pixFmtStr.c_str());
                        continue;
                    }

                    cap.supportedOutputPixelFormats.push_back(videoDecoderStringToPixelFormat(pixFmtStr));
                }
            }

            UT_LOG_INFO("Codec %s: rate=%d, width=%d, height=%d, profiles=%zu",
                        detected_codec.c_str(), cap.maxFrameRate, cap.maxFrameWidth, cap.maxFrameHeight,
                        cap.profiles.size());

            temporary.supportedCodecs.push_back(std::move(cap));
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

        int supportedcolor_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcolorpath.c_str());
        UT_LOG_INFO("Decoder %d: colorimetry count=%d", i, supportedcolor_count);
        for (int32_t j = 0; j < supportedcolor_count; ++j)
        {
            std::string colorpath = supportedcolorpath + "." + std::to_string(j);
            if (vts_videodecoder_helper_profileFieldPresent(colorpath + ".range"))
            {
                // Structured range/matrix/transfer/primaries map (e.g. amlogic/mock/vdevice HFP variants).
                char range_kvp[UT_KVP_MAX_ELEMENT_SIZE]    = {0};
                char matrix_kvp[UT_KVP_MAX_ELEMENT_SIZE]   = {0};
                char transfer_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
                char primaries_kvp[UT_KVP_MAX_ELEMENT_SIZE]= {0};
                UT_KVP_PROFILE_GET_STRING((colorpath + ".range").c_str(),    range_kvp);
                UT_KVP_PROFILE_GET_STRING((colorpath + ".matrix").c_str(),   matrix_kvp);
                UT_KVP_PROFILE_GET_STRING((colorpath + ".transfer").c_str(), transfer_kvp);
                UT_KVP_PROFILE_GET_STRING((colorpath + ".primaries").c_str(),primaries_kvp);
                UT_LOG_INFO("Reading colorimetry path: %s, range=%s, matrix=%s, transfer=%s, primaries=%s",
                            colorpath.c_str(), range_kvp, matrix_kvp, transfer_kvp, primaries_kvp);
                try {
                    temporary.supportedColorimetries.push_back(videoDecoderStringToColorimetry(
                        std::string(range_kvp), std::string(matrix_kvp),
                        std::string(transfer_kvp), std::string(primaries_kvp)));
                } catch (const std::invalid_argument& e) {
                    UT_LOG_WARNING("Invalid colorimetry at path: %s, error: %s", colorpath.c_str(), e.what());
                }
            }
            else
            {
                // Scalar colorimetry preset (e.g. hfp-videodecoder.yaml, ID0/ID1/ID2 variants).
                char preset_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
                UT_KVP_PROFILE_GET_STRING(colorpath.c_str(), preset_kvp);
                std::string presetStr = std::string(preset_kvp);
                UT_LOG_INFO("Reading colorimetry path: %s, preset=%s", colorpath.c_str(), presetStr.c_str());
                if (!presetStr.empty()) {
                    try {
                        temporary.supportedColorimetries.push_back(videoDecoderColorimetryPresetToColorimetry(presetStr));
                    } catch (const std::invalid_argument& e) {
                        UT_LOG_WARNING("Invalid colorimetry preset at path: %s, value: %s, error: %s",
                                      colorpath.c_str(), presetStr.c_str(), e.what());
                    }
                } else {
                    UT_LOG_WARNING("Empty colorimetry preset at path: %s", colorpath.c_str());
                }
            }
        }

        UT_LOG_INFO("Decoder %d: id=%d, %zu codecs, %zu dynamic ranges, %zu colorimetries, %zu opmodes",
                    i, temporary.id, temporary.supportedCodecs.size(),
                    temporary.supportedDynamicRanges.size(),
                    temporary.supportedColorimetries.size(),
                    temporary.supportedOperationalModes.size());

        if (temporary.supportedCodecs.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid codecs, skipping", i);
            continue;
        }
        if (temporary.supportedDynamicRanges.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid dynamic ranges, skipping", i);
            continue;
        }

        if (temporary.supportedColorimetries.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid colorimetries, skipping", i);
            continue;
        }

        gVideodecoders_hfp.push_back(std::move(temporary));
    }

    if (gVideodecoders_hfp.empty()) {
        UT_LOG_ERROR("No valid decoders parsed from HFP");
        return UT_STATUS_FAILURE;
    }

    UT_LOG_INFO("Successfully parsed %zu decoders", gVideodecoders_hfp.size());
    for (const auto& dec : gVideodecoders_hfp) {
        UT_LOG_INFO("HFP Decoder ID: %d, codecs: %zu, dynamic ranges: %zu, colorimetries: %zu, opmodes: %zu",
                    dec.id, dec.supportedCodecs.size(), dec.supportedDynamicRanges.size(),
                    dec.supportedColorimetries.size(),
                    dec.supportedOperationalModes.size());
    }
    return UT_STATUS_OK;
}

bool checkDecoderSupportsConfig(const IVideoDecoder::Id& decoderId, const videodecoder::Capabilities* capabilities,
                                const videoDecoderTestConfiguration_t& testConfig, bool secureDecode)
{
    auto hfpIt = std::find_if(gVideodecoders_hfp.begin(), gVideodecoders_hfp.end(),
        [&decoderId](const decoder_t& item) { return item.id == decoderId.value; });
    if (hfpIt == gVideodecoders_hfp.end())
    {
        UT_LOG_WARNING("[DecoderID: %d] Not available in HFP", decoderId.value);
        return false;
    }
    const decoder_t& hfpDecoder = *hfpIt;

    auto supportsConfigFromHfp = [&](const char* fallbackReason) -> bool
    {
        if (secureDecode && !hfpDecoder.supportsSecure)
        {
            UT_LOG_WARNING("Decoder [%d] doesn't support secure decoding (HFP fallback)", decoderId.value);
            return false;
        }

        auto hfpCodecIt = std::find_if(hfpDecoder.supportedCodecs.begin(), hfpDecoder.supportedCodecs.end(),
            [&testConfig](const CodecCapability& cap) { return cap.codec == testConfig.codec; });
        if (hfpCodecIt == hfpDecoder.supportedCodecs.end())
        {
            UT_LOG_WARNING("Decoder [%d] doesn't support Codec [%s] in HFP",
                           decoderId.value,
                           videodecoder::toString(testConfig.codec).c_str());
            return false;
        }

        if (hfpCodecIt->supportedOutputPixelFormats.empty())
        {
            UT_LOG_WARNING("Decoder [%d] HFP missing supportedOutputPixelFormats for codec [%s]",
                           decoderId.value,
                           videodecoder::toString(testConfig.codec).c_str());
            return false;
        }

        if (std::find(hfpCodecIt->supportedOutputPixelFormats.begin(),
                      hfpCodecIt->supportedOutputPixelFormats.end(),
                      testConfig.pixelFormat) == hfpCodecIt->supportedOutputPixelFormats.end())
        {
            UT_LOG_INFO("Decoder [%d] skipping stream [%s]: pixel format [%s] unsupported in HFP",
                        decoderId.value,
                        testConfig.streamName.c_str(),
                        videodecoder::toString(testConfig.pixelFormat).c_str());
            return false;
        }

        if (testConfig.codedWidth > hfpCodecIt->maxFrameWidth ||
            testConfig.codedHeight > hfpCodecIt->maxFrameHeight)
        {
            UT_LOG_INFO("Decoder [%d] skipping stream [%s]: resolution %dx%d exceeds HFP capability %dx%d",
                        decoderId.value,
                        testConfig.streamName.c_str(),
                        testConfig.codedWidth,
                        testConfig.codedHeight,
                        hfpCodecIt->maxFrameWidth,
                        hfpCodecIt->maxFrameHeight);
            return false;
        }

        if (testConfig.frameRateDenominator > 0)
        {
            // Cross-multiply instead of comparing truncated integer division results,
            // so e.g. 61/2 (30.5fps) isn't rounded down to 30 and wrongly accepted.
            if (testConfig.frameRateNumerator >
                static_cast<uint32_t>(hfpCodecIt->maxFrameRate) * testConfig.frameRateDenominator)
            {
                UT_LOG_INFO("Decoder [%d] skipping stream [%s]: frame rate %u/%u fps exceeds HFP capability %d fps",
                            decoderId.value,
                            testConfig.streamName.c_str(),
                            testConfig.frameRateNumerator,
                            testConfig.frameRateDenominator,
                            hfpCodecIt->maxFrameRate);
                return false;
            }
        }

        UT_LOG_WARNING("Decoder [%d] using HFP fallback for stream [%s] because %s",
                       decoderId.value,
                       testConfig.streamName.c_str(),
                       fallbackReason);
        return true;
    };

    if (capabilities == nullptr)
    {
        return supportsConfigFromHfp("decoder getCapabilities is unavailable");
    }

    if (secureDecode && !capabilities->supportsSecure)
    {
        UT_LOG_WARNING("Decoder doesn't support secure decoding");
        return false;
    }

    if (capabilities->supportedCodecs.empty())
    {
        return supportsConfigFromHfp("decoder getCapabilities missing supportedCodecs");
    }

    auto codecIt = std::find_if(capabilities->supportedCodecs.begin(), capabilities->supportedCodecs.end(),
                                [&testConfig](const videodecoder::CodecCapabilities& cap) { return cap.codec == testConfig.codec; });
    if (codecIt == capabilities->supportedCodecs.end())
    {
        UT_LOG_WARNING("Decoder doesn't support Codec [%s]", videodecoder::toString(testConfig.codec).c_str());
        return false;
    }

    if (codecIt->supportedOutputPixelFormats.empty())
    {
        UT_LOG_INFO(
            "Decoder [%d] skipping stream [%s]: decoder getCapabilities missing supportedOutputPixelFormats",
             decoderId.value,
             testConfig.streamName.c_str());
        return supportsConfigFromHfp("decoder getCapabilities missing supportedOutputPixelFormats");
    }

    if (std::find(codecIt->supportedOutputPixelFormats.begin(),
                  codecIt->supportedOutputPixelFormats.end(),
                  testConfig.pixelFormat) == codecIt->supportedOutputPixelFormats.end())
    {
        UT_LOG_INFO(
            "Decoder [%d] skipping stream [%s]: pixel format [%s] unsupported",
             decoderId.value,
             testConfig.streamName.c_str(),
             videodecoder::toString(testConfig.pixelFormat).c_str());
        return false;
    }

    if (testConfig.codedWidth > codecIt->maxFrameWidth ||
        testConfig.codedHeight > codecIt->maxFrameHeight)
    {
        UT_LOG_INFO(
            "Decoder [%d] skipping stream [%s]: resolution %dx%d exceeds capability %dx%d",
             decoderId.value,
             testConfig.streamName.c_str(),
             testConfig.codedWidth,
             testConfig.codedHeight,
             codecIt->maxFrameWidth,
             codecIt->maxFrameHeight);
        return false;
    }

    if (testConfig.frameRateDenominator > 0)
    {
        // Cross-multiply instead of comparing truncated integer division results,
        // so e.g. 61/2 (30.5fps) isn't rounded down to 30 and wrongly accepted.
        if (testConfig.frameRateNumerator >
            static_cast<uint32_t>(codecIt->maxFrameRate) * testConfig.frameRateDenominator)
        {
            UT_LOG_INFO(
                "Decoder [%d] skipping stream [%s]: frame rate %u/%u fps exceeds capability %d fps",
                 decoderId.value,
                 testConfig.streamName.c_str(),
                 testConfig.frameRateNumerator,
                 testConfig.frameRateDenominator,
                 codecIt->maxFrameRate);
            return false;
        }
    }
    return true;
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
    if (capabilities.supportedCodecs.empty())
    {
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
        UT_LOG_INFO("Codec %d: rate=%d, width=%d, height=%d",
                    static_cast<int32_t>(codecCap.codec),
                    codecCap.maxFrameRate,
                    codecCap.maxFrameWidth,
                    codecCap.maxFrameHeight);
        for (const auto& p : codecCap.profiles) {
            UT_LOG_INFO("  Profile: %d, MaxLevel: %d, MaxBitrate: %lld",
                        static_cast<int32_t>(p.profile),
                        static_cast<int32_t>(p.maxLevel),
                        static_cast<long long>(p.maxBitrateInBps));
        }
        UT_LOG_INFO("Codec %d: supportedOutputPixelFormats count=%zu",
                static_cast<int32_t>(codecCap.codec), codecCap.supportedOutputPixelFormats.size());
    }

    // Validate supportedDynamicRanges
    if (capabilities.supportedDynamicRanges.empty())
    {
        UT_LOG_ERROR("No dynamic ranges in capabilities for decoder ID: %d", decoderId);
        return; // Avoid crash
    }
    UT_ASSERT_TRUE(!capabilities.supportedDynamicRanges.empty());
    UT_LOG_INFO("Capabilities has %zu dynamic ranges", capabilities.supportedDynamicRanges.size());

    // Optional: Validate against gVideodecoders_hfp (if populated)
    if (gVideodecoders_hfp.empty())
    {
        UT_LOG_ERROR("gVideodecoders_hfp is empty for decoder ID: %d", decoderId);
        return; // Avoid crash
    }
    for (const auto& hfpDecoder : gVideodecoders_hfp)
    {
        if (hfpDecoder.id == decoderId)
        {
            UT_LOG_INFO("Found HFP entry for decoder ID: %d", decoderId);
            if (capabilities.supportedCodecs.size() != hfpDecoder.supportedCodecs.size())
            {
                UT_LOG_ERROR("Codec size mismatch: capabilities=%zu, hfpDecoder=%zu",
                             capabilities.supportedCodecs.size(), hfpDecoder.supportedCodecs.size());
                UT_ASSERT_EQUAL(capabilities.supportedCodecs.size(), hfpDecoder.supportedCodecs.size());
                //return; // Avoid out-of-bounds
            }
            UT_ASSERT_EQUAL(capabilities.supportedCodecs.size(), hfpDecoder.supportedCodecs.size());

            if (capabilities.supportedDynamicRanges.size() != hfpDecoder.supportedDynamicRanges.size())
            {
                UT_LOG_ERROR("Dynamic range size mismatch: capabilities=%zu, hfpDecoder=%zu",
                             capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());
                UT_ASSERT_EQUAL(capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());
                //return; // Avoid out-of-bounds
            }
            UT_ASSERT_EQUAL(capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());

            UT_ASSERT_EQUAL(capabilities.supportsSecure, hfpDecoder.supportsSecure);

            // Validate codecs
            for (const auto& cap : capabilities.supportedCodecs)
            {
                // Search for matching codec in hfpDecoder.supportedCodecs
                auto it = std::find_if(
                    hfpDecoder.supportedCodecs.begin(),
                    hfpDecoder.supportedCodecs.end(),
                    [&cap](const CodecCapability& hfpCap) {
                        return static_cast<int32_t>(cap.codec) == static_cast<int32_t>(hfpCap.codec);
                    });
                if (it == hfpDecoder.supportedCodecs.end())
                {
                    UT_LOG_ERROR("Codec %d not found in hfpDecoder for decoder ID: %d", static_cast<int32_t>(cap.codec), decoderId);
                    UT_ASSERT_TRUE(false) << "Codec not found in hfpDecoder";
                    continue;
                }
                const auto& hfpCap = *it;
                UT_LOG_INFO("Validating codec: %d", static_cast<int32_t>(cap.codec));
                UT_ASSERT_EQUAL(static_cast<int32_t>(cap.codec), static_cast<int32_t>(hfpCap.codec));
                UT_ASSERT_EQUAL(cap.profiles, hfpCap.profiles);
                UT_ASSERT_EQUAL(cap.maxFrameRate, hfpCap.maxFrameRate);
                UT_ASSERT_EQUAL(cap.maxFrameWidth, hfpCap.maxFrameWidth);
                UT_ASSERT_EQUAL(cap.maxFrameHeight, hfpCap.maxFrameHeight);

                UT_ASSERT_EQUAL(cap.supportedOutputPixelFormats.size(), hfpCap.supportedOutputPixelFormats.size());
                for (const auto& pixelFormat : cap.supportedOutputPixelFormats)
                {
                    auto pixelIt = std::find_if(
                        hfpCap.supportedOutputPixelFormats.begin(),
                        hfpCap.supportedOutputPixelFormats.end(),
                        [&pixelFormat](const auto& hfpPixelFormat) {
                            return static_cast<int32_t>(pixelFormat) == static_cast<int32_t>(hfpPixelFormat);
                        });
                    if (pixelIt == hfpCap.supportedOutputPixelFormats.end())
                    {
                        UT_LOG_ERROR("Pixel format %d not found in HFP for codec %d on decoder ID %d",
                                     static_cast<int32_t>(pixelFormat), static_cast<int32_t>(cap.codec), decoderId);
                        UT_ASSERT_TRUE(false) << "supportedOutputPixelFormats mismatch with HFP";
                    }
                }
                UT_LOG_INFO("Validated codec: %d", static_cast<int32_t>(cap.codec));
            }

            // Validate dynamic ranges (order-independent)
            UT_ASSERT_EQUAL(capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());
            for (const auto& dynRange : capabilities.supportedDynamicRanges)
            {
                auto it = std::find_if(
                    hfpDecoder.supportedDynamicRanges.begin(),
                    hfpDecoder.supportedDynamicRanges.end(),
                    [&dynRange](const auto& hfpDynRange) {
                        return static_cast<int32_t>(dynRange) == static_cast<int32_t>(hfpDynRange);
                    });
                if (it == hfpDecoder.supportedDynamicRanges.end())
                {
                    UT_LOG_ERROR("Dynamic range %d not found in hfpDecoder for decoder ID: %d", static_cast<int32_t>(dynRange), decoderId);
                    UT_ASSERT_TRUE(false) << "Dynamic range not found in hfpDecoder";
                    continue;
                }
                UT_LOG_INFO("Validated dynamic range: %d", static_cast<int32_t>(dynRange));
                UT_ASSERT_EQUAL(static_cast<int32_t>(dynRange), static_cast<int32_t>(*it));
            }

            // Validate colorimetries (order-independent)
            UT_ASSERT_EQUAL(capabilities.supportedColorimetries.size(), hfpDecoder.supportedColorimetries.size());
            for (const auto& colorimetry : capabilities.supportedColorimetries)
            {
                auto it = std::find_if(
                    hfpDecoder.supportedColorimetries.begin(),
                    hfpDecoder.supportedColorimetries.end(),
                    [&colorimetry](const auto& hfpColorimetry) {
                        return colorimetry == hfpColorimetry;
                    });
                if (it == hfpDecoder.supportedColorimetries.end())
                {
                    UT_LOG_ERROR("Colorimetry not found in hfpDecoder for decoder ID %d: range=%d, matrix=%d, transfer=%d, primaries=%d",
                                 decoderId,
                                 static_cast<int32_t>(colorimetry.range),
                                 static_cast<int32_t>(colorimetry.matrix),
                                 static_cast<int32_t>(colorimetry.transfer),
                                 static_cast<int32_t>(colorimetry.primaries));
                    for (const auto& hfpColorimetry : hfpDecoder.supportedColorimetries)
                    {
                        UT_LOG_ERROR("HFP colorimetry candidate: range=%d, matrix=%d, transfer=%d, primaries=%d",
                                     static_cast<int32_t>(hfpColorimetry.range),
                                     static_cast<int32_t>(hfpColorimetry.matrix),
                                     static_cast<int32_t>(hfpColorimetry.transfer),
                                     static_cast<int32_t>(hfpColorimetry.primaries));
                    }
                    UT_ASSERT_TRUE(false) << "Colorimetry not found in hfpDecoder";
                    continue;
                }
                UT_LOG_INFO("Validated colorimetry: range=%d, matrix=%d, transfer=%d, primaries=%d",
                            static_cast<int32_t>(colorimetry.range),
                            static_cast<int32_t>(colorimetry.matrix),
                            static_cast<int32_t>(colorimetry.transfer),
                            static_cast<int32_t>(colorimetry.primaries));
                UT_ASSERT_EQUAL(colorimetry, *it);
            }
            break;
        }
    }
    UT_LOG_INFO("Completed validation for decoder ID: %d", decoderId);
}

UT_status_t vts_videodecoder_parse_testconfig(void* pTestInstance, const char *module,
                                              const char *testGroup, const char *testName,
                                              vector<videoDecoderTestConfiguration_t>& testConfiguration)
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
    string streamInputPathKey = string(module) + "." + "input_streams_path";
    string streamRefPathKey = string(module) + "." + "reference_streams_path";

    UT_LOG_INFO("Parsing test config for %s", testList.c_str());

    // Get input and reference paths
    char tempBuffer[UT_KVP_MAX_ELEMENT_SIZE] = {0};
    status = ut_kvp_getStringField(kvpTestInstance, streamInputPathKey.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
    if (status != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to get input_streams_path, status: %d", status);
        return UT_STATUS_FAILURE;
    }
    streamInputPath = tempBuffer;
    UT_LOG_INFO("streamInputPath: %s", streamInputPath.c_str());

    status = ut_kvp_getStringField(kvpTestInstance, streamRefPathKey.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
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

        testDetails.colorimetry = Colorimetry{};
        testDetails.masteringDisplayInfo.reset();
        testDetails.contentLightLevel.reset();

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
        string colorimetry = prefix + "colorimetry";
        string masteringDisplayInfo = prefix + "masteringDisplayInfo";
        string contentLightLevel = prefix + "contentLightLevel";
        string pixelAspectRatio = prefix + "pixel_aspect_ratio";
        string scanType = prefix + "scanType";
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
        string openWithResolutionWidth  = prefix + "openWithResolutionWidth";
        string openWithResolutionHeight = prefix + "openWithResolutionHeight";
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

        // openWithResolution hint dimensions (optional, default -1 = use open())
        // Uses the signed getter since -1 is a valid sentinel here: ut_kvp_getUInt32Field()
        // parses "-1" via strtoul() and range-checks against UINT32_MAX, which clamps to 0
        // on platforms where `unsigned long` is 64-bit (e.g. x86_64 dev hosts).
        testDetails.openWithResolutionWidth  = ut_kvp_fieldPresent(kvpTestInstance, openWithResolutionWidth.c_str())
            ? ut_kvp_getInt32Field(kvpTestInstance, openWithResolutionWidth.c_str()) : -1;
        testDetails.openWithResolutionHeight = ut_kvp_fieldPresent(kvpTestInstance, openWithResolutionHeight.c_str())
            ? ut_kvp_getInt32Field(kvpTestInstance, openWithResolutionHeight.c_str()) : -1;

        // Frame rate
        testDetails.frameRateNumerator = ut_kvp_getUInt32Field(kvpTestInstance, frameRateNumerator.c_str());
        testDetails.frameRateDenominator = ut_kvp_getUInt32Field(kvpTestInstance, frameRateDenominator.c_str());

        // Pixel format
        if (ut_kvp_fieldPresent(kvpTestInstance, pixelFormat.c_str()))
        {
            ut_kvp_getStringField(kvpTestInstance, pixelFormat.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
            testDetails.pixelFormat = videoDecoderStringToPixelFormat(string(tempBuffer));
        }

        // Dynamic range
        if (ut_kvp_fieldPresent(kvpTestInstance, dynamicRange.c_str()))
        {
            ut_kvp_getStringField(kvpTestInstance, dynamicRange.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
            testDetails.dynamicRange = videoDecoderStringToDynamicRange(string(tempBuffer));
            if(testDetails.dynamicRange == videodecoder::DynamicRange::DOLBY_VISION)
            {
                string blPresent = prefix + "blPresent";
                string elPresent = prefix + "elPresent";

                testDetails.blPresent = ut_kvp_getUInt32Field(kvpTestInstance, blPresent.c_str());
                testDetails.elPresent = ut_kvp_getUInt32Field(kvpTestInstance, elPresent.c_str());
            } else {
                testDetails.blPresent = 0;
                testDetails.elPresent = 0;
            }
        }

        // Colorimetry (now a 4-axis parcelable: range, matrix, transfer, primaries)
        if (ut_kvp_fieldPresent(kvpTestInstance, (colorimetry + ".range").c_str()))
        {
            char rangeKvp[UT_KVP_MAX_ELEMENT_SIZE]    = {0};
            char matrixKvp[UT_KVP_MAX_ELEMENT_SIZE]   = {0};
            char transferKvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
            char primariesKvp[UT_KVP_MAX_ELEMENT_SIZE]= {0};
            ut_kvp_getStringField(kvpTestInstance, (colorimetry + ".range").c_str(),    rangeKvp,    UT_KVP_MAX_ELEMENT_SIZE);
            ut_kvp_getStringField(kvpTestInstance, (colorimetry + ".matrix").c_str(),   matrixKvp,   UT_KVP_MAX_ELEMENT_SIZE);
            ut_kvp_getStringField(kvpTestInstance, (colorimetry + ".transfer").c_str(), transferKvp, UT_KVP_MAX_ELEMENT_SIZE);
            ut_kvp_getStringField(kvpTestInstance, (colorimetry + ".primaries").c_str(),primariesKvp,UT_KVP_MAX_ELEMENT_SIZE);
            testDetails.colorimetry = videoDecoderStringToColorimetry(
                string(rangeKvp), string(matrixKvp), string(transferKvp), string(primariesKvp));
        }

        // Bitstream-provided pixel aspect ratio (only set when the stream's SPS actually
        // signals PAR - absent means the bitstream is silent on this field).
        if (ut_kvp_fieldPresent(kvpTestInstance, (pixelAspectRatio + ".numerator").c_str()))
        {
            videodecoder::Fraction par{};
            par.numerator   = static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, (pixelAspectRatio + ".numerator").c_str()));
            par.denominator = static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, (pixelAspectRatio + ".denominator").c_str()));
            testDetails.pixelAspectRatio = par;
        }

        // Mastering display info
        if (ut_kvp_fieldPresent(kvpTestInstance, (masteringDisplayInfo + ".whitePointX").c_str()))
        {
            MasteringDisplayInfo mdi{};

            mdi.redPrimaryX = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".redPrimaryX").c_str());
            mdi.redPrimaryY = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".redPrimaryY").c_str());

            mdi.greenPrimaryX = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".greenPrimaryX").c_str());
            mdi.greenPrimaryY = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".greenPrimaryY").c_str());

            mdi.bluePrimaryX = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".bluePrimaryX").c_str());
            mdi.bluePrimaryY = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".bluePrimaryY").c_str());

            mdi.whitePointX = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".whitePointX").c_str());
            mdi.whitePointY = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".whitePointY").c_str());

            mdi.maxLuminance = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".maxLuminance").c_str());
            mdi.minLuminance = ut_kvp_getUInt32Field(
                kvpTestInstance, (masteringDisplayInfo + ".minLuminance").c_str());

            testDetails.masteringDisplayInfo = mdi;
        }

        // Content light level
        if (ut_kvp_fieldPresent(kvpTestInstance, (contentLightLevel + ".maxCLL").c_str()))
        {
            ContentLightLevel cll{};

            cll.maxCLL = ut_kvp_getUInt32Field(
                kvpTestInstance, (contentLightLevel + ".maxCLL").c_str());
            cll.maxFALL = ut_kvp_getUInt32Field(
                kvpTestInstance, (contentLightLevel + ".maxFALL").c_str());

            testDetails.contentLightLevel = cll;
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

        // Sink control fields
        string key = streamList + "." + to_string(i) + ".sink_backpressure";
        testDetails.sink_backpressure = ut_kvp_getBoolField(kvpTestInstance, key.c_str());
        key = streamList + "." + to_string(i) + ".sink_discardFramesUntilPts";
        testDetails.sink_discardFramesUntilPts = static_cast<int64_t>(ut_kvp_getUInt64Field(kvpTestInstance, key.c_str()));
        key = streamList + "." + to_string(i) + ".frames_to_discard";
        testDetails.frames_to_discard = ut_kvp_fieldPresent(kvpTestInstance, key.c_str())
            ? static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, key.c_str())) : 0;
        key = streamList + "." + to_string(i) + ".sink_underflow_pts";
        testDetails.sink_underflow_pts = static_cast<int64_t>(ut_kvp_getUInt64Field(kvpTestInstance, key.c_str()));

        // Mid-stream metadata override fields
        string metadataOverridePts = prefix + "metadata_override_pts";
        testDetails.metadataOverridePts = ut_kvp_fieldPresent(kvpTestInstance, metadataOverridePts.c_str())
            ? static_cast<int64_t>(ut_kvp_getUInt64Field(kvpTestInstance, metadataOverridePts.c_str())) : -1;

        string overrideColorimetry = prefix + "override_colorimetry";
        if (ut_kvp_fieldPresent(kvpTestInstance, (overrideColorimetry + ".range").c_str()) ||
            ut_kvp_fieldPresent(kvpTestInstance, (overrideColorimetry + ".matrix").c_str()) ||
            ut_kvp_fieldPresent(kvpTestInstance, (overrideColorimetry + ".transfer").c_str()) ||
            ut_kvp_fieldPresent(kvpTestInstance, (overrideColorimetry + ".primaries").c_str()))
        {
            char rangeKvp[UT_KVP_MAX_ELEMENT_SIZE]    = {0};
            char matrixKvp[UT_KVP_MAX_ELEMENT_SIZE]   = {0};
            char transferKvp[UT_KVP_MAX_ELEMENT_SIZE]  = {0};
            char primariesKvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
            ut_kvp_getStringField(kvpTestInstance, (overrideColorimetry + ".range").c_str(),    rangeKvp,    UT_KVP_MAX_ELEMENT_SIZE);
            ut_kvp_getStringField(kvpTestInstance, (overrideColorimetry + ".matrix").c_str(),   matrixKvp,   UT_KVP_MAX_ELEMENT_SIZE);
            ut_kvp_getStringField(kvpTestInstance, (overrideColorimetry + ".transfer").c_str(), transferKvp, UT_KVP_MAX_ELEMENT_SIZE);
            ut_kvp_getStringField(kvpTestInstance, (overrideColorimetry + ".primaries").c_str(),primariesKvp,UT_KVP_MAX_ELEMENT_SIZE);
            testDetails.overrideColorimetry = videoDecoderStringToColorimetry(
                string(rangeKvp), string(matrixKvp), string(transferKvp), string(primariesKvp));
        }

        string overrideMdi = prefix + "override_mastering_display_info";
        if (ut_kvp_fieldPresent(kvpTestInstance, (overrideMdi + ".whitePointX").c_str()))
        {
            MasteringDisplayInfo mdi{};
            mdi.redPrimaryX   = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".redPrimaryX").c_str());
            mdi.redPrimaryY   = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".redPrimaryY").c_str());
            mdi.greenPrimaryX = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".greenPrimaryX").c_str());
            mdi.greenPrimaryY = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".greenPrimaryY").c_str());
            mdi.bluePrimaryX  = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".bluePrimaryX").c_str());
            mdi.bluePrimaryY  = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".bluePrimaryY").c_str());
            mdi.whitePointX   = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".whitePointX").c_str());
            mdi.whitePointY   = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".whitePointY").c_str());
            mdi.maxLuminance  = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".maxLuminance").c_str());
            mdi.minLuminance  = ut_kvp_getUInt32Field(kvpTestInstance, (overrideMdi + ".minLuminance").c_str());
            testDetails.overrideMasteringDisplayInfo = mdi;
        }

        string overrideCll = prefix + "override_content_light_level";
        if (ut_kvp_fieldPresent(kvpTestInstance, (overrideCll + ".maxCLL").c_str()))
        {
            ContentLightLevel cll{};
            cll.maxCLL  = ut_kvp_getUInt32Field(kvpTestInstance, (overrideCll + ".maxCLL").c_str());
            cll.maxFALL = ut_kvp_getUInt32Field(kvpTestInstance, (overrideCll + ".maxFALL").c_str());
            testDetails.overrideContentLightLevel = cll;
        }

        string overridePAR = prefix + "override_pixel_aspect_ratio";
        if (ut_kvp_fieldPresent(kvpTestInstance, (overridePAR + ".numerator").c_str()))
        {
            videodecoder::Fraction par{};
            par.numerator   = static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, (overridePAR + ".numerator").c_str()));
            par.denominator = static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, (overridePAR + ".denominator").c_str()));
            testDetails.overridePixelAspectRatio = par;
        }

        // In-band resolution/bitrate ladder breakpoints (AdaptiveBitrate streams only)
        string resolutionChanges = prefix + "resolution_changes";
        int32_t resolutionChangeCount = ut_kvp_getListCount(kvpTestInstance, resolutionChanges.c_str());
        for (int32_t j = 0; j < resolutionChangeCount; j++)
        {
            string entryPrefix = resolutionChanges + "." + to_string(j) + ".";
            ResolutionChange_t change{};
            change.pts    = static_cast<int64_t>(ut_kvp_getUInt64Field(kvpTestInstance, (entryPrefix + "pts").c_str()));
            change.width  = static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, (entryPrefix + "width").c_str()));
            change.height = static_cast<int32_t>(ut_kvp_getUInt32Field(kvpTestInstance, (entryPrefix + "height").c_str()));
            change.bitRate = ut_kvp_getUInt32Field(kvpTestInstance, (entryPrefix + "bit_rate").c_str());
            testDetails.resolutionChanges.push_back(change);
        }

        testConfiguration.push_back(testDetails);
        UT_LOG_INFO("Added test configuration for stream %d, frameDetails size: %zu, expectedPts size: %zu, resolutionChanges size: %zu",
                    i, testDetails.frameDetails.size(), testDetails.expectedPts.size(), testDetails.resolutionChanges.size());
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
