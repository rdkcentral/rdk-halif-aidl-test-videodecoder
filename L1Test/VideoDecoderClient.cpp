
/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /**
* @file VideoDecoderClient.cpp
* @page VideoDecoder hal Level 1 Tests
*
* ## Module's Role
* This module includes Level 1 functional tests (success and failure scenarios).
* This is to ensure that the VideoDecoder hal APIs meet the requirements across all vendors.
*
*/

#include <gtest/gtest.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include "yaml_utils.h"

#include "com/rdk/hal/videodecoder/IVideoDecoderManager.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderControllerListener.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderEventListener.h"
#include "com/rdk/hal/videodecoder/DynamicRange.h"
#include "com/rdk/hal/videodecoder/CodecCapabilities.h"
#include "com/rdk/hal/videodecoder/OperationalMode.h"

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

using namespace com::rdk::hal;
using namespace com::rdk::hal::videodecoder;


sp<IVideoDecoderManager> service;
sp<IVideoDecoder> _videoDecoder;
std::vector<::com::rdk::hal::videodecoder::OperationalMode> supportedOperationalModes;
com::rdk::hal::videodecoder::IVideoDecoder::Id g_videodecoderId;

struct _CodecCapabilities
{
    ::com::rdk::hal::videodecoder::Codec codec;
    int32_t maxFrameRate;
    int32_t maxFrameWidth;
    int32_t maxFrameHeight;
};

struct decoder
{
    int id;
    ::std::vector<_CodecCapabilities> supportedCodecs;
    ::std::vector<::com::rdk::hal::videodecoder::DynamicRange> supportedDynamicRanges;   
    bool supportsSecure;
};


std::vector<decoder> decoders;

int getService()
{
    service = android::interface_cast<IVideoDecoderManager>(
            defaultServiceManager()->getService(String16(IVideoDecoderManager::serviceName().c_str())));
    if (service == nullptr)
    {
        printf("Service not available\n");
        return -1;
    }
    return 0;
}

class VideoDecoderControllerListener : public BnVideoDecoderControllerListener {
public:

    VideoDecoderControllerListener() {};
    virtual ~VideoDecoderControllerListener() = default;

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::videodecoder::FrameMetadata>& metadata) override
    {
        return ::android::binder::Status::ok();
    }
    Status onUserDataOutput(int64_t nsPresentationTime, const ::std::vector<uint8_t>& userData) override
    {
        return ::android::binder::Status::ok();
    }
};

class VideoDecoderEventListener : public BnVideoDecoderEventListener {
public:
    VideoDecoderEventListener() {};
    virtual ~VideoDecoderEventListener() = default;

    Status onDecodeError(::com::rdk::hal::videodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override {
        return ::android::binder::Status::ok();
    }
    Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) override {
        return ::android::binder::Status::ok();
    }
};

bool isValidState(::com::rdk::hal::State state) {
    switch(state) {
        case ::com::rdk::hal::State::UNKNOWN:
        case ::com::rdk::hal::State::CLOSED:
        case ::com::rdk::hal::State::OPENING:
        case ::com::rdk::hal::State::READY:
        case ::com::rdk::hal::State::STARTING:
        case ::com::rdk::hal::State::STARTED:
        case ::com::rdk::hal::State::FLUSHING:
        case ::com::rdk::hal::State::STOPPING:
        case ::com::rdk::hal::State::CLOSING:
            return true;
        default:
            return false;
    }
}

void validateCapabilities(const ::com::rdk::hal::videodecoder::Capabilities& capabilities) 
{
    /*
     "decoders" is a vector, which contains information of video decoders available in hal feature profile.
     The following are the information for each video decoder:
     - The decoder ID.
     - Whether the decoder supports secure video path (SVP) mode. 
     - A list of supported codecs along with their maximum frame rate, width, and height.
     - A list of supported dynamic ranges (e.g., HDR formats).
   */
    for(const auto& _decoder: decoders)
    {
        if(_decoder.id == g_videodecoderId.value)
        {
            /* number of supported codec from Hal Feature Profile should be equal to  number of supported codec from getCapabilities() api */
            ASSERT_EQ(_decoder.supportedCodecs.size(),capabilities.supportedCodecs.size());
            /* iterating over each supported codec from getCapabilities() api */
            for(const auto& supportedcodec_from_api: capabilities.supportedCodecs)
            {
                bool codec_present_flag = false;
                /* iterating over each "supported codec" from Hal Feature Profile */
                for(const auto& supportedcodec_from_profile: _decoder.supportedCodecs)
                {
                    /* if the codec match found, ensure that "maxFrameRate, maxFrameWidth and maxFrameHeight" from api and Hal Feature Profile are equal*/
                    if(supportedcodec_from_api.codec == supportedcodec_from_profile.codec)
                    {
                        codec_present_flag = true;
                        EXPECT_EQ(supportedcodec_from_api.maxFrameRate, supportedcodec_from_profile.maxFrameRate) <<" maxFrameRate of codec : "<<static_cast<int>(supportedcodec_from_api.codec) <<" is not matching with the profile"<<std::endl;
                        EXPECT_EQ(supportedcodec_from_api.maxFrameWidth, supportedcodec_from_profile.maxFrameWidth) <<" maxFrameWidth of codec : "<<static_cast<int>(supportedcodec_from_api.codec) <<" is not matching with the profile"<<std::endl;
                        EXPECT_EQ(supportedcodec_from_api.maxFrameHeight, supportedcodec_from_profile.maxFrameHeight) <<" maxFrameHeight of codec : "<<static_cast<int>(supportedcodec_from_api.codec) <<" is not matching with the profile"<<std::endl;
                    }
                }
                EXPECT_TRUE(codec_present_flag) <<" codec : "<<static_cast<int>(supportedcodec_from_api.codec) <<"not present in the profile" <<std::endl;
            }
            /* number of "supported dynamic ranges" from Hal Feature Profile should be equal to  number of supported codec from getCapabilities() api */
            ASSERT_EQ(_decoder.supportedDynamicRanges.size(), capabilities.supportedDynamicRanges.size());
            for(const auto& supportedDynamicRange_from_api: capabilities.supportedDynamicRanges)
            {
                /* ensure that "supported dynamic ranges" from  getCapabilities() api is present in the hal feature profile */
                auto result = std::find(_decoder.supportedDynamicRanges.begin(),_decoder.supportedDynamicRanges.end(), supportedDynamicRange_from_api);
                EXPECT_NE(result, _decoder.supportedDynamicRanges.end()) <<"supportedDynamicRange of decoder : "<<static_cast<int>(_decoder.id) <<" is not matching with the profile"<<std::endl;
            }

            /* ensure that supportsSecure value from  hal feature profile  is equal to the value from getCapabilities() api */
            EXPECT_EQ(_decoder.supportsSecure, capabilities.supportsSecure);
        }
    }

}

bool validatePropertyValue( const std::optional<::com::rdk::hal::PropertyValue>& propertyValue, const ::com::rdk::hal::videodecoder::Property property)
{
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        return false;
    }
    std::variant<int32_t, std::string> val;

    const auto& propVal = propertyValue->value.value();
    switch (propVal.getTag())
    {
        case PropertyValue::Value::Tag::intValue:
            val = propVal.get<PropertyValue::Value::Tag::intValue>();
            break;
        case PropertyValue::Value::Tag::byteValue:
            val = propVal.get<PropertyValue::Value::Tag::byteValue>();
            break;
        case PropertyValue::Value::Tag::stringValue:
        {
            android::String16 string16Val = propVal.get<PropertyValue::Value::Tag::stringValue>();
            android::String8 string8Val(string16Val);  // Convert String16 to String8
            val = std::string(string8Val.string());    // Convert String8 to std::string
        }
            break;
        case PropertyValue::Value::Tag::booleanValue:
            val = propVal.get<PropertyValue::Value::Tag::booleanValue>() ? 1 : 0;  // Store as int32_t (1 for true, 0 for false)
            break;

        default:
            std::cerr << "Unsupported type in PropertyValue::Value" << std::endl;
            break;
    }

    switch (property)
    {
        /*
        If the property is RESOURCE_ID, then check if the retrieved value (val) is equal to the the video decoder id from profile.
        */
        case ::com::rdk::hal::videodecoder::Property::RESOURCE_ID:
             if(std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) == g_videodecoderId.value)
             {
                return true;
             }
             else
             {
                std::cout<<"invalid RESOURCE_ID "<<std::endl;
                return false;
             }
        /* 
        If the property is OPERATIONAL_MODE, then check if the retrieved value (val) 
        matches any of the supported operational modes defined in the HAL feature profile.
        'supportedOperationalModes' is a vector that contains the valid operational modes 
        from the HAL feature profile.
        */
        case ::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE:
            if( std::holds_alternative<int32_t>(val))
            {
                int _val = std::get<int32_t>(val);
                if(std::find(supportedOperationalModes.begin(), supportedOperationalModes.end(),
                static_cast<::com::rdk::hal::videodecoder::OperationalMode>(_val)) != supportedOperationalModes.end())
                {
                    return true;
                }
                else
                {
                    std::cout<<"invalid OPERATIONAL_MODE "<<std::endl;
                    return false;
                }
            }
            else
            {
                std::cout<<"invalid OPERATIONAL_MODE "<<std::endl;
                return false;
            }
            

        case ::com::rdk::hal::videodecoder::Property::AV_SOURCE:
            if(std::holds_alternative<int32_t>(val) &&
            (std::get<int32_t>(val) == static_cast<int32_t>(::com::rdk::hal::AVSource::AUTO) ||
            std::get<int32_t>(val) == static_cast<int32_t>(::com::rdk::hal::AVSource::UNKNOWN) ||
            (std::get<int32_t>(val) >= static_cast<int32_t>(::com::rdk::hal::AVSource::IP) &&
                std::get<int32_t>(val) <= static_cast<int32_t>(::com::rdk::hal::AVSource::SYSTEM)) ||
            (std::get<int32_t>(val) >= static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_1) &&
                std::get<int32_t>(val) <= static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_5)) ||
            (std::get<int32_t>(val) >= static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_1) &&
                std::get<int32_t>(val) <= static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_5))))
                {
                    return true;
                }
                else
                {
                    std::cout<<"invalid AV_SOURCE "<<std::endl;
                    return false;
                }

        case ::com::rdk::hal::videodecoder::Property::LOW_LATENCY_MODE:
            if(std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1))
                   {
                    return true;
                   }
                   else
                   {
                       std::cout<<"invalid LOW_LATENCY_MODE "<<std::endl;
                       return false;
                   }

        case ::com::rdk::hal::videodecoder::Property::DECODE_ERROR_POLICY:
            if(std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid DECODE_ERROR_POLICY "<<std::endl;                        
                       return false;
                   }

        case ::com::rdk::hal::videodecoder::Property::SHA1_CALC:
            if(std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid SHA1_CALC "<<std::endl;                        
                       return false;                    
                   }

        case ::com::rdk::hal::videodecoder::Property::SECURE_VIDEO:
            if(std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid SECURE_VIDEO "<<std::endl;                        
                       return false;                     
                   }

        case ::com::rdk::hal::videodecoder::Property::INPUT_QUEUE_DEPTH:
            if(std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) > 0))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid SECURE_VIDEO "<<std::endl;                        
                       return false;                     
                   }

        case ::com::rdk::hal::videodecoder::Property::OUTPUT_FRAME_POOL_SIZE:
            if(std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) > 0))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid SECURE_VIDEO "<<std::endl;                        
                       return false;                     
                   }

        case ::com::rdk::hal::videodecoder::Property::METRIC_FRAMES_DECODED:
            if(std::holds_alternative<int32_t>(val))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid METRIC_FRAMES_DECODED "<<std::endl;                        
                       return false;                     
                   }

        case ::com::rdk::hal::videodecoder::Property::METRIC_DECODE_ERRORS:
            if(std::holds_alternative<int32_t>(val))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid METRIC_DECODE_ERRORS "<<std::endl;                        
                       return false;                     
                   }

        case ::com::rdk::hal::videodecoder::Property::METRIC_FRAMES_DROPPED:
            if(std::holds_alternative<int32_t>(val))
                   {
                       return true;
                   }
                   else
                   {
                       std::cout<<"invalid METRIC_FRAMES_DROPPED "<<std::endl;                        
                       return false;                     
                   }

        default:
            return false;
    }
}

/*currently HAL FEATURE PROFILE contain string value for supportedOperationalModes instead of enum. so the helper required to convert*/
com::rdk::hal::videodecoder::OperationalMode StringTooperationalmode(const std::string& format) 
{
    static const std::unordered_map<std::string, com::rdk::hal::videodecoder::OperationalMode> formatMap = {
        {"TUNNELLED", com::rdk::hal::videodecoder::OperationalMode::TUNNELLED},
        {"NON_TUNNELLED", com::rdk::hal::videodecoder::OperationalMode::NON_TUNNELLED},
        {"GRAPHICS_TEXTURE", com::rdk::hal::videodecoder::OperationalMode::GRAPHICS_TEXTURE}
    };

    auto it = formatMap.find(format);
    if (it != formatMap.end()) 
    {
        return it->second;
    }
    return static_cast<com::rdk::hal::videodecoder::OperationalMode>(-1);
}

/*currently HAL FEATURE PROFILE contain string value for codec instead of enum. so the helper required to convert*/
com::rdk::hal::videodecoder::Codec StringTocodec(const std::string& format) 
{
    static const std::unordered_map<std::string, com::rdk::hal::videodecoder::Codec> formatMap = {
        {"MPEG", com::rdk::hal::videodecoder::Codec::MPEG},
        {"H264", com::rdk::hal::videodecoder::Codec::H264},
        {"H265", com::rdk::hal::videodecoder::Codec::H265},
        {"VP9", com::rdk::hal::videodecoder::Codec::VP9},
        {"AV1", com::rdk::hal::videodecoder::Codec::AV1}
    };

    auto it = formatMap.find(format);
    if (it != formatMap.end()) 
    {
        return it->second;
    }
    return static_cast<com::rdk::hal::videodecoder::Codec>(-1);
}

/*currently HAL FEATURE PROFILE contain string value for supportedDynamicRanges instead of enum. so the helper required to convert*/
com::rdk::hal::videodecoder::DynamicRange StringToDynamicrange(const std::string& format) 
{
    static const std::unordered_map<std::string, com::rdk::hal::videodecoder::DynamicRange> formatMap = {
        {"UNKNOWN", com::rdk::hal::videodecoder::DynamicRange::UNKNOWN},
        {"SDR", com::rdk::hal::videodecoder::DynamicRange::SDR},
        {"HLG", com::rdk::hal::videodecoder::DynamicRange::HLG},
        {"HDR10", com::rdk::hal::videodecoder::DynamicRange::HDR10},
        {"HDR10PLUS", com::rdk::hal::videodecoder::DynamicRange::HDR10PLUS},
        {"DOLBY_VISION", com::rdk::hal::videodecoder::DynamicRange::DOLBY_VISION}        
    };

    auto it = formatMap.find(format);
    if (it != formatMap.end()) 
    {
        return it->second;
    }
    return static_cast<com::rdk::hal::videodecoder::DynamicRange>(-1);
}

class L1VideoDecoderOpenTest : public virtual ::testing::Test
{
protected:
    void SetUp() override
    {
        //prerequisite - checking state
        ::com::rdk::hal::State state;
        ::android::binder::Status status = _videoDecoder->getState(&state);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

        // prerequisite - checking capabilities
        status = _videoDecoder->getCapabilities(&capabilities);
        EXPECT_TRUE(status.isOk());
        validateCapabilities(capabilities);

        sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();
        status = _videoDecoder->open(capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, &videoDecoderController);
        ASSERT_TRUE(status.isOk()) << "Failed to open audio decoder, status: " << status.toString8().string();
        ASSERT_NE(videoDecoderController, nullptr) << "audioDecoderController is null after open()";

        status = _videoDecoder->getState(&state);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(state, ::com::rdk::hal::State::READY);
    }
    void TearDown() override
    {
        ::com::rdk::hal::State state;
        Status status = _videoDecoder->getState(&state);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(state, ::com::rdk::hal::State::READY);
        bool _aidl_return;
        status = _videoDecoder->close(videoDecoderController, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
    android::sp<::com::rdk::hal::videodecoder::IVideoDecoderController> videoDecoderController;
    ::com::rdk::hal::videodecoder::Capabilities capabilities;
};

class L1VideoDecoderStartTest : public L1VideoDecoderOpenTest
{
protected:
    void SetUp() override
    {
        L1VideoDecoderOpenTest::SetUp();
        ::android::binder::Status status = videoDecoderController->start();
        ASSERT_TRUE(status.isOk());

        ::com::rdk::hal::State state;
        status = _videoDecoder->getState(&state);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(state, ::com::rdk::hal::State::STARTED);

    }
    void TearDown() override
    {
        ::com::rdk::hal::State state;
        Status status = _videoDecoder->getState(&state);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(state, ::com::rdk::hal::State::STARTED);
        status = videoDecoderController->stop();
        ASSERT_TRUE(status.isOk());
        L1VideoDecoderOpenTest::TearDown();
    }
};

class L1VideoDecoderCloseTest : public L1VideoDecoderOpenTest
{
protected:
    void SetUp() override
    {
        L1VideoDecoderOpenTest::SetUp();
    }
};

class L1VideoDecoderEventRegisterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bool _aidl_return;
        listener = new VideoDecoderEventListener();
        ::android::binder::Status status = _videoDecoder->registerEventListener(listener, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
    android::sp<com::rdk::hal::videodecoder::IVideoDecoderEventListener> listener;
};

class L1VideoDecoderStopTest : public L1VideoDecoderOpenTest
{
protected:
    void SetUp() override
    {
        L1VideoDecoderOpenTest::SetUp();
        ::android::binder::Status status = videoDecoderController->start();
        ASSERT_TRUE(status.isOk());

        ::com::rdk::hal::State state;
        status = _videoDecoder->getState(&state);
        ASSERT_TRUE(status.isOk());
        ASSERT_EQ(state, ::com::rdk::hal::State::STARTED);
    }
    void TearDown() override
    {
        L1VideoDecoderOpenTest::TearDown();
    }
};


bool areElementsUnique(const std::vector<::com::rdk::hal::videodecoder::IVideoDecoder::Id>& decoderIds) {
    std::unordered_set<int> uniqueElements;
    
    for (const auto& _decoderId : decoderIds) 
    {
        if(_decoderId.value < 0)
        {
            std::cout <<"find Invalid decoder ID : " <<_decoderId.value <<std::endl;
            return false;
        } 
        if (uniqueElements.find(_decoderId.value) != uniqueElements.end()) 
        {
            std::cout<<"decoder ID is not unique : " <<_decoderId.value <<std::endl;
            return false;
        }
        uniqueElements.insert(_decoderId.value);
    }
    return true;
}

bool are_operational_modes_Unique(const std::vector<::com::rdk::hal::videodecoder::OperationalMode>& operationalModes) {
    std::unordered_set<::com::rdk::hal::videodecoder::OperationalMode> uniqueElements;
    
    for (const auto& _mode : operationalModes) 
    {

        if (uniqueElements.find(_mode) != uniqueElements.end()) 
        {
            std::cout<<"Duplicate operationalMode : " <<static_cast<int>(_mode) <<std::endl;
            return false;
        }
        uniqueElements.insert(_mode);
    }
    return true;
}

/**
* @brief Test to validate the retrieval of video decoder IDs from the service
*
* This test ensures that the getVideoDecoderIds API correctly retrieves the video decoder IDs from the service and matches them with the IDs listed in the HAL FEATURE PROFILE.
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
* | 01| Initialize local variables to hold video decoder IDs from the service and the profile | decoderIds, decoderId_from_profile | Should be successful |  |
* | 02| Invoke getVideoDecoderIds to fetch decoder IDs from the service | decoderIds | Status isOk() | Should Pass |
* | 03| Ensure decoderIds are unique and greater than or equal to zero | decoderIds | decoderIds should be unique | Should Pass |
* | 04| Check if the number of decoder IDs matches  the number of decoder IDs  from profile | decoderIds.size(), decoders.size() | Sizes are equal | Should Pass |
* | 05| Populate decoderId_from_profile with IDs from the profile | decoders | decoderId_from_profile filled | Should be successful |
* | 06| Verify that each ID from getVideoDecoderIds is listed in the profile | decoderIds, decoderId_from_profile | All IDs found in profile | Should Pass |
*/
TEST(L1VideoDecoderManagerTest, test_l1_videodecoder_positive_1_getVideoDecoderIds)
{
    int i=0;
    std::vector<::com::rdk::hal::videodecoder::IVideoDecoder::Id> decoderIds;
    std::vector<int> decoderId_from_profile;

    ::android::binder::Status status = service->getVideoDecoderIds(&decoderIds);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(areElementsUnique(decoderIds));
    /* checking the number of video decoder ids from the api getVideoDecoderIds() equal to the number of video decoder ids from HAL FEATURE PROFILE  */
    ASSERT_EQ(decoderIds.size(),decoders.size());

    for(const auto& _decoder: decoders)
    {
        decoderId_from_profile.push_back(_decoder.id);
    }
    /* checking whether retrieved video decoder ids from the api getVideoDecoderIds() is listed in the HAL FEATURE PROFILE */
    for(const auto& _decoderId: decoderIds)
    {   
        auto result = std::find(decoderId_from_profile.begin(), decoderId_from_profile.end(), _decoderId.value);
        EXPECT_NE(result, decoderId_from_profile.end());
    }
}

/**
* @brief Test to verify that the getSupportedOperationalModes API correctly retrieves the supported operational modes for the video decoder.
*
* This test checks whether the getSupportedOperationalModes API correctly retrieves the list of supported operational modes from the video decoder service. The test verifies that the retrieved list matches the expected list obtained from the HAL feature profile (HFP).
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
* | 01 | Initialize an empty list to store operational modes | operationalModes = valid vector of type OperationalMode | Initialization Should be successful | |
* | 02 | Call the getSupportedOperationalModes API to retrieve the list of supported operational modes | operationalModes = valid vector of type OperationalMode  | vector `operationalModes` should contain supportedOperationalModes | Should Pass |
* | 03 | Verify that the API call was successful by checking the status | status.isOk() | status.isOk() should be true | Should Pass |
* | 04 | Check that the size of the retrieved list matches the size of the expected list from HAL feature profile | operationalModes.size(), supportedOperationalModes.size() | operationalModes.size() should be equal to supportedOperationalModes.size() | Should Pass |
* | 05 | Iterate through each mode in the retrieved list and verify it is present in the expected list from HAL feature profile | operationalModes = vector containing supported operational modes from api| each operational mode should be listed in HFP | Should Pass |
* | 06 | If a mode is not found in the expected list, mark the test as failed | None | Should Fail | |
*/
TEST(L1VideoDecoderManagerTest, test_l1_videodecoder_positive_1_getSupportedOperationalModes)
{
    std::vector<::com::rdk::hal::videodecoder::OperationalMode> operationalModes;
    ::android::binder::Status status = service->getSupportedOperationalModes(&operationalModes);
    ASSERT_TRUE(status.isOk());
    /* checking the number of supportedOperationalModes from the api getSupportedOperationalModes() equal to the number of supportedOperationalModes from HAL FEATURE PROFILE  */
    ASSERT_EQ(operationalModes.size(), supportedOperationalModes.size());
    ASSERT_TRUE(are_operational_modes_Unique(operationalModes));
    /* checking whether retrieved supportedOperationalModes from the api getSupportedOperationalModes() is listed in the HAL FEATURE PROFILE */
    for(const auto& _mode: operationalModes)
    {
        auto result = std::find(supportedOperationalModes.begin(), supportedOperationalModes.end(), _mode);
        EXPECT_NE(result,supportedOperationalModes.end());
    }
}

/**
* @brief Tests the functionality of the getVideoDecoder function
*
* This test aims to check the functionality of fetching video decoder instances correctly using the getVideoDecoder method@n
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize video decoder ID and video decoder pointer | videoDecoderId = valid, videoDecoder = valid pointer | Initialization Should be successful | |
* | 02 | Start loop through available decoders from the profile and assign ID to videoDecoderId | videoDecoderId.value = _decoder.id | Should be successful | Should be successful |
* | 03 | Call the getVideoDecoder method with videoDecoderId and videoDecoder pointer and check the status | videoDecoderId, videoDecoder | status.isOk() = true | Should Pass |
* | 04 | Check if the video decoder instance is not null | videoDecoder instance | videoDecoder instance shouldn't be null  | Should Pass |
*/
TEST(L1VideoDecoderManagerTest, test_l1_videodecoder_positive_1_getVideoDecoder)
{
    com::rdk::hal::videodecoder::IVideoDecoder::Id videoDecoderId;
    android::sp<com::rdk::hal::videodecoder::IVideoDecoder> videoDecoder;
    for(const auto& _decoder: decoders)
    {
        videoDecoderId.value = _decoder.id;
        android::binder::Status status = service->getVideoDecoder(videoDecoderId, &videoDecoder);
        EXPECT_TRUE(status.isOk());
        EXPECT_TRUE(videoDecoder != nullptr);
    }
}

/**
* @brief Test for verifying the positive scenario of retrieving a video decoder.
*
* This test validates the retrieval of a video decoder instance by calling the getVideoDecoder method with a specific video decoder ID. It ensures that the status of the operation is OK and that the retrieved video decoder is not null. This is to verify the proper functioning of the video decoder retrieval mechanism.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize video decoder ID and video decoder pointer | videoDecoderId = valid, videoDecoder = valid pointer | Initialization Should be successful | |
* | 02 | Start loop through available decoders from the profile and assign ID to videoDecoderId | videoDecoderId.value = _decoder.id | Should be successful | Should be successful |
* | 03 | Call the getVideoDecoder method with videoDecoderId and videoDecoder pointer and check the status | videoDecoderId, videoDecoder | status.isOk() = true | Should Pass |
* | 04 | Check if the video decoder instance is not null | videoDecoder instance | videoDecoder instance shouldn't be null  | Should Pass |
* | 05 | Call getProperty() and store resourceID | status.isok() | status.isok() should be true  | Should Pass |
* | 06 | Call the getVideoDecoder method again with same videoDecoderId and videoDecoder pointer and check the status | videoDecoderId, videoDecoder | status.isOk() = true | Should Pass |
* | 07 | Check if the video decoder instance is not null | videoDecoder instance | videoDecoder instance shouldn't be null  | Should Pass |
* | 08 | Call getProperty() and store resourceID | status.isok() | status.isok() should be true  | Should Pass |
* | 09 | verify that both resourceIDs are same | _resource_id_1, _resource_id_2 | _resource_id_1 should be equal to _resource_id_2  | Should Pass |
*/
TEST(L1VideoDecoderManagerTest, test_l1_videodecoder_positive_2_getVideoDecoder)
{
    com::rdk::hal::videodecoder::IVideoDecoder::Id videoDecoderId;
    android::sp<com::rdk::hal::videodecoder::IVideoDecoder> videoDecoder;
    ::com::rdk::hal::videodecoder::Property property = ::com::rdk::hal::videodecoder::Property::RESOURCE_ID;
    std::optional<::com::rdk::hal::PropertyValue> resource_id_1;
    std::optional<::com::rdk::hal::PropertyValue> resource_id_2;

    for(const auto& _decoder: decoders)
    {
        videoDecoderId.value = _decoder.id;
        android::binder::Status status = service->getVideoDecoder(videoDecoderId, &videoDecoder);
        EXPECT_TRUE(status.isOk());
        EXPECT_TRUE(videoDecoder != nullptr);

        status = videoDecoder->getProperty(property, &resource_id_1);
        EXPECT_TRUE(status.isOk());

        status = service->getVideoDecoder(videoDecoderId, &videoDecoder);
        EXPECT_TRUE(status.isOk());
        EXPECT_TRUE(videoDecoder != nullptr);

        status = videoDecoder->getProperty(property, &resource_id_2);
        EXPECT_TRUE(status.isOk());

        int32_t _resource_id_1 = resource_id_1->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
        int32_t _resource_id_2 = resource_id_2->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();

        /* validating whether multiple invocation gives same videoDecoder interface instance by checking RESOURCE_ID */
        EXPECT_EQ(_resource_id_1, _resource_id_2)<<"GOT DIFFERENT INSTANCE OF VIDEODECODER DURING MULTIPLE INVOCATION. ID for first invocation : "<<_resource_id_1 <<" ID for second invocation : "<<_resource_id_2 <<std::endl;
    }
}

/**
* @brief Test for verifying the positive scenario of retrieving a video decoder.
*
* This test validates the retrieval of a video decoder instance by calling the getVideoDecoder method with  video decoder ID as UNDEFINED. It ensures that the status of the operation is OK and that the retrieved video decoder is not null. This is to verify the proper functioning of the video decoder retrieval mechanism.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize the video decoder ID with UNDEFINED value. | videoDecoderId.value = UNDEFINED | Initialization Should be successful | |
* | 02 | Declare a smart pointer for the video decoder instance. | videoDecoder | Should be successful | |
* | 03 | Call the getVideoDecoder method with the videoDecoderId and videoDecoder reference. | videoDecoderId = UNDEFINED, videoDecoder = valid pointer | status.isOk() should be true, videoDecoder should not be null | Should Pass |
* | 04 | Assert that the status returned is OK. | status.isok() | status.isOk() = true | Should be successful |
* | 05 | Assert that the video decoder instance is not null. | videoDecoder instance | videoDecoder != nullptr | Should be successful |
*/
TEST(L1VideoDecoderManagerTest, test_l1_videodecoder_positive_3_getVideoDecoder)
{
    com::rdk::hal::videodecoder::IVideoDecoder::Id videoDecoderId;
    videoDecoderId.value = com::rdk::hal::videodecoder::IVideoDecoder::Id::UNDEFINED;
    android::sp<com::rdk::hal::videodecoder::IVideoDecoder> videoDecoder;
    android::binder::Status status = service->getVideoDecoder(videoDecoderId, &videoDecoder);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(videoDecoder != nullptr);
}

/**
* @brief Test negative scenario for getting video decoder with invalid ID
*
* This test verifies that the getVideoDecoder method fails when provided with an invalid ID that is less than the minimum valid ID@n
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
* | 01 | Set invalid video decoder ID to one less than minimum valid value | videoDecoderId.value = -2 | videoDecoderId should be set to -2 | Should be successful |
* | 02 | Declare a sp<IVideoDecoder> object | android::sp<com::rdk::hal::videodecoder::IVideoDecoder> videoDecoder | videoDecoder should be valid pointer | Should be successful |
* | 03 | Call getVideoDecoder with invalid ID | videoDecoderId = -2, &videoDecoder = valid pointer | status.isOk() should be false | Should Fail |
* | 04 | Verify videoDecoder is nullptr | videoDecoder instance | videoDecoder should be nullptr | Should be successful |
*/
TEST(L1VideoDecoderManagerTest, test_l1_videodecoder_negative_1_getVideoDecoder)
{
    com::rdk::hal::videodecoder::IVideoDecoder::Id videoDecoderId;
    videoDecoderId.value = -2; // One less than minimum valid ID
    android::sp<com::rdk::hal::videodecoder::IVideoDecoder> videoDecoder;
    android::binder::Status status = service->getVideoDecoder(videoDecoderId, &videoDecoder);
    EXPECT_FALSE(status.isOk());
    EXPECT_TRUE(videoDecoder == nullptr);
}

/**
* @brief Test the positive scenario of getting video decoder capabilities.
*
* This test verifies that the function getCapabilities retrieves the capabilities of the video decoder as expected. The result is checked to ensure the call succeeds and the output meets the required conditions.
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
* | 01 | Initialize capabilities object and call getCapabilities method | capabilities = valid pointer | status.isOk() should be true, capabilities should have valid data | Should Pass |
* | 02 | Validate retrieved capabilities | capabilities = retrieved capabilities | capabilities having correct values | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_1_getCapabilities) 
{
    ::com::rdk::hal::videodecoder::Capabilities capabilities;
    ::android::binder::Status status = _videoDecoder->getCapabilities(&capabilities);
    ASSERT_TRUE(status.isOk());
    validateCapabilities(capabilities);
}

/**
* @brief Tests the getProperty method of the video decoder in a positive scenario.
*
* This test verifies that the getProperty method functions correctly . The objective is to ensure that each requested property is retrieved successfully and validated.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Prepare the list of properties to be tested | properties = {RESOURCE_ID, INPUT_QUEUE_DEPTH, OUTPUT_FRAME_POOL_SIZE, OPERATIONAL_MODE, LOW_LATENCY_MODE, DECODE_ERROR_POLICY, AV_SOURCE, SHA1_CALC, SECURE_VIDEO, METRIC_FRAMES_DECODED, METRIC_DECODE_ERRORS, METRIC_FRAMES_DROPPED} | Properties list is prepared | Should be successful |
* | 02 | Loop through each property in the properties list | prop = current property from properties list | Loop started | Should be successful |
* | 03 | Call the getProperty method with the current property | prop, &aidl_return | aidl_return should contain valid property value | Should Pass |
* | 04 | Check if the status of getProperty call is OK | status.isOk() | status.isOk() should be true | Should be successful |
* | 05 | Validate the returned property value | aidl_return, prop | Property value is valid | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_1_getProperty)
{
    // calling getProperty() from state READY
    std::vector<Property> properties = {
            Property::RESOURCE_ID,
            Property::INPUT_QUEUE_DEPTH,
            Property::OUTPUT_FRAME_POOL_SIZE,
            Property::OPERATIONAL_MODE,
            Property::LOW_LATENCY_MODE,
            Property::DECODE_ERROR_POLICY,
            Property::AV_SOURCE,
            Property::SHA1_CALC,
            Property::SECURE_VIDEO,
            Property::METRIC_FRAMES_DECODED,
            Property::METRIC_DECODE_ERRORS,                        
            Property::METRIC_FRAMES_DROPPED
    };
    for (Property prop : properties)
    {
        std::optional<PropertyValue> aidl_return;
        Status status = _videoDecoder->getProperty(prop, &aidl_return);
        ASSERT_TRUE(status.isOk());
        EXPECT_TRUE(validatePropertyValue(aidl_return,prop));
    }
}
/**
* @brief This test verifies the negative scenario when an invalid property is requested from the video decoder.
*
* The test specifically attempts to retrieve a property using an invalid property ID (-2) and expects the API to return a failure status. The purpose is to ensure that the API gracefully handles invalid inputs and does not succeed erroneously.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Set a variable of type `std::optional<PropertyValue>` named `aidl_return` for holding propertyvalue | aidl_return | Initialization Should be successful | |
* | 02 | Call `getProperty` with arguments `static_cast<Property>(-2)` and address of `aidl_return` | Property ID = -2, aidl_return = valid | Status should not be OK | Should Fail |
* | 03 | Assert that the `status` returned is not OK |status.isOk() | status.isOk() should be false | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_1_getProperty)
{
    std::optional<PropertyValue> aidl_return;
    Status status = _videoDecoder->getProperty(static_cast<Property>(-2), &aidl_return);
    ASSERT_FALSE(status.isOk());
}

/**
* @brief Test case for L1 Video Decoder's negative scenario when setting a property
*
* This test case checks the behavior of the L1 Video Decoder when attempting to set a 
* property to an invalid negative value. The objective is to validate the system's error 
* handling in this scenario.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 0010@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize Property to RESOURCE_ID | property = Property::RESOURCE_ID | Initialization Should be successful | |
* | 02 | Initialize PropertyValue with a negative int value | propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-2) |Initialization Should be successful | |
* | 03 | Set the property on the video decoder with the invalid value | property = RESOURCE_ID, propertyValue = -2 | Status.isok() = true, _aidl_return = false | Should Fail |
* | 04 | Assert that the status is  OK | status.isOk() | status.isOk() should be true | Should pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_1_setProperty)
{
    Property property = Property::RESOURCE_ID;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-2);
    bool _aidl_return;
    Status status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    EXPECT_FALSE(_aidl_return);
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for setting an invalid property on the video decoder
*
* This test aims to verify if the video decoder correctly handles the attempt to set an invalid property.
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
* | Variation / Step | Description | Test Data |Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Set up property with invalid value | property = 9999 | Invalid property set up should be successful | |
* | 02| Set property value to 1  | propertyValue.value = 1 | PropertyValue set successfully | Should be successful |
* | 03| Call setProperty API with invalid property | property = 9999, propertyValue.value = 1, _aidl_return | Status.isok() = true, _aidl_return should be false | Should Fail |
* | 04| Validate that status is  OK | status.isOk() | status.isOk() should be true | Should pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_2_setProperty)
{
    Property property = static_cast<Property>(9999);
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    bool _aidl_return;
    Status status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    EXPECT_FALSE(_aidl_return);
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Negative test for setting a property with an invalid value on the video decoder
*
* This test verifies the behavior of the setProperty function when an invalid propertyvalue is supplied.@n
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Create a Property object and initialize it to OUTPUT_FRAME_POOL_SIZE | property = ::com::rdk::hal::videodecoder::Property::OUTPUT_FRAME_POOL_SIZE | Properly initialized Property object | Should be successful |
* | 02| Create a PropertyValue object and set its intValue to -10 | propertyValue.value = -10 | PropertyValue object with invalid intValue | Should be successful |
* | 03| Call the setProperty function  with the Property and PropertyValue objects | property = OUTPUT_FRAME_POOL_SIZE, propertyValue.intValue = -10, _aidl_return | setProperty function should fail | Should fail |
* | 04| Check the status returned by setProperty function using ASSERT_TRUE | status.isOk() | status.isOk() should be true | Should Pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_3_setProperty)
{
    ::com::rdk::hal::videodecoder::Property property = ::com::rdk::hal::videodecoder::Property::OUTPUT_FRAME_POOL_SIZE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-10);
    bool _aidl_return;

    Status status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    EXPECT_FALSE(_aidl_return);
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Tests the negative case of setting a property for the video decoder.
*
* This test case checks the behavior of the setProperty function when setting the LOW_LATENCY_MODE property to an invalid state.
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
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Set LOW_LATENCY_MODE property to 1 by Invoking  setProperty() | property = Property::LOW_LATENCY_MODE, propertyValue = 1 | Status should be not OK, _aidl_return should be false | Should Fail |
* | 06 | Check the video decoder state is STARTED and then stop it | None | State should be READY, status should be OK | Done by Pre-requisite TearDown function |
* | 07 | Check the video decoder state is READY and close it | None | State should be CLOSED, _aidl_return should be true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_4_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    bool _aidl_return;

    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    Status status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    EXPECT_FALSE(_aidl_return);
    EXPECT_FALSE(status.isOk());
}

/**
* @brief Test for verifying the property setting functionality of the video decoder
*
* This test verifies that the video decoder can successfully set the property LOW_LATENCY_MODE in STATE_READY.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02| Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03| Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04|  retrieving initial property value | property = Property::LOW_LATENCY_MODE | status.isOk() == true, initialvalue should contain valid data | Should Pass |
* | 05| Setting LOW_LATENCY_MODE property value to 1 | property = Property::LOW_LATENCY_MODE, propertyValue = 1 | status.isOk() == true, _aidl_return == true | Should Pass |
* | 06| Reverting the property value to the initial state | property = Property::LOW_LATENCY_MODE, propertyValue = initialvalue.value() | status.isOk() == true, _aidl_return == true | Should Pass |
* | 07 | Check the video decoder state is READY and close it | None | State should be CLOSED, _aidl_return should be true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_positive_1_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: LOW_LATENCY_MODE";

    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : LOW_LATENCY_MODE";
}

/**
 * @brief Test the setProperty method of the video decoder in positive scenarios.
 *
 * This test case verifies the functionality of setting a property (LOW_LATENCY_MODE) 
 * of the video decoder to a new value, and then restores it to the initial value.
 *
 * **Test Group ID:** Basic: 01 @n
 * **Test Case ID:** 015 @n
 * **Priority:** High @n
 * @n
 * **Pre-Conditions:** None @n
 * **Dependencies:** None @n
 * **User Interaction:** None @n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * |:----:| ------------- | ---------- | -------------- | ----- |
 * | 01 | Check video decoder state | None | Video decoder state should be CLOSED | Done by Pre-requisite SetUp function |
 * | 02 | Get video decoder capabilities and validate | None | Should get valid capabilities and validation should be successful | Done by Pre-requisite SetUp function |
 * | 03 | Open the video decoder with supported codec | codec = <some_codec>, supportsSecure = <boolean>, listener = <listener> | Video decoder should open successfully with non-null controller | Done by Pre-requisite SetUp function |
 * | 04 | Verify video decoder state is READY after open | None | Video decoder state should be READY | Done by Pre-requisite SetUp function |
 * | 05 | Store initial value of property LOW_LATENCY_MODE | property = Property::LOW_LATENCY_MODE | Initial value should be retrieved | Should be successful |
 * | 06 | Set property LOW_LATENCY_MODE to intValue(0) | property = Property::LOW_LATENCY_MODE, propertyValue = intValue(0) | Property should be set successfully | Should Pass |
 * | 07 | Restore the initial value of property LOW_LATENCY_MODE | property = Property::LOW_LATENCY_MODE, initialvalue = <initial_value> | Property should be set to initial value successfully | Should be successful |
 * | 08 | Verify video decoder state before close | None | Video decoder state should be READY | Done by Pre-requisite TearDown function |
 * | 09 | Close the video decoder | controller = videoDecoderController | Video decoder should close successfully | Done by Pre-requisite TearDown function |
 */
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_positive_2_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: LOW_LATENCY_MODE";

    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : LOW_LATENCY_MODE";
}

/**
* @brief Tests setting a property value of the video decoder.
*
* This test verifies the ability to set the DECODE_ERROR_POLICY property of the video decoder and ensures that it can be reverted back to its initial value if needed.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 016@n
* **Priority:** High@n
*@n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
*@n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Get the initial value of DECODE_ERROR_POLICY property | property = Property::DECODE_ERROR_POLICY | status.isOk() is true, initialvalue contains property | Should be successful |
* | 02 | Set DECODE_ERROR_POLICY property to 1 | property = Property::DECODE_ERROR_POLICY, propertyValue = 1 | status.isOk()  is true and _aidl_return is true | Should Pass |
* | 03 | Revert DECODE_ERROR_POLICY property to initial value | property = Property::DECODE_ERROR_POLICY, propertyValue = initialvalue.value() | status.isOk() is true | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_3_setProperty)
{
    Property property = Property::DECODE_ERROR_POLICY;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: DECODE_ERROR_POLICY";

    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : DECODE_ERROR_POLICY";
}

/**
* @brief Test to verify setting and resetting a property in L1VideoDecoder
*
* This test checks if the property DECODE_ERROR_POLICY can be successfully set to a new value and then reverted back to its initial value. This is crucial for validating the property setting functionality in the video decoder.
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
* | 01 | Initialize property to DECODE_ERROR_POLICY | property = Property::DECODE_ERROR_POLICY | Initialization Should be successful | |
* | 02 | Store the initial value of the property DECODE_ERROR_POLICY using getProperty() | property = DECODE_ERROR_POLICY | Status is Ok, initialvalue is set | Should Pass |
* | 03 | Set the property DECODE_ERROR_POLICY to this new value using setProperty() | property = DECODE_ERROR_POLICY, propertyValue = 0 | Status is Ok, _aidl_return is true | Should Pass |
* | 04 | Revert property DECODE_ERROR_POLICY back to initial value using setProperty() | property = DECODE_ERROR_POLICY, propertyValue = initialvalue | Status is Ok, _aidl_return is true | Should Pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_4_setProperty)
{
    Property property = Property::DECODE_ERROR_POLICY;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: DECODE_ERROR_POLICY";

    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : DECODE_ERROR_POLICY";
}

/**
* @brief This test case validates setting the AV_SOURCE property of the video decoder to various values.
*
* This test ensures that the video decoder's AV_SOURCE property can be set to different enumerated values without error.
* It checks the functionality by setting the property, verifying the setting, and finally restoring the initial value.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Retrieve initial value of AV_SOURCE property | property = AV_SOURCE | status.isOk() = true | Should be successful |
* | 02 | Set AV_SOURCE to AUTO and verify | property = AV_SOURCE, propertyValue.value = AVSource::AUTO | _aidl_return = true | Should Pass |
* | 03 | Set AV_SOURCE to UNKNOWN and verify | property = AV_SOURCE, propertyValue.value = AVSource::UNKNOWN | _aidl_return = true | Should Pass |
* | 04 | Set AV_SOURCE to IP and verify | property = AV_SOURCE, propertyValue.value = AVSource::IP | _aidl_return = true | Should Pass |
* | 05 | Set AV_SOURCE to TUNER and verify | property = AV_SOURCE, propertyValue.value = AVSource::TUNER | _aidl_return = true | Should Pass |
* | 06 | Set AV_SOURCE to SYSTEM and verify | property = AV_SOURCE, propertyValue.value = AVSource::SYSTEM | _aidl_return = true | Should Pass |
* | 07 | Set AV_SOURCE to HDMI_1 and verify | property = AV_SOURCE, propertyValue.value = AVSource::HDMI_1 | _aidl_return = true | Should Pass |
* | 08 | Set AV_SOURCE to HDMI_2 and verify | property = AV_SOURCE, propertyValue.value = AVSource::HDMI_2 | _aidl_return = true | Should Pass |
* | 09 | Set AV_SOURCE to HDMI_3 and verify | property = AV_SOURCE, propertyValue.value = AVSource::HDMI_3 | _aidl_return = true | Should Pass |
* | 10 | Set AV_SOURCE to HDMI_4 and verify | property = AV_SOURCE, propertyValue.value = AVSource::HDMI_4 | _aidl_return = true | Should Pass |
* | 11 | Set AV_SOURCE to HDMI_5 and verify | property = AV_SOURCE, propertyValue.value = AVSource::HDMI_5 | _aidl_return = true | Should Pass |
* | 12 | Set AV_SOURCE to COMPOSITE_1 and verify | property = AV_SOURCE, propertyValue.value = AVSource::COMPOSITE_1 | _aidl_return = true | Should Pass |
* | 13 | Set AV_SOURCE to COMPOSITE_2 and verify | property = AV_SOURCE, propertyValue.value = AVSource::COMPOSITE_2 | _aidl_return = true | Should Pass |
* | 14 | Set AV_SOURCE to COMPOSITE_3 and verify | property = AV_SOURCE, propertyValue.value = AVSource::COMPOSITE_3 | _aidl_return = true | Should Pass |
* | 15 | Set AV_SOURCE to COMPOSITE_4 and verify | property = AV_SOURCE, propertyValue.value = AVSource::COMPOSITE_4 | _aidl_return = true | Should Pass |
* | 16 | Set AV_SOURCE to COMPOSITE_5 and verify | property = AV_SOURCE, propertyValue.value = AVSource::COMPOSITE_5 | _aidl_return = true | Should Pass |
* | 17 | Restore initial value of AV_SOURCE | property = AV_SOURCE, value = initialvalue | status.isOk() = true | Done by Pre-requisite TearDown function |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_5_setProperty)
{
    ::com::rdk::hal::videodecoder::Property property = ::com::rdk::hal::videodecoder::Property::AV_SOURCE;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AV_SOURCE";

    for (auto avSource :
    {
        ::com::rdk::hal::AVSource::AUTO,
        ::com::rdk::hal::AVSource::UNKNOWN,
        ::com::rdk::hal::AVSource::IP,
        ::com::rdk::hal::AVSource::TUNER,
        ::com::rdk::hal::AVSource::SYSTEM,
        ::com::rdk::hal::AVSource::HDMI_1,
        ::com::rdk::hal::AVSource::HDMI_2,
        ::com::rdk::hal::AVSource::HDMI_3,
        ::com::rdk::hal::AVSource::HDMI_4,
        ::com::rdk::hal::AVSource::HDMI_5,
        ::com::rdk::hal::AVSource::COMPOSITE_1,
        ::com::rdk::hal::AVSource::COMPOSITE_2,
        ::com::rdk::hal::AVSource::COMPOSITE_3,
        ::com::rdk::hal::AVSource::COMPOSITE_4,
        ::com::rdk::hal::AVSource::COMPOSITE_5
    })
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(int32_t(static_cast<int32_t>(avSource)));
        status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AV_SOURCE";
}

/**
* @brief This test verifies the ability to set a property in the video decoder.
*
* The test checks the functionality of setting a specific property (SHA1_CALC) in the video decoder and ensures that the value can be changed and reset successfully.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 019 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Retrieve initial value of SHA1_CALC property | property = SHA1_CALC | status.isOk() should be true | Should be successful |
* | 02| Set SHA1_CALC property to new value | property = SHA1_CALC, propertyValue = 1, _aidl_return | status.isOk() and _aidl_return should be true | Should Pass |
* | 03| Revert SHA1_CALC property to initial value | property = SHA1_CALC, initialvalue, _aidl_return | status.isOk() should be true | Should Pass |
*/
TEST(L1VideoDecoderTest,  test_l1_videodecoder_positive_6_setProperty)
{
    ::com::rdk::hal::videodecoder::Property property = ::com::rdk::hal::videodecoder::Property::SHA1_CALC;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: SHA1_CALC";

    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : SHA1_CALC";
 }

/**
* @brief Test for setting the Property of a video decoder to a specific value and verifying the result.
*
* This test sets the Property 'SHA1_CALC' of the video decoder to a specific integer value and checks whether the setting 
* operation was successful. It also captures the initial value before setting the property and restores it after the test.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Capture initial value of 'SHA1_CALC' property | property = Property::SHA1_CALC, propertyValue = N/A | status.isOk() == true, initialvalue is captured | Should be successful |
* | 02 | Set 'SHA1_CALC' property to integer value 0 | property = Property::SHA1_CALC, propertyValue = 0 | status.isOk() == true, _aidl_return == true | Should Pass |
* | 03 | Restore initial value of 'SHA1_CALC' property | property = Property::SHA1_CALC, propertyValue = initialvalue | status.isOk() == true | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_7_setProperty)
{
    ::com::rdk::hal::videodecoder::Property property = ::com::rdk::hal::videodecoder::Property::SHA1_CALC;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: SHA1_CALC";
    
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : SHA1_CALC";
 }

/**
* @brief Test setting Property::OPERATIONAL_MODE in state STARTED in L1VideoDecoder
*
* This test verifies the setting of the Property::OPERATIONAL_MODE property when the video decoder state is STARTED.
* It ensures that the property can be set to each of the supported operational modes and then reverts it back to its initial value.
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
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Store initial value of Property::OPERATIONAL_MODE | property = Property::OPERATIONAL_MODE | Should be successful |  |
* | 06 | Set Property::OPERATIONAL_MODE to each supported operational mode | supportedOperationalModes as operational_mode | status.isOk() should be true | Should Pass |
* | 07 | Revert Property::OPERATIONAL_MODE to initial value | property = Property::OPERATIONAL_MODE, initialvalue = initialvalue.value() | status.isOk() should be true | Should Pass |
* | 08 | Check the video decoder state is STARTED and then stop it | None | State should be READY, status should be OK | Done by Pre-requisite TearDown function |
* | 09 | Check the video decoder state is READY and close it | None | State should be CLOSED, _aidl_return should be true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_positive_8_setProperty)
{
    // setting Property::OPERATIONAL_MODE in state STARTED
    Property property = Property::OPERATIONAL_MODE;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: OPERATIONAL_MODE";

    /* Set each operational mode from `supportedOperationalModes` (from HAL profile) */
    for(const auto& operational_mode: supportedOperationalModes)
    {
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(static_cast<int32_t>(operational_mode));
        bool _aidl_return;
        status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : OPERATIONAL_MODE";
}

/**
* @brief Test the ability to set properties for the video decoder in the READY state.
*
* This test verifies that the `setProperty` function correctly sets various operational modes for the video decoder and ensures it can revert to the initial state properly.n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 022@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check video decoder state | None | Video decoder state should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get video decoder capabilities and validate | None | Should get valid capabilities and validation should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec | codec = <some_codec>, supportsSecure = <boolean>, listener = <listener> | Video decoder should open successfully with non-null controller | Done by Pre-requisite SetUp function |
* | 04 | Verify video decoder state is READY after open | None | Video decoder state should be READY | Done by Pre-requisite SetUp function |
* | 05 | Retrieve the initial value of Property::OPERATIONAL_MODE | property: OPERATIONAL_MODE | status: isOk() should be true | Should be successful |
* | 06 | Set each supported operational mode and validate | operational_modes from supportedOperationalModes | status: isOk() should be true, _aidl_return: true | Should Pass |
* | 07 | Revert the property to its initial value | property: OPERATIONAL_MODE, initial value | status: isOk(), _aidl_return: true | Should Pass |
* | 08 | Verify video decoder state before close | None | Video decoder state should be READY | Done by Pre-requisite TearDown function |
* | 09 | Close the video decoder | controller = videoDecoderController | Video decoder should close successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_positive_9_setProperty)
{
    // setting Property::OPERATIONAL_MODE in state STARTED
    Property property = Property::OPERATIONAL_MODE;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    bool _aidl_return;

    /* storing initial value */
    Status status = _videoDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: OPERATIONAL_MODE";

    /* Set each operational mode from `supportedOperationalModes` (from HAL profile) */
    for(const auto& operational_mode: supportedOperationalModes)
    {
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(static_cast<int32_t>(operational_mode));
        bool _aidl_return;
        status = _videoDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

    /*clean-up setting back initial value*/
    status = _videoDecoder->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : OPERATIONAL_MODE";
}

/**
* @brief Test the getPropertyMulti function of the L1VideoDecoder interface.
*
* This test checks the positive scenario where multiple properties are retrieved simultaneously from the 
* video decoder using the getPropertyMulti function. The properties tested include RESOURCE_ID, OPERATIONAL_MODE,
* and DECODE_ERROR_POLICY.
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
* | 01| Initialize the PropertyKVPair for RESOURCE_ID. | property = RESOURCE_ID, propertyValue = default | Should be successful | |
* | 02| Push the PROPERTY_ID pair to the propertyKVList. | propertyKVList.push_back(kvPair) | Should be successful | |
* | 03| Initialize the PropertyKVPair for OPERATIONAL_MODE. | property = OPERATIONAL_MODE, propertyValue = default | Should be successful | |
* | 04| Push the OPERATIONAL_MODE pair to the propertyKVList. | propertyKVList.push_back(kvPair2) | Should be successful | |
* | 05| Initialize the PropertyKVPair for DECODE_ERROR_POLICY. | property = DECODE_ERROR_POLICY, propertyValue = default | Should be successful | |
* | 06| Push the DECODE_ERROR_POLICY pair to the propertyKVList. | propertyKVList.push_back(kvPair3) | Should be successful | |
* | 07| Call the getPropertyMulti API of the video decoder with propertyKVList. | propertyKVList, _aidl_return | Status = OK, _aidl_return = true | Should Pass |
* | 08| Validate the first property value. | propertyValue = propertyKVList[0].propertyValue, property = RESOURCE_ID | value for RESOURCE_ID should be valid | Should Pass |
* | 09| Validate the second property value. | propertyValue = propertyKVList[1].propertyValue, property = OPERATIONAL_MODE | value for OPERATIONAL_MODE should be valid | Should Pass |
* | 10| Validate the third property value. | propertyValue = propertyKVList[2].propertyValue, property = DECODE_ERROR_POLICY | value for DECODE_ERROR_POLICY should be valid | Should Pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_1_getPropertyMulti)
{
    ::std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
    ::com::rdk::hal::videodecoder::PropertyKVPair kvPair;
    ::com::rdk::hal::videodecoder::PropertyKVPair kvPair2;
    ::com::rdk::hal::videodecoder::PropertyKVPair kvPair3;
    kvPair.property = ::com::rdk::hal::videodecoder::Property::RESOURCE_ID;
    kvPair.propertyValue.value = ::com::rdk::hal::PropertyValue::Value();
    propertyKVList.push_back(kvPair);
    kvPair2.property = ::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE;
    kvPair2.propertyValue.value = ::com::rdk::hal::PropertyValue::Value();
    propertyKVList.push_back(kvPair2);
    kvPair3.property = ::com::rdk::hal::videodecoder::Property::DECODE_ERROR_POLICY;
    kvPair3.propertyValue.value = ::com::rdk::hal::PropertyValue::Value();
    propertyKVList.push_back(kvPair3);

    bool _aidl_return;
    Status status = _videoDecoder->getPropertyMulti(&propertyKVList, &_aidl_return);

    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    ASSERT_TRUE(validatePropertyValue(propertyKVList[0].propertyValue, propertyKVList[0].property));
    ASSERT_TRUE(validatePropertyValue(propertyKVList[1].propertyValue, propertyKVList[1].property));
    ASSERT_TRUE(validatePropertyValue(propertyKVList[2].propertyValue, propertyKVList[2].property));

}

/**
* @brief Tests the L1 Video Decoder for handling invalid property keys in getPropertyMulti
*
* This test evaluates the behavior of L1 Video Decoder when getPropertyMulti is queried with an invalid property key. This ensures the decoder correctly identifies and handles invalid properties.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 024 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize PropertyKVPair and set invalid property key | invalidPair.property = -1 | Initialization Should be successful | |
* | 02 | Add the invalid property pair to propertyKVList | N/A | N/A | Should be successful |
* | 03 | Call getPropertyMulti with the property list containing the invalid property | propertyKVList | status.isOk() = should be true, _aidl_return = false | Should Fail |
* | 04 | Validate that the status is OK | status.isOk() | status.isOk() = true | Should pass |
* | 05 | Validate that the aidl return is false | _aidl_return | _aidl_return = false | Should pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_1_getPropertyMulti)
{
    ::std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
    ::com::rdk::hal::videodecoder::PropertyKVPair invalidPair;
    invalidPair.property = static_cast<::com::rdk::hal::videodecoder::Property>(-1); //invalid property
    invalidPair.propertyValue.value = ::com::rdk::hal::PropertyValue::Value();
    propertyKVList.push_back(invalidPair);
    bool _aidl_return;
    Status status = _videoDecoder->getPropertyMulti(&propertyKVList, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(_aidl_return);
}

/**
* @brief Tests the positive scenario of the getPropertyMulti method of the L1VideoDecoder
*
* This test validates the successful retrieval of multiple video decoder properties. The 
* test ensures the correct implementation of getPropertyMulti and verifies the expected 
* property values are returned and validated.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize the PropertyKVPair vector and add LOW_LATENCY_MODE property with default value. | kvPair.property = Property::LOW_LATENCY_MODE, kvPair.propertyValue = PropertyValue() | Should be successful | |
* | 02 | Call getPropertyMulti method with propertyKVList containing the LOW_LATENCY_MODE property. | propertyKVList = {kvPair}, _aidl_return = false (initially) | Status.isOk() should be true and _aidl_return is true | Should Pass |
* | 03 | Validate the property value returned for LOW_LATENCY_MODE. | propertyKVList[0].propertyValue, propertyKVList[0].property | Assertion for validatePropertyValue should pass | Should Pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_2_getPropertyMulti)
{

    ::std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
    com::rdk::hal::videodecoder::PropertyKVPair kvPair;
    kvPair.property = ::com::rdk::hal::videodecoder::Property::LOW_LATENCY_MODE;
    kvPair.propertyValue = ::com::rdk::hal::PropertyValue();
    propertyKVList.push_back(kvPair);
    bool _aidl_return;
    Status status = _videoDecoder->getPropertyMulti(&propertyKVList, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(_aidl_return);
    ASSERT_TRUE(validatePropertyValue(propertyKVList[0].propertyValue, propertyKVList[0].property));
}

/**
* @brief Test to validate the positive scenario for getPropertyMulti in L1VideoDecoder
*
* This test checks if the getPropertyMulti function from the video decoder is able to retrieve properties correctly.@n
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
* | 01 | Initialize PropertyKVPair object with property set to SECURE_VIDEO | property = SECURE_VIDEO | Should be successful | |
* | 02 | Create a vector and push back the initialized PropertyKVPair object | propertyKVList.push_back(kvPair) | Should be successful | |
* | 03 | Call getPropertyMulti with the propertyKVList | propertyKVList = list contain valid kvpair | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 04 | Check if the status is OK | status.isOk() | Should be true | Should be successful |
* | 05 | Validate the property value by asserting on expected value | validatePropertyValue(propertyKVList[0].propertyValue, propertyKVList[0].property) | validatePropertyValue should return true | Should Pass |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_3_getPropertyMulti)
{
    ::std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
    com::rdk::hal::videodecoder::PropertyKVPair kvPair;
    kvPair.property = ::com::rdk::hal::videodecoder::Property::SECURE_VIDEO;
    kvPair.propertyValue = ::com::rdk::hal::PropertyValue();
    propertyKVList.push_back(kvPair);
    bool _aidl_return;
    Status status = _videoDecoder->getPropertyMulti(&propertyKVList, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(_aidl_return);
    ASSERT_TRUE(validatePropertyValue(propertyKVList[0].propertyValue, propertyKVList[0].property));
}

/**
* @brief This test case validates the positive scenario for the function getPropertyMulti in the video decoder.
*
* The test checks the behavior of the getPropertyMulti function, ensuring that it correctly retrieves multiple properties from the video decoder API. This is a positive test case where the function is expected to succeed.
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
* | 01 | Initialize a vector of PropertyKVPair and add a pair with Property::AV_SOURCE | None | Vector initialized and pair added successfully | Should be successful |
* | 02 | Call getPropertyMulti with the initialized vector | propertyKVList = list contain valid kvpair | Status.isOk() should return true, _aidl_return is true | Should Pass |
* | 03 | Validate the property value retrieved by getPropertyMulti | propertyValue from propertyKVList | propertyValue matches expected value | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_4_getPropertyMulti)
{
    ::std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
    com::rdk::hal::videodecoder::PropertyKVPair kvPair;
    kvPair.property = ::com::rdk::hal::videodecoder::Property::AV_SOURCE;
    kvPair.propertyValue = ::com::rdk::hal::PropertyValue();
    propertyKVList.push_back(kvPair);
    bool _aidl_return;
    Status status = _videoDecoder->getPropertyMulti(&propertyKVList, &_aidl_return);

    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(_aidl_return);
    ASSERT_TRUE(validatePropertyValue(propertyKVList[0].propertyValue, propertyKVList[0].property));
}

/**
* @brief Test the negative scenario for retrieving multiple properties of a video decoder.
*
* This test examines how the video decoder handles a request to get multiple properties when it is expected to fail. The objective is to ensure that the video decoder appropriately returns an error status and does not retrieve the properties.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 028 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Set up the initial conditions and create a vector to store property key-value pairs. | propertyKVList = empty vector, _aidl_return = uninitialized | Successful setup of test data structures. | Should be successful |
* | 02 | Call the getPropertyMulti method of the video decoder to retrieve multiple properties | propertyKVList = empty vector, _aidl_return = uninitialized | status.isok() should be true, _aidl_return should be false | Should Fail |
* | 03 | Verify that the status and return value  | status.isOk(), _aidl_return| Test should assert true on status.isok and _aidl_return should be false | Should Fail |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_2_getPropertyMulti)
{
    ::std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
    bool _aidl_return;
    Status status = _videoDecoder->getPropertyMulti(&propertyKVList, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(_aidl_return);
}

/**
 * @brief Test the getState method of L1 video decoder for positive scenario
 *
 * This test evaluates the getState method of L1 video decoder to ensure it returns a valid state and successful status.@n
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
 * | Variation / Step | Description | Test Data |Expected Result |Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Invoke getState method on _videoDecoder object | state = valid pointer | status.isOk() should be true, isValidState(state) should be true | Should Pass |
 */
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_1_getState)
{
    ::com::rdk::hal::State state;
    ::android::binder::Status status = _videoDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(isValidState(state));
}

/**
* @brief This test verifies the positive scenario for opening a video decoder using various supported codecs.
*
* This test case checks the initial state and capabilities of the video decoder. It iterates over supported codecs, attempts to open the video decoder, validates the state, and then closes the decoder again. This ensures that the decoder can be successfully opened and closed for all supported codecs.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 030@n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Validate the initial state of the video decoder to ensure it is closed. | N/A | The state should be ::com::rdk::hal::State::CLOSED | Should be successful |
* | 02| Invoke the getCapabilities() of the video decoder. | N/A | The status.isok() should be true and capabilities should be valid | Should be successful |
* | 03| Iterate over each supported codec and attempt to open the video decoder. | codec = iterate over supported codecs, supportsSecure = value from capability, videoDecoderControllerListener, videoDecoderController | The status.isok() should be true, and videoDecoderController should not be null | Should Pass |
* | 04| Close the video decoder for the current codec. | videoDecoderController = valid value | The status.isok() should be true, and _aidl_return should be true | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_1_open)
{
    // prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = _videoDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

    // prerequisite - checking capabilities
    ::com::rdk::hal::videodecoder::Capabilities capabilities;
    status = _videoDecoder->getCapabilities(&capabilities);
    EXPECT_TRUE(status.isOk());
    validateCapabilities(capabilities);

    android::sp<IVideoDecoderControllerListener> videoDecoderControllerListener = new VideoDecoderControllerListener();
    android::sp<IVideoDecoderController> videoDecoderController;
    
    /* iterating over each codec of the videodecoder */
    for(const auto& _codec: capabilities.supportedCodecs)
    {
        status = _videoDecoder->open(_codec.codec, capabilities.supportsSecure, videoDecoderControllerListener, &videoDecoderController);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(videoDecoderController != nullptr);
        //cleaning up
        bool _aidl_return;
        status = _videoDecoder->close(videoDecoderController, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

}

/**
* @brief Negative test for opening a video decoder with an invalid codec
*
* This test is designed to verify that attempting to open a video decoder with an invalid codec results in an error.@n
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :-------------: | ----------- | ---------- |-------------- | ----- |
* | 01 | Initialize state and get video decoder state | N/A | status.isOk() == true, state == ::com::rdk::hal::State::CLOSED | Should be successful |
* | 02 | Invoke the getCapabilities() of the video decoder. | N/A | The status.isok() should be true and capabilities should be valid | Should be successful |
* | 03 | Attempt to open video decoder with invalid codec and check the result | codec = 9999, capabilities.supportsSecure = value from capability, videoDecoderControllerListener, &videoDecoderController) | status.isOk() == false, videoDecoderController == nullptr | Should Fail |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_1_open)
{
    // prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = _videoDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

    // prerequisite - checking capabilities
    ::com::rdk::hal::videodecoder::Capabilities capabilities;
    status = _videoDecoder->getCapabilities(&capabilities);
    EXPECT_TRUE(status.isOk());
    validateCapabilities(capabilities);

    android::sp<IVideoDecoderControllerListener> videoDecoderControllerListener = new VideoDecoderControllerListener();
    android::sp<IVideoDecoderController> videoDecoderController;

    capabilities.supportedCodecs[0].codec = static_cast<Codec>(9999); //invalid codec
    status = _videoDecoder->open(capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, videoDecoderControllerListener, &videoDecoderController);
    ASSERT_FALSE(status.isOk());
    ASSERT_TRUE(videoDecoderController == nullptr);
}

/**
* @brief Test for attempting to open video decoder in an invalid state.
*
* This test checks the behavior of the video decoder when an attempt is made to open it while being in the READY state, 
* which is invalid according to the setup specifications. It verifies that the correct error code is returned.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 032 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check video decoder state | None | Video decoder state should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get video decoder capabilities and validate | None | Should get valid capabilities and validation should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec | codec = valid, supportsSecure = valid, listener = valid | Video decoder should open successfully with non-null controller | Done by Pre-requisite SetUp function |
* | 04 | Attempt to open the video decoder in READY state and expect exception | codec = capabilities.supportedCodecs[0].codec, secure = capabilities.supportsSecure, listener = new VideoDecoderControllerListener(), controller = videoDecoderController | status.exceptionCode() = Status::EX_ILLEGAL_STATE | Should Fail |
* | 05 | Verify video decoder state before close | None | Video decoder state should be READY | Done by Pre-requisite TearDown function |
* | 06 | Close the video decoder | controller = videoDecoderController | Video decoder should close successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_negative_2_open)
{
    sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();
    //calling open() when state READY (EX_ILLEGAL_STATE)
    Status status = _videoDecoder->open(capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, &videoDecoderController);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Test the positive scenario for closing the video decoder
*
* This test validates the functionality of the close method of the video decoder API. The objective 
* is to ensure that the video decoder is closed successfully without any errors and that the return 
* status is OK.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check video decoder state | None | Video decoder state should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get video decoder capabilities and validate | None | Should get valid capabilities and validation should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec | codec = valid, supportsSecure = valid, listener = valid | Video decoder should open successfully with non-null controller | Done by Pre-requisite SetUp function |
* | 04 | Call close API on videoDecoder instance | videoDecoderController = <appropriate_value>, aidl_return = valid | status.isOk() = true, aidl_return = true | Should Pass |
* | 05 | Assert that the status is OK | status.isOk() | status.isOk() = true | Should be successful | 
* | 06 | Assert that the return value aidl_return is true | aidl_return | aidl_return = true | Should be successful | 
*/
TEST_F(L1VideoDecoderCloseTest, test_l1_videodecoder_positive_1_close)
{
    bool aidl_return;
    ::android::binder::Status status = _videoDecoder->close(videoDecoderController, &aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(aidl_return);
}

/**
* @brief Test to validate negative scenario of closing the video decoder from state CLOSED
*
* This test case ensures the proper handling of close operation when the video decoder is already in a CLOSED state. Ensuring such states handle repeated calls gracefully is critical for robustness.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 034@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check video decoder state | None | Video decoder state should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get video decoder capabilities and validate | None | Should get valid capabilities and validation should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec | codec = valid, supportsSecure = valid, listener = valid | Video decoder should open successfully with non-null controller | Done by Pre-requisite SetUp function |
* | 04 | Perform first close operation on video decoder | videoDecoderController = valid value, aidl_return = valid value | status.isOk() should be true, aidl_return should be true | Should be successful |
* | 05 | Perform second close operation on already CLOSED video decoder | videoDecoderController = allready closed, aidl_return = <value> | status.isOk() should be true, aidl_retur should be false | Should Pass |
*/
TEST_F(L1VideoDecoderCloseTest, test_l1_videodecoder_negative_1_close)
{
    bool aidl_return;
    ::android::binder::Status status = _videoDecoder->close(videoDecoderController, &aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(aidl_return);
    //calling close() from state CLOSED
    status = _videoDecoder->close(videoDecoderController, &aidl_return);
    ASSERT_TRUE(status.isOk());
    EXPECT_FALSE(aidl_return);
}

/**
* @brief This test verifies that the video decoder can successfully register and unregister an event listener.
*
* The objective of this test is to confirm that the video decoder successfully registers an event listener and the status returned is OK. The test also confirms the successful unregistration of the event listener to ensure proper cleanup.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 035@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize and allocate a new VideoDecoderEventListener object | listener = new VideoDecoderEventListener() | listener is created successfully | Should be successful |
* | 02 | Call registerEventListener API on _videoDecoder object | listener, &_aidl_return | status.isOk() == true, _aidl_return == true| Should Pass |
* | 03 | Assert the registration status is OK | status.isOk() | status.isOk() is true | Should be successful |
* | 04 | Assert the _aidl_return is true to confirm registration | _aidl_return | _aidl_return is true | Should be successful |
* | 05 | Call unregisterEventListener API on _videoDecoder object | listener, &_aidl_return | status.isOk() == true, _aidl_return == true| Should Pass |
* | 06 | Assert the unregistration status is OK |status.isOk() | status.isOk() is true | Should be successful |
* | 07 | Assert the _aidl_return is true to confirm unregistration | _aidl_return | _aidl_return is true | Should be successful |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_positive_1_registerEventListener)
{
    bool _aidl_return;
    sp<IVideoDecoderEventListener> listener = new VideoDecoderEventListener();
    ::android::binder::Status status = _videoDecoder->registerEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
    //cleaning up
    status = _videoDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

/**
* @brief Test the behavior of registering a Video Decoder Event Listener multiple times.
*
* This test verifies that the system prevents multiple registrations of the same Video Decoder Event Listener. 
* Initially, the listener is registered during the SetUp phase of the test. The main test attempts to register 
* the listener again and expects the registration attempt to fail, confirming that duplicate registrations are 
* properly handled. Finally, the listener is cleanly unregistered.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 036@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Register the listener in the SetUp function. | listener = new VideoDecoderEventListener() | Registration should be successful. | Done by Pre-requisite SetUp function |
* | 02 | Verify successful status and return value in SetUp. | status.isOk() and _aidl_return | Both should be true. | Done by Pre-requisite SetUp function |
* | 03 | Attempt to register the listener again in the test body. | listener | Registration should fail. | Should Fail |
* | 04 | Verify the failure status and return value. | status.isOk() and _aidl_return | status.isok() should be true and _aidl_return should be false. | Should pass |
* | 05 | Unregister the listener to clean up. | listener | Unregistration should be successful. | Should be successful |
* | 06 | Verify successful status and return value after unregistration. | status.isOk() and _aidl_return | Both should be true. | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderEventRegisterTest, test_l1_videodecoder_negative_1_registerEventListener)
{
    bool _aidl_return;
    // Attempt to register the listener again (first registration in L1VideoDecoderEventRegisterTest)
    Status status = _videoDecoder->registerEventListener(listener, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(_aidl_return);
    //cleaning up
    status = _videoDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

/**
* @brief Test function to validate the unregistration of the video decoder event listener.
*
* This test verifies that the event listener for the video decoder can be successfully unregistered. Ensuring that this functionality works is essential for proper management of event listeners in the video decoder module, preventing potential memory leaks or unexpected behavior due to lingering event listeners.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 037@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize and register the event listener through SetUp | bool _aidl_return = true | listener registered successfully |Done by Pre-requisite SetUp function|
* | 02| Unregister the event listener | bool _aidl_return, listener = valid listner | listener unregistered successfully | Should Pass |
* | 03| Validate unregister status | _aidl_return, status| _aidl_return = true, status.isok() = true | Should be successful |
*/
TEST_F(L1VideoDecoderEventRegisterTest, test_l1_videodecoder_positive_1_unregisterEventListener)
{
    bool _aidl_return;
    ::android::binder::Status status = _videoDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

/**
* @brief Tests unregistering a video decoder event listener, validates behavior when unregistering already unregistered listener.
*
* This test case validates that the system correctly handles the unregistration of a video decoder event listener.
* It particularly ensures that unregistering an already unregistered listener is handled as expected.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 038@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize and register the event listener through SetUp | bool _aidl_return = true | listener registered successfully |Done by Pre-requisite SetUp function|
* | 02| Unregister the event listener |  valid listener | The unregistration status should be OK, _aidl_return = true | Should be successful |
* | 03| Unregister the event listener again without prior register | listener | status.ok() is true, _aidl_return = false | Should Fail |
*/
TEST_F(L1VideoDecoderEventRegisterTest, test_l1_videodecoder_negative_1_unregisterEventListener)
{
    bool _aidl_return;
    ::android::binder::Status status = _videoDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
    //calling unregister() again
    status = _videoDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the negative scenario for unregistering a video decoder event listener
*
* This test aims to verify the behavior of the unregisterEventListener function in a negative case scenario where the function should fail both in returning the status and the boolean return value.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 039@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Instantiate the IVideoDecoderEventListener interface | N/A | N/A | Should be successful |
* | 02| Call the unregisterEventListener API with the listener instance | listener = new VideoDecoderEventListener() | status.isok() should true, _aidl_return = false | Should Fail |
*/
TEST(L1VideoDecoderTest, test_l1_videodecoder_negative_2_unregisterEventListener)
{
    bool _aidl_return;
    ::android::sp<::com::rdk::hal::videodecoder::IVideoDecoderEventListener> videoDecoderEventListener = new VideoDecoderEventListener();
    ::android::binder::Status status = _videoDecoder->unregisterEventListener(videoDecoderEventListener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief This test verifies the functionality of starting the video decoder.
*
* It checks whether the video decoder starts correctly and also ensures it stops successfully after starting.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 040@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01               | Set up pre-requisite by checking initial state                  | Initial state: CLOSED                                            | Should be CLOSED                                       | Done by Pre-requisite SetUp function |
* | 02               | Set up pre-requisite by checking decoder capabilities           | Capabilities available                                          | Should return valid capabilities                       | Should be successful                 |
* | 03               | Set up pre-requisite by opening the video decoder               | Codec: <specific codec>, Supports Secure: <boolean>, Listener: <listener>  | Should open the video decoder                          | Should be successful                 |
* | 04               | Verify state after opening                                      | State: READY                                                     | Should be READY                                        | Should be successful                 |
* | 05               | Start the video decoder                                         | NA                                                               | Should start successfully                              | Should Pass                          |
* | 06               | Stop the video decoder                                          | NA                                                               | Should stop successfully                               | Should be successful                 |
* | 07               | Verify state before closing                                    | State: READY                                                     | Should be READY                                        | Done by Pre-requisite TearDown function|
* | 08               | Close the video decoder controller                              | NA                                                               | Should close successfully                              | Done by Pre-requisite TearDown function|
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_positive_1_start)
{
    Status status = videoDecoderController->start();
    ASSERT_TRUE(status.isOk());
    //cleaning up
    status = videoDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Tests the start() method of the video decoder in an illegal state.
*
* This test validates that calling the start() method on a video decoder when it is already in the STARTED state returns an EX_ILLEGAL_STATE exception. This ensures that no illegal state transitions are allowed, thereby maintaining the integrity of the video decoder's state management.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 041@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Attempt to start the decoder again | None | Status should have exception code EX_ILLEGAL_STATE | Should fail |
* | 06 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 07 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_1_start)
{
    //calling start() from STARTED state (EX_ILLEGAL_STATE)
    Status status = videoDecoderController->start();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Test stopping the video decoder controller.
*
* This test checks the functionality of stopping the video decoder controller after it has been started.
* It ensures that the stop() function works correctly and the video decoder transitions to the STOPPED state.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 042@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status.isok() is true, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Call the stop() function. | _N/A_ | Status.isok() is true | Should Pass |
* | 06 | Check the video decoder state is READY and close it | None | State should be CLOSED, _aidl_return should be true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStopTest, test_l1_videodecoder_positive_1_stop)
{
    ::android::binder::Status status = videoDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Testing the stop function on an L1 video decoder when in READY state.
*
* This test evaluates the behavior of the video decoder's stop function when it is called in the READY state.
* The API is expected to return the EX_ILLEGAL_STATE exception code.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 043@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Set up and check initial state | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Check initial capabilities | None | Should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open video decoder with supported Codec and listener | Codec: capabilities.supportedCodecs[0].codec, Secure: capabilities.supportsSecure, Listener: listener | Should Pass | Done by Pre-requisite SetUp function |
* | 04 | Check state transition to READY | None | State should be READY | Done by Pre-requisite SetUp function |
* | 05 | Call stop() from READY state | None | exceptionCode() == EX_ILLEGAL_STATE | Should Pass |
* | 06 | Tear down and close video decoder | None | Should be successful | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_negative_1_stop)
{
    //calling stop() from READY state (EX_ILLEGAL_STATE)
    ::android::binder::Status status = videoDecoderController->stop();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Test for ensuring proper error handling when decoding buffer with an invalid handle.
*
* Ensures the video decoder correctly handles an illegal argument when an invalid buffer handle is provided. Validates that the decodeBuffer function raises the correct exception when called with an improper buffer handle@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 044@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Invoke decodeBufferbuffer api | nsPresentationTime = 100000, bufferHandle = -1 | Status is EX_ILLEGAL_ARGUMENT |  Should Fail |
* | 06 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 07 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/

TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_1_decodeBuffer)
{
    int64_t nsPresentationTime = 100000;
    int64_t bufferHandle = -1;  // Invalid buffer handle
    Status status = videoDecoderController->decodeBuffer(nsPresentationTime, bufferHandle);
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
}

/**
* @brief Test for ensuring proper error handling when decoding buffer with an invalid nsPresentationTime.
*
* Ensures the video decoder correctly handles an illegal argument when an invalid nsPresentationTime is provided. Validates that the decodeBuffer function raises the correct exception when called with an improper buffer handle@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 045@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Invoke decodeBufferbuffer api | nsPresentationTime = -100000, bufferHandle = 1 | Status is EX_ILLEGAL_ARGUMENT |  Should Fail |
* | 06 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 07 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_2_decodeBuffer)
{
    int64_t nsPresentationTime = -100000;// Invalid nsPresentationTime
    int64_t bufferHandle = 1;  
    Status status = videoDecoderController->decodeBuffer(nsPresentationTime, bufferHandle);
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
}

/**
* @brief Tests the positive scenario of flushing the video decoder.
*
* This test validates that the video decoder flush operation is successful when reset is set to true.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 046@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Flush the video decoder with reset = true | reset = true | Status.isok() should be true | Should Pass |
* | 06 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 07 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_positive_1_flush)
{
    bool reset = true;
    Status status = videoDecoderController->flush(reset);
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Tests the flush function in an invalid state.
*
* This test checks that calling the flush function from the READY state returns an EX_ILLEGAL_STATE exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 047@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01     | Verify initial state is CLOSED          | N/A                                | state = CLOSED               | Done by Pre-requisite SetUp function |
* | 02     | Check capabilities                      | N/A                                | capabilities Should be valid | Done by Pre-requisite SetUp function |
* | 03     | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04     | Validate state is READY                 | N/A                                | state = READY                | Should be successful |
* | 05     | Call flush() from READY state           | reset = true                       | status.exceptionCode() = EX_ILLEGAL_STATE | Should FAIL |
* | 06     | Close video decoder and verify state | N/A                                | state will be CLOSED        | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_negative_1_flush)
{
    //calling flush() from READY state (EX_ILLEGAL_STATE)
    bool reset = true;
    Status status = videoDecoderController->flush(reset);
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief This test case verifies that the signalDiscontinuity method works as expected in a positive scenario.
*
* This test ensures that the signalDiscontinuity API is successfully called and returns a valid status, confirming that the video decoder can handle a signal discontinuity scenario correctly.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 048@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the video decoder state is CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Get the capabilities of the video decoder and validate them | None | Capabilities should be valid | Done by Pre-requisite SetUp function |
* | 03 | Open the video decoder with supported codec and set listener | supportedCodec = capabilities.supportedCodecs[0].codec, supportsSecure = capabilities.supportsSecure | Video decoder should be opened and in READY state | Done by Pre-requisite SetUp function |
* | 04 | Start the video decoder | None | Status should be OK, state should be STARTED | Done by Pre-requisite SetUp function |
* | 05 | Invoke signalDiscontinuity and check for a successful status. |status.isOk() | status.isOk() should be true | Should Pass |
* | 06 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 07 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_positive_1_signalDiscontinuity)
{
    Status status = videoDecoderController->signalDiscontinuity();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for video decoder signalDiscontinuity negative test case.
*
* This test checks the behavior of the signalDiscontinuity method in the READY state, expecting it to fail with EX_ILLEGAL_STATE.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 049@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | check initial state | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Check initial capabilities | None | Should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open video decoder with supported Codec and listener | Codec: capabilities.supportedCodecs[0].codec, Secure: capabilities.supportsSecure, Listener: listener | Should Pass | Done by Pre-requisite SetUp function |
* | 04 | Check state transition to READY | None | State should be READY | Done by Pre-requisite SetUp function |
* | 05 | Invoke signalDiscontinuity on the video decoder controller. | None | EX_ILLEGAL_STATE exception code. | Should Fail |
* | 06 | close video decoder | None | Should be successful | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_negative_1_signalDiscontinuity)
{
    //calling signalDiscontinuity() from READY state (EX_ILLEGAL_STATE)
    Status status = videoDecoderController->signalDiscontinuity();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Tests the signaling of EOS (End Of Stream) to the video decoder.
*
* This test ensures that the video decoder can properly handle the EOS signal as expected. It is crucial for validating the correct behavior of the decoder state management on receiving an EOS signal.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 050@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Invokes the SetUp function from L1VideoDecoderOpenTest | None | Should be successful | Done by Pre-requisite SetUp function |
* | 02 | Checks initial state of the video decoder to be CLOSED | None | State should be CLOSED | Should be successful |
* | 03 | Retrieves and validates the decoder capabilities | None | Capabilities should be valid | Should be successful |
* | 04 | Opens the video decoder with the first supported codec | capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, videoDecoderController | Status should be OK, videoDecoderController should not be null | Should be successful |
* | 05 | Checks the state of the video decoder to be READY | None | State should be READY | Should be successful |
* | 06 | Starts the video decoder | None | Status should be OK, state should be STARTED | Should be successful |
* | 07 | Invokes the test case API: signalEOS | Status.isok() | Status.isok() should be true | Should Pass |
* | 08 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 09 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_positive_1_signalEOS)
{
    Status status = videoDecoderController->signalEOS();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test the signalEOS function in an illegal state
*
* This test verifies the behavior of the signalEOS() function when invoked while the video decoder is in an illegal state (READY state). The test expects the function to return an EX_ILLEGAL_STATE exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 051@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Set up and check initial state | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Check initial capabilities | None | Should be successful | Done by Pre-requisite SetUp function |
* | 03 | Open video decoder with supported Codec and listener | Codec: capabilities.supportedCodecs[0].codec, Secure: capabilities.supportsSecure, Listener: listener | Should Pass | Done by Pre-requisite SetUp function |
* | 04 | Check state transition to READY | None | State should be READY | Done by Pre-requisite SetUp function |
* | 05 | Invoke signalEOS() function to check behavior | status.exceptionCode() | Status::EX_ILLEGAL_STATE | Should Fail |
* | 06 | close video decoder | None | Should be successful | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderOpenTest, test_l1_videodecoder_negative_1_signalEOS)
{
    //calling signalEOS() from READY state (EX_ILLEGAL_STATE)
    Status status = videoDecoderController->signalEOS();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Negative test for parsing codec-specific data with invalid input and AVC_DECODER_CONFIGURATION_RECORD as csdVideoFormat.
*
* This test checks the behavior of the video decoder when given invalid codec-specific data.
* The test ensures that the parser correctly identifies and fails on invalid data.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 052@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Checks initial state of the video decoder to be CLOSED | None | State should be CLOSED | Should be successful |
* | 02 | Retrieves and validates the decoder capabilities | None | Capabilities should be valid | Should be successful |
* | 03 | Opens the video decoder with the first supported codec | capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, videoDecoderController | Status should be OK, videoDecoderController should not be null | Should be successful |
* | 04 | Checks the state of the video decoder to be READY | None | State should be READY | Should be successful |
* | 05 | Starts the video decoder | None | Status should be OK, state should be STARTED | Should be successful |
* | 06 | Call parseCodecSpecificData with invalid data. | csdVideoFormat: AVC_DECODER_CONFIGURATION_RECORD, invalidCodecData: {0x00, 0x01, 0x02}, _aidl_return: false | Status.isok() is true, _aidl_return is false | Should Fail |
* | 07 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 08 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_1_parseCodecSpecificData)
{
    std::vector<uint8_t> invalidCodecData = {0x00, 0x01, 0x02};
    bool _aidl_return;
    ::com::rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat = ::com::rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD;
    ::android::binder::Status status = videoDecoderController->parseCodecSpecificData(csdVideoFormat, invalidCodecData, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the negative scenario for parsing codec specific data with an empty codec data and AVC_DECODER_CONFIGURATION_RECORD as csdVideoFormat.
*
* This test determines how the video decoder handles an empty codec data vector which is invalid.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 053@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Checks initial state of the video decoder to be CLOSED | None | State should be CLOSED | Should be successful |
* | 02 | Retrieves and validates the decoder capabilities | None | Capabilities should be valid | Should be successful |
* | 03 | Opens the video decoder with the first supported codec | capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, videoDecoderController | Status should be OK, videoDecoderController should not be null | Should be successful |
* | 04 | Checks the state of the video decoder to be READY | None | State should be READY | Should be successful |
* | 05 | Starts the video decoder | None | Status should be OK, state should be STARTED | Should be successful |
* | 06 | Invoke parseCodecSpecificData with empty codec data | csdVideoFormat = AVC_DECODER_CONFIGURATION_RECORD, invalidCodecData = {} | Status.isok() should true, `_aidl_return` is false | Should Fail |
* | 07 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 08 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_2_parseCodecSpecificData)
{
    std::vector<uint8_t> invalidCodecData = {};
    bool _aidl_return;
    ::com::rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat = ::com::rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD;
    ::android::binder::Status status = videoDecoderController->parseCodecSpecificData(csdVideoFormat, invalidCodecData, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the negative scenario for parsing codec specific data with an empty codec data and HEVC_DECODER_CONFIGURATION_RECORD as csdVideoFormat.
*
* This test determines how the video decoder handles an empty codec data vector which is invalid.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 054@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Checks initial state of the video decoder to be CLOSED | None | State should be CLOSED | Should be successful |
* | 02 | Retrieves and validates the decoder capabilities | None | Capabilities should be valid | Should be successful |
* | 03 | Opens the video decoder with the first supported codec | capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, videoDecoderController | Status should be OK, videoDecoderController should not be null | Should be successful |
* | 04 | Checks the state of the video decoder to be READY | None | State should be READY | Should be successful |
* | 05 | Starts the video decoder | None | Status should be OK, state should be STARTED | Should be successful |
* | 06 | Invoke parseCodecSpecificData with empty codec data | csdVideoFormat = HEVC_DECODER_CONFIGURATION_RECORD, invalidCodecData = {} | Status.isok() should true, `_aidl_return` is false | Should Fail |
* | 07 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 08 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_3_parseCodecSpecificData)
{
    std::vector<uint8_t> invalidCodecData = {};
    bool _aidl_return;
    ::com::rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat = ::com::rdk::hal::videodecoder::CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD;
    ::android::binder::Status status = videoDecoderController->parseCodecSpecificData(csdVideoFormat, invalidCodecData, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the negative scenario for parsing codec specific data with an empty codec data and AV1_DECODER_CONFIGURATION_RECORD as csdVideoFormat.
*
* This test determines how the video decoder handles an empty codec data vector which is invalid.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 055@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Checks initial state of the video decoder to be CLOSED | None | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Retrieves and validates the decoder capabilities | None | Capabilities should be valid | Should be successful |
* | 03 | Opens the video decoder with the first supported codec | capabilities.supportedCodecs[0].codec, capabilities.supportsSecure, listener, videoDecoderController | Status should be OK, videoDecoderController should not be null | Should be successful |
* | 04 | Checks the state of the video decoder to be READY | None | State should be READY | Should be successful |
* | 05 | Starts the video decoder | None | Status should be OK, state should be STARTED | Should be successful |
* | 06 | Invoke parseCodecSpecificData with empty codec data | csdVideoFormat = AV1_DECODER_CONFIGURATION_RECORD, invalidCodecData = {} | Status.isok() should true, `_aidl_return` is false | Should Fail |
* | 07 | Stop the decoder | None | Decoder should stop and state should revert to READY | Done by Pre-requisite TearDown function |
* | 08 | Close the video decoder | None | Decoder should close successfully and return true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1VideoDecoderStartTest, test_l1_videodecoder_negative_4_parseCodecSpecificData)
{
    std::vector<uint8_t> invalidCodecData = {};
    bool _aidl_return;
    ::com::rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat = ::com::rdk::hal::videodecoder::CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD;
    ::android::binder::Status status = videoDecoderController->parseCodecSpecificData(csdVideoFormat, invalidCodecData, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

int main(int argc, char **argv) 
{
    std::string yaml_file;
    int opt;
    int result1,result2;
    int number_of_videodecoders;
    int number_of_supportedOperationalModes;
    int supportedcodec_count;
    int supportedDynamicRanges_count;
    decoder temporary;
    _CodecCapabilities temporary_codeccapability;
    ::com::rdk::hal::videodecoder::DynamicRange temporary_dynamicrange;


    std::string videodecoderpath = "components.IVideoDecoderManager.IVideoDecoder";
    std::string supportedOperationalModeslistpath = "components.IVideoDecoderManager.supportedOperationalModes";

    ::testing::InitGoogleTest(&argc, argv);
    if(getService() == -1)
    {
        return -1;
    }

    while ((opt = getopt(argc, argv, "p:")) != -1) 
    {
        switch (opt) 
        {
            case 'p':
                yaml_file = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -p <yaml_file>" << std::endl;
                return 1;
        }
    }

    if (!yaml_file.empty()) 
    {
        try 
        {
            YAML::Node config = YAML::LoadFile(yaml_file.c_str());
            number_of_videodecoders = get_list_Count_from_profile(config, videodecoderpath);
            if(number_of_videodecoders <= 0)
            {
                std::cout<< " No videodecoders find in the profile" <<std::endl;
                return -1;
            }
            std::cout <<"videodecoders : " <<number_of_videodecoders <<std::endl;
            number_of_supportedOperationalModes = get_list_Count_from_profile(config, supportedOperationalModeslistpath);
            for(int p=0; p<number_of_supportedOperationalModes; ++p)
            {
                std::string supportedOperationalModespath = supportedOperationalModeslistpath+"."+std::to_string(p);
                std::string operationalmode = get_string_value_from_profile(config, supportedOperationalModespath);
                supportedOperationalModes.push_back(StringTooperationalmode(operationalmode));
            }

            for(int i=0; i<number_of_videodecoders; ++i)
            {
                std::string videodecoderidpath = videodecoderpath+"."+std::to_string(i) + ".id";
                std::string supportedcodecpath = videodecoderpath+"."+std::to_string(i) + ".supportedCodecs";
                std::string supportedDynamicRangespath = videodecoderpath+"."+std::to_string(i) + ".supportedDynamicRanges";
                std::string supportsSecurepath = videodecoderpath+"."+std::to_string(i) + ".supportsSecure";

                temporary.id = get_integer_value_from_profile(config,videodecoderidpath);
                temporary.supportsSecure = get_boolean_value_from_profile(config, supportsSecurepath);


                supportedcodec_count = get_list_Count_from_profile(config, supportedcodecpath);
                supportedDynamicRanges_count = get_list_Count_from_profile(config, supportedDynamicRangespath);

                for(int k=0; k<supportedDynamicRanges_count; ++k)
                {
                    std::string supportedDynamicRanges_path = supportedDynamicRangespath+"."+std::to_string(k);
                    std::string supportedDynamicRange = get_string_value_from_profile(config, supportedDynamicRanges_path);
                    com::rdk::hal::videodecoder::DynamicRange dynamicrange = StringToDynamicrange(supportedDynamicRange);
                    temporary.supportedDynamicRanges.push_back(dynamicrange);
                }

                for(int j=0; j<supportedcodec_count; ++j)
                {
                    std::string codecpath = supportedcodecpath+"."+std::to_string(j)+".codec";
                    std::string maxFrameRatepath = supportedcodecpath+"."+std::to_string(j)+".maxFrameRate";
                    std::string maxFrameWidthpath = supportedcodecpath+"."+std::to_string(j)+".maxFrameWidth";
                    std::string maxFrameHeightpath = supportedcodecpath+"."+std::to_string(j)+".maxFrameHeight";

                    std::string _codec = get_string_value_from_profile(config, codecpath);
                    temporary_codeccapability.codec = StringTocodec(_codec);
                    temporary_codeccapability.maxFrameRate = get_integer_value_from_profile(config, maxFrameRatepath);
                    temporary_codeccapability.maxFrameWidth = get_integer_value_from_profile(config, maxFrameWidthpath);
                    temporary_codeccapability.maxFrameHeight = get_integer_value_from_profile(config, maxFrameHeightpath);

                    temporary.supportedCodecs.push_back(temporary_codeccapability);                                
                }
                decoders.push_back(temporary);
                temporary.supportedDynamicRanges.clear();
                temporary.supportedCodecs.clear();
            }
            for(const auto& _decoder: decoders)
            {
                g_videodecoderId.value = _decoder.id;
                Status status = service->getVideoDecoder(g_videodecoderId, &_videoDecoder);
                ::testing::GTEST_FLAG(filter) = "L1VideoDecoderOpenTest.*:L1VideoDecoderStartTest.*:L1VideoDecoderCloseTest.*:L1VideoDecoderEventRegisterTest.*:L1VideoDecoderStopTest.*:L1VideoDecoderTest.*";
                result1 = RUN_ALL_TESTS();
            }
            ::testing::GTEST_FLAG(filter) = "L1VideoDecoderManagerTest.*";
            result2 = RUN_ALL_TESTS();
            return (result1 || result2);
        }
        catch (const YAML::BadFile& e) 
        {
            std::cerr << "Error: Could not open YAML file. " << e.what() << std::endl;
            return -1;
        } 
        catch (const YAML::BadConversion& e) 
        {
            std::cerr << "Error: Bad type conversion in YAML. " << e.what() << std::endl;
            return -1;
        }
        catch (const std::runtime_error& e) 
        {
            std::cerr << "Runtime Error: " << e.what() << std::endl;
            return -1;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return -1;
        }
    }
    else
    {
        std::cout << "USAGE : ./VideoDecoderClient -p <config_file>" <<std::endl;
        return -1;
    }

}
