# 9XD coordination plan

Updated: 2026-10-25 (0.3.6 release validated; publication pending). Shared between you, ChatGPT and Claude. Whoever finishes or changes something updates this file in the same step.

## Where things stand

The Windows UI refresh is built and running, and the Genki reference captures are finished; the work left is turning those captures into native 9XD readers. The canonical research checklist is now at `docs/research/genki/GENKI-CROSS-ANALYSIS-TODO.md`. Capture migration and TXRD2 March archive-to-fixture provenance are complete; other provenance, render tracing and native integration remain open.

| Area | State | Where it lives |
| --- | --- | --- |
| Mac UI port to Windows | Built and running with `windows-local`, not committed | `binary-workbench`, branch `main` |
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
| D. Native integration in 9XD | Claude | You | Reader code, tests, updated docs |
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

- [ ] TXRD2 March first: bounded reading of banks without a global TIM2 header (pixel records and aliases); use the two full matches as a positive fixture
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

- [ ] Commit the UI port on `main` and push it
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

1. You: commit the UI port in CLion and push `main`.
2. You: review and commit/push the research migration together with the desired repository changes; ChatGPT has not committed or pushed the UI or research files.
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
- [ ] Commit and push main, tag v0.3.6 and publish stable GitHub release with ZIP, checksum and public logo.
- Next: finish validation, replace the pending release-note validation text with actual results, then publish. The source-provenance/renderer work remains separate and is not advertised as new reader support.

Release validation complete: docs/releases/v0.3.6.md records actual results; docs/user-guide.md now explains the module sidebar and Ctrl+K. No game decoder changes were introduced. The local Genki images/dumps are excluded from public Git and the portable ZIP; scripts, inventories and reports remain shared documentation. Next: ChatGPT commits/pushes the authorized changes and publishes the stable release, then records its exact URL and commit.

Pre-publication source check: normalized whitespace in migrated research helpers; staged diff checked for game assets, raw dumps, private splash art and local presets. No application behavior changed by this cleanup.
