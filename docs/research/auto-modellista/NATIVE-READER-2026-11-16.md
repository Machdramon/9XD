# Auto Modellista native track reader / Claude handoff — 2026-11-16

Implemented in9XD; development build linked and left closed. No commit, push or release. This supersedes earlier research-only/unassembled-track status. Scope is PS2 static track AMO/AHI and primary materials; car assembly and original render effects are still separate pending work.

## Native application integration

- `src/auto_modellista.h`: bounded/cancelable word-oriented Meltw, outer slot table, AMO vertex/normal/UV/material/index validation, independent triangle strips, adjacent AHI model association and world transforms. Candidate split per placement/material ensures multiple materials never collapse into one texture. Source SHA256 rejects stale candidates. Input32MiB/decompressed128MiB caps are additionally bounded by configured memory budget.
- `src/asset_discovery.h`: calls native scanner, returning renderable meshes directly.
- `src/mesh_profile_decode.h`: dispatches native decode; works with file snapshots and in-memory geometry through existing ReadRequest. Automatic texture lookup requires a usable extracted source path and sibling bank.
- `src/mesh_preview_registry.h`: includes Auto Modellista instance/material/source/context identity. A dedicated test prevents two material groups of the same model from collapsing.
- Auto texture context: `stgNN_mK.bin` uses sibling `stgNNN.bin`; a `stgNNN.bin` or `_q` bank opened directly uses itself. No arbitrary bank search/substitution. Quality/weather selection for an m-package remains regular/default; no unsupported alternate-bank guess.
- Source vertex/index provenance is decompressed, NOT original file offsets: editable raw provenance arrays contain-1, while material bindings retain decompressed offsets, AHI node/offset, row-major world matrix, material/slot/bank identity and shader limitations.

## AHI layout and transform proof

AHI root type0xc0000000, ordinary12-byte records. Root entries with low typebyte0 list tree root ordinals. Nonzero node records1/2 have payload: id+0, parent+4, first child+8, next sibling+12, scale float4+16, Euler-radian float4+32, translation float4+48, AMO model index+64 (-1 means grouping node), state/group at+68. IDs, indices, finite transforms, complete reachability, parent agreement, cycles and depth checked. No guessed spatial reassembly.

Original `plCreateInitMotionSetFromAHI` VA0x13e494..0x13e55c copies associations/SRT; `flGetHierarchyData` VA0x12bb58..0x12bb9c builds diagonal scale, calls `flmatRotXYZ33`, then assigns translation. `flGetHierarchy2_sub` links first-child and sibling; `plInitPostureHierarchy` composes child with parent while siblings retain the parent frame. Native column-vector convention is T * Rz * Ry * Rx * S and world=parent*local; normals use inverse transpose, reflected transforms reverse triangle winding.

Independent arithmetic lift of original `flmatRotXYZ33` COP2 matrix-composition blocks (VA0x1292dc..0x129320,0x12947c..0x1294c0,0x12961c..0x12965c) agrees with the matrix order for105 axis/mixed/random cases. This substitutes host sin/cos for PS2's trig approximation; it is an algebra/order check, NOT original bit-exact trig emulation. Instruction encoding meanings checked against [PCSX2's primary disassembly tables](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/DebugTools/DisR5900asm.cpp); no emulator implementation copied into native code. See rotation_order_probe.py (USA VA layout) and ahi-validation.json.

## Two missing layers fixed

1. Raw road packages contain AMO/AHI pairs: apply AHI, rather than displaying all models at local origins.
2. `stgNNN.bin` contains geometry AFTER the TIM2 bank, not only images. `create_stage_sub` explicitly creates this additional model. Native scanner now exposes these models too; its own bank supplies their textures. USA stg001 adds49 placements/2650 faces. Earlier coverage metrics for only `_m` packages did not include this geometry.

European and some USA streams can begin with flags0x0004. Only the first two control bits need to describe literals for the structural prefix check; requiring the entire16-bit flag to be zero caused false negatives. Regression fixture retains the real prefix. Full parsing still validates output/AMO/AHI; the prefix alone is never recognition.

## Verification actually performed

|Corpus|Files|AHI placements|Faces|Primary-textured faces|Reader errors|
|---|---:|---:|---:|---:|---:|
|USA regular banks +road packages|66|3507|734645|734645|0|
|Europe regular banks +road packages|57|3053|642339|642339|0|

These are source-record/placement/image availability checks, NOT visual validation of every circuit in-game or original-shader fidelity. Full per-file results: NATIVE-TRACK-COVERAGE.csv, ahi-native-corpus.json, ahi-native-europe.json. Standalone native probe exercised the same header decoder as the application; no Python/export-only substitution for the native scan.

Independent Python parsing reproduces all734645 USA source/placement face counts.198 native world-space vertex samples agree with the independent hierarchy transform within0.000060675 units (tolerance0.02 for native float arithmetic). Source research hierarchy pass covers48 road packages/3140 placements; final native coverage also includes18 regular banks/367 placements. See ahi-validation.json.

`autoModellistaContracts`: literal and overlap/zero/extended Meltw streams, truncation/output budget/cancellation, two materials on same model, nested-parent translation, cyclic/out-of-range AHI, stale source/candidate and preview registry identity. `autoModellistaTrackCorpus`: native scan and actual MeshProfiles decode for five road packages plus stg001 bank; all expected source faces and primary images retained. Focused7 passed/0 failed/3 optional other-game skips (build/windows-local/automod-native-focused.txt). Final full application regression150 passed/0 failed/87 optional fixture skips (build/windows-local/automod-native-full.txt,127.8s). Builds in build/automod-native-build.log and build/raycity-app-build.log. Development build/windows-local/binary_workbench.exe linked successfully; no UI launched.

An interrupted texture-bank decode is not cached as a permanent missing bank; retry can load it normally. Mesh offsets are never falsely presented as original compressed-file addresses. Existing RayCity/F&F changes preserved.

## How to test a track in9XD

For Suzuka USA, keep these extracted files in the same folder:

- Open/scan `stg01_m0.bin`, `stg01_m1.bin`, `stg01_m2.bin` for road/terrain packages.
- Also open/scan `stg001.bin` for its additional scene geometry. Its49 TIM2s are used automatically by the three road packages.
- Render the results from all four sources in the same scene; no manual Track Texture Context needed for the regular bank. This is still individual source scanning through existing multi-file workflow, not a new single-file stage aggregator.
- For stage0 use stg00_m0/m1 +stg000; stage2 use stg02_m0/m1 +stg002. Files may differ between regions; do not mix source/bank copies across versions.

AHI-applied reference OBJ/MTL exports and combined road-only Suzuka rendering: build/auto-modellista-research/ahi/exports and ahi-stg01-combined.png. This render combines the three road packages; it intentionally does not claim the bank's background geometry is displayed. Full native reader coverage includes that geometry. Do not use the old raw/unassembled previews as validation of the updated reader.

## Remaining work, explicitly pending

- Original vertex-color modulation, lighting/cel shading, reflection-coordinate generation, alpha/depth/pass ordering and default visibility of helper passes. Primary image100% is NOT100% appearance.
- Car layouts lack the adjacent AHI road pattern and use `mkModelCAR`/`car_create_clay` stateful part/pass selection. Native track reader does not emit the misleading all-variants car preview. Car primary material research remains available in MATERIALS-2026-11-16.md.
- Quality/weather-specific bank selection for road packages; direct q-bank scan uses its own images but q-corpus completeness was not audited here.
- Automatic mounting/linking of sibling resources inside an unopened AFS archive. Existing unpacked/virtual geometry reads work, but automatic sibling textures require extracted source paths.
- Optional whole-stage aggregation convenience, descriptive object names if source labels can be recovered, and in-game visual comparison (owner).

Next owner Codex/Claude: preserve validated AHI/material semantics, add original render-state interpretation with evidence. Do not compensate with guessed UV flips, triangle deletion or distance-based placement.
