# 9XD coordination plan

Updated: 2026-10-28 (Codex: preparing owner-authorized 0.4.0 commit, push and stable release).






## Release 0.4.0 - 2026-10-28 (in progress)

- [x] Owner authorized committing/pushing the current combined work and publishing stable 0.4.0. Reviewed changes since 0.3.7; notes distinguish validated fixture coverage from incomplete readers/material semantics. Preserved Claude contributions.
- [x] Version bumped in CMakeLists.txt and src/main.cpp; README download links and docs/releases/v0.4.0.md updated. Added required Windows runtime cat icon to public asset allowlist so clean Windows source builds have their referenced resource. Private splash and game/capture assets remain excluded.
- [x] Clean windows-release build/package passed (build/release-configure.log, build/release-package.log); EXE file/product version 0.4.0. Reader 45 passed /0 failed /0 skipped; window132 /0 /86 optional fixture skips (build/windows-local/release040-*-tests.txt). Portable deployment, bundled Python/script runner, ZIP CRC, GUI subsystem, required runtimes and source EXE equality checked. Separate clean-machine test pending. Whitespace-only cleanup in two research files; no raw game/capture/splash source assets staged.
- [ ] Commit/push/tag and stable publication pending. No new format claims introduced by release preparation; newer validation above supersedes historical untested UI notes for build/test coverage, not every visual combination.

## Viewport ignore-alpha preview - 2026-10-28 (tested and installed)

- [x] Added live Ignore texture alpha checkbox in 3D View Settings, public panel getter/setter and tile/new-viewport inheritance. Vulkan fragment shader forces sampled alpha1 before alpha discard when preview flag is on. RGB/source images/materials/exports unchanged; no mesh rebuild/texture upload required. Default off.
- [x] Shader compiled; focused settings/tile inheritance test3 passed/0 failed/no skips (build/windows-local/viewport-ignore-alpha.txt). Setting toggles live and propagates to split views/new viewports; no asset edits or texture buffer rebuild.
- [x] Full suite: 132 passed, 0 failed, 86 optional skipped (build/windows-local/blur-window-full.txt); application linked successfully (build/blur-app-build.log), installed after owner closure. Live BARCELL scan loaded EDIFICIO_1 (3,092 vertices / 1,654 faces); Settings checkbox toggled on and previously discarded textured faces become visible. Left preview enabled for owner review. This verifies the override, not game-correct alpha/material semantics. No commit/push/release.

## Super Trucks completeness / Render All correction - 2026-10-28 (tested and installed)

- [x] Owner correctly challenges sparse geometry. Independent BARCELL native geometry walk: 558 declared Geometry chunks, old decoder emitted 278 (280 empty), 19,082 primary-scene triangles vs29,194 declared. Serialized flag1 blocks begin with a DMA RET tag; old nativeMesh misread it as S-32 UNPACK and skipped data. Bounded prototype TTE at+8, QWC payload end, no relocation-tail parsing: all558 geometry chunks emit, 29,114 primary-scene triangles. Remaining80 declared-count difference unproven; private instrumentation finds substantial duplicate-position pruning but does not prove the exact80 difference.
- [x] Added bounded DMA RET handling in src/rw_core.h and synthetic bounds/address/truncation tests. Render All default exclusions now explicit: button Render Default Set, scan option includes alternative geometry/texture sets and changes label to Render All. Normal default policy retained, explicit all includes variants/unplaced while preserving heuristic/part-of-candidate safety filters.
- [x] Independent main-PS2-SLI walk across10 circuits plus CARSEL: every enumerated Geometry chunk emits, zero empty chunks. Actual production default corpus still100% textured, all2,237 rasters decode. BARCELL default106 surfaces/30,586 triangles vs old77/20,138. Focused11/0/no skips; full131/0/86 optional skips. Details and all per-track denominators: docs/research/renderware/SUPER-TRUCKS-GEOMETRY-COMPLETENESS-2026-10-28.md.
- [x] Application rebuilt/installed after owner closure. Live BARCELL default set:106 entries/30,586 triangles, zero failures/omissions/warnings. Enabled explicit alternatives option: Render All processed all206 recognized results,100 added+106 already present,0 failed/omitted,66,280 triangles total. Left open in Wire with alternatives enabled to inspect geometry independently of alpha. All-results includes overlapping dry/rain/BSP/environment alternatives, not a single scene.
- [ ] Prior100% assignment did not prove completeness; earlier sparse counts superseded. Residual declared-vs-decoded small triangle differences, alpha holes/red/dark material behavior and scene instance/placement fidelity remain open. No source game changes, commit/push/release.

## Super Trucks UV2 integration - 2026-10-28 (verified primary UV coverage)

- [x] Added address-aware V4-32/address1 primary XY UV decoding to src/rw_core.h; preserved position/address0 path and existing V2 path. Per-batch finite/exact UV counts and conservative whole-part coverage prevent partial, truncated or mixed-format streams being reported as textured. Geometry retained if UVs invalid.
- [x] Focused suite 8 passed/0 failed/no skips including synthetic UV2 isolation/count/NaN/truncation/batch-completeness and actual D1GP/Super Trucks corpus regression. Actual production decoder now assigns images to 100% of default surfaces/triangles across all ten circuits and two ancillary files; all 2,237 TXD images decode, zero geometry/image failures or bank warnings. D1GP existing counts/coverage retained. Detail: docs/research/renderware/SUPER-TRUCKS-UV2-INTEGRATION-2026-10-28.md.
- [x] Full suite: 129 passed/0 failed/86 optional skipped; development executable rebuilt and installed after user closure. Logs build/windows-local/blur-window-full.txt and build/blur-app-build.log. Live BARCELL scan/Render All loaded 77 surfaces, 20,138 triangles, 76 new +1 existing, zero failures/omissions/texture warnings; left open for review.
- [ ] Visual fidelity remains open: some BARCELL facades appear solid red/dark despite valid UV0..1. Isolated production EDIFICIO_1 RGB image is a coherent non-red facade, but alpha=0 for 28,837/32,768 pixels; ASFALTO1 alpha also varies. These may be effect/mask data rather than coverage; current rw_model blanket alphaMask=true needs material/render-state investigation. Do not claim game-correct appearance from 100% base-image assignment. Private visual.cpp and isolated image evidence in workspace supertrucks-research. No shader/alpha guess applied; next Codex: source material alpha/blend semantics and visual comparison, secondary UV/shading, plus optional geometry and Selene placement. No commit/push/release.

## Super Trucks executable UV2 analysis - 2026-10-28

- [x] Read-only SLUS_214.50 ELF analysis found exact named cluster descriptors PS2uv (V2-32 template0x64002001) and PS2uv2 (V4-32 template0x6c002001), VA0x535e30/0x535e40. Pipeline registration string xref also recorded. No complete decompilation or runtime shader proof.
- [x] Private instrumented production-core copy identifies omitted V4-32/address1 UV streams in missing-UV batches. Private prototype uses first XY Float32 pair at stride16, leaving topology unchanged; all 12 archives / ten circuits have 100% default triangles UV+image-ready, zero UV count mismatches/nonfinite values and identical triangle counts. Existing prior image validation remains 2,237/2,237. This is prototype readiness, NOT installed production textured coverage or visual game equivalence.
- [x] Evidence, exact descriptor addresses, file hash, per-track denominators and integration checks: docs/research/renderware/SUPER-TRUCKS-EXECUTABLE-UV2-2026-10-28.md. Private harness/reports under workspace supertrucks-research. No app code/build/release changed, sources read-only.
- [ ] Next Codex: integrate bounded/address-aware UV2 path with channel isolation/malformed tests and actual BARCELL/STADIO/ALBACETE plus D1GP regression, then visual road/sign/channel/orientation checks. Exact secondary UV/shader semantics and Selene part placement remain unresolved.

## Super Trucks multi-track coverage - 2026-10-28

- [x] Owner requested testing other tracks; located full extracted game under F:/downloads/Super Trucks Racing 2/Super Trucks Racing 2/Tracks. Current production scan/mesh decode checked every default candidate in 12 MIX archives (10 circuits including Albacete control, CARSEL and ENVSPHER ancillary resources), plus every TXD record. Zero default mesh failures, image failures or archive warnings; 2,237 images decoded (1,993 from nine additional circuits).
- [x] Additional default triangle texture coverage: ALPI 79.84%, BARCELL 72.12%, BERLINO 84.47%, CORSICA 77.39%, COSTA 84.22%, HIGH 79.27%, STADIO 95.40%, TOSCANA 76.64%, VALENCIA 78.67%. Exact denominators, surfaces, missing material names and source SHA256 in docs/research/renderware/SUPER-TRUCKS-TRACK-COVERAGE-2026-10-28.md. Baseline 43,974/52,136 rounds to 84.34%, correcting historical 84.35% rounding only.
- [x] All untextured default surfaces have an image binding but mesh hasUvs=false. Supported recovered TXD encodings across corpus: PSMT4/PSMT8/PSMCT32/PSMCT16S. Surface-level conservative UV aggregation may suppress texturing on otherwise valid contributing parts. No absent-image claim or blanket flip. CPU production decode, not live GPU/game equivalence; optional geometry excluded from default coverage.
- [ ] Next Codex: native VIF UV/batch recovery, compare BARCELL and STADIO then repeat same measurements. No app code/build/release changed; source game assets read-only. Private reproducible harness and JSON in workspace supertrucks-research; source hashes in derived report. Prior full suite not rerun for research-only work.

## Blur multi-draw integration - 2026-10-28 (verified local-section scope)

- [x] Added bounded per-draw descriptors and local-section scan candidates in src/blur_model.h. Exact index partition, minimum/count and stream offsets validated; no multi-draw texture guessing.
- [x] Focused suite 10 passed/0 failed/no skips: six actual models, malformed ranges/overlap/stream offsets, stale candidates and scanner Render All/dedup. Unused vertices compacted, source offsets retained. Log build/windows-local/blur-multidraw-tests.txt.
- [x] Full window regression: 128 passed/0 failed/86 optional skipped (build/windows-local/blur-window-full.txt); application rebuilt successfully (build/blur-app-build.log), installed after owner confirmed closure. Live Vulkan normal scan + Render All: pallet 9 entries/526 triangles, standinglight 10 entries/2610 triangles, zero failed/omitted and existing first entry skipped. Both visibly recognizable; left lamp active, pallet in adjacent document.
- [ ] Object transforms/dynamic state selection and original multi-draw materials unresolved. Lamp Render All visibly overlays bent/alternative pieces; all sections are local inspection candidates, not a verified single game state. Texture warnings are expected (no guessed multi-draw assignment). Next Codex: per-object state/material reference resolution; Audi and full maps remain separate pending work. No release/publication; source game files unchanged.

## Blur pallet / standing-light rejection — 2026-10-28

- [x] Owner reports no 3D scan results on dynamicwoodenpallete and standinglight. Read-only original shared/construction fixtures confirm CPModel with stride20 geometry; current native reader intentionally requires exactly one full-buffer draw, so these multi-draw files are excluded, unrelated to RAM, filename or missing geometry.
- [x] Independently validated six files: dynamicwoodenpallete variants 1/2/3/4 have 9/9/10/9 draw ranges, 989/1365/2479/3787 vertices; standinglight/standinglight2 each 10 draws and 3898/3850 vertices. All ranges exactly partition each index buffer; declared minimum/count agree with observed indices and streamByteOffset + index*20 remains in bounds. Per-draw strip reset produces 526/738/1268/1930 and 2610/2730 nondegenerate triangles, respectively. Aggregating strips would create 16–18 extra boundary faces.
- [ ] Codex next: implement/test counted multi-draw reader preserving stream byte offsets and per-draw strip parity, then resolve object transforms/state/LOD and material references before claiming assembled textured objects. Current private probes do not prove which dynamic pieces/states belong together. No code/build changed, no live render of these unsupported fixtures claimed. Workspace evidence: blur-research/pallet-light.private.json and pallet_light*.py; source files unchanged.

## Blur additional UV fixtures — 2026-10-28

- [x] Owner confirms named prop texture is correct, asks for further UV comparison. Opened original trashcontainer_small.model through normal Scan for 3D Data: live textured Vulkan 100 vertices/52 triangles. Owner says it appears correct but is uncertain. Opened concreteblock1.model the same way: live textured 24 vertices/12 triangles, coherent box with concrete and tie-hole details. Left block visible, bin in adjacent document. No code or orientation change applied.
- [x] Independent read-only scan of shared/construction finds three single-buffer/full-draw tuple candidates (the block and two already tested bins); production scanner verified block admission. Private uv_compare.py inspects original small-bin atlas/half UVs: U 0..1.0009765625, V 0..1, outer top vertices V0; atlas generated in private workspace only. Shader uses 1-V; sourceUvTopLeft changes upload orientation. Blur currently omits this flag. This is an orientation hypothesis, not a confirmed UV correction: full material transform/game reference still pending.
- [ ] Codex next: stronger asymmetric visual reference / exact source UV convention and material transforms before changing orientation. Uniform concrete texture cannot independently prove U/V direction. No new suite/build needed (research/live comparison only); game assets unchanged, no release/publication.

## Blur Audi UV follow-up — 2026-10-28

- [x] Owner reports unusual UV appearance on prop preview and asks for Audi comparison. Read-only independent probe of audi_s3_stock.model found 12 complete attribute declaration candidates, with strides 12/16/24/28/32/36/40; all declared vertex count times stride match blob length. This extends the earlier strict complete-payload walker (one stride12 record), not 12 proven renderable meshes.
- [x] UV semantic 5 includes type9/channel0, type8/channel1 and additional channels2/3/4, unlike admitted single-draw props' type8 half2 at offset16. Type9 half4 is a hypothesis, not validated decoding. Current successor/padding gate uniquely resolves only two payloads (neither supplies admitted type8/9 UVs); others remain unresolved. No guessed padding or fixed stride20 interpretation applied to Audi.
- [x] Exact image-object naming recovers seven printable resources: paintdent, tyrewall, tyretred, tyretred_bump_nrm, breakdisc, cali_02, smashedglass_nrm. An additional raw name candidate contains control bytes and is rejected by production ASCII validation. These resources do not provide a single vehicle-wide diffuse binding.
- [ ] Audi cannot yet be used as a native rendered UV validation fixture. Codex next: resolve typed vertex blob boundaries/references and per-draw channels/material bindings, then checkerboard/source-texture comparison on car. Prop UV visual concern remains unverified; no blanket flip or material claim. Private probes audi_uv_followup.py/audi_uv_values.py and JSON under workspace blur-research; source game files unchanged. No app code/build/release changed in this follow-up; prior 126-pass result is unchanged, not a new test run.

## Blur material preview and split images — 2026-10-28 (verified limited scope)

- [x] Owner requested attempting materials and map rendering. Added exact serialized image-name association (name record + RA image object boundary), Float16 UV0 for admitted stride20 single draws, and an explicitly approximate unique diffuse-named resource preview. This does NOT prove original GPU material binding: sourceBindings textureAssignmentVerified=false, gameShaderApplied=false; preview is unlit. Multiple diffuse names, invalid UVs or unsupported meshes remain unassigned.
- [x] Added bounded raw ARCH split-image extraction using exact base/full-chain byte sizes, segment bounds, zero padding and known RA successor versions; only first 128 image recipes listed, source-offset identities preserved. No unsafe global key map or arbitrary nearest-image binding.
- [x] Focused suite: 8 passed/0 failed/no skips (build/windows-local/blur-material-preview-tests.txt). Full window suite: 126 passed/0 failed/86 optional skips (build/windows-local/blur-window-full.txt). Development executable rebuilt successfully (build/blur-app-build.log). Live normal Scan for 3D Data rendered dam_dirtyconstructionequipment_b.model with visibly textured construction skip, 385 vertices/302 triangles in NVIDIA RTX 4060 Ti Vulkan. App left open for owner review. This confirms the named-albedo preview, not game-exact material fidelity. Source game files unchanged; no release/publication.
- [ ] Map assembly remains unresolved: typed stream catalogs do not yet prove per-draw resource/instance/material links. Do not present individual props or collision positions as a reconstructed textured showroom/track. Native decoder work will continue from docs/research/blur/ARCH-FINDINGS-2026-10-27.md after the current validation slice.

## Blur ARCH analysis continuation — 2026-10-27

- [x] Added docs/research/blur/ARCH-FINDINGS-2026-10-27.md and roadmap link. Read-only private probes validate lamposta's nine ranges as exact partition of 5,292 indices, with per-range minimumVertex/count matching observed indices and explicit stream byte offsets within the 3,055-vertex pool. Resetting strips per draw gives 2,063 triangles vs aggregate 2,079 (16 invalid boundary faces). Early private baseVertex label corrected in handoff; no production topology change yet.
- [x] Bounded structural candidates: Audi 100 packets x 4 ranges; showroom 226 packets x 1. Not rendered mesh counts; bindings/LOD alternatives unresolved. Typed declaration walker finds five bounded showroom payloads with stride20/24 and coherent half-UV evidence; not complete geometry coverage.
- [x] Corrected external-only texture hypothesis: size-28/sentinel headers often have separate resident RA blobs. 854 base images independently decode (717 DXT1/137 DXT5; 213 full-chain and 641 base-only stored), 17 signatures fail conservative gate. Selected private previews coherent; actual production split-image support still pending.
- [x] Identity caution: 209 candidate key values /258 key-format-dimension combinations; 42 combinations have conflicting decoded RGBA. ARCH-segment scoping alone insufficient. Preserve source resource identities, never choose a global last matching hash or nearest image. Raw assets/previews/full probes remain private workspace blur-research; no app code/build/release changed.
- [ ] Next native work: tested split-image reader, counted draw arrays with minimumVertex separate from stream offsets, object/buffer references and material namespace resolution; then showroom and one track. No full scene render/material fidelity claimed.

## Blur live scenery-object validation — 2026-10-27

- [x] User requested rendering a track model. Read-only bounded probe examined 117 eligible scenery object files and stopped after three full-single-draw candidates: shared/construction/concreteblock1.model (24 vertices/12 triangles), trashcontainer_small.model (100/52), dam_dirtyconstructionequipment_b.model (385/302). Counts are local geometry candidates, not a whole track or placement proof.
- [x] Used Windows Computer Use to open and scan original trashcontainer_small.model and dam_dirtyconstructionequipment_b.model in the rebuilt 9XD. Both automatically loaded through normal Scan for 3D Data into the live NVIDIA RTX 4060 Ti Vulkan viewport, with the exact 100/52 and 385/302 status counts and visibly coherent closed bin/open construction skip geometry. App left on the latter for owner inspection; former remains in another document tab.
- [x] jpn_mountaintrack/lamposts/lamposta.model correctly reports unsupported layout/zero mesh results: it shares a typed buffer family but lacks the admitted full-single-draw tuple. No weaker admission rule or guessed connectivity added. User resumed/authorized window control after minimizing; no source game files modified.
- [ ] These previews have no linked textures/UVs and are shared scenery props, not a reconstructed race circuit. Full levelrender/levelstream instance assembly, multi-draw materials and source placements remain pending. No application code/build changes in this visual check; no publication.

## Blur native first slice — 2026-10-27 (verified limited scope)

- [x] Added src/blur_model.h and scan/decode dispatch for a strictly bounded PC CPModel single-static-draw ARCH layout (typed stride-20 position buffer, UInt16 indices, explicit full-buffer triangle-strip draw). Does not admit multi-buffer Audi or skinned character geometry. Source offsets retained; normals generated, source basis preserved, UV/material assignment pending.
- [x] Added conservative inline DXT1/3/5 image candidates using existing 2D codecs and exact mip-chain bounds. Streamed texture headers are excluded. Images are not guessed onto meshes.
- [x] Independent private probe: envsphere/specsphere each 401 vertices, 1,090 indices, 760 nondegenerate triangles. All 760 sphere triangles outward by geometric cross-product/centroid test. First actual native corpus check passed; synthetic zero-prefixed blob ambiguity found and tightened via typed successor.
- [x] Final focused Blur suite: 6 passed/0 failed/no skips, including actual GLB/FBX and 14 image decodes; scanner-to-scene/dedup uses Qt offscreen, not live Vulkan proof. Full suite: 124 passed/0 failed/86 optional skipped (build/windows-local/blur-window-full.txt). Development executable rebuilt successfully (build/blur-app-build.log). No release/publication.
- [x] Full regression exposed prior About icon initialization outside normal startup; src/about_dialog.h now falls back to platform icon when application icon is null, preserving Claude entrance animation/layout changes. Forced fresh translation-unit rebuild before final full suite.
- [ ] Next: per-draw ARCH buffer/material identities for multi-buffer Audi; standalone/streamed texture linkage, scenes and native PAK support. Do not claim full Blur support from the sphere tests.

## Blur binary reconnaissance and roadmap — 2026-10-27

- [x] Owner requested exploration and durable format/implementation roadmap, not immediate reader integration. Added docs/research/blur/ROADMAP.md. Read three study fixtures plus showroom companions and full-installation file headers, all read-only: 1,544 models (1,495 CPModel/49 raw Model), 301 Texture files, 139 DDS, 19 scene quartets, 7 PAKs, skeletons/cycles and related resources.
- [x] Verified bounded CPModel child framing to EOF on both samples; preserved hashes. Study levelstream matches showroom exactly, not a full race map. Conservative inline texture probes validate 7 records per model and 15 in showroom levelrender; first character DXT1 atlas decodes coherently. Stream texture headers carry size-28/sentinel references, not inline pixels. These are not material-coverage counts.
- [x] Read local blur.bms as reference, never executed. Independent PAK probe recovers seven name tables, validates root.pak 3,601 directory entries and boot.pak 931; one segmented model payload decompresses to exact fixture SHA256. All-PAK payload validation and opposite-endian variants pending. Xentax attachment lists only legacy exe, not used.
- [ ] Next native owner: ARCH object/reference serialization -> vertex declarations/index draws -> albedo bindings -> Audi/wheels -> showroom companion assembly -> one track. No geometry, face counts, material percentage, export or live support claimed; no app code/build/publication. Raw game images/reports/probes stay in private workspace blur-research; only derived roadmap copied into repo.

## Theme effects and C77 themes — 2026-10-28 (Claude)

- [x] Theme options for every theme and style (theme editor › Effects; duplicate a theme to edit): `options.roundness` (0–24 px; all stylesheet radii scale from the 6 px button radius), `options.neon` (neon panel lines in colour `style.neon`: nine-patch glow on cards, group boxes, tab panes, viewport bars, menus and popups; neon lines under panel titles, on the rail, headers, selected tabs, deck bars, status bar, splitters and focused fields; text is never tinted) and `options.glow` (style, none, soft, strong; text glows in its own colour). `validateTheme` accepts them.
- [x] New colour `background.gradient` (Backgrounds): when set, windows, docks and dialogs fade from the window colour into it.
- [x] Included themes C77 Day (electric yellow, cyan highlights, black text/details, cyan neon, square corners), C77 Night 1 (dark red to black, vivid red highlights, red/aquamarine text, red neon) and C77 Night 2 (dark yellow to black, yellow highlights, yellow/cyan text, yellow neon). The Theme menu now lists included themes from `includedThemeOrder()`.
- [x] Text on primary buttons now uses whichever of white or the text colour reads better on light themes too (needed for cyan on yellow).
- [ ] Not built or verified here (computer access ended); the owner tests visually. `main.cpp` re-saved so ninja rebuilds the theme code.

## 3D viewport: Transform/Material card, Camera menu, selection tool — 2026-10-28 (Claude)

- [x] Transform / Material / Presets (`meshInspectorTabs`) moved into a foldable card in the viewport's top-right corner (`VulkanPanel::hostInspector`, from `main.cpp`; starts folded; native sibling of the Vulkan surface, opaque). A standalone `createMeshCollection()` (tests) still contains the tabs.
- [x] Camera menu ("Camera · 45°"; `viewportCameraButton`/`viewportCameraPanel` unchanged): FOV, clipping, and eye position X/Y/Z, yaw, pitch and pivot distance in model units (`cameraPose()/setCameraPose()`).
- [x] Corner text "Perspective / Grid …" removed from the main viewport (tile views keep their view name).
- [x] Tools: none active by default; clicking the active tool turns it off. Select tool (`setSelectionTool`): clicks add/remove meshes. Without it a click still selects one mesh (Ctrl/Shift add), so picking tests hold.
- [x] Verified live except multi-selection with a loaded model. Not run: window/reader suites.

## Hex Viewer panel rework: Structure, Find, Magic Bytes, Checksum, Compare Data — 2026-10-28 (Claude)

- [x] `src/adaptive_layout.h`: panels stack below Settings › Interface mode › "Side by side from" (`interface/sideBySide`, 640 px) and go side by side above it; foldable sections; compact tool rows.
- [x] Formatting Rules renamed Structure (objectName `formatRulesDock` unchanged): Rules and Appearance sections; + and double-click open the rule editor with Appearance. Find, Magic Bytes, Checksum and Compare Data decluttered (less text, compact tools, folded options). Test lookups kept.
- [x] Command Deck module tabs show names only. Verified live narrow and wide. Not run: window/reader suites.

## Interface modes: styles × layouts × themes, Command Deck, Floating HUD, adaptive panels — 2026-10-28 (Claude)

- [x] Interface mode is now three independent choices (Settings › Appearance › Interface mode): **Style** (`interface/style`, migrates `interface/mode`: Classic, Hackerman CRT, Aqua 2003, Aero Glass), **Panel layout** (`interface/layout`: Classic, Inspector stack, Command Deck, Floating HUD) and the **theme** (always the user's: Themes page or Theme menu; styles never force a palette). The Theme menu also lists the user's own themes.
- [x] Styles derive all gradients from the palette plus four new theme colours (theme editor group "Interface Styles": `style.bar`, `style.glass`, `style.panelTitle`, `style.progress`; palette-derived fallbacks; Aqua/Aero themes set them). Aqua on dark themes: readable table headers; compact and in-table buttons keep a tight gel.
- [x] Per-layout dock arrangements (ROADMAP I5): Classic stays in `moduleLayouts/v1/<module>`, others in `interface/layouts/<layout>/<module>`; switching saves and restores (`WorkspaceWindow::applyPanelLayout`). A layout is only laid out automatically when it has no saved arrangement for a module, or on Reset Panel Layout; otherwise the user's arrangement (tab groups, floating panels) is kept.
- [x] Inspector stack / Command Deck (`InterfaceModes::DockStack`): side and bottom panels in one right column; click a title to fold (content to zero height, so width is kept); tab groups never fold; a panel pulled out floats normally at a usable size, one dropped back joins the stack. Work surfaces (3D viewport, UV canvas, script editor/console, reconstruction canvases, AI chat) move to the left area so the column sits beside them. Command Deck adds a header (module tabs with counts, command search, open-file chip, settings), item-count summaries in titles and an activity strip (finished progress, status messages, threads).
- [x] Floating HUD (`InterfaceModes::HudCards`, `HudModeDock`): panels become translucent floating cards with a slim title bar, snap to the work area and each other, follow the window; only the raised tab of each Classic tab group starts as a card; a card docked back loses its card look; module/search/settings dock at the bottom replaces the rail.
- [x] Adaptive panels (`src/adaptive_layout.h`: `flip()` and `section()`): Magic Bytes stacks library over matches below 640 px with foldable sections ("Outline selected rows" label shortened, full text in the tooltip; test uses startsWith, unaffected); Script Library Manager splitter turns vertical below 620 px, its actions stack below 330 px.
- [x] Verified live: Command Deck (hex and 3D), HUD with Aero, pulling a folded panel out, docking a floating panel into a tab group, Aqua on a dark theme, Magic Bytes narrow. Not run: window/reader suites. Not yet verified live: HUD card flow with real heights (code in place), Library Manager narrow.
- [ ] Next: owner arranges the inspectors per layout; then optimise from that structure (summaries per panel, more adaptive panels, HUD minimap and radial menu).

## Aqua 2003 and Aero Glass interface modes, slice 1 — 2026-10-28 (Claude)

- [x] Both modes are now `experimental` in `InterfaceModes::all()` with included themes `aqua` (light: #e9eef5 window, #2c7ae6 primary) and `aero` (dark: #0e171f window, #2a8ad8 primary, #4fc3ff focus). Theme menu › Included lists them after Hackerman.
- [x] Options (Settings › Appearance › Interface mode, one group per mode, keys `interface/<mode>/<key>`, all default on):
  - Aqua: `gel` (gel buttons, blue gel primary buttons, segments, tabs), `pinstripes` (4×4 palette texture on the window background only), `taskPanes` (blue gradient dock titles and module rail, beige #ece9d8 menu and status bars). Block-chunk progress bars always.
  - Aero: `glassBars` (black glass gradient on menu bar, toolbars, status bar, dock titles, viewport toolbar and headers; blue glass menu selection; dark teal rail), `glassButtons` (dark glass buttons with blue hover, blue glass primaries, green shiny progress), `glow` (shared `drawItemText` glow).
- [x] All of it is CSS in `theme.h` (4-stop `qlineargradient` helper) plus the palette brush; no layout changes, Classic untouched. Verified live: switching Classic → Aqua → Aero from Settings, menu bar, dock titles, rail, gel/glass buttons.
- [ ] Next (ROADMAP): slice 2 = I7 custom dock title bars (real gel/glass bars with highlight line), Aqua traffic-light dock buttons, Aero frameless glass frame (DWM on Windows); slice 3 = Aero glass over the 3D view via I6. Not run: window/reader suites. `main.cpp` needed a touch to rebuild (stale ninja dep again).

## Interface modes framework + Hackerman CRT slice 1 — 2026-10-27 (Claude)

- [x] Interface modes are whole presentations: theme + shell layout + chrome (menus, panel titles, corners) + effects. Registry and chrome in `src/interface_modes.h`; roadmap, constraints and per-mode plan in `docs/ui-modes/ROADMAP.md` (concept canvas "9XD UI Concepts", nine boards).
- [x] Settings › Appearance › Interface mode (`settingsInterfaceMode`): **Classic (0.3.7) is the default and changes nothing**. Hackerman CRT is experimental; the seven other concepts are listed as planned (disabled). Keys: `interface/mode`, `interface/hackerman-crt/{statusLine,boxTitles,terminalMenus,glow,scanlines}`. The page is appended last in the settings stack so the fixed page indices are unchanged.
- [x] `loadWorkbenchPreferences`: the mode's theme overrides the user theme only while the mode is active (the user's `theme` setting is untouched); then `InterfaceModes::Chrome::apply()` runs.
- [x] Hackerman CRT slice 1, verified live:
  - terminal status line `[9XD] 1:hex … 6:tools` plus document info, `ctrl+k` and `settings`, replacing the module rail; Alt+1…6 work only in that mode (`WorkspaceWindow::applyInterfaceMode`, `refreshStatusLine`);
  - box-drawn uppercase dock titles (`┤ INSPECTOR ├`); originals are kept and restored, and later title changes are decorated again;
  - terminal menus (inverted selection, square) and square corners (not combo or spin boxes: a border rule drops their frame);
  - scanline texture as palette brushes that keep their colour;
  - text glow in `WorkbenchStyle::drawItemText`.
- [x] Constraint recorded: the native Vulkan surface makes ancestors (and their siblings) native, so a full-window translucent overlay cannot be used. Effects over the 3D view must go through a Vulkan overlay pass (ROADMAP I6).
- [ ] Next: I10 round-trip test (Classic → CRT → Classic restores the theme, titles, rail and shortcuts); CRT slice 2 (Vulkan scanlines/vignette, box frames on inspector cards, ASCII progress); then Command Deck or Aqua 2003 per the owner's choice. Not run: window/reader suites.

## About animation, scan sweep, quiet tooltips, 3D controls help, Hackerman theme — 2026-10-27 (Claude)

- [x] About (`about_dialog.h`): the window is centred on the screen of the main window (not on the window). Entrance: fade in, rise 28 px with a slight overshoot and the artwork settling from 92 % to full size; then one light glint crosses the logo, clipped to its alpha. Closing fades out (170 ms); the close button now calls reject. The close × is plain text with a drop shadow (no circle). The existing about test (click close → deleted) still applies.
- [x] Scan sweep (`scan_sweep.h`): a decorative "flatbed scanner" beam in the theme's focus colour, with glow, fading trail and faint CRT lines. It runs while an AssetScanPanel scan or resource opening is active. It covers the Hex Viewer's populated byte and text columns only (no offset column, nothing below the last row) when the Hex Viewer is showing, otherwise the panel's results list. `HexView::setScanAnimation` nests. Verified live on Tokyo_sunny.pak (purple in Nines, green in Hackerman).
- [x] Tooltips (`tooltip_policy.h`): minimal by default. Only icon-only buttons, tab bars and widgets with property `keepTooltip` show them; item views, labels and fields no longer pop tooltips. Settings > General > Tooltips "Show detailed tooltips" (`interface/detailedTooltips`, default off) restores all. Tooltip texts are unchanged.
- [x] 3D Controls help: "▸ Controls" at the bottom left of each viewport, drawn in the Vulkan overlay (transparent background); a click expands three condensed lines (mouse, selection/handles, configured movement keys). `appendCornerTitle` gained optional pixelSize/bold.
- [x] Hackerman included theme: black, phosphor green #39ff6a text/lines, bright green accent with dark text, and `options.monospace` → terminal font (Cascadia Mono / Consolas / Lucida Console / system fixed) for the whole UI. Other themes restore the original font. `validateTheme` accepts `monospace`; the theme editor keeps unknown option keys when toggling its two checkboxes.
- [x] Build hygiene: `magic_panel.cpp` and `format_panel.cpp` include `hex_view.h` but were not recompiled by ninja after HexView changed (this session added a virtual `changeEvent` and the `scanSweep_` member). Fixed by touching both; magic_panel.obj changed size. **Rule: after changing a class layout in hex_view.h, touch magic_panel.cpp and format_panel.cpp** (or rebuild clean).
- [ ] Not run: window/reader suites. The Lighting menu fields still need the popup treatment (see previous entry).

## Viewport quick controls, grid settings, dock title hover — 2026-10-27 (Claude)

- [x] Panel (dock) title buttons: float/close are invisible until the pointer is over that panel's title bar (`src/dock_title_hover.h`, installed from `applyWorkbenchTheme`). Implemented with an opacity effect, so the title layout doesn't shift and the buttons stay clickable. A short poll hides them when the pointer leaves.
- [x] Quick camera control: a toolbar button next to Perspective shows the current FOV (`45°`, plus `· clip` when clipping is manual). One click opens a popup (`viewportCameraPanel`) with FOV slider/field/45° reset, Automatic clipping and Near/Far; changes apply live. It replaces the Camera / Clipping dialog; View > Camera / Clipping opens the same popup. Test `cameraInspectorControls` updated to the popup.
- [x] Grid: `ReferenceGridSettings` (spacing, major every N, half size, numbers) in `reference_grid.h`, default 1 / 5 / ±10 (identical output, 42 lines). At most 400 lines per side; number labels thinned to about 11 per axis. Display > Grid: Show grid, Show numbers, "Spacing and size..." popup (`viewportGridPanel`) with Reset. `VulkanPanel::setReferenceGrid` applies to tile views and new viewports; the corner title reflects the values. Test extended (0.5 / 4 / ±3 → 26 lines, labels −2/0/2, invalid spacing falls back to 1).
- [x] Text fields inside a QMenu don't receive typed keys here and Enter triggered the active menu item (it switched the grid off). Hence the popup panels; `MenuSpinEnterGuard` keeps Enter from triggering menu items for the Lighting menu fields. **Open:** the Lighting menu's Azimuth/Elevation/Ambient/Intensity fields likely can't be typed into either; move them to a popup the same way.
- [x] Primary-button icons follow the readable colour on the accent (`workbenchOnPrimary`), so they are dark on Zuz's olive.
- [x] Verified live (Zuz): title buttons appear/disappear on hover; FOV popup slider and typed value (60) apply live; grid 0.5 / ±5 with labels, numbers off, grid off, reset. Build checked: main.cpp.obj and vulkan_panel.cpp.obj recompiled.
- [ ] Not run: window/reader suites (changed test: `cameraInspectorControls`).

## UI review, Zuz/Nines themes, inspector polish — 2026-10-27 (Claude)

- [x] Included themes (`src/included_themes.h`): **Zuz** (black, cream text/lines, olive accent from the mascot) and **Nines** (black, silver text/lines, midnight-purple accent from the logo). Listed under Windows > Theme (section "Included") and in Settings > Appearance > Themes as "· Included" (read-only; Duplicate makes an editable copy). Keys `zuz` / `nines`.
- [x] `theme.h`: text on accent-filled controls picks white or the window colour by contrast (olive needs dark text); `WorkbenchStyle` (QProxyStyle over Fusion) draws check boxes and radio buttons from theme roles. Fusion derived their outline from a darkened Window colour, so they were invisible on near-black themes (Default included).
- [x] Hex Viewer and Data Inspector refresh cached cell colours on PaletteChange (the selected byte stayed blue after a theme switch). Data Inspector: byte order / Hex integers / Copy Value moved above the values in one row; "N/A" values dimmed; Copy disabled until a row is selected. ObjectNames and value texts unchanged.
- [x] Camera / Clipping inspector rebuilt as two cards (Lens, Clipping) with a 45° reset, a Close button, a note when the view is orthographic and a visible error line for Near >= Far. `setViewOrientation` now emits `cameraChanged`. ObjectNames used by `cameraInspectorControls` unchanged. View menu: separator before Camera / Layout.
- [x] Mesh inspector: "Materials" tab renamed "Presets" (was confusable with "Material"); full mesh name in the row tooltip; Source maps button explains when it is disabled; Source maps dialog gives the map list more space, bolds the previewed map, double-click previews, default status explains the flow.
- [x] Verified live on the dev build: both themes in Hex, 3D (PGR3 288 GTO, 195 parts) and Settings; checkbox rendering; hex cursor colour after switching; inspector layout; camera inspector in Front view. Restored CRLF endings on the CRLF files edited.
- [ ] Not run: window/reader suites. Relevant: `tabularInspector`, `cameraInspectorControls`, theme editor test (Midnight Teal), stop/batch tests.
- [ ] Review findings left for later: 2D Settings dock is one long form (group into Source / Layout / Palette / Output cards); in Automatic clipping the disabled Near/Far fields show the stored manual values, not the effective range; Mesh Collection middle-elides names so PGR rows look identical (tooltip now shows the full name; a dedicated short-name column would fix it properly).

## Binary progress reporting review — 2026-10-27

- [x] Owner visually confirms Windows runtime cat icon looks correct. Reviewed native-scanning, linked-scene-materials, priority-racing-formats, Genki, WM, RenderWare and format-roadmap records for progress table. No new decoder tests or measurements in this review.
- [x] Reporting rule: material percentages mean supported base-image assignment on a specified fixture, not full shader fidelity or entire-game completeness. Keep draw/part denominators separate from triangle-weighted coverage; unknown percentages remain unmeasured. Newer specialist supersession notes override historical inventory rows (Initial D codec, Super Trucks banks, NFSU2 TPK/HUFF, WM PS3 and ITC).
- [ ] Next implementation candidates: PGR3/4 layered materials/UV-atlas bindings; Super Trucks dry-track UV streams; broader Genki external/alias contexts; PSP GIM and MW-specific TPK linkage. No runtime code changed in this status review.

## Windows runtime cat icon only — 2026-10-27

- [x] Owner clarified scope: retain existing logo and executable/file icon exactly, use selected P cat only while running on Windows. Added assets/windows-runtime-cat.ico (transparent square canvas, aspect preserved, 16/24/32/48/64/128/256 sizes), Windows-only Qt resource, Q_OS_WIN application icon branch in src/main.cpp. Other platforms retain original logo icon. About dialog inherits application icon instead of overriding it; its artwork remains unchanged.
- [x] Verified source alpha 0/255, ICO representations and SHA256 preservation of assets/logo-sticker.png, assets/app.ico and assets/about-splash.png. No creative redraw or background removal. Logo integration trials rejected by owner; not installed.
- [x] windows-local binary_workbench build completed successfully (resource compilation and executable link). Visual Windows title/taskbar/DPI verification remains pending. No release/publication. Do not replace executable RC icon: user explicitly wants existing static icon preserved.

## Mascot P minimal polish, O discarded — 2026-10-27

- [x] Owner rejected O for broken proportions and explicitly requested deletion and return to N. Deleted O workspace PNG/prompt and generated PNG; prior displayed chat preview cannot be removed via file deletion. O is not an approved direction.
- [x] Used N alone for a very restrained built-in image_gen eye-glint polish, with instructions to preserve geometry/asymmetry, eye size, muzzle, ears, whiskers and brows. P adds small secondary catchlights and fresher olive, with proportions visually close to N. Saved private workspace branding-concepts/cat-face-p-subtle-life.png and exact -prompt.txt; displayed for review.
- [ ] Owner review; generative preservation not pixel-identical, alpha-edge cleanup and final small-size checks pending. Runtime logo/icon and splash untouched. No build/publication.

## Mascot N round pupils and minimal palette — 2026-10-27

- [x] Owner clarifies affectionate expression uses round black pupils, not vertical slits/lens shapes. Requests black fur, white chin/whiskers/brows/ear hairs, olive eyes and dark-gray nose. Built-in image_gen edited M with round pupils and near-uniform black fur, preserving feature geometry closely. Saved private workspace branding-concepts/cat-face-n-round-pupils.png and exact -prompt.txt; visually inspected and shown for review.
- [ ] Owner approval pending; strict four-color count not validated and residual subtle tonal variation/edge debris still visible. Final palette/alpha cleanup and small-size packaging validation remain before installation. Active app branding and splash unchanged; no build/publication.

## Mascot M logo colors and eyes — 2026-10-27

- [x] Owner approves L direction and explicitly locks muzzle, ear shape/proportions, brows, whiskers and nose. Requested simpler logo colors and cuter less realistic eyes. Built-in image_gen instructed to preserve locked geometry and edit only fills/eye interiors; M retains silhouette closely, simplifies dark fills and adds graphic olive lower iris crescent/larger catchlights. Saved private workspace branding-concepts/cat-face-m-flat-eyes.png and exact -prompt.txt, visually reviewed and displayed.
- [ ] Generative edit is not pixel-identical preservation; owner comparison pending. Some residual tonal variation and external alpha flecks remain; strict palette count/production contour and small-size validation pending. Active app branding and splash untouched; no build/publication. Next: owner selection then production asset cleanup/packaging.

## Mascot L frontal open eyes — 2026-10-27

- [x] Owner likes K direction and requests frontal gaze at same slight elevated viewpoint, fully visible open eyes, more cartoon simplicity with real cat identity. Built-in image_gen edited K using three owner photos for olive eye/marking references; open olive eyes/slit pupils, frontal nose, defined forehead and broad muzzle retained. Private workspace branding-concepts/cat-face-l-open-eyes.png and exact -prompt.txt saved; displayed for review.
- [ ] Still has more fur/shading detail than requested and substantial white alpha debris around whiskers; not production-ready. Strict <=10 color palette not validated. Next: owner art/proportion review, then simplify and clean production edges before any icon packaging. Active app branding/splash unchanged; no build/publication.

## Mascot K reference viewpoint reset — 2026-10-27

- [x] Owner rejected reuse of frontal perspective; requests rebuilding solely from latest annotated photo with approximately 10 colors and minimal shading. Built-in image_gen restarted from only that photo, followed by simplification pass preserving asymmetric ears, lowered viewpoint, narrow eyes and large muzzle. Saved private workspace branding-concepts/cat-face-k-reference-angle.png and exact two-pass -prompts.txt; shown for review.
- [ ] Requested seven-color palette in simplification prompt, but actual strict color count not validated; generated shading still varies slightly. White edge/whisker alpha debris remains and needs cleanup before production. User review of viewpoint/proportions next. Runtime icon/logo and splash unchanged; no build/publication.

## Mascot J wide muzzle — 2026-10-27

- [x] Owner supplied annotated cat photo clarifying a substantially larger black upper muzzle. Built-in image_gen edited I using sketch as anatomy reference: broader black lip lobes and nose, white chin reduced to a narrow band, existing eyes/forehead/pointed ears retained. Private workspace branding-concepts/cat-face-j-wide-muzzle.png and exact -prompt.txt saved; visually inspected and shown for review.
- [ ] User proportion review; final contour/alpha and small-size validation pending. Concept only, not installed; runtime logo/icon and splash untouched. No build/publication.

## Mascot I defined expression — 2026-10-27

- [x] Owner requests defined forehead/cowl-like natural face planes, larger black upper-lip lobes, stronger white brows/whiskers, pointed ears with spiky inner hairs. Generated I from H and owner cat photo with built-in image_gen, preserving open olive eyes/adult female identity. White fur framing mouth is intended as fang-like marking, not actual teeth. Saved private workspace branding-concepts/cat-face-i-defined-mask.png and exact -prompt.txt; shown for review.
- [ ] Owner selection/refinement; output has more angular shading and ear detail, and fine roughness on outside keyline remains. Needs final contour/small-size validation if selected. Not approved/installed; runtime logo/icon and splash untouched, no build/publication.

## Mascot H adult proportions — 2026-10-27

- [x] Owner finds G too infantile; requests more mature relative eye/muzzle/ear/chin proportions and a slightly more circular face. Generated H with built-in image_gen from G: smaller eyes/pupils and inner-ear patches, shorter brow strokes, more defined nose/muzzle and rounded jaw. Private workspace branding-concepts/cat-face-h-adult-proportions.png and exact -prompt.txt saved; displayed for review, not approved/installed.
- [ ] Ear size and white chin remain fairly prominent; outer-edge roughness still needs production cleanup. Next owner: user proportion/expression review, then further refinement and platform small-size validation. Runtime logo/icon and splash unchanged; no build/publication.

## Mascot G cartoon expression study — 2026-10-27

- [x] Owner supplied Maxwell cartoon references to clarify cute/naughty rather than realistic angry expression. Built-in image_gen used F plus three cartoon references: larger round pupils with olive lower iris crescent, shorter brows, smaller muzzle, simpler shading; second pass smoothed chin. Saved private workspace branding-concepts/cat-face-g-cartoon-expression.png with exact two-pass -prompts.txt. Shown for review; not approved/installed.
- [ ] Some rough white outer-edge alpha remains after cleanup; production contour cleanup and small-size validation still required if owner selects this direction. Active logo/icon and splash unchanged. Next owner: user expression selection, then Codex refinement/integration. No build or publication.

## Cat expression refinement F — 2026-10-27

- [x] Reviewed five additional owner photos; used three clear expression/marking references with E as primary edit target in built-in image_gen. Kept female cartoon identity, white brow strokes, black upper :3 muzzle and separate purple exterior edge; slightly more half-lidded expression and smaller muzzle. Output still has a broad white chin and some pointed cheek tufts; further simplification remains an owner-review decision.
- [x] Saved non-destructively in private workspace branding-concepts/cat-face-f-photo-refined.png and exact -prompt.txt; inspected output and confirmed 1254x1254 RGBA with alpha 0/255. Displayed for review, not approved or installed. Runtime assets and splash unchanged; no code/build/publication.
- [ ] Owner selection/refinement, then integration into original 9XD and actual small-size/DPI validation. Photo references remain private; no photos copied into repo.

## Female cartoon mascot refinement E — 2026-10-27

- [x] Owner rejected D as too masculine/realistic and prefers the original logo's cat, with only slight shape/polish corrections: cute but mildly mean female cartoon, not a stern male/esports redesign. Treat this as current direction; previous D is not approved.
- [x] Inspected both supplied photos (JPG and HEIC). HEIC decoded locally to a private PNG solely for viewing/reference using a workspace-only pillow-heif reader; original photos unchanged, no application dependency added. Built-in image_gen used original logo as primary identity and both photos as marking/proportion references, followed by one targeted edge/lower-jaw cleanup. Preserved olive eyes, white brow swooshes, small black nose/:3 upper muzzle; softer round face, modest black/white ears, neutral dark fur with purple confined to outer accent. No realistic frown wrinkles or beard. Output inspected; PNG alpha confirmed0/255.
- [x] Versioned concept saved as workspace branding-concepts/cat-face-e-soft-cartoon.png, exact two-pass prompts beside it in cat-face-e-soft-cartoon-prompts.txt. Displayed inline for owner review; not approved/installed. Active app icon/logo and splash untouched; no code/build/publication.
- [ ] Owner refinement/selection; if selected, integrate gently into original9XD head/bowl and package/test true small-size platform icons. Edge quality and16px readability still need production validation; do not report this concept as a runtime scaling fix.


## Cat icon variants and sterner expression — 2026-10-27

- [x] Owner rejected purple mixed into black fur. Generated three distinct transparent concepts with built-in image_gen: A neutral black/silver satin, B simplified flat black/ivory, C neutral-black fur with purple confined to an exterior outline separated by white. Preserved olive eyes, white brows and black nose/upper muzzle. Shown inline; small-size legibility still untested.
- [x] Owner then requested a less friendly, meaner/watchful expression, supplying Maxwell cat photo as reference. Edited C into D: narrowed half-lidded eyes, lowered/knitted white brows, smaller unsmiling black muzzle and compact white lower-jaw bib rather than broad smile crescent. Purple remains outside the black fur. Reference photo used only for expression/anatomy, not as an instruction or background. Output alpha checked RGBA with0/255 coverage; shown inline for review. D has more fur detail around chin than earlier smooth variant, so simplification remains an option if selected for16px use.
- [x] All four PNGs and exact built-in prompts saved non-destructively in workspace branding-concepts/: cat-face-a-silver, cat-face-b-flat, cat-face-c-purple-outline, cat-face-d-stern (PNG plus -prompt.txt). Active application icon/logotype and splash untouched. No build, commit, push or publication.
- [ ] Owner artwork selection/refinement, then consistent square platform icon packaging and actual Windows/macOS DPI/small-size validation. New concepts do not alone fix icon resource/scaling differences.


## Branding refinement v2 — 2026-10-27

- [x] Owner feedback: preserve olive eyes, nose, black upper :3 muzzle/lips and white brows; smooth strange cuts at lower muzzle, rounder head fitting the9, black ears with white interior. Built-in image_gen edits generated both a transparent cat-face-only v2 and matching full9XD logo v2. Smooth white chin crescent and rounder cheek/face contour replace the previous chopped lower edges; black/white ears retain subtle purple reflected highlights. XD chrome lettering and purple9 tail retained in the full-logo concept. Splash artwork not edited.
- [x] Versioned local assets and exact prompts saved in workspace branding-concepts/: cat-face-v2.png, logo-9xd-v2.png and matching -prompt.txt files. Both outputs inspected and alpha validated as RGBA with0/255 coverage. Two concepts displayed for owner review. The approved active logo-sticker.png, ICO/PNG icon size set and Qt resources remain unchanged; no app rebuild or publication in this design pass.
- [ ] Next: settle artwork, then use face-only asset for small square icons and full9XD for larger branding; generate aspect-preserving platform representations and test small-size/DPI readability and Windows/macOS presentation. These concepts alone do not resolve the runtime icon-source discrepancy recorded below.


## Small-size app icon audit/concept — 2026-10-27

- [x] Owner reports intermittent blur/vertical stretching in Windows vs macOS. Read-only audit: assets/app.ico contains16/24/32/48/64/128/256 square representations, preserving wide sticker with padding. At16px, alpha bounds occupy16×10 pixels. Meanwhile main.cpp/about_dialog.h set QIcon from nonsquare logo-sticker.png; railLogo uses52×34 with KeepAspectRatio and smooth scaling. Different OS/Qt resources and very low useful pixel area explain inconsistent/detail-poor presentation. A specific native Windows vertical-stretch path is NOT yet reproduced/proven; no macOS runtime test performed.
- [x] Built-in image_gen produced transparent cat-face concept v1 from existing logo identity: no 9/XD/tail/body; olive eyes, white brows and cute black :3 muzzle/white lower jaw, purple/silver edging. Square1254×1254 RGBA, alpha includes0 and255. Private workspace output: C:/Users/Jarky/.codex/.chatgpt-projects/g-p-6aafac188d248191a492c99f58d0a766/branding-concepts/cat-face-v1.png. Exact prompt saved beside it as cat-face-v1-prompt.txt. Concept shown to owner; not installed or added to Qt resources. Existing icon/splash unchanged.
- [ ] After concept selection: prepare optically checked small-size variants and consistent square/padded app/window icons, retain aspect ratio and alpha; verify Windows title/taskbar/Explorer at multiple DPI values, macOS separately. Existing full logo remains suitable for larger brand placements. Generation does not by itself prove legibility at16px or fix OS icon cache/scaling. No build or code changes in this research/concept pass.


## PGR3/4 material-source integration — 2026-10-27 (validated inspection slice)

Owner: Codex. User requested application integration after executable audit. Faithful shader composition remains gated on serializer/CPU binding evidence; do not guess sampler→source channel remaps.

- [x] Implemented and tested: preserve typed raw material fields and reachable alias/layer graph, with bounded/cycle-safe traversal and all source texture references. Vehicle four-reference evidence retained without asserting unproven roles. Source fields and GPU sampler registers remain distinct.
- [x] Implemented and tested: Mesh Collection > Material > Source maps inspector with original resource/slot/channel references and raw graph evidence. Explicit source-map preview re-decodes and refreshes the existing mesh/material, including transformed meshes, rather than adding duplicate geometry. Automatic restores the original conservative base preview. Manual inspection is unlit and retains current UVs; no invented blend/atlas transform. Missing/foreign source keys rejected; original document/revision and batch state checked.
- [x] Compiled dev executable (build/pgr-source-final-build.log); current app is installed/open. Final material corpus 8 passed/0 failed/no skips, including actual source-map pixels vs automatic, unchanged triangles, invalid source rejection, all available maps on reference cars, MainWindow preview/restore without duplicates, and ten distributed Tokyo Night draws per game. PGR3 Ferrari: 87 textured draws/31 maps; PGR4 Alfa:72/32, original counts retained.
- [x] Full shared suite: window120 passed/0 failed/86 optional skipped; reader45/0/no skips. Logs build/windows-local/pgr-source-{corpus-final,full-window,reader}-tests.txt. An intermediate UI test assumed the first auto-preview had a texture; the real first tyre material has none. Harness now selects a map-bearing source part explicitly, then verifies complete car totals195/158 and faces118377/78771.
- [x] Full native Tokyo Sunny decode passed, 11,785 meshes, 10,097 textured, no failed draws; report build/pgr-source-validation/pgr3-sunny-full.json. This is unchanged coverage (1,536 unresolved layer composites +152 missing/unsupported source maps); not a claim that multilayer shading is solved.
- [x] Live Vulkan PGR3 Ferrari Render All:195 visible meshes/118,377 triangles, no failed/omitted. Material82/reference3 changed texture33(256×128)→9(128×64) in Unlit; Automatic restored texture33/Lit; mesh and triangle counts unchanged. Private evidence build/pgr-source-validation/pgr-source-inspector-live.png. Full original blend/atlas appearance not claimed. Handoff: docs/CLAUDE-PGR34-MATERIAL-SOURCES-HANDOFF.md; specialist README, worksheet and user guide synchronized. No commit/push/release.
- [ ] Remaining native work: family-specific serializer→CPU/shader binding, UV/atlas constants and exact base/overlay/road composition. This slice does not implement original game shaders or improve automatic texture-coverage counts.


## PGR3/4 executable material audit — 2026-10-27 (current)

- [x] Owner-authorized local static unpacking of both default.xex files (AES + basic zero-block expansion), valid PowerPC mapped images. PGR3 9,961,472 expanded bytes; PGR4 14,090,240. Separate raw-section analysis PE derivatives validated section-by-section; XEX runtime base0x82000000 used instead of PGR3's stale PE base. Originals untouched, not executed; artifacts/keys/disassembly remain outside repo in private workspace pgr-xex-private/.
- [x] Recovered330/994 shader constant tables and initial PowerPC XREFs. Generic scene diffuse uses sampler0; PGR3 road base diffuse uses8; PGR4 Gotham4 road uses5, legacy road8. Atlas/crop, overlay, light/specular/normal and wear/detail roles evidenced. Serialized SCNE channel is NOT the GPU sampler index. Review: docs/research/pgr34-executables/README.md. No application code, texture assignment or coverage numbers changed; no build/test required for documentation-only research.
- [ ] Codex/native owner: recover exact material declaration/group → CPU binding → shader family/constants and validate selected Tokyo/car samples before changing assignments. Current track channel0 preview and car decal/reference0 fallback remain approximations, not proven game-exact. Need layered rendering, UV/crop transforms and blend evidence; no blanket channel/UV remap from sampler numbers.
- [x] Worksheet/PGR report synchronized; supersedes earlier requirement that the owner must unpack XEX manually. No commit/push/release or game data publication.


## Tokyo pipeline + camera controls — 2026-10-27 (validated)

Owner: Codex; preserve Claude's handedness conversion, texture-library index, scan progress and collapsed options. Requested worksheet: docs/CHATGPT-WORKSHEET-2026-10-27.md.

- [x] Optional local JSONL profiling (`WORKBENCH_MESH_TRACE`): worker decode (with nested texture phase), scene/library/registry, worker-to-GUI interval and dispatch gap. Baseline full Tokyo Qt pipeline: 190,805 ms, 11,785 meshes, 10,097 textured instances, 5,498 shared images; five selections 5 ms. Offscreen test, not a Vulkan frame/upload benchmark. Private logs: build/tokyo-pipeline-before.jsonl and build/windows-local/tokyo-pipeline-before-tests.txt.
- [x] Implemented and tested: reference-counted texture budget in MeshScene; monotonic span lookup in Vulkan upload; tagged two-job prefetch restricted to native-pgr34-track. Stop drains outstanding results before ending the scene batch. Other decoders retain a single job; memory limits unchanged.
- [x] Implemented and tested: View > Camera / Clipping inspector with perspective FOV slider (10–120 degrees), logarithmic Near/Far sliders and exact model-unit fields; automatic clipping preserved, close-surface ray-distance refinement and inherited camera controls for new/tiled viewports.
- [x] Full Tokyo Qt UI pipeline: 190.805 s → 104.052 s (-45.5%, one run each; filesystem warmth not isolated). Both runs preserve every candidate metadata tuple and exactly 11,785 meshes / 2,333,476 vertices / 1,297,789 triangles / 10,097 textured instances / 5,498 shared image identities, zero failures. Last-500 scene insertion .665 → .011 ms. Full curve and phase caveats in docs/PGR34-TOKYO-REPORT.md; offscreen timing is not a Vulkan frame benchmark.
- [x] Live Vulkan on RTX 4060 Ti: Render All finished 11,784 new + 1 already loaded, 0 failed/omitted; 11,785 visible entries and 1,297,789 triangles. Textured city visible; FOV 45→83.9→44.9 changed perspective during the batch; manual Near slider .01→.000186, Far 100000→.281838 visibly clipped the city, Automatic restored it. Private screenshot build/tokyo-live-pipeline.png. Existing texture warnings (1,704) remain; warnings are not an exact missing-texture count. Live run was interactive and concurrent with compilation/tests, so its elapsed time is not a controlled benchmark. App remains slower than offscreen; repaint/event delivery and GPU upload are next profiling targets.
- [x] Final focused regressions covered by full window suite: 118 passed / 0 failed / 85 optional skipped; reader 45/0/no skips. Logs: build/windows-local/pipeline-{focused,full-window,reader}-tests.txt. Full-suite detection corrected an instrumentation regression: read an optional profile name with value() so empty Genki/ARCP recipes stay empty. Tests ran with the VS developer environment for C++ scripting. All scene-layout consumers (including UV panel) rebuilt to avoid stale MSVC header dependency objects.
- [x] User closed the interactive app; final relink succeeded (build/pipeline-final-build.log), dev executable updated. Implementation/repair handoff: docs/CLAUDE-VIEWPORT-PIPELINE-HANDOFF-2026-10-27.md; worksheet, PGR34 report and user guide synchronized. No commit, push or release.
- [ ] Future structural work (Claude/native owner): incremental/per-entry GPU uploads, optional static material merging with faceSources provenance, block-based loading. Current upload still rebuilds the full buffer; span optimization does not replace that architecture.


## Akagi GIM stripes + viewport clipping — 2026-10-27 (current)

- [x] Documentation-only follow-up: docs/CLAUDE-INITIAL-D-SS-HANDOFF.md now begins with the future repair workflow (symptom/layer table, GIM format/transfer and bounds contract, normalized-vs-model camera units, focused test names, restart/re-scan/live validation and pending work). Specialist README synchronized; obsolete pending-test/build paragraphs corrected to completed status. No code changes, new tests, rebuild or publication in this pass. Next owner: Claude/user for remaining warning, original attributes/effects and close-view confirmation.

- [x] Root cause confirmed: 33 T4 images uploaded as PSMCT32 were wrongly treated as linear. Replay GS transfer using declared header format/dimensions with shared gs address helper and written coverage. Private before/after tree/asphalt/map images visually coherent; 106 other images unchanged, all 139 dump-probe images decode. Details/evidence in docs/research/initial-d-ss/README.md and docs/CLAUDE-INITIAL-D-SS-HANDOFF.md.
- [x] Close zoom/fine Shift+wheel, automatic scene/camera near/far clipping, orthographic depth coverage and optional manual near/far in model units implemented. Settings live under viewport Display > Settings; clipping is per session, inherited by new views/tiles. Close navigation speed scales down. Geometry/UV/LOD policy unchanged.
- [x] Targeted 9/0/no skips; reader 45/0; window 115/0 (85 optional skips); application rebuild succeeded after owner closed 9XD, diff check passes. Logs: build/windows-local/idss-swizzle-camera-{focused,reader,window}-tests.txt; build/review-fixes-app-build.log. New application launched; Akagi loaded with zero geometry failures, existing one texture warning. Active human input interrupted specific manual close-view/settings inspection; that visual confirmation stays pending, not claimed complete. Primary implementation: idss_core.h, ps2_gs_address.h, image_recipe.h, viewport_camera.h, vulkan_panel.cpp/.h, camera_navigation.h; tests/window_tests.cpp. No commit/push/release.
- [ ] User/Claude: original colors/fog/blend, remaining texture warning and in-game material equivalence still pending; re-scan/re-render with new build to replace old texture snapshots.

## Initial D SS Akagi independent audit — 2026-10-26 (current)

- [x] Live application propagation verified by Codex: fresh Akagi scan/Render All, 122 main surfaces, 158,296 triangles; 0 failed/omitted, one texture warning. Sky/cloud image, forest terrain and switchback road visibly textured; selected KAGI_DAY078_002 material reads Unlit. App left open in zoomed Top view, sky hidden for inspection. Private screenshot: build/idss_test/akagi-live-review.png. Detailed live receipt appended to research README and dedicated Claude handoff. This supersedes “live check not performed”; in-game comparison and single texture-warning identification remain pending.

- [x] Dedicated implementation handoff: docs/CLAUDE-INITIAL-D-SS-HANDOFF.md (root causes, affected code, real test counts, installed build, reproduction steps and ordered remaining work). Implementation/build/logs rechecked before handoff; no additional decoder changes or publication in this documentation pass. Next owner: user/Claude for live comparison and native colors/effects follow-up.

- [x] Codex: read-only executable/VIF/VU inspection and independent PAC packet audit. 137 models, 216,950 triangles, exact record counts; 415,596 UV pairs use masked UNPACK 0x74 which the saved reader ignored. Corrected UV read, GIF triangle-strip parity (not W bit 2), named LOD isolation (158,296 main / 58,654 LOD triangles), texture/LOD preview identity and explicit unlit base-texture material. No invented/up-facing/BFS faces or global UV flip. Executable not run; no game assets/code published.
- [x] Focused synthetic/actual Akagi tests 6/0/no skips; reader 45/0; full window 113/0 (84 optional skips); Genki actual context/WM/isolation/March 7/0/no skips. Dev executable rebuilt successfully and diff check passes. No commit/push/release. Exact evidence, executable/PAC hashes, VU offsets and semantic limitations are in docs/research/initial-d-ss/README.md; HANDOFF.md superseded where it described fixes absent from saved source. Logs: build/windows-local/idss-analysis-{focused,reader,window}-tests.txt; build/review-fixes-app-build.log.
- [ ] User/Claude: live main-only Akagi scan/render verification; source vertex-color modulation, fog/alpha/blend, V3-32 attributes, active VU variant/front-face conventions and exact LOD visibility remain pending. Main/LOD separation follows original resource names, not recovered runtime distance rules. Native owner should implement colors/effects after visual comparison; per-document PAC parse cache is a separate pending performance issue.

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

## Start here: next session with Claude - 2026-10-26

This section supersedes historical "pending validation" wording in the chronological entries below. Read AGENTS.md and CLAUDE.md first. Work in C:/Users/Jarky/Documents/GitHub/binary-workbench; preserve all uncommitted Codex/Claude changes. Do not copy the older workspace mirror over this checkout. The development executable includes the fixes below; public release remains 0.3.7. No new commit, push or release is authorized by this hand-off.

### Current verified baseline

- Native TXR3 Oval-A/B global/local texture resolution is automatic, using embedded fingerprinted profiles and verified companion locations. Manual context loading is an advanced override, not a required user step. Source-only study copies need a previously verified original resource location.
- 00004a60.nfc: GS region-repeat masks and atlas region-clamp require per-axis sampling and UV scale/offset. Example part 0x55a0 has V scale 4, offset -1.5. Atlas source extent currently uses a power-of-two region envelope; this remains an inference, not a complete GS trace.
- 00000af9.nfc: three nested model-local banks, six textured parts, independent image namespaces. Decoder now loads all six images; another model's bank must never be substituted.
- Open File: optimized asynchronous listing, lightweight icons, deferred search and recent/last locations. Actual TXR3 directory test: 27,112 entries, 3,354 ms, maximum observed GUI gap 349 ms. Normal desktop confirmation remains pending.
- WM PS2 00000740.0iv height/winding correction and prior native-profile isolation fixes are implemented/tested. WM external texture-context mapping remains unresolved.
- Latest functional tests: reader 45 passed/0 failed; full window 104 passed/0 failed/80 optional skipped; real TXR3 region/wheel 5/0/no skips; real context/WM/profile-isolation/March 7/0/no skips. Development windows-local executable builds successfully. Automated/offscreen results do not establish visual equivalence to the game.

### Ordered continuation

1. [ ] User + Claude: reopen, scan and render 00004a60.nfc and 00000af9.nfc with the current development executable. Confirm wheel texture assignment and the specific track surfaces previously distorted. Record exact mesh/descriptor, bank/image and screenshot for any remaining error. Do not mark visual validation complete based on decode/export tests.
2. [ ] Claude / native decoder work: if distortion remains, distinguish decoded image/palette/swizzle from atlas normalization, U/V orientation and per-axis clamp/repeat. Recover the original normalized source extent independently from scripts/GS captures where possible. Do not apply a global UV flip or rescale all textures.
3. [ ] Claude: improve texture diagnostics presentation if needed, keeping automatic loading as the default. Preserve source/bank fingerprints, model-scoped nested namespaces, missing-bank warnings, per-document ownership and refresh-in-place when UV/wrap/bank changes. Do not duplicate already loaded geometry.
4. [ ] Native analysis continuation: expand verified companion maps beyond TXR3 Oval A/B and identify WM-specific banks before claiming broader support. Noncontiguous GS masks, exact subrectangle edge sampling, secondary shaders and animation remain pending; do not borrow TXR3 numeric mappings for other games.
5. [ ] Reconstruction UI continuation: use docs/CLAUDE-RECONSTRUCTION-HANDOFF.md and docs/reconstruction-tools.md. Both inspectors belong to Reconstruction. Remaining functionality includes COLMAP sparse execution/import, shared dependency paths and image-plane/graphical spline editing; frames alone are not a reconstructed model. Prioritize after this texture regression check unless the user redirects.
6. [ ] After changes: run focused tests and relevant regression checks, rebuild, then update this plan and the linked specialist checklist in the same operation. Commit/release only if separately requested by the user.

### Entry points and reproducible checks

- Texture code: src/genki_sampling.h, genki_materials.h, genki_track_context.h, genki_geometry.h, mesh_preview_registry.h; automatic scan UI: src/asset_scan_panel.h. Profile data: profiles/genki-txr3-oval-contexts.json.
- Research/checklists: docs/research/genki-track-banks/README.md, docs/research/genki/GENKI-CROSS-ANALYSIS-TODO.md, docs/research/wm-ps2/README.md. Open File details: docs/file-picker.md.
- Tests in tests/window_tests.cpp: genkiRegionSampling, localGenkiNestedWheelTextures, localGenkiTrackContextLinks, genkiTrackContextContracts, wmTrackHeightConvention, genkiTopologyProfilesAreIsolated, localMarchPrimaryMaterials.
- Optional actual TXR3 tests require WORKBENCH_TXR3_TRACK_ROOT pointing to the original "Nueva carpeta working one" extraction, NOT its differently numbered "uclless" sibling. Game assets remain local, read-only and excluded from publication.
- Existing logs under build/windows-local/: txr3-region-texture-tests.txt, genki-track-context-tests.txt, release037-reader-tests.txt, release037-window-tests.txt. Application build log: build/review-fixes-app-build.log. Use VS2022 vcvars64, Qt 6.11.2 runtime PATH and offscreen platform for automated Qt tests. Touch the test/main translation unit when needed for header-only rebuilds; do not run concurrent builds in the same directory.

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
- [x] Review staged source for private assets, commit/push main, tag v0.3.7 and publish stable ZIP/checksum/public logo.

Changed version/release files: CMakeLists.txt, src/main.cpp, assets/app.rc, README.md, docs/releases/v0.3.7.md. No external reconstruction binaries or game captures enter public source/package. Next: complete validation and publication, then record receipt.

Release 0.3.7 validation: Windows Release/package build and deployment/runtime checks passed; reader 45 passed/0 failed; window 99 passed/0 failed/75 optional skipped; fixture-enabled Genki 14 passed/0 failed/0 skipped. Claude's requested compiler/offscreen rerun is complete. Final package regenerated with completed release notes before archive verification/publication. Qt deployment warns only that optional translation catalogs are unavailable; the app UI is English. Clean-machine validation remains pending.

Final portable archive verified: 9XD-0.3.7-windows-x64.zip, 73,939,562 bytes; SHA-256 3cc15d2d5885cafee1b672d6b0443bfbf4bdeaee02fe5ab33a084dddc31dc846. ZIP contents, required dependencies, Windows GUI subsystem and equality with tested Release executable verified. No research captures, reconstruction binaries/model weights or standalone splash art included. Commit/push and stable publication follow; receipt will be recorded afterward.

## Release 0.3.7 publication receipt — 2026-10-25

- Stable public release, verified latest (not draft/prerelease): https://github.com/Machdramon/9XD/releases/tag/v0.3.7
- Release code commit/tag target: beb47d932cc5d5b6a7d458b61cce33ef941db1e3.
- Verified uploaded assets: 9XD-0.3.7-windows-x64.zip, its .zip.sha256 and 9XD-logo.png.
- ZIP SHA-256: 3cc15d2d5885cafee1b672d6b0443bfbf4bdeaee02fe5ab33a084dddc31dc846; 73,939,562 bytes.
- Build/deployment/portable checks passed; reader 45 passed/0 failed; window 99 passed/0 failed/75 optional skipped; fixture-enabled Genki 14 passed/0 failed/0 skipped.
- Optional FFmpeg/COLMAP/Python packages and weights are not bundled. Game assets/captures and standalone splash artwork remain excluded. Existing embedded About presentation is retained.
- This documentation follow-up records publication after the release code commit; the tag identifies the tested build. No application change in this receipt.
- Next: user validates portable 0.3.7 and spline UI with their images. Claude retains cosmetic inspector follow-ups; backend reconstruction/import/calibration and unresolved Genki resources remain pending.

## Claude study-case format inventory — 2026-10-26

Owner: Claude. Research only; no application code changed. Report: docs/research/format-inventory/FORMAT-INVENTORY.md, probe scripts beside it (no game data). Sources: study cases (about 50 folders) and the F: backup downloads (mostly ISOs/archives).

Findings (evidence level in the report):
- Tested in the development build: Wangan Midnight PS2 000000a3.0iv already decodes through the GIF-strip profile (28 parts, 3,452 vertices, 2,880 triangles; 20 unresolved external texture references). The fnf ps2/01mrszagato ARCX banks decode with the existing ARCX decoder (Xbox-style layout 3 / stride 24).
- Measured: Kaido Racer has no per-record draw marker; plain per-packet strips agree with stored normals for 99.98 % of 5,072 faces and show zero orientation conflicts (C1 calibration: 92.8 % plain vs 99.98 % with its verified Y-bit rule). Kaido is blocked today.
- Measured: every 0IVX model in the corpus ends with a DMA RET quadword (0x60000000) right after its last packet, matching directory boundaries where they exist. This can establish boundaries for concatenated TXR Zero 0x493/0x4d2 and WM PS2 0x740 models, blocked today.
- Hypothesis: a trailing count + 16-byte (id, XYZ) table after the last concatenated model looks like placements.
- Measured: ITC (Xbox 360) and Wangan Midnight PS3 use big-endian XMD.001 containers with XTD.002 (PS3 GTF), X3D0037 models, X2D and XPR2. 9XD already decodes standalone GTF. NFS MW PC GEOMETRY.BIN shares the NFSU2 chunk family and solid version 0x16.

Proposed next tasks (owner to confirm with the user):
- [ ] Kaido 2-vector plain-strip topology profile, isolated from C1/TXRD2 (admission sketch in the report).
- [ ] DMA RET terminator as an accepted 0IVX model boundary for concatenated files.
- [ ] Locate the WM PS2 texture bank file (images 0/52/85/119 of bank 0).
- [ ] XTD.002 container adapter exposing GTF textures (WM PS3), then X3D0037 geometry.
- [ ] Quick app tests: NFS MW PC with the NFSU2 profile; C1 00001010.dat (v1) and other C1 multi-model files.
- [ ] Later: XPR2 (ITC), Blur CPModel, placement-table verification.

Validation performed: Python probes on 140 samples; three live loads in the development 9XD build (WM PS2 wheel, F&F BODY_0 and BODY_X). No build or test-suite run needed (no code change).

## Claude workspace usability fixes — 2026-10-26

Requested by the project owner before the format follow-ups. Owner: Claude. Status: implemented, built and checked in the running app; uncommitted on `main` (no commit/push/release authorized).

1. Open files from every module. The Hex Viewer keeps its MDI document tabs. Every other module (and the main window while the Hex Viewer is detached) now shows an "Open files" strip above the module page: one tab per open document in the same order, close buttons, double-click or the hex button returns to the Hex Viewer. Selecting a tab switches the active document in place without leaving the module. The strip re-applies the switch after window activation, because QMdiArea re-activated its previous document when the click also focused the window (seen in the app, fixed and rechecked).
2. Reconstruction tools out of the 3D scanners. Video Reconstruction and Image / Spline Modeling moved from the 3D module and menu to a new **Reconstruction** module (rail icon "film", menu between UV / 2D and Scripting). Default layout: both docks tabbed, Video first. Dock object names and inspector contracts unchanged. The 3D menu now holds only binary-data tools.
3. Scan no longer resizes the window. Cause: the top-level window had no explicit minimum, so Qt raised its minimum to the layout's (scan panel + docks) when a scan dock appeared, enlarging and un-maximizing the window. Fix: explicit minimum sizes on the workspace (720×480) and detached module windows (560×380); the scan panel content now scrolls inside its dock (the tree keeps ≥150 px), action buttons are split into two short rows and the selection hint moved to the results tooltip.

Files: `src/workspace_window.h`, `src/module_menus.h`, `src/module_windows.h`, `src/asset_scan_panel.h`, `src/theme.h`, `tests/window_tests.cpp`; docs `reconstruction-tools.md`, `CLAUDE-RECONSTRUCTION-HANDOFF.md`.

Validation: `binary_workbench` built with `windows-local` (CLion). App check on the user's monitor: two C1 files open, strip shown in 3D/Reconstruction and hidden in Hex Viewer, switching works with the window focused and unfocused, File Scan and 3D scan keep the window maximized, scan panel scrolls instead of overlapping. `window_tests` from CLion (no vcvars, not offscreen): 96 passed / 4 failed / 75 skipped; new `openFilesStripAndReconstructionModule` and updated `moduleMenusAndDockIsolation` pass. The 4 failures are the known environment ones (`embeddedScriptingPythonAndCpp`, `scriptingLibraryManagerAndImports` need vcvars; `hexDisplayControlsAndOffsetNavigation`, `vulkanPanelFailsGracefullyWithoutDesktop` need offscreen).

Remaining:
- [ ] Full suite with vcvars64 + offscreen (ChatGPT or next session with a shell) before any release.
- [ ] Optional: show the open files strip in detached module windows too (their title already names the active file).
- [ ] Optional: standard saved layout for the `reconstruction` module in `default_workspace_layout.h` (currently falls back to the tabbed default).
- [ ] Note for saved user layouts: old 3D layouts that contained the reconstruction docks are ignored for those docks; no migration needed.

## Claude generic PS2 VIF geometry — 2026-10-26

Requested by the project owner: let 9XD find and build 3D geometry from different games by algorithm, then learn how textures link. Owner: Claude. Status: geometry part implemented and checked; texture linking researched, not implemented. Uncommitted on `main`.

What it does: any file scanned for 3D data is searched for PS2 VIF code streams; UNPACK writes are grouped per microprogram call, attribute roles are inferred from the data, and the strip/restart rule is chosen by measured mesh quality per stream signature. Parts split at DMA RET tags. Results appear in 3D Data Scan as "PS2 VIF geometry (generic, N parts)" plus "PS2 VIF part N" rows (parts are selected explicitly; Render All renders only the full result). Results are marked "Candidate — verify manually" when under 80 % of batches fit or a native Genki decoder already covers the file. Method, corpus table and limits: `docs/research/ps2-vif-generic/README.md` (with the Python prototype and C++ harness).

New results: Kaido Racer car (blocked before) renders and auto-previews; TXR Zero / WM PS2 concatenated files now render (as parts); R Racing Suzuka course (30,631 vertices), an unknown `.nud` sky dome and Super Trucks `ALBACETE.MIX` are new. C1, TXR3, TXRD1 match the native results; TXRD2 partially.

Files: new `src/ps2_vif_core.h` (Qt-free core, validated on Linux against the prototype), `src/ps2_vif_geometry.h` (scan/decode adapter); `src/asset_discovery.h`, `src/mesh_profile_decode.h` (`native-ps2-vif` dispatch), `src/asset_scan_panel.h` ("Part of the full result" rows), `tests/window_tests.cpp` (`genericPs2VifGeometry`: synthetic two-part stream + noise rejection).

Validation: window_tests built and run from CLion: 97 passed / 4 failed (the same 4 environment failures) / 75 skipped; `genericPs2VifGeometry` passes. App built and checked live: Kaido `000004ac.dat` auto-previews the full car; R Racing `CourseSuzukaF/Model` loads 30,631 vertices / 26,850 faces.

Texture findings: GS TEX0 writes are absent from Genki model files and rare elsewhere (only F&F ARCP, D1GP, Initial D course AFS have GS register uploads), so generic TEX0 linking only covers a few games. Genki TIM2 headers carry VRAM placement (GsTex0) but models use bank/image indices.

Next (proposed, owner Claude unless reassigned):
- [ ] Material key discovery per RET part (fields whose range matches the texture count) and UV/texture-size checks, to link textures generically.
- [ ] Apply per-part matrices (F&F ARCP) and STROW/STMOD offsets; indexed VU programs.
- [ ] Remaining TXRD2 groups (find the missing restart rule).
- [ ] Packet explorer panel (tree of DMA/VIF/batches with jump-to-bytes) on top of the same core.
- [ ] Full vcvars + offscreen suite before release.

## Owner direction and community knowledge — 2026-10-26

Owner feedback on the generic VIF method: close, but it makes errors on complex formats and misses format-specific rules. Decision: keep it **only as a fallback for unknown formats**; known formats must use (and improve) their native decoders.

Done (Claude): `src/asset_discovery.h` now runs `Ps2Vif::scan` only when no native decoder produced a mesh candidate (Kaido still gets it until its native path renders). Build/test of this one-line change is pending (the app was in use).

New sources from the owner: `F:\discord dump\gatos` (Gato's Lair Discord export with Genki/TXR/ITC/WM/SBOL/D1GP/PGR/FM1 research scripts) and `F:\xentax` (Xentax dump, 13,628 entries; needs a file list). Map and gaps: `docs/research/community-knowledge/README.md`, with `ivx_struct.py`, which walks the community 0IVX hierarchy (model → container with locator matrix → part descriptor → packets) on every Genki study case without a span error.

Proposed next (owner to confirm the order):
- [ ] Genki native: exact model/container/part hierarchy, container locators, per-part names; replaces boundary heuristics (unblocks TXR Zero/WM PS2 placed objects and Kaido natively).
- [ ] Genki textures: descriptor +13/+14 global/animated/local containers across files; per-segment UV scale.
- [ ] Genki file roles/LOD labels (TXRD2/Kaido car file order, track LODs).
- [ ] New native families with community readers: ITC XMD, WM PS3, TXR2 (Dreamcast), D1GP (validate against the attached OBJ exports).

## ChatGPT review of Claude changes — 2026-10-26

User requested review. Report: docs/CLAUDE-REVIEW-2026-10-26.md. No application source changes, no publication/commit. Review includes the newer uncommitted GenkiHierarchy and Kaido changes, beyond the previous narrative update.

Validation: reader 45 passed/0 failed; full window suite 100 passed/1 failed/76 optional skipped under vcvars64/offscreen. Actual failure: genkiTopologyProfilesAreIsolated, damaged C1 admitted as Kaido. Targeted real corpus: 4 passed/2 failed; hierarchy and March materials pass; second failure is the obsolete local test expectation that Kaido remains blocked. Detailed logs and distinction in report.

- [x] P1: enforce distinguishing Kaido admission; keep malformed C1 rejection (src/genki_mesh_evidence.h).
- [x] P2: repair DMA tag mask that rejects all permitted IDs (src/ps2_vif_core.h).
- [x] P2: include rejected groups in generic scanner confidence denominator (src/ps2_vif_geometry.h).
- [x] P2: inverse-transpose stored normals for nonuniform node scales (src/genki_geometry.h).
- [x] Update intentional Kaido coverage expectations and native documentation, preserving profile-isolation tests; validate generic fallback only for unknown formats.

Next owner: Claude/user for fixes and full/corpus revalidation before a new release. Findings are static/test-backed as described; not a fresh visual inspection. Existing release 0.3.7 remains unchanged.

## Review corrections implementation — 2026-10-26

User explicitly authorized ChatGPT/Codex to fix all four review findings. Source changes preserve Claude's uncommitted work. No commit/push/release requested.

Implemented pending validation: Kaido admission requires complete hierarchy and a distinguishing Float32 UV/V4-16 normal attribute witness; damaged C1 no longer gains Kaido admission. Generic fallback is suppressed for files with recognized native mesh evidence. DMA mask now preserves and checks the tag ID bits. Generic confidence denominator includes every classified batch. Native normals use normalized inverse-transpose under nonuniform scale; singular scales regenerate normals from transformed faces; reflected transforms reverse triangle winding to preserve normal/face consistency.

Files: src/genki_mesh_evidence.h, ps2_vif_core.h, ps2_vif_geometry.h, genki_hierarchy.h, genki_geometry.h, asset_discovery.h, tests/window_tests.cpp. Added reviewDecoderRegressions (DMA chains/bounds/reserved bits, low/full coverage, nonuniform/singular/mirrored normals and native mirrored/singular geometry decode). Updated real-corpus isolation expectation only for explicit Kaido profile in Kaido fixtures; malformed-C1 rejection test retained unchanged.

Build/full tests underway. Required before completion: full window suite, actual Kaido/Genki fixtures, material regression and updated development executable. Optional ongoing format/material research remains separate.

## Review corrections completed — 2026-10-26

- [x] All four findings corrected with regression tests; fallback also suppressed for known native mesh evidence, preserving the owner's unknown-formats-only rule.
- [x] Full suite: reader 45 passed/0 failed; window 102 passed/0 failed/76 optional skipped.
- [x] Real-file targeted hierarchy/isolation/March run: 6 passed/0 failed/0 skipped; confirms Kaido and concatenated models while retaining damaged-C1 rejection and March materials.
- [x] Broader fixture-enabled Genki/material/preview/export checks: 14 passed/0 failed/0 skipped.
- [x] Development binary_workbench.exe built and replaced successfully; git diff --check passed.

Logs: build/review-fixes-app-build.log; build/windows-local/claude-review-targeted.txt; release037-reader-tests.txt, release037-window-tests.txt and release037-material-tests.txt (local filenames reused, not a new release). Historical review report now has a resolution note; Genki/community/generic documentation synchronized. No commit/push/new release. Next: user tests current development executable; Claude can continue from these fixes. External texture links, scene/LOD selection and other pending format work remain open.

## WM PS2 height and texture investigation — 2026-10-26

User reports inverted height in wm ps2/00000740.0iv and requests existing texture research. Owner: ChatGPT/Codex. Source scripts read only, never executed.

Evidence: F:/discord dump/gatos/assets/IVX_rework_rev0.77_local_read-C18E0.py lines 771 and 1058–1059 distinguish raw track input and invert position Y for the track mode. The reported WM fixture contains three raw consecutive GIF-strip 0IVX models and a bounded final seven-record ID/XYZ table; TXR Zero raw track examples share this carrier. The independent WM wheel has no placement trailer and is excluded from automatic conversion.

Implementation pending validation: bounded rawTrackYDown carrier check in genki_hierarchy.h, explicit sourceYDown/coordinateConvention evidence in genki_mesh_evidence.h, Y reflection after node/world transform for positions and normals with combined reflection/winding handling in genki_geometry.h. Added wmTrackHeightConvention test: actual fixture Y inversion with X/Z preserved, malformed/truncated trailer rejection, winding and standalone-wheel exclusion. Full compile/tests running.

Texture sources: txr3_reworkglob-dump_rev1.32-0909B.py reads descriptor +13 bank and +14 image; 0 is common/global, 4 animated/global, others local in that TXR3 script. Its image filenames use one-based display numbering (+1), while stored IDs are zero-based. WM must be validated independently before reusing these bank-role rules. WM Discord notes say manual texture assignment may be needed. Survey of extracted WM game files found one valid bank with >=120 images: quickbms unpack/00000150.tm2, 169 images. Candidate, not yet automatic binding. Source bank/game files unchanged. Next: validate material index/dimensions, finish tests and update development executable.

## WM PS2 correction result — 2026-10-26

- [x] Bounded raw-track Y conversion implemented and tested on 00000740.0iv (2,488 vertices / 1,826 triangles); X/Z preserved, normals/winding corrected, wheel and wrapped cars excluded.
- [x] Development executable built and updated; reader 45/0, windows 102/0 (77 optional skipped), fixture-enabled WM/hierarchy/isolation/March 7/0; git diff --check passed.
- [x] Read legacy scripts/Discord and survey extracted banks. Research and limits: docs/research/wm-ps2/README.md with reproducible metadata-only survey helper. Found 317 TIM2 bank regions; 00000150.tm2 (169 images) covers the wheel's reference range but is not verified for the track. Additional track-range banks and CLAMP-layout mismatches recorded.
- [ ] User: visually verify corrected height after rescanning/reloading the model.
- [ ] Validate exact WM external bank/material selection before automatic texture assignment. No guessed binding introduced.

Documentation synchronized with community and Genki checklists. No commit/push/release; prior published 0.3.7 unchanged. Only source-code changes and metadata reports; game assets/scripts unchanged.

## Genki global/local track texture contexts — 2026-10-26

User requested review of their Discord scripts for global/local/weather container binding. Owner: ChatGPT/Codex. Report/prototype: docs/research/genki-track-banks/README.md, txr3-oval-contexts.prototype.json and validation-summary.json. No app code or game files changed; no commit/push/release.

- [x] Locate original global/local messages and material +13/+14 selector logic, keeping weather/build/segment scope explicit.
- [x] Validate correct extraction provenance: study 603f matches Nueva carpeta working one; uclless numeric filenames represent different resources.
- [x] Resolve 1,592 Oval-A and 1,920 Oval-B descriptor references using explicit common/animated/global and per-segment local maps. Index binding proof only; not final visual/material validation.
- [x] Compare bank payloads across variants: common 36/54 same, 18 differ; animated 1/1 same. Both global and local selection may depend on the track/condition context.
- [x] Implement native GenkiTrackContext resolver/selection and connect to scan/render/export for verified TXR3 Oval profiles; fingerprinted profiles/genki-txr3-oval-contexts.json is loaded by the application. Older research prototype remains separate.
- [ ] Resolve sampler/CLAMP/UV semantics (only 10 size-exact matches per variant under current receipts) and verify materials visually.
- [ ] Identify WM-specific contexts; do not reuse TXR3 numeric IDs or selector exceptions blindly.

Validation: own bounded read-only Python prototype over real resources, 3,512 index-resolved references, no reported span walks failed. Metadata only added to repo; no binaries/images/captures. Next owner: ChatGPT/Codex native resolver; Claude UI after functional contract is implemented. Broader Genki texture/LOD tasks remain pending.

## Native track-context integration — 2026-10-26

User explicitly requested implementation. Owner: ChatGPT/Codex. Files: new src/genki_track_context.h and profiles/genki-txr3-oval-contexts.json; src/asset_discovery.h, asset_scan_panel.h, genki_geometry.h, mesh_preview_registry.h, main.cpp and tests/window_tests.cpp.

Implemented: explicit context/profile and extracted-root selection from Track Texture Context… in File/3D scans; fingerprinted common/animated/global and per-segment local banks; copied-fixture identity verification; bounded TIM2 directory/picture checks; per-image cache keyed by full bank identity; native primary image decode, provenance and image-library/export propagation. Context changes re-render existing untransformed Genki parts in place rather than duplicating geometry. Unknown sampler regions use Repeat preview with visible texture warnings, animated banks use a static source image. No automatic guessing or climate labels assigned to Oval A/B.

Current tests: reader 45/0; full windows 103/0/78 optional skipped, including synthetic red/blue contexts, missing/mismatched bank fingerprints, geometry mismatch, path bounds, changed bank invalidation and material-refresh plan. Optional real Oval A/B scan/decode/GLB/UI tests running; app build underway. No commit/push/release.

Pending before completion: real-file results, updated executable, user instructions and limitations in native-scanning/context research docs. WM-specific context identification and exact sampler/atlas/animation behavior remain separate.

## Track-context integration completion — 2026-10-26

- [x] Native fingerprinted global/local context selection and decoding integrated; per-document scan controls and native material provenance connect to render, image collection and GLB/FBX export.
- [x] Context changes update an existing visible untransformed part rather than duplicate it. Actual UI test confirms linked texture→clear context→rescan removes the external texture while retaining one mesh.
- [x] Reader 45/0, windows 103/0 (78 optional skipped); real context/WM/isolation/March 7/0 (no skips). Actual Oval-A model: 21 textured parts; Oval-B: 31, each decoded and exported as GLB and FBX. Unit tests cover wrong root/hash, changed bank, geometry mismatch, bounded paths and cross-context image cache isolation.
- [x] Development binary_workbench.exe built and updated. git diff --check passed. User instructions recorded in docs/research/genki-track-banks/README.md and docs/native-scanning.md.

Logs: build/windows-local/genki-track-context-tests.txt, release037-reader-tests.txt and release037-window-tests.txt; build/review-fixes-app-build.log. Initial optional UI harness tried scanning before file opening completed; readiness wait and nonpositive-size scan guard corrected, final run passes. No publication/commit.

Remaining: WM/TXRD2/other context maps and selector proofs, exact sampler/atlas/animation/shader behavior, wider native corpus/visual checks and polished context preferences/UI. Oval A/B climate names intentionally remain unlabelled. Current native resolver accepts the verified TXR3 Float32 primary-UV path; unmatched parts remain unresolved. Claude may refine the functional context controls without breaking provenance, panel/session ownership or refresh safeguards.

## Large-folder Open File — 2026-10-26

User reports freezing while navigating Open File into TXR3 folder with around 30,000 entries; explicitly identified Open File rather than the track-context folder selector. Owner: ChatGPT/Codex.

Implemented pending final checks: lightweight generic cached icons and extension-derived types avoid per-file shell handlers; retain QFileSystemModel's asynchronous gathering; uniform/batched view layout and interactive rather than content-scanned column widths; empty-query filter fast path, 220-ms search debounce, bounded search text, loading/entry-count status, default file-type groups. Multiple selection, recent locations and last successful folder retained. Shared single/multiple/folder wrappers use consistent optimized listing; related context-picker routes through them. No third-party UI code copied.

Sources/reference designs: Qt QFileSystemModel threading/performance docs; Windows File Explorer navigation/details/search; Files Community and Blender File Browser as design references. App code: src/file_dialogs.h and src/asset_scan_panel.h. Tests: window_tests.cpp existing search/selection tests adapted for asynchronous filtering, optional largeFileDialogListing measures event-loop gaps and filtering on actual read-only TXR3 folder. Initial search test used nonpersistent model indices across deferred sorting; corrected to wait for rows and use QPersistentModelIndex, final rerun underway.

Required before completion: full suite, real large-folder measurement, updated executable and documentation. No release/push/commit requested. Network/offline-volume latency and opening thousands of documents at once are separate from directory listing; no new hard document count limit introduced.

## Large-folder picker completed — 2026-10-26

- [x] Optimize icon/type work, empty filter, deferred search and view layout; add loading/count feedback and type filters while retaining selection/history behavior.
- [x] Full reader 45/0 and window 103/0 (79 optional skipped), correctly configured environment.
- [x] Actual TXR3 folder: 27,112 entries loaded in 3,354 ms, worst measured GUI event gap 349 ms, 246 heartbeat events. Focused run 5 passed/0 failed/0 skipped, including name/wildcard search, cancel and multi-select/recents. Read-only/offscreen test; no game files modified.
- [x] Development executable built and updated; git diff --check passed. Documentation: docs/file-picker.md, with measured scope, limitations and Qt/Explorer/Files/Blender reference links.

Source changes limited to src/file_dialogs.h, src/asset_scan_panel.h and tests/window_tests.cpp. Original root cause was not debugger-profiled; expensive shell and filtering work removed, observed responsiveness validated on the reported folder. No new release/commit/push. Next: user tests Open File on the same folder in their normal desktop session; Claude may refine presentation using documented references without reintroducing eager per-file shell/preview work. Offline/network-volume latency and opening huge document batches remain separate work.

## Automatic track textures — 2026-10-26

User requested removal of mandatory manual context loading. Owner: ChatGPT/Codex. Implemented pending validation: fingerprinted TXR3 profiles embedded into app/test resources; discoverAssets automatically tries mapped profiles beside the source (bounded parent checks) and previously verified locations, with model/bank identity validation. No whole-folder enumeration or arbitrary bank guesses. Verified contexts are remembered by the scan UI for later copied study fixtures. Explicit overrides remain advanced controls; Automatic resets the override, not texture support.

Files: CMakeLists.txt, src/genki_track_context.h, asset_scan_panel.h, tests/window_tests.cpp. Test now calls normal discovery without context, scans through UI without setting a profile, and checks copied-model discovery from a remembered verified location. Reader/window build and real corpus runs pending. No commit/push/release.

## Automatic track textures verified — 2026-10-26

- [x] Verified profiles embedded in app/test resources; normal discovery automatically checks mapped model/companion identities in the source folder, bounded parents and remembered verified contexts. No JSON selection required in the normal workflow.
- [x] UI defaults to Track textures: automatic. Texture options… is an advanced override; Automatic clears only the override. Successfully discovered resource locations are remembered (maximum eight contexts) for later copied study fixtures.
- [x] Reader 45 passed/0 failed; full windows 103 passed/0 failed/79 optional skipped. Real context/WM/isolation/March run 7 passed/0 failed/no skips, including both automatic Oval variants (21/31 textured parts), GLB/FBX, UI scan with no context setter, returning to Automatic without duplicate geometry, and a model copied to a bank-less directory resolved through remembered verified resources.
- [x] Development executable built/updated and git diff --check passed. CMake resource list, source, tests and user/research documentation synchronized. No commit/push/release.

Scope remains the verified TXR3 contexts. Missing or mismatched companions produce a visible note and keep geometry; unsupported WM/other mappings are not fabricated. No broad drive/directory scan introduced. Next: user follows Open File → Scan → Render; outstanding format maps and sampler/animation work retain their status.

## TXR3 wheel / texture-region correction — 2026-10-26

User reports partial incorrect sampling on 00004a60.nfc and no textures on wheel 00000af9.nfc. Owner: ChatGPT/Codex. Investigation found three bounded nested model/TIM2/tail containers in the wheel, with independent image namespaces (128, 64, 16/32 px); the previous parser inspected only root TIM2 sections. Track descriptors store GS mode 3 region-repeat masks/origins, and mode 2 atlas rectangles: reading MIN/MAX as full-image bounds and using Repeat on both axes loses extracted-region scale/origin and independent clamp/repeat modes. Real UV examples: part 0x55a0 uses V .375–.625 for source region 96–159 of a 256px axis, decoded image height 64; part 0x18540 uses U .5–.75 / V .625–.75 for a 16px image at (32,80).

Implemented pending tests: bounded model-scoped nested bank receipts; contiguous GS region-repeat validation; per-axis wrap and extracted-region UV scale/offset. Source extent inferred as power-of-two envelope of the contiguous stored region; noncontiguous masks/unmatched extents remain unresolved. No blanket UV flip or image resize. Files: src/genki_sampling.h, genki_materials.h, genki_track_context.h, genki_geometry.h, tests/window_tests.cpp.

- [x] Synthetic region math/invalid-mask checks, actual six wheel parts and bank-isolation negative test (5 passed/0 failed/no skips in focused run).
- [x] Both track variants decode/export (21/31 external textured parts); part 0x55a0 uses V scale 4 and offset -1.5, checked through native decode. GLB/FBX and UI rescan checks pass.
- [x] Reader 45/0; full window 104/0 with 80 optional skipped; additional real Genki context/WM/isolation/March run 7/0/no skips. Development executable built/updated successfully. Final follow-up changes to diagnostic wording/comment only; functional tests cover final sampling and refresh behavior.
- [ ] User visual confirmation of reported surfaces; full game shader/animation and arbitrary atlas extent remain unverified.

No commit/push/release requested. Claude should preserve scoped bank identities and per-axis sampling during UI work.

TXR3 follow-up completed: scoped-bank decode yields all six wheel parts with images from their own 128/64/16/32px banks; missing/broken nested directory does not leak another model bank. Re-render refresh also detects UV scale/offset and wrap changes. Files additionally include src/mesh_preview_registry.h. Logs: build/windows-local/txr3-region-texture-tests.txt, genki-track-context-tests.txt, release037-reader-tests.txt, release037-window-tests.txt, build/review-fixes-app-build.log. Source game files unchanged; no commit/push/release. Next: user visually checks 00004a60.nfc and 00000af9.nfc after reopening/rescanning/rendering; source atlas extent remains inferred from the stored power-of-two region envelope, not recovered from a full GS execution trace. Full shader/secondary-stage/animation and noncontiguous region masks remain pending.

## Additional documented format candidates - 2026-10-26

User asked which new families can be implemented from local documentation and current study cases. Research cross-check completed, no decoder changes/tests/build in this session. See docs/research/format-inventory/IMPLEMENTATION-CANDIDATES.md for exact references, samples, evidence strength and limitations.

Recommended: WM PS3 (Discord native-reader reference plus Xentax MaxScript; XMD/XTD/GTF/X3D), ITC (geometry/UV/material references, XPR2 still needed), NFS MW PC (test existing NFSU2 pipeline before separate variant), then D1GP (related source/OBJ pairs), Ridge Racer PSP and Auto Modellista. No support claimed yet. TXR2-named attachment actually contains PS2/TXRD2 logic; do not use as Dreamcast proof. SBOL lacks a matching identified sample. Old Kaido/Genki-boundary inventory blockers have been resolved and should not be reimplemented.

Validation: read-only script text/ZIP listing, sample sizes/signatures and current native support search. Scripts/converters not executed; no game assets copied or added. Next owner: user selects priority, then Claude/Codex independently implements and validates the selected native path. Existing texture visual-regression priority remains pending, not superseded by this research list.

## Wangan Midnight PS3 native reader — 2026-10-26

Owner asked to implement the changes from the community-knowledge map. Most Genki items (hierarchy, locators, Kaido, WM PS2 Y-down, TXR3 global/local contexts) were already completed by ChatGPT/Codex and were not reimplemented. Claude picked up priority 1 of `docs/research/format-inventory/IMPLEMENTATION-CANDIDATES.md`. Owner: Claude. Uncommitted on `main`; no push/release.

- [x] Xentax dump indexed from `F:\xentax-lista.txt`: attachment-only (13,576 files, no thread text). Relevant: WM PS3 MaxScript (5 revisions + gtf2dds), GT4 memory BMS, Enthusia sample, Gotham Racing tools, axmd.py.
- [x] Native reader `src/wm_ps3_core.h` (Qt-free, validated on Linux against both samples) + `src/wm_ps3_xmd.h` (scan/decode). XMD tree with nested XMD/XTD, X3D block table, strings, materials, texture slots, shape bindings, geometry streams (strip indices with 0xFFFF restart, positions, normals, UV set 0). Texture chain: material slot → sibling XTD entry → (bank, index) reference → embedded GTF decoded by the existing GTF reader. One candidate per geometry with its own material/texture; GTFs also listed as images; neon/LED/spot tuning shapes marked as variants.
- [x] `src/asset_discovery.h` (scan before FAF), `src/mesh_profile_decode.h` (`native-wm-ps3-xmd`), `tests/window_tests.cpp` (`wmPs3NativeXmd`: synthetic XMD + truncation rejection; optional real JZS161 checks with `WORKBENCH_STUDY_CASES`: 162 candidates, 8 images, 121 textured, interior 1024×1024).
- [x] Results: JZS161.CAR 162 geometries / 35,648 vertices / 46,376 triangles, 121 textured; GRID 40 geometries / 10,485 / 6,078, textures in external course banks (not in the file).
- [x] Validation: app built (CLion windows-local) and checked live: car scanned, Render All 0 failures, decoded interior/light/plate textures appear in the library. window_tests from CLion: 101 passed / 4 failed (the known vcvars/offscreen environment failures) / 80 skipped; `wmPs3NativeXmd` passes (real-sample block skipped without the env var).
- [ ] Course XTD banks for GRID files (needs a course folder sample); extra UV sets/lightmaps; default customization variant selection; node transforms (identity on the sample).
- [ ] Full vcvars + offscreen suite with `WORKBENCH_STUDY_CASES` set, before any release.

Notes: `docs/research/wm-ps3/README.md`. Next candidates unchanged: ITC (XMD family, shares the container), NFS MW PC, D1GP, Ridge Racer PSP, Auto Modellista.

## PSP GMO + NFS MW PC native readers, study-case roadmap — 2026-10-26

Owner asked to implement what is feasible from the unanalyzed study cases and to start a roadmap for the rest. Owner: Claude. Uncommitted on `main`; no push/release.

- [x] PSP GMO reader: `src/gmo_core.h` (Qt-free) + `src/gmo_model.h` (`native-psp-gmo`). Both header variants (standard / Genki). Arrays + draw-arrays with GU vertex types and fixed-point offset/scale. One candidate per model. GIM textures are external and reported by name. Initial D `n_sy2_blk_01_0.bin` has 6,131 tris; Street Supremacy `000009cc.sdb` has 2,072. Checked live in the app. Test `pspGmoNative`.
- [x] NFS Most Wanted PC `GEOMETRY.BIN`: `src/nfs_mw_core.h` (Qt-free) + `src/nfs_mw_solid.h` (`native-nfs-mw-solid`). This is a separate variant: NFSU2's `nfsu2_scene.h` requires scenery chunk 0x80034100 and the 0x44-byte mesh header, and stays untouched. Layout: 0x134B02 0x68-byte submesh records, solid-global u16 triangle lists in 0x134B03, 36-byte vertices in 0x134B01, labels in 0x134C02.
  - 911 Turbo: 291 solids, 76,339 tris, all indices in range, no degenerate triangles; the render looks correct.
  - Candidates: a stock-car assembly (21 LOD-A stock parts, 11,029 tris) plus each solid. Parts of the assembly are marked `partOfCandidate`. LOD B–E, damage, decals, aftermarket kits and hoods are `optionalVariant`. Tires and brakes are listed on their own (modelled at the origin).
  - The NFSU2 and NFS PSP files are rejected by the structural checks.
  - Test `nfsMwSolids`: synthetic + broken-index rejection; optional real check is 292 candidates / 11,029 tris.
- [x] window_tests from CLion: 103 passed / 4 failed (the known vcvars/offscreen environment failures) / 80 skipped. `pspGmoNative` and `nfsMwSolids` pass.
- [x] Roadmap `docs/research/format-roadmap/ROADMAP.md` covers:
  - done formats and their follow-ups;
  - 11 next readers with evidence and effort;
  - formats needing analysis;
  - encrypted/compressed blockers (GT4, PCARS, WMMT, MCLA, Dacia);
  - out-of-scope items;
  - suggested order: AFS container → ITC → NFS PSP + Ridge Racer PSP + GIM → TPK textures.
- [ ] NFS MW not yet checked live in the app UI (validated in the test suite and with an offline render).
- [ ] TPK textures (`TEXTURES.BIN`) for NFS MW and NFSU2; GIM decoder for GMO; GMO model-name parse (shows `D`).
- [ ] Full vcvars + offscreen suite with `WORKBENCH_STUDY_CASES` set, before any release.

## ITC X360, RenderWare (D1GP / Super Trucks), roadmap update — 2026-10-26

Owner priority: ITC, Super Trucks, Initial D SS, GT6, D1GP, Blur, NUD. Owner: Claude. Uncommitted on `main`; no push/release.

- [x] ITC: `src/itc_core.h`, `src/itc_model.h` (`native-itc-x360`), `src/xpr2_textures.h`. The XPR2 decoder reuses the Xenos untile in `texture_decode.h`; `image_decode.h` dispatches on the `XPR2` magic. Generic XPR2 image listing in discovery. Test `itcX360Model` passes.
- [x] RenderWare 3.x: `src/rw_core.h`, `src/rw_model.h` (`native-renderware`): clumps with frames, worlds (plane/atomic sectors), plain and PS2-native geometry, plus a Selene scene scan for `.MIX` files. Validated offline: D1GP car/track, Super Trucks cars/track. Test `renderWareClump` was added but **not run yet** (CLion could not be focused).
- [x] Roadmap update in `docs/research/format-roadmap/ROADMAP.md`. Initial D SS is blocked by an unknown LZ codec; GT6 needs mesh analysis; NUD is covered by the generic VIF; Super Trucks car placement is pending.
- [ ] Run window_tests (`renderWareClump`) and check D1GP/Super Trucks/ITC live in the app.
- [ ] RenderWare PS2 TXD textures; Selene node placement.

## Joint native-reader validation picked up - 2026-10-26

User asked what remains and what Codex can start. Owner: ChatGPT/Codex. No active cl/link/ninja/cmake processes were observed before starting the serialized windows-local reader/window test build. Application source logic unchanged. Preserve all Claude/Codex uncommitted work; no commit/push/release authorized.

- [x] Build current reader_tests/window_tests using VS2022 vcvars64 and Qt/offscreen.
- [x] Run new-reader focused tests with WORKBENCH_STUDY_CASES set; extended RenderWare coverage to actual scan/decode of D1GP and Super Trucks. Final focused run: 9/0/no skips.
- [x] Run full configured reader/window regression: 45/0 and 110/0 (81 optional skipped). Earlier CLion failure set does not reproduce under vcvars64/offscreen.
- [x] Verify actual RenderWare corpus scan/decode: 69/16 D1GP candidates, 80/10 Super Trucks candidates; finite output and valid indices. CARS repairs five normal vectors across three parts. ALBACETE coverage discrepancy remains open.
- [x] Update roadmap with actual results and next owner. No desktop visual proof claimed by this batch.

Next implementation priorities after validation: RenderWare PS2 TXD decode/material mapping (D1GP + Super Trucks), PSP GIM decode/linking (GMO), NFS MW TPK, external WM PS3/ITC banks, Selene/car-part placement and default customization variants. Renderer/scan reuse by document revision should be considered before more large scenes; do not use new hard counts to conceal repeated parsing costs. Initial D SS compression remains a distinct blocker; GT6/Blur remain research work.

Logs planned: build/native-readers-validation-build.log and build/windows-local/native-readers-{reader,focused,window}-tests.txt. Header-only build safety: timestamp of tests/window_tests.cpp refreshed, no test content changed by this step.

### Joint validation findings and corrective work - 2026-10-26

Initial configured run passed: reader 45/0, full window 109/0/80 optional skipped; five-reader focused run 7/0/no skips with study cases enabled. This supports the earlier CLion failures being environment-dependent; it does not establish live visual correctness.

Independent real RenderWare core walk: D1GP A31 16,275 tris / HARE_USA 40,239; Super Trucks CARS 196,987, ALBACETE 101,577. CARS has 15 nonfinite normal components in three parts (source offsets 28485656, 32424010, 34059636), while positions/UV/index checks pass. ALBACETE differs from the historical 128,409 count; coverage discrepancy is pending investigation, not treated as a successful complete extraction.

Fix in src/rw_model.h: preserve finite positions and topology, clear nonfinite normal vectors and regenerate them from recovered faces using the existing normal-generation path; expose repairedNormalCount/normalWarning in source bindings. Invalid positions continue to fail. Added renderWareNonfiniteNormals and optional localRenderWareCorpus tests in tests/window_tests.cpp, covering all four files through the actual native decode adapter. Rebuild/focused/full results pending. No community scripts or game binaries executed/modified.

### Joint native-reader final tests - 2026-10-26

Final focused run 9 passed/0 failed/no skips, including synthetic NaN-normal repair, optional actual four-file corpus and existing WM/ITC/GMO/MW fixtures. Full reader 45/0, full window 110/0/81 optional skipped. Actual CARS adapter yields 80 candidates, 196,987 triangles, five repaired normal vectors; D1GP 69/16 candidates and 16,275/40,239 tris; ALBACETE 10 candidates and 101,577 tris. No decoded geometry dropped by the repair. Correct environment removes the previously reported four CLion failures in this run. git diff --check passes. Application rebuild in progress; no release/commit/push. Next functional priority: TXD texture decode plus per-material surfaces, then GIM/TPK/external banks; keep ALBACETE discrepancy and desktop visual checks pending.

Development application rebuild completed successfully with the latest native readers and RenderWare normal repair (build/review-fixes-app-build.log). No release/commit/push; next owner may start TXD after desktop regression review. No tests were claimed for external-bank linkage or material completeness.

## RenderWare TXD implementation picked up - 2026-10-26

User requested the next point: TXD textures for D1GP/Super Trucks. Owner: Codex. No commit/push/release. Existing Claude work preserved.

Implemented pending full verification: src/rw_txd.h reads bounded native PS2 raster records and version-2 GIF transfer payloads; uses existing GS page addressing and PS2 CLUT/alpha conversion, handles supported indexed8/indexed4 and direct16/32 layouts, base mip only. Legacy version 0 linear supported, version 1 swizzle explicitly rejected. Raster-format specification cross-checked against https://github.com/aap/librw/blob/master/src/ps2/ps2raster.cpp; independent parser, no legacy script/converter execution or library dependency added.

src/rw_model.h partitions recovered triangles by primary material texture name, links exact names inside the explicit source/scene scope, and exposes different same-name TXD liveries as optional alternatives. Default variant 0 is explicitly a preview choice, not proof of runtime livery selection. Default surfaces partition geometry without duplicating it. Missing dictionaries preserve geometry with warnings. Session cache reuses immutable patched source, scoped parsed geometry and decoded images; document changes replace the session. Receipt validation rejects changed sources/cross-scope bindings.

Integration: asset_discovery image candidates, encoded native TXD preview in image_decode/image_recipe, RenderWare Session in mesh_profile_decode/main, material/texture identity in mesh_preview_registry. Added synthetic material-partition/scope/variant/preview/export/cache tests and optional all-image test. Existing real corpus test now decodes default surfaces through a shared session.

Read-only survey: D1GP A31 has 56 native raster records, HARE_USA 94; Super Trucks CARS 1,106 in repeated livery dictionaries. ALBACETE contains no PS2 native TXD records (needs separate texture-resource analysis). Independently decoded samples inspected: BFGoodrich side atlas and course spectator atlas are readable; this does not prove complete in-game UV/shader equivalence. Raw files unchanged. Full focused/corpus/window/reader runs underway; build log build/rw-txd-validation-build.log.

Remaining outside this step: ALBACETE external texture resources and coverage discrepancy, Selene placement, exact runtime livery/configuration, secondary shader/blend parameters, mip chains and unsupported TXD storage variants. Next material family after TXD: PSP GIM or NFS TPK per roadmap.

### TXD integration verified - 2026-10-26

- [x] Native raster decode: all 56 A31, 94 HARE_USA and 1,106 CARS raster records decode at declared dimensions; pixel/palette extents and displaced-base-transfer rejection checked. Unsupported raster/version/transfer storage fails explicitly.
- [x] Primary material surfaces and optional dictionary liveries integrated into scan/render. Name lookup follows RenderWare case-insensitive semantics; same-name images stay inside the explicit source/scene. No external or neighboring-bank guess. Variant 0 is a preview default, not runtime configuration proof.
- [x] All bounded native TXD image previews exposed (no inherited 128-image truncation for this reader); encoded previews read only the declared native record, so early textures in CARS >32 MiB can open.
- [x] Material-specific registry identities, immutable source/patch session reuse, storage-aware image hashes (normalized TEX0 + sampler + payload) and shared decoded-image cache. Document changes replace cache; stale source/scope/texture receipts rejected.
- [x] Synthetic scope isolation, mixed-case names, two-material triangle partition, alternate liveries, encoded preview, cache/patch invalidation, GLB/FBX export and malformed data checks pass. Real default surfaces preserve original recovered triangle totals.
- [x] Reader 45/0; full window 111/0 (82 optional skipped); focused TXD/new readers 11/0/no skips; additional Genki real context/WM/isolation/March 7/0/no skips. Logs: build/windows-local/rw-txd-{reader,window,focused}-tests.txt, genki-track-context-tests.txt. git diff --check pending final build.

Measured primary coverage (triangles, not proof of shader fidelity): A31 16,184/16,275 textured, 112/122 default surfaces; HARE_USA 40,047/40,239, 101/106; CARS 88,162/196,987, 100/247. CARS has 417 selectable candidates including optional texture variants; default surfaces still contain exactly 196,987 triangles. ALBACETE 101,577 recovered tris, 135 default surfaces, none textured by this TXD reader. Five bad CARS normals still regenerated.

CARS unresolved references predominantly belong to Selene scenes with no local TXD dictionary (including lower-detail/shared-looking vehicle sets and aquaplaning resources); runtime linkage is not proven and is NOT guessed from nearest offsets or reused names. ALBACETE has no native PS2 TXD records in this sample: its texture storage may be separate/compressed/another layout; absence of TXD does not prove absent textures. Prior ALBACETE triangle discrepancy remains open.

Next owner: user visually tests A31_C00.BIN / GRANDPRIX_HARE_USA.D1 / CARS.MIX via Scan for 3D Data -> Render All. Claude can refine labels/variant grouping while preserving receipts, default-vs-alternate filtering and source scope. Next implementation candidates: PSP GIM and NFS TPK; remaining MIX bank associations, Selene placement, shader/blend parameters, exact runtime liveries and mip chains remain visibly pending. No commit/push/release; no game assets added. Application final rebuild underway (build/review-fixes-app-build.log).

Final application build completed successfully and development binary_workbench.exe updated (2026-10-26 16:57 local; build/review-fixes-app-build.log). git diff --check passed. No commit/push/release. Automated primary-image coverage and remaining resource gaps are recorded above; user desktop verification remains pending. Safe first visual case: study cases/d1gp/A31_C00.BIN -> Scan for 3D Data -> Render All. Avoid interpreting alternate texture variants as additional geometry.

## Initial D Special Stage native reader - 2026-10-26

Owner: Claude. Uncommitted; no push/release. Codex/ChatGPT work preserved (fresh copies patched).

- [x] Codec solved from the owner's `SLPM_652.68` (read-only MIPS analysis; see `docs/research/initial-d-ss/README.md`). Independent reimplementation, no game code copied.
- [x] `src/idss_core.h` (Qt-free): PAC entries, LZ codec, SMD (VIF strips, W bit 15 = no kick), GIM (T8 GS-swizzled / T4, CLUT shuffle). Offline: AKAGI_DAY.PAC 137 SMD = 216,950 tris (every model matches its header count), 139 GIM decode, 0 failures. Sample textures checked visually (sky, wall, grass).
- [x] `src/idss_model.h` (`native-idss-pac`): records merged per texture across the PAC (world-space geometry), GIM from the same PAC. Wired in `asset_discovery.h` / `mesh_profile_decode.h`; test `initialDssPac` (codec + synthetic PAC/SMD). **Not built/run yet.**
- [ ] Build + window_tests; live check: open COURSE.AFS -> child PAC -> Scan for 3D Data -> Render All.
- [ ] UV V orientation assumed top-left (to confirm visually); car models (CAR.AFS not in samples); colour (V4-8) and alpha/blend flags unused.

## Bizarre handedness, texture-library index, Tokyo review — 2026-10-27

Owner: Claude. Uncommitted; no push/release.

- [x] `src/mesh_profile_decode.h`: `mirrorLeftHanded()` (Z negated, triangle order reversed, copy-on-write)
  applied to the PGR1/2/3/4 and MSR profiles. Owner confirmed visually: car and garage text reads correctly.
- [x] `src/image_workbench.h`: the duplicate check in `addDecodedTexture()` is now indexed (it was a full scan per
  texture per mesh). Tokyo loads faster but still in bursts (owner); about 19 draws/s in the first minute.
  A full profile is pending.
- [x] Report `docs/PGR34-TOKYO-REPORT.md` (handedness, textures, XEX status: encrypted + compressed, load
  pipeline).
- [ ] Handoff worksheet `docs/CHATGPT-WORKSHEET-2026-10-27.md`: Tokyo stall profiling, merge per material,
  incremental GPU upload, scan-panel progress bar, XEX route, settings cleanup (FM1 flip toggles to be confirmed
  with the owner), and earlier Initial D / Super Trucks items.

## Scan panel progress bar and collapsed options — 2026-10-27 (Claude)

- `src/asset_scan_panel.h` (UI only, no decode changes):
  - The progress bar (`scanProgress`) now sits directly under the scan buttons. It is busy during scans and resource opening. During Render All / multi-select batches it is determinate: `done / total · rate/s · ETA m:ss · N failed`.
  - `scanStopBatch` (now labelled "Stop") moved next to the bar and is visible only while a batch runs.
  - "Texture options…", the track-texture label and the three scan checkboxes moved into a collapsed "Scan options" section (`scanOptionsToggle` / `scanOptions`). The toggle text summarizes the texture mode.
  - The long intro text moved into the status tooltip. All existing objectNames and status messages are unchanged.
- Verified live: the PGR4 garage (101 parts), the busy bar on the Tokyo scan, Tokyo Render All showing `271 / 11785 · 23/s · ETA 8:20`, Stop, and expanding/collapsing the options.
- Not run: the window_tests suite. Render All and stop tests only use objectNames and status text, which are unchanged.
- Observed, not fixed (ChatGPT worksheet section 0): the GUI is unresponsive for a few seconds right after Render All starts on Tokyo. Steady rate is about 23 parts/s, so the full city takes about 8.5 min sequentially.
