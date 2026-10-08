# Test Stream specifications

L3 test media are local data files for the test suite to inject directly into the HAL.

They are read by the test suite, written into linear buffers and passed into the video decoder.

```bash

<CODEC>_<PRO>_<RES>_<DR>_<COL>_<FR>_<PIX>_<BITRate>_<description>.es (elementary stream file)
e.g.
H265_MAIN10_3840x2160_DR_HLG_DR_BT2020_25p_PIXEL_FORMAT_YCBCR420_18000.es

```

## Elementary stream files

- Single I-frame or IDR frame
- Single GOPs (closed)
- Multiple GOPs (open)
- Multiple GOPs (open and closed)

||Codec|Resolution|Dynamic Range|ColorSpace|Depth|Frame Rate|Pixel Formate|Bit Rate|
|-|-----|----------|------------|---------|-----|----------|-------------|--------|
|1|H.265|3840x2160 | SDR        |BT.709   |10 bit|50/60 | YUV420 |~18Mbps|

### Test Dimensions

| #|Test Dimension | Comments | Filename Part |
|--|---------------|----------|---------------|
| 1 | Codec | Test every codec which the platform capabilities report. <br> Test unsupported codecs to check for errors. | `<CODEC>` <br> Same as `com.rdk.hal.video.decoder.Codec` enum. <br> e.g. "CODEC_H265" |
| 2 | Profile/Level | For codecs that break down into profiles and levels. | `<PRO>` <br> e.g. "MAIN10" |
| 3 | Resolution | Test all Viper resolutions: <br> - 3840x2160 <br> - 2560x1440 <br> - 1920x1080 <br> - 1280x720 <br> - 960x540 <br> - 640x360 <br> - 320x240 <br> YouTube portrait mode videos from phones. <br> YouTube adds: <br> - 426x240 <br> - 7680x4320 (8K) | `<RES>` <br> HxW syntax. <br> e.g. "3840x2160" |
| 4 | Dynamic range | - SDR <br> - HLG <br> - HDR10 <br> - HDR10+ <br> - Dolby Vision | `<DR>` <br> Same as `com.rdk.hal.video.decoder.DynamicRange` enum. <br> e.g. "DR_HLG" |
| 5 | Color space | - BT.601 <br> - BT.709 <br> - BT.2020 | `<COL>` <br> Same as `com.rdk.hal.video.decoder.DynamicRange` enum. <br> e.g. "DR_BT2020" |
| 6 | Frame rate | Viper Frame rates (UK): 25, 50 <br> Viper Frame rates (US): ??? <br> Netflix Frame Rates: 23.976, 24, 25, 29.97, 30, 50, 59.94, 60 | `<FR>` <br> e.g. "23.976" |
| 7 | Frame types | Progressive and interlaced frame types. | `<FR>` <br> Suffix as "i" or "p" |
| 8 | Pixel format | - YCbCr4:4:4 <br> - YCbCr4:2:2 <br> - YCbCr4:2:0 | `<PIX>` <br> Same as `com.rdk.hal.video.decoder.PixelFormat` enum. <br> e.g. "PIXEL_FORMAT_YCBCR420" |
| 9 | Bit rates | Viper bitrates (UK): 18000, 12200, 5800, 4250, 1850, 1200, 500 <br> Check the maximum bit rates supported by codec/profile/level | `<BIT>` |
| 10 | Reference frames | Single reference frames are passed for decode and are expected to be output immediately. | |
| 11 | GOP structure | Open GOPs <br> Closed GOPs <br> Min and max M and N for each supported codec/profile/level | |
| 12 | Active Format Description | Test all AFD codes appear in FrameMetadata. <br> Complete list of AFD codes | |
| 13 | Resolution switching | Moving between different resolution encodings, without breaking GOP sequences. | |
| 14 | Aspect ratio switching | Picture aspect ratios must be reported correctly in FrameMetadata. <br> Pixel aspect ratios must be reported correctly in FrameMetadata. |
| 15 | End of stream | EOS signalling from application. <br> EOS in-band signalling. |
| 16 | Discontinuity/Data Loss | Loss of frames (incomplete GOPs). <br> Partial loss of frame data - check for decode errors. | |
| 17 | Secure/non-secure | Video frames passed in clear buffers. <br> Video frames passed in secure buffers. | |
| 18 | Low latency | Measure time improvement when low latency enabled. | |
| 19 | Flushing | Check that flush call follows state machine. <br> Check that flushed frames are returned. <br> Check that flushed frames never get output. | |
| 20 | Bad encoding (frame drops) | Check that errors, drops, metrics are updated as expected. <br> Corrupt video encoding. <br> Corrupt metadata encoding. | |

## Error Streams Info

AV VTS L4 includes playback of error (corrupted) streams to evaluate the error recovery capability of the product.

As part of this verification, various video error streams are generated and used during testing.

**Example:** Video Error Stream File Name

```bash

h265_2160p_hdr10_60.0f_BT2020_yuv422p_vps-corruption_18M_a0_v3600_60sec.mp4

```

**Stream File Name Breakdown:**

|Codec|Resolution|Dynamic Range|Frame Rate|ColorSpace|Pixel Format|Error Type|Bit Rate|Audio frames|Video Frames|Duration|
|------|----------|------------|---------|----------|-------------|--------|------|----|----|----|
|H.265|2160p | HDR10        |60|BT.2020   | YUV422p |vps-corruption|18Mbps|0|3600|60sec|

### Supported Video Error Types:

Below is the list of supported video error types used for AV VTS L4 recovery validation:

#### Generic Corruption Modes

| **Video Error Type** | **Details** | **Impact** | **Applies to Codec Type** |
|----------------------|-------------|------------|---------------------------|
| **light-fuzz-all** | Applies minor random bit modifications across the entire bitstream. | Subtle, intermittent glitches. | H264, H265, MPEG2 |
| **entropy-bit-flip** | Flips bits in CABAC/CAVLC encoded data. | Desynchronizes the arithmetic decoder, causing massive macroblock corruption. | H264, H265, MPEG2 |
| **partial-frame-truncation** | Cuts off data before the frame end. | Visual corruption at the bottom of the frame; usually triggers decoder error concealment. | H264, H265, MPEG2 |
| **gop-truncation-duplication** | Truncates or duplicates entire Groups of Pictures (GOPs). | Breaks reference chains; triggers IDR/keyframe warnings. | H264, H265, MPEG2 |
| **pts-dts-corruption** | Alters Presentation Time Stamp (PTS) or Decoding Time Stamp (DTS). | Muxer/Decoder warnings (e.g., "Non-monotonous DTS"). Causes sync issues or playback stalls. | H264, H265, MPEG2 |

#### H264 Specific Corruption Modes

**Basic / Soft H264 Corruptions**

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **bitflip-slice** | Randomly flips bits within a slice. | Visual artifacts (blocking/color errors) or minor decode warnings. |
| **overwrite-macroblock** | Replaces specific macroblock data with random bytes. | Visible distortion in localized parts of the frame. |
| **insert-noise-pframe** | Injects noise specifically into P-frames (inter-predicted frames). | Artifacts may propagate across subsequent frames due to motion compensation. |
| **temporal-duplication** | Duplicates frames or slices within the stream. | Stuttering playback (repeated frames) and potential timestamp warnings. |
| **drop-partial-slices** | Deletes portions of slice data. | Missing blocks (grey/green zones) and decoder logs regarding incomplete data. |
| **mixed-soft-corruption** | Combines multiple soft corruption techniques listed above. | Unpredictable variety of artifacts and decoder warnings. |

**NAL / Macroblock-Level H264 Corruptions**

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **nal-header-corruption-h264** | Modifies NAL unit headers (e.g., forbidden_zero_bit, nal_ref_idc). | Can confuse the decoder regarding slice types, leading to dropped frames. |
| **macroblock-skipping** | Simulates missing macroblocks within a coded frame. | Visual holes in the image or severe distortion. |
| **motion-vector-corruption** | Alters motion vector data in P or B frames. | "Smearing" artifacts where objects move incorrectly or reference wrong blocks. |
| **slice-reordering** | Changes the order of slices within a frame or GOP. | Triggers "illegal reordering" warnings; may cause decode failure for that frame. |
| **nal-injection** | Inserts invalid or "garbage" NAL units into the stream. | Parsing errors; may break reference frame chains. |
| **cross-slice-corruption** | Spreads corruption across the boundaries of multiple slices. | Affects entropy decoding context; generates significant warnings. |

**Advanced / Structural H264 Corruptions**

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **sps-pps-corruption** | Modifies Sequence Parameter Set (SPS) or Picture Parameter Set (PPS). | Critical errors regarding resolution or profile; may prevent file muxing entirely. |
| **sei-corruption** | Corrupts Supplemental Enhancement Information (SEI) messages. | Parser warnings (e.g., "truncated SEI"). Rarely affects video decoding but may break HDR/captions. |
| **fake-reference-frame** | Replaces valid reference frame data with garbage data. | Valid headers pointing to invalid image data; causes severe decode artifacts. |
| **slice-boundary-offset** | Shifts the start/end positions of slices. | Breaks prediction boundaries; triggers slice parsing errors. |

#### H.265 (HEVC) Specific Corruptions

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **vps-corruption** | Modifies the Video Parameter Set (VPS) layer ID or profile info. | Critical initialization failure; decoder may reject stream immediately. |
| **tile-entry-point-corruption** | Corrupts entry point offsets in slice headers (Wavefront Parallel Processing). | Can cause parallel decoding threads to hang or access invalid memory. |
| **scaling-list-corruption** | Modifies quantization scaling matrices in SPS/PPS. | Mathematical overflows during inverse quantization; severe visual noise. |
| **nal-header-corruption-h265** | Flips bits in the 2‑byte NAL header (e.g., nal_unit_type, layer ID, temporal ID) to confuse decoder about frame type or hierarchy. | Often causes decode abort or severe prediction errors, leading to visual distortion or stream rejection. |

#### MPEG2 Specific Corruptions

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **sequence-header-corruption** | Sequence headers define resolution, aspect ratio, frame rate. Corrupting these often causes severe decode errors or visual distortion. | Decoder misinterprets stream parameters → jitter, wrong scaling, or crash. |
| **gop-header-corruption** | GOP headers control temporal references and time codes. Corruption here breaks prediction chains. | Motion prediction fails → jittery playback or frozen frames. |
| **quant-matrix-corruption** | Quantization matrices strongly influence visual quality. Corruption causes extreme blockiness or color noise. | Visible artifacts without immediate decode failure. |
| **motion-vector-field-corruption** | Motion vectors are critical for inter-frame prediction. Corruption leads to warped or scrambled motion. | Severe visual distortion, but stream remains decodable. |

#### VP9 Specific Corruptions

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **superframe-index-corruption** | Corrupts the size/index bytes located in the footer of a Superframe. | Parser reads garbage data as the next frame; causes massive sync loss. |
| **prob-table-desync** | Flips bits in the compressed header (probability update section). | Decoder uses wrong math for coefficients; results in neon/purple block artifacts. |
| **loop-filter-max** | Sets loop filter strength to illegal maximum values in Frame Header. | Visual "wash out" or potential crash in the post-processing filter block. |
| **resolution-change-stress** | Injects a header with a new resolution (DRC) but same buffer size. | Stresses memory reallocation logic; common cause of driver crashes. |

#### AV1 Specific Corruptions

| **Video Error Type** | **Details** | **Impact** |
|----------------------|-------------|------------|
| **obu-header-fuzz** | Modifies the `obu_type` field (e.g., Frame OBU → Sequence Header OBU). | Confuses the state machine; decoder loses track of stream structure. |
| **film-grain-param-overflow** | Sets `num_y_points` in film grain params to > 14 (illegal value). | Often causes hardware buffer overflows in the film grain synthesis block. |
| **cdf-update-corruption** | Corrupts the Cumulative Distribution Function (CDF) update bits. | Desynchronizes entropy decoding; valid bitstream becomes unreadable garbage. |
| **tile-group-size-mismatch** | Sets `tile_group_size` larger than remaining data in the OBU. | Buffer over-read; parser tries to read past the end of the input buffer. |
