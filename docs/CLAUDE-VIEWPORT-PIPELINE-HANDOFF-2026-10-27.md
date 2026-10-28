# Claude handoff: mesh loading and camera controls

Updated: 2026-10-27. Owner: Codex for this measured slice; Claude/native owner for next GPU/GUI work. No commit/push/release requested. Preserve other uncommitted work.

## Implemented

- `mesh_load_trace.h`: opt-in `WORKBENCH_MESH_TRACE` JSONL profiling, shared sample per request, thread-local worker stage binding. `main.cpp` instruments decode, scene, library, registry and notification intervals; `pgr34_track.h` adds the nested texture stage. Texture phase is part of decode. GUI finish writes the log. Use a new local filename each run; data append, not overwrite. Negative dispatch-gap values with prefetch indicate overlap.
- `asset_scan_panel.h`: only an all-native-pgr34-track batch uses two in-flight requests. Each has a unique `_scanRequest` token stripped before recipe identity/decode. Other profiles remain serial. Out-of-order completions work; duplicates/stale tokens are ignored. Stop/cancel/budget errors drop pending work and drain outstanding requests before ending MeshScene batching. `VulkanPanel::meshCalculationFinished` now carries optional third request ID (default zero); old two-argument emit sites remain source-compatible.
- `main.cpp`: active native-preview count keeps Render disabled until all jobs finish. Prefetch bypass is valid only inside a scene batch with a native job already active; it must not bypass an unrelated manual decoder. RAII releases the active count on every result/error path. **Never use mutating `profile["name"]` just to trace the name:** empty ARCP/Genki recipes must remain empty; use `value()`.
- `MeshScene`: QImage cache-key reference counts and unique byte total replace a scan of every entry for each add/setTexture budget check. Remove, replacement texture and clear update the accounting. Existing budgets remain enforced, shared images counted once. `retainedTextureBytes()` is read-only diagnostics. Geometry replacement retains its previous texture until explicit setTexture, as before.
- Vulkan `upload()`: monotonically advances sorted face/vertex spans and caches material/texture entry per face span. `geometrySpanLookups` reports face-span lookups. Does **not** implement incremental buffers: geometry/visibility changes still rebuild and upload the full buffer.
- `viewport_camera.h` and Vulkan camera inspector: Perspective/View dropdown > **Camera / Clipping…**, nonmodal singleton per viewport. FOV 10–120 degrees; exact numeric input plus slider. Near/Far use original model units; logarithmic slider covers 1e-6…1e12. Automatic disables manual fields; invalid Near>=Far keeps the last valid range. Shared setter emits cameraChanged to synchronize the inspector and existing Display settings. FOV affects perspective projection and frame fitting; orthographic zoom retains previous behavior. Newly created/tiled views inherit FOV and clipping; independent views remain independently adjustable. Settings are per session, not saved global preferences.
- Automatic perspective Near is refined using center-ray surface distance. `MeshDebug::hit` has an optional float distance output (existing callers still receive the original face ID). Do not confuse face index with distance. Ray result cached for unchanged camera/scene; BVH is reused. Orthographic automatic signed planes remain unchanged. Wheel and Shift+wheel fine zoom preserved.

## Verified

- Actual Tokyo PAK + stream2 Qt/MainWindow scan → Render All → prepared scene: **190.805 → 104.052 seconds (-45.5%)**, one run each; filesystem/cache warmth not isolated. 11,785 meshes, 2,333,476 vertices, 1,297,789 triangles, 10,097 textured instances, 5,498 shared image identities, zero failures, before/after per-candidate metadata identical. Curve/phase interpretation: PGR34-TOKYO-REPORT.md. This timing is **offscreen CPU/Qt**, not a live GPU benchmark.
- Live RTX4060Ti Vulkan: full Tokyo city visibly textured, 11,785 visible entries, exact vertex/triangle totals; Render complete 11,784 new +1 already loaded, 0 failed/omitted. Existing 1,704 texture warnings remain (not the missing-base-map count). FOV changed live while loading, Near/Far sliders updated values, small Far clipped the scene and Automatic restored it. Private evidence `build/tokyo-live-pipeline.png`. Interactive live run concurrent with compilation/tests; no controlled live time comparison. The live path remains slower than offscreen.
- Focused tests: cameraInspectorControls, viewportCloseInspection, taggedPgrBatchDrainsBeforeFinishing, sharedTextureAccountingSurvivesReplacement, renderAllContinuesAfterFailure. Focused 7/0/no skips (including setup/cleanup).
- Final complete suites: **window 118 passed /0 failed /85 optional corpus skipped; reader45/0/no skips**. Full suite includes ARCP scan actions, Genki profile isolation, UV canvas and C++ scripting. Logs `build/windows-local/pipeline-{focused,full-window,reader}-tests.txt`.
- Dev executable rebuilt/relinked successfully after the user closed the live test: `build/windows-local/binary_workbench.exe`; final build `build/pipeline-final-build.log`. No release/export publication.

## Rebuild caution

Localized MSVC include dependencies have left stale consumers after header layout/signature changes. Recompile every MeshScene/VulkanPanel consumer, including mesh_collection, mesh_controls, UV panel, script panel and export code, plus main/tests and MOC. Stale objects caused a collection/UV crash during intermediate tests; final rebuilt suite passes. Run tests inside vcvars64 environment so scripting can find the standard C++ headers. Do not report an intermediate crash or missing compiler include path as a new format-decoder defect.

## Next work (still open)

1. Profile the **interactive** GUI repaint/result delivery and frame scheduling. Surface timer is 16ms and actual GPU work/event scheduling differs materially from offscreen; the 104-second test does not prove a 104-second interactive load.
2. Incremental/per-entry GPU geometry ranges and texture updates; avoid whole-scene rebuilds on visibility changes. Keep picking/spans/source offsets and cancellation coherent.
3. Optional static draw merging by material (never mix base maps); retain faceSources/per-draw candidates, transforms and source bindings. City-block filtering is another independent option.
4. Retain Claude's handedness conversion, image-library index, scan progress and collapsed options. Texture/shader-layer recovery, InitialDSS effects and remaining format work are separate tasks. Do not revisit completed Akagi UV/swizzle/MIX texture fixes as if still missing.
5. If requested later, persist camera defaults/settings; current controls deliberately apply to the current viewport/session. Manual extremely wide clip ranges lose depth precision. Center-ray refinement is not a full frustum-wide proximity solver.

Always update COORDINATION-PLAN.md and the worksheet with changes, evidence and remaining work in the same operation.
