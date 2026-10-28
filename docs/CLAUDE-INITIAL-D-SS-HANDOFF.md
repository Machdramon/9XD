# Claude handoff: Initial D Special Stage / Akagi

Updated: 2026-10-27
Status: targeted corrections implemented, tested and built in windows-local. Live comparison and original game shading remain pending. Read AGENTS.md and docs/COORDINATION-PLAN.md first; preserve all contributors' uncommitted changes. No commit, push or release performed.


## Start here: corrective workflow for future work — 2026-10-27

The latest implemented baseline is the UV/strip/LOD correction **plus** the GIM transfer/swizzle and viewport clipping follow-up below. The code is installed in the development windows-local build. Earlier “validation in progress” notes are superseded by the final results. No code change, rebuild, commit or publication was performed in this documentation-only handoff update.

### Diagnose the failing layer before changing the output

| Symptom | Verify first | Correction / entry point |
|---|---|---|
| Texture becomes one flat color | Count actual UV UNPACK shapes, including the VIF mask bit; inspect decoded UVs | Accept masked V2-32 (0x74) in idss_core.h. Do not infer missing images from a flat preview. |
| Image has stripes, shuffled blocks or repeated fragments | Decode the image independently of the mesh; compare storage format and upload format/dimensions | Read GIM pixel PSM at +0x34, transfer width/height at +0x36/+0x38 and transfer PSM at +0x3A. Replay upload through GS memory before sampling native pixels. |
| Faces have inconsistent lighting/orientation | Check GIF topology, batch boundaries, ADC and generated normals separately | Preserve source strip parity and no-kick history. W bit 2 is not a demonstrated winding rule; do not force normals upward. |
| Surfaces flicker or appear doubled | Check selected named LOD alternatives and previously loaded preview snapshots | Keep main/LOD candidates separate; Render All uses main set. Do not merge alternatives into one default texture group. |
| Close approach cuts away the model or zoom stops too early | Inspect normalized camera distance versus model radius, projection near/far and orthographic depth range | viewport_camera.h + vulkan_panel.cpp: adaptive clipping; user controls under Display > Settings; extended zoom and fine Shift+wheel. Do not rescale source geometry to repair the camera. |
| Whole track remains too dark | Verify material reaches the actual renderer; distinguish source colors from extra diffuse lighting | Unlit base-texture preview is deliberate. Original colors/fog/blend remain a separate task, not a global brightness patch. |

### Exact GIM repair contract

1. Read the declared pixel and transfer formats separately. T4 is **not always linear** and not always swizzled. Akagi contains both forms.
2. For PSMCT32 uploads, validate `transferWidth * transferHeight * 4 == pixelBytes`, pixel/palette bounds and the GS storage budget.
3. Write each four-byte source transfer pixel to its PSMCT32 GS address using the declared transfer pitch. Mark every written byte.
4. Read each final texel at its PSMT4/PSMT8 GS address using the texture pitch. For T4 select the correct nibble. Reject reads from unwritten storage; do not silently pad an invalid layout into a plausible image.
5. Use **ps2_gs_address.h** for page/block/column addressing. image_recipe.h only exposes the compatibility wrapper. Keep IdssCore independent of Qt; do not introduce a second competing swizzle formula.
6. For direct native uploads, require matching pixel/transfer format and dimensions. Reject unknown layouts until independently understood.
7. Keep palette interpretation and UV orientation separate from storage repair. This fix changed neither palette order nor UV flips/scales.

Known result for AKAGI_DAY.PAC: **33 T4 images corrected, 106 images unchanged, 139 decoded**. Inspect KAGI_DAY003_007 (trees), KAGI_DAY078_002 (asphalt), navi (course outline) and unchanged KAGI_DAY000_000 (sky). The before/after contact sheet is private at build/idss_test/idss-swizzle-comparison.png. Pixel hashes in localInitialDssGimSwizzle protect these inspected results; hashes establish regression stability, not correctness of every shader or untested game.

### Camera contract to preserve

- Camera calculations operate in normalized render space; manual Near/Far fields use **original model units** and must be converted by the current debug/framing radius. Reframing a selection must not silently change the meaning of manual settings.
- Automatic perspective near plane follows approach distance; far plane covers scene extent and camera/target position. Orthographic zoom must not slice the model merely because the virtual camera crosses its depth.
- Do not restore fixed `.01 / 100` projection planes or the `.005` normalized minimum zoom. Those values prevented close inspection of large tracks.
- Wheel zoom; Shift+wheel fine zoom; movement speed scales down with approach distance. Frame Selected can rebase a mesh, face or vertex for detail inspection.
- Manual settings require finite, positive Near < Far; invalid edits retain the previous valid range. Preserve inheritance for newly created views and propagation to tiled views. Settings are currently per viewport session, not persisted.
- Verify perspective AND orthographic views, large map AND small selected mesh, automatic AND manual range. Check selection/gizmo behavior as well as visible geometry.

### Required validation before handing back a change

1. Read the actual saved source and current coordination plan. Earlier handoff descriptions once differed from the code on disk; do not assume prose proves implementation.
2. Rebuild the affected translation units with the windows-local preset. For header-only changes, ensure main/tests are recompiled; keep builds in the same directory sequential. Do not replace a running executable or use a stale binary for the live check.
3. Run focused Qt tests with Qt runtime on PATH, QT_QPA_PLATFORM=offscreen and WORKBENCH_STUDY_CASES pointing to the owner's study cases. Required functions: `initialDssGimTransfers localInitialDssGimSwizzle viewportCloseInspection initialDssPac localInitialDssAkagi renderWareTxdMaterials localRenderWareTxdImages`. The local functions must actually run, not skip.
4. Run reader_tests and the full window_tests suite after shared GS/camera changes. Latest baseline: focused **9/0/no skips**, reader **45/0**, full window **115/0, 85 optional skips**. Consult the idss-swizzle-camera logs for details; optional skips are not coverage.
5. Build binary_workbench, restart it, reopen the PAC and run a fresh scan/render to replace old texture snapshots. Verify original main-set counts (158,296 triangles), then inspect the four image examples and close-view behavior in the real Vulkan viewport. Do not label an offscreen decode test as visual validation.
6. Record exact source/model/material for remaining issues, actual tests/results and limits. Update this handoff, the specialist README and COORDINATION-PLAN.md in the same work session. Keep game files/captures local and excluded from releases.

### Remaining work (do not mark completed by these fixes)

- [ ] Identify the single texture warning in the live Akagi render by exact material and cause.
- [ ] Preserve original per-vertex attributes; distinguish V4-8 color from the 40 V3-32 attribute batches before assigning semantics.
- [ ] Implement verified color modulation, fog and alpha/blend behavior, then compare with the game.
- [ ] Confirm active VU variants, absolute front-face/basis convention and original LOD distance/visibility rules through runtime evidence.
- [ ] Confirm the new manual clipping controls and close approach visually with the user; automated camera tests pass, but that final manual interaction was not completed during active user input.
- [ ] Cache PAC parsing per document revision/patch identity rather than once per material.

## Problem and root causes

Akagi appeared dark, with nearly flat texture colors and questionable faces. Independent inspection of the owner's PAC and static inspection of SLPM_652.68 found several distinct problems:

1. **UVs were present but ignored.** All 10,432 UV UNPACK packets use 0x74, the masked V2-32 form. The saved reader accepted only 0x64, leaving vertex UVs at zero. Sampling one image coordinate makes a textured surface look like a flat color. This was not missing texture data.
2. **Unproven face-winding rule.** The reader used position-W bit 2 to choose index order. Embedded GIF tags specify triangle strips (PRIM 0x1c / 0x5c); W bit 15 suppresses a drawing kick. Standard strip parity is the supported reconstruction. Correct face counts alone did not validate the old orientation rule.
3. **LOD resources were merged with main geometry.** Named crslod/skylod/mntlod resources shared texture groups with the main resources, so Render All could overlay alternative surfaces. This risks z-fighting and misleading visual inspection.
4. **Extra viewport lighting.** The saved adapter did not actually set material.unlit, despite earlier handoff wording. Generated normals and generic diffuse lighting could darken the result. Original per-vertex lighting/colors, fog and blend behavior are still not represented; unlit texture display is a diagnostic preview, not complete game shading.

## Changes already implemented

| File | Change |
|---|---|
| src/idss_core.h | Accept UV UNPACK after removing mask bit 0x10; reconstruct per-batch triangle-strip parity; skip ADC kicks without resetting parity/history. |
| src/idss_model.h | Keep main and named LOD records in separate texture-group maps; expose LOD candidates with idssLod/optionalVariant; exclude alternatives from default Render All through existing filtering; decode selected set independently. |
| src/idss_model.h | Set material.unlit=true; retain double-sided preview; explicitly report sourceVertexColorsApplied=false and pending original effects in sourceBindings. No global UV flip, normalization or brightness correction. |
| src/mesh_preview_registry.h | Include texture and idssLod in preview identity so different equal-size texture/LOD groups are not silently treated as duplicates. |
| tests/window_tests.cpp | Masked-UV and W=7 strip regression; synthetic main/LOD isolation; actual Akagi record/model counts, finite coordinates, valid indices, exact UV count and main/LOD totals. |

Do not add BFS/up-facing normal coercion or regenerate arbitrary faces to hide winding problems. No such heuristic is part of this correction. Named LOD filtering is a preview policy; actual runtime distance/visibility rules are not reconstructed.

## Measured results

- Akagi: 137 SMD models, 3,111 records, **216,950 triangles**; each record/model matches its declared triangle count.
- **415,596 vertices and UV pairs**, including exactly **399,754 nonzero UV pairs**. Previously the masked UV pairs were ignored.
- Main named resource set: **158,296 triangles**, **122 texture-group surfaces**.
- Named LOD alternatives: **58,654 triangles**, across 39 models. Available separately; excluded from default Render All.
- 10,351 GIF tags have PRIM 0x1c, 81 have 0x5c; both specify triangle strips. Fog is enabled in the latter set.
- Private crs01 record-0 OBJ/UV and linked GIM export inspected; image is legible. Game-derived exports remain outside the repo.

## Validation and installed build

- Focused checks: **6 passed, 0 failed, no skips**.
- Reader suite: **45 passed, 0 failed**.
- Full window suite: **113 passed, 0 failed, 84 optional skipped**.
- Actual Genki context/WM/isolation/March checks: **7 passed, 0 failed, no skips**.
- windows-local binary_workbench.exe rebuilt successfully after source changes. Build log: build/review-fixes-app-build.log. Executable timestamp verified 2026-10-26 23:47 local.
- Logs: build/windows-local/idss-analysis-{focused,reader,window}-tests.txt, genki-track-context-tests.txt. git diff --check passed.

These tests verify decoding contracts, not visual equivalence to the running game. Do not mark live appearance validation complete.

## How to verify in 9XD

1. Launch the current build/windows-local/binary_workbench.exe.
2. Open study cases/initial d special stage/COURSE/COURSE.AFS, then its AKAGI_DAY.PAC child.
3. **Run a fresh Scan for 3D Data.** Old candidates merge LODs and have stale counts. Avoid keeping the previously loaded merged scene alongside the new results.
4. Render All for the default main set. Confirm a recognizable road/wall/terrain texture, not a single sampled color. Check that materials show unlit.
5. Inspect named LOD alternatives separately; do not add them over the complete main set and interpret the overlap as a decoder error.
6. Compare road, terrain, walls and two-sided objects against the game. Record exact candidate/material and a screenshot for remaining errors. Test U/V orientation with a recognizable asymmetric image before changing it.

## What remains necessary, in order

- [ ] **User + Claude: live validation** of the corrected main set. Capture concrete examples for any residual flipped faces/UVs or dark surfaces. A successful offline count is not sufficient.
- [ ] **Native reader/render pipeline: source vertex colors.** Preserve per-vertex source attributes into MeshData, GPU rendering and exports where supported; implement PS2 color modulation semantics after verification. Do not silently multiply by byte/255 when the source neutral convention differs. There are 10,392 V4-8 attribute packets and 40 V3-32 attribute batches; classify the latter before assigning them color semantics.
- [ ] **Native material work: fog, alpha/blend and other GS state.** Base texture/unlit is intentionally approximate. Diagnose each effect separately; do not compensate with global brightness, gamma, V flipping or UV rescaling.
- [ ] **Static/runtime reverse engineering:** establish which VU microprogram variant executes for each draw; confirm coordinate basis/front-face convention and original LOD visibility rules. Only the inspected static paths and source GIF topology are currently supported evidence.
- [ ] **Performance:** cache parsed PAC by document revision/patch identity. Current material decoding reparses the archive; do not introduce arbitrary small limits as a substitute.
- [ ] Update COORDINATION-PLAN.md and the specialist notes in the same session as every follow-up change. Rebuild and run focused/regression tests after code changes.

## Evidence and prior handoff

Detailed hashes, offsets and findings: docs/research/initial-d-ss/README.md. Older hypotheses: docs/research/initial-d-ss/HANDOFF.md; its supersession section applies. Do not reinstate the old W-bit-2/BFS or missing-UV assumptions.

Owner executable was inspected read-only and never executed. No game code or game assets are copied into source/release packages.


## Live 9XD render check — 2026-10-26

Codex launched the corrected windows-local executable through Windows Computer Use, opened the private AKAGI_DAY.PAC copy under build/idss_test, ran a fresh Scan for 3D Data and Render All. The automatic first sky preview was reused: UI completion reports **121 loaded, 1 already loaded, 0 failed, 0 omitted; texture warnings: 1**. Collection: **122/122 visible**, Vulkan **299,963 vertices / 158,296 triangles**, matching the main-only test total. LOD alternatives stayed excluded.

Observed in the actual Vulkan window: sky texture shows blue sky/cloud detail, rather than a flat sampled color. After temporarily hiding KAGI_DAY000_000 (sky only), the terrain/forest and continuous switchback road are visible from Top view, with texture variation. Road surface picking selected a material showing **Surface: Unlit** and linked **KAGI_DAY078_002 (128x128)**. With sky hidden, the renderer reports 121/122 visible, 299,811 vertices / 158,214 triangles (82 sky triangles hidden, not deleted).

Left app open on Akagi in a zoomed Top view, sky hidden, road material selected, search filter cleared; scan/settings panels were closed temporarily for viewport space. No source/game file edited. Evidence screenshot is private/ignored: build/idss_test/akagi-live-review.png (not published or committed).

This verifies that the fixes reach the live application and that real textured geometry renders. It does **not** establish in-game visual equivalence, every face's front-side convention or original color/fog/blend fidelity. The remaining single texture warning has not been traced to a specific material and remains follow-up work; do not report 100% material fidelity. User should compare the road/terrain at close range and record any specific residual issue. Original vertex-color modulation, fog/blend and runtime VU/LOD analysis remain pending.


## GIM swizzle and close-inspection follow-up — 2026-10-27

Owner report: close zoom/clipping prevented texture inspection; some images had swizzle stripes. Both are separate from the previous UV/LOD correction.

### Implemented

- GIM header +0x3A is the **transfer PSM**, +0x36/+0x38 are transfer width/height. A texture may be indexed PSMT4 at +0x34 but uploaded as PSMCT32 at half width and quarter height. The previous reader always treated T4 as linear, corrupting 33 Akagi images. T8 commonly uses half-width/half-height PSMCT32 transfers.
- Reconstruct the declared PSMCT32 transfer in GS local memory and read back in the declared PSMT4/PSMT8 format. Validate dimensions/byte counts, GS bounds and written-byte coverage. Direct transfers require matching pixel/transfer format and dimensions; unsupported layouts fail explicitly. Palette and UV logic are unchanged.
- Shared address math moved without semantic changes from image_recipe.h into **src/ps2_gs_address.h**; imageGsBitAddress remains a compatibility wrapper. IdssCore uses the same tested GS page/block/column address helper; the Qt-free model reader remains usable without Qt.
- Private before/after comparison: **33 changed T4 images; 106 images unchanged**. KAGI_DAY003_007 now shows coherent trees, KAGI_DAY078_002 coherent asphalt cracks, navi a continuous course outline. All 139 local GIM records decode in the independent dump probe. T8 and directly uploaded images remain unchanged. Private comparison image: build/idss_test/idss-swizzle-comparison.png (game-derived; ignored, do not publish).
- **Viewport camera:** remove the .005 normalized minimum distance (roughly 10 model units on a 2,000-unit-radius track); allow close zoom and Shift+wheel fine zoom. Navigation speed now scales down at close distances. New **src/viewport_camera.h** computes automatic near/far clipping from camera distance and normalized scene bounds. Orthographic zoom uses a depth range spanning the scene, rather than slicing the geometry when the virtual eye crosses it.
- **Display > Settings… / 3D View Settings:** Automatic clipping toggle plus Near clip / Far clip in original model units. Manual near must be positive and smaller than far; invalid input retains the previous range. New tile/independent views inherit clipping settings; tiled views follow owner changes. Values currently belong to the viewport session; no persistence added.
- Existing native geometry, coordinates, face order, UVs and main/LOD filtering are unchanged. No global texture flipping, brightness adjustment or guessed unswizzle is applied.

### Verification status (final results below)

Targeted tests initialDssGimTransfers, localInitialDssGimSwizzle and viewportCloseInspection are complete and passing, together with the full validation recorded below. The updated development executable is installed. Only the specific user close-view/manual-settings visual confirmation remains pending; the build/test work is complete.

Retain earlier pending original vertex colors/fog/blend and texture-warning investigation; swizzle repair is not a claim of full game-rendering fidelity. Maintain this handoff plus COORDINATION-PLAN.md for follow-up changes.

### Final validation and build — 2026-10-27

- Targeted tests: **9 passed, 0 failed, no skips** (GIM linear/PSMCT32-transfer malformed contracts; all 139 Akagi GIMs and four RGBA hashes; close/fine zoom and manual/automatic projection; Akagi UV/LOD; RenderWare partition + 1,256 TXD records).
- Reader: **45/0/no skips**. Full window: **115/0, 85 optional skipped**. Logs: build/windows-local/idss-swizzle-camera-{focused,reader,window}-tests.txt. Application build succeeded: build/review-fixes-app-build.log. git diff --check passed.
- Development executable updated after the owner closed 9XD. New app launched successfully; current Akagi main scene shows the expected 122 loaded sets (121 visible with sky hidden), zero load failures and the existing single texture warning. There was active human input during attempted settings/close-zoom GUI inspection, so that specific visual interaction is left for user confirmation rather than claimed as tested. Projection/zoom behavior and manual-range validation are covered by tests.
- Use wheel for zoom, Shift+wheel for finer increments; viewport Display (layers icon) > Settings… exposes Automatic clipping, Near clip and Far clip in model units. Defaults are automatic. Existing loaded texture snapshots must be re-rendered in the updated app. Full original vertex-color/fog/blend fidelity and the single texture warning remain pending. No commit/push/release or game-data publication.
