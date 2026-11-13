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
 * @defgroup Video_Decoder_Halif_Test_L2 Video Decoder HALIF Tests L2 File
 * @{
 * @parblock
 *
 * ### L2 Tests for Video Decoder HALIF AIDL :
 *
 * Level 2 test cases for all APIs of Video Decoder HALIF AIDL Interface
 *
 * **Pre-Conditions:**  None@n
 * **Dependencies:** None@n
 *
 * Ref to API Definition specification documentation :
 *
 * @endparblock
 */


/**
 * @file vts_videodecoder_l2_tests.cpp
 *
 */

#include "vts_videodecoder.h"

using ::com::rdk::hal::videodecoder::State;

struct PropertyTestCase {
    ::com::rdk::hal::videodecoder::Property property;
    int32_t testValue;
    const char* propertyName;
};
std::vector<PropertyTestCase> testCases_readyState = {
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::AUTO), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::IP), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::TUNER), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::SYSTEM), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::HDMI_1), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::HDMI_2), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::HDMI_3), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::HDMI_4), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::HDMI_5), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::COMPOSITE_1), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::COMPOSITE_2), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::COMPOSITE_3), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::COMPOSITE_4), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::AV_SOURCE, static_cast<int32_t>(AVSource::COMPOSITE_5), "AV_SOURCE"},
    {::com::rdk::hal::videodecoder::Property::LOW_LATENCY_MODE , 1, "LOW_LATENCY_MODE"},
    {::com::rdk::hal::videodecoder::Property::LOW_LATENCY_MODE, 0, "LOW_LATENCY_MODE"},
    {::com::rdk::hal::videodecoder::Property::DECODE_ERROR_POLICY  , 1, "DECODE_ERROR_POLICY"},
    {::com::rdk::hal::videodecoder::Property::DECODE_ERROR_POLICY , 0, "DECODE_ERROR_POLICY"},
    {::com::rdk::hal::videodecoder::Property::SHA1_CALC   , 1, "SHA1_CALC"},
    {::com::rdk::hal::videodecoder::Property::SHA1_CALC  , 0, "SHA1_CALC"},
};
std::vector<PropertyTestCase> testCases_startedState = {
    {::com::rdk::hal::videodecoder::Property::SHA1_CALC   , 1, "SHA1_CALC"},
    {::com::rdk::hal::videodecoder::Property::SHA1_CALC  , 0, "SHA1_CALC"},
};

class VTS_L2_VideoDecoder : public UTCore {
    public:
        std::vector<std::tuple<sp<IVideoDecoder>, sp<VideoDecoderControllerListener>, sp<VideoDecoderEventListener>,videodecoder::Capabilities *>> mVideoDecoderList;
        /**
         * @brief Default constructor.
         * Relies on UTCore's default initialization; no additional setup required.
         */
        VTS_L2_VideoDecoder() = default;

        /**
         * @brief Virtual destructor.
         * Ensures proper cleanup; defaults to UTCore's implementation.
         */
        ~VTS_L2_VideoDecoder() override = default;

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
            std::vector<IVideoDecoder::Id> decoderIds;
        /* Get the Video Sink IDs */
            VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

            android::ProcessState::self()->startThreadPool();

            for(const auto& _decoderId: decoderIds)
            {
                sp<IVideoDecoder> videoDecoder;
                std::unique_ptr<videodecoder::Capabilities> capabilitiesOwner(new videodecoder::Capabilities());
                videodecoder::Capabilities *capabilities = capabilitiesOwner.get();
                sp<VideoDecoderEventListener> eventListener = new VideoDecoderEventListener();

                sp<VideoDecoderControllerListener> controlListener = new VideoDecoderControllerListener();

                /* Get Video Sink handle */
                VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

                /* Register events */
                VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

                VIDEODECODER_GETCAPABILITIES(videoDecoder,capabilities);

                eventListener->setDecoderID(_decoderId.value);
                controlListener->setDecoderID(_decoderId.value);

                mVideoDecoderList.push_back({videoDecoder, controlListener, eventListener, capabilitiesOwner.release()});
            }
            // Custom setup code here, e.g., initialize decoder mocks or profiles
        }

        void checkCompleteStateTransition(sp<IVideoDecoder> decoder, sp<VideoDecoderEventListener> eventListener, ::com::rdk::hal::videodecoder::State oldState, ::com::rdk::hal::videodecoder::State intermediateState, ::com::rdk::hal::videodecoder::State newState)
        {
            // Wait for the intermediate state
            std::this_thread::sleep_for(std::chrono::milliseconds(VIDEO_MAX_TRANSITORY_STATE_WAIT_MS));

            // Check if the transition from old to intermediate state occurred
            UT_ASSERT_TRUE(eventListener->checkStateTransistion(oldState, intermediateState));

            // Check if the transition from intermediate to new state occurred
            UT_ASSERT_TRUE(eventListener->checkStateTransistion(intermediateState, newState));
        }
        /**
         * @brief Cleans up the test environment after each test case.
         * Calls UTCore::TearDown() to ensure base class cleanup.
         * Add custom cleanup logic here if needed.
         */
        void TearDown() override {
            // Custom cleanup code here, e.g., release resources
            UTCore::TearDown();  // Call base class teardown (if it exists)
            for (const auto& [decoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
            {
             /* Un-register the events */
                VIDEODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
                delete capabilities;
            }
            mVideoDecoderList.clear();
        }
    };

/**
* @brief Test the ability to set and get properties on video decoders
*
* This test verifies that the video decoders are correctly instantiated and their properties can be set and retrieved. It ensures that the decoder is not null, checks the setting and getting of read-write properties, and validates the closing of the decoder.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 001@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_L2_Low-level_TestSpecification.md](../docs/pages/VideoDecoder_L2_Low-level_TestSpecification.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestGetAndSetProperties) {
    // Fetch decoder IDs from the manager
    for(const auto& [videoDecoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
    {
        for (const auto& _codecCaps : capabilities->supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;

            sp<IVideoDecoderController> controller;

            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities->supportsSecure,
                                      controlListener, controller, false);

            for (const auto& testCase : testCases_readyState)
            {
                PropertyValue setValue;
                setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(testCase.testValue);

                VIDEODECODER_SETPROPERTY_SUCCESS(controller,testCase.property,setValue);

                std::optional<PropertyValue> getPropertyValue;
                VIDEODECODER_GETPROPERTY(videoDecoder, testCase.property, getPropertyValue);
                UT_ASSERT_EQUAL(getPropertyValue->value->get<com::rdk::hal::PropertyValue::Value::Tag::intValue>(), testCase.testValue);
            }

            //Check and set and get properties for Started State
            VIDEODECODER_START_SUCCESS(videoDecoder,controller,true);
            for (const auto& testCase : testCases_startedState)
            {
                PropertyValue setValue;
                setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(testCase.testValue);

                VIDEODECODER_SETPROPERTY_SUCCESS(controller,testCase.property,setValue);

                std::optional<PropertyValue> getPropertyValue;
                VIDEODECODER_GETPROPERTY(videoDecoder, testCase.property, getPropertyValue);
                UT_ASSERT_EQUAL(getPropertyValue->value->get<com::rdk::hal::PropertyValue::Value::Tag::intValue>(), testCase.testValue);
            }
            // Stop decoder
            VIDEODECODER_STOP_SUCCESS(videoDecoder,controller,true);
            // Close the decoder
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, true);
        }
    }
}

/**
* @brief Test the ability to read and set properties on multi instances of video decoder
*
* This test verifies the functionality of retrieving and setting properties on video decoders. It ensures that the decoder IDs can be retrieved, decoders can be opened and closed, and properties can be set and retrieved correctly.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 002@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_L2_Low-level_TestSpecification.md](../docs/pages/VideoDecoder_L2_Low-level_TestSpecification.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestGetAndSetProperties_multipleInstance)
{
    std::vector<sp<IVideoDecoderController>> videoDecoderControllersList;

    for(const auto& [videoDecoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
    {
        videodecoder::Codec _codec = capabilities->supportedCodecs[0].codec;
        sp<IVideoDecoderController> controller;
            // Open to READY
        VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities->supportsSecure,
                                      controlListener, controller, false);
        videoDecoderControllersList.push_back(controller);

    }
    for(size_t i = 0; i < mVideoDecoderList.size(); ++i)
    {
        const auto& [videoDecoder,controlListener,eventListener,capabilities] = mVideoDecoderList[i];
        const auto& controller = videoDecoderControllersList[i];

        for (const auto& testCase : testCases_readyState)
        {
            PropertyValue setValue;
            setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(testCase.testValue);

            VIDEODECODER_SETPROPERTY_SUCCESS(controller,testCase.property,setValue);

            std::optional<PropertyValue> getPropertyValue;
            VIDEODECODER_GETPROPERTY(videoDecoder, testCase.property, getPropertyValue);
            UT_ASSERT_EQUAL(getPropertyValue->value->get<com::rdk::hal::PropertyValue::Value::Tag::intValue>(), testCase.testValue);
        }

        //Check and set and get properties for Started State
        VIDEODECODER_START_SUCCESS(videoDecoder,controller,true);
        for (const auto& testCase : testCases_startedState)
        {
            PropertyValue setValue;
            setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(testCase.testValue);

            VIDEODECODER_SETPROPERTY_SUCCESS(controller,testCase.property,setValue);

            std::optional<PropertyValue> getPropertyValue;
            VIDEODECODER_GETPROPERTY(videoDecoder, testCase.property, getPropertyValue);
            UT_ASSERT_EQUAL(getPropertyValue->value->get<com::rdk::hal::PropertyValue::Value::Tag::intValue>(), testCase.testValue);
        }
        // Stop decoder
        VIDEODECODER_STOP_SUCCESS(videoDecoder,controller,true);
    }

    for(size_t i = 0; i < mVideoDecoderList.size(); ++i)
    {
        const auto& [videoDecoder,controlListener,eventListener,capabilities] = mVideoDecoderList[i];
        const auto& controller = videoDecoderControllersList[i];
        // Close the decoder
        VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, true);
    }

}

/**
* @brief Test the state transitions of video decoders
*
* This test verifies the ability to open, transition states, and close video decoders using the provided codec and secure mode settings. It ensures that the decoder transitions through the expected states and returns the correct status.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 003@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_L2_Low-level_TestSpecification.md](../docs/pages/VideoDecoder_L2_Low-level_TestSpecification.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestDecoderStateTransitions)
{
    for(const auto& [videoDecoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
    {
        for (const auto& _codecCaps : capabilities->supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;

            sp<IVideoDecoderController> controller;
            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities->supportsSecure,
                                      controlListener, controller, false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::CLOSED,::com::rdk::hal::videodecoder::State::OPENING,::com::rdk::hal::videodecoder::State::READY);

            // Start to STARTED
            VIDEODECODER_START_SUCCESS(videoDecoder, controller, false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::READY,::com::rdk::hal::videodecoder::State::STARTING,::com::rdk::hal::videodecoder::State::STARTED);
            VIDEODECODER_FLUSH_SUCCESS(videoDecoder,controller,false,false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::STARTED,::com::rdk::hal::videodecoder::State::FLUSHING,::com::rdk::hal::videodecoder::State::STARTED);
            VIDEODECODER_FLUSH_SUCCESS(videoDecoder,controller,true,false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::STARTED,::com::rdk::hal::videodecoder::State::FLUSHING,::com::rdk::hal::videodecoder::State::READY);
            VIDEODECODER_START_SUCCESS(videoDecoder, controller, false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::READY,::com::rdk::hal::videodecoder::State::STARTING,::com::rdk::hal::videodecoder::State::STARTED);
            // Cleanup
            VIDEODECODER_STOP_SUCCESS(videoDecoder, controller, false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::STARTED,::com::rdk::hal::videodecoder::State::STOPPING,::com::rdk::hal::videodecoder::State::READY);
            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, false);
            checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::READY,::com::rdk::hal::videodecoder::State::CLOSING,::com::rdk::hal::videodecoder::State::CLOSED);
        }
    }
}


/**
* @brief Test the state transitions of multiple instances of video decoders
*
* This test verifies the ability to open, transition states, and close video decoders using the provided codec and secure mode settings. It ensures that the decoder transitions through the expected states and returns the correct status.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 004@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_L2_Low-level_TestSpecification.md](../docs/pages/VideoDecoder_L2_Low-level_TestSpecification.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestDecoderStateTransitions_multipleInstance)
{
    std::vector<sp<IVideoDecoderController>> videoDecoderControllersList;

    for(const auto& [videoDecoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
    {
        videodecoder::Codec _codec = capabilities->supportedCodecs[0].codec;
        sp<IVideoDecoderController> controller;
            // Open to READY
        VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities->supportsSecure,
                                      controlListener, controller, false);
        videoDecoderControllersList.push_back(controller);

    }
    for(size_t i = 0; i < mVideoDecoderList.size(); ++i)
    {
        const auto& [videoDecoder,controlListener,eventListener,capabilities] = mVideoDecoderList[i];
        const auto& controller = videoDecoderControllersList[i];
        VIDEODECODER_START_SUCCESS(videoDecoder, controller, false);
        checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::READY,::com::rdk::hal::videodecoder::State::STARTING,::com::rdk::hal::videodecoder::State::STARTED);
        VIDEODECODER_FLUSH_SUCCESS(videoDecoder,controller,false,false);
        checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::STARTED,::com::rdk::hal::videodecoder::State::FLUSHING,::com::rdk::hal::videodecoder::State::STARTED);
        VIDEODECODER_FLUSH_SUCCESS(videoDecoder,controller,true,false);
        checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::STARTED,::com::rdk::hal::videodecoder::State::FLUSHING,::com::rdk::hal::videodecoder::State::READY);
        VIDEODECODER_START_SUCCESS(videoDecoder, controller, false);
        checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::READY,::com::rdk::hal::videodecoder::State::STARTING,::com::rdk::hal::videodecoder::State::STARTED);
        // Cleanup
    }
    for(size_t i = 0; i < mVideoDecoderList.size(); ++i)
    {
        const auto& [videoDecoder,controlListener,eventListener,capabilities] = mVideoDecoderList[i];
        const auto& controller = videoDecoderControllersList[i];
        VIDEODECODER_STOP_SUCCESS(videoDecoder, controller, false);
        checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::STARTED,::com::rdk::hal::videodecoder::State::STOPPING,::com::rdk::hal::videodecoder::State::READY);
        VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, false);
        checkCompleteStateTransition(videoDecoder,eventListener,::com::rdk::hal::videodecoder::State::READY,::com::rdk::hal::videodecoder::State::CLOSING,::com::rdk::hal::videodecoder::State::CLOSED);
    }


}

UT_ADD_TEST(VTS_L2_VideoDecoder, TestSignalEndOfStreamStateContract)
{
    for (const auto& [videoDecoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
    {
        for (const auto& _codecCaps : capabilities->supportedCodecs)
        {
            videodecoder::Codec _codec = _codecCaps.codec;
            sp<IVideoDecoderController> controller;

            VIDEODECODER_OPEN_SUCCESS(videoDecoder, _codec, capabilities->supportsSecure,
                                      controlListener, controller, true);

            // READY state must reject EOS signaling.
            VIDEODECODER_SIGNAL_EOS_FAILURE(controller, Status::EX_ILLEGAL_STATE);

            VIDEODECODER_START_SUCCESS(videoDecoder, controller, true);

            // STARTED with no submitted buffers must reject EOS signaling.
            VIDEODECODER_SIGNAL_EOS_FAILURE(controller, Status::EX_ILLEGAL_STATE);

            // Ensure no EOS callback was emitted for rejected calls.
            UT_ASSERT_EQUAL(controlListener->getEndOfStreamCount(true), 0);

            VIDEODECODER_STOP_SUCCESS(videoDecoder, controller, true);
            VIDEODECODER_SIGNAL_EOS_FAILURE(controller, Status::EX_ILLEGAL_STATE);

            VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, true);
        }
    }
}

/** @} */ // End of Video_Decoder_Halif_Test_L2
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK
