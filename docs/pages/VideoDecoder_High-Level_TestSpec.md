# VideoDecoder High Level Test Specification Document

## Table of Contents

- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [Introduction](#introduction)
- [Module Description](#module-description)
- [Test Stream specifications](#test-stream-specifications)
- [L2 Test Specification](#l2-test-specification)
- [L3 Test Specification](#l3-test-specification)

## Acronyms, Terms and Abbreviations

- `HAL`    - Hardware Abstraction Layer
- `API`    - Application Programming Interface
- `AIDL`   - Android Interface Definition Language
- `Caller` - Any user of the interface via the APIs
- `NA`     - Not Applicable
- `L2`     - Level 2 Testing
- `L3`     - Level 3 Testing
- `CB`     - Call-back function (suffix)
- `HDR`    - High Dynamic Range
- `SDR`    - Standard Dynamic Range
- `HEVC`   - High Efficiency Video Coding
- `RDK`    - Reference Design Kit
- `EOS`    - End of Stream
- `CSD`    - Codec Specific Data
- `NA`     - Not Applicable
- `Y`      - Yes

## Introduction

This document provides an overview of the testing requirements for the Video Decoder module. It outlines the scope of testing, objectives, testing levels and approaches, specific test requirements.


- `HAL` Interface Header [Interface Header](https://github.com/rdkcentral/rdk-halif-aidl/tree/v0.7/videodecoder/current/com/rdk/hal/videodecoder)
- `HAL` specification documentation in this link - [Video Decoder HAL Specification info](https://rdkcentral.github.io/rdk-halif-aidl/halif/video_decoder/current/video_decoder/)


## Module Description

The Video Decoder `HAL` service provides the interfaces for compressed video to be passed to the vendor layer for decoding.  It may be passed secure buffers if it indicates support for secure video processing.

The output of an video decoder can take two routes:

1. **Non-tunnel mode** - Decoded video frames with metadata are sent back to the `RDK` media pipeline.
2. **Tunnel mode** - Video is passed directly to the vendor layer mixer.

## Test Stream specifications

- List of video test streams: [Video TestStreams](VideoDecoder_TestStreamsInfo.md)

- Each Test should refer [vts_l3_test_config.yaml](../../test_configuration/vts_l3_test_config.yaml) for the streams and information like `PTS`, frame sizes for the test.

### L2 Test Specification

|#|Test |Description|HAL APIs|
|-|-----|-----------|--------|
|1|Set and Get video Decoder Properties| - Get Supported Video Decoders `IVideoDecoderManager` class by calling `getVideoDecoderIds()`. <br> - Verify that the returned array of `IVideoDecoder.Id` objects is not null and contains valid IDs. <br> - Loop through each supported video decoder ID. <br> - Get Video Decoder Interface using `getVideoDecoder(videoDecoderId)`. <br> - Verify that the returned `IVideoDecoder` object is not null. <br> - Set and Verify Read-Write Properties: <br> &emsp; - For each read-write property in the `Property` enum <br> &emsp; - Set the property using `setProperty(property, propertyValue)`. <br> &emsp; - Retrieve the property value using `getProperty(property)`. <br> &emsp; - Verify that the retrieved value matches the set value.<br> &emsp; - Close the decoder using `close()`. <br>  | - `IVideoDecoderManager:` getVideoDecoderIds, getVideoDecoder <br> - `IVideoDecoder:` open, getProperty, close <br> - `IVideoDecoderController:`  setProperty |
|2|Verify Set and Get with multiple video Decoder Properties| - Get Supported Video Decoders `IVideoDecoderManager` class by calling `getVideoDecoderIds()`. <br> - Verify that the returned array of `IVideoDecoder.Id` objects is not null and contains valid IDs. <br> - Check how many decoders are supported by counting the elements in the array<br > - Loop through each supported video decoder ID. <br> - Get Video Decoder Interface using `getVideoDecoder(videoDecoderId)`. <br> - Verify that the returned `IVideoDecoder` object is not null. <br> - Open and Set Properties for Multiple Decoders: <br> &emsp; -  For each supported video decoder ID:<br> &emsp;- Open the decoder using open().<br> &emsp;- For each read-write property in the `Property` enum <br> &emsp; - Set the property using `setProperty(property, propertyValue)`. <br> &emsp; - Retrieve the property value using `getProperty(property)`. <br> &emsp; - Verify that the retrieved value matches the set value.<br> -  Close the decoder using `close()`. <br> | - `IVideoDecoderManager:` getVideoDecoderIds, getVideoDecoder <br> - `IVideoDecoder:` open, getProperty, close <br> - `IVideoDecoderController:`  setProperty |
|3|Verify Decoder State Changes|- Get Supported Video Decoders using the `IVideoDecoderManager` class by calling `getVideoDecoderIds()`. <br> - Verify that the returned array of `IVideoDecoder.Id` objects is not null and contains valid IDs. <br> - Loop through each supported video decoder ID: <br>&emsp; - Get the Video Decoder Interface using `getVideoDecoder(videoDecoderId)`. <br> &emsp; - Verify that the returned `IVideoDecoder` object is not null. <br> &emsp; - Register event listeners using `registerEventListener`. <br> &emsp; - Open the decoder using `open()`. <br> &emsp; - Ensure each state(i.e. in the enum `State`) transitions(`open -> start -> flush -> stop -> close`) work as expected using `getState()`. <br> &emsp; - Validate event listener notifications using `onStateChanged`. <br>  - Close the decoder using `close()`. <br> | - `IVideoDecoderManager:` getVideoDecoderIds, getVideoDecoder <br> - `IVideoDecoder:` open, getState, registerEventListener, unregisterEventListener, close <br> - `IVideoDecoderController` - start, stop, flush <br> |
|4|Verify Decoder State Changes with multiple video Decoder |- Get Supported Video Decoders using the `IVideoDecoderManager` class by calling `getVideoDecoderIds()`. <br> - Verify that the returned array of `IVideoDecoder.Id` objects is not null and contains valid IDs. <br> - Loop through each supported video decoder ID: <br>&emsp; - Get the Video Decoder Interface using `getVideoDecoder(videoDecoderId)`. <br> &emsp; - Verify that the returned `IVideoDecoder` object is not null. <br> &emsp; - Register event listeners using `registerEventListener`. <br> &emsp; - If platform supports multiple decoder, open all the instances of the decoders using `open()`. <br> &emsp; - Ensure each state(i.e. in the enum `State`) transitions(`open -> start -> flush -> stop -> close`) work as expected using `getState()`. <br> &emsp; - Validate event listener notifications using `onStateChanged`. <br>  - Close the decoder using `close()`. <br> | - `IVideoDecoderManager:` getVideoDecoderIds, getVideoDecoder <br> - `IVideoDecoder:` open, getState, registerEventListener, unregisterEventListener, close <br> - `IVideoDecoderController` - start, stop, flush <br> |

### L3 Test Specification
### Prerequisites

#### AV Buffer

Video frames are supplied to the Video Decoder during testing, which necessitates a pre-validated AVBuffer module. Therefore, the AVBuffer must successfully pass VTS L1, L2, and L3 testing before being used in the Video Decoder tests

### Feature List

VTS L3 validates all the Video Decoder features in both sequential and concurrent decoding modes (if the platform supports multiple decoders) as represented below.

#### Sequential Decoding

The Sequential Decoding test evaluates each video decoder individually, one after another, to ensure functional correctness across all supported features. This approach helps isolate issues and verify that every decoder instance performs as expected in a standalone environment.

```mermaid
block-beta
    block:1
        VideoStream("Video Stream") space AVBuffer("AVBuffer") space Decoder("Decoder_1") space Output("Output")
        VideoStream --> AVBuffer
        AVBuffer --> Decoder
        Decoder --> Output
    end
```

#### Concurrent Decoding

The Concurrent Decoding test assesses the system's ability to handle multiple video decoders operating simultaneously, each configured with different supported features. This test is designed to validate the robustness, resource management, and feature compatibility of the platform under concurrent load, and is applicable only if the platform supports multiple decoder instances.

```mermaid
block-beta
columns 1
    block:1
        VideoStream_1("Video Stream") space AVBuffer_1("AVBuffer") space Decoder_1("Decoder_1") space Output_1("Output")
        VideoStream_1 --> AVBuffer_1
        AVBuffer_1 --> Decoder_1
        Decoder_1 --> Output_1
    end
    space
    block:2
        VideoStream_2("Video Stream") space AVBuffer_2("AVBuffer") space Decoder_2("Decoder_2") space Output_2("Output")
        VideoStream_2 --> AVBuffer_2
        AVBuffer_2 --> Decoder_2
        Decoder_2 --> Output_2
    end
    space
    block:N
        VideoStream_N("Video Stream") space AVBuffer_N("AVBuffer") space Decoder_N("Decoder_N") space Output_N("Output")
        VideoStream_N --> AVBuffer_N
        AVBuffer_N --> Decoder_N
        Decoder_N --> Output_N
    end
```

The following table outlines the core features validated in the L3 test phase

|#|Testing Feature|Description|
|-|---------------|-----------|
|1|Tunnel/Non-Tunnel|Executes the decoder in tunnel mode or in non tunnel mode based on the property OPERATION_MODE set by the user. The test validates metrics, metadata and bufferHandle.|
|2|Secure and Non-Secure|Initializes the decoder in secure mode, ensuring all buffer operations occur in secure memory|
|3|Error Streams|Verifies decoder behavior with error streams. Ensures appropriate events are raised and frame metadata is updated accurately|
|4|Codecs|Tests the decoder’s ability to decode all supported codec types per its capabilities, using input streams defined in the configuration file.|
|5|Discontinuity|This test Verifies that the discontinuity signal is processed, ensuring subsequent frames decode without errors and metadata reflects the discontinuity.|
|6|EOS|Tests the decoder’s handling of an End-of-Stream (EOS) signal, ensuring all frames up to EOS are decoded, metadata is updated, post-EOS buffers are ignored, and decoding resumes after restart.|
|7|Flush without reset|Test the decoder's flush functionality. The decoder should discard any buffers already queued and resume normal decoding of subsequent frames|
|8|Flush with reset|Test the decoder’s flush functionality combined with reset. The decoder should discard all queued buffers and transition to the READY state. After restarting, the decoder should properly process incoming frames|
|9| Resolution| Tests the decoder’s ability to decode streams with different resolutions (up to maxFrameWidth and maxFrameHeight per CodecCapability) as defined in the configuration file, validated via frame metadata|
|10| HDR Modes |Tests the decoder’s ability to decode all supported HDR modes per its capabilities, using input streams defined in the configuration file.|
|11| Frame Rates | Tests the decoder’s capability for supported frame rates (up to maxFrameRate per CodecCapability) and checks streams with different frame rates as defined in the configuration file, validated via frame metadata|
|12| GOP Structure |Tests the decoder’s ability to decode various GOP structures, including single I-frame or IDR frame, single closed GOPs, multiple open GOPs, and mixed open/closed GOPs, as defined in the configuration file.|
|13| Low Latency mode | Tests the decoder’s low latency mode, ensuring B-frames are skipped, as required for low-latency applications.|
|14| Adaptive Bitrate Streaming | Tests the decoder’s ability to decode Adaptive Bitrate Streams (ABR) with dynamic bitrate changes, as defined in the configuration fileTests the decoder’s ability to decode Adaptive Bitrate Streams (ABR) with dynamic bitrate changes, as defined in the configuration file.|
|15|Concurrent decoding|Tests multiple decoders simultaneously, if supported, validating decoded outputs and their respective metrics (e.g., frame timing, error rates).|

|#|Test|Tunnel|Secure|Error streams|CODEC|Discontinuity|EOS|Flush|Flush+Rest|Resolutions|HDR Modes|Frame Rates|GOP|Low Latency|Concurrency|
|-|----|------|------|-------------|----|-------------|---|-----|----------|-----------|---------|-----------|---|-----------|-----------|
|1|VDecode_Sequential_NonSecure|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|2|VDecode_Sequential_NonSecure_Error|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|3|VDecode_Sequential_NonSecure_CODEC|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|4|VDecode_Sequential_NonSecure_Discontinuity|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|5|VDecode_Sequential_NonSecure_EOS|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|6|VDecode_Sequential_NonSecure_Flush|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|
|7|VDecode_Sequential_NonSecure_FlushReset|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|
|8|VDecode_Sequential_NonSecure_Resolutions|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|9|VDecode_Sequential_NonSecure_HDRModes|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|10|VDecode_Sequential_NonSecure_FrameRate|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|11|VDecode_Sequential_NonSecure_GOPStructure|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|12|VDecode_Sequential_NonSecure_LowLatency|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|13|VDecode_Sequential_Secure|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|14|VDecode_Sequential_Secure_Error|✘|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|15|VDecode_Sequential_Secure_CODEC|✘|✔|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|16|VDecode_Sequential_Secure_Discontinuity|✘|✔|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|17|VDecode_Sequential_Secure_EOS|✘|✔|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|18|VDecode_Sequential_Secure_Flush|✘|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|
|19|VDecode_Sequential_Secure_FlushReset|✘|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|
|20|VDecode_Sequential_Secure_Resolutions|✘|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|21|VDecode_Sequential_Secure_HDRModes|✘|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|22|VDecode_Sequential_Secure_FrameRate|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|23|VDecode_Sequential_Secure_GOPStructure|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|24|VDecode_Sequential_Secure_LowLatency|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|25|VDecode_Sequential_Tunnel_NonSecure|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|26|VDecode_Sequential_Tunnel_NonSecure_Error|✔|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|27|VDecode_Sequential_Tunnel_NonSecure_CODEC|✔|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|28|VDecode_Sequential_Tunnel_NonSecure_Discontinuity|✔|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|29|VDecode_Sequential_Tunnel_NonSecure_EOS|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|30|VDecode_Sequential_Tunnel_NonSecure_Flush|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|
|31|VDecode_Sequential_Tunnel_NonSecure_FlushReset|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|
|32|VDecode_Sequential_Tunnel_NonSecure_Resolutions|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|33|VDecode_Sequential_Tunnel_NonSecure_HDRModes|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|34|VDecode_Sequential_Tunnel_NonSecure_FrameRate|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|35|VDecode_Sequential_Tunnel_NonSecure_GOPStructure|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|36|VDecode_Sequential_Tunnel_NonSecure_LowLatency|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|37|VDecode_Sequential_Tunnel_Secure|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|38|VDecode_Sequential_Tunnel_Secure_Error|✔|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|39|VDecode_Sequential_Tunnel_Secure_CODEC|✔|✔|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|40|VDecode_Sequential_Tunnel_Secure_Discontinuity|✔|✔|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|41|VDecode_Sequential_Tunnel_Secure_EOS|✔|✔|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|42|VDecode_Sequential_Tunnel_Secure_Flush|✔|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|
|43|VDecode_Sequential_Tunnel_Secure_FlushReset|✔|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|
|44|VDecode_Sequential_Tunnel_Secure_Resolutions|✔|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|45|VDecode_Sequential_Tunnel_Secure_HDRModes|✔|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|46|VDecode_Sequential_Tunnel_Secure_FrameRate|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|47|VDecode_Sequential_Tunnel_Secure_GOPStructure|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|48|VDecode_Sequential_Tunnel_Secure_LowLatency|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|49|VDecode_Concurrent_NonSecure|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|50|VDecode_Concurrent_NonSecure_Error|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|
|51|VDecode_Concurrent_NonSecure_CODEC|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|
|52|VDecode_Concurrent_NonSecure_Discontinuity|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|53|VDecode_Concurrent_NonSecure_EOS|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✘|✔|
|54|VDecode_Concurrent_NonSecure_Flush|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✘|✔|
|55|VDecode_Concurrent_NonSecure_FlushReset|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✘|✔|
|56|VDecode_Concurrent_NonSecure_Resolutions|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|57|VDecode_Concurrent_NonSecure_HDRModes|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|58|VDecode_Concurrent_NonSecure_FrameRate|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|59|VDecode_Concurrent_NonSecure_GOPStructure|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✔
|60|VDecode_Concurrent_NonSecure_LowLatency|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|61|VDecode_Concurrent_Secure|✘|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|62|VDecode_Concurrent_Secure_Error|✘|✔|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|63|VDecode_Concurrent_Secure_CODEC|✘|✔|✘|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|64|VDecode_Concurrent_Secure_Discontinuity|✘|✔|✘|✘|✔|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|65|VDecode_Concurrent_Secure_EOS|✘|✔|✘|✘|✘|✔|✘|✘|✔|✘|✘|✘|✘|✔|
|66|VDecode_Concurrent_Secure_Flush|✘|✔|✘|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|✔|
|67|VDecode_Concurrent_Secure_FlushReset|✘|✔|✘|✘|✘|✘|✘|✔|✔|✘|✘|✘|✘|✔|
|68|VDecode_Concurrent_Secure_Resolutions|✘|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|69|VDecode_Concurrent_Secure_HDRModes|✘|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|70|VDecode_Concurrent_Secure_FrameRate|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|71|VDecode_Concurrent_Secure_GOPStructure|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✔
|72|VDecode_Concurrent_Secure_LowLatency|✘|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|73|VDecode_Concurrent_Tunnel_NonSecure|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|74|VDecode_Concurrent_Tunnel_NonSecure_Error|✔|✘|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|75|VDecode_Concurrent_Tunnel_NonSecure_CODEC|✔|✘|✘|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|76|VDecode_Concurrent_Tunnel_NonSecure_Discontinuity|✔|✘|✘|✘|✔|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|77|VDecode_Concurrent_Tunnel_NonSecure_EOS|✔|✘|✘|✘|✘|✔|✘|✘|✔|✘|✘|✘|✘|✔|
|78|VDecode_Concurrent_Tunnel_NonSecure_Flush|✔|✘|✘|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|✔|
|79|VDecode_Concurrent_Tunnel_NonSecure_FlushReset|✔|✘|✘|✘|✘|✘|✘|✔|✔|✘|✘|✘|✘|✔|
|80|VDecode_Concurrent_Tunnel_NonSecure_Resolutions|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|81|VDecode_Concurrent_Tunnel_NonSecure_HDRModes|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|82|VDecode_Concurrent_Tunnel_NonSecure_FrameRate|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|83|VDecode_Concurrent_Tunnel_NonSecure_GOPStructure|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✔
|84|VDecode_Concurrent_Tunnel_NonSecure_LowLatency|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|85|VDecode_Concurrent_Tunnel_Secure|✔|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|86|VDecode_Concurrent_Tunnel_Secure_Error|✔|✔|✔|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|87|VDecode_Concurrent_Tunnel_Secure_CODEC|✔|✔|✘|✔|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|88|VDecode_Concurrent_Tunnel_Secure_Discontinuity|✔|✔|✘|✘|✔|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|89|VDecode_Concurrent_Tunnel_Secure_EOS|✔|✔|✘|✘|✘|✔|✘|✘|✔|✘|✘|✘|✘|✔|
|90|VDecode_Concurrent_Tunnel_Secure_Flush|✔|✔|✘|✘|✘|✘|✔|✘|✔|✘|✘|✘|✘|✔|
|91|VDecode_Concurrent_Tunnel_Secure_FlushReset|✔|✔|✘|✘|✘|✘|✘|✔|✔|✘|✘|✘|✘|✔|
|92|VDecode_Concurrent_Tunnel_Secure_Resolutions|✔|✔|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|93|VDecode_Concurrent_Tunnel_Secure_HDRModes|✔|✔|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|94|VDecode_Concurrent_Tunnel_Secure_FrameRate|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|95|VDecode_Concurrent_Tunnel_Secure_GOPStructure|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✘|✔
|96|VDecode_Concurrent_Tunnel_Secure_LowLatency|✔|✔|✘|✘|✘|✘|✘|✘|✘|✘|✘|✘|✔|✔|

