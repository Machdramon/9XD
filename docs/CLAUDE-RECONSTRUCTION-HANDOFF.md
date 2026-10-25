# Claude hand-off: reconstruction inspectors

Updated: 2026-10-25. Requested by the project owner. Status: implemented by Claude on 2026-10-25; build and focused tests pass; full vcvars/offscreen suite rerun and a visual check of the spline panel remain (see COORDINATION-PLAN.md).

## Assignment

Define and refine the UI/interaction for the new Video Reconstruction and Image / Spline Modeling inspectors. Keep the functional backend intact. The user explicitly reserved visual design for Claude. Read AGENTS.md, CLAUDE.md, docs/COORDINATION-PLAN.md and docs/reconstruction-tools.md first.

## Starting point

Actual repository: C:/Users/Jarky/Documents/GitHub/binary-workbench. Current changes are uncommitted on main, including unrelated Genki material fixes; preserve them. Do not replace current files with an older workspace mirror. No commit, push or release is authorized by this hand-off alone.

- src/reconstruction_inspectors.h: Reconstruction::VideoInspector and Reconstruction::SplineInspector (QWidget subclasses, no Q_OBJECT).
- src/main.cpp: creates document-session docks and checks active jobs/unsaved projects before document replacement or close.
- src/module_menus.h: both docks belong to the 3D module and appear in its menu.
- tests/window_tests.cpp: reconstructionInspectorContracts and reconstructionFrameJob.

## UI work, in order

1. Fit both inspectors into the established theme, compact grouped controls and existing docking behavior; avoid a second viewer or large text-heavy toolbars. Keep UI text English.
2. Video: clearly distinguish input, output destination, sampling budget, action/status and bounded log. Make Extract Frames/Cancel states obvious. Show that COLMAP reconstruction is not implemented yet; do not imply frames are already a 3D model.
3. Image/spline: improve reference-image selection, curve/point organization and save/load feedback. Plan image-plane/graphical curve editing against the existing UV/2D and 3D workspaces. A reference preview or drawing editor is additional functionality; maintain pixel-coordinate ordering and persistence if implementing it.
4. Preserve app themes/accent tokens and accessible labels. Keep cross-platform presentation in mind; do not merge unrelated Mac build settings.

## Functional contracts to preserve

- videoReconstructionDock / videoReconstructionInspector, splineModelingDock / splineModelingInspector. Session ownership, 3D-only docking, detachable module behavior and saved layout use the existing infrastructure.
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
