# Priority racing formats â€” 22 October 2026

User priority: FM1, MSR (including alpha), PGR1/2/3/4 and R-Tuned. This is staged native import work, not universal support for these games.

## Current first milestone

| Family | Implemented in this milestone | Still unresolved |
|---|---|---|
| FM1 | CAB v15 extraction plus native CARBIN named-part geometry from the archive or RAM. Int16 XYZ / 256, source SNorm16 normals, independent Xbox primitive-6 strips, primary draw set 1. Multi-select preview and export. XB2 aligned zlib extraction; Autocross native track import with 270 direct meshes, UV0, 79 textures and portable primary materials. | CARBIN alternate draw sets/LODs, damage, pivots and UV/material decoding; track special shaders, packed tree lightmaps and secondary layer composition. |
| PGR1 | Native bounded GOTHPOK LZ extraction validated on all 15 supplied car/track resources. | NRM/DYP/DYV geometry and material assembly. |
| PGR2 | Validated 16-byte indexed resource tables and bounded zlib sync-flush sections; native compact vehicle MESH/GPUD/VB/INFO first-LOD groups, independently reset strips and packed UVs. | WMSH track/PAK_STREAM assembly, component/state choice, texture assignment and game shaders; only supplied 240Z geometry validated. |
| PGR3 | ENDI/ENSW outer section tables; zlib compressed ENSW sections; strictly identified SCNE GPU-block descriptors and streams. | Other compressed block descriptor variants, STREAM2 traversal, joining vertex/index/declaration data, transforms and materials. |
| PGR4 | Same confirmed outer variants, independently checked against supplied Alfa/Garage samples. | BPR/STREAM2 variants, vertex declarations, assembly/materials. Do not assume PGR3 layouts apply globally. |
| MSR | Native alpha primary vehicle LOD and track BSP leaf/material geometry, independent strips, local centers and per-corner UVs; tested Alfa 156 and Asakusa day/night, UI bulk load and GLB export. | Retail Pak compression, special alpha nodes, further vehicle LODs and texture binding. Do not feed retail bytes into alpha or PGR rules. |
| R-Tuned | FArC extraction, geometry.bin v8 discovery and stage collision import (Collision132 and COLI0200), directly from FArC or decoded RAM resources; common preview/export path. | object.bin placement, hierarchy/variant selection, material and texture linkage, packed original normals for visual meshes, and other stage/visual formats. |

## How to use

- R-Tuned: open `rx8sesp00000.farc`, **Scan for 3D Data**, select `geometry.bin`, **Open Resource**. Its RAM document scans automatically. Select the seven mesh candidates and preview together; an already previewed candidate is skipped. These are local geometry pieces, not a fully resolved game scene.
- R-Tuned stage: open `cs_STGHKN.farc` and **Scan for 3D Data**. Twelve collision surface candidates are available directly from the archive. Ground previews automatically and contains the full road network. Add selected wall/route groups as needed; alternatives can overlap. The same importer runs when a `_colli.bin` or `COL_LEN_*` child is opened as a RAM document. The seven `_block.bin` resources are spatial lookup tables and receive a diagnostic instead of a fabricated mesh.
- R-Tuned visual course: open **`rtuned/course/stghkn.farc`**, then **Scan for 3D Data**. The course previews as one combined mesh; Backdrop / Sea and Sky are separate optional groups. Open `geometry.bin` for individual buffer inspection. The other `rtuned/STGHKN.farc` contains animation/camera data rather than the track's mesh buffers.
- FM1: open `car_abflugsupra.cab`, scan, select the named resource and **Open Resource**. Scanning the CAB now lists 30 renderable CARBIN parts; use Select All and Preview to load the primary car geometry. The same parser works when opening the CARBIN resource in RAM. For the supported Autocross XB2, scan the original file directly; its combined decoded CAB can also be opened and scanned in RAM. Select the desired meshes and Preview. The texture scan lists the 79 supported image resources for batch extraction.
- PGR: scan a supported outer PAK/PAK_HRD; open a named section or supported GPU block. Decompression is a separate stage from mesh discovery. Stream buffers are not labeled as meshes without validated connectivity/declarations.

## Validation and source evidence

Completed: 45 reader tests and 73 window tests passed, zero failures; 21 optional tests skipped. The enabled local priority-corpus test verified 220 zlib resource views, all 97 CAB entries, and seven R-Tuned meshes (10,641 vertices / 11,282 triangles). The R-Tuned test checks OBJ/GLB/FBX export, scan-to-collection batch loading and repeat-preview deduplication. This is decoder/UI/export validation, not a claim of reference-image or material correctness. The local CLion executable was rebuilt successfully; no GitHub release was created.

- `study-case-audit/priority-families/survey.json`: original paths, SHA-256, sizes, headers and independently inspected compression evidence for these eight folders.
- `native-validation.json`: production container extraction/geometry decode report (local optional test). Checks bounded source ranges, declared output sizes, zlib checksum/end-of-stream, positions, indices and production export.
- FM1 CAB directory layout cross-checked with Luigi Auriemma's original reference: https://mirror.aluigi.org/bms/forza_motorsport.bms . Reference script read, never executed; implementation uses the app reader and native zlib. No QuickBMS runtime dependency.
- Historical references: `WIP_ForzaMotorsport1-5EBF7.ms`, `iguess-21CBF.py`, `pgr3_unpack-1F68D.py`, `project_gotham_4-47AB7.bms` in the user's Discord archive. These were read only, not executed or copied into the app. Old WIP scripts contain unproven geometry assumptions; they do not establish correctness.
- FM1 CARBIN sample: 30 named parts, 11,951 source vertices and 196 subparts in five source draw sets. Import only set 1 (61 draws / 10,541 triangles); do not merge alternate sets. Preserve part names, including LOD suffixes. Position decoding is signed Int16 / 256, stride 20; normals are SNorm16 at +6. Xbox primitive 6 is an independent triangle strip. Convert clockwise source faces to match outward normals. UV encoding is intentionally unresolved, rather than guessing a texture scale.
- PGR section entries may overlap exposed inner streams intentionally; they are alternative resource views. The scene preview duplicate registry still applies to decoded mesh candidates, not archive byte ranges.
- Generic resource budgets remain 128 MiB input/output per extraction; zlib member inspection has a 512 MiB aggregate decompression-work budget and cancellation. Members and nested resources retain existing depth/entry caps. No files are written automatically by archive names.

## Next investigation order within this priority group

1. Expand FM1 beyond primary CARBIN geometry: explicit draw-set/LOD selection, damage, pivots, UV/material/texture decoding and track XB2 internals.
2. R-Tuned object/geometry correspondence, material slots and TXP texture linking.
3. PGR3/4 vertex declarations and compressed buffer grouping, checked separately for each game.
4. PGR2 inner INDX/WMSH and sibling stream addressing.
5. PGR1 GOTHPOK and retail MSR compression, then their geometry; alpha MSR remains a distinct case.

Acceptance requires correct topology, orientation, grouping/LOD and a visual reference; finite positions and successful export alone are insufficient.

## R-Tuned stage collision records

The supplied `cs_STGHKN.farc` has 19 resources: seven fixed-record polygon resources, five `COLI0200` surface resources, and seven spatial block indices. The 12 geometry resources decode to 111,535 polygon-corner vertices and 54,849 triangles. Ground alone has 54,895 corners and 26,529 triangles. These are collision surfaces, not buildings/decorations/textured visual assets.

- Collision132: LE count, data offset 32, reserved zero header, 132-byte records with polygon flag 0/1, reserved field, four Float32 position/normal pairs and extra collision metadata. Flag 0 repeats corner 2 as corner 3 and produces one triangle; flag 1 produces a quad. Final source padding aligns to 32 bytes.
- COLI0200: explicit ordered table offsets, repeated polygon counts, 128-byte surface records with four positions, separate 48-byte normal records, and aligned ancillary tables. Other record fields and spatial tables are not interpreted as geometry.
- Triangulation uses the stored boundary order. Concave quads choose the internal diagonal consistent with the source normals; COLI0200 winding is reversed to match those normals. Unsupported or degenerate layouts are rejected.
- RAM results retain vertex/face byte provenance. Direct archive previews intentionally omit byte mappings to compressed child data, since decompressed offsets cannot be used as offsets into the original archive.
- The scanner uses bounded native extraction, including pending byte edits, with cancellation and source snapshot checks. No AI, external extractor or temporary on-disk extraction is needed.
- Local test `localRTunedStageScan` exercises all 19 resources, archive-vs-RAM geometry equivalence, multi-preview without duplicates, and OBJ/GLB/FBX export. `rtunedCollisionRamAndValidation` checks triangle/quad reconstruction, concavity, corrupt/truncated/NaN inputs and cancellation.
- Full regression after stage integration: 45 reader tests and 75 window tests passed, no failures; 21 optional tests skipped. An independent orthographic/perspective review image is available locally in `study-case-audit/priority-families/cs_STGHKN-collision-review.png`; it is a software inspection render, not a Vulkan screenshot or an in-game visual reference.

## Visual course and animation archives

The supplied top-level `STGHKN.farc` uses **FArc stored entries**, whereas `course/stghkn.farc` uses **FArC gzip entries**. Both observed directory variants now open natively. The top-level file has 73 textual A3DA camera/animation/effect resources and 118 distinct external object names; it does not contain the track mesh buffers. Metadata, references and the reason no mesh can be rendered are reported when its children are scanned.

The course archive contains `geometry.bin` (40,257,664 decoded bytes), `object.bin` (2,403,584 bytes) and `texture.bin` (70,137,068 bytes). Geometry has 2,377 buffers; object metadata has 1,143 objects and 2,377 mesh descriptors. Every descriptor's geometry ID, flags, declaration and vertex count is checked against its geometry buffer. Source names are retained for grouping.

The second geometry header word is a **table offset**, not a format version: it is 8 for the supplied car and 32 for this course. Supported flags are 0x03/0x13/0x33/0x113 with declarations 18/26/34, optional UV0 and validated UInt16 restart strips. Extra attributes and original packed normals are not imported; rendering uses calculated normals.

| Preview group | Buffer parts | Buffer vertices | Triangles |
|---|---:|---:|---:|
| Visual course | 2,372 | 1,136,820 | 892,434 |
| Backdrop / sea | 4 | 422 | 383 |
| Sky | 1 | 81 | 132 |
| Total | 2,377 | 1,137,323 | 892,949 |

Grouping uses explicit sky and night-view/sea source names. All groups are available; the initial preview avoids distant backdrop planes dominating framing and occluding the untextured city. Geometry is combined before one scene insertion, avoiding thousands of separate collection additions and buffer rebuilds. Repeated previews of the same group are skipped. Individual geometry remains accessible by opening its resource.

Static source positions and UV0 are preserved. UV-less pieces use zero UVs in a combined group. Materials/TXP texture decoding and A3DA animation/placement are **not** applied; this is not a fully textured or animated game scene. Direct archive previews omit invalid raw-byte mappings into compressed data.

Validation: synthetic pointer-table/object-binding tests; all 73 animation resources; full course decode; RAM geometry discovery; default GLB export; scanner-button-to-collection flow; optional backdrop/sky loading and group deduplication. Independent local reports and review images are in `study-case-audit/priority-families/`. Source game files remain unchanged.

Final regression with the animation archive, collision stage and visual course enabled: **45 reader + 79 window tests passed**, zero failures; 21 optional tests skipped. The local CLion executable was rebuilt; no GitHub release was created.

R-Tuned course topology correction: object.bin mesh draw descriptors are 92 bytes; primitive 5 / UInt16 draws use explicit byte offsets and counts. Validate contiguous full index-buffer coverage and reset strip history and winding at each draw. The course has 2,500 draws across 2,377 buffers: respecting 123 boundaries removes 246 cross-draw faces (up to 1,759.74 source units). Course preview/export: 892,434 triangles; all groups: 892,949. No distance-based face filtering is used. Standalone geometry.bin lacks this draw metadata.

FM1 validation: CAB and RAM discovery of all 30 primary parts, bounded chunk lengths/counts, rejected truncated/out-of-range/unsupported-primitive fixtures, independent strip winding, full multi-select UI preview without duplicate auto-preview, and GLB export of 10,541 triangles. Independent review image: `study-case-audit/priority-families/FM1-car-primary-review.png`. Xbox primitive numbering reference: https://github.com/PatrickvL/Dxbx/blob/master/Source/Delphi/src/DxbxKrnl/EmuD3D8/uPushBuffer.pas . Historical scripts were only read as investigative references; no script runtime or copied implementation is used.

FM1 track survey: Autocross1/QuickPreview1.xb2 expands from six padded zlib members into a 24,880,128-byte CAB v15 with 539 resources (including resnametable/resfullnametable and instance translation/range/scale metadata). It does not contain CARBIN Verts/Subpart chunks. Native track geometry interpretation remains pending. CARBIN source normals are retained where present; missing normals are reconstructed from primary faces (unused/degenerate vertices receive an up-vector fallback for valid export).

## FM1 native track import (Autocross1)

Scanning QuickPreview1.xb2 now follows its combined zlib -> CAB snapshot directly. The same importer accepts the decoded CAB in a RAM document. Source IDs from resfullnametable bind each model descriptor to its vbuff and texture objects. The scan lists 270 supported direct meshes (including optional sky) and 79 conventional textures. Special vegetation/crowd shaders (94 meshes) and packed tree lightmap resources (78) remain explicitly excluded.

Preview imports Float32 XYZ, independent clockwise strips, UV0 and the primary texture/material automatically. Packed UV components decode as `(UInt16 - 32768) * texcoordscale / 65535`, using each vertex layout's UV0 at +16. Primary textures are BC1/BC3 with validated Xbox metadata and exact full mip-chain lengths. Missing normals are calculated and isolated/degenerate normals receive an up-vector fallback.

Primary images and decoded secondary source maps are added to the image library; the texture/image scan can independently extract all 79 supported images. Document-revision caches reuse the decoded CAB and texture pixels; edits invalidate the cache. Shared primary images are counted/uploaded once instead of once per mesh.

FBX retains assigned materials/textures through the existing SDK exporter. GLB now embeds assigned PNG textures, materials, alpha-mask information and source material bindings in extras, sharing image payloads. Additional shader layers (lightmap/detail/terrain blend) are retained as references rather than presented as reconstructed game shaders. Their exact combination is NOT applied yet. This is portable base-color material support, not a promise of the original game lighting or complete vegetation.

Bulk preview updates are grouped: scene entries and textures are staged while the decoder reports progress, then geometry/material/selection inspectors and the viewport rebuild once at completion or stop. Failure and document edits release the batch and retain already-loaded snapshots. Shared texture images are counted once against the scene texture budget and uploaded once per GPU rebuild.

Autocross bulk-load failure root cause: its 42 unique primary texture images require 104.22 MiB decoded, so the previous fixed 64 MiB scene texture limit stopped at 91 meshes. Scene textures and the image library now each derive their retained-image allowance from one eighth of the configured asset budget (1 GiB at the default 8 GiB). Images remain shared; this allowance does not reserve memory. GPU storage-buffer limits are still checked separately at upload.


## FM1 retail Tokyo track

Tokyo/QuickPreview1.xb2 is a 50,323,456-byte input with 23 padded zlib blocks, expanding into a 92,421,120-byte CAB v15 with 8,534 resources and 5,946 mesh descriptors. Retail descriptor +4 contains runtime addresses rather than the debug USER marker; it is not interpreted as geometry. Descriptor/table/resource/index bounds remain validated. Mesh table capacity now follows validated table bounds up to a defensive one-million-entry ceiling rather than rejecting every track above 4,096 meshes.

Additional verified direct layouts: shader 11/14 use 24-byte vertices and texture references +22/+24; shader 15/16 use 20-byte vertices and primary reference +22; shader 18 uses 20-byte vertices and references +22/+24; shader 23 uses 32-byte vertices and primary reference +22. All use packed UV0 at +16 and independent primitive-6 strips. Unknown shaders are counted and omitted rather than blocking compatible geometry. Exact game shader blending is still unresolved.

Native decode/export validation: 5,377 meshes, 861,191 vertices, 509,168 triangles; 2,507 supported texture resources are discoverable. A 108,807,836-byte GLB contains 1,236 unique primary images (208.85 MiB as RGBA) and all compatible meshes. The remaining 569 special/unsupported meshes are explicitly omitted (including vegetation, crowd and shadow variants). These are partial supported-track imports, not complete original rendering.

FM1 decoding now builds a reusable block snapshot once per cached document revision, instead of copying the whole decoded CAB for each mesh. Parsing failures appear in scan notes. Settings > 3D > Import > Flip V for FM1 track UVs applies V = 1 - V to new scan candidates, leaving texture pixels unchanged; preview identity includes the option. Remove old meshes and rescan after changing it. The user visually confirmed that flipped V matches Autocross.


## FM1 complete local track survey

Validated all 33 available QuickPreview1.xb2 files with the production parser and decoder, applying independent U and V flips: 105,160 compatible meshes, 17,014,132 vertices and 11,711,729 triangles; zero decode failures. This is geometry validation, not per-track visual/game-shader verification. 36,515 special/unknown-shader meshes remain omitted across the corpus. Vancouver has two meshes retained without textures because their primary formats are unresolved. Four reverse-course folders have auxiliary files but no XB2 of their own.

Fixed CAB resource count ceilings (retail Nurburgring has 27,955 resources), combined zlib's legacy 128 MiB output cap, and the 128 MiB compressed-input cap for zlib inspection. Whole zlib input and combined output now use one quarter of the configured asset budget; each compressed member retains the existing bounded decompression and checksum validation. Nurburgring expands to 270,084,608 bytes and yields 15,963 supported meshes. The parser retains structural and resource/face bounds checks. Geometry decoding now tolerates unresolved primary texture formats and records their applied=false material binding, preserving usable meshes.

Settings > 3D > Import now offers independently persisted Flip U (U = 1 - U) and Flip V (V = 1 - V); both are snapshotted into scan candidates and preview registry identity. Texture pixels are unchanged. Clear previously loaded meshes and rescan after changing these preferences. Full corpus audit: 116.6 seconds, all 33 geometry files passed.


## MSR alpha and PGR2 native geometry profiles

Native scanning now recognizes the uncompressed MSR alpha track and primary vehicle LOD independently of retail Pak compression. Float32 local centers are applied; vehicle strips are UInt16 length-prefixed and independently reset; track leaf corners carry UV0/color/source-vertex index, so UV seams remain distinct. Supported Asakusa day/night imports each contain 2,187 material groups and 31,944 triangles. Alfa 156 contains 155 groups and 4,432 triangles. Invalid source UVs explicitly disable UVs for the affected material group without discarding its geometry. Special/empty track nodes, later vehicle LODs and texture binding remain pending.

PGR2 indexed tables now expose individual sections with validated offsets and framing. Five indexed corpus files / 65 sections pass extraction tests. The vehicle adapter follows MESH groups and first-LOD subdraws to shared compact GPUD/VB vertices with INFO scale and signed packed UVs. Supplied 240Z: 79 subdraws / 12,038 triangles. Vertices are compacted per draw before export, avoiding zero normals from unused shared vertices. Component/state alternatives are not merged into a guessed assembly. WMSH tracks and raw sequential CTH resources remain separate pending work.

Six versioned research JSON guides under `docs/format-guides` record signatures, fields, dependencies, limitations and corpus hashes. They are analysis/helper documents, not generic executable Mesh Profiles. MSR retail/PGR1 codecs and PGR3/4 model/scene assembly remain unresolved. Discovery notes and the assistant's context guide state these limits to prevent treating compressed bytes or GPU streams as validated geometry.


## MSR named-container correction from the Discord reference corpus

The October 2020 archive areas-B9C3A.rar includes the same Asakusa alpha data already supplied, plus Shibuya and Shinjuku day/night scenes and exported OBJ references. The native track adapter now descends through both +48 ordinary nodes and +64 named containers, follows declared strip/corner counts, and validates leaf trailers of 0 or 12 bytes. Repeated corners are legal: strip corner count need not be less than the number of unique source positions; every actual vertex index is still checked. Empty nodes remain omitted. No per-track start offsets or hardcoded relocated-index conversion are required for these six alpha files.

Historical source strip counts: Asakusa 43,992 vertices / 37,635 triangles; Shibuya 106,905 / 91,252; Shinjuku 49,258 / 44,730. The rounded historical Asakusa OBJ has one zero-area face; the original binary retains its finite source coordinates and native import preserves all 37,635 source triangles. These records replace the earlier partial Asakusa 31,944-triangle supported-node census. Material/texture binding and the retail Pak/GOTHPOK codecs remain unresolved.


## PGR1 native GOTHPOK decoder — 2026-10-23

The earlier unresolved-codec status above is superseded. BizarreResearch's public QuickBMS reference identified a decoder routine in the user's default.xbe. Static inspection established a 30-token LZ control format; 9XD now implements it in portable bounded C++, without executing or distributing the game executable. Payload begins at +20, not +16. All 15 supplied resources pass native extraction/hash checks; truncated payloads, invalid references, size overruns, cancellation and trailing data are tested. PGR2 regression still passes five archives / 65 sections. Geometry assembly remains pending.

Independent decoded Porsche911 inspection finds five LODs: 61/61/46/10/7 groups, with finite positions and bounded indices/subdraws. First LOD has 10,383 group-referenced vertices and 11,101 non-repeated-index strip triangles before geometric/visibility filtering. These are structural measurements, not a validated assembled preview.

References: https://github.com/bartek360/BizarreResearch/blob/main/pgr1_unpack.bms and https://reshax.com/topic/1466-project-gotham-racing-1-map-convert/


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
