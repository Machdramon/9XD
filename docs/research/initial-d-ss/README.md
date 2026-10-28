# Initial D Special Stage (PS2): PAC + SMD compression

Source: the user's own executable `SLPM_652.68` (v2.00), analysed read-only with a minimal MIPS disassembler.
No game code is copied here; this note only describes the format.

- **PAC** (`PAC\0`): u32 count at +4, entry table offset at +12. Entries are 32 bytes:
  {name[16], offset, size, type, flags}. Flag bit 0 = compressed.
  - The loader at 0x17C470 walks the entries; 0x17CAC0 decompresses the flagged ones.
- **Compressed entry**: {u32 0x01DA3D12 (never checked), u32 raw size, u32 packed size}, then the stream.
  The decoder is at 0x1C54F0:
  - **Window:** 64 KiB, zero-filled, write position starting at 0xFEFD.
  - **Flags:** read lazily, LSB first (`flags >>= 1`; when bit 8 is clear, load a byte | 0xFF00).
  - **Bit 1 = literal:** one byte, written to the output and the window.
  - **Bit 0 = reference:** three bytes, little-endian 24-bit value w. Offset = w & 0xFFFF (absolute window
    position); length = (w >> 16) + 4.
- **Verified:** all 137 compressed entries of AKAGI_DAY.PAC decode to `SMD\0` with exactly the declared packed
  size consumed.
- **Next:** analyse the decompressed SMD model layout; textures are the `GIM\0` entries (type 1).


## Independent Akagi face/UV/LOD audit — Codex, 2026-10-26

This section supersedes the earlier winding/UV/lighting hypotheses in HANDOFF.md. Inputs were read-only; the game executable was NOT run and no game program/data bytes are included in the repository.

### Evidence

- Owner executable: `F:/download/Initial D - Special Stage (Japan) (v2.00)/Initial D - Special Stage (Japan) (v2.00)/SLPM_652.68`, 1,933,488 bytes, SHA-256 `a4d4587c50697057895f7b8183eca0abe5638910e0eeff7acf0dc642e74d2a80`. ELF32 load segment file offset 0x80, address 0x100000. Static VIF MPG uploads found at file offsets 0x1220dc–0x1240fc (first microprogram, VU PCs 0–0x4ea); additional program images also exist, so a runtime trace is still needed to establish every active path.
- `AKAGI_DAY.PAC`, SHA-256 `87e3c55560dd33aeba3b8129f895e4dfc263323daa999ae08710b02709b22ed3`: 137 SMD models, 3,111 records, 216,950 declared/decoded triangles, 415,596 vertices. Independent packet walk finds exactly 415,596 UV pairs, 399,754 nonzero pairs. Every record and every model retains its declared face count.
- 10,432 UV UNPACKs use **0x74**, not 0x64: V2-32 with mask enabled. The saved reader tested only `cmd==0x64`; all these UVs were silently replaced by zero. Fix: recognize the shape after removing mask bit 0x10. Example `crs01` record 0 starts UV `(4,-1)`, `(4.00006199,0)`, `(-4,-1)`, `(-4,0)`, not `(0,0)`. UVs repeat outside 0–1; this alone is NOT evidence of texel units or a need to divide by image size. Global V flip/scale remains unproven and is not applied.
- Per-batch control qwords are GIF tags: PRE=1, 10,351 PRIM=0x1c and 81 PRIM=0x5c. Both low three bits are **4 = triangle strip**; the extra 0x40 is fog enable, not another topology. Typical register descriptor is 0x412 (ST, RGBAQ, XYZF2). ADC/no-kick is position-W bit 15. Skipped kicks still contribute vertices to strip history and parity.
- First uploaded VU program: MSCAL 0x0c branches to 0x0a0; the transformed strip path at 0x221 onward reads incoming W with MTIR, combines clipping suppression and forwards W through MFIR to output XYZF. This supports preserving ADC rather than interpreting low W bit 2 as a new face-winding rule. The saved reader's `W & 4` winding inference is removed: standard per-batch strip parity is used. No BFS orientation propagation, per-piece +Y forcing or invented faces are added. Absolute front-face/basis equivalence and all microprogram variants remain runtime-validation work.
- Explicit named alternatives: `skylod00`, `crslod00`–`crslod36`, `mntlod00` (39 models). These contribute **58,654 triangles**; remaining named main resources contribute **158,296**. Previously they were merged into the same texture groups. Rendering them together risks overlapping surfaces/z-fighting. Split candidate sets by named LOD status and exclude LOD alternatives from default Render All. Naming is evidence for this preview policy, not a reconstruction of the game's visibility distances.
- Native color packets exist (10,392 V4-8 packets; 40 batches instead use V3-32 attribute packets). Original per-vertex colors, fog and source blend state are not reproduced by MeshVertex/the generic material path. The saved adapter also did NOT set unlit despite the earlier handoff saying it did. The corrected material uses an explicitly documented **unlit base-texture preview** to avoid extra normal-based viewport darkening. This is not faithful baked vertex lighting: color modulation/fog/GS effects remain pending.

### Applied changes and validation

`src/idss_core.h`: masked UV command handling and GIF strip parity. `src/idss_model.h`: main/LOD groups, optional LOD candidates and unlit preview with explicit sourceBindings limitations. `src/mesh_preview_registry.h`: texture and LOD are part of candidate identity, preventing equal-size different surfaces from being mistaken for one another. `tests/window_tests.cpp`: masked UV + non-ADC W=7 topology fixture, isolated LOD fixture, real Akagi declared-count/index/finite/UV and group-total checks.

Local private proof export: `crs01` record 0 OBJ with UVs and its linked decoded GIM; the texture is legible when inspected. Game-derived exports are not stored/published in the repo. Re-scan the PAC after updating; old candidates merge LODs and have stale counts.

- [ ] Live visual validation: road, walls and terrain using only the main candidate set; compare texture mapping and face orientation to the game before asserting fidelity.
- [ ] Native rendering owner: preserve/map source per-vertex colors, distinguish V3-32 attribute batches, reproduce PS2 modulation (neutral 128), alpha/blend/fog state; do not compensate with a global brightness/gamma/UV adjustment.
- [ ] Runtime VU/GS trace: identify which uploaded program variant executes for course draws, confirm front-face/basis and actual LOD visibility/selection. Do not claim all variants validated from the static first-program path.
- [ ] Performance follow-up: parse/cache PAC once per document revision rather than once per material decode (existing adapter still reparses). This audit does not add shared scene caching.

### Completed validation — 2026-10-26

Focused tests: 6 passed/0 failed/no skips (actual Akagi, masked-UV/strip/LOD synthetic fixtures, RenderWare isolation and LZW); reader: 45/0/no skips; full window: 113/0 with 84 optional skips; Genki context/WM/isolation/March: 7/0/no skips. Logs: build/windows-local/idss-analysis-{focused,reader,window}-tests.txt and genki-track-context-tests.txt. Real Akagi: 137 models, 216,950 triangles, 415,596 vertices, exactly 399,754 nonzero UVs; 158,296 default / 58,654 optional LOD triangles, 122 default texture surfaces. Development binary_workbench.exe rebuilt successfully (build/review-fixes-app-build.log); diff check passes. No commit/push/release. Automated tests and the private sample texture inspection do not replace in-game visual validation.

Dedicated current implementation handoff: [Claude / Initial D SS](../../CLAUDE-INITIAL-D-SS-HANDOFF.md). It records installed corrections, validation and remaining visual/material work; read it before following historical hypotheses.


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

## Future correction guide — 2026-10-27

The dedicated [Claude handoff](../../CLAUDE-INITIAL-D-SS-HANDOFF.md) now starts with a symptom-to-layer diagnosis table, the exact GIM transfer contract, camera unit/clipping requirements, focused test names, rebuild/live-check workflow and explicit remaining work. Use that current section instead of historical pending-build notes. This documentation pass changes no decoder or rendering behavior.
