/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 */

/**
 * @addtogroup VIDEO_DECODER Video Decoder Module
 * @{
 */

/**
 * @addtogroup VIDEO_DECODER_HALTEST Video Decoder HAL Tests
 * @{
 */

/**
 * @defgroup VIDEO_DECODER_HALTEST_L3 Video Decoder HAL Tests L3 File
 * @{
 * @parblock
 *
 * ### L3 Test Cases for Video Decoder HAL :
 *
 * This module includes Level 3 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 *
 * Refer to UT specification documentation [videoDecoder_High-Level_TestSpecification.md]
 *
 * @endparblock
 */

/**
 * @file vts_videodecoder_l3_test.cpp
 *
 * @brief Contains L3 test cases for the Video Decoder HAL.
 */

#include "vts_videodecoder.h"

#include <vector>
#include <future>
#include <memory>

using namespace std;

int32_t VideoDecoderEventListener::mUsedDecoderID = -1;

UT_ADD_TEST_TO_GROUP(VTS_L3_VideoDecoder, UT_TESTS_L3)

static const char *gTestGroup = "L3";

static sp<IAVBuffer> vts_getAVBufferService()
{
    return
        android::interface_cast<IAVBuffer>(
            android::defaultServiceManager()->getService(
                android::String16(IAVBuffer::serviceName().c_str())
            )
        );
}

class VTS_L3_VideoDecoder : public UTCore
{
public:
    VTS_L3_VideoDecoder() : UTCore(), mAVbufferHelper(nullptr) {}

    ~VTS_L3_VideoDecoder() override = default;

    vector<tuple<IVideoDecoder::Id, sp<IVideoDecoder>, sp<VideoDecoderControllerListener>, sp<VideoDecoderEventListener>, videodecoder::Capabilities *>> mVideoDecoderList;
    sp<IAVBuffer> mAVBufferManager;
    IAVBufferHelper* mAVbufferHelper;

    void SetUp() override
    {
        mAVBufferManager = vts_getAVBufferService();

        if (mAVBufferManager == nullptr) {
            UT_FAIL_FATAL("AVBuffer Service not available");
        }

        if (gVideoDecoderManager == nullptr) {
            UT_FAIL_FATAL("VideoDecoder Service not available");
        }
        vector<IVideoDecoder::Id> decoderIds;

        // Parse HFP to populate gVideodecoders_hfp
        UT_status_t status = vts_videodecoder_parseVideoDecoderHFP();
        if (status != UT_STATUS_OK) {
            UT_LOG_ERROR("Failed to parse video decoder HFP");
            UT_ASSERT_TRUE(false) << "HFP parsing failed";
            return;
        }

        if (gVideodecoders_hfp.empty()) {
            UT_LOG_ERROR("No valid decoders found in HFP");
            UT_ASSERT_TRUE(false) << "No decoders in HFP";
            return;
        }

        mAVbufferHelper = getAVBufferHelperInstance();
        if (mAVbufferHelper == nullptr) {
            UT_FAIL_FATAL("AV Buffer Helper not available");
        }

        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

        android::ProcessState::self()->startThreadPool();

        // Filter decoder IDs based on gVideodecoders_hfp
        for (const auto& _decoderId : decoderIds)
        {
            int decoderIdValue = _decoderId.value; // Assuming IVideoDecoder::Id has a 'value' field
            auto it = std::find_if(gVideodecoders_hfp.begin(), gVideodecoders_hfp.end(),
                [decoderIdValue](const decoder& item) { return item.id == decoderIdValue; });
            if (it == gVideodecoders_hfp.end()) {
                UT_LOG_WARNING("Decoder ID %d found in system but not in HFP, skipping", decoderIdValue);
                continue;
            }
            sp<IVideoDecoder> videoDecoder;
            std::unique_ptr<videodecoder::Capabilities> capabilitiesOwner(new videodecoder::Capabilities());
            videodecoder::Capabilities *capabilities = capabilitiesOwner.get();
            sp<VideoDecoderEventListener> eventListener = new VideoDecoderEventListener();
            sp<VideoDecoderControllerListener> controlListener = new VideoDecoderControllerListener();

            VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

            VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

            VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities);

            mVideoDecoderList.push_back({_decoderId, videoDecoder, controlListener, eventListener, capabilitiesOwner.release()});
            UT_LOG_INFO("Added decoder ID %d to test list", decoderIdValue);
        }
        if (mVideoDecoderList.empty()) {
            UT_LOG_ERROR("No valid decoders found matching HFP configuration");
            UT_ASSERT_TRUE(false) << "No valid decoders available";
        }
    }

    bool isTunnelModeSupported() const
    {
        // Check if any decoder supports tunnel mode by querying the manager
        std::vector<OperationalMode> supportedModes;
        android::binder::Status status = gVideoDecoderManager->getSupportedOperationalModes(&supportedModes);

        if (!status.isOk()) {
            UT_ASSERT_TRUE(status.isOk())
                 << "Failed to query supported operational modes: " << status.exceptionMessage().c_str();
            return false;
        }

        for (const auto& mode : supportedModes) {
            if (mode == OperationalMode::TUNNELLED) {
                return true;
            }
        }
        return false;
    }

    bool isNonTunnelModeSupported() const
    {
        // Check if any decoder supports non-tunnel mode by querying the manager
        std::vector<OperationalMode> supportedModes;
        android::binder::Status status = gVideoDecoderManager->getSupportedOperationalModes(&supportedModes);

        if (!status.isOk()) {
            UT_ASSERT_TRUE(status.isOk())
                 << "Failed to query supported operational modes: " << status.exceptionMessage().c_str();
            return false;
        }

        for (const auto& mode : supportedModes) {
            if (mode == OperationalMode::NON_TUNNELLED) {
                return true;
            }
        }
        return false;
    }

    bool isSecureModeSupported() const
    {
        // Check if any decoder supports secure decoding
        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            if (capabilities && capabilities->supportsSecure) {
                return true;
            }
        }
        return false;
    }

    void runSequentialDecode(std::vector<videoDecoderTestConfiguration_t>& testConfigurations,
                             bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        int32_t activeDecoders = 0;

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            bool isSinkAttached = false;
            videodecoder::Codec activeCodec = static_cast<videodecoder::Codec>(0);
            bool streamExecutedOnDecoder = false;

            std::unique_ptr<videoDecoderTest> decodeTest;

            for (size_t i = 0; i < testConfigurations.size(); ++i)
            {
                auto& testConfig = testConfigurations[i];
                HeapMetrics beforeHeapMetrics, afterHeapMetrics;

                if (!checkDecoderSupportsConfig(_decoderId, capabilities, testConfig, secureDecode))
                {
                    UT_LOG_WARNING("Decoder [%d] doesn't support the required configuration for stream [%s]",
                                   _decoderId.value, testConfig.streamName.c_str());
                    continue;
                }

                // Reset per-stream so EOS/decode-buffer-available counts and any queued
                // frames from the previous stream config don't bleed into this one.
                eventListener->resetQueues();
                controlListener->resetQueues();

                if (!decodeTest)
                {
                    decodeTest = std::make_unique<videoDecoderTest>(_decoderId, decoderHandle, controlListener,
                                                                    eventListener, mAVBufferManager, secureDecode,
                                                                    tunnelMode, testConfig, mAVbufferHelper, isSinkAttached);
                }
                else
                {
                    decodeTest->setTestConfiguration(testConfig);
                }

                const bool codecChanged = (activeCodec != static_cast<videodecoder::Codec>(0)) && (activeCodec != testConfig.codec);
                if (codecChanged)
                {
                    decodeTest->closeDecoderSession();
                }

                AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

                decodeTest->run(concurrentDecodeConsume);

                decodeTest->closeDecoderSession();

                AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

                UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
                UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
                UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);

                activeCodec = testConfig.codec;
                streamExecutedOnDecoder = true;
            }

            if (decodeTest)
            {
                decodeTest.reset();
            }

            if (streamExecutedOnDecoder)
            {
                activeDecoders++;
            }
        }

        UT_ASSERT_TRUE(activeDecoders > 0) << "No decoders support the requested configuration";
    }

    void runConcurrentDecode(videoDecoderTestConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        std::vector<std::unique_ptr<videoDecoderTest>> decoderTests;
        std::vector<std::future<void>> futures;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;
        bool isSinkAttached = false;
        int32_t activeDecoders = 0;

        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

        decoderTests.reserve(mVideoDecoderList.size());
        futures.reserve(mVideoDecoderList.size());

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            eventListener->resetQueues();
            controlListener->resetQueues();

            if (!checkDecoderSupportsConfig(_decoderId, capabilities, testConfig, secureDecode))
            {
                UT_LOG_WARNING("[DecoderID: %d] Doesn't support the required configuration", _decoderId.value);
                continue;
            }

            auto decoderTest = std::make_unique<videoDecoderTest>(_decoderId, decoderHandle, controlListener, eventListener,
                                                            mAVBufferManager, secureDecode, tunnelMode, testConfig,mAVbufferHelper, isSinkAttached);
            futures.push_back(std::async(std::launch::async, &videoDecoderTest::run,
                                         decoderTest.get(), concurrentDecodeConsume));
            decoderTests.push_back(std::move(decoderTest));
            activeDecoders++;
        }

        UT_ASSERT_TRUE(activeDecoders > 0) << "No decoders support the requested configuration";

        for (auto& f : futures)
        {
            try {
                f.get();
            } catch (const std::exception& e) {
                UT_LOG_ERROR("Exception in decoder thread: %s", e.what());
                UT_ASSERT_TRUE(false) << "Decoder thread failed";
            }
        }

        for (auto& decoderTest : decoderTests)
        {
            decoderTest->closeDecoderSession();
        }

        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

        UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
        UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
        UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);
    }

    void runTest(const char *testName, bool secureMode, bool tunnelMode, bool concurrentMode)
    {
        // Check if required capabilities are supported before running test
        if (tunnelMode && !isTunnelModeSupported()) {
            UT_LOG_INFO("Skipping test '%s': Tunnel mode is not supported by this system", testName);
            GTEST_SKIP() << "Tunnel mode is not supported by this system";
            return;
        }

        if (!tunnelMode && !isNonTunnelModeSupported()) {
            UT_LOG_INFO("Skipping test '%s': Non-tunnel mode is not supported by this system", testName);
            GTEST_SKIP() << "Non-tunnel mode is not supported by this system";
            return;
        }

        if (secureMode && !isSecureModeSupported()) {
            UT_LOG_INFO("Skipping test '%s': Secure mode is not supported by any decoder", testName);
            GTEST_SKIP() << "Secure mode is not supported by any decoder";
            return;
        }

        if (strcmp(testName, "Decode_BackPressure_Callback") == 0)
        {
            runBackPressureDecode();
            return;
        }

        vector<videoDecoderTestConfiguration_t> testConfiguration;

        UT_status_t status = vts_videodecoder_parse_testconfig(gVideoDecoderTestConfiguration, VIDEODECODERMGR, gTestGroup, testName, testConfiguration);
        if (status != UT_STATUS_OK || testConfiguration.empty())
        {
            UT_LOG_ERROR("Failed to parse test configuration for test: %s", testName);
            UT_ASSERT_TRUE(false) << "Test configuration parsing failed";
            return;
        }

        if (concurrentMode)
        {
            for (auto& test : testConfiguration)
            {
                runConcurrentDecode(test, secureMode, tunnelMode, true);
            }
        }
        else
        {
            runSequentialDecode(testConfiguration, secureMode, tunnelMode, true);
        }
    }

    void runBackPressureDecode()
    {
        int activeDecoders = 0;
        int decodersWithBackPressure = 0;

        // Use a real stream so the decoder actually queues the input instead of rejecting it.
        vector<videoDecoderTestConfiguration_t> testConfiguration;
        UT_status_t status = vts_videodecoder_parse_testconfig(gVideoDecoderTestConfiguration, VIDEODECODERMGR, gTestGroup,
                                                               "Decode_BackPressure_Callback", testConfiguration);
        if (status != UT_STATUS_OK || testConfiguration.empty())
        {
            UT_ASSERT_TRUE(false) << "Test configuration parsing failed for Decode_BackPressure_Callback";
            return;
        }

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            if (!decoderHandle || !capabilities || capabilities->supportedCodecs.empty())
            {
                UT_LOG_WARNING("[DecoderID: %d] Missing decoder handle/capability data, skipping callback test", _decoderId.value);
                continue;
            }

            auto config = std::find_if(testConfiguration.begin(), testConfiguration.end(),
                                       [&](const videoDecoderTestConfiguration_t& c)
                                       { return checkDecoderSupportsConfig(_decoderId, capabilities, c, false); });
            if (config == testConfiguration.end())
            {
                UT_LOG_WARNING("[DecoderID: %d] No supported stream for callback test, skipping", _decoderId.value);
                continue;
            }

            activeDecoders++;

            videoDecoderTest decodeTest(_decoderId, decoderHandle, controlListener,
                                        eventListener, mAVBufferManager, false,
                                        false, *config, mAVbufferHelper, false);

            if (decodeTest.runBackPressureTest())
            {
                decodersWithBackPressure++;
            }
        }

        UT_ASSERT_TRUE(activeDecoders > 0) << "No active decoders available for callback test";
        UT_ASSERT_TRUE(decodersWithBackPressure > 0)
            << "Back-pressure was not observed on any decoder; onDecodeBufferAvailable semantics could not be validated";
    }

    void TearDown() override
    {
        for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            VIDEODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
            delete capabilities;
        }
        mVideoDecoderList.clear();
    }
};

/**
 * @brief Test the ability to decode video frames in sequential decoding
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 001
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 002
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 004
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 005
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 006
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test adaptive bitrate streaming: validates the resolution/bitrate ladder
 *        as the decoder switches mid-stream. Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 007
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}


/**
 * @brief Test decoding with GOP option
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 008
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_GOP)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_GOP";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 009
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 010
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 011
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 012
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 013
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test error streams with discontinuity
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 014
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Error_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test error streams with EOS
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 015
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Error_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test discontinuity with EOS
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 016
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Discontinuity_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Discontinuity_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 017
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_Flush_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Flush_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset and error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 018
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_FlushReset_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FlushReset_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test mid-stream colorimetry/HDR override via InputBufferMetadata (e.g. an
 *        ABR period transition from SDR to HDR10), verifying FrameMetadata reflects
 *        the new values from the override PTS onward.
 *
 * **Test Group ID:** 01 @n
 * **Test Case ID:** 019 @n
 * **Priority:** High @n
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_MetadataOverride_HDR)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MetadataOverride_HDR";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test mid-stream colorimetry/content-light-level override via InputBufferMetadata
 *        simulating an SSAI ad-insertion splice point, verifying it persists across buffers.
 *
 * **Test Group ID:** 01 @n
 * **Test Case ID:** 020 @n
 * **Priority:** High @n
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_MetadataOverride_SSAI)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MetadataOverride_SSAI";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test that a pixel aspect ratio override via InputBufferMetadata is ignored
 *        when the bitstream itself signals PAR (this stream signals 1:1 SAR).
 *
 * **Test Group ID:** 01 @n
 * **Test Case ID:** 021 @n
 * **Priority:** Medium @n
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_NonSecure_MetadataOverride_PAR)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MetadataOverride_PAR";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 022
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 023
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 024
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 025
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 026
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 027
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 028
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with discontinuity
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 029
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with EOS
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 030
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 031
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush and reset
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 032
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with error streams
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 033
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 034
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 035
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_CODEC)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_CODEC";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 036
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_AdaptiveBitrate";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 037
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_HDRModes)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_HDRModes";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 038
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_FrameRate)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_FrameRate";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 039
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test secure mode with GOP
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 040
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_GOP)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_GOP";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 041
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 042
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_EOS";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 043
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Flush";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 044
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_FlushReset";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test secure mode with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 045
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in secure mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 046
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 047
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_CODEC)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_CODEC";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 048
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_AdaptiveBitrate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_AdaptiveBitrate";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 049
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_HDRModes)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_HDRModes";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 050
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_FrameRate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_FrameRate";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 051
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}


/**
 * @brief Test decoding with GOP structure in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 052
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_GOPStructure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 053
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 054
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_EOS";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 055
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Flush";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 056
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_FlushReset";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 057
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 058
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 059
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 060
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 061
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 062
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 063
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 064
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 065
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 066
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 067
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 068
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel mode with error streams
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 069
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in tunnel mode
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 070
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 071
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 072
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 073
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 074
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 075
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 076
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 077
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 078
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 079
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 080
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 081
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel and secure modes together
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 082
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 083
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_CODEC)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_CODEC";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 084
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_AdaptiveBitrate";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 085
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_HDRModes)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_HDRModes";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 086
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_FrameRate)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_FrameRate";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 087
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 088
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_GOPStructure)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_GOPStructure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 089
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 090
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_EOS)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_EOS";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 091
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_Flush)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Flush";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 092
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_FlushReset)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_FlushReset";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 093
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Tunnel_Secure_Error)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in tunnel and secure modes
 *        Decoding type: Concurrent, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 094
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Tunnel_Secure)
{
    const char *testName = "Decode_Concurrent_Tunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test onDecodeBufferAvailable callback semantics under decoder input back-pressure.
 *        Verifies one callback is emitted when queue transitions from full to has-space.
 * **Test Group ID:** 01 **Test Case ID:** 095
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_BackPressure_Callback)
{
    runTest("Decode_BackPressure_Callback", false, false, false);
}

/** @} */ // End of VIDEO_DECODER_HALTEST_L3 File
/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
