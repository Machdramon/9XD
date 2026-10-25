# 9XD

9XD is a local hexadecimal editor and binary analysis tool built with C++ and Qt. It provides byte interpretation, visual formatting, pattern searches, signature scans, entropy analysis, experimental Vulkan 3D and UV views, and Python/C++ scripting.

## Files and editing

Use **File → Open File** (Ctrl+O) to open a local binary. **File → New File** (Ctrl+N) creates an empty, unnamed document in RAM. It grows as you enter data and creates no backing file until you save it. Save asks for a destination when needed. RAM documents support inserting and deleting bytes; existing disk-backed files support overwriting without changing file size or offsets.

**Open Recent** lists recently opened files. **Save** (Ctrl+S) commits pending changes; **Save as** saves to another path. **Hex Viewer → Update Viewer** reloads the current file. Opening another file, reloading or closing prompts you to save, discard or cancel when changes are pending.

Click and drag for a sequential selection. Alt+drag selects a rectangle. Shift+click or Shift+arrow extends the selection from its anchor. **Edit → Select All** (Ctrl+A) selects the entire document.

Type hexadecimal pairs in the byte columns, or characters in the decoded text column. RAM documents append at the end. Press Insert to switch between inserting and overwriting; Delete and Backspace remove bytes in RAM documents. **Edit → Insert Bytes** inserts a hexadecimal sequence. Cut and paste can also resize RAM documents. Type directly in existing disk-backed files to overwrite bytes. Double-click, or use **Edit → Overwrite bytes** (F2), to enter hexadecimal pairs such as `00 FF 2A`. A single selected byte allows writing forward from that offset. Larger or rectangular selections require exactly the same byte count. Paste accepts copied binary data or hexadecimal text. Undo and Redo are available until saving.

Editing is limited to 1 MiB per operation and 4096 rows for rectangular operations. History retains at most 20 MiB of replacement bytes, 1024 operations or 16384 regions. Saving resets history. Disabled menu items represent planned features.

## Modules and command search

The left sidebar switches between Hex Viewer, 3D, UV / 2D, Scripting and Other Tools. Hover an icon to see its module name. Drag a module tab out of the sidebar to detach it; its context menu also provides window controls. Search and Settings shortcuts sit at the bottom of the sidebar.

Press **Ctrl+K** or click Search to find modules, enabled menu commands and main panel actions. Type words to filter, use the arrow keys to select and Enter to run a command. Escape closes the search.

## Documents, layout and appearance

Open documents appear as movable, closable tabs. Each document keeps its own contents, selection, undo history and interpretation settings. The inspector and other shared panels follow the active document. Opening an already open path focuses its existing document.

**Windows → Documents** provides Next/Previous Document (Ctrl+Tab / Ctrl+Shift+Tab), Tabbed View, vertical and horizontal tiling, Cascade, Save All, Close All and an open-document list. Ctrl+W closes the active document, with a save prompt when necessary.

**Windows → Theme** offers Light, Dark and System modes, including graphics backgrounds and grids. Explicit custom graphics colors are preserved. Dock panels can be moved or floated; the workspace restores its layout separately from the earlier single-document layout.

Row backgrounds, column visibility, byte groups and offset base can be configured. Offsets use 0x for hexadecimal, no prefix for decimal and 0c for octal. The plain-text offset header supports direct navigation; modified bytes are highlighted.

## Navigation and interpretation

The vertical scrollbar covers the entire file. The wheel and arrow keys move by rows; Page Up and Page Down move by a screen. Home and End navigate to the beginning and end. Scrollbar dragging is approximate for very large files; **Edit → Go to Offset** (Ctrl+G) accepts exact decimal or `0x` hexadecimal offsets.

The bytes-per-row setting accepts values from 1 to 256. Changing it rearranges the view without modifying the file. **Hex Viewer → Main Viewer → Bytes per row** opens a dialog for entering the row width.

**Hex Viewer → Codification** offers ASCII, UTF-8, UTF-16LE, UTF-16BE and Japanese CP932. Below the separator, choose **Little Endian** or **Big Endian** for numeric interpretation. These options stay synchronized with the viewer and Data Inspector controls. Explicit UTF-16LE/BE text encodings retain their specified byte order.

Numeric interpretation supports 8-, 16-, 32- and 64-bit integers, Float32 and Float64. Viewer rows are decoded independently, so a character or value split by a row boundary may appear incomplete. The inspector reads from the selection anchor even when a value crosses rows or blocks.

**Hex Viewer → Inspector** (Ctrl+Shift+D) shows numeric values, text, dates and color interpretations. It can read subsequent bytes outside the selection. Interpretations do not guarantee that the actual file format uses those types. Ctrl+C in the inspector copies the selected value.

## Formatting and searches

**Hex Viewer → Structure / Formatting Rules → Formatting Rules** opens controls for text color, background, outline, underline, bold and italic styles. Rules can describe a selection, repeated regions or search matches. If rules overlap, the last rule takes precedence. Use the arrow buttons to reorder rules.

Formatting rules are stored separately from binary data. Import or export them as JSON. Export rules before leaving if you want to preserve changes. Limits are 256 rules and 10,000 marked positions.

**Hex Viewer → Find / Coincidences → Find** (Ctrl+F) focuses the pattern field. Searches support hexadecimal wildcards (`??`, `A?`, `?F`), byte regular expressions, literal text and text regular expressions. Text uses the viewer encoding. Text and regex searches are limited to a region of 1 MiB. Hexadecimal wildcard searches can scan the entire file.

**Find next** (F3) and **Find Previous** (Shift+F3) navigate existing search results. **Found Coincidences** opens the same search panel. Mark results to create a formatting rule. Editing the document invalidates previous search results. Search-and-replace is not yet implemented.

## Signatures and entropy

Open **Hex Viewer → Structure / Formatting Rules → Magic Bytes Library** to edit or scan signatures. PNG, DDS and gzip signatures are included as examples. A match is a clue; it does not validate a format, determine its extent or decompress its contents.

Scan the entire file, the file start, the selection start or the starts of marked regions. Signature offsets accept decimal or `0x` values. Click a result to navigate; select multiple rows to outline them using the current formatting style. Import/export the signature library separately as JSON. Limits are 64 signatures and 10,000 results.

**Hex Viewer → Statistics** opens the entropy graph. Analyze the entire file or a contiguous selection. Region sizes increase automatically for large files to keep the graph within 8192 points. Hover for entropy, zero-byte and printable ASCII statistics; click to select a region. High entropy alone does not establish compression or encryption.

## 3D meshes and UVs

Open **3D → 3D Settings / Geometry Calculator** to configure vertex, face index, UV and normal offsets, counts, strides and component types. Set byte order and position scale to match the source. You can add to the collection or update the active mesh.

The test sphere preset uses `examples/sphere128.bin`. Open it from File if you need to preserve or export pending formatting rules first. **Render Current File** decodes the configured regions in the background. Meshes retain snapshots when the document changes; render again to update them.

The **Mesh Collection** panel controls visibility and active selection. Double-click to rename a mesh. Selection and visibility are shared between the 3D and UV views.

In **3D View · Vulkan**, click to select, drag to orbit, right/middle-drag to pan and use the wheel to zoom. Choose vertices, faces or meshes as the selection mode. Options include colors, gradients, grid, lighting and index labels. Frame All fits the visible geometry while preserving the viewing direction; Home resets the direction. Frame Grid locates the reference grid. A suitable Vulkan GPU and native graphical session are required; other editor features remain available if the view cannot start.

In **UV View**, click to select, Alt+click to cycle overlapping hits, right/middle-drag to pan and use the wheel to zoom. UVs outside the 0–1 tile are preserved. Active Only restricts the display to the active mesh; Frame UVs fits the available coordinates.

## Export

**3D → Export 3D model** exports selected meshes to GLB, OBJ or STL. Select active, visible or all meshes and configure scale, axis mapping, axis signs, face winding, normals and UVs.

- GLB preserves separate meshes, names, geometry and optional normals, UVs and a matte color material. It uses meters and Y-up.
- OBJ preserves separate objects, names, geometry and optional normals and UVs. This exporter does not generate MTL materials.
- STL combines triangulated surfaces and face normals. It does not preserve names, UVs, vertex normals or materials.

Exports use actual coordinates, independently of camera framing. Reflections correct face winding. Textures, bones, animations, debug gradients, binary offsets and formatting rules are not exported. The source binary is protected from being overwritten by an export.

## Scripting

Open **Scripting → Scripting Window**. Python runs with embedded CPython; C++ is compiled and executed with the configured compiler. Each run starts in a new context. Scripts run with your local permissions.

The examples demonstrate `workbench.read`, mesh access and mesh publication. Reads include pending edits and are limited to 1 MiB. A changed document or mesh invalidates stale script results. Stop terminates execution; output is limited to 2 MiB. Save scripts before replacing them or closing if you want to retain edits.

## Panels and background activity

Use each module menu to show or hide its panels. Panels can be moved, docked, floated and tabbed. **Reset Panel Layout** restores the default layout. Layout persistence is local to the application.

The **Threads** indicator opens background activity details. Nine task channels cover the viewer, inspector, copy, save, search, signatures, entropy, direct editing and checksums. The display shows requests, elapsed times and counters; it is not a CPU usage meter. Short tasks may finish between the 200 ms updates but are still counted.

A shared cache retains up to twenty 1 MiB blocks, loaded on demand with least-recently-used eviction. This is not the total memory limit: visible cells, history, in-flight reads and analysis results require additional memory. Copy operations are limited to a 1 MiB source region.

Saving writes a complete temporary copy beside the destination and replaces the destination only on success. Allow enough disk space for a full copy. Saving pauses editing and closing; an error preserves pending changes. File size and modification time are checked, but this is not an exclusive file lock or a complete snapshot mechanism. Use a stable local copy for reproducible analysis.

## Settings

Open **Other Tools → Settings** or the Settings toolbar button. General selects System default or English; additional translation packs are not available yet. Appearance selects Dark, Light or System. Hex Viewer settings control text size (8–24 pt) and alternate row backgrounds across existing and new documents.

**Apply** saves and applies preferences while keeping Settings open. **OK** applies and closes. **Cancel** discards changes made since the last Apply. **Restore Defaults** resets the controls; use Apply or OK to save the defaults. Appearance > Themes now supports custom themes, grouped overrides, a live preview and .bwtheme import/export; see [Theme editor](themes.md).


## Module workspaces

The module strip switches between Hex Viewer, 3D, UV / 2D, Scripting and Other Tools. Each module has its own docking container: panels can move, tab together or float within their owning module, and cannot dock into another module. Floating panels hide when you leave their module and return when you switch back. The active document remains the source for all modules.

Hex Viewer contains Main Viewer, Inspector, Find / Coincidences, Structure / Formatting Rules, Statistics, Compare Data and Checksum. Find / Coincidences has a dedicated search panel. 3D contains 3D View, Mesh Collection and 3D Settings / Geometry Calculator, plus model export. UV / 2D contains 2D View, Texture / Image Collection and 2D Settings / Image Calculator. Scripting separates Scripting Window, Console and Library Manager. Other Tools contains Calculator, Thread Activity and Settings.

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

## Programmer Calculator

Open **Other Tools > Calculator**. Select HEX, DEC, OCT or BIN and an 8-, 16-, 32- or 64-bit word. All four representations stay visible. Signed mode uses two's complement, including arithmetic right shifts and signed division/remainder; arithmetic wraps to the selected word size.

The keypad provides integer arithmetic, remainder, AND/OR/XOR/NOT, variable shifts, one-bit shifts, rotations and sign change. Operations run left to right, without expression precedence or parentheses. Show / hide bits opens a clickable bit view. MS/MR/MC store, recall and clear memory; M+ and M− update it.

While the calculator has focus, type digits (A–F in HEX), +, -, *, /, %, &, |, ^ or ~. Use < and > for shifts, Enter or = for the result, Esc to clear, Delete to clear the current entry, and Backspace to remove a digit. Ctrl+C copies the displayed value; Ctrl+V accepts an integer in the selected base or with a 0x, 0b or 0c prefix. Invalid digits are disabled. The calculator operates independently of the active document.

## Per-mesh Position Delta

In **3D > 3D Settings / Geometry Calculator**, the **Position Delta · placement** section sets Delta X, Y and Z for the piece being decoded. Coordinates are placed as `decoded XYZ × Scale + Delta`, in model units. Positive and negative values are supported. Add successive parts to the collection with their respective deltas to assemble a model whose binary vertices use local origins.

In **Mesh Collection**, select a mesh and use **Active Mesh · Position Delta > Apply Position Delta** to adjust placement without rereading the binary. **Reset to Origin** clears the translation. Updating an existing mesh from 3D Settings applies the delta currently entered there. The test-sphere preset resets the delta to zero.

Local vertices, UVs, normals and source offsets are preserved; the source binary is not changed. The 3D view and its picking/labels use the placed geometry. OBJ/GLB/STL exports bake each selected mesh's translation into its exported vertices before the export scale/axis conversion. Scripts continue to access the local mesh coordinates; publishing a replacement for the active mesh retains its placement. Placement currently lasts for the active workspace session.

### Read Position Delta from the binary

Set **Position Delta > Source** to **Read from binary**. Enter an absolute **XYZ offset** in decimal or `0x` hexadecimal, or click **Use Hex Viewer Cursor**. The decoder reads X there, Y at `offset + component spacing`, and Z at `offset + 2 × component spacing`. Use spacing 4 for consecutive Float32/Int32 values or 2 for Float16/Int16; larger spacing supports padding.

Select the component type, delta byte order and delta scale independently of the vertex stream. The final position is `decoded vertex × vertex scale + decoded delta × delta scale`. The values are read from the current document, including unsaved changes, during Render. Invalid ranges and nonfinite values stop the operation without replacing the active mesh. The resolved delta appears in the result status and in Mesh Collection, where it remains adjustable. Manual mode remains available.

## Coordinate formats and transforms

**3D Settings** sections fold using their arrow headers. Vertices and Faces start expanded; Coordinate Transform, Position Delta, UV, Normals and Reading start collapsed. **Expand All / Collapse All** change visibility without changing parameters or enabling/disabling decoding.

Vertex component types include Int8/16/32, UInt8/16/32, Float16/32, SNorm8/16, UNorm8/16 and signed Fixed16.16. SNorm maps signed integers to [-1,1], UNorm maps unsigned integers to [0,1], and Fixed16.16 divides a signed 32-bit value by 65536. Use vertex scale and axis scales for dequantization.

**Coordinates** supports XYZ and homogeneous XYZW (Float16/Float32, dividing XYZ by W). XYZ can also read a four-component record while ignoring W by keeping XYZ and setting the full stride (for example, 16 bytes for Float32). XYZW rejects zero/nonfinite W. Set a sufficient stride for the selected layout.

**Coordinate Transform** provides all six axis permutations, axis inversions, independent XYZ scales and Euler rotations in degrees. The fixed order is decode → homogeneous divide if selected → vertex scale → axis permutation → axis scale/inversion → rotate X, then Y, then Z → Position Delta. Delta is expressed in final model axes; it is not rotated or scaled by this transform. Zero axis scales are rejected.

Supplied normals use the inverse-transpose transformation and are normalized. Calculated normals use the transformed geometry. Reflections automatically reverse triangle winding and matching source-index metadata. UVs remain unchanged. These transforms are applied when Render decodes the piece; exported meshes use the resulting geometry. This does not interpret arbitrary binary transform matrices, skeletons or skinning.

## Standard panel layout and personal preferences

New user profiles start with the product's captured standard arrangement for Hex Viewer, 3D, UV / 2D, Scripting and Other Tools. Dock positions, tabs, visibility and relative sizes are embedded in the executable, so the same initial arrangement ships with development builds and Release packages. Initial window geometry is adjusted to the available screen; monitor-specific detached-window positions are not distributed.

Your own saved module layouts take priority. Closing the workspace saves your panel arrangement, window geometry and detached module windows in your user profile. Updating or extracting a newer Release does not overwrite those preferences. New document tabs share the current module layout.

Use **Windows > Reset Panel Layout** to restore the standard panel arrangement and toolbar visibility. Detached modules return to the main window. This does not reset documents, decoding parameters or other preferences. Subsequent changes are saved normally. Missing or invalid module layouts fall back to the shipped arrangement.

## Local AI assistance

Other Tools > AI Analysis connects to a separately installed local Ollama model. It can inspect bounded regions of the active document and fill its 2D / 3D decoding settings. Use the native Scan File / Scan for 3D Data / Scan for Textures / Images actions to review results, load settings and preview supported assets. See [Native scanning](native-scanning.md). See [Local AI assistance](ai-assistance.md) for setup, limits and sample prompts.

Opening dialogs remember the last folder across restarts; Cancel preserves it.


## 3D scene editing and FBX
See [3D workspace](3d-workspace.md) for transforms, material presets, shared viewports and FBX export.

## Native archive and geometry scanning
Supported archives and compressed entries can be opened directly from scanner results as separate RAM documents. No AI is required. See [Native containers and geometry patterns](native-import-pipeline.md).


### PGR1 textured tracks

Open the original track `.DyP` (day) or `.NtP` (night), leaving the matching `.DyT/.DyV` or `.NtT/.NtV` files in the same folder. Use **Scan for 3D Data**, then select material groups and Preview (Select All previews the base track). GOTHPOK decompression, sibling lookup, texture assignment and texture-library population are automatic. GLB and FBX export retain the assigned materials/textures. The original companion snapshots are checked; rescan after files change.

Validated on Asakusa day/night: 470 material groups and 120,695 exported triangles each. Two invalid-UV triangles are omitted. Two unsupported texture entries use magenta placeholders. Binary texture alpha and face normals are approximations; original vertex lighting and additional scene sections are not reproduced. This adapter does not add PGR1 vehicle or PGR2 track support. RAM-only decoded resources need the original disk P/T/V files for automatic companion resolution.


### MSR / PGR2 material integration (2026-10-23)

MSR alpha primary-LOD cars now resolve embedded Dreamcast VQ textures. Track previews resolve matching DYT/NTT files beside the original DYP/NTP. Missing companions or invalid source UVs retain geometry without fabricating a texture mapping. Native preview sessions share parsed geometry and decoded texture images; these sessions reset when the document changes. Secondary masks and original lighting/shaders are pending.

PGR2 MESH cars resolve MAT/TEXT material references and supported BC3 images in GPUD. Conventional diffuse textures are assigned; source paint/environment records retain neutral surfaces and their references rather than applying a misleading atlas as diffuse. Automatic shader/state assembly, WMSH tracks and PGR3/4 scenes remain pending. 84 supplied indexed car archives have structurally valid material tables; only the separately documented native mesh samples are geometry validated.

MSR late/commercial `Pak` resources now have native bounded decompression available through the resource list. The additional downloads folder contains 37 Pak files and 19 files with a different `yzb_` prefix; that prefix is not decoded by this adapter. Decompression alone does not add retail mesh rendering. Old unresolved-codec/material status notes above are superseded by this section.


## PGR2 base tracks and MSR retail tracks (2026-10-23)

Scan for 3D Data now recognizes supported PGR2 WMSH base tracks and MSR retail Pak tracks natively. Select the scan results together and choose Preview to load the geometry with supported textures; the loaded scene can be exported with its materials. Keep MSR DYT/NTT texture companions beside DYP/NTP geometry files. Missing or unsupported companions retain geometry and report the texture limitation.

PGR2 Barcelona day passes native decoding, bulk UI loading and GLB export: 2,644 material groups and 364,078 triangles, sharing 1,190 decoded images. Structural parsing accepts 34 of 35 supplied area archives; the remaining archive has no supported WMSH. This covers static base geometry, not INST props or additional stream assembly. Original lighting, secondary texture layers and effect shaders are not reproduced. Effect inputs are retained as source references rather than assigned as misleading diffuse textures.

MSR retail uses a separate header layout from the alpha. Asakusa day independently validates 77,617 triangles and a 466-entry companion texture table. Retail vehicles and yzb_ wrappers remain unsupported. PGR3/4 still expose resources only; mesh/scene assembly remains pending.


### PGR2 placed track objects (2026-10-23)

Supported compact INST prototypes now load at their source WMSH placements as separate Scan for 3D Data candidates. The parser validates the nested INST framing, prototype table, independent strips, per-draw vertex pointers, material declarations and affine matrices. Packed prototype/color bits are removed from matrix elements before applying the source transform. Textures share the existing track material decoder. This adds 1,078 placed objects / 129 material groups / 204,534 triangles to Barcelona day, for 568,612 triangles with the base track. The 378 references using unsupported command layouts remain explicitly omitted. COLR instance lighting, command-based prototypes, additional streams and game effect shaders remain pending. Native export is tested; this does not claim full original-game scene reproduction.


### PGR1 vehicles and PGR2 UV correction (2026-10-23)

PGR1 primary vehicle LODs are now native Scan for 3D Data candidates, using the original NRM/HRD/SFT, its _v vertex companion, and _res/default texture companion. Tested source 911.nrm: 39 material groups and 9,212 triangles, exported with textures. SCR/damaged alternatives are excluded; roof/component alternatives and original shaders remain approximate. PGR1 adapters accept buffered RAM bytes when the original disk path and valid companion snapshots are retained. Bounded zlib/GOTHPOK nesting is supported. Detached RAM resources without an original companion path still require the original files.

PGR2 track UV0 is at byte 16 in full world vertices, and byte 6 in compact INST vertices. The previous 0x1c/0x24 layout incorrectly read normal/secondary data as UV0; explicit source-coordinate tests now cover a Barcelona building and compact placed objects. PGR2 vehicle vertices retain their distinct UV-at-10 layout. Triangle topology was not changed by this correction. User-reported triangles connecting Barcelona buildings remain unconfirmed and require a concrete example; the UV fix must not be presented as a topology fix. PGR3/4 mesh assembly remains pending; MSR alpha is confirmed working by the user.


### PGR3/4 native vehicle geometry (2026-10-23)

Scan for 3D Data now previews and exports supported SCNE vehicle primary-LOD draws. The adapter validates big-endian declarations, exact zlib frames, UInt16 FFFF strip restarts, metadata version 3/4 draw ranges and declaration-selected vertex buffers. Every source index must belong to exactly one documented LOD span. All six scene extrema must constrain one integer position quantizer; inconsistent scales fail explicitly. Shared source buffers are cached per document, and exported meshes compact unused vertices.

Validated samples: PGR3 288_GTO_Evo.pak_hrd has 73 geometry groups / 195 primary draws / 118,377 nondegenerate triangles; PGR4 AlfaRomeo_SZ.pak_hrd has 72 geometry groups / 158 primary draws / 78,771 nondegenerate triangles. All multi-buffer groups in these samples are mapped. The historical Ferrari FBX has the same 195 primary draws; 194 match geometric topology exactly after accounting for vertex remapping. One draw differs by two source faces. Native decoding removes 34 exactly collinear source triangles. Material IDs now resolve through each node's explicit 20-byte binding records into the 42-byte material table. Null texture slots are retained (34 slots / 31 embedded textures for the Ferrari; 35 / 32 for the Alfa), preventing index shifts. BC1/BC3/BC5 maps use Xbox 360 tiled addresses and 16-bit word swapping. All 25 Ferrari BC1/BC3 top-level payloads match the historical DDS reference exactly. Decal and base-color slots are displayed; normal/environment maps are retained in the surface library, not used as diffuse. Original shader blending/reflection, secondary UV channels and component-state selection remain approximations. UV0 uses nominal signed-short / 320 coordinates; the historical reference was manually adjusted, so exact source UV scaling still needs visual verification.

Track scenes remain pending: Tokyo_night.pak has separate SR/PR scene resources, and garage scenes require STREAM2 dependencies. PGR3 STREAM2 version 5 uses supported zlib framing in observed blocks; PGR4 version 6 uses a different compressed payload that is not decoded by this vehicle adapter. Neither is treated as a vehicle SCNE payload.

User visual validation confirms PGR1 cars/tracks, PGR2 cars/tracks and MSR alpha cars/tracks. The apparent Barcelona connecting triangles were wires; the previously reported UV issue is corrected. No topology filter was added to hide those wires.

### PGR3/4 direct static track models

Scan for 3D Data offers native static draw candidates, not archive sections to extract. Open the scene PAK (for example Garage_01_sunny.pak or Tokyo_night.pak) or its matching STREAM2; both files must be beside each other. Game-only PAKs do not replace a scene PAK. A study-case copy with missing companions cannot reconstruct the scene.

The adapter follows STRM resource offsets and source vertex-stream/material references, handles PGR3 zlib and PGR4 XMem-framed LZX natively, and validates every rendered vertex against the source leaf bounds. Geometry and texture resources are loaded lazily and cached per document. Static parts can be selected together and rendered/exported directly. Instanced objects and game shader effects remain pending; this is a static-scene adapter, not full scene parity. Texture storage dimensions and streamed mip variants are handled separately.

The LZX implementation uses libmspack (LGPL-2.1) in a replaceable shared library; packages include its source subset and license. No Xbox SDK DLL is required.

Verification: the garage fixtures expose 171 PGR3 static draws / 27,380 triangles and 101 PGR4 static draws / 85,323 triangles. The corrected streamed texture reader assigns base maps to 75 PGR3 garage draws and 87 PGR4 garage draws. The earlier conclusion that PGR4 referenced pixels were absent was caused by skipping the texture tables in nonzero detail levels; those pixels are present in the companion STREAM2. Layered shaders remain unresolved. Tokyo discovery finds 11,738 PGR3 and 13,559 PGR4 static draws; ten distributed draw previews per city were decoded and source-bounds checked. This sample test is not a complete visual validation of all city meshes.

### PGR3 Tokyo Sunny batch and texture correction

The source texture descriptor distinguishes logical/original dimensions from the dimensions of the stored streamed mip. Decoding now uses the stored dimensions, rather than the maximum of both pairs. The original full-corpus run stopped on draw 2818 with `Incomplete texture data`; nine draws failed for this reason. The corrected decoder loads all 11,785 Tokyo Sunny static draws, including the 32x512 stored base map at that first failure.

Only the source color channel (0) is selected as diffuse. Missing base-map pixels do not fall back to normal/specular channels. Multi-layer materials that require source shader blending remain explicitly unresolved, with neutral geometry instead of an arbitrary layer. Source references and texture warnings remain in material metadata. Textures retain compressed payloads and decode only when needed; cached images use the renderer's RGBA format so shared maps do not become copies per mesh. Texture errors retain geometry and are reported as warnings.

Render All renders recognized model results in one click, clears the name filter and skips existing model instances. Per-model decoding failures continue through the batch and appear in the final counts and error details. Cancellation, source changes and a scene memory-budget failure still stop the batch explicitly.

Batch performance: ordinary model identities and track draw offsets now use indexes. Scene IDs remain ordered as meshes are appended/removed, allowing binary lookup. Batch loading avoids repeatedly rebuilding the scan selection controls and reopening/raising the same viewport for every part. Regression coverage includes hidden/replaced/deleted meshes, NFSU2 instance overlap, one-click complete Tokyo Sunny loading, and shared image cache identities.

Streamed texture correction: every resource detail level contains an index-count field before its texture table, even when it contains no indices. Reading that field only in level zero previously hid nearly all higher-level textures. The reader now consumes it in every level and keeps the highest available stored resolution for each source texture slot. Cache accounting charges only retained compressed mip payloads; decoded-image and geometry allowances are independent. Shared decoded images can use one quarter of the configured asset budget (2 GiB with the default 8 GiB setting).

Full Tokyo Sunny decoding now loads all 11,785 static draws with zero geometry failures and assigns base maps to 10,097 (85.68%). The remaining 1,688 (14.32%) comprise 1,536 unresolved layered-material draws and 152 unresolved/missing base-map draws. Texture presence alone does not establish visual material parity: original layered shaders, lighting and material effects remain pending. A portable two-level resource test checks detail-level texture recovery, cache sharing and discarded-mip accounting.

The Vulkan texture upload plans a power-of-two preview reduction only when the shared texture buffer would exceed the device storage-buffer range. Shared map identities are preserved. Source images remain at decoded resolution in the scene and exports; preview scaling does not rewrite them.

PGR3/4 track labels use `source filename / Mesh resource ID / Part ordinal`. Material labels identify their source group/index, and texture labels identify their color-map resource/slot. The original hexadecimal SCNE offset remains in candidate and material metadata for tracing. Track materials declare their source top-left UV origin, matching the source V=0 upper vertices; decoded image pixels and source U coordinates are preserved.

PGR3 partial source hints: inactive version-2 SCNE records can retain source-path fragments beside resource references. Their overwritten fields are shown as question marks. Validated fragments can be attached through a direct color-texture resource/slot reference to a material and draw; conflicting resource signatures are excluded. Labels explicitly say `[texture hint]`, retain mesh/part IDs, and expose the partial path and SCNE provenance in scan details and exported material extras. These are not reconstructed original mesh names. Unsupported layouts, layered-material references, ambiguous resources and unlinked meshes retain the filename/mesh/part fallback.

Tokyo Night validation recovered 116 nonconflicting resource hints, rejected 29 conflicting resource associations and added qualified hints to 13 static draw labels. The ODAKYU sign at SCNE 0x71a8c1 still has no verified source-name hint. This limited coverage does not establish a complete filename or mesh-name table. Synthetic checks cover truncated/out-of-range records, repeated references and conflicts; real hinted draws are decoded and their source metadata checked.

### Viewport navigation and asset lists

Click a 3D viewport to focus it. Hold W/S to move forward/back, A/D to move left/right, Q/E to turn left/right and R/F to move up/down. Shift multiplies movement speed by four. In orthographic views, W/S and A/D pan on screen, and Q/E preserve the fixed view orientation. Mouse orbit, pan, zoom and transform handles remain available. Settings > 3D > Navigation changes the eight single-key bindings and view-relative movement speed. Bindings must be distinct; modified application shortcuts and Escape remain reserved. Navigation stops on focus loss or when a viewport is hidden, so typing in another control does not move the camera.

Mesh Collection and scan results support header sorting, movable/resizable columns, a Columns menu and text filtering. Header layout is remembered. Collection filtering searches names, texture labels, counts and source tooltips. Hidden rows are excluded from bulk actions. Rows retain their mesh IDs during sorting, filtering and collection updates. Bulk scan selection blocks per-row notifications and updates controls once. Collection refreshes are coalesced and update existing rows instead of clearing the list; selection updates touch only affected rows.

Mesh clicks default to whole-mesh selection. Ctrl-click toggles membership; Shift-click adds a mesh. Picking unprojects a near/far ray so orthographic views use parallel rays. The scene caches exact transformed bounds per mesh, avoiding repeated vertex scans on selection. For scenes with at least 200,000 source vertices, assembly and the triangle selection index are prepared on background workers from snapshots; obsolete results are discarded and only the latest pending state is built next. The last prepared view remains visible, and picking waits until its matching index is ready. Camera framing reuses prepared geometry where practical and requests a new background normalization when framing a tiny selection or a very distant region.

Qt widgets and Vulkan presentation/uploads still run on the GUI thread. This change reduces CPU work and repeated rebuilding there; it does not make all GPU uploads asynchronous or guarantee a fixed frame time for every scene.

### Reference grid in model units

The reference grid is finite, fixed at the scene origin, and extends from -10 to +10 along each of its two axes (20 cells across). Minor lines are 1 model unit apart; major lines occur every 5 units. The origin axes have distinct colors, and projected numeric ticks mark 0, +/-5 and +/-10. Perspective and top/bottom views use the XZ plane at Y=0; front/back use XY at Z=0; right/left use ZY at X=0. These are scene/model units after decoding and scene transforms, not an assumed conversion to meters.

The grid has its own tiny vertex buffer and uses a world-to-preview transform that compensates for the viewer's model normalization. Framing, changing models and mesh scaling do not redefine the size of a grid cell. Lines use depth testing and do not write depth; opaque model surfaces occlude reference lines, including coplanar surfaces. Grid ticks are omitted when occluded. It is no longer a depth-free, adaptive screen background.

Frame > Frame Grid moves only the camera to the reference grid, including in an empty viewport or when the model is far from the origin. The grid is not a scene mesh, is not selectable or transformed by mesh tools, does not enter model bounds or the picking index, and is not included in exported geometry. View Settings retains its grid visibility and color controls.
