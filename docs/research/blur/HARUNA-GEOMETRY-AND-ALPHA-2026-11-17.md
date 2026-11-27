# Haruna geometry gaps and texture alpha

2026-11-17. Codex. Focused native validation passed; development executable updated.

## Proven missing geometry

The previous static reader accepted76/98 RenderLists.22 were rejected at parameter type8/9, not because their geometry buffers were absent. Original typed-field reader0x6d96e0 dispatches8/9 to framed packed references0x6d61c0. A reference is12 serialized bytes versus4 logical parameter bytes; native declared-size validation now accounts for8 extra framing bytes per reference. Type10 retains a4-byte scalar width. Further19 lists then exposed nonempty extra descriptor/value arrays: original0x6e9800 ->0x6e90e0 ->0x6e8e60 reads descriptor tables AND parameter values for each array entry, not descriptor tables alone. Both schemas are now consumed strictly.

All98 prefix paths reach their reference tables. Float3 static geometry grows from3,718/561,672 to4,513/704,254(sections/faces). No offsets hardcoded, no guessed faces.

## Stored billboard geometry

The first arrays additionally contain1,102 Float4-position draws:895 BillBoard_LN.fx,205 FoliageBillBoard_LN.fx,2 LightFlare_LN.fx. Sample/aggregate examination finds zero W on all referenced vertices, finite XYZ and nonzero stored triangle areas. Admit only the two named billboard layouts, identity world transforms and zero-W positions with declaration/range checks. Native uses the stored XYZ quads, not generated tree volumes. Light flares and unrelated Float4 layouts remain excluded. Billboards export double-sided/alpha-masked, and provenance marks billboardFacingApplied=false. Their orientation is fixed until the original camera-facing shader is reproduced.

Haruna now5,613 sections/903,060 faces; previous3,718/561,672 missed1,895 sections/341,388 faces. Showroom gains three foliage quads:176 sections/36,794 faces.3,182 fourth-array packets reference different instance/bone parameters (including an empty primary instance block); do not assume they are identity static geometry or silently duplicate their source buffers. Their transform resolution remains pending.

## Alpha audit and diagnostic control

Production native catalog/texture decoder audit across admitted Haruna sections:450 referenced identities,441 available decoded primary images,354 opaque and87 with partial alpha; zero images entirely below the viewport alpha discard threshold.26 sections have no decoded primary image and use neutral surfaces. Missing images therefore do not automatically remove their mesh.

shaders/mesh.frag discards samples below0.01 alpha and output is source-alpha blended. Partial transparency can still hide individual pixels or soften whole surfaces; zero fully-transparent images does not prove all original alpha semantics are correct. Existing3D View Settings -> Ignore texture alpha(show textures opaque) forces sampled alpha1, changes preview only and is suitable for a before/after check. Do not globally remove valid cutouts from trees/fences as a geometry repair.

Private metadata audit haruna-alpha-audit.json, sequential probe_haruna.py/haruna-probe.json, check_float4.py and original executable disassembly in Codex workspace blur-research. No extracted pixels or game binaries in repository.

## Validation pending

Focused tests cover framed reference sizes/corruption, updated real geometry counts, zero-W billboard rejection, image alpha statistics, source decode/caching, UI scan/preview and existing alpha toggle/Blur regressions. Final results/build receipt appended after execution. No interactive application launch, commit/push/release.

## Final validation / executable

-21 passed/0 failed/0 skipped,54,733ms; build/windows-local/blur-geometry-alpha-tests.txt. Typed-reference framing/declared size corruption, exact Haruna counts, zero-W billboard rejection,441-image/87-partial/0-fully-transparent alpha audit and representative decodes covered.
- Offscreen showroom Render All now176 sections/36,794 faces with actual images; repeated operation does not duplicate geometry. Haruna scan/textured preview plus original CPModel/image/UI/cache/budget regressions and existing viewport Ignore Alpha setting pass. Full Haruna Render All and Vulkan appearance were not tested; no claim of complete map recovery.
- Development EXE rebuilt after main.cpp timestamp-only dependency workaround; new billboardFacingApplied provenance marker and SHA verified. Receipt build/windows-local/blur-geometry-alpha-receipt.json. App left closed; no commit/push/release. Full repository suite not rerun.
- Audit low-alpha examples are consistent with sparse cutouts: pylon01linesalpha_diff mean7.0/255 and97.3% below discard threshold; whitegrass_diff mean19.2 and86.4% below; fencewire_diff mean31.9 and87.5% below. This is descriptive evidence, not proof of every original render state. Keep the preview alpha override available for comparison instead of silently overwriting texture data.
