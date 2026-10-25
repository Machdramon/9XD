# Scan result selection and preview batches

Scan result lists use desktop extended selection: Ctrl-click adds/removes rows, Shift-click selects a range, and Ctrl+A selects all visible rows. The list also provides Select All Visible and Clear Selection, plus an explicit selected count on the preview button.

Preview Selected queues meshes, images and supported geometry evidence in display order. The former 64-result count limit has been removed. Each result uses the existing decoder and reports completion before the next begins. Stop After Current discards pending work, retaining already loaded results. A failed decoder stops the queue and reports the error; changing documents invalidates pending results.

The batch has an estimated geometry/image allocation budget from Settings > Performance (8 GiB by default) (not a hard guarantee of total process memory usage). Rows outside that budget or unsupported entry types are omitted with a reported count. Individual decoder limits still apply. Container resources must currently be opened individually, and a single batch operates on the active document. Folder/multiple-document scanning and BUILD.TOC/DAT browsing are not implemented by this change.

Scene and sector candidates are alternative views of overlapping source instances. Preview loading now tracks visible, untransformed instances for the current document revision: selecting both loads only missing instances, and repeating a supported/native preview skips an identical visible result. Removing or replacing a mesh permits loading it again; hidden or manually transformed meshes are not treated as the original visible placement. Source edits invalidate preview provenance. These previews are in-memory meshes, not newly generated files on disk.

Tests cover Ctrl/Shift clicks, Ctrl+A, a 70-result asynchronous queue, clearing selection and stopping after the current result. The optional NFSU2 profile test exercises a real three-mesh preview batch.
