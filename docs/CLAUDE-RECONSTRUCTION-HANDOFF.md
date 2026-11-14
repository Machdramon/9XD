# Claude hand-off: reconstruction inspectors

Updated: 2026-10-28. Original inspector UI by Claude; YouTube ingestion and sparse/surface increments by Codex are built, tested and installed locally. See the current section below and COORDINATION-PLAN.md; older preparation status is historical.

## Real-video initialization failure - 2026-10-28 (tested and installed)

- [x] Diagnosed owner log and actual five-frame job: extraction/matching succeeded; mapper found no initial pair. All 8 verified pair records were configuration6 (planar/panoramic), with 3,310-4,642 correspondences. Direct first/last-frame inspection shows a stationary vehicle at a red light, with moving traffic; no useful static-scene camera baseline. Not a CUDA problem. The original job was subsequently replaced by the owner during testing; retained observations are recorded here, not raw media.
- [x] Corrected stage-specific errors in src/reconstruction_job.cpp: mapper initialization failures explain camera translation/parallax and selecting a moving continuous clip; CUDA guidance only accompanies PatchMatch errors. No relaxation that invents depth from stationary images. Saved result/checkpoint preservation retained.
- [x] Fixed Load Job thumbnail accumulation; regression exercises repeated restore plus mapper nonzero-exit diagnostic, preserved logs/prior cloud and generic feature-stage failure without CUDA advice.
- [x] Focused11/0/no skips; full138 passed/0 failed/87 optional skipped (build/windows-local/sfm-diagnostics-full-correct-env.txt). First full invocation omitted vcvars and corpus roots:128/2/95, with two C++ scripting failures from missing compiler environment; rerun with the normal developer environment passed. Application linked successfully (build/sfm-diagnostics-install.log), updated after owner closure and reopened.
- [x] Real owner-video clip40-55s at2fps,30 frames,640x360: first solve12/30 cameras,460 points, mean0.4516px, CPU coarse538 faces. Independent repeat recovered30/30,1,255 points, mean0.4696px,2 components (largest selected), then actual CUDA dense/fusion/Poisson surface6,748 faces. Both native-controller tests3/0/no skips; dense run201.7s (sfm-valencia-{real,dense}.txt). This variation demonstrates initialization sensitivity; counts are evidence, not full-scene/texture quality certification.
- [x] Private Load Job checkpoint prepared at build/reconstruction-valencia/moving-40-55/job.json, now pointing to the successful dense result. Live updated app restored the first checkpoint and imported460 sparse points into Vulkan; owner pressed physical Escape before the second result could be displayed, so all further computer input stopped. New dense result is saved but has not been visually reviewed in the app.
- [ ] Owner: use40-55s/2fps or Load Job above, then Show Surface in3D; evaluate partial reconstruction. Codex/Claude: adaptive segment selection/parallax diagnostics, dynamic-object/dashboard masks and more deterministic initialization remain future work. Private source/diagnostics stay ignored; do not publish user video. No commit/push/release.

## Assignment

Define and refine the UI/interaction for the new Video Reconstruction and Image / Spline Modeling inspectors. Keep the functional backend intact. The user explicitly reserved visual design for Claude. Read AGENTS.md, CLAUDE.md, docs/COORDINATION-PLAN.md and docs/reconstruction-tools.md first.

## Starting point

Actual repository: C:/Users/Jarky/Documents/GitHub/binary-workbench. Current changes are uncommitted on main, including unrelated Genki material fixes; preserve them. Do not replace current files with an older workspace mirror. No commit, push or release is authorized by this hand-off alone.

- src/reconstruction_inspectors.h: Reconstruction::VideoInspector and Reconstruction::SplineInspector (QWidget subclasses, no Q_OBJECT).
- src/main.cpp: creates document-session docks and checks active jobs/unsaved projects before document replacement or close.
- src/module_menus.h: both docks belong to the Reconstruction module (moved out of 3D on 2026-10-26) and appear in its menu.
- tests/window_tests.cpp: reconstructionInspectorContracts and reconstructionFrameJob.

## UI work, in order

1. Fit both inspectors into the established theme, compact grouped controls and existing docking behavior; avoid a second viewer or large text-heavy toolbars. Keep UI text English.
2. Video: clearly distinguish input, output destination, sampling budget, action/status and bounded log. Make Extract Frames/Cancel states obvious. Keep Frames / Sparse Points / Surface distinct: the current COLMAP backend is implemented and tested; frames alone are not a reconstructed model.
3. Image/spline: improve reference-image selection, curve/point organization and save/load feedback. Plan image-plane/graphical curve editing against the existing UV/2D and 3D workspaces. A reference preview or drawing editor is additional functionality; maintain pixel-coordinate ordering and persistence if implementing it.
4. Preserve app themes/accent tokens and accessible labels. Keep cross-platform presentation in mind; do not merge unrelated Mac build settings.

## Functional contracts to preserve

- videoReconstructionDock / videoReconstructionInspector, splineModelingDock / splineModelingInspector. Session ownership, Reconstruction-module docking (3D until 2026-10-26), detachable module behavior and saved layout use the existing infrastructure.
- Video uses asynchronous QProcess directly with FFmpeg, unique UUID job folders, bounded frame count/resolution/logs and atomic job.json. Cancel retains partial output; document close/replacement is blocked while extraction is active. Never overwrite existing job output.
- Current sampling ranges: 0.05–10 fps, 2–10000 frames, maximum image dimension 256–4096; defaults 1 fps, 300 frames, 1600 pixels.
- Spline JSON: schema 1, type 9xd-spline-project, image path and ordered points with curve/x/y. Reject invalid/nonfinite values and malformed loads before replacing existing state. Preserve unsaved-change confirmation.
- Named controls used by tests: reconstructionVideo, reconstructionOutput, reconstructionFfmpeg, reconstructionFps, reconstructionFrameLimit, reconstructionExtract, reconstructionStatus, splineReferenceImage, splineControlPoints. Preserve them or migrate tests deliberately.

## Prepared dependencies and limits

build/reconstruction-tools contains verified FFmpeg 9.0.2 and COLMAP 4.2.1 no-CUDA, plus Open3D 0.19.0/ONNX Runtime 1.23.2 CPython 3.12 core wheels. receipt.json records URLs/hashes. They are local and Git-ignored. Complete Python dependencies and SAM model weights are not installed. SAM ONNX mask decoder alone is insufficient: image embeddings require a separate compatible encoder.

Still pending, not claimed complete: COLMAP sparse job execution, camera/point import, shared Settings dependency paths, calibrated spline fitting/lofting, segmentation, full surface reconstruction. Coordinate scope additions through the shared plan; avoid misleading enabled controls for unavailable operations.

## Validation and required follow-up

Application builds with windows-local. Focused tests: 4 passed/0 failed/0 skipped; full window suite: 98 passed/0 failed/75 optional skipped. Use the VS2022 vcvars64 environment for existing C++ scripting tests and Qt 6.11.2 runtime PATH. Logs are in build/windows-local/reconstruction-inspectors-tests.txt and reconstruction-window-full.txt; build log is build/reconstruction-inspectors-build.log.

Re-run focused tests after changes, relevant menu/docking regression, and visually inspect the refined panels. Record actual results, changed files, remaining tasks and ownership in docs/COORDINATION-PLAN.md and docs/reconstruction-tools.md in the same session before reporting completion. Mark this hand-off picked up when work begins; do not mark it accepted merely because it exists.


## Separate scanner UI follow-up — 2026-10-26

Genki track context controls were added to File/3D Data Scan: scanTrackContext / scanClearTrackContext, public setTrackTextureContext. Functional behavior and schema: docs/research/genki-track-banks/README.md. Claude may refine appearance later while preserving explicit per-document context, fingerprints and material refresh without duplicates; not a reconstruction-inspector feature.


## Automatic track context update — 2026-10-26

Verified TXR3 global/local contexts now load automatically from embedded profiles and validated companion locations. Manual JSON loading is no longer normal setup. Scan controls default to automatic; Texture options… remains an advanced override. Remembered contexts allow matching study-case copies. Real automatic tests and GLB/FBX pass; source/texture fingerprints and no-duplicate-material refresh remain required. Other game maps remain pending. See docs/research/genki-track-banks/README.md.

## YouTube ingestion handoff - 2026-10-28

Preserve the existing inspector contracts plus these additions when refining the UI:

- Source selector reconstructionSourceKind: Local file / YouTube URL. reconstructionYoutubeUrl and reconstructionInspectLink inspect one public HTTPS video. reconstructionDownloadResolution caps height, reconstructionDownloadMiB caps storage, reconstructionKeepVideo retains the source. reconstructionClipStart / reconstructionClipEnd are seconds, End=0 means the end; used for local and downloaded sampling. The whole selected stream downloads before trimming, not only the fragment.
- Tools paths reconstructionFfmpeg / reconstructionYtdlp / reconstructionDeno. Optional official executables are staged by tools/prepare_youtube_tools.py with GitHub SHA256 receipts; no global PATH or release changes. Editing visible paths remembers preferences. Keep external tools independently updateable.
- src/youtube_video_source.h/.cpp owns the asynchronous metadata/download controller, bounded structured output, errors, timeouts and cancel (Windows Job Object when supported). Do not put Windows headers back into the public header: near/far/FAR macros collided with theme and zlib code during initial compilation.
- Inspector manages per-job temporary disk cache, source retention, failed partial/oversized-file cleanup and FFmpeg sampling. Existing job.json gains sourceType, canonical URL/curated metadata, clip bounds, budgets, tool paths and cleanup result. Failed inspection must not overwrite a previous successful manifest. Do not persist raw metadata, signed media URLs, credentials or browser cookies. No direct whole-video RAM mode.
- tests/video_source_stub.cpp is only built with tests; it is not a shipped downloader. youtubeSourceContracts, youtubeDownloadJobs and youtubeInspectorImport cover errors/cancel/budgets/cleanup and a real FFmpeg clip; existing local extraction/drawing contracts remain. Focused8/0/no skips, full135/0/86 optional skips. Separate real upstream metadata/download smoke succeeded; owner link still to test.

Local application rebuilt and UI visually checked; left open on Video Reconstruction / YouTube URL with tools detected. Public0.4.0 remains unchanged; no commit/push/release performed for this increment. Next: owner tests the desired URL, Claude may refine presentation while keeping these states and names. Sparse reconstruction, import of points/cameras and dense surfaces remain unavailable.

## Sparse points and surface handoff - 2026-10-28

The sparse/3D pipeline is implemented in src/reconstruction_job.h/.cpp and VideoInspector, with MainWindow's importToScene callback routing to the document's existing Vulkan scene. Preserve the previous YouTube increment. The pipeline has three real stages (Frames / Sparse points / 3D surface), plus explicit result import buttons. No AI is required.

Functional controls: reconstructionColmap, reconstructionMatching, reconstructionSharedCamera, reconstructionThreads, reconstructionSurfaceMode, reconstructionDenseSize, reconstructionCacheGiB, reconstructionMeshingDepth, reconstructionSurfaceTrim, reconstructionImportLimit. Actions: reconstructionLoadJob, reconstructionBuildSparse, reconstructionBuildSurface, reconstructionImportPoints, reconstructionImportCameras, reconstructionImportSurface; quality label reconstructionSummary. Existing frame/source/cancel object names remain unchanged.

Please refine presentation without changing these guarantees: at least3 registered cameras/20 points, largest-component disclosure, matching TXT/PLY counts, bounded asynchronous parsing, unique attempt folders, previous successful checkpoint preserved on error/cancel, stage-boundary Load Job resume, and disabled conflicting actions while busy. Camera centers are a separate helper; poses/calibration stay in saved outputs. No arbitrary faces are guessed from a point cloud: the CPU option is explicitly coarse sparse/Delaunay; dense uses CUDA depth/fusion/Poisson. Mesh scale is arbitrary and preview has no photo texture atlas/per-vertex RGB yet.

Important discovered behavior: upstream Poisson trim10 at depth7 yielded an empty mesh even with47,105 fused points; explicit trim4 generated18,881 vertices/37,193 faces. Keep Surface trim visible/accessible with clear empty-output diagnostics. Cache GiB is a COLMAP cache setting, not a hard total-memory guarantee. Do not label a low-poly coarse surface as dense reconstruction or merge disconnected components automatically.

COLMAP runs as a separate process with its own Qt DLL/plugin paths. CPU sparse mode is portable; dense requires a CUDA-enabled executable/driver. tools/prepare_colmap_cuda.py stages a pinned checksum-verified optional CUDA build under ignored build/reconstruction-tools. No binaries/game assets enter Git. Tests: parser validation, pipeline helper, inspector continuation and actual scene import; separate real CPU/CUDA fixture check and full regression receipts in the shared plan. No commit/push/release authorized by this handoff.

Next work: texture atlas baking/material assignment, calibration/scale, camera frustums, optional masks/dynamic content filtering and UI refinement. Video reconstruction is a native offline numerical workflow after ingestion; Ollama should not invent points/faces or replace geometry validation.


Completion receipt2026-10-28: focused11/0/no skips; full138/0/87 optional skips; real COLMAP CPU/CUDA pipeline3/0/no skips. Final procedural scene12 cameras/586 points; coarse764 faces, dense39,621 faces (20,135 vertices), mean reprojection error0.3947px. Load Job and actual surface import/render verified live; local EXE updated, public release unchanged. UI follow-up: compact/collapse the reconstruction settings and consider blocking unfocused spinbox wheel edits; scrolling over Surface trim changed it during review and it was restored to4. Keep strict separation between source frames, sparse evidence, coarse and dense results. Owner's actual-video quality check remains next.
