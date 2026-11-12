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
    bool mBackPressureObserved = false;      // set by runSequentialDecode() in a back-pressure test
    bool mBackPressureNonTunnelRun = false;  // a back-pressure run used NON_TUNNELLED (back-pressure can be forced)

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

    /* Returns true if at least one stream was executed on a decoder (unsupported ones are skipped) */
    bool runSequentialDecode(std::vector<videoDecoderTestConfiguration_t>& testConfigurations,
                             bool secureDecode, bool concurrentDecodeConsume, bool backPressureTest = false)
    {
        bool isTestExecuted = false;

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            bool isSinkAttached = false;

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
                                                                    testConfig, mAVbufferHelper, isSinkAttached);
                    decodeTest->setBackPressureTest(backPressureTest);
                }
                else
                {
                    decodeTest->setTestConfiguration(testConfig);
                }

                AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

                decodeTest->run(concurrentDecodeConsume);

                decodeTest->closeDecoderSession();

                AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

                UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
                UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
                UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);

                isTestExecuted = true;

                if (backPressureTest)
                {
                    // Back-pressure runs once per decoder, on the first stream it supports
                    if (decodeTest->isBackPressureObserved())
                    {
                        mBackPressureObserved = true;
                    }
                    if (decodeTest->getOperationalMode() == OperationalMode::NON_TUNNELLED)
                    {
                        mBackPressureNonTunnelRun = true;
                    }
                    break;
                }
            }

            if (decodeTest)
            {
                decodeTest.reset();
            }
        }

        return isTestExecuted;
    }

    /* Returns true if at least one decoder executed the stream (unsupported ones are skipped) */
    bool runConcurrentDecode(videoDecoderTestConfiguration_t& testConfig, bool secureDecode, bool concurrentDecodeConsume)
    {
        std::vector<std::unique_ptr<videoDecoderTest>> decoderTests;
        std::vector<std::future<void>> futures;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;
        bool isSinkAttached = false;
        bool isTestExecuted = false;

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
                                                            mAVBufferManager, secureDecode, testConfig,mAVbufferHelper, isSinkAttached);
            futures.push_back(std::async(std::launch::async, &videoDecoderTest::run,
                                         decoderTest.get(), concurrentDecodeConsume));
            decoderTests.push_back(std::move(decoderTest));
            isTestExecuted = true;
        }

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

        return isTestExecuted;
    }

    /* rdk-halif-aidl 0.23.0: tunnel / non-tunnel is selected by the vendor; each test reads the mode after start()
     * and validates against it, so there is one test per scenario instead of a Tunnel / NonTunnel pair. */

    /* backPressureTest: send input until the decoder refuses a buffer and check onDecodeBufferAvailable(),
     * instead of decoding and validating the stream (sequential, one stream per decoder) */
    void runTest(const char *testName, bool secureMode, bool concurrentMode, bool backPressureTest = false)
    {
        vector<videoDecoderTestConfiguration_t> testConfiguration;

        UT_status_t status = vts_videodecoder_parse_testconfig(gVideoDecoderTestConfiguration, VIDEODECODERMGR, gTestGroup, testName, testConfiguration);
        if (status != UT_STATUS_OK || testConfiguration.empty())
        {
            UT_LOG_ERROR("Failed to parse test configuration for test: %s", testName);
            UT_ASSERT_TRUE(false) << "Test configuration parsing failed";
            return;
        }

        bool isTestExecuted = false;
        if (concurrentMode)
        {
            for (auto& test : testConfiguration)
            {
                if (runConcurrentDecode(test, secureMode, true))
                {
                    isTestExecuted = true;
                }
            }
        }
        else
        {
            mBackPressureObserved = false;
            mBackPressureNonTunnelRun = false;
            isTestExecuted = runSequentialDecode(testConfiguration, secureMode, true, backPressureTest);
        }

        /* Skip (not fail) when no decoder supports any configuration of this test, e.g. codec or secure not supported */
        if (!isTestExecuted)
        {
            UT_LOG_INFO("Skipping test '%s': No valid test configuration executed", testName);
            GTEST_SKIP() << "No valid test configuration executed";
        }

        if (backPressureTest && !mBackPressureObserved)
        {
            /* Back-pressure is forced by holding output frames, which is only possible in NON_TUNNELLED mode */
            if (!mBackPressureNonTunnelRun)
            {
                UT_LOG_INFO("Skipping test '%s': back-pressure cannot be forced in TUNNELLED mode", testName);
                GTEST_SKIP() << "Back-pressure cannot be forced in TUNNELLED mode";
            }
            UT_ASSERT_TRUE(mBackPressureObserved)
                << "Back-pressure was not observed on any decoder; onDecodeBufferAvailable semantics could not be validated";
        }
    }

    void TearDown() override
    {
        for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            VIDEODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
            delete capabilities;
        }
        mVideoDecoderList.clear();

        /* rdk-halif-aidl 0.23.0: release the AV Buffer helper instance taken in SetUp() */
        releaseAVBufferHelperInstance(mAVbufferHelper);
        mAVbufferHelper = nullptr;
    }
};

/**
 * @brief Test the ability to decode video frames in sequential decoding
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 001
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 002
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_CODEC)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 004
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_HDRModes)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 005
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_FrameRate)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 006
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_LowLatency)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test adaptive bitrate streaming: validates the resolution/bitrate ladder
 *        as the decoder switches mid-stream. Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 007
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}


/**
 * @brief Test decoding with GOP option
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 008
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_GOP)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_GOP";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 009
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 010
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush functionality
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 011
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Flush)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 012
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_FlushReset)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 013
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test error streams with discontinuity
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 014
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Error_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test error streams with EOS
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 015
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Error_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error_EOS";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test discontinuity with EOS
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 016
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Discontinuity_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Discontinuity_EOS";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush with error streams
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 017
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_Flush_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Flush_Error";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush with reset and error streams
 *        Decoding type: Sequential, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 018
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_FlushReset_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FlushReset_Error";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
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
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_MetadataOverride_HDR)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MetadataOverride_HDR";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test mid-stream colorimetry/content-light-level override via InputBufferMetadata
 *        simulating an SSAI ad-insertion splice point, verifying it persists across buffers.
 *
 * **Test Group ID:** 01 @n
 * **Test Case ID:** 020 @n
 * **Priority:** High @n
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_MetadataOverride_SSAI)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MetadataOverride_SSAI";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test that a pixel aspect ratio override via InputBufferMetadata is ignored
 *        when the bitstream itself signals PAR (this stream signals 1:1 SAR).
 *
 * **Test Group ID:** 01 @n
 * **Test Case ID:** 021 @n
 * **Priority:** Medium @n
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_NonSecure_MetadataOverride_PAR)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MetadataOverride_PAR";
    bool secureMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 022
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 023
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_CODEC)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 024
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 025
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_HDRModes)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 026
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_FrameRate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 027
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 028
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with discontinuity
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 029
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with EOS
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 030
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_EOS)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 031
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_Flush)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush and reset
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 032
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_FlushReset)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with error streams
 *        Decoding type: Concurrent, Modes: Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 033
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_NonSecure_Error)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 034
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 035
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_CODEC)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_CODEC";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 036
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_AdaptiveBitrate";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 037
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_HDRModes)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_HDRModes";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 038
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_FrameRate)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_FrameRate";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 039
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_LowLatency)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test secure mode with GOP
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 040
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_GOP)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_GOP";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 041
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 042
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_EOS";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush functionality in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 043
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_Flush)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Flush";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in secure mode
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 044
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_FlushReset)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_FlushReset";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test secure mode with error streams
 *        Decoding type: Sequential, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 045
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Sequential_Secure_Error)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in secure mode
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 046
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 047
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_CODEC)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_CODEC";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 048
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_AdaptiveBitrate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_AdaptiveBitrate";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 049
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_HDRModes)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_HDRModes";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 050
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_FrameRate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_FrameRate";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 051
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}


/**
 * @brief Test decoding with GOP structure in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 052
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_GOPStructure";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 053
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 054
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_EOS)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_EOS";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush functionality in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 055
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_Flush)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Flush";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 056
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_FlushReset)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_FlushReset";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Secure
 * **Test Group ID:** 01 **Test Case ID:** 057
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_Concurrent_Secure_Error)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test onDecodeBufferAvailable callback semantics under decoder input back-pressure.
 *        Verifies one callback is emitted when queue transitions from full to has-space.
 * **Test Group ID:** 01 **Test Case ID:** 095
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, VDecode_BackPressure_Callback)
{
    runTest("Decode_BackPressure_Callback", false, false, true);
}

/** @} */ // End of VIDEO_DECODER_HALTEST_L3 File
/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
