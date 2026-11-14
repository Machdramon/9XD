# Reconstruction tools

Updated: 2026-10-28. Owner: ChatGPT/Codex; shared integration tracking in COORDINATION-PLAN.md.

Both tools are approved for development. Later dated implementation sections supersede historical preparation notes; local changes are not automatically part of a published package.

## Video to 3D

Pipeline: inspect video with ffprobe → sample frames with FFmpeg → feature matching and sparse reconstruction with COLMAP → confidence filtering and voxel downsampling → import into the existing 3D workspace. Preserve camera poses, input timestamps, tool versions and processing parameters in a per-job manifest.

Start with sparse points and cameras. A dense mesh and textured surfaces are later milestones, not guaranteed outputs of sparse reconstruction. Repeated textures, reflective vehicles, moving objects and nearly straight camera motion can make reconstruction ambiguous. Real scale requires a known measurement or calibration.

Modes: linear/dashcam uses sequential matching with loop checks; orbit/drone starts with sequential matching and adds wider candidates. Frame limits, image resolution, CPU threads and memory estimates must be visible before processing. Quality metrics decide whether to continue rather than silently generating speculative geometry.

## Image and spline modeling

Native editable splines, image planes, calibrated cameras and user-placed corresponding points come first. Triangulate corresponding observations, align known points, and offer loft/extrude/revolve operations. Keep generated geometry separate from source curves and allow undo.

Segmentation is an optional mask suggestion, not a geometry decoder. SAM's standard ONNX exporter provides the prompt encoder/mask decoder and requires image embeddings from a separate image encoder. Select and validate the complete encoder/decoder pair before downloading model weights. Ollama Vision may suggest semantic labels or operations; numerical camera fitting, projections, visibility checks and geometry construction remain deterministic. Occluded surfaces are hypotheses requiring user approval.

## Dependency preparation

Run `tools/prepare_reconstruction_tools.py` with Windows x64 Python 3.12. It stages tools under ignored `build/reconstruction-tools`, records download URLs and SHA-256 in receipt.json, validates archive paths and does not modify global PATH or the application's Python runtime.

| Component | First role | Setup |
| --- | --- | --- |
| FFmpeg / ffprobe | Video inspection and frame sampling | Essentials ZIP from the provider linked by ffmpeg.org, official provider GitHub mirror SHA-256 verified |
| COLMAP | Feature matching, sparse points and cameras | Official Windows no-CUDA release with GitHub SHA-256; CPU baseline first |
| Open3D 0.19.0 | Point filtering, voxel reduction, later surface processing | Core Python 3.12 wheel staged, dependencies not installed yet |
| ONNX Runtime 1.23.2 | Later mask inference | Core wheel staged, model and dependency setup pending |
| SAM image encoder + mask decoder | Optional assisted selections | Pending validated model choice/export; no weights downloaded |

COLMAP CUDA is a later optional acceleration path for compatible NVIDIA hardware; it is not required for the baseline sparse workflow. Dense GPU reconstruction needs a separate capability check. Keep COLMAP's runtime/DLL directory isolated from 9XD's Qt and use the supplied launcher environment when necessary.

No binaries or model weights enter Git or the release package in this preparation step. Review upstream notices and distribution obligations before bundling components in a release.

## Implementation checklist

- [x] Confirm both tools with the user and document scope.
- [x] Add repeatable download/checksum preparation script.
- [x] Verify COLMAP 4.2.1 help and FFmpeg 9.0.2 version; generate a 3-second video and extract 3 bounded frames through ffprobe/FFmpeg.
- [ ] Create isolated Python environment with complete pinned dependency lock and import checks.
- [ ] Add shared asynchronous QProcess job runner: progress, cancellation, bounded logs, output folders, resumable manifests and failure reporting.
- [ ] Add Settings → Reconstruction: executable paths, capability checks and resource budgets.
- [x] Implement frame sampling, sparse COLMAP reconstruction and point/camera-center import; CPU coarse and CUDA dense surface stages added2026-10-28. Full camera frustums, scale calibration and texture baking remain pending.
- [ ] Add image planes, editable splines and landmark alignment; then loft/extrude/revolve.
- [ ] Validate complete SAM encoder/decoder, add optional masks and manual correction.
- [ ] Add fixture jobs, cancellation/failure checks and user documentation before enabling release UI.

References: https://ffmpeg.org/download.html ; https://www.gyan.dev/ffmpeg/builds/ ; https://colmap.github.io/install.html ; https://github.com/colmap/colmap/releases ; https://www.open3d.org/docs/release/getting_started.html ; https://github.com/facebookresearch/segment-anything/blob/main/scripts/export_onnx_model.py

## Preparation receipt — 2026-10-25

COLMAP 4.2.1 Windows no-CUDA (127,731,595 bytes) and FFmpeg 9.0.2 Essentials verified against upstream GitHub asset digests and extracted. FFmpeg provider download stalled; switched to its own official GitHub mirror. Open3D 0.19.0 (69.2 MB) and ONNX Runtime 1.23.2 (13.5 MB) Windows CPython 3.12 core wheels downloaded with local SHA-256 receipts. Complete wheel dependencies/import validation are still pending. Binaries are local under build/reconstruction-tools; receipt.json records resolved URLs and hashes.

First executable backend: tools/reconstruction_frames.py inspects video, validates sampling budgets, refuses existing output folders, caps frame count/resolution and records commands, settings, source metadata, errors and extracted count in job.json. A generated 320×240, 3-second video produced exactly 3 frames, dimensions bounded at 256. This validates extraction plumbing only, not photogrammetric reconstruction. No UI is exposed yet. Cancellation/progress for app integration still needs the shared job runner.

Next: complete isolated dependency environment, then add the cancellable process runner and COLMAP sparse job. In parallel planning terms, spline modeling begins with native image planes/curves and corresponding-point storage; SAM remains optional and requires a complete encoder/decoder decision.
## Inspector integration contract — 2026-10-25

User requested functional inspectors while reserving UI definition/refinement for Claude. New header: src/reconstruction_inspectors.h. MainWindow owns two document-session docks, both scoped to the 3D module through module_menus.h and using existing docking/detachment/layout persistence:

- videoReconstructionDock / videoReconstructionInspector: source video, output parent folder, FFmpeg executable, sampling rate, frame limit and image dimension budget; Extract Frames, Cancel, bounded log and status. QProcess runs asynchronously; each job uses a unique output directory and atomic job.json with parameters, arguments, state and frame count. Incomplete output remains inspectable. Active extraction blocks document replacement/close until canceled. No Python dependency is needed for this native extraction path.
- splineModelingDock / splineModelingInspector: image path and ordered curve control points in pixel coordinates; add/remove, Save Project and Load Project. JSON schema 1/type 9xd-spline-project records image and points (curve/x/y). Loads validate completely before replacing current data; malformed/nonfinite coordinates rejected. Unsaved changes require a discard confirmation before close/document replacement or load.

Claude may replace form/table presentation and add viewport interaction while retaining these object names and persistence contracts or migrating them explicitly. No final visual design is implied. Sparse COLMAP execution, camera/point import, graphical spline editing, calibration, 3D curve fitting and segmentation remain pending. Both inspectors are accessed from the 3D menu. Runtime settings are currently per inspector; shared Settings dependency-path preferences remain pending.

Application and test targets compiled. Focused inspector run: 4 passed/0 failed/0 skipped, including real FFmpeg extraction and cancellation. Existing uncommitted Genki changes preserved.

Inspector test evidence: initial application/test build passed; contract tests passed (round-trip, invalid-coordinate rejection, malformed-load preservation, dock registration), and real native QProcess extraction/cancellation passed against the staged FFmpeg. Rebuilding once more after adding an explicit non-empty output-folder requirement; final regression results follow in the shared plan. This step does not add SAM model weights or require Open3D/Python installation.

Final inspector validation: application rebuilt and updated; focused 4 passed/0 failed; full window regression 98 passed/0 failed/75 optional skipped. First full run lacked the compiler environment for existing C++ scripting tests; corrected environment rerun passed. Diff whitespace check passed. Inspector integration and job/save-load foundations are complete for this increment; remaining checklist items still apply. Access: 3D → Video Reconstruction / Image / Spline Modeling. Claude owns visual and interaction design next.

Claude UI assignment: see docs/CLAUDE-RECONSTRUCTION-HANDOFF.md (2026-10-25), also linked from CLAUDE.md. Ready for pickup; direct session delivery/acceptance not confirmed.

## Claude inspector UI — 2026-10-25

Both inspectors redesigned by Claude; functional contracts above are unchanged. Video Reconstruction shows the pipeline (frames now; sparse points and import shown as unavailable), ffprobe video details, a live frame estimate, FFmpeg tool state, progress, state-coloured status, a collapsible log and result thumbnails. Image / Spline Modeling adds a drawing canvas inside the inspector (click to add, drag to move, Delete to remove, wheel zoom, drag pan), curve picker and colours, reordering within a curve, bounds checks and clearer save/load feedback. The canvas is in the inspector rather than the UV / 2D view because those docks belong to different modules. New test: reconstructionInspectorDrawing. Focused tests pass; see COORDINATION-PLAN.md for full validation and remaining items.

Release 0.3.7 validation (2026-10-25): Claude's UI/drawing refinements compile and pass the full properly configured window suite: 99 passed/0 failed/75 optional skipped, including inspector contracts, drawing, actual FFmpeg extraction/cancel. Optional external reconstruction tools remain excluded from the portable package.

## Module change — 2026-10-26

At the owner's request the inspectors no longer live in the 3D module: they have their own **Reconstruction** module (rail and menu: Reconstruction → Video Reconstruction / Image / Spline Modeling), separate from the 3D binary scanners. Dock names, session ownership, detachment and layout persistence are unchanged; the module id is `reconstruction`. Earlier "3D → …" access notes above are superseded.

## YouTube source integration - 2026-10-28 (tested and installed)

Video Reconstruction now accepts Local file / YouTube URL. Inspect Link reads title, duration and dimensions; Import and Extract Frames runs metadata, bounded video-only download and the existing FFmpeg sampler. Start / End are seconds (End=0 means the end), shared with local-video sampling. The selected stream is downloaded before trimming, not just the requested fragment. Resolution caps downloaded height; Longest side caps extracted frames separately.

Tools exposes FFmpeg, yt-dlp and Deno paths (remembered after editing). Developer setup: run tools/prepare_youtube_tools.py; verified official executables are staged under ignored build/reconstruction-tools/youtube and discovered by the local build. Portable users can select their own executables; the published 0.4.0 ZIP is unchanged and does not contain this increment or the new tools.

Temporary mode uses an owned per-job disk directory, removed after extraction, cancellation or errors. Keep video retains the complete source in the job/source folder; incomplete downloader parts are removed. Download MiB is monitored during transfer and verified at completion; a small overshoot is possible. No direct full-video RAM mode. Existing extracted frames remain available. job.json records canonical source URL, curated metadata, clip bounds, budget, resolution, executable paths, source retention/cleanup and frame state; signed media URLs/raw metadata and browser cookies are not persisted.

The source helper uses argument arrays (no shell), ignores downloader configuration and plugins, allows only canonical HTTPS single-video YouTube links, rejects live/playlist results, bounds metadata/log output and times out stalled jobs. Windows cancellation also closes the downloader's job group when supported, to terminate child JavaScript processes. Site changes or restricted/unavailable videos may require tool updates or a local source; no login/cookie import or access bypass is implemented.

Upstream: https://github.com/yt-dlp/yt-dlp#embedding-yt-dlp and https://github.com/yt-dlp/yt-dlp#dependencies. Deno: https://github.com/denoland/deno/releases. Downloaded tool versions/checksums are recorded locally in build/reconstruction-tools/youtube/receipt.json. This adds video ingestion and frame extraction only; COLMAP reconstruction, point/camera import and dense geometry remain pending.


Validation: focused 8 passed / 0 failed / no skips; full window suite 135 passed / 0 failed / 86 optional fixture skips. The external stub covers metadata errors, live rejection, cancellation, budget enforcement including oversized final files, cleanup, source retention, prior job preservation and failure of frame extraction. Actual FFmpeg extracts the requested two-second fixture clip into two frames. Separate official yt-dlp/Deno online smoke reads Blender video metadata and downloads a bounded 144p stream (4,639,733 bytes) with structured progress; this is separate from the offline inspector tests. Live UI switching, controls and detected tool paths checked in rebuilt local 9XD. Owner-specific link remains to test; the public release was not modified.

## Sparse and surface pipeline - 2026-10-28 (tested and installed)

The previous statements that sparse reconstruction/import are unavailable are historical. Video Reconstruction now provides:

1. Extract Frames (local or YouTube), or Load Job and select an existing job.json with at least three saved frames.
2. Build Sparse Points: CPU SIFT features, sequential video matching (or exhaustive matching for a short orbit), incremental mapping, largest-component selection and TXT/PLY export.
3. Review the registered-frame fraction, point count, component count and mean reprojection error. Show Points in 3D imports the sparse cloud; Show Cameras in 3D imports camera centers as a separate helper point set. Full camera intrinsics, quaternions and translations remain in the COLMAP model and summary.
4. Build 3D Surface: Dense surface (CUDA) runs undistortion, geometric PatchMatch stereo, depth fusion and Poisson meshing; Coarse surface (sparse / CPU) runs visibility-based Delaunay meshing from the sparse model.
5. Show Surface in 3D imports the validated mesh into the normal Vulkan workspace, where existing transforms, inspection and geometry exports apply.

COLMAP4.2.1 is the validated CLI version. Tools exposes its executable; development builds prefer the staged CUDA build when available. tools/prepare_colmap_cuda.py downloads the pinned official Windows CUDA archive, verifies SHA256 and stages it under ignored build/reconstruction-tools/colmap-cuda-4.2.1, without changing global PATH. The original CPU build remains available. The application launches the executable directly and gives its child process its own DLL/plugin paths to avoid mixing COLMAP Qt with 9XD Qt. No COLMAP or CUDA runtime is linked into 9XD itself or added to the published0.4.0 package.

Controls: matching strategy, shared lens/intrinsics, CPU threads, dense image size, stereo cache GiB, Poisson depth/Surface trim, import vertex count (twice that for faces). Sparse SIFT is CPU by default, capped at8192 features per frame; dense depth requires a CUDA build and compatible NVIDIA driver. Cache is not a hard process RAM or VRAM limit. Import parsing has independent hard ceilings of4M vertices/8M faces. ASCII and little-endian binary PLY validate counts, finite coordinates, property types, triangular indices and bounds before touching the scene. File parsing/normal generation and sparse summary validation run off the UI thread.

Surface trim defaults to4: upstream's10 removed the entire mesh at a low test depth despite47,105 fused points. The first real test exposed this; explicit trim4 at depth7 produced18,881 vertices/37,193 faces. Empty output is an actionable failure rather than a successful blank viewport. A lower trim may retain unsupported boundary surfaces, and Poisson can fill unseen regions; reconstruction still needs visual review.

Quality gates require at least3 registered frames and20 sparse points, and consistent TXT/PLY point counts. Poor inputs may produce no model. Multiple disconnected components are not silently merged: the largest is selected and the count is shown. Lower registered fractions and large reprojection errors are surfaced for review. Scene units/orientation are uncalibrated. Surface preview currently has no texture atlas or per-vertex RGB; original point colors, calibration and intermediate data remain in the job's COLMAP/PLY files. Camera helpers show centers, not camera frustums. Texture baking, physical-scale calibration, masks/dynamic-object removal and automatic cleanup remain follow-up work.

Each attempt creates sfm-UUID or surface-UUID within the frame job; run.json records options, stages/arguments, exit status and bounded output tails. job.json records relative successful-result paths and camera/quality summary. Failed/canceled attempts retain diagnostics and never overwrite the previous successful checkpoint. Load Job resumes from completed saved stages, not mid-command. Cancel stops the active native process or cancels parsing; active work blocks document replacement/close. Partial COLMAP files are kept for inspection and consume disk space until the owner removes the job.

Tests: reconstructionPlyValidation, reconstructionJobPipeline, reconstructionInspectorStages and optional reconstructionRealColmap. The deterministic helper exercises orchestration, CPU/CUDA branches, unsupported CUDA, cancellation, persisted continuation, native scene import and repeated-import deduplication. tools/generate_reconstruction_fixture.py creates a12-view procedural nonplanar scene (NumPy/Pillow only, developer fixture); actual COLMAP CPU sparse/coarse and RTX4060Ti CUDA dense stages are checked separately. Final counts/build/UI receipts are in COORDINATION-PLAN.md.

Primary references: https://colmap.github.io/cli.html ; https://colmap.github.io/format.html ; https://github.com/colmap/colmap/releases/tag/4.2.1 .


Final receipt: focused11 passed /0 failed /no skips; full window138 passed /0 failed /87 optional fixture skips. Actual COLMAP real integration3 passed /0 failed /no skips (one pipeline test plus setup/cleanup):12/12 registered views,586 sparse points, mean reprojection error0.3947px; coarse391 vertices/764 faces; CUDA dense20,135 vertices/39,621 faces at512px/depth7/trim4. Exact reports live under build/windows-local/sfm-{focused,real,full}.txt and build/reconstruction-tools/sfm-fixture/validation.json. Current executable rebuilt and Load Job / Show Surface in 3D visually verified in Vulkan; actual dense surface left visible. Test scene is generated, not a real-user video; broader reconstruction quality and GPU portability still need validation. All implementation and previous YouTube changes are uncommitted; published0.4.0 is unchanged.

### If sparse reconstruction finds no initial pair

Feature extraction and thousands of matches do not guarantee depth. Select a continuous moving-camera section with overlapping static surroundings. A car stopped at a junction, a pure camera pan, cuts and independently moving traffic can prevent initialization. Increasing sampling inside the same stationary interval cannot create a baseline. Start with roughly20-60 frames from a moving section; inspect registered cameras and reprojection error before generating a surface. Mapper failure is distinct from a CUDA dense-stereo failure. The controller now reports these separately and preserves the per-attempt log/manifest and previous successful results.

Validated owner dashcam recipe (2026-10-28): start40s/end55s,2fps,30 frames. Repeated solve recovered30/30 cameras and1,255 points; CUDA surface6,748 triangles at dense size512/depth7/trim4. First solve registered only12/30; inspect coverage and alignment, as initialization can vary. Private reusable checkpoint: build/reconstruction-valencia/moving-40-55/job.json. Dense geometry still needs visual review; no texture atlas.
