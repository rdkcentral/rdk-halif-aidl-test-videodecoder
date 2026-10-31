# VideoDecoder HAL Test Suite

This repository contains the Vendor Test Suite (VTS) for the `IVideoDecoder` AIDL HAL interface. It is designed to validate the functionality of the VideoDecoder HAL implementation.

## Build Steps

This test suite is part of Unified VTS for AIDL HALs. For initial setup, environment configuration, and build instructions, please refer to the Unified VTS README documentation.

- **[Unified VTS README](https://github.com/rdkcentral/rdk-halif-aidl-unified-vts/blob/main/README.md)**

## Running the VideoDecoder Tests

While the Unified VTS can run all tests at once, you can execute the `VTS_L1_VideoDecoder`, `VTS_L2_VideoDecoder` and `VTS_L3_VideoDecoder` test suite individually for targeted validation. This is useful for debugging and development of the `VideoDecoder` HAL.

### AIDL 0.23.0

L3 cases are operation-based, for example `VDecode_Sequential`,
`VDecode_Sequential_CODEC`, `VDecode_Sequential_HDRModes` and
`VDecode_Concurrent_CODEC`. Tunnel/non-tunnel and secure/non-secure variants
have been consolidated; update test filters and YAML configuration keys accordingly.

Each decoder is opened with `secure = Capabilities.supportsSecure`. After every
`start()`, `IVideoDecoder.getCurrentOperationalMode()` identifies the vendor-selected
output mode. Non-tunnelled callbacks must transfer valid AVBuffer handles; the
client passes them to an attached sink or frees them when no sink is attached.
Tunnelled callbacks must carry `-1`, with output delivery and cleanup owned by
the HAL. Standalone L3 tests have no attached sink and verify heap cleanup after
closing the session. HFP profiles no longer require `supportedOperationalModes`.

The mock selects non-tunnelled output for decoder 0 and tunnelled output for
decoder 1, so the same operation tests exercise both callback contracts.

## Reference Documents

|SNo|Document Name|Document Description|Document Link|
|---|-------------|--------------------|-------------|
|1|High Level Test Specification Document|High Level Test Specification Documentation for this module|[VideoDecoder_High-Level_TestSpec.md](docs/pages/VideoDecoder_High-Level_TestSpec.md)|
|2|`L2` Low Level Test Specification and Procedure Document|`L2` Test Specification and Procedure Documentation|[VideoDecoder_L2_Low-level_TestSpecification.md](docs/pages/VideoDecoder_L2_Low-level_TestSpecification.md)|
|3|`L3` Low Level Test Specification and Procedure Document|`L3` Test Specification and Procedure Documentation|[VideoDecoder_L3_Low-level_TestSpecification.md](docs/pages/VideoDecoder_L3_Low-level_TestSpecification.md)|
