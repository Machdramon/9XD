# F&F retail PS2 materials and GS textures — 2026-11-15

## Implemented pipeline

Scan for3D Data -> native ARCP mesh packet -> type7 material ordinal -> type2 material prefix -> image ordinal across type1/type52 entries -> bounded GS upload decoder -> MeshSurface texture/library -> viewport and textured GLB/OBJ export. Geometry, source UVs and instance placement are unchanged. This is the primary color pass, not original-game shader emulation.

Source files: src/fnf_ps2_surfaces.h, src/fnf_ps2_shared_textures.h and src/fnf_ps2.h. Existing scene loading automatically puts decoded referenced images in the application image library. General standalone Scan for Textures does not yet recognize the custom GS wrapper directly.

## Texture layout and decoding

- 14-byte header: byte0 is PSM (0=RGBA32,19=Indexed8,20=Indexed4), low nibbles of bytes1/2 are TW/TH, byte3 is GS TBW in64-pixel units, u16+8 is CLUT block base. Byte3 is NOT a mip count. Several mip levels can share an IMAGE upload rectangle; only the verified base surface is sampled.
- At+14 is an inline DMA END tag; its QWC must exactly bound the resource after the30-byte header/tag prefix. The GIF stream uses AD state packets and IMAGE payloads. BITBLTBUF/TRXPOS/TRXREG/TRXDIR describe host-to-local32-bit transfers, including nonzero origins for CLUT/mip data.
- Replay transfers into4MiB GS memory with per-byte written tracking, using shared src/ps2_gs_address.h. Sample the requested PSM/TBW; reject unwritten/out-of-range reads rather than trying alternate swizzles. Source U/V and pixel orientation are retained; no image/UV flips added.
- CSM1 palette lookup samples GS memory from the declared CBP. This matters for the car's16x8 palette uploads: reading the first16 raw RGBA entries is incorrect because the colors are physically interleaved across GS blocks. Preview uses CLUT bank0; dynamic paint/color banks are not reproduced. Convert GS alpha0..128 to0..255, preserving RGB.
- Reference-only14-byte image headers and zero-byte type52 entries are never treated as pixel data.

Standard GS address/CLUT semantics were cross-checked against [PCSX2 GSClut.cpp](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSClut.cpp). Production implementation reuses the project's own GS address helper, not emulator source code.

## Binding and shared resources

Type2's40-byte prefix has a native image-table index at+0, source RGB modulation at+4 and GS CLAMP at+16. Wrap0/1 is Repeat/Clamp; unsupported region modes are not silently approximated. Remaining state bytes and render-mode byte37 are retained in provenance. Runtime animation/additive/reflection semantics are not inferred from them.

Type25 sprite resources consume their preceding type7 state markers. The earlier geometry-only reader carried those markers into the following type16 mesh and could therefore select a glow texture as the mesh's primary image. This is corrected. Across231 TRACK_ packages:82,548 packets have one material marker and97 have two. Every marker indexes a valid material. For real multi-pass meshes, only the first stage supplies the color texture; other stages remain explicit references/images and never replace a missing primary stage.

Shared resolution is automatic for extracted .arc files. Index only sibling ARCP resource directories, bounded by file count/memory, then match exact image ID, PSM and dimensions. A zero-size external entry without dimensions selects the highest available resolution for that ID. Normalize neither names nor filename guesses: compare matching decoded pixel results and refuse conflicting variants. Missing/ambiguous references remain visibly untextured with diagnostics. The index is cached under a mutex; source size/time is checked around shared reads and cached-image reuse. In-memory child resources can use local pixels; references need an actual extracted source path.

Decoded images are implicitly shared for repeated instances and GLB image deduplication. Per-worker catalog/image caches and shared index/image caches obey fractions of the configured memory budget. No source assets are rewritten.

## Measured coverage

All231 TRACK_ packages, including *_PSP variants (128 without that suffix):

| Measurement | Result |
|---|---:|
|Native material references associated with static packets|82,645/82,645 (100%)|
|Packets with verified primary pixels and usable UVs|82,047/82,645 (99.28%)|
|Placed mesh results with primary texture|142,421/144,375 (98.65%)|
|Unique-source faces with primary texture|3,878,909/3,918,311 (98.99%)|
|Decoded per-package image slots used by primary materials|12,385|
|Shared image slots resolved automatically|94|

The remaining598 packet resources are:490 ambiguous shared pixel variants,102 references with no matching pixels/dimensions and6 invalid-source-UV resources. These are texture-availability/binding measurements, not proof of original shader appearance or100% visual correctness. Receipt: ignored build/fnf-ps2/material-corpus.json. Public metadata index: TRACK-MATERIALS.csv.

Actual application discovery/decoder/registry/scene/export tests:

|Scene|Meshes with texture / loaded|Textured faces / faces|
|---|---:|---:|
|UCP000|292/292|59,071/59,071|
|tokyo000|1,686/1,686|34,043/34,043|
|tokyo048|1,179/1,179|38,312/38,312|
|suia000|242/243|31,022/31,074|
|USD000|590/590|36,251/36,251|

The Evo VII BODY reference has9/10 textured results. Its Details image can be resolved by exact ID from another package; the MOD reference and environment/generated shading remain unresolved. This does not establish complete assembled-car paint/shaders.

## Validation and visual limits

Focused14 passed/0 failed/0 skipped, including synthetic GS upload/CBP/TBW/cancellation/truncation checks, shared-ID/dimension/ambiguity checks, actual source material leakage regression, P4/P8 golden RGBA digests and real RGBA32 samples. Five textured GLBs preserve exact scene face counts; source images are embedded. Evidence: build/windows-local/fnf-ps2-material-focused.txt and build/fnf-ps2/material-validation.json. Final full regression148 passed/0 failed/87 optional fixture skips (build/windows-local/fnf-ps2-material-full.txt). Updated development EXE linked successfully and left closed (build/raycity-app-build.log); git diff --check clean. Receipt also synchronized in docs/COORDINATION-PLAN.md.

Headless rasterizations were inspected: build/fnf-ps2/UCP000-textured-review.png and tokyo000-textured-review.png. Readable source textures/contact sheet confirms the base GS decode; original blending is NOT reproduced. Reflection proxies and additive/multiplicative glow/shadow layers can appear as opaque black geometry in raw Render All previews. Do not describe this as a faithful full-material render.

REFLECTION_ is an explicit original object namespace: SLUS file offsets0x470438/0x471660; calls at VA0x2b22e4/0x2b863c query it separately through0x399f48 and store distinct pointers. These helpers are now marked optional in Scan results: Render Default Set omits them; Render All retains raw helper geometry for analysis. This is not a replacement for the actual reflection pass.

## Next work / handoff

- Codex: decode original material blending/test flags and emulate supported additive/multiplicative/alpha effects; retain unsupported state warnings. Do not invent alpha from luminance or silently turn a reflection mask into albedo.
- Codex: determine authoritative shared-bank/context selection for the490 conflicting variants; trace runtime resource lookup or archive load context rather than choose an arbitrary same-name image. Locate102 unmatched references or establish generated-resource semantics.
- Codex: dynamic car paint palettes, vertex color lighting and UV/shader animation; CLUT bank0 is only a baseline.
- Claude/UI: optionally expose material/source-role diagnostics and native GS images in standalone texture scanning; referenced decoded images already populate the image library.
- Owner: visually compare a reopened/re-scanned scene. Application was rebuilt without opening it; no push/release authorized.