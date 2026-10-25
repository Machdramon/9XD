# 9XD coordination plan

Updated: 2026-10-25 (0.3.7 release preparation and validation). Shared between you, ChatGPT and Claude.

## Where things stand

The Windows UI refresh is committed, pushed and published in stable release 0.3.6, and the Genki reference captures are finished; the work left is turning those captures into native 9XD readers. The canonical research checklist is now at `docs/research/genki/GENKI-CROSS-ANALYSIS-TODO.md`. Capture migration and TXRD2 March archive-to-fixture provenance are complete; other provenance, render tracing and native integration remain open.

| Area | State | Where it lives |
| --- | --- | --- |
| Mac UI port to Windows | Committed and pushed; Windows 0.3.6 Release and regression checks passed | `binary-workbench`, branch `main` |
| Mac branch on GitHub | `build/macos-support`, 5 commits ahead of `main`, not merged | GitHub `Machdramon/9XD` |
| Genki PCSX2 references | 9 captures from 5 games, with serials and SHA-256 | `docs/research/genki/`; original workspace copies preserved |
| Cross-analysis | Inventory and per-game separation done; linking, render tracing, validation and native integration open | `docs/research/genki/GENKI-CROSS-ANALYSIS-TODO.md` |

### UI changes ported from the Mac branch (2026-10-25)

Only visual changes; Mac build setup (macOS presets, OSX sysroot, MoltenVK shader rename) left out.

- New dark/light theme and stylesheet; blue accent for each panel's main action; new `accent.primary` theme token.
- Module rail on the left (9XD logo, module icons, Search and Settings at the bottom), replacing the horizontal module tabs.
- Ctrl+K command palette over menus, modules and main panel actions.
- Code-drawn icons (`workbench_icons.h`), empty-state hints (`empty_state.h`), command palette (`command_palette.h`).
- 3D module: new toolbar (Perspective, Realistic/Cel/Shaded/Wire, icon menus, Scan), larger transform tools, mesh count badge, coloured X/Y/Z, monospace transform fields, Reset before Apply, section cards with summary badges.
- Files: `theme.h`, `theme_tokens.h`, `module_windows.h`, `workspace_window.h`, `vulkan_panel.cpp`, `mesh_collection.cpp`, `mesh_controls.cpp`, `ai_chat_panel.h`, `image_workbench.h`, `magic_panel.cpp`, `script_library_manager.h`, `script_panel.cpp`, plus the three new headers.

## Workstreams and owners

| Workstream | Owner | Hands off to | Hand-off artifact |
| --- | --- | --- | --- |
| A. Provenance: link captures to source files | ChatGPT | ChatGPT (B) | Per-capture report: exact match, shared reference, hypothesis or pending |
| B. Render tracing: texture and palette uploads in draw order | ChatGPT | ChatGPT (C) | Upload timeline per capture, materials tied to draws |
| C. Validation cases, one game at a time | ChatGPT, with you for in-game checks | Claude (D) | A verified rule per variant, with fixture bytes and expected output |
| D. Native integration in 9XD | Claude; ChatGPT for the user-requested March/TIM2 fixes | You | Reader code, tests, updated docs |
| E. UI follow-ups from the Mac port | Claude | You | Commit on `main`, checked on Windows |
| Build, run and merge | You | Everyone | A green `windows-local` build, pushed to GitHub |

Only rules marked verified in C cross into D.

## Task list, in working order

### A. Provenance and reproducibility (ChatGPT)

- [x] Save captures, serials, hashes and a shared inventory
- [x] Separate game, version and condition for each reference
- [ ] Link each candidate to its original file and TOC entry by hash and size; don't equate hex ordinals, decimal names and physical offsets
- [x] TXRD2 March provenance: TOC entry 672 → bounded BUILD.DAT UCL reextraction → 2022/000002a0.sdb → test fixture, byte-identical; see `docs/research/genki/txrd2-march-capture/SOURCE-PROVENANCE.md`
- [x] Copy nine capture folders and shared checklist into `docs/research/genki/`, verify hashes and preserve originals
- [ ] Resolve the TXRD2 copies found inside folders labelled TXR3, and drop them from the TXR3 fixtures
- [ ] Keep one report per proof: exact match, shared reference, hypothesis or pending

### B. Follow the render in order (ChatGPT)

- [ ] Replay image uploads to VRAM in time order (BITBLTBUF, TRXPOS, TRXREG, TRXDIR), keeping state per draw
- [ ] Track TEX0, TEX1, CLAMP and the CLUT/CSA cache at draw time, not only the starting VRAM
- [ ] Compare indices, palette and final colour separately; support the pending RGB24 palette case
- [ ] Tie textures and palettes to specific materials and draws, allowing for reused memory and shared resources

### C. Validation cases (ChatGPT, you for in-game checks)

- [x] TXRD2 March first: bounded reading of banks without a global TIM2 header (pixel records and aliases); use the two full matches as a positive fixture
- [ ] TXRD2 Hakone: close the RGB24 case, find local banks in the upload history, check road, vegetation, signs and transparency
- [ ] C1 WRX: separate SportsWagon from sedan geometry and the kits/LOD; resolve the main atlas and body colour; don't infer the model from shared logos alone
- [ ] C1 Nikko: find which bank variant is Hare/Dry; compare `00002498.dat` images to the capture; resolve the main bank's geometry and check group 2498 to 2531 and placements
- [ ] TXRD1 Alto: confirm material/palette and the exact match to `00001363.dat`; separate kits and optional parts
- [ ] TXRD1 Hakone: find the original banks and tie them to the circuit; confirm direction and condition before labelling variants
- [ ] TXR Zero AE86: confirm `0x00000014.dat` with textures and structures; study rough geometry, LOD and overlapping parts. The filtered identification view is not a decoder fix
- [ ] TXR3 Evo V: review the upload history and specific atlases; one shared texture doesn't identify the source model
- [ ] TXRD2 AZ1 (optional): close only if it adds coverage March doesn't

### D. Native integration in 9XD (Claude)

- [ ] Turn only verified rules into native readers per variant, keeping C1, GIF-strip v2 and TXRD2 v1 separate
- [ ] Connect materials and textures to Scan for 3D Data and Render All; report unresolved resources
- [ ] Keep provenance, part/LOD selection and placements in Mesh Collection
- [ ] Validate GLB/FBX export with textures, UV, transparency and no duplicated geometry
- [ ] Add tests for confirmed cases and for rejecting incompatible variants; check image and geometry counts together
- [ ] Build, check in the app, and update the coverage docs

### E. UI follow-ups from the Mac port (Claude, then you)

- [x] Commit the UI port on `main` and push it (release commit `83630b7`, stable 0.3.6 published)
- [ ] Merge or retire `build/macos-support` so the two platforms don't drift
- [ ] Check the Mesh Collection count badge ("0 / 0 visible") shows when the panel is narrow
- [ ] Show Cmd+K instead of Ctrl+K in the search tooltip on macOS

## New options to build in 9XD

| Option | Where it shows in the UI | Waits on |
| --- | --- | --- |
| TXRD2 v1 bank reader (no global TIM2 header, pixel records and aliases) | 3D Data Scan and Scan for Textures / Images | TXRD2 March |
| RGB24 palette support | UV / 2D image settings | TXRD2 Hakone |
| C1 reader, kept separate from GIF-strip v2 | 3D Data Scan profiles | C1 WRX and C1 Nikko |
| Materials and textures attached to scan results, with an unresolved-resources report | Scan for 3D Data, Render All | B (render tracing) |
| Provenance, part/LOD picker and placements per mesh | Mesh Collection | C1 WRX, TXRD1 Alto |
| GLB/FBX export with textures, UV and transparency | File › Export | The two items above |
| Regression tests for confirmed and rejected variants | `window_tests`, `reader_tests` | Each reader as it lands |

## Coordination rules

**Mandatory project-owner rule (2026-10-25):** Every project change must be documented here in the same work session, before completion or hand-off. Refresh the date, update affected tasks, record changed files/evidence, validation, limitations and the next action/owner, and synchronize linked specialist checklists. This applies to all contributors and all future work, including code, UI, research, tests, configuration and documentation. A change is incomplete until this record is current. Root AGENTS.md defines the rule; CLAUDE.md explicitly applies it to Claude.

- This file is the shared plan. Tick tasks, change owners and add new items here, with the date at the top.
- Canonical capture folders and checklist now live in `docs/research/genki/`. Update this shared copy; original workspace copies are historical backups. Derived uncompressed dumps and reextracted game bytes remain local and Git-ignored.
- A rule crosses from research into code only when its report says exact match. Shared references and hypotheses stay in the notes.
- C1 extraction folders use different numbering schemes; never convert names between them automatically.
- Don't reuse TXRD2 IDs for TXRD1 assets, and don't treat a shared texture or logo as proof of a model.
- Ask for a new capture only when the upload history can't prove a specific state.
- The goal is native extraction that works without the emulator or AI in normal use.

## Next three actions

1. Claude / user: review the remaining narrow Mesh Collection badge and macOS shortcut-label UI follow-ups; the Windows UI port is already committed and released.
2. User: test the portable 0.3.6 Windows release; Claude / user retain ownership of merging or retiring build/macos-support. Research reports/checklists are pushed; game-derived capture files remain local.
3. ChatGPT: continue March upload/draw tracing and bounded headerless-bank validation. Provenance is exact; per-mesh bindings and alias semantics still need proof before Claude integrates them.

## Research hand-off — 2026-10-25

ChatGPT confirmed read/write access and preserved all uncommitted UI changes. Nine capture directories were copied to `docs/research/genki/` with per-file SHA-256 verification; original workspace copies were retained. A local .gitignore excludes reproducible raw GS/state dumps and the single reextracted game payload. No commit or push performed.

TXRD2 March source provenance is exact: BUILD.TOC has 19,869 16-byte records; entry 672 addresses BUILD.DAT byte 182,845,440, compressed length 181,593 and decoded length 626,528. Reextraction matches both source and fixture SHA-256 `fbf9042dde61c5de82e6b458c83d003c7c36fd9433b2d6f425ff99349aa618ed`. This verifies source identity, not unique runtime geometry/LOD selection. The archive-to-fixture report is ready for Claude; native reader and per-draw material integration remain open.

## Project-rule update — 2026-10-25

- [x] Add the owner's permanent coordination/documentation requirement to root AGENTS.md and CLAUDE.md, and synchronize this plan.
- Validation: read back all three documents; no application source changed and no build needed for this documentation-only change.
- Next owner: every contributor must follow this rule starting with the next project change. Existing research and UI tasks retain their owners.


## Release 0.3.6 — 2026-10-25

Owner: ChatGPT, explicitly authorized by the user to commit/push the UI update and publish a full stable release.

- [x] Review the Windows UI port and preserve Mac-specific exclusions.
- [x] Update CMake project version, application version, Windows version resource, README and release notes to 0.3.6.
- [x] Keep game-derived captures/media local and Git-ignored; publish research reports/checklists only. Exclude docs/research from the portable package. Files: docs/research/genki/.gitignore, docs/research/genki/README.md, tools/package_windows.py.
- [x] Build Windows Release and test targets; reader 45 passed/0 failed, window 95 passed/0 failed/74 optional skipped. Focused command-palette/module-rail UI check and portable ZIP/runtime checks passed.
- [x] Commit and push main, tag v0.3.6 and publish stable GitHub release with ZIP, checksum and public logo.
- Release 0.3.6 is published and its receipt is below. Research remains separate and is not advertised as new reader support.

Release validation complete: docs/releases/v0.3.6.md records actual results; docs/user-guide.md now explains the module sidebar and Ctrl+K. No game decoder changes were introduced. The local Genki images/dumps are excluded from public Git and the portable ZIP; scripts, inventories and reports remain shared documentation. Publication completed; see the receipt below.

Pre-publication source check: normalized whitespace in migrated research helpers; staged diff checked for game assets, raw dumps, private splash art and local presets. No application behavior changed by this cleanup.

## Release publication receipt — 2026-10-25

- Stable public release (not draft, not prerelease): https://github.com/Machdramon/9XD/releases/tag/v0.3.6
- Release code commit and tag target: `83630b7c0b2cb5caaaffe90435fa94ed1fe614dd` (`v0.3.6`).
- Assets verified through GitHub API: Windows ZIP, .zip.sha256 and 9XD-logo.png; release verified as latest.
- ZIP SHA-256: `d2be554a811e5680bdbc267e3e9b1d9735f112ffa8e1e0c162a6a637bef96e70`.
- Windows Release deployment and portable Python checks passed; reader 45 passed/0 failed; window 95 passed/0 failed/74 optional skipped; focused command-palette/module-rail smoke passed.
- Standalone splash art and game-derived captures excluded from public source/package. Local captures are preserved.
- This follow-up documentation records publication after the release code commit; the tag continues to identify the tested application build. No application code changes in this receipt.
- Next: user validates 0.3.6 in normal workflows; ChatGPT resumes March draw/upload tracing and verified reader hand-off; Claude owns native integration and remaining UI items.

## Capture cross-check hand-off — 2026-10-25

Owner: ChatGPT. Canonical report: `docs/research/genki/checks/RESULTS.md`; compact machine evidence and positive/negative checks beside it. Full timelines are local under `checks/local/`, Git-ignored. Scripts are under `docs/research/genki/scripts/`. No application source was changed; no commit/push or new release in this research step.

- [x] All nine compressed hashes and fresh decompressions match; embedded serial/state/GIF bounds verified.
- [x] Replay 10,546 supported host-image transactions in chronology and sample source indices at observed drawing bindings. Conservatively invalidate framebuffer/Z-buffer and local-copy destinations. This is partial B, not full GS or CLUT-cache emulation.
- [x] Separate stored palette checks, including the pending RGB24 Hakone global surface at 0x6ef40. Exact storage equality is verified; final colour/cache emulation remains open.
- [x] March research bank admission prototype: six slots with 3 pixel records and 3 unresolved 48-byte aliases; real positive fixture, 14 malformed/truncated rejections and 9 synthetic transfer/invalidation checks passed.
- [x] Alto capture/source correction: USA study fixture is SLUS-21236, capture is SLES-53191 Europe; Europe candidates 00000442/00000447 yield two distinct full storage pairs. Do not transfer resource IDs between builds.

Deduplicated exact index+stored-palette pairs at drawing bindings: March 2; TXRD2 Hakone 36 (including two local banks now observed); C1 Nikko 30; C1 WRX 6; Europe Alto 2; TXR3 c4 1; initial AZ1 1. Zero has one indexed-payload match but no palette confirmation. Counts do not imply unique models/materials; C1 Nikko's 85 matching source records collapse to 30 payloads shared across variants. The Europe Hakone search found shared images across many banks and does not identify the circuit variant.

**Next ChatGPT work:** complete CLUT reload/cache semantics and 16-bit palette handling, TEX1/CLAMP/effective context state and Zero local copies; resolve March aliases/material slots and exact scene/LOD/condition identity. **Claude hand-off now available:** the narrowly verified March record layout and positive/negative fixtures, plus the RGB24 storage observation. Native integration (D) and the broader C tasks remain unchecked until implemented and verified in scan/render/export. Do not hardcode runtime relocation or fill unresolved aliases with nearby images. No additional game captures needed yet.

## Native material implementation — 2026-10-25

The user explicitly asked ChatGPT to trace and implement the material/texture fixes, superseding the previous Claude-only ownership for this bounded change. ChatGPT owns this implementation and its validation; Claude retains other ongoing integration/UI work.

New evidence: `correlate_march_uv.py` compares source UV pairs against captured packed ST/Q. All 342 primary pairs of the first dual-UV body group match runtime; none of its 650 secondary pairs match the primary atlas. Material prefix +68 carries namespace 2 and slot at +70; +72 is GS PRIM/TME and +88 is CLAMP. Primary slots 0 (512×512) and 4 (256×256) have matching source pixels/palettes and runtime UVs.

Implementation in progress: bounded headerless picture bank with preserved alias slots, v1 primary material references, 8-byte UV strides, encoded-image previews, and corrected TIM2 RGB24 interpretation. Files: src/genki_texture_bank.h, src/genki_materials.h, src/genki_mesh_evidence.h, src/genki_geometry.h, src/image_containers.h, src/image_decode.h, src/image_workbench.h, src/asset_discovery.h. No topology rules changed; aliases/secondary stages remain unresolved. Tests, corpus exports and build are pending before completion.

Build preparation: use explicit qsizetype arguments for QByteArray adaptation to avoid the MSVC/Qt overload ambiguity; native probe validation continues.

Native probe result: March retains 54 parts and admits primary textures on 19 parts; 51 packets now expose the verified 8-byte dual-UV stride. Added synthetic bank/alias/RGB24 tests and an optional real March decode/stride test; existing v1 export test now checks decoded texture presence against the actual material receipt. Also extended the strictly checked Zero transfer-form profile to observed 16-pixel dimensions and RGB16 palette banks; complete transfer coverage remains required. Build and regression pending.

Export validation preparation: preserve each original texture slot in surface names when a headerless record is adapted internally to single-picture TIM2; prevents different source slots sharing an export texture name.

Validation extended to the actual Scan image candidates and RAM-backed encoded-image decoder: March must expose exactly its three stored image slots and decode each to its declared dimensions; empty aliases are not image candidates.

Integration regression caught an obsolete blanket assertion that v1 materials never contain texture references; replaced it with explicit March 19-slot positive checks while XVI/embedded cases remain unresolved. Also routed the 3D inspector's material-texture Preview action through the headerless container hint, with an actual scan→select→Preview UI regression. Rebuilding/retesting before completion.

End-to-end UI regression exposed the missing `container` key in the shared image-settings schema, and saved recipes only allowed Auto/GTF. Added one common image-container enum used by UI, recipe serialization validation and AI/native settings validation. Existing normal TIM2 material previews and new headerless previews must both pass before completion. Files: src/image_recipe.h, src/image_recipe_io.h, src/ai_decoder_setup.h, src/image_workbench.h.

All 14 targeted Genki checks now pass. Export/sampler review found one remaining requirement: the verified full-image v1 CLAMP state must be passed to the existing Vulkan/GLB/FBX texture-wrap fields. Added Clamp U/V receipts and assertions so UV values near/outside the edge sample the border rather than repeat. Final rebuild and full window regression follow.

## Native implementation completion — 2026-10-25

- [x] Bounded headerless March bank with all 6 slots preserved; only 3 stored images become image candidates, aliases remain unresolved.
- [x] Verified v1 primary material slot, TME/CLAMP and primary UV pair (stride 4 or 8), with actual scan→material Preview and RAM-image decoding. 54 parts / 8,328 triangles, 19 primary textured parts.
- [x] Clamp sampling preserved through existing Vulkan, GLB and FBX material fields; source slot names remain distinct.
- [x] TIM2 RGB24 corrected; observed Zero transfer layouts/RGB16 palettes admitted with complete written-byte coverage. AE86 reference: 42/42 images decoded, 26 native local material references.
- [x] Development application built and executable updated after the user closed 9XD. Final fixture-enabled Genki run 14 passed/0 failed/0 skipped; reader 45 passed/0 failed; full window 96 passed/0 failed/75 optional skipped. GLB structure independently checked (54 meshes, 8,328 triangles, 19 textured materials, 3 distinct image payloads, clamp samplers); FBX export passed.

Implementation files include src/genki_texture_bank.h, genki_materials.h, genki_mesh_evidence.h, genki_geometry.h, image_containers.h, tim2_storage.h, image_decode.h, asset_discovery.h, asset_scan_panel.h, image_recipe.h, image_recipe_io.h, image_workbench.h and ai_decoder_setup.h, plus tests/window_tests.cpp. Documentation updated in docs/genki-topology.md, docs/native-scanning.md and the shared Genki checklist. Evidence and recipe details: docs/research/genki/checks/MATERIAL-INTEGRATION.md.

The broad D workstream remains open: external/alias resources, secondary shader/reflection layers, part/LOD visibility and cross-file track bindings are not complete. User can now test the updated windows-local executable with March Scan/Render All and image previews. Claude can continue from the verified native implementation; do not reintroduce the old blanket v1-no-texture assumption. No game source files were modified; no Git commit/push or new public release was performed for these changes.

## Reconstruction tools preparation — 2026-10-25

User explicitly confirmed both new tools: video-to-3D and image/spline modeling. Owner: ChatGPT/Codex. Specialist checklist: docs/reconstruction-tools.md.

- [x] Document staged architecture, shared viewer integration, deterministic geometry versus optional AI suggestions, resource limits and separate SAM image encoder requirement.
- [x] Add tools/prepare_reconstruction_tools.py: optional local downloads, upstream SHA-256 checks, archive traversal validation and local receipts under ignored build/reconstruction-tools.
- [x] Stage verified FFmpeg/COLMAP archives and core Open3D/ONNX Runtime wheels; verify executable startup and initial video extraction. Full Python dependency/runtime checks remain pending.
- [ ] Implement asynchronous job runner, Settings dependency paths, frame extraction and sparse reconstruction, then native splines and optional segmentation.

No application UI or reader behavior changed. No commit, push or release requested. Existing uncommitted Genki implementation preserved. Binary staging is preparation only; complete Python environment, model selection, runtime integration and workflow validation remain pending. Next owner: ChatGPT/Codex.
Reconstruction preparation result: official COLMAP 4.2.1 no-CUDA and FFmpeg 9.0.2 archives checksum-verified; COLMAP help and FFmpeg version checks passed. Provider FFmpeg endpoint stalled, so downloader now uses the provider's official GitHub mirror. Open3D 0.19.0 and ONNX Runtime 1.23.2 CPython 3.12 core wheels staged with hashes, not installed. Added tools/reconstruction_frames.py: bounded, non-overwriting frame extraction with ffprobe metadata and job manifest. Generated video smoke passed: 3 seconds, 3 frames, maximum dimension 256. docs/reconstruction-tools.md updated with actual receipt and pending tasks. No app rebuild required: application code unchanged. Next: isolated dependencies, asynchronous app job runner/COLMAP workflow, native spline foundations, validated optional SAM model pair. No commit/push/release performed.
## Reconstruction inspector integration — 2026-10-25

Owner: ChatGPT/Codex for functional integration, explicitly requested by user; Claude owns subsequent UI definition/refinement. Updated specialist contract: docs/reconstruction-tools.md.

- [x] Add document-scoped Video Reconstruction and Image / Spline Modeling inspectors in the 3D module, using existing docking/menu ownership.
- [x] Implement asynchronous native FFmpeg extraction with unique job directories, resource bounds, atomic manifest, bounded logs and cancellation; spline point project save/load with strict validation and unsaved-change guard.
- [x] Build application/test targets; focused inspector/runtime checks and full window regression passed (results below).
- [x] Claude: define/refine inspector UI and graphical controls after reviewing the functional contract (see Claude reconstruction UI result below).
- [ ] ChatGPT/Codex: COLMAP sparse jobs/import, shared dependency Settings, calibrated spline modeling and optional segmentation remain future work.

Changed: src/reconstruction_inspectors.h, src/main.cpp, src/module_menus.h, tests/window_tests.cpp and specialist documentation. No commit/push/release requested. Compilation and tests completed as recorded below; no completed reconstruction or surface generation claimed.

Inspector integration completion: development executable updated successfully. Focused inspector run 4 passed/0 failed/0 skipped (schema round-trip and malformed-input preservation, dock registration, real native frame extraction, cancellation and output isolation). Full window regression with compiler environment: 98 passed/0 failed/75 optional skipped. Initial full run omitted vcvars setup and failed two existing C++ scripting checks due missing standard include paths; rerun with the required environment passed. git diff --check passed. Logs: build/reconstruction-inspectors-build.log, build/windows-local/reconstruction-inspectors-tests.txt and reconstruction-window-full.txt.

User entry points: 3D → Video Reconstruction; 3D → Image / Spline Modeling. Functional presentation only; Claude owns final layout/interaction refinement. Full COLMAP processing/import, image viewport spline drawing/3D fitting, dependency preferences and segmentation remain unchecked future work. Updated docs/reconstruction-tools.md provides the object names and JSON/job contracts. No publication or commit performed; existing unrelated work retained.

## Claude reconstruction UI hand-off — 2026-10-25

User explicitly requested passing the new-tool instructions to Claude. No identifiable Claude session is accessible through the available chat tools, so a durable repository hand-off was created in docs/CLAUDE-RECONSTRUCTION-HANDOFF.md and linked from CLAUDE.md. It specifies UI ownership, source files, object/data contracts, prepared dependencies, pending backend scope and required validation/documentation. Status: ready for pickup; no direct message delivery or acceptance claimed. Documentation only; no application changes/build needed. Next owner: Claude, after the user points their session to the hand-off.

## Claude reconstruction UI pickup — 2026-10-25

Owner: Claude. Hand-off docs/CLAUDE-RECONSTRUCTION-HANDOFF.md picked up; work in progress, not complete. ChatGPT is not editing these files during this work (confirmed by the user).

Scope agreed with the user: redesign both inspectors and add graphical point drawing on the reference image now, in the existing UV / 2D image view (no second viewer).

- [x] Video Reconstruction: pipeline strip (frames, then sparse reconstruction and import shown as not available), Source / Sampling / Output cards, FFmpeg tool row, primary Extract Frames, progress, status, collapsible log, frame thumbnails and Open Folder.
- [x] Image / Spline: reference image card with size and bounds check, curve organization (picker, colours, new curve, reorder), toolbar, clear save/load feedback.
- [x] Drawing (see decision below): reference image drawing, click to add, drag to move, synchronized with the point table; pixel coordinates and order preserved.
- [x] Build windows-local, focused reconstruction tests, menu/docking regression, visual check; record results here and in docs/reconstruction-tools.md.

Contracts kept: object names used by tests, job.json/output folders, spline JSON schema 1, sampling limits, close/discard guards, dock/menu ownership. No commit/push/release.

## Claude reconstruction UI result — 2026-10-25

Owner: Claude. Hand-off docs/CLAUDE-RECONSTRUCTION-HANDOFF.md implemented. No commit, push or release.

Changed files: src/reconstruction_inspectors.h (both inspectors rewritten; functional contracts kept), src/theme.h (inspector card, pipeline step, state-coloured status and canvas rules), src/workbench_icons.h (plus, minus, up, down, film, curve icons), tests/window_tests.cpp (new reconstructionInspectorDrawing test; canvas looked up as QGraphicsView because SplineCanvas has no Q_OBJECT). main.cpp was rewritten byte-identical only to force Ninja to recompile it: its dependency record did not pick up the header change.

- Video Reconstruction: pipeline strip (1 Frames current; 2 Sparse points and 3 Import to 3D shown as unavailable, never enabled); Source card with ffprobe length/resolution/fps; Sampling card with live frame estimate and a warning below 2 frames; Output card; collapsible Tools row with FFmpeg path and found/not-found state; primary Extract Frames and Cancel; progress bar counting written frames; state-coloured status (ready, running, done, attention, canceled, failed); collapsible bounded log that opens on failure; results card with up to 8 thumbnails and Open Folder.
- Image / Spline: reference image card with size and read errors; drawing canvas; curve picker with colours and New curve; Points card with add/remove/move up/move down; out-of-bounds and invalid rows highlighted; project name with Unsaved marker; Save turns primary when dirty; save/load messages, including "Nothing was changed" on rejected loads.
- Drawing decision: the canvas lives inside the spline inspector, not the UV / 2D view. The UV view belongs to the UV module and the spline dock to the 3D module, and only one module shows at a time, so drawing in UV would hide the point table. The canvas reuses the UV preview interaction (wheel zoom, drag pan, fit) in a small dedicated QGraphicsView. Click adds to the active curve (inserted after that curve's last point), drag moves (clamped to the image), Delete removes, F fits. The table stays the source of truth; coordinates are image pixels rounded to 0.01 when drawn.
- Contracts preserved: all object names used by tests, job.json/output folders, sampling limits, spline JSON schema 1, unsaved-change and active-job guards, dock/menu ownership. New object names: reconstructionMaxSize, reconstructionCancel, reconstructionProgress, reconstructionLog, reconstructionOpenFolder, reconstructionThumbnails, reconstructionPipeline/pipelineStep, splineCanvas, splineActiveCurve, splineNewCurve, splineAddPoint, splineRemovePoint, splineMoveUp, splineMoveDown, splineSaveProject, splineLoadProject, splineStatus.

Validation actually performed (CLion, windows-local, MSVC 2022, Qt 6.11.2): binary_workbench and window_tests built with no errors. Focused: reconstructionInspectorContracts, reconstructionInspectorDrawing (new) and reconstructionFrameJob (real staged FFmpeg extraction and cancel) all PASS. Full window_tests from CLion run configuration (real display, no vcvars): 94 passed / 5 failed / 75 skipped. The 5 failures are environment-only and outside the changed code: two C++ scripting tests (cannot open iostream/cstdint without vcvars), hexDisplayControlsAndOffsetNavigation and compactTabsAndPinnedHexFooter (layout checks that expect offscreen; the footer one passed on a second run) and vulkanPanelFailsGracefullyWithoutDesktop (expects no Vulkan desktop). CTest (offscreen) from CLion: reader_tests passed; window_tests failed without captured per-test output, most likely the two vcvars-dependent scripting tests. Visual check: Video Reconstruction panel inspected in the running app; the spline panel was not visually inspected (session limit reached) and is covered by the automated drawing test.

Remaining / next owner:
- [x] ChatGPT/Codex: full window suite rerun for release 0.3.7 with VS2022 vcvars64/offscreen: 99 passed / 0 failed / 75 optional skipped.
- [ ] User: visually check Image / Spline Modeling with a real image (3D → Image / Spline Modeling) and report layout issues to Claude.
- [ ] Claude: cap the width of the Sampling spin boxes in very wide floating docks (cosmetic).
- [ ] Later (ChatGPT/Codex backend, Claude UI): COLMAP sparse job and import, shared Settings dependency paths, image planes in the 3D view, calibrated fitting/lofting, optional segmentation. The pipeline strip steps 2 and 3 should be enabled only when those backends exist.

## Release 0.3.7 — 2026-10-25

User authorized update, push and full stable release. Owner: ChatGPT/Codex. Includes verified native material work, reconstruction inspector foundations and Claude's UI/drawing refinements. Preserve all existing contributor work.

- [x] Version CMake/application/Windows metadata, README and release notes to 0.3.7; state feature limits and optional external dependencies explicitly.
- [x] Release build, reader/window regression with vcvars64/offscreen, fixture-enabled material checks and portable ZIP verification.
- [ ] Review staged source for private assets, commit/push main, tag v0.3.7 and publish stable ZIP/checksum/public logo.

Changed version/release files: CMakeLists.txt, src/main.cpp, assets/app.rc, README.md, docs/releases/v0.3.7.md. No external reconstruction binaries or game captures enter public source/package. Next: complete validation and publication, then record receipt.

Release 0.3.7 validation: Windows Release/package build and deployment/runtime checks passed; reader 45 passed/0 failed; window 99 passed/0 failed/75 optional skipped; fixture-enabled Genki 14 passed/0 failed/0 skipped. Claude's requested compiler/offscreen rerun is complete. Final package regenerated with completed release notes before archive verification/publication. Qt deployment warns only that optional translation catalogs are unavailable; the app UI is English. Clean-machine validation remains pending.

Final portable archive verified: 9XD-0.3.7-windows-x64.zip, 73,939,562 bytes; SHA-256 3cc15d2d5885cafee1b672d6b0443bfbf4bdeaee02fe5ab33a084dddc31dc846. ZIP contents, required dependencies, Windows GUI subsystem and equality with tested Release executable verified. No research captures, reconstruction binaries/model weights or standalone splash art included. Commit/push and stable publication follow; receipt will be recorded afterward.
