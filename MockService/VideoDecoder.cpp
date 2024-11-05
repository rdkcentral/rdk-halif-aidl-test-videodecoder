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
/** @brief
* VideoDecoder.cpp
*
* Implementation of VideoDecoder service
*/

#include "VideoDecoder.h"

namespace com {
    namespace rdk {
        namespace hal {
            namespace videodecoder {

                VideoDecoderManager::VideoDecoderManager()
                {
                    printf("VideoDecoderManager service initialized\n");
                }

                VideoDecoderManager::~VideoDecoderManager()
                {
                    printf("VideoDecoderManager service destroyed\n");
                }

                android::binder::Status VideoDecoderManager::getVideoDecoderIds(  std::vector<IVideoDecoder::Id>* _aidl_return  )
                {
                    IVideoDecoder::Id id1;
                    id1.value = 22; // Set value
                    IVideoDecoder::Id id2;
                    id2.value = -1; // Set value
                    _aidl_return->push_back(id1);
                    _aidl_return->push_back(id2);
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoderManager::getSupportedOperationalModes(  std::vector<OperationalMode>* _aidl_return  )
                {
//                    OperationalMode mSupportedModes = { OperationalMode::TUNNELLED, OperationalMode::NON_TUNNELLED };
                    _aidl_return->push_back(OperationalMode::TUNNELLED);
                    _aidl_return->push_back(OperationalMode::NON_TUNNELLED);
                    _aidl_return->push_back(OperationalMode::TUNNELLED);
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoderManager::getVideoDecoder(  const IVideoDecoder::Id& videoDecoderId, android::sp<IVideoDecoder>* _aidl_return  )
                {
                    android::sp<IVideoDecoder> videoDecoder = new VideoDecoder();
                    *_aidl_return = videoDecoder;
                    return android::binder::Status::ok();
                }

                VideoDecoder::VideoDecoder()
                {
                    printf("VideoDecoder service initialized\n");
                }

                VideoDecoder::~VideoDecoder()
                {
                    printf("VideoDecoder service destroyed\n");
                }

                android::binder::Status VideoDecoder::getCapabilities(  videodecoder::Capabilities* _aidl_return  )
                {
                    // Initialize the Capabilities object
                    Capabilities capabilities;

                    // Set supported codecs
                    CodecCapabilities codec1;
                    codec1.codec = Codec::MPEG;
                    codec1.maxFrameRate = 60;
                    codec1.maxFrameWidth = 1920;
                    codec1.maxFrameHeight = 1080;

                    CodecCapabilities codec2;
                    codec2.codec = Codec::H264;
                    codec2.maxFrameRate = 60;
                    codec2.maxFrameWidth = 1920;
                    codec2.maxFrameHeight = 1080;

                    CodecCapabilities codec3;
                    codec3.codec = Codec::H265;
                    codec3.maxFrameRate = 60;
                    codec3.maxFrameWidth = 3840;
                    codec3.maxFrameHeight = 2160;

		    CodecCapabilities codec4;
                    codec4.codec = Codec::VP9;
                    codec4.maxFrameRate = 60;
                    codec4.maxFrameWidth = 3840;
                    codec4.maxFrameHeight = 2160;

                    CodecCapabilities codec5;
                    codec5.codec = Codec::AV1;
                    codec5.maxFrameRate = 60;
                    codec5.maxFrameWidth = 3840;
                    codec5.maxFrameHeight = 2160;

                    capabilities.supportedCodecs.push_back(codec1);
                    capabilities.supportedCodecs.push_back(codec2);

                    capabilities.supportedCodecs.push_back(codec3);
                    capabilities.supportedCodecs.push_back(codec4);
                    capabilities.supportedCodecs.push_back(codec5);



                    // Set supported dynamic ranges
                    capabilities.supportedDynamicRanges.push_back(DynamicRange::SDR);
                    capabilities.supportedDynamicRanges.push_back(DynamicRange::HDR10);
                    capabilities.supportedDynamicRanges.push_back(DynamicRange::DOLBY_VISION);

                    // Indicate support for secure video
                    capabilities.supportsSecure = true;

                    // Assign the filled capabilities object to the return parameter
                    *_aidl_return = capabilities;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::getProperty(  videodecoder::Property property, std::optional<PropertyValue>* _aidl_return  )
                {
                    if (!_aidl_return)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    com::rdk::hal::PropertyValue value;
                    switch (property) {
                        case videodecoder::Property::RESOURCE_ID:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(22);
                            break;
                        case videodecoder::Property::INPUT_QUEUE_DEPTH:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::byteValue>(2332);
                            break;
                        case videodecoder::Property::AV_SOURCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* your AV source value here */ 0);
                            break;
                        case videodecoder::Property::OUTPUT_FRAME_POOL_SIZE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(1);
                            break;
                        case videodecoder::Property::OPERATIONAL_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(5);
                            break;
                        case videodecoder::Property::LOW_LATENCY_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(0);
                            break;
                        case videodecoder::Property::DECODE_ERROR_POLICY:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(0);
                            break;
                        case videodecoder::Property::SHA1_CALC:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(0);
                            break;
                        case videodecoder::Property::SECURE_VIDEO:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(1);
                            break;
                        default:
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = value;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::setProperty(  videodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::getPropertyMulti(  std::vector<videodecoder::PropertyKVPair>* propertyKVList, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::getState(  State* _aidl_return  )
                {
                    *_aidl_return = State::CLOSED;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::open(  videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, android::sp<videodecoder::IVideoDecoderController>* _aidl_return  )
                {
                    *_aidl_return = new VideoDecoderController();
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::close(  const android::sp<videodecoder::IVideoDecoderController>& videoDecoderController, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::registerEventListener(  const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::unregisterEventListener(  const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }
                
                VideoDecoderController::VideoDecoderController() 
                {
                    printf("VideoDecoderController service initialized\n");
                }
                
                VideoDecoderController::~VideoDecoderController() 
                {
                    printf("VideoDecoderController service destroyed\n");
                }
                
                android::binder::Status VideoDecoderController::start(    ) 
                {
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::stop(    ) 
                {
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::decodeBuffer(  int64_t nsPresentationTime, int64_t bufferHandle  ) 
                {
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::flush(  bool reset  ) 
                {
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::signalDiscontinuity(    ) 
                {
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::signalEOS(    ) 
                {
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::parseCodecSpecificData(  rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return  ) 
                {
                    return android::binder::Status::ok();
                }
            }
        }
    }
}



