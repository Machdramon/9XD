# File picker: large folders and usability

Updated: 2026-10-26. Owner: ChatGPT/Codex. User identified Open File as freezing when entering the large TXR3 folder. This change improves the existing Qt browser rather than embedding a separate file manager.

## Implemented

- Lightweight generic file/folder/drive icons and extension-derived type labels replace per-file operating-system icon/type lookups. No thumbnail or binary-content previews are requested for the listing.
- Retain QFileSystemModel's asynchronous gathering. The previous model already gathered in a worker; the fix addresses additional icon, filter and view work rather than claiming to move Qt widgets to another thread.
- Uniform tree rows, batched list layout (256), interactive column widths and detail view prevent unbounded per-item sizing work.
- Search waits 220 ms after typing pauses, with an empty-filter fast path and a 256-character limit. Escape clears immediately; Enter in search does not accidentally open a file. Directories remain visible for navigation. Filtering is current-folder name matching, not recursive/content search.
- Loading indicator and final entry count distinguish work in progress from an unresponsive dialog.
- Standard type groups: all files, binary/game resources, images, scripts/profiles; caller-provided filters still apply.
- Multiple selection, sortable/resizable columns, recent-folder sidebar and last successful folder preserved. Cancel does not change location history.
- Shared single/multiple/folder wrappers initialize the lightweight model before setting the large target directory. Track-context profile/folder selection uses the same wrappers. Save/export and other unrelated raw Qt dialogs are not globally replaced.

Implementation: src/file_dialogs.h and src/asset_scan_panel.h. Lightweight icon provider is application-owned, immutable during use and shared safely across dialog instances. No source code from other file managers was copied.

## Actual checks

Full suite with vcvars64/offscreen: reader 45 passed/0 failed; window 103 passed/0 failed/79 optional skipped. Search test now uses persistent model indices across asynchronous sorting and waits for deferred filters; previous nonpersistent indices made the first rerun fail despite model reordering.

Optional real-folder run: 5 passed/0 failed/0 skipped, covering largeFileDialogListing, name search and multi-select/recent behavior. Read-only test folder: F:/copia de seguridad 08.04.2019/downloads/Tokyo Xtreme Racer 3 [NTSC]/Tokyo Xtreme Racer 3 [NTSC]/Tokyo Xtreme Racer 3/Nueva carpeta working one.

- 27,112 entries loaded in 3,354 ms.
- Worst measured GUI event-loop gap: 349 ms; 246 heartbeat events during measured loading interval.
- Filename/wildcard filtering, Escape reset and cancel passed on that same listing.

This is an offscreen local-folder measurement on this machine, not a cold-cache/network benchmark or a guarantee of zero pauses on every filesystem. The original Windows freeze was not separately profiled with a debugger, so shell callbacks are a suspected cost removed, not a uniquely proven original cause. Slow/offline sidebar volumes and opening thousands of documents at once are separate issues; no new document-count restriction added.

Logs: build/windows-local/large-file-dialog-tests.txt, release037-reader-tests.txt and release037-window-tests.txt; build/review-fixes-app-build.log. Development executable built and updated; no commit/push/release.

## Design references

- Qt QFileSystemModel documentation: threaded gathering, cached metadata, directoryLoaded. https://doc.qt.io/qt-6/qfilesystemmodel.html
- Microsoft File Explorer: navigation/sidebar, detail columns, search. https://support.microsoft.com/en-us/accessibility/windows/use-a-screen-reader-to-explore-and-navigate-file-explorer-in-windows
- Files Community: modern Windows file-manager layout reference. https://github.com/files-community/Files
- Blender File Browser manual: path/search, type filters, sorting and recent/bookmark regions. https://docs.staging.blender.org/manual/en/latest/editors/file_browser.html

Future visual refinement may borrow the clear location/search/quick-access separation, keeping costly image previews opt-in and lazy for large binary folders. This implementation does not embed Files/Blender, add recursive thumbnail scans, or claim their complete feature sets.
