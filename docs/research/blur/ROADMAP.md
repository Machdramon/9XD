# Blur native analysis roadmap

Updated: 2026-11-17. Owner: Codex/native analysis; UI refinement may be handled by Claude.
Status: native CPModel, paired texture inspection and bounded static scene geometry integrated; focused validation passed; development EXE rebuilt. Showroom subset313 sections/58,857 faces; Haruna25,450 sections/4,876,380 faces with large companion reading and shared sessions corrected; base-colour textures linked for accepted sections; full shaders, additional packet kinds and race-map coverage pending. See NATIVE-SCENE-GEOMETRY-2026-11-17.md.

## Blur UV origin — 2026-11-17 (tested; EXE updated)

- [x] Owner reports upside-down cars, advertising and some trees; road/buildings appear correct. Confirmed viewer samples image row `1-V`, while Blur readers previously forwarded source D3D/top-origin UVs unchanged. Repeated/symmetric surfaces can conceal this discrepancy; no source texture swizzle change inferred.
- [x] Read-only original PC shaders-speed.pak: 1,256 directory entries; nine selected FXB resources decompressed to exact declared sizes and DX9 programs disassembled with D3DDisassemble. Default_LN_NB_NV vertex shader forwards UV0.xy directly; base pixel sampler reads those XY. SpeedTree and SpeedTreeLeaf_LN also forward primary XY, not 1-Y. g_vUVOffset audit in parsed Haruna instance sets found no nonzero offsets. Private shader binaries/disassembly remain outside repository.
- [x] src/blur_model.h + src/blur_scene_geometry.inc now adapt `V=1-sourceV` once to MeshData convention, preserving U, tiling, source offsets, geometry and decoded pixels. Bindings expose sourceUvTopLeft/uvVFlipped. No common shader/export flip added. Secondary UV channels, original wind and billboards remain pending.
- [x] Validation12 passed/0 failed/0 skipped,31,284ms: asymmetric four-corner sampling/tiling; standalone decoder; real showroom source-half UV comparison; static/multidraw/material/scan and Haruna geometry/billboard regressions. Counts remain showroom313/58,857 and Haruna25,450/4,876,380. Both targets rebuilt after touching translation units; development EXE SHA256 4e83db180108de6c7fa3bf2a4527ecf992853ef14ab2675c2588403c4b333b1b. Logs build/windows-local/blur-uv-origin-tests.txt and blur-bulk-final-build.txt; blur-uv-origin-receipt.json records hashes. App left closed. No new visual Vulkan verification; do not claim all material-specific UV issues resolved.
- Handoff: docs/research/blur/UV-ORIGIN-2026-11-17.md. Next owner: owner visual recheck after clearing/reloading old Mesh Collection results.


## Blur explicit placed instances — 2026-11-17 (tested; full batch passed; EXE updated)

- [x] Identified second RenderList array(stride20) as placement records: prototype index into fourth array(stride48), typed instance-parameter reference, optional secondary binding override and flags. Haruna22,966 records reference3,182 prototypes. Previous reader discarded both arrays. No nearest-geometry/transform matching; exact serialized indices used.
- [x] Native resolves g_mWorld affine matrices, composes stored XYZ into world space, corrects negative-determinant winding, and checks output against corresponding serialized instance bounds. Every accepted ordinary rigid placement fits bounds within0.05 world units; stored SpeedTree leaf quads allow5% of bound extent due fixed camera-facing representation. Source colour-only stream overrides are validated as4-byte semantic9 records; baked colours/wind remain unapplied. Skeletal palettes are explicitly excluded rather than placed with identity.
- [x] Independent sequential analysis and native probe agree:19,837 new placements/3,973,320 faces,3 exact duplicate placements suppressed,3,126 skeletal instances pending,0 other unsupported placement records. Haruna total25,450 sections/4,876,380 faces; showroom313/58,857 (+137 placed sections). Leaf/billboard poses remain fixed. All placement matrices in this corpus have negative determinant; bounds corroborate row-vector convention.
- [x] Candidate vertex count now reports used retained vertices, not the entire shared source pool for each mesh. Indexed session identity lookup removes linear searches across25k sections. Cached source/images retain revision/patch guards. Scene notes expose placed/pending/duplicate counts; unassigned primary global references stay neutral and explicitly marked, never remove geometry.
- [x] Resume audit: previous23 focused checks passed and EXE linked; interrupted prior full-scene run had no final result. Repeated full-batch validation exposed ARGB32 -> RGBA8888 conversion in MeshScene::add making a new image for every instance, defeating source image cache sharing and inflating retained texture memory. Blur now normalizes to RGBA8888 once before caching. New regression verifies two actual MeshScene entries share one cache key and exactly one image allocation. Native/full-batch tests and EXE refresh repeated after this correction.
- [x] RGBA correction passes23 focused checks including actual scene image sharing. Profiling the long Qt test showed~2ms decode but~18ms worker-to-GUI delay from its polling wait. Full-scene harness now waits on batch completion in a normal nested event loop with the same300s deadline and exact25,450/4,876,380 assertions; it does not relax completion or alter product behavior. Final completion-driven full batch passed; see final validation below.
- [x] Large trace also measured26.7s between auto-preview and first bulk item. Bulk tree selection now submits contiguous model ranges once; batch extraction uses selected model rows rather than per-item selection queries. Default/alternative/heuristic/part and hidden-row rules are preserved; no visual redesign. Added deterministic selection-contract test. Native/full-batch validation repeated for this necessary25k-row path.
- [x] Final focused checks24 passed/0 failed/0 skipped,56,682ms; separate full Haruna Render All3 passed/0 failed/0 skipped,66,107ms. All25,450 admitted pieces/4,876,380 faces and19,837 placements reached MeshScene. Trace compared every expected model identity and per-mesh vertex/face count:0 missing/unexpected/duplicate/mismatched results;64 pieces remain neutral for unresolved global images. Shared RGBA cache and bulk selection/filter/current-row tests pass. Development EXE rebuilt; build/windows-local/blur-placed-instances-receipt.json records SHA/source hashes. Logs blur-instance-tests.txt, blur-haruna-render-all-tests.txt and blur-bulk-final-build.txt. App left closed; real Vulkan appearance not claimed.
- [ ] Next Codex:3,126 skeletal instances (crowds; require actual palette/bind-pose/animation evaluation), global texture table64 untextured Haruna sections/29 showroom sections, original wind/camera-facing effects and source LOD/pass selection. Do not claim complete map/original rendering.
- Handoff: docs/research/blur/PLACED-INSTANCES-2026-11-17.md. Private analyze_rigid_instances.py, rigid-instance-analysis.json, verify_instance_bounds.py / instance-bounds-validation.json and native probe in Codex workspace. No source assets/pixels added, interactive app launch, commit/push/release.

## Blur Haruna geometry gaps versus texture alpha — 2026-11-17 (tested; development EXE updated)

- [x] Confirmed geometry loss independently of texture alpha.22 RenderLists failed on framed typed parameter references(types8/9) and nonempty extra descriptor/value groups. Original readers0x6d96e0/0x6d61c0 and0x6e9800/0x6e90e0/0x6e8e60 traced; native parser now includes reference framing overhead in declared byte validation and consumes complete extra parameter groups.
- [x] All98 Haruna RenderList prefixes now parse. First-array Float3 subset rises3,718->4,513 sections and561,672->704,254 faces.1,100 additional stored Float4 quads belong specifically to BillBoard_LN.fx/FoliageBillBoard_LN.fx; admitted only with verified identity world matrix, finite XYZ, zero W, valid declaration/ranges. Fixed-orientation preview, two-sided alpha-masked export; original camera-facing behavior not implemented. Two light-flare packets remain excluded.
- [x] Native probe total: Haruna5,613 sections/903,060 faces (+1,895 sections/+341,388 faces over prior build); showroom176 sections/36,794 faces (three stored foliage quads added). Remaining3,182 fourth-array packets have separate instance/bone parameter semantics and are not automatically treated as identity static draws.
- [x] Native alpha audit:450 referenced primary identities;441 available images (354 opaque,87 partial-alpha), zero fully-transparent images;26 sections lack a decoded primary image and remain neutral. Thus alpha cannot explain all missing geometry. Viewport already supports Ignore texture alpha(show textures opaque), bypassing discard/blending opacity for diagnosis; actual fragment shader discards alpha<0.01 and otherwise blends, so partial alpha can still affect visibility.
- [x] Focused Qt run21 passed/0 failed/0 skipped,54,733ms (19 methods plus setup/cleanup). Exact Haruna5,613/903,060 and billboard1,100 counts,441-image alpha audit, malformed reference/size and nonzero-W rejection; showroom Render All176/36,794, Haruna scan/textured preview, cache/budget and original static/UI/alpha-toggle regressions pass. Development EXE rebuilt and billboard provenance marker/SHA verified; build/windows-local/blur-geometry-alpha-receipt.json. Logs blur-geometry-alpha-tests.txt / blur-scene-app-build.txt. App left closed; full Haruna Render All/GPU appearance not tested.
- [ ] Next Codex: owner's visual recheck after rescan; source extra-array instance/animation transforms, billboard camera orientation and original render states/shaders. No whole-map completeness or alpha-perfect claim.
- Evidence/handoff: docs/research/blur/HARUNA-GEOMETRY-AND-ALPHA-2026-11-17.md; private haruna-probe.json, haruna-alpha-audit.json, check_float4.py and original-reader disassembly in Codex workspace. No game pixels/source binaries added, interactive app launch, commit/push/release.

## Blur Haruna empty scan / large scene loading — 2026-11-17 (tested; development EXE updated)

- [x] Reproduced on owner's exact levels/jpn_mharuna/Level.levelstream(307,269,632bytes), paired Level.levelrender(7,434,653bytes). Raw native parser found3,718 sections/561,672 faces while filesystem wrapper returned0. Root cause: companion requested in a single readPage call, whose hard limit is1MiB. Showroom companion978,033bytes hid the defect.
- [x] Added shared paged readComplete helper used by scan and decode; preserves size/mtime/cancel checks and surfaces companion read errors. Removed decoder's fixed128MiB cap in favor of configured asset budget/4. Low configured budget still rejects explicitly.
- [x] Shared immutable parsed-scene session keyed by both file revisions and pending patches avoids reading/parsing307MiB per mesh. Decoded QImages are shared by exact resource identity with a bounded cache, so repeated sections can share retained image storage. Native paired scene discovery bypasses unrelated whole-file heuristics.
- [x] Standalone production-header probe:441 blocks,221 base buffer groups,3,718 admitted sections/561,672 faces; paged filesystem wrapper now returns3,718 candidates. Representative sections0/1859/3717 decode with images. Snapshot remains a partial static subset: additional22 RenderLists and1,102 Float4-position packets not admitted by current reader.
- [x] Focused Qt run18 passed/0 failed/0 skipped,25,433ms (16 methods plus setup/cleanup). Real discoverAssets returns3,718 Haruna rows/561,672 faces; offscreen application scan and textured preview pass. Companion paging, large source decode, session/image reuse, budget/cancel rejection and all previous Blur focused regressions covered. Development EXE rebuilt and native fast-path marker/SHA verified; build/windows-local/blur-haruna-receipt.json. Logs blur-haruna-tests.txt / blur-scene-app-build.txt. Left closed; full Haruna Render All/GPU appearance not tested.
- [ ] Next Codex: owner's Haruna rendering check; remaining RenderList/Float4/instance formats, actual full-map completeness and runtime material/LOD behavior. No full-map or100% material claim from the static subset.
- Handoff: docs/research/blur/HARUNA-LARGE-SCENE-2026-11-17.md; source blur_scene.h / blur_scene_geometry.inc / asset_discovery.h. Private probe_haruna.py, probe_scene.cpp and haruna-probe.json in Codex workspace. No interactive app launch, commit/push/release.

## Blur scene base-colour textures — 2026-11-17 (tested; development EXE updated)

- [x] Native material descriptors retain type7 slots; draw stream0 parameter -> RenderList +0xb0 pair -> exact resource(group,index). g_tAlbedo is preferred; g_tTexture used only as explicit alternative. No name/proximity/hash matching. Nonnegative local groups supported; unresolved globals left pending.
- [x] Unified bounded mixed-resource image catalog follows empty texture updates and complete base buffer arrays; largest decoded image variant selected for preview, original runtime mip/LOD selection remains unverified. Tightened image successor to complete serialized resource/image prefix, resolving false RA signatures inside DXT data (light_paint_01 group102). Existing89 image-inspection recipes retained.
- [x] Recovered missing UV declaration type9 (8-byte half4); decode XY as primary UV0, preserving source coordinates. Type8 half2 remains supported. Independent research:217 texture identities,96 primary identities referenced by all173 accepted showroom sections. All173 now have resolvable base-colour and UV0; not full game shader coverage.
- [x] Focused Qt tests16 passed/0 failed/0 skipped,20,296ms (14 methods plus setup/cleanup). Actual images verified on all173 Render All entries; representative exact image names,217-entry catalog,96 referenced identities, corrupted padding/cancel and prior Blur regressions covered. Development EXE rebuilt and new material marker verified; receipt build/windows-local/blur-scene-material-receipt.json; logs blur-scene-material-tests.txt / blur-scene-app-build.txt. Application left closed.
- [ ] Next Codex: inspect textured rendering with owner; original additive/alpha/detail/normal/specular/vertex-colour shader behavior and runtime variant selection, remaining scene packet kinds and race-map validation. No complete-map/full-material claim.
- Evidence: docs/research/blur/NATIVE-SCENE-TEXTURES-2026-11-17.md; source src/blur_scene.h and src/blur_scene_geometry.inc; private check_material_coverage.py / scene-material-coverage.json. No game assets committed; no push/release or interactive app launch.

## Blur native scene geometry — 2026-11-17 (tested; development EXE updated)

- [x] Recovered original PC mixed resource vertex/index serializers and native RenderList prefix through its index/vertex reference tables. Parser derives offsets from the paired directory, ordered resources, declarations and draw records; no fixture offsets or emulation snapshots in production.
- [x] Added src/blur_scene_geometry.inc, integrated through blur_scene.h, asset_discovery.h and mesh_profile_decode.h. Scan for 3D Data now emits native scene section candidates; decoder reparses sources, compacts used vertices, retains half UVs and generates normals. Identity world matrix is required; each strip range resets independently and validates exact min/max indices.
- [x] Independent Python comparison against isolated original-reader snapshots: showroom first packet arrays179 records;173 admitted Float3 sections /36,750 nondegenerate faces;6 Float4/billboard records excluded. Six resource groups recovered end-to-end. All referenced instance matrices of this subset are identity.
- [x] Focused native tests15 passed/0 failed/0 skipped,16,631ms. Offscreen Render All173 meshes/36,750 faces; second click no duplicates. Corrupt used indices/cancellation/stale identity/RAM context and five representative native decodes covered. Development EXE rebuilt, native profile marker/SHA receipt verified; left closed. Logs build/windows-local/blur-scene-geometry-tests.txt and blur-scene-app-build.txt; receipt blur-scene-geometry-receipt.json.
- [ ] Next Codex: additional47 fourth-array packets,138 alternate records, mixed textures/nonzero node groups, nonidentity placement and exact original per-draw material/variant binding. No complete race-map support claimed; geometry is currently the validated static scene subset. Claude UI work need not infer materials from names.
- Evidence/handoff: docs/research/blur/NATIVE-SCENE-GEOMETRY-2026-11-17.md; private scripts under Codex workspace blur-research. No extracted game assets added; no app launch, commit/push/release.

## Scope and evidence

Read-only study samples: `C:/Users/Jarky/Documents/study cases/blur`.
Read-only full PC installation: `F:/copia de seguridad 08.04.2019/downloads/blur/Blur`.
Private probe scripts/reports/previews: `C:/Users/Jarky/.codex/.chatgpt-projects/g-p-6aafac188d248191a492c99f58d0a766/blur-research/`.
Source game data, decoded images and third-party executables are not included in this repository.

| Fixture | Bytes | SHA-256 |
|---|---:|---|
| audi_s3_stock.model | 4,343,421 | ecd921ef2281b64c2974948a155c80ba780ddba14360a2d38def767e2fe7b521 |
| igm_japan_minor2.model | 233,288 | 1702bf3556907c8c3a85efe118acd5e04a7311fedd6de45197c420bedb3f7cd3 |
| level.levelstream | 24,725,504 | 7c0d17eb35c3a0969f7903a97fc478cf7ddb6c88d562d4f8da985c36c24541ee |

The study stream is byte-identical to the installation's `levels/showroom/level.levelstream`.
It is the showroom, not evidence of a complete race track. The named minor character is a skinned
female character, not an ideal first static-mesh fixture. The installation also has a separately
named wheel resource for the Audi: do not confuse the body and wheel files sharing one basename.

## Format inventory

Counts are files in this particular installation, not counts of supported assets.

| Format/family | Count | Observed structure / purpose | Current result and remaining work |
|---|---:|---|---|
| cache/*.pak | 7 | All observed `2KAP`; six v2, shaders-speed v1; alignment 2048 | Name recovery and two directories validated; one v2 model decompressed exactly. Full native adapter and codec variants pending. |
| *.model | 1,544 | 1,495 start with `  CPModel`; 49 start with `Model` | Two CPModel chunk trees traversed to EOF. Parse raw Model variant separately; no meshes decoded yet. |
| *.texture | 301 | `Texture` chunk prefix | Standalone texture schema needs validation; do not assume DDS wrapper. |
| *.dds | 139 | Existing standard image family | Candidate for existing DDS pipeline; no corpus decode performed this session. |
| *.level | 19 | `  CPLevel` | Metadata/container, not standalone mesh stream. Showroom chunks validated to EOF. |
| *.levelrender | 19 | `  CPRender`, Scene with ARCH data | Shader/material names and 15 bounded inline DXT records in showroom; 184,287 trailing bytes need interpretation. |
| *.levelstream | 19 | ARCH serialization, repeated aligned segments | Showroom has 31 paired ARCH signatures; starts aligned to 2048. Segment lengths/references and streaming payload association unresolved. |
| *.levelphysics | 19 | `  CPPhysic`, Header/Main/Routes | Showroom outer chunks validated. Collision reader must remain distinct from visual geometry. |
| *.skeleton | 20 | Binary hierarchy candidates; observed leading versions 6/7 | Bone names also visible in character ARCH data; hierarchy/bind-pose layout pending. |
| *.cycle | 805 | Observed leading integers 14/15 | Animation candidates; channel/keyframe layout pending. |
| *.bpr | 4 | `2RZB` | UI resource family by paths; investigate separately, not as CPModel geometry. |
| *.dat | 367 | Mostly `JBOA`, plus unrelated files | Serialized gameplay/config candidates; classify by magic/path, never extension alone. |
| *.ipf | 75 | UI-associated paths, binary headers | Low-priority UI resource research. |
| *.baf / *.bik | 126 / 31 | Audio-associated BAF paths / video files | Outside initial 3D extraction scope. |
| *.pf / *.particlemesh | 5 / 1 | Particle-related candidates | Defer until static meshes/materials work. |
| shaders-speed.pak | included above | 1,256 declared entries, recovered names include shader files | Later shader role/constant research; no shaders executed/decompiled here. |

## What is actually verified

### PAK container and compression

- Read local `cache/blur.bms` (QuickBMS Blur script 0.3.3) as a reference, not executed.
- Independent bounded Python probe recovered readable name tables for all seven PAK headers.
- Native data directory parsing verified 3,601 entries in root.pak (3,388 data-bearing) and 931
  in boot.pak (718 data-bearing): name offsets/null terminators and data start offsets checked.
  This does not yet verify every compressed payload extent or CRC.
- A segmented deflate/zlib path recovered igm_japan_minor2.model from root.pak to exactly 233,288 bytes,
  matching both the extracted file and study fixture SHA-256. This is one successful payload,
  not proof that every compression variant is solved.
- Opposite-endian `PAK2`/XMem variants are described by the local script but NOT observed or validated here.
- The Xentax attachment `6778/blur_loc_ext.7z` lists only blur_loc_ext.exe; it was not executed.
  It is not a mesh import recipe.

### CPModel and ARCH

The two CPModel samples have a 20-byte outer header; child records have an 8-byte padded tag,
little-endian total length including the 16-byte child header, and a flags word. Their children
cover the entire file with bounds checked. Nested children were admitted only when they cover
the complete parent payload; arbitrary embedded strings were not accepted as chunks.

Audi top-level children: Header, MdlDat, Render, Coll, MtaDbg. Character has the same family.
MdlDat exposes StrTab/Models/Element/Constr; Render contains nested Render/Header/Scene and ARCH.
Keep chunk framing separate from ARCH object serialization; `RA` markers are not proven mesh tags.

Audi metadata includes body, door, wheel, light and interior names and veh_Paint/Glass/Light/Alloy
shader families. Showroom includes RenderingData::CullNode/RenderListNode_Common, g_mWorld,
g_vUVOffset, g_tAlbedo, g_tLightmap, g_tNormal and effect names. These are useful evidence for
scene/material links, not a validated assignment algorithm. Never equate a string occurrence
with a complete buffer descriptor or reuse PGR sampler/layout rules automatically.

### Images

A conservative structural probe validates seven inline DXT records in each study model and 15
in showroom levelrender. Only records with plausible power-of-two dimensions, depth 1, bounded
mip count, exact declared size and complete block payload are counted. Other DXT occurrences
remain unresolved; these counts are NOT the total texture count or material coverage.

At the observed inline FOURCC offset F: hash at F-24, record size F-20, width F-16, height F-12,
depth F-8, mip count F-4; data begins F+12. Declared size equals 28 plus complete mip-chain bytes.
Use this only after the enclosing serializer confirms the record; signature scanning is research.
The first character texture (128x128 DXT1, 8 mips) was wrapped in a temporary DDS and decoded;
face/hair atlas preview is coherent. Private output remains outside repo.

Showroom stream references instead declare size 28 with a sentinel at F+4; there is NO inline
pixel payload under this schema. Resolve external/streamed payloads before decoding them.
Do not treat bytes following the header as pixels, or guess textures by nearest offset.

## Ordered implementation checklist

1. [x] Inventory study fixtures and full-installation formats, preserve hashes and private probes.
2. [x] Verify CPModel outer framing, conservative inline DXT layout and one actual decoded texture.
3. [x] Verify PAK names/directories and exact decompression of one model without old executables.
4. [ ] Native bounded CP chunk reader: distinguish CPModel/Model/CPLevel/CPRender/CPPhysic variants;
   preserve unknown chunks and reject overflow/truncation. Investigate levelrender tail.
5. [ ] ARCH serializer: map IDs/references, object types, arrays, versioning and segment boundaries.
   Record source offsets and avoid a generic scan of float triples as an importer.
6. [ ] First static mesh: use system envsphere.model or another small prop, then Audi. Recover
   vertex declarations/strides, index type, draw ranges, primitive topology/restarts, bounds,
   coordinate basis, UVs and normals. Validate each index; do not inherit PGR layouts blindly.
7. [ ] Material graph: draw -> material -> named shader inputs -> image identity -> pixel payload.
   Resolve standalone Texture records, inline versus streamed records, mips and alpha/samplers.
   Start with albedo only; retain lightmap/normal/environment roles separately.
8. [ ] Audi assembly: preserve named parts, LOD/damage alternatives, wheel companions, local matrices.
   Compare stock body and wheels; no automatic loading of every overlapping variant.
9. [ ] Scene assembly: join the showroom level/levelrender/levelstream by validated references,
   then one race track. Recover per-instance matrices, streaming resource IDs and LOD selection.
10. [ ] Optional native PAK reader: port only observed variants, retain source paths and bounded
    RAM resources, add limits/cancellation/source identity checks. Full decompression corpus next.
11. [ ] Wire verified meshes directly into Scan for 3D Data / Render All and normal export path;
    images into 2D scan. No AI or external converter required for recognized layouts.
12. [ ] Synthetic malformed-input tests plus actual static prop, Audi/wheels, showroom and one
    track; finite vertices/UVs, indices, no cross-draw faces/duplicate LODs, GLB/FBX and live Vulkan.
13. [ ] Later: character skinning, skeleton/cycle animation, original shaders, collision and particles.

## Acceptance and current limits

No vertex or triangle reconstruction, material application percentage, 9XD rendering or export
is claimed in this research pass. No application code changed or build was needed. Geometry
and material binding are the main next tasks; full-installation raw resources mean PAK extraction
does not have to block the first mesh prototype. Existing BC codecs/shared renderer are reusable;
Blur serialization, declarations and links require their own validated adapter.

For future progress use separate columns: vertices, faces, compression, material-assigned parts,
decoded images, placements/LODs, export and live validation. Material percentage must specify
fixture and denominator; decoding a texture is not assigning it to a model.

## First native integration receipt — 2026-10-27

This supersedes the reconnaissance-only status above without marking the full ordered checklist complete.

- Implemented `src/blur_model.h`, discovery in `src/asset_discovery.h` and dispatch in
  `src/mesh_profile_decode.h`. Recognizes complete CPModel framing and a Scene ARCH v1 carrying
  exactly one supported typed stride-20 position buffer, UInt16 index buffer and a verified
  full-buffer primitive-5 draw. Padding is bounded and the vertex blob's typed successor is
  checked, avoiding ambiguous marker matches after zero-prefix vertices.
- Finite positions, valid indices, bounded input/memory, patch-aware reads, cancellation and
  source snapshot recheck; degenerate strip triangles omitted with parity retained. Source basis
  preserved, normals generated; UVs, original packed normals and material binding NOT implemented.
- Real envsphere.model and specsphere.model: each 401 vertices, 1,090 indices and 760 triangles.
  Independent cross-product/centroid probe finds all 760 sphere triangles outward. Other ARCH
  layouts, multi-buffer Audi and skinned character are deliberately not admitted as meshes.
- Inline DXT1/3/5 records exposed to 2D image scanning using existing codecs, bounded parent Scene,
  dimensions and exact mip-chain size. Seven images each on Audi/character decode at expected
  dimensions in production decoder. No images assigned speculatively to material slots.
- Focused suite: 6 passed / 0 failed / 0 skipped (four test functions plus lifecycle), log
  `build/windows-local/blur-tests.txt`. Synthetic malformed/truncated/nonfinite/index/cancel/stale
  candidate checks; actual sphere geometry, GLB/FBX export; actual 14 image decodes and malformed
  image header; asynchronous scanner-to-scene load and repeat-preview deduplication.
- These UI checks use Qt offscreen, not a live Vulkan framebuffer comparison. Full regression
  and application build receipts are recorded in COORDINATION-PLAN.md.

### How to try the first slice

Open installation `cache/root/dontship/models/system/envsphere.model` (or specsphere.model),
then Scan for 3D Data; the single mesh can be previewed and exported. Open the original Audi
or character model and Scan for Textures / Images to inspect the seven supported inline images.
Their 3D scan reports unsupported layout until the multibuffer/skinning stages are implemented.

### Next implementation priority

Map ARCH declaration/buffer identities and per-draw vertex/index/material references for the
Audi before supporting multiple buffers. Do not pair arrays merely by proximity, ignore LOD
boundaries, or report a car as supported based on these sphere tests. Native PAK integration,
standalone Texture, streamed payloads, scene assembly, skinning and shader fidelity remain open.
Final validation: full window suite 124 passed / 0 failed / 86 optional skips; development binary_workbench.exe rebuilt successfully. A prior About icon fallback regression was repaired without changing Claude's animation work. No live Vulkan screenshot comparison, release, commit or push performed in this slice.

## Live scenery props — 2026-10-27

The existing reader was tested live via Scan for 3D Data in 9XD's RTX 4060 Ti Vulkan viewport:

| Shared construction resource | Vertices | Triangles | Live result |
|---|---:|---:|---|
| trashcontainer_small.model | 100 | 52 | Closed bin, coherent geometry, rendered |
| dam_dirtyconstructionequipment_b.model | 385 | 302 | Open construction skip with lifting eyes, coherent geometry, rendered |
| concreteblock1.model | 24 | 12 | Private structural probe only; not loaded live |

Source folder: installation cache/root/models/objects/shared/construction. The app remains on
its second rendered object. No base images are assigned yet; the viewport explicitly warns of
missing source color texture. These are scenery props, not extracted track surface or proven
placements in a particular course. This supersedes the earlier lack of live Vulkan validation
for this narrow static profile, not the outstanding full-scene work.

A search of 117 eligible scenery resources stopped at three full-single-draw candidates.
Japanese lamposta/b and several other props have plausible buffers but no full-draw tuple;
actual lamposta scan correctly yields no mesh. Extend per-draw semantics before admitting them.
No decoder change or rebuild was needed for this live check.
## ARCH research continuation — 2026-10-27

See [draw/declaration/texture findings](ARCH-FINDINGS-2026-10-27.md). This corrects the earlier
external-only hypothesis for size-28 texture records: many have locally resident split RA blobs.
Independent probe decodes 854 showroom base images (including repeated records), with 213
full-chain/641 base-only stored layouts; 17 DXT occurrences remain unresolved. Candidate keys
are not globally unique: 42 key/format/dimension combinations produce different RGBA pixels.
No speculative binding added.

Lamposta: nine ranges exactly partition 5,292 indices; minimumVertex must not be added as a
base vertex. Stream byte offsets resolve its 3,055-vertex pool; independent strip decoding gives
2,063 triangles, removing 16 spurious boundary faces from aggregate interpretation. Audi contains
100 bounded packets of four ranges, showroom 226 single-range candidates; these are structural
counts, not admitted native meshes or a rendered scene. Several typed vertex declarations and
coherent half UV streams are documented. Production reader remains unchanged in this pass.

Next: native split-image support with bounded tests, counted draw arrays and exact object/buffer
references, source material mapping with namespaces, then showroom assembly. Keep all LOD and
state alternatives separate until their selection is known.
## Named-albedo preview and split images — 2026-10-28

Implemented for the already admitted single-static-draw profile:

- Float16x2 UV0 at vertex offset16/stride20, source coordinates retained. Invalid/nonfinite UVs
  preserve geometry but disable UV/texture assignment; no blanket U/V flip introduced.
- Image names are tied to their exact serialized image object boundary (RA v0/type2 counted
  string followed by RA v1 image definition), not to an untrusted global 32-bit identifier.
- If and only if the file has one unambiguous image named *_diff / *_diffuse / *_albedo and the
  admitted mesh has finite UVs, it is used as a named color preview. **This is an approximation
  from resource naming, not a recovered original GPU binding.** Metadata explicitly records
  textureAssignmentVerified=false, gameShaderApplied=false and the original name/offset.
  The preview is unlit; no ipr/normal/environment map is substituted. Ambiguous names stay unbound.
- The raw ARCH stream image scanner now reads bounded split blobs, either full resident mip
  chains or base-only payloads. First 128 image recipes are listed, preserving offset identity;
  this is not a claim that all 854 privately decoded records are listed or mapped to materials.
- No map geometry, per-instance assembly, multi-draw or skinned mesh support added in this slice.

Changed src/blur_model.h and tests/window_tests.cpp. Focused suite: 8 passed/0 failed/no skips,
including named diffuse images on both construction props, finite UVs, invalid-UV geometry
preservation, ambiguity rejection, textured GLB export, three actual split-image decodes, corrupt
length rejection and cancellation, plus existing geometry/images/UI tests. Log:
`build/windows-local/blur-material-preview-tests.txt`. Full regression/build/live receipt follows
in COORDINATION-PLAN.md. Source game files unchanged; no release/publication.
Final validation: full window suite 126 passed/0 failed/86 optional skipped; development executable
rebuilt. Live normal Scan for 3D Data loaded dam_dirtyconstructionequipment_b.model in Vulkan
with visible color texture, 385 vertices/302 triangles. Left open for owner review. Original
GPU bindings and complete scene assembly remain pending. Next native owner: Codex, counted
draw arrays and exact buffer/object/instance references before full-map material assignment.

## Audi UV comparison follow-up — 2026-10-28

Owner flags prop UV appearance. Audi is present but still not renderable by the current native
profile. Broader read-only attribute scan finds 12 declarations (strides12–40), including
UV semantic5 type9/channel0 and additional channels; type9 encoding and blob/reference
boundaries remain unverified. Seven exact printable image names cover tyres/brakes/damage
resources rather than one car-wide diffuse map. Do not apply the prop stride20/offset16 rule
or assign one image to the whole car. Next: exact typed blob and draw/material references,
then car UV checker/source-texture comparison. Prop UV concern remains open. No app changes.

Additional live UV fixtures (2026-10-28): small bin textured 100 vertices/52 triangles;
concreteblock1 textured 24/12, left visible. Owner finds bin plausibly correct. Concrete
appearance coherent but insufficiently asymmetric to establish orientation. Shader 1-V
versus missing Blur sourceUvTopLeft flag is a hypothesis to validate against material/game
reference, not an applied fix. No code/build change; preserve existing source UVs for now.

## Pallet and standing-light admission — 2026-10-28

No native scan results because current profile requires one full-buffer draw. Pallet variants
1–4 have 9/9/10/9 draw ranges; standinglight and standinglight2 have 10 each. Private validation
confirms exact index partitions, minimum/count and byte-offset bounds. Per-draw strip resets
yield 526/738/1268/1930 pallet triangles and 2610/2730 light triangles; an aggregate strip
adds 16–18 false boundary faces. Next: tested counted draws plus exact object/state/material
references. Geometry is present; fully assembled textured preview not yet supported. No app change.

## Native local-section support - 2026-10-28

Implemented per-draw candidates for a single typed stride20 vertex/index buffer: exact index
partition, minimum/count checks, bounded stream-byte offsets and independent strip parity.
Unused vertices are compacted for correct section framing/export; source vertex offsets remain.
Render All loads sections independently with unique source identities. Multi-draw textures
remain unassigned: original material links, dynamic object transforms/states and LOD selection
are still pending. This is a local-section preview, not proven complete game-object assembly.
Focused suite: 10 passed/0 failed/no skips, six actual models plus malformed range/overlap/offset
rejection, stale candidate, existing single-draw behavior and scanner Render All/dedup test.
Log build/windows-local/blur-multidraw-tests.txt. Full regression and live validation pending.

Final validation: 128 passed/0 failed/86 optional skips, executable rebuilt/installed. Live
Vulkan scan + Render All verified pallet 9 entries/526 triangles and standinglight 10/2610,
zero failed or omitted, existing first section deduplicated. Both recognizable; lamp active
and pallet in adjacent document. Lamp all-sections view overlays bent/alternative components,
so dynamic state selection is explicitly unresolved. Expected texture warnings: multi-draw
material assignment remains pending. No complete-state/full-map fidelity claimed or release made.


## Executable serializer follow-up — 2026-11-16

Research-only receipt: [original reader findings](EXECUTABLE-SERIALIZERS-2026-11-16.md).
CullNode reader/writer and inherited Node field order traced in the PC executable;
full record/object identity and RenderNode/MaterialNode graph still pending.
No new app support or complete map render claimed. Next owner: Codex/native analysis.


## Bounded nodes and parameter layouts — 2026-11-16 follow-up

- [x] 3,082 named CullNode bodies across38 scene files accepted by bounded reader; four showroom endpoints agree with isolated original-reader emulation.
- [x] First showroom RenderList material parameter descriptions decoded, including typed fields and offsets; generic value reader dispatch traced.
- [ ] RenderList complete traversal, type7 resource lookup, actual draw/material/texture links and full map assembly remain pending.
- Evidence and exact limits: [node/parameter progress](NODE-PARAMETER-PROGRESS-2026-11-16.md), coverage CSV and schema JSON. No app code/build change.


## Current resource-link result — 2026-11-16

- [x] Four showroom RenderList file paths consumed;262 parameter blocks,329 local texture slots and815 redundant texture dependencies checked.
- [x] Original two-level resolver traced: local slot -> (resource group, texture index), with global scope separate. First named link: LightFlare g_tColour -> global0 lightflare_diff.
- [x] Scene header:145 allocated groups,23 shader names,16 declared globals. Allocation is not decoded group coverage.
- [ ] Actual source group-to-stream locations/contents, remaining global images and packet-to-buffer/parameter binding; alternate138 records need semantics.
- Receipt: [resource links](RESOURCE-LINKS-2026-11-16.md); validation JSON alongside it. Earlier TLS1151 limitation is superseded for the four tested RenderList file paths only; no new production app support.


## Stream locations and texture variants — 2026-11-16

- [x] Actual31-record directory recovered from levelrender; exact contiguous stream coverage and all ARCH pair counts validated.
- [x] Ordered(group,variant) subrecords identified; do not iterate unique group IDs or apply last-copy-wins.
- [x]20 complete texture-only blocks +5 partial prefixes,582 decoded images,663 inherited entries,89 named pairs;96/329 reviewed texture parameter slots resolve to names (not mesh coverage).
- [ ] Mixed resource buffers, partial/compressed updates, correct variant selection, packet/material/geometry binding and full-map/native integration.
- Receipt: [stream directory and variants](STREAM-DIRECTORY-AND-TEXTURE-VARIANTS-2026-11-16.md); directory/validation JSON alongside it.


## Native texture integration — 2026-11-16 current

- [x] Native directory/ordered texture variants wired to discovery; filesystem stream automatically uses same-folder render metadata.89 named current-document recipes, no original mesh material binding claimed.
- [x] Integration test corrected pixel alignment:25 complete texture blocks,712 nonempty records,868 inherited; prior20/5/582/663 receipt counts superseded. All89 production image decodes pass.
- [x] Focused12 passed/0 failed/0 skipped including prior CPModel/UI paths. Build/link status in coordination plan.
- [ ] Mixed-resource geometry, original variant choice and actual textured map draw. Receipt: [native integration](NATIVE-TEXTURE-INTEGRATION-2026-11-16.md).


### Quick object preview — 2026-11-16

Textured software preview of trashcontainer_small.model generated/inspected:100 vertices,52 faces. Workspace blur-research/quick-render/blur-object-render.png. Single existing-compatible scenery object; not a map or new scene-link validation. Native mixed-buffer/map work remains pending.


### Owner scan clarification — 2026-11-16

Owner used Scan for3D Data on showroom level.levelstream; empty model list is expected until mixed scene buffers are integrated. Current paired-scene integration is exposed via image scan. Standalone CPModel preview is not evidence of map support.
