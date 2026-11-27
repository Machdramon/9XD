# Auto Modellista regional comparison and render diagnosis — 2026-11-16

Current status: [native track reader integrated and tested](NATIVE-READER-2026-11-16.md). Earlier research-only/AHI-pending statements below are historical; cars and original shader effects remain pending.

Research follow-up to README.md. The old car044 preview overlaid all36 models in the first AMO and is not a valid assembled car. No 9XD code/build/install/launch changed.

## What was wrong with the preview

The research exporter emitted all models; first car044 AMO has36 meshes/1952 triangles, the whole file202/8344. An isolation contact sheet shows multiple body-shaped representations and separate panels, glazing-like surfaces and thin helper/detail pieces. Exact LOD/pass/damage semantics are NOT yet assigned. Rendering all as opaque, untransformed surfaces overlays them. The prior painter renderer also had approximate occlusion; replaced for this comparison by an independent triangle depth buffer. Even with correct depth, all36 still look wrong; isolated mesh0 (405 triangles) is coherent. See car044-isolation.png and car044-meshes.png. Mesh0 is incomplete (no wheels/other panels); do not ship this arbitrary selection as the default car.

Static native evidence strengthens the strip interpretation: `ConvertModelMeshAMO_NormalModel` writes each source primitive count and cull flag separately; `flPS2ConvClayData` VA0x123000..0x123010 writes ADC0x8000 for the first two emitted vertices of a primitive and clears it afterwards. `mdl_output_data` VA0x31a890 contains output PRIM words0x5c and0x4c (primitive low3 bits=4, triangle strip). Source primitive boundaries must remain independent. Full culling, render passes, per-model attributes and in-game comparison remain pending. There is no evidence that reducing a fixed fraction of triangles is the correct repair.

## Both regional executables are useful

Both SLUS_206.42 (USA) and SLES_511.91 (Europe) retain ELF symbols. Use USA as the primary existing research baseline and Europe as a comparison; neither is inherently an incompatible/undecodable format. Europe Meltw atVA0x2af9b0 has216 bytes vs USA0x263bb0/200 bytes. Original European MIPS64 instructions independently emulated on EU car044 produce exactly the bounded research decoder output and consume the entire source. Do not equate differing machine bytes/jump relocations with differing semantics.

|Sample|USA models/vertices/triangles|Europe models/vertices/triangles|Comparison|
|---|---|---|---|
|car044.bin|202 /9945 /8344|195 /9889 /8304|Different source assets; both decode|
|car000.bin|202 /10562 /8357|195 /10491 /8325|Different source assets; both decode|
|stg01_m0.bin|90 /28261 /15048|90 /28259 /15048|Small content difference; same face count|
|stg01_m1.bin|68 /27481 /14732|68 /27481 /14732|Compressed and decoded bytes identical|
|stg01_m2.bin|85 /29771 /16130|85 /29771 /16130|Compressed and decoded bytes identical|

Counts are all source meshes, not default scene coverage. Texture banks stg001.bin and stg001_q.bin are also byte-identical across these two copies. See regional-comparison.json.

## The missing track layer: AHI

`create_stage_mdl` VA0x1ca1e0 calls Meltw, then consumes table slots in adjacent pairs. VA0x1ca2a0..0x1ca2c4 passes offsets[index] and offsets[index+1] to `create_model`. `create_model` stores AMO at model+4 and the second resource at+0x24. `mkModelNW` VA0x1fca6c calls `set_hierarchy` with that second pointer. `set_hierarchy` calls `plAHIGetModelNum`, `plAHIGetTreeNum`, `plCreateInitMotionSetFromAHI`, `flGetHierarchy` and `flFindGroupRoot`.

Example stg01_m0: table has10 offsets, five AMO/AHI pairs. AMO offsets0x40,0x4df00,0xa39c0,0xf7400,0x14af80; first AHI0x4c8c0 begins type0xc0000000. Previous probe ignored these AHI blocks. Rendered raw AMO groups show displaced helper/scene pieces; need model association, hierarchy transforms and source selection before complete map correctness. Do not repair by shifting groups by guessed bounds or removing long triangles.

## Files for testing

USA source directory: `F:\copia de seguridad 08.04.2019\downloads\Auto Modellista (USA)\Auto Modellista (USA)\afs unpack 2`.

|File|Models|Vertices|Strip triangles|Use|
|---|---:|---:|---:|---|
|stg00_m0.bin|76|27413|14369|First oval-like track portion; useful simpler starting case|
|stg00_m1.bin|63|23155|12071|Other package for same stage|
|stg01_m0.bin|90|28261|15048|More detailed track sector; compare with Europe|
|stg01_m1.bin|68|27481|14732|Byte-identical regional comparison|
|stg01_m2.bin|85|29771|16130|Byte-identical regional comparison|
|stg02_m0.bin|55|23224|12156|Different stage/layout|
|stg13_m0.bin|61|30525|17018|Mountain-style comparison candidate|
|stg18_m0.bin|12|9874|5589|Smallest tested package; scene role/name unconfirmed|

USA stage_name_tbl VA0x37a890 contains localized names by index:0 US Speedway,1 Suzuka Circuit,2 US Dirt Track,3/4 Ura Rokko,5/6 Shinjuku B clear,7/8 Shinjuku A,9/10 Shinjuku B,11/12 Hanshin Expressway loop,13/14 Akagi. `check_data_no` VA0x2235e0 uses StageDataOffset$219 VA0x362910 + package index; LoadBin remaps through tableVA0x3658a0 into AFS member indices. This gives stage0 AFS340/341 stg00_m0/m1, stage1 AFS342..344 stg01_m0/m1/m2, stage2 AFS345/346 stg02_m0/m1. UI name lookup `grace_model_0014` VA0x2e8298..0x2e82b8 uses stage_name_tbl. Direction/weather/remaining menu-ID translation still needs full call-chain confirmation; filenames are the definitive test identifiers. Do not call these packages individual named corners or assume each is the whole track.

Europe is not extracted on disk: corresponding BIN members are in `Auto Modellista (Europe) (En,Fr,De,Es,It)\Auto Modellista (Europe) (En,Fr,De,Es,It)\afs01_pp.afs` (625 members; USA afs01_pu has745). Research extracts selected members read-only from original AFS; source archives untouched.

Geometry-only provisional OBJ exports for the table and isolated car mesh are in `build/auto-modellista-research/`. They can be inspected in an OBJ-compatible viewer. These are not textured or AHI-assembled. Native Auto Modellista Scan for 3D Data is still unimplemented; opening original BIN in9XD is not yet an equivalent test. TEST-FILES.csv and track-candidates.json record counts. No full app regression run because app code is unchanged.

## Next work

- [x] Compare both regional assets and symbol-bearing executables; byte-exact Europe Meltw check.
- [x] Separate render occlusion artifact from all-model assembly error; isolate coherent source body.
- [x] Trace original strip kicks and AMO/AHI pair consumption; prepare eight track OBJ probes.
- [ ] Codex: implement bounded AHI model associations/initial transforms; cross-check one assembled stage in-game.
- [ ] Codex: trace car selection/render-state/LOD/damage helpers; label exclusive representations without guessing from size.
- [ ] Codex: link material9/texture10 records to exact stage TIM2 banks, then native scan/export + fixtures. No arbitrary face reduction or global UV flip.
