# PGR3/4 Tokyo: handedness, textures, executables and render pipeline (2026-10-27)

Owner: Claude. No commit/push/release.

## 1. Handedness fix (implemented, needs owner visual confirmation)

- **Symptom (owner):** maps from Bizarre games are mirrored on a horizontal axis; height is correct.
- **Cause:** Bizarre titles (Dreamcast WinCE/D3D, Xbox, Xbox 360) store Direct3D left-handed coordinates. The
  viewer and the exporters are right-handed, and the adapters passed positions through unchanged.
- **Fix:** `mesh_profile_decode.h::mirrorLeftHanded()` is applied once after decoding these profiles:
  `native-pgr34-track`, `native-pgr34-car`, `native-pgr1-car`, `native-pgr1-track`, `native-pgr2-track(-instances)`,
  `native-pgr2-car`, and `native-msr-alpha-car/track`, `native-msr-retail-track`.
  - It negates Z for positions and normals, swaps triangle vertices 1/2 (with face sources) so front faces stay
    front, and negates `positionDelta.z`.
  - It copies the mesh, so cached decoder data is never mirrored twice.
  - It sets `handednessConverted` in the source bindings. Exports inherit the conversion.
- **Axis choice:** Z, the conventional D3D → right-handed conversion. Negating X would fix the mirror equally but
  rotate the map 180°. The owner should confirm against a real Shinjuku map; if north ends up reversed, add a
  rotation rather than changing the mirror.
- **Validation done:**
  - The app builds and loads 288_GTO_Evo (195 draws) and the PGR4 garage (101 draws) after the change.
  - The existing tests that assert PGR/MSR coordinates only check X/UV, so they are unaffected.
  - **Not yet verified:** that text on signs and car liveries now reads correctly. On the 288 GTO the side livery
    renders as a jumble of atlas fragments, a separate UV/material issue noted below; no legible text was found
    there.
- Other Direct3D-era formats (FM1, ITC, NFS) were not changed: the mirror was only reported for Bizarre titles.
  Check them the same way, with text on textures, before converting them.

## 2. Load speed for very large scenes (Tokyo 11,785 draws)

- **Measured:** about 36 draws/s average over the first 2 minutes, dropping to about 10 draws/s past 4,500.
  The total for Tokyo was several minutes.
- **Confirmed per-item linear work (makes the batch quadratic):**
  - `ImageWorkbench::addDecodedTexture` (`image_workbench.h`) scans **all** library entries, comparing name/path
    (and pixels when the name matches) for every texture of every loaded mesh. It runs once per mesh result in
    `main.cpp`.
  - Fix: a hash index keyed by (name, source, image cacheKey).
- **Not quadratic (checked):**
  - `MeshScene::beginBatch` blocks scene signals, so the collection tree and GPU rebuild run once at the end.
  - `MeshPreviewRegistry` uses keyed maps.
- **Still to profile** (needs a timed run on the owner's machine; ChatGPT/Codex have the shell): the scan-panel
  batch loop (QTreeWidget item updates), the per-candidate PGR session cache hit rate, and image thumbnail
  creation.
- **Pipeline improvements for city-scale scenes, in order of value:**
  1. **Merge draws per material** into one scene entry (Tokyo would go from 11,785 entries to roughly the number
     of distinct base maps). This removes most per-entry overhead in the scene, collection UI, registry and GPU
     spans.
  2. **Index the texture library** (above), and create its thumbnails lazily.
  3. **Incremental GPU upload:** `VulkanPanel` rebuilds and uploads the whole vertex array on every geometry change
     (visibility toggles included). Allocate per-entry buffer ranges and update only what changed.
  4. **Optional:** frustum/distance culling and a streamed or partial load by city block (`block_XX` names already
     exist in the candidates). That allows loading Shinjuku only.

## 3. Textures (current state from the project docs and live runs)

- **Tokyo Sunny (PGR3):** 10,097 of 11,785 draws (85.7%) get a base map.
  - 1,536 draws use **layered materials** (source shader blending). They are deliberately left neutral, because no
    single layer is a correct diffuse.
  - 152 draws have unresolved or missing base maps.
  - Only channel 0 (colour) is used. Normal, specular, environment, microtexture palette
    (`Materials/MicroTexturePalette.DDS`), lighting, fog and cube maps are not applied.
- **Vehicles:** the 288 GTO livery shows atlas fragments on the body side. Paint/livery layering
  (base paint + decal atlas + masks) is not reconstructed. This is the vehicle-equivalent of the layered-material
  gap.
- **What is needed to close the gaps:** the material/shader semantics, which blend inputs each layered material
  uses and how. Two sources:
  1. **Data-side (first):** the material records in the scene PAK and STREAM2 (layer count, texture slots,
     per-layer parameters), compared statistically across the 1,536 draws. No executable needed.
  2. **Shader-side (when the data does not explain the blend):** the compiled Xenos shaders and the code that binds
     them. In PGR3 they are not in `Materials/`, which holds only DDS maps; they are likely inside the scene/game
     PAKs or the executable.

## 4. Do we need to unpack the executable?

- `pgr3/default.xex` (6.0 MB) is **XEX2, encrypted (AES) and compressed (basic)**. PGR4 `default.xex` (12.4 MB)
  is the same family.
- No shader or material strings are readable in it as shipped.
- **For geometry and base textures: no.** Everything so far has come from the data files.
- **For layered materials, livery blending and shader-exact appearance: probably yes**, unless step 3.1 already
  answers it.
- Decrypting and decompressing an XEX needs the console retail key, as handled by community tools such as
  xorloser's XexTool. This is the owner's step on their own copy; the project should not embed keys or decrypted
  binaries. Analysing the resulting PowerPC image also needs a PPC disassembler (e.g. Ghidra), which is not
  available in Claude's cloud workspace; static analysis would have to run on the owner's machine or via exported
  function listings.
- **Recommendation:** do 3.1 first. Only if the layer semantics stay ambiguous, decrypt the XEX locally and look
  for the material binding code and the embedded shader microcode.

## Codex measured continuation — 2026-10-27

Claude's handedness conversion is retained (owner confirmed visually in the worksheet). Texture-library name/source index and progress UI are retained.

Baseline `WindowTests::localPgr3SunnyRenderAll`, original Tokyo PAK + stream2: **190,805 ms**, **11,785 meshes / 10,097 textured instances / 5,498 shared image identities**, no failures. Five selections: 5 ms. This exercises the real MainWindow scan/decode/library/scene path on Qt offscreen, including CPU scene preparation; it does **not** measure live Vulkan GPU upload or frame time. Trace total after first completion is 186,075 ms; that epoch omits initial scan/first-decode setup.

The last 500 rows averaged 4.29 ms worker decode (3.89 ms nested texture phase), .665 ms scene insertion, .050 ms library insertion, .007 ms registry and 9.55 ms worker-to-GUI interval (includes notification/event-loop overhead, not a pure OS queue metric). Texture phase is a subset of decode, not additive. The GUI texture-library cost does not justify speculative thumbnail changes.

Implemented, validation pending at this point: two tagged in-flight native PGR3/4 track requests; drain on stop/error, stale request rejection; constant-time retained texture accounting; one material/span lookup per face span during Vulkan upload. The PGR session mutex still serializes its native decode, so this is bounded prefetch overlapping result handling, not two unrestricted decoders. Current full GPU rebuild/upload remains; incremental entry buffers, block loading and optional per-material static merge with faceSources are future work. Private baseline evidence stays in ignored build/; no game assets or captures are published.

### Validated before/after CPU pipeline result

Same actual PAK/stream2 and same offscreen MainWindow test; one measured run each (sequential, warm filesystem/cache effects not isolated): **190.805 s → 104.052 s (45.5% reduction, 1.83× throughput)**. Both runs have **11,785 meshes, 2,333,476 vertices, 1,297,789 triangles, 10,097 textured instances and 5,498 shared image identities**. Zero failed; no geometry/material collapsing or skipped objects. Five mesh selections: 5 ms → 4 ms.

Cumulative time after the first completed draw (excludes initial scan/setup):

| Completed draws | Before (s) | After (s) | Before / after ms per draw in this interval |
| --- | ---: | ---: | ---: |
| 1,000 | 21.439 | 14.065 | 21.44 / 14.07 |
| 3,000 | 52.602 | 30.553 | 15.58 / 8.24 |
| 6,000 | 98.939 | 54.594 | 15.45 / 8.01 |
| 9,000 | 144.967 | 78.113 | 15.34 / 7.84 |
| 11,785 | 186.075 | 99.268 | 14.76 / 7.60 |

After, last 500: scene insertion **.011 ms** (before .665), library .056 ms (before .050), worker decode 6.557 ms (before 4.29, now includes waiting for the session mutex), nested texture phase 3.956 ms (before 3.89), worker-to-GUI interval 9.398 ms (before 9.55). Prefetch overlaps these intervals; do not sum per-job durations to get wall time. `dispatch_gap_ms` becomes negative when requests overlap and is a relative overlap marker, not negative idle time. Profiling is opt-in and appends to the given JSONL; choose a fresh file for each run.

Focused camera/scene/tagged-batch regression: **7 passed / 0 failed / 0 skipped**. Stop test completes out-of-order requests, rejects a duplicate completion and drains both in-flight requests before ending the batch. Full shared-component tests, installed executable and live GPU receipt follow below when complete. Future work remains incremental uploads/static merging; do not mark those worksheet tasks complete from this result.

### Final regression and live receipt

Final rebuilt shared suites: **window 118 passed / 0 failed / 85 optional skipped; reader 45/0/no skips**. Genki/ARCP empty-profile compatibility was restored after full regression testing exposed a mutating `profile["name"]` read in tracing; the final read uses `value()`. Tests include the actual scan button path, not just isolated decoders. Force rebuild all MeshScene consumers (UV panel included) when changing class layout; localized MSVC include dependencies previously left stale objects. Scripting tests need the VS developer environment.

Actual Vulkan RTX 4060 Ti run completed **11,784 new + 1 already loaded, 0 failed, 0 omitted**, total **11,785 visible meshes / 2,333,476 vertices / 1,297,789 triangles**. Textured city shown. Existing 1,704 texture-warning messages are retained (do not equate warnings to missing base-map meshes). During loading, FOV slider changed 45→83.9°, then back to44.9°; Near .01→.000186, Far100000→.281838; shortening Far removed the scene and restoring Automatic brought it back. Private screenshot: build/tokyo-live-pipeline.png. This was an interactive run with simultaneous tests/compilation, not a controlled GPU wall-time benchmark. Live load remains slower than offscreen; investigate GUI repaint/frame scheduling next, followed by incremental GPU buffers.

## PGR3/4 executable audit update — 2026-10-27

The earlier owner-unpacking blocker is superseded: Codex locally decrypted/basic-expanded both default.xex files and exported validated static-analysis images. 330/994 embedded shader constant tables recovered. Regular diffuse s0, PGR3 road base s8, PGR4 Gotham4 road base s5 and legacy road s8; distinct atlas/overlay/specular/normal/light/wear roles. **Do not equate these GPU register IDs with source SCNE channel IDs.** Current generic base previews are not proof of faithful multilayer rendering; car reference0-as-decal preview still approximates diffuse/overlay composition. No assignment code changed. Next: prove per-family serializer/CPU/shader binding with selected draws, then add atlas/layer rendering. Details, hashes, artifacts, confidence and pending work: docs/research/pgr34-executables/README.md; shared coordination updated. No binaries, keys or original shader code published.
