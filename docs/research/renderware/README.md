# RenderWare native validation

Updated: 2026-10-26. Owner: Codex; reader authored by Claude. Current code: src/rw_core.h and src/rw_model.h. Scope: native geometry plus supported primary PS2 TXD images/material surfaces. Remaining shared-bank linkage, shader parameters and Selene placement are pending.

## Measured corpus

Read-only source folder: C:/Users/Jarky/Documents/study cases. Independent core walk and finite position/normal/UV/index checks:

| File | Groups | Vertices | Triangles | Finding |
|---|---:|---:|---:|---|
| d1gp/A31_C00.BIN | 69 | 12,890 | 16,275 | All checked values finite / indices valid |
| d1gp/GRANDPRIX_HARE_USA.D1 | 16 | 63,338 | 40,239 | Same |
| super trucks racing/CARS.MIX | 847 | 313,148 | 196,987 | 15 nonfinite normal components in three parts; positions/UV/indices valid |
| super trucks racing/ALBACETE.MIX | 593 | 266,377 | 101,577 | Values valid; differs from historical 128,409 count, so full coverage unproven |

Group counts are core atomic/clump groups, not UI candidate counts: the adapter combines atomics per Selene scene. No desktop visual equivalence is inferred from these checks.

## Nonfinite source normals

CARS source part offsets: 28485656, 32424010 and 34059636. The adapter previously rejected a complete scene when any source normal was NaN. It now zeroes the affected normal vector, regenerates it from existing faces, and reports repairedNormalCount and normalWarning. Positions and triangle connectivity are unchanged; invalid positions still reject the decode. Unused vertices get the existing neutral normal fallback.

Regression tests: renderWareClump (translation / truncated rejection), renderWareNonfiniteNormals (NaN-normal repair), localRenderWareCorpus (optional WORKBENCH_STUDY_CASES, actual scan/decode plus finite values and index bounds over four files). Final focused run: 9/0/no skips (includes all four real files through the native adapter), full reader 45/0 and full window 110/0/81 optional skipped. CARS repairs five normal vectors while preserving 196,987 triangles. See COORDINATION-PLAN.md for build status.

## Next

1. Investigate ALBACETE count discrepancy before claiming full track coverage.
2. Complete remaining shared MIX texture associations; base-level TXD/material partition is now implemented and tested.
3. Resolve Selene node hierarchy/placements and runtime wheel attachment separately.
4. Measure per-candidate repeated reads/parsing and add document-revision reuse if material.

Game inputs and legacy references are read-only and remain excluded from source/release packages.

## PS2 TXD and primary surfaces - 2026-10-26

Native implementation in src/rw_txd.h and src/rw_model.h. Bounded TXD dictionary/native/raster children, 64-byte metadata, pixel/palette extents and GIF transfer spans are checked. Base-level indexed transfer payloads are replayed through existing GS addressing with written-byte coverage; direct color uses bounded host-transfer pixels. CLUT order and PS2 alpha reuse the common image decoder. New raster support does not execute or depend on legacy scripts or gtf/Noesis tools.

Primary triangles are partitioned by material texture name. Matches stay inside the same source/scene; duplicate names in different livery dictionaries become optional texture alternatives. Default variant 0 is a visible preview default; runtime customization remains unverified. Render All excludes alternatives, preserving one copy of each recovered triangle. Native TXD previews appear in image scans; rendered images are added to the texture library and flow through existing GLB/FBX export.

Cache is per document snapshot/patch identity, with bounded decoded images. Source scope and texture receipts are revalidated. A stale/bad binding does not reuse another dictionary's image. Empty/unmatched material names retain neutral geometry. Existing RenderWare frame/topology behavior and invalid-normal repair are preserved.

Real raster counts: A31 56, HARE_USA 94, CARS 1,106. ALBACETE has no PS2 native TXD signatures in this sample; missing track textures are not repaired by a guessed match to the cars. Multiple CARS dictionaries carry different SideA/Chassis/Number pixel data under the same names; they are genuine alternative liveries. Independently inspected decoded BFGoodrich and spectator images are legible, without global UV/image flipping.

New tests: renderWareTxdMaterials; localRenderWareTxdImages; localRenderWareCorpus now checks default surfaces with a shared session. Validation results pending in COORDINATION-PLAN.md. Limitations: base mip; version-1 legacy swizzle and unsupported pixel/transfer patterns rejected; full shader/alpha-blend/environment rendering not emulated; source customization and Selene placement pending.

Format reference: [librw PS2 raster stream layout](https://github.com/aap/librw/blob/master/src/ps2/ps2raster.cpp). Used for format verification; decoder is independently written around existing 9XD image addressing.

### Final measured primary coverage

| File | Default surfaces | Textured surfaces | Textured / recovered triangles |
|---|---:|---:|---:|
| A31_C00.BIN | 122 | 112 | 16,184 / 16,275 (99.44%) |
| GRANDPRIX_HARE_USA.D1 | 106 | 101 | 40,047 / 40,239 (99.52%) |
| CARS.MIX | 247 | 100 | 88,162 / 196,987 (44.76%) |
| ALBACETE.MIX | 135 | 0 | 0 / 101,577 |

These measure primary image assignment, not correct runtime shader/livery selection or complete scene coverage. Some untextured surfaces legitimately have no texture name. CARS has 417 selectable candidates including alternate texture variants; only default surfaces enter Render All. Remaining named references predominantly occur in scenes with no local TXD, so linkage to other resources stays unresolved. ALBACETE's texture encoding/location remains unknown, not proven absent or external.

Final tests: reader 45/0; full window 111/0 with 82 optional skipped; TXD/new-reader focused 11/0/no skips; additional Genki corpus 7/0/no skips. All 1,256 TXD raster records decode at expected dimensions. Material partition preserves triangle totals; source normal repair still handles five CARS vectors. No in-game appearance equivalence claimed. Cache identity includes normalized TEX0 storage fields and sampler state; physical GS base pointers are excluded. Zero-origin base transfer required; version-1/multi-transfer/unsupported layouts are explicit errors.

All TXD image candidates are exposed, bypassing the older generic 128-preview truncation for this family. Each encoded preview reads the bounded native record rather than the file tail. GLB/FBX texture flow tested with a synthetic two-material model; native surfaces use the same export path.

Name lookup specification cross-check: [librw TexDictionary::find](https://github.com/aap/librw/blob/master/src/texture.cpp) compares names case-insensitively. Inputs and scripts remain read-only. Final build status lives in COORDINATION-PLAN.md.

Development executable successfully rebuilt/updated on 2026-10-26; final diff check passes. No publication or commit. Start visual review with A31_C00.BIN, then GRANDPRIX_HARE_USA.D1. Super Trucks CARS remains partial for shared/bank-less scenes, and ALBACETE texture location/encoding is still unresolved.


## Super Trucks MIX texture linkage — 2026-10-26 (current; supersedes earlier partial TXD status)

- [x] Codex: decode the original MIX TOC (XOR filename bytes with index*58), retain resource paths, bounds, compression flag and expanded size. Native independent LSB-first LZW (clear 256/end 257, late 9–12-bit widths) decodes compressed TXD resources in RAM. No legacy executable/script is executed or shipped. Reference inspected: author MIX format/extractor source at https://mirror.aluigi.org/papers/msmixext.zip; implementation is independent.
- [x] Bind by texture name AND original vehicle/track resource path, not physical proximity. Same-name vehicle liveries remain optional. Dry/rain banks and unrelated vehicle folders stay isolated. Original paths are now candidate names. Alternative SLF/BSP/weather geometry remains selectable but excluded from default Render All to avoid overlapping alternatives; this is a preview policy, not a proven game resource manifest.
- [x] Compressed TXD images are exposed to image scans/previews through native-rw-mix-texture, with original-source hash, bank path and raster identity checks. Source snapshot/session cache is shared with geometry; no extracted game data is added to the repo.
- [x] Geometry walk now follows bounded MIX resources, recovering BSP resources missed by the old Selene-only scan. ALBACETE total across all geometry alternatives is 128,409 triangles, resolving the earlier 101,577-versus-128,409 discrepancy. This total is NOT the default scene triangle count.

Measured native corpus (primary texture variant, all platform/weather geometry alternatives included): CARS 247 surfaces, 231 textured, 196,795/196,987 triangles with textures (99.90%); ALBACETE 137 surfaces, 117 textured, 84,991/128,409 triangles. CARS has 657 candidates with optional livery alternatives; ALBACETE 137. Render All defaults: CARS PS2 set 116/116 surfaces and 88,548/88,548 triangles textured; ALBACETE dry set 61/70 surfaces, 43,974/52,136 triangles textured (84.35%). Do not compare all-alternative totals to default-only percentages.

All 236 Albacete raster records decode with no bank warnings. Together with 1,106 CARS and 150 D1GP records, 1,492 sample rasters decode. Inspected decoded Albacete asphalt/road-marking image is legible. This does not establish in-game material/shader or UV equivalence.

Validation: reader 45 passed/0 failed; full window 112 passed/0 failed/83 optional skipped; focused real-corpus/LZW/material/isolation/source-mutation/new-reader checks 13/0/no skips; Genki context/WM/isolation/March 7/0/no skips. Synthetic cases verify nonadjacent car bank lookup, same names in different cars, dry/rain isolation, compressed preview and malformed/canceled/over-budget streams. Development windows-local executable rebuilt successfully; diff check passed. No commit/push/release performed.

- [ ] Native decoder owner: recover missing UV streams in Albacete. Nine default dry materials have matching decoded textures but no UV stream recovered for some contributing parts: ALBASFALTO01 (2,098 tris), ALBASFALTO02 (222), ALBASFALTOBOX (28), ALBERBA02 (1,997), ALBERBA03 (1,464), ALBERBA04 (1,236), ALBMURETTO (133), ALBSAND (326), ALBSAND_BORDER (658). Total 8,162. Do not report these images absent or apply a global UV flip; inspect native VIF layouts/batch UV coverage. Scan diagnostics now avoid reporting those surfaces as textured.
- [ ] User/Claude: reopen and RESCAN CARS.MIX/ALBACETE.MIX (old scan candidates have old scopes), then Render All. Visually verify default PS2 truck set, selected livery, dry track and optional rain geometry. Full Selene part placement, shader passes, blend/environment behavior, runtime livery choice and visual equivalence remain pending.

Affected implementation: src/selene_mix.h, src/rw_model.h, src/main.cpp, tests/window_tests.cpp. Current work continues the earlier rw_txd integration; preserve all other uncommitted contributions.

## Multi-track coverage follow-up - 2026-10-28

See [validated corpus report](SUPER-TRUCKS-TRACK-COVERAGE-2026-10-28.md): nine additional circuits plus Albacete control and two ancillary resources. All default geometry candidates and 2,237 TXD images decode without failure. Circuit triangle texture coverage is 72.12%-95.40%; all untextured default surfaces have image bindings but incomplete UV coverage. Next: native VIF UV streams/batch aggregation, especially BARCELL versus STADIO. No application changes or live GPU fidelity claim.

## Executable UV2 discovery - 2026-10-28

[Executable and packet evidence](SUPER-TRUCKS-EXECUTABLE-UV2-2026-10-28.md) identifies an omitted PS2uv2 V4-32/address1 stream. Private prototype yields 100% default UV+image readiness across all ten tested circuits without triangle-count changes. Production coverage above is unchanged until integration/tests and visual verification; primary pair/secondary shading semantics require validation.

## UV2 installed - 2026-10-28 (supersedes prototype-only status)

See [production integration receipt](SUPER-TRUCKS-UV2-INTEGRATION-2026-10-28.md). All ten
default circuits now have100% decoded base-image assignment, verified with actual production
mesh and texture decoders. Full suite129/0; installed build. BARCELL77 surfaces/20,138
triangles loaded live without warnings. Appearance is still partial: red/dark facade areas
need alpha/material state investigation; isolated RGB facade is correct but most alpha is0.
No complete shader fidelity, optional-alternative coverage or resolved car placement claimed.

## Geometry completeness correction - 2026-10-28 (supersedes sparse counts)

[DMA parsing and Render All receipt](SUPER-TRUCKS-GEOMETRY-COMPLETENESS-2026-10-28.md):
old parser mistook serialized DMA RET tag for VIF and lost entire/initial batches. All558
BARCELL main geometry blocks now emit; default106 surfaces/30,586 triangles. Every enumerated
main Geometry block across ten circuits emits after correction. All-results mode explicitly
includes alternatives and loaded206 BARCELL candidates/66,280 triangles live. Default and all
variant totals must remain separate. Full131/0, focused11/0; installed. Alpha appearance, small
metadata triangle discrepancies and exact Selene instance/placement remain pending.

## Viewport alpha diagnostic (2026-10-28)

3D View Display > Settings > Ignore texture alpha (show textures opaque) bypasses sampled-texture alpha clipping and forces sampled alpha to one. Default off; live preview only, source RGB/images/materials and exports unchanged. Split views and newly created viewports inherit the flag. This helps inspect Super Trucks EDIFICIO_1, whose alpha is mostly zero; exact game shader semantics remain unresolved. Full suite 132 passed / 0 failed / 86 optional skipped, shader and executable rebuilt and installed. Live BARCELL EDIFICIO_1 (3,092 vertices / 1,654 faces) changed from discarded faces to visible textured surfaces with the override. See the coordination plan for current follow-up.
