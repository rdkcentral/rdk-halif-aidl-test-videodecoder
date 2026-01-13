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
 * @defgroup Video_Decoder_Halif_Test_L1 Video Decoder HALIF Tests L1 File
 * @{
 * @parblock
 *
 * ### L1 Tests for Video Decoder HALIF AIDL :
 *
 * Level 1 test cases for all APIs of Video Decoder HALIF AIDL Interface
 *
 * **Pre-Conditions:**  None@n
 * **Dependencies:** None@n
 *
 * Ref to API Definition specification documentation :
 *
 * @endparblock
 */


/**
 * @file vts_videodecoder_l1_tests.cpp
 *
 */

#include "vts_videodecoder.h"

using ::com::rdk::hal::State;

// Properties writable in READY only
static const std::vector<videodecoder::Property> validIntPropertyReady = {
    videodecoder::Property::LOW_LATENCY_MODE,
    videodecoder::Property::DECODE_ERROR_POLICY,
    videodecoder::Property::AV_SOURCE
};

// Properties writable in READY and STARTED
static const std::vector<videodecoder::Property> validIntPropertyAll = {
    videodecoder::Property::OPERATIONAL_MODE,
    videodecoder::Property::SHA1_CALC
};

// Define valid property values as standalone constants
static const int32_t intValueOpModeTunnelled = static_cast<int32_t>(OperationalMode::TUNNELLED); // 0
static const int32_t intValueSHA1On = 1;
static const int32_t intValueLowLatencyOn = 1;
static const int32_t intValueDecodeErrorConceal = 1;
static const int32_t intValueAVSourceUnknown = 0;


/**
 * @brief Test the retrieval of video decoder IDs from the VideoDecoderManager service.
 *
 * This test verifies that the VideoDecoderManager service can successfully return a list of video decoder IDs.
 * It checks the status of the operation, ensures that the list is not empty,
 * and validates that the number of IDs matches the expected count. Additionally,
 * it confirms that each expected ID is present in the retrieved list.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 001@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize decoderIds vector to store video decoder IDs. | decoderIds = empty vector | Should be successful | |
 * | 02 | Call getVideoDecoderIds to retrieve video decoder IDs. | OUT: decoderIds | status.isOk() = true, decoderIds not empty | Should Pass |
 * | 03 | Log the result of getVideoDecoderIds call. | decoderIds.size(), status | Log should show size and status | Should be successful |
 * | 04 | Assert that the status is OK. | status.isOk() | Assertion should pass | Should Pass |
 * | 05 | Assert that decoderIds is not empty. | decoderIds | Assertion should pass | Should Pass |
 * | 06 | Retrieve expected count of video decoder IDs. | expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT("IVideoDecoderManager.IVideoDecoder") | Should be successful | |
 * | 07 | Assert that the size of decoderIds matches expected count. | decoderIds.size(), expectedCount | Assertion should pass | Should Pass |
 * | 08 | Iterate over expected IDs and verify each is present in decoderIds. | expectedId, decoderIds | Assertion should pass for each ID | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder,getVideoDecoderIds_pos) {
    std::vector<videodecoder::IVideoDecoder::Id> decoderIds;
    size_t expectedCount = 0;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    std::string videodecoder_count_path = "IVideoDecoderManager.IVideoDecoder";
    expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT(videodecoder_count_path.c_str());
    UT_LOG_INFO("Expected count from profile: %zu", expectedCount);
    UT_ASSERT_TRUE(expectedCount > 0) << "Profile returned invalid count.";
    UT_ASSERT_EQUAL(decoderIds.size(), expectedCount) << "Decoder ID count mismatch.";

    for (size_t i = 0; i < expectedCount; ++i) {
        std::string videodecoderidpath = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".id";
        int32_t expectedId = UT_KVP_PROFILE_GET_UINT32(videodecoderidpath.c_str());
        UT_LOG_INFO("Checking ID at %s: %d", videodecoderidpath.c_str(), expectedId);
        auto it = std::find_if(decoderIds.begin(), decoderIds.end(), [expectedId](const auto& id) {
            return id.value == expectedId;
        });
        UT_ASSERT_TRUE(it != decoderIds.end()) << "Expected VideoDecoder ID not found: " << expectedId;
    }
}


/**
 * @brief Test to verify the retrieval of supported operational modes from the VideoDecoderManager.
 *
 * This test checks the functionality of the getSupportedOperationalModes API to ensure it correctly retrieves
 * the list of supported operational modes. It verifies that the API call is successful, the list is not empty,
 * and the modes match the expected configuration.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 002@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize supportedModes vector to store operational modes. | supportedModes = empty vector | Should be successful | Should be successful |
 * | 02 | Call getSupportedOperationalModes to retrieve supported modes. | supportedModes = empty vector | status.isOk() = true, supportedModes not empty | Should Pass |
 * | 03 | Verify the size of supportedModes matches expected count from configuration. | supportedModes.size() = actual size, expectedCount = value from config | supportedModes.size() == expectedCount | Should Pass |
 * | 04 | Iterate through each expected mode and verify it exists in supportedModes. | expectedMode = value from config, supportedModes = list of modes | Each expectedMode found in supportedModes | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, getSupportedOperationalModes_pos) {
    std::vector<videodecoder::OperationalMode> supportedModes;

    VIDEODECODER_MANAGER_GETSUPPORTEDOPERATIONALMODES(supportedModes);

    std::string operationalmode_count_path = "IVideoDecoderManager.supportedOperationalModes";
    size_t expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT(operationalmode_count_path.c_str());

    UT_ASSERT_EQUAL(supportedModes.size(), expectedCount) << "Mismatch in number of supported operational modes.";

    for (size_t i = 0; i < expectedCount; ++i) {
        char pszReturnedString[UT_KVP_MAX_ELEMENT_SIZE];
        std::string operationalmode_path = "IVideoDecoderManager.supportedOperationalModes." + std::to_string(i);
        UT_KVP_PROFILE_GET_STRING(operationalmode_path.c_str(), pszReturnedString);
        OperationalMode expectedMode;
        if (strcmp(pszReturnedString, "TUNNELLED") == 0) {
            expectedMode = OperationalMode::TUNNELLED;
        } else if (strcmp(pszReturnedString, "NON_TUNNELLED") == 0) {
            expectedMode = OperationalMode::NON_TUNNELLED;
        } else if (strcmp(pszReturnedString, "GRAPHICS_TEXTURE") == 0) {
            expectedMode = OperationalMode::GRAPHICS_TEXTURE;
        } else {
            UT_FAIL("Unsupported operational mode in YAML configuration.");
        }
        UT_ASSERT_TRUE(std::find(supportedModes.begin(), supportedModes.end(), expectedMode) != supportedModes.end())
            << "Operational mode " << pszReturnedString << " not found in supported modes.";
    }
}


/**
 * @brief Test the retrieval of video decoder instances using valid decoder IDs.
 *
 * This test verifies the functionality of retrieving video decoder instances from the service using valid decoder IDs obtained from a YAML configuration. It ensures that the service correctly returns non-null video decoder objects and that the operation status is successful.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 003@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve Video Decoder IDs from YAML configuration | decoderIds = empty | decoderIds should not be empty | Should be successful |
 * | 02 | Prepare the videoDecoderId using the macro | videoDecoderId.value = UT_KVP_PROFILE_GET_UINT32("IVideoDecoderManager.IVideoDecoder.<index>.0") | videoDecoderId should be valid | Should be successful |
 * | 03 | Call getVideoDecoder with the valid videoDecoderId | IN: videoDecoderId, OUT: videoDecoder | videoDecoder should not be null | Should Pass |
 * | 04 | Verify that the return value is Status::ok() | status = gVideoDecoderManagergetVideoDecoder(videoDecoderId, &videoDecoder) | status.isOk() should be true | Should Pass |
 * | 05 | Log the result | videoDecoderId.value, videoDecoder.get(), status.isOk(), status.exceptionCode() | Log should reflect correct values | Should be successful |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder,getVideoDecoder_pos) {
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds); // Get IDs first

    int i = 0;
    for (const auto& id : decoderIds) {
        IVideoDecoder::Id videoDecoderId;
        std::string decoder_id_path = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".id";
        videoDecoderId.value = UT_KVP_PROFILE_GET_UINT32(decoder_id_path.c_str());
        i++;

        sp<IVideoDecoder> videoDecoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(videoDecoderId, videoDecoder); // Macro replaces the call and checks
    }
}

/**
 * @brief Test the behavior of getVideoDecoder with invalid inputs
 *
 * This test evaluates the getVideoDecoder function's response to invalid inputs, ensuring that it handles errors gracefully and returns appropriate status codes.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 004@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize an invalid Video Decoder ID using UNDEFINED | invalidDecoderId.value = IVideoDecoder::Id::UNDEFINED | videoDecoder should be nullptr, status.exceptionCode() should be EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 02 | Call getVideoDecoder with invalid Video Decoder ID | input: invalidDecoderId, output: videoDecoder | videoDecoder should be nullptr, status.exceptionCode() should be EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 03 | Initialize a valid Video Decoder ID from configuration | validDecoderId.value = UT_KVP_PROFILE_GET_UINT32("IVideoDecoderManager.IVideoDecoder.<index>.0") | Should be successful | Should be successful |
 * | 04 | Call getVideoDecoder with a valid Video Decoder ID and null videoDecoder instance | input: validDecoderId, output: nullptr | videoDecoder should be nullptr, status.exceptionCode() should be EX_NULL_POINTER | Should Fail |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder,getVideoDecoder_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    IVideoDecoder::Id invalidID;
    sp<IVideoDecoder> videoDecoder;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds); // Get IDs first

    if(decoderIds.size() > 1)
        std::sort(decoderIds.begin(), decoderIds.end());

    invalidID = decoderIds.front();
    invalidID.value -= 1;

    VIDEODECODER_MANAGER_GETVIDEODECODER_FAILURE(invalidID, videoDecoder);

    invalidID = decoderIds.back();
    invalidID.value += 1;

    VIDEODECODER_MANAGER_GETVIDEODECODER_FAILURE(invalidID, videoDecoder);

    // Test Case 1: Invalid Video Decoder ID
    IVideoDecoder::Id invalidDecoderId;
    invalidDecoderId.value = IVideoDecoder::Id::UNDEFINED; // Using UNDEFINED as invalid ID

    VIDEODECODER_MANAGER_GETVIDEODECODER_FAILURE(invalidDecoderId, videoDecoder);

}

/**
 * @brief Test the positive scenario for retrieving video decoder capabilities.
 *
 * This test verifies the functionality of the `getCapabilities` method for video decoders. It checks if the method correctly retrieves the capabilities of each decoder, including supported codecs, dynamic ranges, and secure support.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 005@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and retrieve the list of video decoders. | decoders = list of video decoders | Decoders list should be initialized | Should be successful |
 * | 02 | Iterate over each decoder and call the `getCapabilities` method. | decoder[i], capabilities = OUT | Status should be OK | Should Pass |
 * | 03 | Verify the count of supported codecs against the expected count from the profile. | codecCount = expected count | Supported codecs count should match | Should Pass |
 * | 04 | For each codec, verify the codec type and its attributes like profile, level, maxFrameRate, maxFrameWidth, and maxFrameHeight. | codecCapability, profile, level, maxFrameRate, maxFrameWidth, maxFrameHeight | Codec type should be valid, attributes should match | Should Pass |
 * | 05 | Verify the count of supported dynamic ranges against the expected count from the profile. | dynamicRangeCount = expected count | Supported dynamic ranges count should match | Should Pass |
 * | 06 | For each dynamic range, verify its presence in the capabilities. | dynamicRangeValue | Dynamic range should be found | Should Pass |
 * | 07 | Verify the supportsSecure flag against the expected value from the profile. | supportsSecure = expected value | SupportsSecure flag should match | Should Pass |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, getCapabilities_pos)
{
    videodecoder::Capabilities capabilities={};
    std::vector<IVideoDecoder::Id> decoderIds={};
    android::sp<IVideoDecoder> videoDecoder;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_LOG_INFO("Found %zu video decoder IDs", decoderIds.size());
    UT_ASSERT_TRUE(!decoderIds.empty());

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        UT_ASSERT_TRUE(videoDecoder != nullptr);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        /* Log capabilities for debugging */
        UT_LOG_INFO("Decoder ID: %d", _decoderId.value);
        UT_LOG_INFO("Supports Secure: %d", capabilities.supportsSecure);
        UT_LOG_INFO("Supported Codecs: %zu", capabilities.supportedCodecs.size());
        for (const auto& codecCap : capabilities.supportedCodecs)
        {
            UT_LOG_INFO("Codec: %d, Profile: %d, Level: %d, maxFrameRate: %d, maxFrameWidth: %d, maxFrameHeight: %d",
                        static_cast<int32_t>(codecCap.codec),
                        static_cast<int32_t>(codecCap.profile),
                        static_cast<int32_t>(codecCap.level),
                        codecCap.maxFrameRate,
                        codecCap.maxFrameWidth,
                        codecCap.maxFrameHeight);
        }
        UT_LOG_INFO("Supported Dynamic Ranges: %zu", capabilities.supportedDynamicRanges.size());
        for (const auto& dr : capabilities.supportedDynamicRanges)
        {
            UT_LOG_INFO("Dynamic Range: %d", static_cast<int32_t>(dr));
        }

        /* Validate capabilities */
        vts_videodecoder_helper_validateCapabilities(_decoderId.value, capabilities);
    }
}

/**
 * @brief Test the positive scenario of retrieving properties from VideoDecoder
 *
 * This test verifies the ability of the VideoDecoder to correctly retrieve various properties such as RESOURCE_ID, OPERATIONAL_MODE, SECURE_VIDEO, and METRIC_FRAMES_DECODED. It ensures that the decoder returns the expected property values without any errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 006@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and iterate over each decoder | decoders = list of decoders | Should be successful | |
 * | 02 | Call getProperty with RESOURCE_ID and check the result | property = RESOURCE_ID, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 * | 03 | Call getProperty with OPERATIONAL_MODE and check the result | property = OPERATIONAL_MODE, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 * | 04 | Call getProperty with SECURE_VIDEO and check the result | property = SECURE_VIDEO, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 * | 05 | Call getProperty with METRIC_FRAMES_DECODED and check the result | property = METRIC_FRAMES_DECODED, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 */

 UT_ADD_TEST(VTS_L1_VideoDecoder, getProperty_pos) {
    // Fetch decoder IDs from the manager
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    // Iterate over each decoder ID and get the corresponding decoder
    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;
        ::std::optional<PropertyValue> aidl_return;

        // Fetch decoder_id dynamically
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::RESOURCE_ID, aidl_return);
        // Correctly access the intValue from the union
        int decoder_id = aidl_return->value->get<PropertyValue::Value::Tag::intValue>();
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::RESOURCE_ID)) << "RESOURCE_ID validation failed";

        // Test INPUT_QUEUE_DEPTH
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::INPUT_QUEUE_DEPTH, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::INPUT_QUEUE_DEPTH)) << "INPUT_QUEUE_DEPTH validation failed";

        // Test OUTPUT_FRAME_POOL_SIZE
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::OUTPUT_FRAME_POOL_SIZE, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::OUTPUT_FRAME_POOL_SIZE)) << "OUTPUT_FRAME_POOL_SIZE validation failed";

        // Test OPERATIONAL_MODE
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::OPERATIONAL_MODE, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::OPERATIONAL_MODE)) << "OPERATIONAL_MODE validation failed";

        // Test LOW_LATENCY_MODE
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::LOW_LATENCY_MODE, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::LOW_LATENCY_MODE)) << "LOW_LATENCY_MODE validation failed";

        // Test DECODE_ERROR_POLICY
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::DECODE_ERROR_POLICY, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::DECODE_ERROR_POLICY)) << "DECODE_ERROR_POLICY validation failed";

        // Test AV_SOURCE
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::AV_SOURCE, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::AV_SOURCE)) << "AV_SOURCE validation failed";

        // Test SHA1_CALC
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::SHA1_CALC, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::SHA1_CALC)) << "SHA1_CALC validation failed";

        // Test SECURE_VIDEO
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::SECURE_VIDEO, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::SECURE_VIDEO)) << "SECURE_VIDEO validation failed";

        // Test METRIC_FRAMES_DECODED
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::METRIC_FRAMES_DECODED, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::METRIC_FRAMES_DECODED)) << "METRIC_FRAMES_DECODED validation failed";

        // Test METRIC_DECODE_ERRORS
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::METRIC_DECODE_ERRORS, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::METRIC_DECODE_ERRORS)) << "METRIC_DECODE_ERRORS validation failed";

        // Test METRIC_FRAMES_DROPPED
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::METRIC_FRAMES_DROPPED, aidl_return);
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return,
            videodecoder::Property::METRIC_FRAMES_DROPPED)) << "METRIC_FRAMES_DROPPED validation failed";
    }
}

/**
 * @brief Test the negative scenario for the VideoDecoder getProperty function
 *
 * This test checks the behavior of the VideoDecoder's getProperty function when an invalid property is requested. It ensures that the function handles unknown properties gracefully and returns the expected error status.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 007@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and prepare the list of decoders | decoders = list of VideoDecoder instances | Should be successful | |
 * | 02 | Iterate over each decoder and call the getProperty function with an invalid property | property = UNKNOWN_PROPERTY, aidl_return = null | status.isOk() = false, aidl_return.has_value() = false | Should Fail |
 * | 03 | Log the result of the getProperty function call | aidl_return = null, status = error status | Should be successful | |
 * | 04 | Assert that the status is not OK and the aidl_return is empty | status.isOk() = false, aidl_return.has_value() = false | Should be successful | |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, getProperty_neg) {
    Status status;
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds); // Get IDs first

    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;
        ::std::optional<PropertyValue> aidl_return;
        UT_LOG_INFO("Calling getProperty(IN:property:[UNKNOWN_PROPERTY], OUT:_aidl_return[])");
        status = decoder->getProperty(static_cast<videodecoder::Property>(-1), &aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[UNKNOWN_PROPERTY], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                    aidl_return.has_value() ? "not null" : "null",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
        UT_ASSERT_FALSE(aidl_return.has_value());
    }
}



/**
 * @brief Test the positive scenario of retrieving multiple properties from the VideoDecoder.
 *
 * This test verifies the functionality of the getPropertyMulti method in the VTS_L1_VideoDecoder class.
 * It checks whether multiple properties can be successfully retrieved and populated in the PropertyKVPair list.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 008@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the propertyKVList with valid properties | propertyKVList initialized with RESOURCE_ID, INPUT_QUEUE_DEPTH, OUTPUT_FRAME_POOL_SIZE, OPERATIONAL_MODE, SECURE_VIDEO, METRIC_FRAMES_DECODED, METRIC_DECODE_ERRORS, METRIC_FRAMES_DROPPED | Properties should be initialized successfully | Should be successful |
 * | 02 | Call getPropertyMulti with the initialized propertyKVList | IN: propertyKVList, OUT: _aidl_return | Status should be OK, _aidl_return should be true | Should Pass |
 * | 03 | Verify each PropertyKVPair in the list has its propertyValue correctly populated | propertyKVList | Each propertyValue should have a value | Should be successful |
 */

 UT_ADD_TEST(VTS_L1_VideoDecoder, getPropertyMulti_pos) {
    // Fetch decoder IDs from the manager
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    // Iterate over each decoder ID and get the corresponding decoder
    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;

        // Use RESOURCE_ID to fetch the decoder_id dynamically
        ::std::optional<PropertyValue> aidl_return_single;
        VIDEODECODER_GETPROPERTY(decoder, videodecoder::Property::RESOURCE_ID, aidl_return_single);
        int decoder_id = aidl_return_single->value->get<PropertyValue::Value::Tag::intValue>();
        UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, aidl_return_single,
            videodecoder::Property::RESOURCE_ID)) << "RESOURCE_ID validation failed";

        // Populate properties list with properties to query
        std::vector<videodecoder::Property> properties;
        VIDEODECODER_ADD_PROPERTY(properties, RESOURCE_ID);
        VIDEODECODER_ADD_PROPERTY(properties, INPUT_QUEUE_DEPTH);
        VIDEODECODER_ADD_PROPERTY(properties, OUTPUT_FRAME_POOL_SIZE);
        VIDEODECODER_ADD_PROPERTY(properties, OPERATIONAL_MODE);
        VIDEODECODER_ADD_PROPERTY(properties, SECURE_VIDEO);
        VIDEODECODER_ADD_PROPERTY(properties, METRIC_FRAMES_DECODED);
        VIDEODECODER_ADD_PROPERTY(properties, METRIC_DECODE_ERRORS);
        VIDEODECODER_ADD_PROPERTY(properties, METRIC_FRAMES_DROPPED);

        // Call getPropertyMulti and validate result
        std::vector<PropertyKVPair> propertyKVList;
        bool aidl_return = false;
        VIDEODECODER_GETPROPERTYMULTI(decoder, properties, propertyKVList, aidl_return);

        // Validate each returned PropertyKVPair
        UT_ASSERT_EQUAL(propertyKVList.size(), properties.size())
            << "Returned propertyKVList size does not match input properties size.";

        for (size_t i = 0; i < propertyKVList.size(); i++) {
            UT_ASSERT_EQUAL(propertyKVList[i].property, properties[i])
                << "Property mismatch at index " << i;
            UT_ASSERT_TRUE(propertyKVList[i].propertyValue.value.has_value())
                << "Property " << static_cast<int>(propertyKVList[i].property) << " has no value.";
            UT_ASSERT_TRUE(vts_videodecoder_helper_validatePropertyValue(decoder_id, propertyKVList[i].propertyValue, propertyKVList[i].property))
                << "Validation failed for property " << static_cast<int>(propertyKVList[i].property);
        }
    }
}



/**
 * @brief Test the behavior of the VideoDecoder when invalid or empty property keys are provided.
 *
 * This test checks the VideoDecoder's response to invalid property keys, empty property lists, and null out-parameters. It ensures that the decoder handles these error scenarios gracefully and returns appropriate error codes per the API specification.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 009@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize invalid property key and add to list | propertiesInvalid = [-1] | aidlReturnInvalid = false | Should Fail |
 * | 02 | Call getPropertyMulti with invalid property list | propertiesInvalid, propertyKVListInvalid, aidlReturnInvalid | statusInvalid.isOk() = false, statusInvalid.exceptionCode() = EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 03 | Initialize empty property list | propertiesEmpty = [] | aidlReturnEmpty = false | Should Fail |
 * | 04 | Call getPropertyMulti with empty property list | propertiesEmpty, propertyKVListEmpty, aidlReturnEmpty | statusEmpty.isOk() = false, statusEmpty.exceptionCode() = EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 05 | Initialize valid properties list | propertiesValid = [RESOURCE_ID, INPUT_QUEUE_DEPTH] | Should be successful | Should be successful |
 * | 06 | Call getPropertyMulti with null out-parameter (propertyKVList) | propertiesValid, NULL, aidlReturnNull | statusNull.isOk() = false, statusNull.exceptionCode() = EX_NULL_POINTER | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, getPropertyMulti_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    UT_ASSERT_FALSE(decoderIds.empty()) << "No decoder IDs available";
    sp<IVideoDecoder> decoder;
    VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(decoderIds[0], decoder);
    UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null";

    // Negative Test 1 - Invalid Property Key
    std::vector<videodecoder::Property> propertiesInvalid;
    propertiesInvalid.push_back(static_cast<videodecoder::Property>(-1));

    std::vector<PropertyKVPair> propertyKVListInvalid;
    bool aidlReturnInvalid = false;
    VIDEODECODER_GETPROPERTYMULTI_FAILURE(decoder, propertiesInvalid, propertyKVListInvalid, aidlReturnInvalid, Status::EX_ILLEGAL_ARGUMENT);

    // Negative Test 2 - Empty Property List
    // Per API spec: "Passing an empty `properties` array is an error" with EX_ILLEGAL_ARGUMENT
    std::vector<videodecoder::Property> propertiesEmpty;
    std::vector<PropertyKVPair> propertyKVListEmpty;
    bool aidlReturnEmpty = false;
    VIDEODECODER_GETPROPERTYMULTI_FAILURE(decoder, propertiesEmpty, propertyKVListEmpty, aidlReturnEmpty, Status::EX_ILLEGAL_ARGUMENT);

    // Negative Test 3 - Null out-parameter (propertyKVList)
    // Per API spec: "If a required out-parameter is null (e.g. `propertyKVList`), the call fails with `EX_NULL_POINTER`"
    std::vector<videodecoder::Property> propertiesValid;
    VIDEODECODER_ADD_PROPERTY(propertiesValid, RESOURCE_ID);
    VIDEODECODER_ADD_PROPERTY(propertiesValid, INPUT_QUEUE_DEPTH);

    std::vector<PropertyKVPair> propertyKVListNull;
    bool aidlReturnNull = false;
    VIDEODECODER_GETPROPERTYMULTI_FAILURE(decoder, propertiesValid, propertyKVListNull, aidlReturnNull, Status::EX_NULL_POINTER);

}

/**
 * @brief Test the positive scenario of getting the state of a video decoder.
 *
 * This test verifies that the `getState` function of the video decoder returns a valid state and that the operation completes successfully. It ensures that the decoder is in one of the expected states.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 010@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and prepare the list of decoders. | decoders = list of video decoders | Decoders list should be initialized | Should be successful |
 * | 02 | Iterate over each decoder and call the `getState` function to retrieve the current state. | decoder = current decoder, state = output state | Status should be OK, state should be valid | Should Pass |
 * | 03 | Verify that the returned state is one of the expected states: UNKNOWN, CLOSED, OPENING, READY, STARTING, STARTED, FLUSHING, STOPPING, CLOSING. | state = output state | State should be one of the expected values | Should Pass |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, getState_pos) {
    if (decoderIds.empty()) {
        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    }
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        sp<IVideoDecoder> videoDecoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);

        videodecoder::Capabilities capabilities;
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing getState for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            // Test getState
            //State state;
            //VIDEODECODER_GETSTATE(videoDecoder, state);
            //UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::READY) << "State mismatch after open";

            // Start to STARTED
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            // Cleanup
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
        }
    }
}

/**
 * @brief Test the positive scenario of opening a video decoder.
 *
 * This test verifies the ability to open a video decoder with various codecs and secure modes. It ensures that the decoder can be opened successfully and that a valid controller is returned.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 011@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize listener for video decoder controller. | listener = new VideoDecoderControllerListener() | Listener should be initialized successfully. | Should be successful |
 * | 02 | Retrieve secure mode from YAML configuration. | securePathKey = "IVideoDecoderManager.IVideoDecoder.i.supportsSecure" | Secure mode should be retrieved successfully. | Should be successful |
 * | 03 | Retrieve supported codecs count from YAML configuration. | codecCountKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs" | Codec count should be retrieved successfully. | Should be successful |
 * | 04 | Loop through each supported codec and retrieve codec name. | codecPathKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs.j" | Codec name should be retrieved successfully. | Should be successful |
 * | 05 | Map codec name to Codec enum. | codecName = "MPEG"/"H264"/"H265"/"VP9"/"AV1" | Codec should be mapped successfully. | Should be successful |
 * | 06 | Call open function with codec, secure mode, and listener. | codec = Codec::MPEG/H264/H265/VP9/AV1, secure = true/false, listener = VideoDecoderControllerListener | Decoder should open successfully, controller should not be null. | Should Pass |
 * | 07 | Assert that the status is OK and controller is not null. | status.isOk() = true, controller != nullptr | Assertions should pass. | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, open_pos) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec;
            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            ::com::rdk::hal::State beforeState;
            VIDEODECODER_GETSTATE(videoDecoder, beforeState);
            UT_ASSERT_EQUAL(beforeState, ::com::rdk::hal::State::CLOSED) << "Decoder must be in CLOSED state before open";

            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            ::com::rdk::hal::State currentState;
            VIDEODECODER_GETSTATE(videoDecoder, currentState);
            UT_ASSERT_EQUAL(currentState, ::com::rdk::hal::State::READY) << "Decoder must be in READY state after open";

            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
        }
    }
}


/**
 * @brief Test the negative scenarios for opening a video decoder.
 *
 * This test evaluates the behavior of the video decoder when attempting to open it with unsupported codecs, unsupported secure modes, and without meeting preconditions. It ensures that the decoder handles these scenarios gracefully and returns appropriate error statuses.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 012@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve secure mode from YAML configuration | securePathKey = "IVideoDecoderManager.IVideoDecoder.i.supportsSecure" | Secure mode value retrieved | Should be successful |
 * | 02 | Retrieve supported codecs from YAML configuration | codecCountKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs" | Supported codecs count retrieved | Should be successful |
 * | 03 | Test with unsupported codec | codec = 999, secure = secure, listener = listener, controller = nullptr | status.isOk() = true, controller = nullptr | Should Pass |
 * | 04 | Retrieve valid codec from YAML configuration | codecPathKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs.0", codecName = codecName | Valid codec value retrieved | Should be successful |
 * | 05 | Test with unsupported secure mode | codec = validCodec, secure = !secure, listener = listener, controller = nullptr | status.isOk() = true, controller = nullptr | Should Pass |
 * | 06 | Test without meeting precondition (not in CLOSED state) | codec = validCodec, secure = secure, listener = listener, controller = nullptr | status.isOk() = false, status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, open_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    std::vector<videodecoder::Codec> allCodecs = { videodecoder::Codec::AV1, videodecoder::Codec::H264_AVC, videodecoder::Codec::H265_HEVC,videodecoder::Codec::MPEG2_VIDEO,videodecoder::Codec::VP9 };
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        sp<IVideoDecoderController> videoDecoderController;
        sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

        // Select a valid codec
        videodecoder::Codec validCodec = capabilities.supportedCodecs.empty() ?
            videodecoder::Codec::H264_AVC : capabilities.supportedCodecs.front().codec;

        bool isCodecFound = false;
        for ( const auto& codecVal: allCodecs){
            for (const auto& codecCap : capabilities.supportedCodecs){
                if(codecCap.codec == codecVal){
                    isCodecFound = true;
                    break;
                }
            }
            if(!isCodecFound)
            {
                VIDEODECODER_OPEN_FAILURE(videoDecoder, codecVal, capabilities.supportsSecure, controllerListener,
                                  videoDecoderController, Status::EX_ILLEGAL_ARGUMENT);
            }
            isCodecFound = false;
        }

        // Test 3: Unsupported secure mode
        bool unsupportedSecure = !capabilities.supportsSecure;
        VIDEODECODER_OPEN_FAILURE(videoDecoder, validCodec, unsupportedSecure, controllerListener,
                                  videoDecoderController, Status::EX_ILLEGAL_ARGUMENT);

        // Test 4: Null listener
        VIDEODECODER_OPEN_FAILURE(videoDecoder, validCodec, capabilities.supportsSecure, nullptr,
                                  videoDecoderController, Status::EX_NULL_POINTER);

        // Test 5: Illegal state
        VIDEODECODER_OPEN_SUCCESS(videoDecoder, validCodec, capabilities.supportsSecure, controllerListener,
                                  videoDecoderController, true);
        VIDEODECODER_OPEN_FAILURE(videoDecoder, validCodec, capabilities.supportsSecure, controllerListener,
                                  videoDecoderController, Status::EX_ILLEGAL_STATE);

        VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
    }
}


/**
 * @brief Test the positive scenario of closing a video decoder.
 *
 * This test verifies the functionality of the close API for video decoders. It ensures that the video decoder can be successfully closed after being started, and checks the return status and result of the close operation.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 013@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if VideoDecoderController is available | controllers = non-empty | Should be successful | |
 * | 02 | Start the video decoder to ensure it is in the READY state | controller = valid | status.isOk() = true | Should be successful |
 * | 03 | Log the call to the close API | controller = valid | Log entry created | Should be successful |
 * | 04 | Invoke the close API on the video decoder | controller = valid, result = false | status.isOk() = true, result = true | Should Pass |
 * | 05 | Log the result of the close API call | controller = valid, result = true | Log entry created | Should be successful |
 * | 06 | Verify the return value and output parameter of the close API | status.isOk() = true, result = true | status.isOk() = true, result = true | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, close_pos) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec; // Extract Codec from CodecCapabilities
            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            // Open the decoder
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);

            // Close the decoder
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
        }
    }
}

/**
 * @brief Test the negative scenario of closing a VideoDecoder when not in READY state.
 *
 * This test verifies the behavior of the VideoDecoder's close function when the decoder is not in the READY state. It ensures that the function fails as expected, which is crucial for maintaining the integrity of the decoder's state management.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 014@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if VideoDecoderController is available | controllers = non-empty | Should be successful | |
 * | 02 | Log the call to close API | controller = VideoDecoderController instance | Should be successful | |
 * | 03 | Invoke the close API on VideoDecoderController | controller = VideoDecoderController instance, result = false | result = false, status.isOk() = false | Should Fail |
 * | 04 | Log the result after calling the API | controller = VideoDecoderController instance, result = false, status = failure | Should be successful | |
 * | 05 | Verify the result of close API | result = false | Expected close to fail | Should Fail |
 * | 06 | Verify the status of close API | status.isOk() = false | Expected status to indicate failure | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, close_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        sp<IVideoDecoderController> videoDecoderController;
        sp<IVideoDecoderControllerListener> controllerListener =
            new VideoDecoderControllerListener();


        VIDEODECODER_CLOSE_FAILURE(videoDecoder, nullptr,  Status::EX_NULL_POINTER);

        // Test 2: Close when in CLOSED state (invalid state)
        videodecoder::Codec validCodec = capabilities.supportedCodecs.empty() ?
            videodecoder::Codec::H264_AVC : capabilities.supportedCodecs.front().codec;
        VIDEODECODER_OPEN_SUCCESS(videoDecoder, validCodec, capabilities.supportsSecure,
                                  controllerListener, videoDecoderController, true);
        VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController,  true);
        // Now in CLOSED, try closing again
        VIDEODECODER_CLOSE_FAILURE(videoDecoder, videoDecoderController,
            Status::EX_ILLEGAL_STATE);

        // Test 3: Close when in STARTED state (invalid state)
        VIDEODECODER_OPEN_SUCCESS(videoDecoder, validCodec, capabilities.supportsSecure,
                                  controllerListener, videoDecoderController, true);
        videoDecoderController->start(); // Transition to STARTED
        //State currentState;

        //Close with Null videoDecoderController
        sp<IVideoDecoderController> videoDecoderController_nullptr = nullptr;
        VIDEODECODER_CLOSE_FAILURE(videoDecoder, videoDecoderController_nullptr, Status::EX_NULL_POINTER);

        //VIDEODECODER_WAIT_FOR_STATE(videoDecoder,::com::rdk::hal::State::STARTED, currentState);
        //UT_ASSERT_EQUAL(currentState,::com::rdk::hal::State::STARTED) << "Decoder must be in STARTED state";
        VIDEODECODER_CLOSE_FAILURE(videoDecoder, videoDecoderController,
            Status::EX_ILLEGAL_STATE);

        // Clean up
        videoDecoderController->stop(); // Back to READY
        //VIDEODECODER_WAIT_FOR_STATE(videoDecoder,::com::rdk::hal::State::READY, currentState);
        VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
    }
}

/**
 * @brief Test the positive scenario of registering a video decoder event listener.
 *
 * This test verifies that the `registerEventListener` function successfully registers a video decoder event listener and returns the expected results. It ensures that the function call is successful and the listener is correctly registered.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 015@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a new instance of VideoDecoderEventListener. | listener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Iterate over each decoder in the decoders list. | decoders = list of decoders | Should iterate over all decoders | Should be successful |
 * | 03 | Call registerEventListener with the listener and check the status and aidl_return. | listener = VideoDecoderEventListener instance, aidl_return = false | status.isOk() should be true, aidl_return should be true | Should Pass |
 * | 04 | Log the result of registerEventListener call. | listener = VideoDecoderEventListener instance, aidl_return = result from call | Log should display the correct status and aidl_return | Should be successful |
 * | 05 | Assert that the status is OK and aidl_return is true. | status.isOk() = true, aidl_return = true | Assertions should pass | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, registerEventListener_pos) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);

        sp<IVideoDecoderEventListener> eventListener =  new VideoDecoderEventListener();

        // Register events
        VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

        // Unregister events
        VIDEODECODER_UNREGISTER_EVENT_SUCCESS(videoDecoder, eventListener);
    }
}



/**
 * @brief Test the negative scenario of registering the same event listener twice with a video decoder.
 *
 * This test checks the behavior of the video decoder when attempting to register the same event listener multiple times. The first registration should succeed, while the second should fail, ensuring that duplicate registrations are handled correctly.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 016@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a VideoDecoderEventListener instance | listener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Check if decoders list is not empty | decoders.empty() | Should be false | Should be successful |
 * | 03 | Retrieve the first decoder from the list | decoder = decoders.front() | Decoder instance should be retrieved successfully | Should be successful |
 * | 04 | Register the listener for the first time | listener = [0x%X], firstRegistrationResult = false | firstRegistrationResult = true, status.isOk() = true | Should Pass |
 * | 05 | Attempt to register the same listener again | listener = [0x%X], secondRegistrationResult = false | secondRegistrationResult = false, status.isOk() = true | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, registerEventListener_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    Status status;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    for (const auto& _decoderId : decoderIds) {
        sp<IVideoDecoderEventListener> eventListener =  new VideoDecoderEventListener();
        sp<IVideoDecoderEventListener> eventListener_nullptr = nullptr;

        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);

        // Test 1: Register with null listener
        VIDEODECODER_REGISTER_EVENT_FAILURE(videoDecoder, eventListener_nullptr,Status::EX_NULL_POINTER);

        // Test 2: Register a valid listener
        VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

        // Test 3: Register the same listener again (duplicate)
        VIDEODECODER_REGISTER_EVENT_FAILURE(videoDecoder, eventListener,Status::EX_NONE);

        // Clean up: Unregister the listener
        VIDEODECODER_UNREGISTER_EVENT_SUCCESS(videoDecoder, eventListener);
    }
}

/**
 * @brief Test the positive scenario of unregistering a VideoDecoder event listener.
 *
 * This test verifies the successful unregistration of an event listener from a VideoDecoder instance. It ensures that the event listener is correctly unregistered after being registered, which is crucial for managing event listeners and avoiding memory leaks or unwanted callbacks.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 017@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure that there are available VideoDecoder instances. | decoders.empty() = false | Assertion should pass | Should be successful |
 * | 02 | Create a new VideoDecoderEventListener instance. | videoDecoderEventListener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 03 | Register the event listener to the first VideoDecoder instance. | videoDecoderEventListener = [instance], registerResult = false | registerStatus.isOk() = true, registerResult = true | Should Pass |
 * | 04 | Log the call to unregister the event listener. | videoDecoderEventListener = [instance] | Log should be successful | Should be successful |
 * | 05 | Unregister the event listener from the first VideoDecoder instance. | videoDecoderEventListener = [instance], unregisterResult = false | unregisterStatus.isOk() = true, unregisterResult = true | Should Pass |
 * | 06 | Log the result of the unregister call. | videoDecoderEventListener = [instance], unregisterResult = [result], unregisterStatus = [status] | Log should be successful | Should be successful |
 * | 07 | Verify the unregistration status and result. | unregisterStatus.isOk() = true, unregisterResult = true | Assertion should pass | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, unregisterEventListener_pos) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);

        sp<IVideoDecoderEventListener> videoDecoderEventListener = new VideoDecoderEventListener();

        // Test: Unregister without prior registration
        VIDEODECODER_UNREGISTER_EVENT_FAILURE(videoDecoder, videoDecoderEventListener, Status::EX_NONE);
    }
}



/**
 * @brief Test the negative scenario of unregistering a video decoder event listener
 *
 * This test verifies the behavior of the `unregisterEventListener` function when attempting to unregister a video decoder event listener that has not been registered. The test ensures that the function handles this scenario gracefully without causing any errors or unexpected behavior.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 018@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a new instance of VideoDecoderEventListener | videoDecoderEventListener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Ensure the videoDecoderEventListener has not been registered | N/A | No prior registration should exist | Should be successful |
 * | 03 | Log the API call for unregisterEventListener | IN: videoDecoderEventListener, OUT: aidl_return | Log should be generated | Should be successful |
 * | 04 | Call the unregisterEventListener API | IN: videoDecoderEventListener, OUT: aidl_return | aidl_return should be false, status should be OK | Should Fail |
 * | 05 | Verify the return value and output parameter | aidl_return = false, status.isOk() = true | aidl_return should be false, status should be OK | Should be successful |
 * | 06 | Log the result of the API call | IN: videoDecoderEventListener, OUT: aidl_return, status | Log should be generated | Should be successful |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, unregisterEventListener_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);

        sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();
        sp<IVideoDecoderEventListener> eventListener_nullptr = nullptr;

        // Test 1: Unregister without prior registration
        VIDEODECODER_UNREGISTER_EVENT_FAILURE(videoDecoder, eventListener,
                                              Status::EX_NONE);

        // Test 2: Register events
        VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

        // Test 3: Unregister with null listener
        VIDEODECODER_UNREGISTER_EVENT_FAILURE(videoDecoder, eventListener_nullptr,
                                              Status::EX_NULL_POINTER);

        // Test 4: Unregister the events
        VIDEODECODER_UNREGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

        // Test 5: Unregister again after successful unregistration
        VIDEODECODER_UNREGISTER_EVENT_FAILURE(videoDecoder, eventListener,
                                              Status::EX_NONE);
    }
}

/**
 * @brief Test the positive scenario of starting the Video Decoder Controller
 *
 * This test verifies that the Video Decoder Controller can be successfully started when it is in the READY state. It ensures that the start API is called correctly and that the controller transitions to the expected state without errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 019@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve the current state of the Video Decoder to ensure it is READY | controller = <controller_instance>, currentState = <state_variable> | currentState should be READY | Should be successful |
 * | 02 | Log the action of calling the start API | None | Log should indicate "Calling start()" | Should be successful |
 * | 03 | Call the start API on the Video Decoder Controller | controller = <controller_instance> | status.isOk() should be true | Should Pass |
 * | 04 | Log the result of the start API call | status = <status_variable> | Log should indicate the status of the start call | Should be successful |
 * | 05 | Verify the return value of the start API indicates success | status = <status_variable> | status.isOk() should be true | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, start_pos) {

    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;

    // Use global decoderIds if already populated, otherwise fetch
    if (decoderIds.empty()) {
        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    }
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        sp<IVideoDecoder> videoDecoder;
        videodecoder::Capabilities capabilities;

        // Fetch decoder and capabilities
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing start for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            // Start to STARTED
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            // Stop to READY
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            // Close to CLOSED
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);

            // Optionally store in global containers if needed
            // controllers.push_back(videoDecoderController);
        }
    }
}



/**
 * @brief Test the behavior of VideoDecoderController when attempting to start an already started decoder.
 *
 * This test verifies that the VideoDecoderController correctly handles an attempt to start the video decoder
 * when it is already in the STARTED state. The expected behavior is that the start operation should fail
 * with an EX_ILLEGAL_STATE exception, indicating that the decoder is already started.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 020@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the Video Decoder is in the STARTED state by calling start() as a prerequisite. | controller = instance, status = result of start() | status.isOk() should be true | Should be successful |
 * | 02 | Attempt to start the Video Decoder again while it is already in the STARTED state. | controller = instance, status = result of start() | status.isOk() should be false, status.exceptionCode() should be EX_ILLEGAL_STATE | Should Fail |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, start_neg)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    android::sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_LOG_INFO("Found %zu video decoder IDs", decoderIds.size());
    UT_ASSERT_TRUE(!decoderIds.empty());

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        UT_ASSERT_TRUE(videoDecoder != nullptr);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);
        UT_LOG_INFO("Decoder ID: %d, Supports Secure: %d, Codecs: %zu",
                    _decoderId.value, capabilities.supportsSecure, capabilities.supportedCodecs.size());
        UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());

        for (const auto& codecCap : capabilities.supportedCodecs)
        {
            auto _codec = codecCap.codec;
            android::sp<IVideoDecoderController> videoDecoderController;
            android::sp<IVideoDecoderControllerListener> controllerListener =
                new VideoDecoderControllerListener();
            android::sp<IVideoDecoderEventListener> eventListener =
                new VideoDecoderEventListener();

            /* Open the decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            UT_ASSERT_TRUE(videoDecoderController != nullptr);

            /* Start the decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Start the decoder again (in STARTED state) */
            VIDEODECODER_START_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);

            /* Stop the decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Close the decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Start the decoder after close (in CLOSED state) */
            VIDEODECODER_START_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
 * @brief Test the positive scenario of stopping the Video Decoder Controller
 *
 * This test verifies that the Video Decoder Controller can be successfully stopped after being started.
 * It ensures that the stop function works correctly when the controller is in the STARTED state.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 021@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Start the Video Decoder Controller | controller = STARTED | Controller should start successfully | Should be successful |
 * | 02 | Log the call to stop the controller | None | Log should be recorded | Should be successful |
 * | 03 | Call the stop API on the Video Decoder Controller | controller = STOPPED | Controller should stop successfully | Should Pass |
 * | 04 | Log the result of the stop call | None | Log should be recorded | Should be successful |
 * | 05 | Verify the stop call was successful | status.isOk() = true | Assertion should pass | Should Pass |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, stop_pos)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    android::sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_LOG_INFO("Found %zu video decoder IDs", decoderIds.size());
    UT_ASSERT_TRUE(!decoderIds.empty());

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        UT_ASSERT_TRUE(videoDecoder != nullptr);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);
        UT_LOG_INFO("Decoder ID: %d, Supports Secure: %d, Codecs: %zu",
                    _decoderId.value, capabilities.supportsSecure, capabilities.supportedCodecs.size());
        UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());

        for (const auto& codecCap : capabilities.supportedCodecs)
        {
            auto _codec = codecCap.codec;
            android::sp<IVideoDecoderController> videoDecoderController;
            android::sp<IVideoDecoderControllerListener> controllerListener =
                new VideoDecoderControllerListener();
            android::sp<IVideoDecoderEventListener> eventListener =
                new VideoDecoderEventListener();

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            UT_ASSERT_TRUE(videoDecoderController != nullptr);

            /* Start the decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Stop the decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Close the decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
        }
    }
}


/**
 * @brief Test the negative scenario of stopping the VideoDecoderController when it is not in the STARTED state.
 *
 * This test verifies that the stop() function of the VideoDecoderController returns an EX_ILLEGAL_STATE exception when the controller is not in the STARTED state. This ensures that the stop() function behaves correctly under invalid conditions.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 022@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is not in the STARTED state. | N/A | N/A | Should be successful |
 * | 02 | Log the action before calling the stop API. | N/A | N/A | Should be successful |
 * | 03 | Call the stop() API on the VideoDecoderController. | controller = instance, method = stop() | status.exceptionCode() = EX_ILLEGAL_STATE | Should Pass |
 * | 04 | Log the result of the stop API call. | N/A | N/A | Should be successful |
 * | 05 | Verify that the stop API returns EX_ILLEGAL_STATE. | status.exceptionCode() = EX_ILLEGAL_STATE | Assertion should pass | Should Pass |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, stop_neg)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    android::sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_LOG_INFO("Found %zu video decoder IDs", decoderIds.size());
    UT_ASSERT_TRUE(!decoderIds.empty());

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        UT_ASSERT_TRUE(videoDecoder != nullptr);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);
        UT_LOG_INFO("Decoder ID: %d, Supports Secure: %d, Codecs: %zu",
                    _decoderId.value, capabilities.supportsSecure, capabilities.supportedCodecs.size());
        UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());

        for (const auto& codecCap : capabilities.supportedCodecs)
        {
            auto _codec = codecCap.codec;
            android::sp<IVideoDecoderController> videoDecoderController;
            android::sp<IVideoDecoderControllerListener> controllerListener =
                new VideoDecoderControllerListener();
            android::sp<IVideoDecoderEventListener> eventListener =
                new VideoDecoderEventListener();

            /* Open the decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            UT_ASSERT_TRUE(videoDecoderController != nullptr);

            /* Start the decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Stop the decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Stop the decoder again (in READY state) */
            VIDEODECODER_STOP_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);

            /* Close the decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Stop the decoder again (in CLOSED state) */
            VIDEODECODER_STOP_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}


/**
 * @brief Test the positive scenarios for setting properties on the VideoDecoderController.
 *
 * This test verifies that various properties can be successfully set on the VideoDecoderController when it is in the READY state.
 * It ensures that the setProperty function behaves as expected for different property values.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 023@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is in the READY state | controller = current controller, currentState = READY | currentState should be READY | Should be successful |
 * | 02 | Set Operational Mode to NON_TUNNELLED | property = OPERATIONAL_MODE, propertyValue = NON_TUNNELLED, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 03 | Set Low Latency Mode to ON | property = LOW_LATENCY_MODE, propertyValue = ON, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 04 | Set Decode Error Policy to Conceal Corrupt Frames | property = DECODE_ERROR_POLICY, propertyValue = Conceal Corrupt Frames, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 05 | Set AV Source to AVUNKNOWN | property = AV_SOURCE, propertyValue = AVUNKNOWN, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 06 | Set SHA1 Calculation to ON | property = SHA1_CALC, propertyValue = ON, result = false | status.isOk() should be true, result should be true | Should Pass |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, setProperty_pos)
{

    std::vector<IVideoDecoder::Id> decoderIds;
    android::sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    std::vector<videodecoder::OperationalMode> supportedModes;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_LOG_INFO("Found %zu video decoder IDs", decoderIds.size());
    UT_ASSERT_TRUE(!decoderIds.empty());

    /*Get Supported Operationl Modes*/
    VIDEODECODER_MANAGER_GETSUPPORTEDOPERATIONALMODES(supportedModes);

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        UT_ASSERT_TRUE(videoDecoder != nullptr);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);
        UT_LOG_INFO("Decoder ID: %d, Supports Secure: %d, Codecs: %zu",
                    _decoderId.value, capabilities.supportsSecure, capabilities.supportedCodecs.size());
        UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());

        for (const auto& codecCap : capabilities.supportedCodecs)
        {
            auto _codec = codecCap.codec;
            android::sp<IVideoDecoderController> videoDecoderController;
            android::sp<IVideoDecoderControllerListener> controllerListener =
                new VideoDecoderControllerListener();
            android::sp<IVideoDecoderEventListener> eventListener =
                new VideoDecoderEventListener();

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                    controllerListener, videoDecoderController, true);
            UT_ASSERT_TRUE(videoDecoderController != nullptr);

            /* Define property lists for READY state */
            std::vector<std::pair<videodecoder::Property, int32_t>> readyIntProps = {
                {videodecoder::Property::LOW_LATENCY_MODE, 1},      // On
                {videodecoder::Property::DECODE_ERROR_POLICY, 1},   // Conceal corrupt frames
                {videodecoder::Property::AV_SOURCE, 1},             // Non-zero AVSource
                {videodecoder::Property::SHA1_CALC, 1}              // On
            };

            /* Define property list for READY state again */
            std::vector<std::pair<videodecoder::Property, int32_t>> readyIntPropsAgain = {
                {videodecoder::Property::LOW_LATENCY_MODE, 1},      // On
                {videodecoder::Property::DECODE_ERROR_POLICY, 1},   // Conceal corrupt frames
                {videodecoder::Property::AV_SOURCE, 1},             // Non-zero AVSource
                {videodecoder::Property::SHA1_CALC, 1}              // On
            };

            /* Define property list for STARTED state */
            std::vector<std::pair<videodecoder::Property, int32_t>> startedIntProps = {
                {videodecoder::Property::SHA1_CALC, 0}  // Off
            };

            /* Operational modes setting*/
            for (const auto& _supportedModes : supportedModes)
            {
                readyIntProps.push_back({videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(_supportedModes)});
                readyIntPropsAgain.push_back({videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(_supportedModes)});
                startedIntProps.push_back({videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(_supportedModes)});
            }

            /* Set properties in READY state */
            VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, readyIntProps, intValue);

            /* Start the decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Set properties in STARTED state */
            VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, startedIntProps, intValue);

            /* Stop the decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Set properties in READY state again */
            VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, readyIntPropsAgain, intValue);

            /* Close the decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
        }
    }
}

/**
 * @brief Test the negative scenarios for setting properties in VideoDecoderController
 *
 * This test evaluates the behavior of the VideoDecoderController when attempting to set properties with invalid or unsupported values. It ensures that the controller correctly handles these scenarios by not allowing the property to be set and returning the appropriate status.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 024@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if VideoDecoderController is available for testing | controllers.empty() | Should be false | Should be successful |
 * | 02 | Set Operational Mode to an Unsupported Mode | property = OPERATIONAL_MODE, propertyValue = 999 | result = false, status.isOk() = false | Should Fail |
 * | 03 | Set Property in Invalid State | property = LOW_LATENCY_MODE, propertyValue = 1 | result = false, status.isOk() = false | Should Fail |
 * | 04 | Set Property with Invalid Value Type | property = DECODE_ERROR_POLICY, propertyValue = "invalid" | result = false, status.isOk() = false | Should Fail |
 * | 05 | Set Property with Out-of-Range Value | property = SHA1_CALC, propertyValue = 999 | result = false, status.isOk() = false | Should Fail |
 */

UT_ADD_TEST(VTS_L1_VideoDecoder, setProperty_neg)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    android::sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_LOG_INFO("Found %zu video decoder IDs", decoderIds.size());
    UT_ASSERT_TRUE(!decoderIds.empty());

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        UT_ASSERT_TRUE(videoDecoder != nullptr);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);
        UT_LOG_INFO("Decoder ID: %d, Supports Secure: %d, Codecs: %zu",
                    _decoderId.value, capabilities.supportsSecure, capabilities.supportedCodecs.size());
        UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());

        for (const auto& codecCap : capabilities.supportedCodecs)
        {
            auto _codec = codecCap.codec;
            android::sp<IVideoDecoderController> videoDecoderController;
            android::sp<IVideoDecoderControllerListener> controllerListener =
                new VideoDecoderControllerListener();
            android::sp<IVideoDecoderEventListener> eventListener =
                new VideoDecoderEventListener();

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            UT_ASSERT_TRUE(videoDecoderController != nullptr);

            /* Define invalid and valid property lists */
            std::vector<std::pair<videodecoder::Property, int32_t>> invalidIntPropertyAll = {
                {static_cast<videodecoder::Property>(9999), 1} // Unknown property
            };
            std::vector<std::pair<videodecoder::Property, int32_t>> invalidIntPropertyReady = {
                {static_cast<videodecoder::Property>(10000), 0} // Another unknown property
            };
            std::vector<std::pair<videodecoder::Property, int32_t>> validIntPropertyReady = {
                {videodecoder::Property::LOW_LATENCY_MODE, 1},
                {videodecoder::Property::DECODE_ERROR_POLICY, 1},
                {videodecoder::Property::AV_SOURCE, 1}
            };

            /* Define Read only property */
            std::vector<std::pair<videodecoder::Property, int32_t>> readOnlyProps = {
                {videodecoder::Property::RESOURCE_ID, 2},
                {videodecoder::Property::INPUT_QUEUE_DEPTH, 10},
                {videodecoder::Property::OUTPUT_FRAME_POOL_SIZE, 10},
                {videodecoder::Property::SECURE_VIDEO, 10},
                {videodecoder::Property::METRIC_FRAMES_DECODED, 1000},
                {videodecoder::Property::METRIC_DECODE_ERRORS, 1001},
                {videodecoder::Property::METRIC_FRAMES_DROPPED, 1002}
            };

            /* Set invalid properties in READY state */
            VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, invalidIntPropertyAll, intValue,
                                                  Status::EX_ILLEGAL_ARGUMENT);
            VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, invalidIntPropertyReady, intValue,
                                                  Status::EX_ILLEGAL_ARGUMENT);

            /* Start the decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Set valid properties in invalid state (STARTED) */
            VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, validIntPropertyReady, intValue,
                                                  Status::EX_ILLEGAL_STATE);

            /* Stop the decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Set read only  properties in state (READY) */
            VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, readOnlyProps, intValue,
                                                  Status::EX_UNSUPPORTED_OPERATION);

            /* Close the decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Set valid properties in invalid state (CLOSED) */
            VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, validIntPropertyReady, intValue,
                                                  Status::EX_ILLEGAL_STATE);
        }
    }
}



/**
 * @brief Test the negative scenarios for the decodeBuffer function in VideoDecoderController
 *
 * This test evaluates the behavior of the decodeBuffer function when provided with invalid inputs or states. It ensures that the function handles these scenarios gracefully by returning appropriate error codes and results.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 025@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Attempt to decode buffer without starting the decoder | nsPresentationTime = 3000000, bufferHandle = 1, result = false | result = false, exceptionCode = EX_ILLEGAL_STATE | Should Fail |
 * | 02 | Attempt to decode buffer with an invalid buffer handle | nsPresentationTime = 4000000, bufferHandle = -1, result = false | result = false, exceptionCode = EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 03 | Attempt to decode buffer with an invalid presentation time | nsPresentationTime = -1000000, bufferHandle = 1, result = false | result = false, exceptionCode = EX_ILLEGAL_ARGUMENT | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, decodeBuffer_neg) {

    std::vector<IVideoDecoder::Id> decoderIds;
    android::sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    if (decoderIds.empty()) {
        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    }
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        sp<IVideoDecoder> videoDecoder;
        videodecoder::Capabilities capabilities;

        // Fetch decoder and capabilities
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing start for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                          controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            bool result = false;
            Status status;

            // Test Case 3: Invalid State - Decoder Not Started
            UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[3000000], IN:bufferHandle:[1], OUT:_aidl_return[])");
            status = videoDecoderController->decodeBuffer(3000000, 1, &result);
            UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[3000000], IN:bufferHandle:[1], OUT:_aidl_return[%d]), status:[isOK:%s, exception:%s]", result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_FALSE(result);
            UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_ILLEGAL_STATE);

            /* Start the Decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            // Test Case 4: Invalid Buffer Handle
            UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[4000000], IN:bufferHandle:[-1], OUT:_aidl_return[])");
            status = videoDecoderController->decodeBuffer(4000000, -1, &result);
            UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[4000000], IN:bufferHandle:[-1], OUT:_aidl_return[%d]), status:[isOK:%s, exception:%s]", result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_FALSE(result);
            UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);

            // Test Case 5: Invalid nsPresentationTime
            UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[-1000000], IN:bufferHandle:[1], OUT:_aidl_return[])");
            status = videoDecoderController ->decodeBuffer(-1000000, 1, &result);
            UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[-1000000], IN:bufferHandle:[1], OUT:_aidl_return[%d]), status:[isOK:%s, exception:%s]", result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_FALSE(result);
            UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);

            //Reverting state to READY
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Close the Decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
        }
    }
}


/**
 * @brief Test the flush functionality of the VideoDecoderController with different reset values.
 *
 * This test verifies the behavior of the flush method in the VideoDecoderController when invoked with different reset values. It ensures that the decoder transitions to the correct states and handles the flush operation correctly.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 026@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the decoder and controller objects | controllers, openedDecoders | Should be successful | |
 * | 02 | Start the decoder to ensure it is in the STARTED state | controller, started = false | Decoder should start successfully | Should be successful |
 * | 03 | Flush the decoder with reset = true | reset = true | Flush should succeed, decoder should be in READY state | Should Pass |
 * | 04 | Stop the decoder to ensure it is in the READY state | controller, ready = false | Decoder should stop successfully | Should be successful |
 * | 05 | Start the decoder again to ensure it is in the STARTED state | controller, started = false | Decoder should start successfully | Should be successful |
 * | 06 | Flush the decoder with reset = false | reset = false | Flush should succeed, decoder should remain in STARTED state | Should Pass |
 * | 07 | Stop the decoder to ensure it remains in the STARTED state | controller, ready = false | Decoder should stop successfully | Should be successful |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, flush_pos) {
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        sp<IVideoDecoder> videoDecoder;
        videodecoder::Capabilities capabilities;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);
        UT_ASSERT_FALSE(capabilities.supportedCodecs.empty()) << "No supported codecs for decoder ID: " << _decoderId.value;

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing flush for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            // Start to STARTED
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            // Test flush without reset
            VIDEODECODER_FLUSH_SUCCESS(videoDecoder,videoDecoderController,false,true);

            // Test flush with reset
            VIDEODECODER_FLUSH_SUCCESS(videoDecoder,videoDecoderController,true,true);

            // Close
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
        }
    }
}


/**
 * @brief Test the negative flush operation of the VideoDecoderController
 *
 * This test verifies the behavior of the flush operation when the video decoder controller is not in the STARTED state.
 * It ensures that the flush operation returns an illegal state exception when invoked in an inappropriate state.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 027@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is in a state other than STARTED by stopping it | controller = instance, result = false | status.isOk() = true, result = true | Should be successful |
 * | 02 | Call the flush API with reset = true | reset = true | status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 * | 03 | Call the flush API with reset = false | reset = false | status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, flush_neg) {
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds) {
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing flush for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

            /* Test flush in CLOSED state (before open) */
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
            if (!videoDecoderController) {
                UT_LOG_INFO("Controller is null, expecting failure (CLOSED state)");
                //UT_EXPECT_TRUE(true) << "Test passes: flush not called on null controller";
            } else {
                VIDEODECODER_FLUSH_FAILURE(videoDecoderController, false, Status::EX_ILLEGAL_STATE);
                VIDEODECODER_FLUSH_FAILURE(videoDecoderController, true, Status::EX_ILLEGAL_STATE);
            }

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Test flush in READY state (after open) */
            VIDEODECODER_FLUSH_FAILURE(videoDecoderController, false, Status::EX_ILLEGAL_STATE);
            VIDEODECODER_FLUSH_FAILURE(videoDecoderController, true, Status::EX_ILLEGAL_STATE);

            /* Start the Decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            /* Stop the Decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Test flush in READY state (after stop) */
            VIDEODECODER_FLUSH_FAILURE(videoDecoderController, false, Status::EX_ILLEGAL_STATE);
            VIDEODECODER_FLUSH_FAILURE(videoDecoderController, true, Status::EX_ILLEGAL_STATE);

            /* Close the Decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);

            /* Test flush in CLOSED state (after close) */
            VIDEODECODER_FLUSH_FAILURE(videoDecoderController, false, Status::EX_ILLEGAL_STATE);
            VIDEODECODER_FLUSH_FAILURE(videoDecoderController, true, Status::EX_ILLEGAL_STATE);
        }
    }
}


/**
 * @brief Test the positive scenario of signaling discontinuity in VideoDecoderController
 *
 * This test verifies the behavior of the VideoDecoderController when signaling a discontinuity in the STARTED state. It ensures that the controller transitions correctly between states and handles the signalDiscontinuity call without errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 028@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the VideoDecoderController and ensure it is in the READY state by calling start() | controller = initialized, status = uninitialized | status.isOk() = true | Should be successful |
 * | 02 | Call signalDiscontinuity() to test the transition in the STARTED state | controller = started, status = uninitialized | status.isOk() = true | Should Pass |
 * | 03 | Call stop() to transition back to the READY state | controller = started, status = uninitialized | status.isOk() = true | Should be successful |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, signalDiscontinuity_pos)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing signalDiscontinuity for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Start the Decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            /* Signal discontinuity to the decoder */
            VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Stop the Decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Close the Decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
        }
    }
}


/**
 * @brief Test the negative scenario for the signalDiscontinuity API in VideoDecoderController
 *
 * This test verifies that the signalDiscontinuity API returns an error status when the Video Decoder is not in the STARTED state. The expected error is EX_ILLEGAL_STATE.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 029@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the Video Decoder is in a state other than STARTED by calling the stop function | controller = instance, result = false | status.isOk() = true, result = true | Should be successful |
 * | 02 | Call the signalDiscontinuity API on the controller | controller = instance | status.isOk() = false, status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, signalDiscontinuity_neg)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing signalDiscontinuity for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

            /* Test signalDiscontinuity in CLOSED state (before open) */
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
            if (!videoDecoderController) {
                UT_LOG_INFO("Controller is null, expecting failure (CLOSED state)");
                //UT_EXPECT_TRUE(true) << "Test passes: flush not called on null controller";
            } else {
                VIDEODECODER_SIGNAL_DISCONTINUITY_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);
            }

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Test signalDiscontinuity in READY state (after open) */
            VIDEODECODER_SIGNAL_DISCONTINUITY_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);

            /* Start the Decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            /* No test in STARTED, as it's the valid state */

            /* Stop the Decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Test signalDiscontinuity in READY state (after stop) */
            VIDEODECODER_SIGNAL_DISCONTINUITY_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);

            /* Close the Decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);

            /* Test signalDiscontinuity in CLOSED state (after close) */
            VIDEODECODER_SIGNAL_DISCONTINUITY_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
 * @brief Test the positive scenario of signaling EOS in VideoDecoderController
 *
 * This test verifies that the VideoDecoderController can successfully signal the end of stream (EOS) when it is in the STARTED state. It ensures that the controller transitions correctly and that the signalEOS function behaves as expected.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 030@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the Video Decoder is in the STARTED state by calling start() | controller = <controller_instance>, started = false | status.isOk() = true, started = true | Should be successful |
 * | 02 | Log before calling signalEOS | None | None | Should be successful |
 * | 03 | Call signalEOS on the Video Decoder Controller | controller = <controller_instance> | status.isOk() = true | Should Pass |
 * | 04 | Log the result of signalEOS | None | None | Should be successful |
 * | 05 | Verify the return status of signalEOS is successful | status.isOk() = true | status.isOk() = true | Should Pass |
 * | 06 | Stop the Video Decoder as teardown by calling stop() | controller = <controller_instance>, stopped = false | status.isOk() = true, stopped = true | Done by Pre-requisite TearDown function |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, signalEOS_pos)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing signalEOS for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Start the Decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            /* Signal EOS to the decoder */
            VIDEODECODER_SIGNAL_EOS_SUCCESS(videoDecoder, videoDecoderController, true);

            /* Stop the Decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Close the Decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
        }
    }
}



/**
 * @brief Test the behavior of VideoDecoderController when signalEOS is called in an invalid state.
 *
 * This test verifies that the VideoDecoderController correctly handles the signalEOS call when the decoder is not in the STARTED state. It ensures that the function returns an EX_ILLEGAL_STATE exception, indicating that the operation is not allowed in the current state.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 031@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve the current state of the Video Decoder to ensure it is not STARTED. | controller = instance, currentState = READY | currentState != STARTED | Should be successful |
 * | 02 | Call signalEOS() on the Video Decoder. | controller = instance | status.exceptionCode() = EX_ILLEGAL_STATE | Should Pass |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, signalEOS_neg)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            UT_LOG_INFO("Testing signalEOS for codec %d, decoder ID %d",
                        static_cast<int32_t>(_codec), _decoderId.value);

            sp<IVideoDecoderController> videoDecoderController;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

            /* Test signalEOS in CLOSED state (before open) */
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
            if (!videoDecoderController) {
                UT_LOG_INFO("Controller is null, expecting failure (CLOSED state)");
                //UT_EXPECT_TRUE(true) << "Test passes: flush not called on null controller";
            }else {
                VIDEODECODER_SIGNAL_EOS_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);
            }

            /* Open the Decoder */
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                      controllerListener, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Test signalEOS in READY state (after open) */
            VIDEODECODER_SIGNAL_EOS_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);

            /* Start the Decoder */
            VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

            /* No test in STARTED, as it's the valid state */

            /* Stop the Decoder */
            VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

            /* Test signalEOS in READY state (after stop) */
            VIDEODECODER_SIGNAL_EOS_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);

            /* Close the Decoder */
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
            VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);

            /* Test signalEOS in CLOSED state (after close) */
            VIDEODECODER_SIGNAL_EOS_FAILURE(videoDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}


/**
 * @brief Test the positive scenarios for parsing codec specific data in VideoDecoderController.
 *
 * This test verifies that the VideoDecoderController can correctly parse codec specific data for different video formats such as AVC, HEVC, and AV1. It ensures that the controller is able to start, process the codec data, and stop without errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 032@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the VideoDecoderController is not empty and available for testing. | controllers | Should be successful | |
 * | 02 | Start the VideoDecoderController to bring it to the STARTED state. | result = false | Should Pass | |
 * | 03 | Parse codec specific data for AVC_DECODER_CONFIGURATION_RECORD. | csdVideoFormat = AVC_DECODER_CONFIGURATION_RECORD, codecData = 0x01, _aidl_return = false | Should Pass | Should Pass |
 * | 04 | Parse codec specific data for HEVC_DECODER_CONFIGURATION_RECORD. | csdVideoFormat = HEVC_DECODER_CONFIGURATION_RECORD, codecData = 0x01, _aidl_return = false | Should Pass | Should Pass |
 * | 05 | Parse codec specific data for AV1_DECODER_CONFIGURATION_RECORD. | csdVideoFormat = AV1_DECODER_CONFIGURATION_RECORD, codecData = 0x01, _aidl_return = false | Should Pass | Should Pass |
 * | 06 | Stop the VideoDecoderController after tests. | result = false | Should Pass | |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, parseCodecSpecificData_pos)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    // Valid codec-specific data per codec
    std::map<videodecoder::Codec, std::pair<CSDVideoFormat, std::vector<uint8_t>>> codecDataMap = {
        {videodecoder::Codec::H264_AVC, {CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD,
                       {0x01, 0x4D, 0x40, 0x1E, 0xFF, 0xE1, 0x00, 0x04, 0x67, 0x4D, 0x40, 0x1E,
                        0x01, 0x00, 0x04, 0x68, 0xEE, 0x3C, 0x80}}}, // H.264 SPS/PPS
        {videodecoder::Codec::H265_HEVC, {CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD,
                       {0x01, 0x01, 0x60, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0xF0, 0x00, 0xFC, 0xFD}}}, // H.265 VPS/SPS/PPS
        {videodecoder::Codec::AV1, {CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD,
                      {0x81, 0x01, 0x01, 0x00}}} // AV1 config
    };

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            if (codecDataMap.find(_codec) != codecDataMap.end())
            {
                CSDVideoFormat csdVideoFormat = codecDataMap[_codec].first;
                std::vector<uint8_t> codecData = codecDataMap[_codec].second;

                UT_LOG_INFO("Testing parseCodecSpecificData for codec %d, decoder ID %d",
                            static_cast<int32_t>(_codec), _decoderId.value);

                sp<IVideoDecoderController> videoDecoderController;
                sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
                sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

                /* Open the Decoder */
                VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                          controllerListener, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

                /* Start the Decoder */
                VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

                /* Set CSD data to the decoder */
                VIDEODECODER_PARSE_CSD_SUCCESS(videoDecoder, videoDecoderController, csdVideoFormat,
                                               codecData, true);

                /* Stop the Decoder */
                VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

                /* Close the Decoder */
                VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
            }
        }
    }
}


/**
 * @brief Test the negative scenarios for parsing codec specific data in VideoDecoderController.
 *
 * This test evaluates the behavior of the VideoDecoderController when parsing codec specific data with invalid inputs or states. It ensures that the function handles these scenarios gracefully without causing unexpected behavior.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 033@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and iterate over each controller. | controllers = list of controllers | Should be successful | |
 * | 02 | Call parseCodecSpecificData with empty codec data. | csdVideoFormat = AVC_DECODER_CONFIGURATION_RECORD, codecData = empty, result = false | result = false | Should Fail |
 * | 03 | Simulate the decoder not being in the STARTED state by calling stop. | isStarted = false | status.isOk() = true | Should be successful |
 * | 04 | Call parseCodecSpecificData with valid HEVC data while decoder is not started. | csdVideoFormat = HEVC_DECODER_CONFIGURATION_RECORD, codecData = validHEVCData, result = false | result = false | Should Fail |
 * | 05 | Ensure the decoder is in the STARTED state by calling start. | isStarted = false | status.isOk() = true | Should be successful |
 * | 06 | Call parseCodecSpecificData with mismatched csdVideoFormat. | csdVideoFormat = invalid, codecData = validData, result = false | result = false | Should Fail |
 */
UT_ADD_TEST(VTS_L1_VideoDecoder, parseCodecSpecificData_neg)
{
    std::vector<IVideoDecoder::Id> decoderIds;
    sp<IVideoDecoder> videoDecoder;
    videodecoder::Capabilities capabilities;
    Status status;

    // Valid and invalid codec-specific data per codec
    std::map<videodecoder::Codec, std::pair<CSDVideoFormat, std::vector<uint8_t>>> validCodecDataMap = {
        {videodecoder::Codec::H264_AVC, {CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD,
                       {0x01, 0x4D, 0x40, 0x1E, 0xFF, 0xE1, 0x00, 0x04, 0x67, 0x4D, 0x40, 0x1E,
                        0x01, 0x00, 0x04, 0x68, 0xEE, 0x3C, 0x80}}}, // H.264
        {videodecoder::Codec::H265_HEVC, {CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD,
                       {0x01, 0x01, 0x60, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0xF0, 0x00, 0xFC, 0xFD}}}, // H.265
        {videodecoder::Codec::AV1, {CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD,
                      {0x81, 0x01, 0x01, 0x00}}} // AV1
    };
    std::vector<uint8_t> invalidCodecData1 = {0xFF, 0xFF}; // Invalid format
    std::vector<uint8_t> invalidCodecData2 = {}; // Empty

    /* Get the Video decoder IDs */
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder instances available.";

    for (const auto& _decoderId : decoderIds)
    {
        /* Get Video decoder handle */
        VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

        /* Get Capabilities of Video Decoder */
        VIDEODECODER_GETCAPABILITIES(videoDecoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            if (validCodecDataMap.find(_codec) != validCodecDataMap.end())
            {
                CSDVideoFormat csdVideoFormat = validCodecDataMap[_codec].first;
                std::vector<uint8_t> validCodecData = validCodecDataMap[_codec].second;

                UT_LOG_INFO("Testing parseCodecSpecificData for codec %d, decoder ID %d",
                            static_cast<int32_t>(_codec), _decoderId.value);

                sp<IVideoDecoderController> videoDecoderController;
                sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
                sp<IVideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

                /* Test in CLOSED state (before open) */
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);
                if (!videoDecoderController) {
                    UT_LOG_INFO("Controller is null, expecting failure (CLOSED state)");
                    //UT_EXPECT_TRUE(true) << "Test passes: flush not called on null controller";
                }else {
                    VIDEODECODER_PARSE_CSD_STATE_FAILURE(videoDecoderController, csdVideoFormat,
                        validCodecData, Status::EX_ILLEGAL_STATE);
                }

                /* Open the Decoder */
                VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities.supportsSecure,
                                          controllerListener, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

                /* Test in READY state (after open) */
                VIDEODECODER_PARSE_CSD_STATE_FAILURE(videoDecoderController, csdVideoFormat,
                                                     validCodecData, Status::EX_ILLEGAL_STATE);

                /* Start the Decoder */
                VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::STARTED);

                /* Test invalid codec data in STARTED */
                VIDEODECODER_PARSE_CSD_DATA_FAILURE(videoDecoderController, csdVideoFormat,
                                                    invalidCodecData1,Status::EX_ILLEGAL_ARGUMENT);
                VIDEODECODER_PARSE_CSD_DATA_FAILURE(videoDecoderController, csdVideoFormat,
                                                    invalidCodecData2,Status::EX_ILLEGAL_ARGUMENT);

                /* Stop the Decoder */
                VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::READY);

                /* Test in READY state (after stop) */
                VIDEODECODER_PARSE_CSD_STATE_FAILURE(videoDecoderController, csdVideoFormat,
                                                     validCodecData, Status::EX_ILLEGAL_STATE);

                /* Close the Decoder */
                VIDEODECODER_CLOSE_SUCCESS(videoDecoder, videoDecoderController, true);
                VIDEODECODER_CHECK_STATE(videoDecoder, ::com::rdk::hal::State::CLOSED);

                /* Test in CLOSED state (after close) */
                VIDEODECODER_PARSE_CSD_STATE_FAILURE(videoDecoderController, csdVideoFormat,
                                                     validCodecData, Status::EX_ILLEGAL_STATE);
            }
        }
    }
}



/** @} */ // End of Video_Decoder_Halif_Test_L1
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK
