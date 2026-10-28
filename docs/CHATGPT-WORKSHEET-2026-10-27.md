# Worksheet for ChatGPT/Codex: open tasks from the Claude session (2026-10-27)

Read `AGENTS.md`, `docs/COORDINATION-PLAN.md` and `docs/PGR34-TOKYO-REPORT.md` first. Preserve all uncommitted work.
No commit/push/release unless the owner asks.

The owner wants direct communication:
- Report exactly what was verified (and how) and what was not.
- Never present an untested change as fixed.
- When something cannot be solved, say so and propose an alternative method.

Verify every user-visible change in the live app.

## Done in this session (verify, don't redo)

- **Bizarre handedness:** `mesh_profile_decode.h::mirrorLeftHanded()` negates Z and reverses the triangle order for
  PGR1/2/3/4 and MSR profiles. **Owner confirmed visually:** text on the 288 GTO and in the PGR4 garage reads
  correctly, and the conversion is right.
- **Texture library index:** `image_workbench.h::addDecodedTexture()` used to scan every library entry for each
  texture of each loaded mesh, which made the batch quadratic. It now uses a `QMultiHash` keyed by
  name+source and rebuilt when the library size changes.
  - **Owner observation after the change:** Tokyo loads faster but still in bursts ("loads a few, stops, loads a
    few").
  - **Measured:** about 19 draws/s in the first minute (10 → 1,038 in about 54 s). Not yet compared over the whole
    run.

## 1. Tokyo (PGR3, 11,785 draws) load stalls: profile first, then fix

File: `F:\copia de seguridad 08.04.2019\downloads\pgr3\Game\Areas\Tokyo_sunny.pak` (+ `.stream2`).

1. Instrument the Render All batch loop (`asset_scan_panel.h` batch + the `main.cpp` result lambda):
   - per-draw time split into decode (worker), texture decode, `scene->add`, image-library add (including the
     64×64 thumbnail `QPixmap` in `ImageWorkbench::append`) and registry;
   - time between results (dispatch gaps).
   Find the stall pattern. Likely suspects:
   - **Texture decode on the UI thread:** the first use of each shared map.
   - **Thumbnail creation:** `image.scaled(64,64)` for every new library image, on the GUI thread.
   - **Worker queueing:** a fixed small number of in-flight decodes, then waiting for the UI to drain.
   - **PGR34 session cache:** lock contention or eviction under the asset budget (texture cache = ¼ of the budget).
2. Fix by measurement, not by guessing. Candidates:
   - decode textures and thumbnails off the GUI thread;
   - create thumbnails lazily;
   - keep N decodes in flight;
   - batch `scene->add` of several results per event-loop turn.
3. **Structural (largest gain):** offer "merge static draws per material" for city-scale PGR3/4 scenes. Tokyo would
   go from about 11.8k entries to roughly the number of distinct base maps.
   - Keep per-draw candidates available for inspection.
   - Merged output must keep each draw's source offsets (`faceSources`) and must not mix textures.
4. **Viewer:** `VulkanPanel` rebuilds and uploads the whole vertex array on every geometry change, including
   visibility toggles. Move to per-entry buffer ranges or incremental updates.
5. **Optional:** load by city block (`block_XX` names are in the candidate names), so the owner can load only
   Shinjuku.

Acceptance: report Tokyo total load time and the time-per-draw curve before and after, plus the final
draw/triangle/texture counts (11,785 draws, 10,097 with base maps at the last count).

## 2. Scan panel UX (owner request)

- Move the "Texture options…" button and the long status/notes text out of the top of the 3D Data Scan panel
  (for example into a collapsible "Details" section or the settings).
- Put a **progress bar** there for scan and Render All: current/total, rate, ETA, and a cancel button.
  - The existing label "Loading selected result X / Y" becomes the bar text.
  - Keep accessibility names, because tests use `objectName`s.
- Update window tests that look for the moved widgets.

## 3. PGR3/PGR4 executables (owner: try a different unpacking route)

- `pgr3/default.xex` (6.0 MB) and `PGR4/default.xex` (12.4 MB) are XEX2 with encryption = 1 (AES) and compression
  = 1 (basic), so nothing is readable as shipped.
- **Goal:** the material/shader semantics for the 1,536 layered Tokyo materials and the car livery layering (the
  288 GTO side shows atlas fragments).
- **Route:**
  - The owner decrypts/decompresses locally with a community tool (e.g. XexTool). Do not commit keys or decrypted
    binaries.
  - Then use static PowerPC analysis (Ghidra with an Xbox 360 loader) to find the material binding code and the
    embedded Xenos shader microcode.
- **Before that:** check the data-side material records in the scene PAK/STREAM2 (layer count, slots, parameters)
  across the 1,536 draws. They may explain the blend without the executable.

## 4. Settings cleanup (owner: remove obsolete options)

- The owner remembers a settings option made redundant by the Bizarre handedness fix. The only import options in
  `settings_dialog.h` are **"Flip U / Flip V for FM1 track UVs"** (`ImportPreferences::fm1FlipU/V`). FM1 is
  Turn 10, not Bizarre, so they are not obviously redundant.
- Confirm with the owner which option was meant. If it is FM1, first verify FM1 track handedness and UVs with
  readable texture text; only then remove the toggles and hard-code the verified orientation.
- Also check `mesh_controls.cpp` / mesh profiles for per-profile axis negate options that duplicate
  `mirrorLeftHanded`.

## 5. Other pending items (from earlier handoffs)

- **Initial D SS:**
  - source vertex colours (V4-8) and fog/blend;
  - the single texture warning;
  - cache PAC parsing per document revision (it currently reparses per material).
  - See `docs/CLAUDE-INITIAL-D-SS-HANDOFF.md`.
- **Super Trucks car-part placement:**
  - Each Selene object ends with an 80-byte node: u32 0x50, 8-byte id, flags, u32, then a 3×3 rotation at +20, a
    translation at +56, and `0x10/0/2, 3` at +68/+72.
  - Wheels are at the origin; they are placed at runtime.
  - Verify against `@HIERARCHY_DATA@` parents before applying.
- **Super Trucks ALBACETE:** texture source and the triangle-count discrepancy (101,577 vs 128,409).
- **Remaining texture families:** PSP GIM (GMO), NFS MW TPK, external WM PS3 / ITC banks.

## Validation rules

- Rebuild `windows-local`. For header-only changes, make sure `main.cpp` and the tests recompile; ninja has missed
  header changes before.
- Run only the focused tests for the touched area, plus a live check. Run the full suite only after shared
  changes, such as the scan panel, scene or image library.

## Codex continuation — 2026-10-27 (validation in progress)

See the current Tokyo pipeline + camera entry in COORDINATION-PLAN.md. Baseline is now measured end-to-end through the Qt UI callback path: 190.805 s, 11,785 meshes, 10,097 textured instances, 5,498 unique image identities; optional JSONL captures per-draw phases. The last 500 draws averaged 4.29 ms worker decode (3.89 ms nested texture phase), 0.665 ms scene insertion and 0.050 ms image-library insertion. The worker-to-GUI interval averaged 9.55 ms. Texture work occurs in the worker in this path; no evidence warrants moving thumbnails now.

Under test: two tagged in-flight PGR3/4 track jobs, texture refcount/budget lookup, linear GPU span binding, FOV and clipping inspector. Existing progress UI/collapsed scan options and handedness fix retained. Incremental GPU buffers and per-material static merging remain pending. Updated results will follow after validation; offscreen timing must not be called a live Vulkan benchmark.

Measured after: **104.052 s vs 190.805 s (-45.5%)** in the offscreen MainWindow pipeline. Both produce exactly 11,785 meshes / 1,297,789 triangles / 2,333,476 vertices / 10,097 textured instances / 5,498 shared image identities; zero failures. Full curve and profiling interpretation: docs/PGR34-TOKYO-REPORT.md. Focused regressions 7/0/no skips. Live GPU validation and full shared-suite receipt still pending at this point.

Earlier worksheet ALBACETE texture/triangle discrepancy is superseded by the native MIX/TXD work documented in COORDINATION-PLAN.md (all alternatives 128,409 triangles; default dry 84.35% textured triangles). Super Trucks placements remain open. Likewise Akagi masked UV/strip parity/GIM swizzle are already fixed; source colors/effects remain pending.

### Final continuation receipt

- [x] Measured and reduced Tokyo CPU/Qt pipeline time; actual per-candidate metadata matches before/after. Full curves in PGR34-TOKYO-REPORT.md.
- [x] Bounded tagged prefetch, stop/drain and shared-texture accounting validated. Profile-name tracing must use `value()` to preserve empty ARCP/Genki recipes; full suite catches this.
- [x] Viewport camera inspector live-verified: use **Perspective/View dropdown > Camera / Clipping…**. FOV and logarithmic Near/Far sliders, exact model units, Automatic toggle. Original Display > Settings clipping remains synchronized via the common setter. FOV applies only to perspective and framing. New/tiled views inherit values. Camera settings are per viewport/session, not a persistent global preference.
- [x] Live Tokyo Render All finished, 11,785 visible meshes / 2,333,476 vertices / 1,297,789 triangles, zero failed/omitted. City textured in Vulkan; known material warnings retained. FOV and clipping controls remained responsive during loading. Interactive live load is slower than offscreen; do not advertise the 104-second test as live GPU timing.
- [x] Full window tests **118/0**, 85 optional corpus skips; reader **45/0**, no skips. Initial partial runs caught stale header objects and the empty-profile trace regression; final rebuilt suite passes. Run scripting tests within vcvars64 environment.
- [ ] Next performance slice: profile interactive GUI repaint/event delivery and GPU frame/upload stalls; implement incremental entry buffers, optional per-material static merging retaining faceSources, or block loading. Full upload still rebuilds on geometry changes. Decoder/material shader work and remaining worksheet families are separate pending tasks.

Handoff details: docs/CLAUDE-VIEWPORT-PIPELINE-HANDOFF-2026-10-27.md. Do not redo Claude's handedness/library/progress changes or the completed Akagi/MIX decoder fixes.

## PGR3/4 executable audit update — 2026-10-27

The earlier owner-unpacking blocker is superseded: Codex locally decrypted/basic-expanded both default.xex files and exported validated static-analysis images. 330/994 embedded shader constant tables recovered. Regular diffuse s0, PGR3 road base s8, PGR4 Gotham4 road base s5 and legacy road s8; distinct atlas/overlay/specular/normal/light/wear roles. **Do not equate these GPU register IDs with source SCNE channel IDs.** Current generic base previews are not proof of faithful multilayer rendering; car reference0-as-decal preview still approximates diffuse/overlay composition. No assignment code changed. Next: prove per-family serializer/CPU/shader binding with selected draws, then add atlas/layer rendering. Details, hashes, artifacts, confidence and pending work: docs/research/pgr34-executables/README.md; shared coordination updated. No binaries, keys or original shader code published.

## PGR3/4 material-source integration — 2026-10-27

- [x] Track raw fields/layer graph and vehicle source references preserved; Material > Source maps… inspector added. Explicit map preview updates the chosen mesh (transforms retained, duplicate copies distinguished); Automatic restores default. Source roles remain unresolved unless proven. Current UVs retained; manual inspection unlit.
- [x] Actual material corpus8/0/no skips, full window120/0 (86 optional skips), reader45/0/no skips. Native Tokyo Sunny unchanged11,785 loaded/10,097 textured/0 failures. Live Ferrari195 meshes/118,377 triangles, source texture33→9→33 verified without duplicates. Dev exe updated/open; no release.
- [ ] This is the evidence/inspection slice only: automatic texture coverage and original layered rendering are unchanged. Need source declaration→CPU/shader correspondence and atlas/UV/blend constants before automatic material corrections. Remaining1,536 layered Tokyo composites are not solved by the new inspector.

Implementation and reproduction: docs/CLAUDE-PGR34-MATERIAL-SOURCES-HANDOFF.md; research details and final receipts: docs/research/pgr34-executables/README.md. Preserve Claude's handedness and the completed viewport-load changes.
