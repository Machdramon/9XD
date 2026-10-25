# Reconstruction tools

Updated: 2026-10-25. Owner: ChatGPT/Codex; shared integration tracking in COORDINATION-PLAN.md.

Both tools are approved for development. This document describes preparation, not a shipped feature.

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
- [ ] Implement frame sampling and sparse COLMAP reconstruction; import points/cameras into current 3D module.
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
