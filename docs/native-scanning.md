# Native file and asset scanning

Discovery belongs to each document and works without Ollama. The Hex Viewer, 3D and UV/2D scanners and the assistant use the same bounded discovery engine and share current scan results. Editing or changing the document invalidates those results, pending scans and cached AI evidence. Unsaved byte edits are included.

## Entry points

- **Hex Viewer > Scan File** is at the end of the Stride / Text / Interpret row. It opens File Scan with recognizable structures, image/model recipes, geometry evidence and texture-name references. Mark recognized structures after scanning is enabled initially; turn it off to review results first. The scanner appends visual formatting rules prefixed Scan / metadata. It does not change file bytes, replace existing rules or duplicate identical rules. Export formatting rules separately if needed.
- **3D > Scan for 3D Data** appears in the main viewport toolbar and opens 3D Data Scan. Split tiles share the main scan button; floating viewports forward scans to their document.
- **UV/2D > Scan for Textures / Images** appears above Image Preview / UV Layout and opens Texture / Image Scan.

3D and UV/2D scans initially preview the first supported result automatically. Disable Preview first supported result after scanning to inspect metadata first. The remaining results stay available for individual preview; unsupported packet formats never receive invented faces. The Hex Viewer scan marks structures without automatically rendering assets.

These inspectors dock only inside their owning module, and are also available from that module's menu. Personal layouts continue to take priority over the bundled default.

## Using a result

- **Go to Bytes** switches to Hex Viewer and selects the detected offset.
- **Load Settings** copies a supported recipe into its 3D or 2D controls for manual review. It does not decode anything.
- **Preview Selected** uses the production decoder, opens the appropriate view, and reports actual completion or failure in the scan inspector. Decoded images enter the normal image collection and can be exported as PNG. Supported triangle models enter Mesh Collection and use the existing model export tools.
- **Mark Recognized Structures** appends visual structure rules for the recognized ranges. Unsupported layouts are not presented as working raw decoder recipes.
- **Preview Points (No Faces)** is available for the validated ARCP packet profile. It opens measured positions as a point cloud, explicitly named NO FACES. It does not invent triangle indices, hierarchy, placement matrices, materials or texture assignment.

Texture filename references are shown separately from image data. A string such as Body-Top.png cannot by itself be decoded into pixels; reference rows support byte navigation, not image preview.

## Scope and limits

The scanner examines the first 32 MiB, with cancellation and bounded metadata results. Its status states how many bytes were actually inspected and whether that window was truncated. It is a format-aware discovery pass, not a universal model extractor or a proof that no assets exist. Current full decoder candidates include bounded GLB Float32 position/index layouts, TIM2 pictures, and signature/header-validated PNG, JPEG, BMP, GIF and DDS images. The production decoder verifies the selected image/geometry before reporting success. Some formats supported by the manual decoders still require an explicit recipe and are not recognized automatically.

Genki 0IVX/XMDL packet evidence remains distinct from ARCP. For 000000c4.arc, the tested ARCP profile contains 57 resource-directory entries and 10 validated DMA/VIF packets, with 3,256 Float32 XYZ vertices. The first validated positions start at 0x20c0. The validated control profile reconstructs 2,222 triangles across 517 independent strips. Preview Recovered Mesh uses those connections, with alternating winding reset at each strip boundary. Each masked S8 control table contains a strip count and triangle counts, consuming exactly the position records; six trailing control bytes remain uninterpreted and never become triangle indices. Invalid or incomplete controls retain only point-preview support. Material and hierarchy reconstruction remain unimplemented. The signature and measured layout do not by themselves establish a game's developer.

## Assistant

Calculate 2D and Render 3D buttons have been removed from the chat. Ask the assistant to explain the current scan or inspect the document; it can use the same native evidence. Existing permission-controlled decoder tools remain available for explicit requests to decode supported assets. Only successful decoder receipts mean an output was loaded. An unsupported recipe is not absence of geometry, and empty caches cannot substitute for a scan.

## Genki PS2 packet geometry

The validated 0IVX00.1 / LDMX00.2 / MRON00.2 profile now supports native Preview Recovered Mesh and AI decode_asset(kind=mesh). For 000004b8.dat this scans 867 bounded DMA/VIF packets and reconstructs 26,532 vertex records and 21,942 nondegenerate faces. Packet-local strips use Y bit0 suppression and DMA parity; the marker is removed from decoded coordinates. No raw index offset is guessed. Source bytes and unsaved patch semantics are preserved.

The result enters Mesh Collection and can be exported through the usual FBX/GLB/OBJ/STL tools. Stored normals are retained; missing normals are generated from recovered faces, with a neutral normal for isolated unused records. UV pairs use the sample's plausible 1/4096 scale; missing UVs remain zero. Materials and UV-to-texture binding are not reconstructed. All local parts and overlapping tuning/visibility variants remain together; this is not a resolved game scene.

TIM2 discovery walks all declared pictures within the bounded scan, capped at 128 image candidates instead of four. The test file supplies 40 pictures; each can be previewed in UV/2D and exported independently as PNG. Geometry annotations take priority when the 32-region structure summary fills. Discovery remains cancellable and bounds-checked.
