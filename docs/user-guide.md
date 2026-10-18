# Binary Workbench

Binary Workbench is a local hexadecimal editor and binary analysis tool built with C++ and Qt. It provides byte interpretation, visual formatting, pattern searches, signature scans, entropy analysis, experimental Vulkan 3D and UV views, and Python/C++ scripting.

## Files and editing

Use **File → Open File** (Ctrl+O) to open a local binary. **File → New File** (Ctrl+N) creates an empty, unnamed document in RAM. It grows as you enter data and creates no backing file until you save it. Save asks for a destination when needed. RAM documents support inserting and deleting bytes; existing disk-backed files support overwriting without changing file size or offsets.

**Open Recent** lists recently opened files. **Save** (Ctrl+S) commits pending changes; **Save as** saves to another path. **Hex Viewer → Update Viewer** reloads the current file. Opening another file, reloading or closing prompts you to save, discard or cancel when changes are pending.

Click and drag for a sequential selection. Alt+drag selects a rectangle. Shift+click or Shift+arrow extends the selection from its anchor. **Edit → Select All** (Ctrl+A) selects the entire document.

Type hexadecimal pairs in the byte columns, or characters in the decoded text column. RAM documents append at the end. Press Insert to switch between inserting and overwriting; Delete and Backspace remove bytes in RAM documents. **Edit → Insert Bytes** inserts a hexadecimal sequence. Cut and paste can also resize RAM documents. Type directly in existing disk-backed files to overwrite bytes. Double-click, or use **Extras → Workbench Tools → Overwrite bytes** (F2), to enter hexadecimal pairs such as `00 FF 2A`. A single selected byte allows writing forward from that offset. Larger or rectangular selections require exactly the same byte count. Paste accepts copied binary data or hexadecimal text. Undo and Redo are available until saving.

Editing is limited to 1 MiB per operation and 4096 rows for rectangular operations. History retains at most 20 MiB of replacement bytes, 1024 operations or 16384 regions. Saving resets history. Disabled menu items represent planned features.

## Documents, layout and appearance

Open documents appear as movable, closable tabs. Each document keeps its own contents, selection, undo history and interpretation settings. The inspector and other shared panels follow the active document. Opening an already open path focuses its existing document.

**Windows → Documents** provides Next/Previous Document (Ctrl+Tab / Ctrl+Shift+Tab), Tabbed View, vertical and horizontal tiling, Cascade, Save All, Close All and an open-document list. Ctrl+W closes the active document, with a save prompt when necessary.

**Windows → Theme** offers Light, Dark and System modes, including graphics backgrounds and grids. Explicit custom graphics colors are preserved. Dock panels can be moved or floated; the workspace restores its layout separately from the earlier single-document layout.

Rows alternate subtly, groups of eight bytes are separated, offsets use aligned hexadecimal digits, and modified bytes are highlighted.

## Navigation and interpretation

The vertical scrollbar covers the entire file. The wheel and arrow keys move by rows; Page Up and Page Down move by a screen. Home and End navigate to the beginning and end. Scrollbar dragging is approximate for very large files; **Edit → Go to Offset** (Ctrl+G) accepts exact decimal or `0x` hexadecimal offsets.

**Stride** controls bytes per row, from 1 to 256. Changing it rearranges the view without modifying the file. **Hex Viewer → Bytes per row** focuses this control.

**Hex Viewer → Codification** offers ASCII, UTF-8, UTF-16LE, UTF-16BE and Japanese CP932. Below the separator, choose **Little Endian** or **Big Endian** for numeric interpretation. These options stay synchronized with the viewer and Data Inspector controls. Explicit UTF-16LE/BE text encodings retain their specified byte order.

Numeric interpretation supports 8-, 16-, 32- and 64-bit integers, Float32 and Float64. Viewer rows are decoded independently, so a character or value split by a row boundary may appear incomplete. The inspector reads from the selection anchor even when a value crosses rows or blocks.

**Hex Viewer → Data Inspector** (Ctrl+Shift+D) shows numeric values, text, dates and color interpretations. It can read subsequent bytes outside the selection. Interpretations do not guarantee that the actual file format uses those types. Ctrl+C in the inspector copies the selected value.

## Formatting and searches

**Windows → Formatting Rules** opens controls for text color, background, outline, underline, bold and italic styles. Rules can describe a selection, repeated regions or search matches. If rules overlap, the last rule takes precedence. Use the arrow buttons to reorder rules.

Formatting rules are stored separately from binary data. Import or export them as JSON. Export rules before leaving if you want to preserve changes. Limits are 256 rules and 10,000 marked positions.

**Search → Find** (Ctrl+F) focuses the pattern field. Searches support hexadecimal wildcards (`??`, `A?`, `?F`), byte regular expressions, literal text and text regular expressions. Text uses the viewer encoding. Text and regex searches are limited to a region of 1 MiB. Hexadecimal wildcard searches can scan the entire file.

**Find next** (F3) and **Find Previous** (Shift+F3) navigate existing search results. **Found Coincidences** opens the same search panel. Mark results to create a formatting rule. Editing the document invalidates previous search results. Search-and-replace is not yet implemented.

## Signatures and entropy

Open **Extras → Workbench Tools → Magic Bytes Library** to edit or scan signatures. PNG, DDS and gzip signatures are included as examples. A match is a clue; it does not validate a format, determine its extent or decompress its contents.

Scan the entire file, the file start, the selection start or the starts of marked regions. Signature offsets accept decimal or `0x` values. Click a result to navigate; select multiple rows to outline them using the current formatting style. Import/export the signature library separately as JSON. Limits are 64 signatures and 10,000 results.

**Analyse → Statistics** opens the entropy graph. Analyze the entire file or a contiguous selection. Region sizes increase automatically for large files to keep the graph within 8192 points. Hover for entropy, zero-byte and printable ASCII statistics; click to select a region. High entropy alone does not establish compression or encryption.

## 3D meshes and UVs

Open **Windows → 3D Settings** to configure vertex, face index, UV and normal offsets, counts, strides and component types. Set byte order and position scale to match the source. You can add to the collection or update the active mesh.

The test sphere preset uses `examples/sphere128.bin`. Open it from File if you need to preserve or export pending formatting rules first. **Render Current File** decodes the configured regions in the background. Meshes retain snapshots when the document changes; render again to update them.

The **Mesh Collection** panel controls visibility and active selection. Double-click to rename a mesh. Selection and visibility are shared between the 3D and UV views.

In **3D View · Vulkan**, click to select, drag to orbit, right/middle-drag to pan and use the wheel to zoom. Choose vertices, faces or meshes as the selection mode. Options include colors, gradients, grid, lighting and index labels. Frame All resets the camera. A suitable Vulkan GPU and native graphical session are required; other editor features remain available if the view cannot start.

In **UV View**, click to select, Alt+click to cycle overlapping hits, right/middle-drag to pan and use the wheel to zoom. UVs outside the 0–1 tile are preserved. Active Only restricts the display to the active mesh; Frame UVs fits the available coordinates.

## Export

**Extras → Workbench Tools → Export 3D model** exports selected meshes to GLB, OBJ or STL. Select active, visible or all meshes and configure scale, axis mapping, axis signs, face winding, normals and UVs.

- GLB preserves separate meshes, names, geometry and optional normals, UVs and a matte color material. It uses meters and Y-up.
- OBJ preserves separate objects, names, geometry and optional normals and UVs. This exporter does not generate MTL materials.
- STL combines triangulated surfaces and face normals. It does not preserve names, UVs, vertex normals or materials.

Exports use actual coordinates, independently of camera framing. Reflections correct face winding. Textures, bones, animations, debug gradients, binary offsets and formatting rules are not exported. The source binary is protected from being overwritten by an export.

## Scripting

Open **Windows → Scripting · Python / C++**. Python runs with embedded CPython; C++ is compiled and executed with the configured compiler. Each run starts in a new context. Scripts run with your local permissions.

The examples demonstrate `workbench.read`, mesh access and mesh publication. Reads include pending edits and are limited to 1 MiB. A changed document or mesh invalidates stale script results. Stop terminates execution; output is limited to 2 MiB. Save scripts before replacing them or closing if you want to retain edits.

## Panels and background activity

Use **Windows** to show or hide panels. Panels can be moved, docked, floated and tabbed. **Reset Panel Layout** restores the default layout. Layout persistence is local to the application.

The **Threads** indicator opens background activity details. Nine task channels cover the viewer, inspector, copy, save, search, signatures, entropy, direct editing and checksums. The display shows requests, elapsed times and counters; it is not a CPU usage meter. Short tasks may finish between the 200 ms updates but are still counted.

A shared cache retains up to twenty 1 MiB blocks, loaded on demand with least-recently-used eviction. This is not the total memory limit: visible cells, history, in-flight reads and analysis results require additional memory. Copy operations are limited to a 1 MiB source region.

Saving writes a complete temporary copy beside the destination and replaces the destination only on success. Allow enough disk space for a full copy. Saving pauses editing and closing; an error preserves pending changes. File size and modification time are checked, but this is not an exclusive file lock or a complete snapshot mechanism. Use a stable local copy for reproducible analysis.

## Settings

Open **Edit → Settings** or the Settings toolbar button. General selects System default or English; additional translation packs are not available yet. Appearance selects Dark, Light or System. Editor controls text size (8–24 pt) and alternate row backgrounds across existing and new documents.

**Apply** saves and applies preferences while keeping Settings open. **OK** applies and closes. **Cancel** discards changes made since the last Apply. **Restore Defaults** resets the controls; use Apply or OK to save the defaults. Theme creation and import/export remain planned.


## Module workspaces

The module strip switches between Hex Viewer, 3D, UV / 2D, Scripting and Other Tools. Each module has its own docking container: panels can move, tab together or float within their owning module, and cannot dock into another module. Floating panels hide when you leave their module and return when you switch back. The active document remains the source for all modules.

Hex Viewer contains Main Viewer, Inspector, Find / Coincidences, Structure / Formatting Rules, Statistics, Compare Data and Checksum. Find / Coincidences has a dedicated search panel. 3D contains 3D View, Mesh Collection and 3D Settings / Geometry Calculator, plus model export. UV / 2D contains 2D View and planned image tools. Scripting separates Scripting Window and Console, with Library Manager planned. Other Tools contains Calculator, Thread Activity and Settings.

Tools whose functions are not implemented remain disabled. File, Edit, Windows and Help retain common document operations. Panel layouts are stored separately for each module and shared when switching documents.


## Modules on multiple monitors

Each module can open as an independent window: drag its module tab outside the tab strip, double-click the tab, use the tab context menu, or choose Windows → Module Windows. Move the separate window to another monitor using its title bar. Several modules can remain visible at the same time and continue following the same active document.

Use the module tab context menu or Windows → Module Windows to reattach it. Closing a separate module window also reattaches it; it does not close documents. File actions and keyboard shortcuts remain available in separate windows. Panels still dock only within their own module.

The application remembers which modules were separate and their positions when you exit. If monitor availability changes, inaccessible windows return to an available screen. Exiting the main application checks unsaved documents and closes all module windows; canceling exit keeps them open.



## Document tabs and hexadecimal footer

Document tabs use a uniform compact width of up to 180 px. They shrink to a minimum of 110 px as more files are opened, then use scroll arrows instead of shrinking further. Long names are elided in the middle; hover over a tab for the full document name or path.

The hexadecimal module keeps file status and thread activity in a footer below the complete docking area. It follows the active document and stays at the bottom when panels are rearranged or Hex Viewer is moved to a separate window.


## Checksum inspector

Open **Hex Viewer → Checksum**. The inspector belongs to Hex Viewer and can be docked, tabbed or floated within that module, including when Hex Viewer is in a separate window.

Choose Checksum-8, Checksum-16, Checksum-32, CRC-32, MD5, SHA-1, SHA-256 or SHA-512, or calculate all algorithms in one pass. Choose the entire document, a contiguous selection or a specified range. Range start and length accept decimal or hexadecimal values with a 0x prefix. Calculations include unsaved edits and work on RAM documents.

The optional **Expected result** compares a hexadecimal value against the calculated result and shows Match or Mismatch. Spaces and a 0x prefix are accepted. With all algorithms selected, results of other lengths are marked Different length. Editing the expected value updates verification without recalculating. Copy one result or all rows from the results table.

Calculation runs in the background with progress and cancellation. Document changes invalidate previous results. Checksum-8/16/32 sum unsigned bytes modulo the named width; CRC-32 uses the standard reflected IEEE algorithm. The inspector calculates and compares values; it does not insert or repair checksum fields in the document.

## UV / 2D image library and calculator

Open UV / 2D → Texture / Image Collection and 2D Settings / Image Calculator. Both panels dock only in UV / 2D, including separate module windows. 2D View offers Image Preview and the existing UV Layout as separate tabs. Image Preview supports wheel zoom and dragging to pan.

Open Folder adds up to 512 regular files from the chosen folder (not subfolders), without loading all their contents. Select a file and calculate with the desired layout. Known image extensions select Encoded image; actual decoding uses content detection. For embedded encoded images, enter their start offset. Raw images require explicit parameters; headers, compression and console tiling are not detected automatically.

Source can be Active document, including RAM and unsaved changes, or Selected library file. Raw formats: RGBA8, BGRA8, RGB8, BGR8, Gray8, RGB565 and Indexed8/Indexed4. RGB565 supports both byte orders. Indexed formats use a separate offset into the same source for 256 or 16 RGBA8 palette entries. Indexed4 reads the high nibble first. Start, row stride, palette offset and image step accept decimal or 0x hexadecimal. Row stride 0 means tightly packed rows. Use Hex Selection Start copies the first selected offset without duplicating the hexadecimal editor.

Set Images to generate and Image step to generate a sequence from one binary; step 0 uses the calculated image span. Encoded images are decoded individually. Calculate captures the source and pending edits, reads through the shared binary reader in 1 MiB blocks and runs in the background. Cancel keeps already completed images. Changing the source document cancels its current calculation. Existing images remain snapshots and are not refreshed automatically.

Export Selected PNG exports an image. Save Library writes generated/decoded image snapshots into a .bwil library; unprocessed source files are not embedded. Open Library appends those snapshots without requiring the original binaries. A saved snapshot has no source binary for recalculation. Libraries are separate for each open document and must be saved before closing to retain them.

Limits: 8192 pixels per dimension, 4 million pixels per image, 32 MiB of source data per image, 128 generated images per batch, 512 collection entries and 128 MiB of decoded images per document. BC/DXT compression, swizzle/tiling, mipmaps, format-specific importers and texture assignment to 3D meshes remain future work.

### Standard images and generic unswizzle

Open Files adds individual images or binaries to the UV / 2D collection. Encoded image reads PNG, JPEG, BMP and the other Qt handlers present in this installation; the calculator displays the available format list. TGA has an additional built-in decoder for raw and RLE truecolor, indexed and grayscale images, including image origin flags. Interleaved TGA is not supported. Animated formats currently show a single frame. PNG remains the export format. This does not scan an arbitrary binary for every embedded image; set the start offset of an embedded image explicitly.

Memory layout / Unswizzle offers Linear, Morton Z-order (X bits in even positions, Y bits in odd positions) and Tiled 8x8 (row-major tiles with row-major pixels inside each tile). Morton requires square power-of-two dimensions; 8x8 tiling requires dimensions divisible by eight. Both require row stride 0 and operate on the existing raw pixel formats, including indexed pixels. They are generic layouts, not complete PS2/Xbox 360/PS4 texture presets. Encoded images use their own decoder and ignore the raw layout setting.

TIFF, WebP, DDS/BCn and additional console formats are not provided by the current decoder set and remain separate additions.
