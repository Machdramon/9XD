# PGR3/4 executable unpacking and texture-binding audit

Updated: 2026-10-27. Research by Codex, requested by owner. Static analysis only: original game executables not executed or changed. No app decoder/rendering changes and no commit/push/release in this pass.

## Outcome and limits

Both default.xex files were decrypted and basic-decompressed successfully from the owner's copies. Valid XEX2 → MZ/PE PowerPC images; executable instruction streams disassemble at the mapped .text RVAs. Recovered embedded shader constant tables: **PGR3 330, PGR4 994** (tables, not necessarily distinct shader programs). Parsed big-endian 28-byte D3DX-style constant headers and 20-byte constant records; sampler register-set3 is distinguished from boolean0 and float2. Names/indices below are evidence from embedded shader metadata, not guessed from pixel colors.

This proves that shaders bind several different texture roles and that sampler numbers depend on shader family. It does **not** yet prove which SCNE serialized material field maps to each role for every material group. In particular source channel0 is NOT synonymous with GPU sampler s0. Do not apply a global channel/register remapping from this audit.

## Private local artifacts and validation

Private directory (outside the repository):
`C:/Users/Jarky/.codex/.chatgpt-projects/g-p-6aafac188d248191a492c99f58d0a766/pgr-xex-private/`.

For each game:
- `<game>.pe`: original expanded **mapped-memory image**, not a conventional raw-section PE. Runtime address = XEX image base + file offset. Raw offsets in its PE header do not describe its section positions.
- `<game>.analysis.pe`: derivative for static-analysis tools, section payloads copied from RVAs to PE raw offsets; ImageBase/entry corrected from XEX optional headers. Not a runnable/repacked XEX. Every nonempty section was compared byte-for-byte to the mapped image after conversion.
- `<game>-report.json`: input/output hashes, section table, addresses, relevant strings and sampler counts.
- `<game>-shader-tables.json`, `<game>-named-scene-shaders.json`: table offsets and parsed constants; nearby shader-name association is bounded to a unique table within512 bytes, not a runtime draw trace.
- `<game>-xrefs.json` and private text listings: selected PowerPC cross-references. Analysis-only Capstone package in python-deps. No keys, game binaries, shader microcode or disassembly listings are added to the repo.

The original XEX encryption and compression are both type1; basic decompression copies each data block and inserts declared zero blocks, with AES-CBC state preserved across packed data. This is independent local scripting following the documented behavior of [Xenia's loader](https://github.com/xenia-project/xenia/blob/master/src/xenia/cpu/xex_module.cc), not execution of either game.

| Game | XEX bytes | Expanded mapped bytes | Analysis PE bytes | Runtime base | Entry |
|---|---:|---:|---:|---|---|
| PGR3 | 6008832 | 9961472 | 5368832 | 0x82000000 | 0x8236ee60 |
| PGR4 | 12431360 | 14090240 | 11367424 | 0x82000000 | 0x8267ca00 |

Input SHA256:
- pgr3: `5e631714661cc2f6234903f1d34247ccbd5efe0332d25e4c4bb718f917982048`
- pgr4: `f9a411d8f1a1a3c38bd4a5ab509b40a9bf9c07338187c66c073f0493b89f423e`

PGR3's PE header originally says image base0x00400000, while XEX actual load base is0x82000000. Use the XEX value for cross-references. Both raw .text offsets differ from mapped RVAs; using the wrong layout can generate false disassembly or miss references.

## Concrete findings

| Family | Base/color | Other roles evidenced |
|---|---|---|
| PGR3 regular scene `psSceneDiffuse` (table image+0x8b920) | diffuseMap s0 | bump s1, materialMap s2, materialCropSheet s3, cube s7, materialDefMap s11, diffuseCropSheet s13, light s14 |
| PGR3 road `psSceneRoadSurface` (+0x89240) | baseDiffuseMap s8 | base specular s0, detail specular s1, markings specular s2, dirt specular s3, base bump s4, detail bump s5, detail diffuse s9, markings diffuse s10, dirt diffuse s11, light s14 |
| PGR4 Gotham4 road (+0x17b204) | baseDiffuseMap s5 | base specular s0, detail specular s1, wear specular s2, markings specular s3, dirt specular s4, detail diffuse s6, wear diffuse s8, markings diffuse s9, dirt diffuse s10, reflection s13, light s14 |
| PGR4 legacy road (+0x17d450) | baseDiffuseMap s8 | legacy-style specular/detail/markings/dirt slots; reflection s13, light s14 |
| Vehicle shaders (both games, several variants) | g_DiffuseTex s0 | g_BumpmapTex s1, g_OverlayTex s2, environment s3; variant-dependent shadow, damage and other maps |

PGR3 scene shader registration is reachable through code around0x82205150; original shader names/data addresses are passed to helper0x821139c0. Selected XREF listings are private. A candidate texture-binding/cache helper at0x82185588 records texture pointers at a stage-indexed stride8 and maintains a stage-presence mask; callers include stage8. This corroborates indexed binding, but its full D3D wrapper semantics and material serializer path are not yet proven.

Additional metadata includes `g_cropSheetOffsetAndScale`, `g_sceneUV1`, `g_sceneUV2`, `g_diffuseScaling`, road settings vectors, and PGR4 color texture matrix/wear/reflection variants. Their presence supports atlas/coordinate/layer work; it does not establish the exact math without reading the shader instructions or binding constants. `g_textureSet_*` entries are boolean registers, not texture samplers.

## Comparison with the current application

- `src/pgr34_track.h`: preserves resource/slot/channel triples; assigns only source channel0; follows a single alias and deliberately declines multi-surface layer composites. `gameShaderApplied=false` correctly signals approximation. This audit is compatible with generic scene base-color previews, but does not independently validate source-channel0 for every SCNE group. Road/color crop sheets, secondary UV channels, light maps and multi-pass materials are not reproduced.
- `src/pgr34_geometry.h`: vehicle material has four source texture references. Current selection uses reference0 when present, otherwise2, marking `decalLayerApproximation`; normal/specular references are not shown as color. Embedded shaders confirm distinct diffuse+overlay inputs, so a single decal-as-diffuse preview is not full material composition. The exact storage-reference→diffuse/overlay mapping still needs CPU binding/serializer evidence; do not replace it globally with sampler indices.
- Vehicle UV scale remains explicitly nominal, not validated by the executable audit. Finding atlas-related constants is not proof of a UV flip, swizzle bug, or permission to change1/320 globally.
- Existing missing maps/crop-sheet fragments may involve separate shader roles, not absent pixels. No new coverage percentage is claimed; no map-count test or render was changed in this research pass.

## Next implementation gate / owner

1. Codex/native owner: trace SCNE material group/declaration → CPU material object → selected shader variant → stage binding and constants. Start with regular scene, legacy road, Gotham4 road and one vehicle diffuse/overlay material. Preserve exact source resource, layer, UV set, crop parameters and byte offsets.
2. Match one actual Tokyo draw per family and one car part against these bindings. Separate serialized channel, resource-local texture slot and GPU sampler register in the resulting manifest.
3. Add data-side material-layer/UV/atlas evidence to sourceBindings first; unknown roles remain unknown. Test it independently from rendering.
4. Only then implement per-family base/overlay composition and UV/crop transforms, with focused regression images and owner/emulator comparison. Do not invent blend factors or treat a specular register as base color.
5. Shader microcode decompilation or runtime frame capture may be needed to recover the precise blend/lighting. Executables are now available locally for further analysis; obtaining them is no longer blocked on owner unpacking.

Completed: unpacking, image layout/export verification, shader metadata recovery and initial assignment audit. Pending: end-to-end per-material binding proof and faithful layered rendering. No claim that all existing assignments have been proven correct or globally corrected.

## Application integration — 2026-10-27 (validated inspection slice)

The owner requested integration after the audit. This implements the data/provenance and explicit inspection slice, **not original shader composition**:

- Track material records retain every declaration's type/usage/index, decoded-SCNE byte offset and raw bytes. Their reachable alias/layer graph is traversed once per preview with a 256-node bound, cycle/duplicate protection and missing-target warnings. All source resource/slot/channel references are available, including branches that the automatic preview deliberately cannot blend.
- Vehicles expose the four actual serialized references and available map IDs. Roles are explicitly unresolved rather than assigning GPU sampler meanings to storage indices.
- Mesh Collection > Material > Source maps… opens a compact table plus raw evidence, follows the active mesh, and offers Preview source map / Automatic. Explicit selection is an unlit single-map preview using the existing mesh UVs. No blend, UV/crop transform or texture-coverage improvement is claimed. Shader/microcode/game keys are not bundled.
- The decoder validates that a selected identity belongs to the current material graph. Texture decoding retains its normal cache/budget/layout rules (BC5 allowed for explicit track inspection); automatic channel0 selection, aliases and unresolved multilayer neutrality remain unchanged.
- Material preview identity excludes the source-map selection, so a change refreshes the existing mesh instead of adding another. An ephemeral target mesh ID distinguishes transformed/duplicate copies; it is removed from the stored source candidate. MainWindow checks document path, stale state, active scene and batch state before decode. Position/rotation/scale are retained. Automatic re-decodes the original conservative approximation. Existing sourceBindings and exports carry evidence/selection and still report gameShaderApplied=false.

Affected: pgr34_track.h, pgr34_geometry.h, mesh_preview_registry.h, mesh_collection.cpp, vulkan_panel.h, main.cpp, tests/window_tests.cpp. Tests include graph cycles/missing targets, role uncertainty, inspector signals, exact target refresh, actual PGR3/PGR4 track source-map pixels/geometry, car maps and the MainWindow source-preview/restore path. Final receipts follow after validation. No claim that layered materials are now game-exact.

### Final validation receipt

- Dev app compiled and updated: build/pgr-source-final-build.log. No publication.
- Final material corpus **8 passed/0 failed/0 skipped**, including both actual track/map pixel comparisons and car source references plus MainWindow preview/restore; ten distributed Tokyo Night draws loaded per game (11,738 PGR3 and13,559 PGR4 candidates). Reference car automatic counts unchanged: Ferrari87 textured draws/31 images; Alfa72/32. UI full car counts195/158, faces118,377/78,771.
- Full window **120 passed/0 failed/86 optional skipped**; reader45/0/no skips. Logs build/windows-local/pgr-source-{corpus-final,full-window,reader}-tests.txt.
- Full native Tokyo Sunny method passed:11,785 loaded/10,097 textured/0 failed; 1,536 layered composites remain neutral and152 source base maps missing/unsupported. JSON receipt build/pgr-source-validation/pgr3-sunny-full.json. Intermediate corpus log pgr-source-corpus-tests.txt has a failed UI harness assumption (first auto-preview was an untextured tyre); final corpus selects a map-bearing part and passes. Do not treat that intermediate harness failure as a decoder regression or hide it behind a final total.
- Live PGR3 Ferrari full car:195 visible meshes/118,377 triangles, no failed/omitted. Open Material > Source maps…; material82/reference3 selected map9(128×64) instead of automatic33(256×128), Unlit preview; Automatic restored33/Lit. Counts stayed195/118,377. Private screenshot build/pgr-source-validation/pgr-source-inspector-live.png. This demonstrates selection/refresh, not faithful original game rendering.

The next owner must still prove per-family source-field→shader binding, recover UV/crop transforms and implement exact layered composition. This integration deliberately preserves conservative automatic assignments and the existing coverage, while exposing the source data needed for that work.
