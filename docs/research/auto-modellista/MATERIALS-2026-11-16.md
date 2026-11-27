# Auto Modellista primary materials / textures â€” 2026-11-16

Current status: [native track reader integrated and tested](NATIVE-READER-2026-11-16.md). Earlier research-only/AHI-pending statements below are historical; cars and original shader effects remain pending.

Status: linking recovered and tested in research exports. Not integrated into native9XD Scan. Original cel shading, environment-coordinate generation, dynamic overlays, alpha passes and AMO/AHI assembly remain pending. Reference evidence comes from the local USA executable and source files, not filename heuristics.

## Exact reference chain

1. Model attribute0x60000 contains one u32 material index for each primitive across all index lists. `GetMaterialIndexAMOModelMesh` VA0x13f020 sums preceding list primitive counts then indexes this array.
2. This is a LOCAL index into model attribute0x50000, not a direct root material number. `plAMOGetMaterialList` VA0x13ead0 copies that list. Example car044 mesh3 has root list[6,7,8] and primitive-local indices[0,1,2]; treating0,1,2 as root material numbers is wrong.
3. The selected root directory9 record (type0x130000,272 bytes seen) has payload at+12. `mkMaterial` VA0x1fd694..0x1fd6b4 tests payload u32+0x34 (texture enabled), then reads payload u32+0x100 as the ROOT texture-record index.
4. Root directory10 record payload firstu32 is a LOGICAL runtime slot. It is not an outer-file offset and not necessarily the local TIM2 ordinal. `mkMaterial` looks up the per-model context table if present, or adds model texture base; then resolves the global texture-handle array.
5. `set_material` VA0x1fd560 copies payload float4+0x10 to runtime+4, float4+0 to runtime+0x24, float4+0x20 to runtime+0x14 and float+0x30 (integer conversion) to runtime+0x48. Research MTL uses+0x10 RGB/alpha as primary color; exact original lighting model remains pending. Texture upload uses existing9XD decodeTim2 unchanged.

## Track banks

`create_stage_sub` VA0x1ca0f8 decompresses the bank. stageTbl VA0x355400 -> descriptor(first outer slot, count, following model slot). Stage1 descriptor atVA0x3b2430 is(0,49,49). `create_tex` VA0x1c9db0 loads49 TIM2s from outer slots0..48 into runtime slots74..122. Stage meshes create materials with base74; root texture logical slot selects the exact bank image. Stage0 uses55 images, stage2 uses54. This matches tested stg000/stg001/stg002 banks. Quality/weather bank selection is a separate context decision: stg001_q.bin also contains49 bounded images (correcting earlier48 found by signature search). Do not substitute q-bank solely by dimensions.

## Car context / shared images

`create_car_common` VA0x1ca520 loads car_com slot0 into runtime40, slot1 into41, slot4 into61. `create_car` VA0x1ca760 also loads/updates common slots2(sticker),3(sticker),6(number-plate base). `texIndexTbl$291` VA0x355690 initially maps car logical slots0..7 to runtime[50,16,32,42,40,41,24,62] for car instance0. Values are updated per instance; do not hard-code runtime handles across cars.

`carIndex` VA0x362bc0 maps file code to CAR_DATA_LIST profile VA0x355150. car044 code44 maps profile20 (pointerVA0x353e20); car000 maps profile0. Both tested profiles place paint variants at outer slots28..30 and an additional texture at32. `create_car` reads profile+0x50 plus selected paint and profile+0x58+1 for the extra texture. LoadBin table index45 resolves AFS member21, car044.bin, confirming decimal code44. Research selects paint0 explicitly, not an inferred variant from image appearance.

|Logical slot|Traced source for tested paint0 / instance0|
|---|---|
|0|car_com.bin outer3, dynamic sticker base|
|1|local car profile paint start+paint index (28 for these two cars)|
|2|environment runtime32: see below|
|3|car_com.bin outer2, dynamic sticker base|
|4|car_com.bin outer0|
|5|car_com.bin outer1|
|6|car_com.bin outer6, number-plate base (game patches content)|
|7|local car profile extra start+1 (32 for these two cars)|

Missing slot2 is NOT an absent wheel texture: `create_env` VA0x1ca3fc..0x1ca420 uses envTexTbl VA0x355530 descriptor(0,2,2), loading environment image slots0/1 into runtime32/33. `load_env` VA0x223640 uses selected context+0x530 in LoadBin. Context1 resolves envtex01.bin; the research comparison explicitly supplies this environment, rather than silently choosing it for every car. Binding an environment image is not the same as reproducing generated reflection UVs. `car_ch_wheel.bin` was decoded as additional evidence, not used to fill missing slots by guess.

## Validation and measured coverage

- Compiled a headless standalone probe against current9XD `image_containers.h::decodeTim2` (Qt6.11.2).332/332 bounded TIM2 images decoded to PNG,0 errors. Sources: car0444, car0004, car_com6, car_ch_wheel107, stg00149, stg001_q49, stg00055, stg00254, envtex014. Full decodedRGBA hashes/dimensions in native-textures.json. Atlases/logos/fences inspected in car-textures.png and stg001-textures.png; no extra swizzle or global UV flip applied.
- Original MIPS64 `GetMaterialIndexAMOModelMesh` and callees emulated for10 first/last primitive cases across car meshes0/3/4/19/22; exact agreement with local primitive array indexing. Validation includes nonzero local material indices. See validation.json.
- Portable saved material_probe.py rerun on car044 body0:405/405 faces, no missing slots; saved extract_tim2.py rerun:car0444/envtex014 bounded images.
- All exported primitive local/global material references are in bounds.10 export cases retain source triangle counts. This is primary-reference/image coverage, NOT a100% original-render claim.

|Case|Meshes with all primary images|Triangles with primary images|Without environment|
|---|---:|---:|---|
|car044 isolated source body0|1/1|405/405 (100%)|No environment needed|
|car044 all source variants, envtex01 explicit|202/202|8344/8344 (100%)|7036/8344 (84.32%);1308 request environment|
|car000 all source variants, envtex01 explicit|202/202|8357/8357 (100%)|7076/8357 (84.67%);1281 request environment|
|stg00_m0 +stg000|76/76|14369/14369 (100%)|N/A|
|stg01_m0 +stg001|90/90|15048/15048 (100%)|N/A|
|stg01_m1 +stg001|68/68|14732/14732 (100%)|N/A|
|stg01_m2 +stg001|85/85|16130/16130 (100%)|N/A|
|stg02_m0 +stg002|55/55|12156/12156 (100%)|N/A|

Textured OBJ/MTL/PNG proofs: `build/auto-modellista-research/materials/exports/`. Car body0 is visibly coherent with its orange Fairlady Z atlas (car044-textured.png); it remains intentionally isolated/incomplete. Whole-car exports still overlap alternative passes/representations. Track preview stg01-textured.png has correct primary images but raw AMO geometry is NOT AHI-assembled. Source GS ST preserved in sampling; OBJ export writes V=1-sourceT only to convert OBJ/image coordinate conventions. No speculative horizontal or vertical source-data correction.

## Reproduction / handoff

Research scripts are adjacent: meltw.py, elf_reference.py, extract_tim2.py, material_probe.py and texture_probe.cpp. Extract with `python extract_tim2.py --assets EXTRACTED_BIN_DIR --output OUT car044.bin car_com.bin envtex01.bin stg001.bin`. Compile texture_probe.cpp with existing9XD src and QtCore/Gui headers/libs, then run `texture_probe OUT/tim2 OUT/png`. Export with `python material_probe.py --assets EXTRACTED_BIN_DIR --exe PATH_TO_SLUS_206.42 --output OUT --name car044.bin --only 0 0`; remove --only to retain all variants, add `--environment envtex01.bin` for explicit context. Track: `--name stg01_m0.bin --bank stg001.bin`. These are research scripts, use assertions, and are NOT hardened production decoders. European materials haven't been validated here; portable ELF lookup alone does not establish compatibility of context loaders.

- [x] Primitive ->local list ->root material ->texture record ->context ->TIM2 chain recovered.
- [x] Existing native decoder exercised; positive material-index original-instruction checks; textured exports inspected.
- [ ] Codex: complete AMO/AHI associations/initial transforms and car pass/LOD selection before claiming correct assembled geometry.
- [ ] Codex: reproduce texture-coordinate generation, reflections, source material flags/lighting, alpha and dynamic number plates/stickers. Base images alone are not the final appearance.
- [ ] Codex: validate matching Europe tables and broader vehicle/stage corpus; identify weather/quality context exactly.
- [ ] Native integration: bounded Meltw/AMO/AHI/material scanner + automatic sibling bank context, image registry/export, meaningful malformed/cancel/budget fixtures and full regression. No app code/build/install/launch/push/release performed in this research session.


## Track render review — 2026-11-16

Owner requested actual track renders. Generated five1600x1050 depth-tested textured source-package views (stg00_m0, stg01_m0/m1/m2, stg02_m0), plus three isolated first-AMO views. Inspected overview and individual views. Roads/terrain have primary texture binding, but secondary groups remain displaced and clustered because AHI is unapplied; do not present as complete assembled maps or validated original shading. Full-resolution PNGs in build/auto-modellista-research/track-renders; track-overviews.png is the review contact sheet. Source UVs unchanged; alpha cutoff and flat lighting are only research preview approximations. No app launch or production code change. Next: AHI assembly, original pass selection and rendering.
