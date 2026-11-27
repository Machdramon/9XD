# Blur placed instances: prototype plus world matrix

2026-11-17. Codex. Native integration tested; complete admitted Haruna batch passed; development EXE updated.

## Missing geometry mechanism

RenderList first array stores direct world-space draws. Second array uses reader0x6e4900(memory stride20): version, prototype index, framed instance-parameter reference, binding overrides and flags. Fourth array reuses the draw serializer(memory stride48) for geometry prototypes. Reading fourth-array geometry with its empty primary instance block would wrongly place it at the origin; the second-array record supplies the actual world matrix. Counts in Haruna:98 lists,3,182 prototypes,22,966 placement records. Second bounds array count matches second placement-array count for all98 lists. This was verified sequentially, without proximity/name matching.

## Native implementation

List retains prototypes, instances, world parameter values and serialized instance bounds. resolve() follows exact prototype/buffer/material references. affine() requires finite, non-singular affine4x4 matrices. position() applies row-vector convention (translation members12..14). Negative determinant reverses triangle winding before generated normals. validate() checks every retained world vertex, finite source XYZ, zero W for the admitted Float4 quad layouts, exact strip ranges and world bounds.

Rigid defaults/HighVis geometry, SpeedTree bark, stored SpeedTree leaf quads and SyntheticBillboard quads are admitted. Source override stream1 is allowed only if its parameter block is empty and referenced buffer has stride4/type11/semantic9(offset0), valid offset and vertex ranges: this is auxiliary colour data, not replacement positions. Baked vertex colours, live wind and camera-facing leaf/billboard generation remain unapplied. Skin shader instances are explicitly pending.

Exact duplicate geometry placements(same prototype and normalized matrix) are skipped; aliases do not generate duplicate preview faces. Source LOD/pass behavior is not recreated: all admitted serialized geometry is exposed and may include distance/pass alternatives. No object family/LOD is guessed from names.

Candidate vertex counts now equal actual compacted retained vertices; using whole shared buffer counts per small section inflated budget estimates. Session adds direct identity lookup so per-mesh decode does not scan25k rows. File/patch revision checks and shared decoded-image storage remain active. Notes expose outstanding instance counts. Global texture references remain neutral, with explicit source binding status.

## Independent validation before application tests

Python analysis:19,837 placed sections/3,973,320 faces;3 duplicate placements;3,126 skeletal instances; no other exclusions. Ordinary rigid transformed vertices fit the matching file bounds within floating-point rounding (~0.00012 in sampled lists).2,576 leaf quad placements extend beyond posed bounds by up to0.729 units but at most4.539% of bound extent; a documented5% leaf tolerance preserves stored fixed-orientation quads. Other rigid objects retain0.05 tolerance. All19,837 accepted matrices have determinant approximately-1.

Example list876904 instance0 yields world bounds min(1188.362362,518.643306,-3115.142570), max(1193.469602,520.633788,-3111.133422). A regression checks native output against these independently calculated coordinates, not only total counts.

Production-header probe: Haruna25,450 sections/4,876,380 faces; showroom313/58,857. Haruna64 sections and showroom29 lack primary images because their exact texture reference selects the global scene table; geometry remains visible with a neutral surface. Previous static texture coverage remains checked separately. No substitute image is chosen.

## Remaining geometry and rendering

3,126 placements select skin shaders/palettes. Their raw prototype geometry and world transforms are present, but original bone matrices/pose evaluation remain unresolved. Do not silently assume identity bone palettes. Global scene images, source LOD/pass selection, wind/billboard animation and original shader/render states remain pending. Counts are admitted geometry, not proof of a complete original game scene.

Full Haruna Render All and focused tests are being run; final receipt appended after actual completion. No interactive app launch, source assets/pixels in repo, commit/push/release.

## Resume: shared image representation

Previous23 focused tests passed and EXE linked, but full batch had no completed receipt after usage interruption. A fresh large-scene run revealed that MeshScene::add converts every surface to RGBA8888, while the source decoder's shared cache retained ARGB32. Every conversion allocated a distinct image/cache key, so repeated trees consumed scene texture budget repeatedly. Normalizing the image once before adding it to the Blur shared cache preserves identity through Scene::add. New regression verifies two scene entries retain exactly one image allocation, not only that decoder results share a key. The incomplete repeat run was stopped to install this correction; full batch will be rerun.

Other format caches returning ARGB32 may require the same normalization contract or a general MeshScene conversion cache; leave that wider follow-up explicit rather than changing unrelated loaders during this Blur fix.

## Final resume validation

- Focused checks24 passed/0 failed/0 skipped,56,682ms (22 methods plus setup/cleanup). Includes typed parameter/reference checks, independent world-bound example, source counts, scene-level image identity/memory sharing, zero-W/alpha/source corruption cases, filtering/alternative/default selection and original Blur regressions.
- Separate completion-driven Haruna Render All3 passed/0 failed/0 skipped,66,107ms (one method plus setup/cleanup). Actual MeshScene contains25,450 pieces/4,876,380 faces, including19,837 placed sections. Global image references leave64 neutral pieces; geometry still loads. This is an offscreen application pipeline test, not a Vulkan screenshot/original-shader comparison.
- Private expected-candidate list was compared against successful GUI scene-add trace by exact model name/identity and each model's vertex and face count.25,450 expected and25,450 loaded;0 missing/unexpected/duplicate names and0 per-model geometry count mismatches. Private haruna-full-batch-validation.json records totals; source-backed expected identities remain outside repo with other analysis metadata.
- Texture normalization occurs once before Blur caching, so MeshScene's RGBA format conversion preserves the shared image/cache key. Two real scene entries retain one image allocation. Generic normalization for other format caches remains a separate follow-up.
- Bulk tree operations use contiguous selection ranges and model-selected rows. Existing visible/default/alternative/heuristic/part filters remain intact. Current index moves to the first eligible result, fixing the stale-active-row case when only one eligible result remains. No UI layout/appearance redesign. Initial bulk dispatch gap in the local trace dropped from26.7s to0.275s; event-driven test delivery also avoids polling-induced worker-to-GUI delays. These are local measurements, not hardware-independent guarantees.
- New source changes are src/blur_scene.h, src/blur_scene_geometry.inc, src/asset_scan_panel.h and tests/window_tests.cpp. main.cpp timestamp-only touch forced dependency rebuild; content unchanged. Build log blur-bulk-final-build.txt; executable/source hashes and checks in build/windows-local/blur-placed-instances-receipt.json. Application left closed; no commit/push/release. Full repository suite not rerun.
- Remaining3,126 skeletal placement records need original bone palette/pose evaluation;64 admitted Haruna pieces and29 showroom pieces need global images. LOD/pass selection, live wind and camera-facing quads remain unresolved. Do not relabel this as full original scene fidelity.
