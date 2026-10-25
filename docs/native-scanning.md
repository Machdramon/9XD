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

## Containers and generic geometry candidates
See [Native containers and geometry patterns](native-import-pipeline.md) for AFS/ZIP/gzip/FArC resource opening, native candidate detection, limits and the corpus runner.

## Genki 0IVX variants
Genki face generation uses isolated C1-v2, explicit GIF-strip v2 and TXRD2/v1 mode/ADC profiles. TXRD2 selects per-record suppression from Y or color/alpha only after its packet and group mode/count/span checks pass. Other unvalidated variants expose only explicitly selected raw point previews. See [Genki topology isolation](genki-topology.md).


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


## Fast and Furious: Xbox ARCX static geometry

The July 7, 2006 prototype has a native static-geometry decoder. Open `03IS300_LOD0_GRA.ARC`, scan for 3D data, then use **Render All** for the placed stock parts. Native triangle lists/strips, material names and parent transforms are reconstructed. Alternative body kits and unplaced LOD geometry remain explicitly selectable without being loaded automatically on top of the stock assembly. Track-sector ARCs, including `TOKYO004_1A.ARC`, use the same decoder. Textures, UV normalization, runtime effects, external wheel assembly and skinned/morph resources remain separate work. See [the supported layout and validation details](faf-xbox-geometry.md).

## TXRD2 primary images and materials — development update 2026-10-25

Open the March reference `txrd2/car/000002a0.sdb`, run **Scan for 3D Data**, then **Render All**. The validated source contains 54 parts / 8,328 triangles; 19 parts now carry their primary local image. **Preview Texture** reads the same bounded bank as **Scan for Textures / Images**; slots 0, 3 and 4 contain pixels, while the three empty references remain explicit. Raw/RAM documents use the same readers.

The native material recipe preserves wide-UV stride and primary UV selection, source slot identities and clamp sampling in the viewport and GLB/FBX. Shared TIM2 decoding also corrects RGB24 colours and extends validated Zero transfer-form/RGB16 palette storage. Full game shaders, reflection stages and arbitrary external texture banks are still separate work. See [the implementation report](research/genki/checks/MATERIAL-INTEGRATION.md) for validation and sample-specific scope.
