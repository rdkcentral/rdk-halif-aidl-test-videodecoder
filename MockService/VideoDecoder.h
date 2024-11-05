/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
* VideoDecoder.h
*
* Header file for the VideoDecoder service implementation
*/

#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <com/rdk/hal/videodecoder/BnVideoDecoderManager.h>
#include <com/rdk/hal/videodecoder/IVideoDecoderManager.h>
#include <com/rdk/hal/videodecoder/BnVideoDecoder.h>
#include <com/rdk/hal/videodecoder/BnVideoDecoderController.h>
#include <binder/BinderService.h>
#include <optional>
#include <string>
#include <utils/StrongPointer.h>
#include <vector>

namespace com {
    namespace rdk {
        namespace hal {
            namespace videodecoder {

                class VideoDecoderManager :public android::BinderService<VideoDecoderManager>,
                                           public BnVideoDecoderManager {
                public:
                    VideoDecoderManager();
                    virtual ~VideoDecoderManager();

                    static char const* getServiceName() { return IVideoDecoderManager::serviceName().c_str(); }

                    android::binder::Status getVideoDecoderIds( std::vector<IVideoDecoder::Id>* _aidl_return ) override;
                    android::binder::Status getSupportedOperationalModes( std::vector<OperationalMode>* _aidl_return ) override;
                    android::binder::Status getVideoDecoder( const IVideoDecoder::Id& videoDecoderId, android::sp<IVideoDecoder>* _aidl_return ) override;
                };

                class VideoDecoder :public android::BinderService<VideoDecoder>,
                                    public BnVideoDecoder {
                public:
                    VideoDecoder();
                    virtual ~VideoDecoder();

                    android::binder::Status getCapabilities( videodecoder::Capabilities* _aidl_return ) override;
                    android::binder::Status getProperty( videodecoder::Property property, std::optional<PropertyValue>* _aidl_return ) override;
                    android::binder::Status setProperty( videodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return ) override;
                    android::binder::Status getPropertyMulti( std::vector<videodecoder::PropertyKVPair>* propertyKVList, bool* _aidl_return ) override;
                    android::binder::Status getState( State* _aidl_return ) override;
                    android::binder::Status open( videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, android::sp<videodecoder::IVideoDecoderController>* _aidl_return ) override;
                    android::binder::Status close( const android::sp<videodecoder::IVideoDecoderController>& videoDecoderController, bool* _aidl_return ) override;
                    android::binder::Status registerEventListener( const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return ) override;
                    android::binder::Status unregisterEventListener( const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return ) override;
                };

                class VideoDecoderController :public android::BinderService<VideoDecoderController>,
                                              public BnVideoDecoderController {
                public:
                    VideoDecoderController();
                    virtual ~VideoDecoderController();

                    android::binder::Status start(  ) override;
                    android::binder::Status stop(  ) override;
                    android::binder::Status decodeBuffer( int64_t nsPresentationTime, int64_t bufferHandle ) override;
                    android::binder::Status flush( bool reset ) override;
                    android::binder::Status signalDiscontinuity(  ) override;
                    android::binder::Status signalEOS(  ) override;
                    android::binder::Status parseCodecSpecificData( rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return ) override;
                };
            }
        }
    }
}

#endif VIDEODECODER_H

