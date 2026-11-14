# RayCity SCRIPTCE cell geometry

Updated: 2026-11-14. Owner: Codex. Native geometry integrated, tested and installed locally.

## Evidence and scope

Owner folder: C:/Users/Jarky/Documents/study cases/raycity. 71 files:35 cell .sce files,35 matching _tex.sce banks under f1/, and terra.sce. Originals read only. v1 cells contain a plain typed scene body despite obfuscated leading tables. Owner confirmed terra.sce is a car (v3), not world metadata. Its geometry serialization remains undecoded. Client now supplied separately at C:/Users/Jarky/Downloads/RebirthRC/RebirthRC; static inspection only.

Independent local binary measurements across35 cells identify448 placed parts. All per-part local indices are in range. Each partition covers the shared vertex/index arrays exactly, without overlaps or holes. Row-major3x4 placement puts every vertex inside its declared conservative world AABB (maximum measured excursion0.0000611). Transposing the basis fails containment for99 parts; tight-AABB equality would incorrectly reject conservative rotated bounds.

## Measured v1 layout (little endian)

| Offset | Field |
|---|---|
|0|UTF-16LE SCRIPTCE Data Format v1.0s, padded to0x80|
|0x80-0x2ff|Obfuscated tables/directory; deliberately not interpreted|
|0x300|Serialized root AA47 / class0x0a2202a0 / id0|
|0x317|Serialized scene AA47 / class0x17f10415 / id2|
|0x31f|UInt16 part count|
|0x321|Part descriptors,112 bytes each|
|Descriptor+0|12 Float32 values: row-major3x4 local-to-world matrix|
|Descriptor+48|6 Float32: conservative world minXYZ/maxXYZ|
|Descriptor+72..97|Flags/unknown state/padding; +80 UInt32 material selector (1-based)|
|Descriptor+98|UInt16 vertex count|
|Descriptor+100|UInt32 start vertex in shared buffer|
|Descriptor+104|UInt16 index count, divisible by3|
|Descriptor+106|Unknown/padding; never treat as high index-count bits|
|Descriptor+108|UInt32 start index in shared buffer|
|After descriptors|UInt32 global vertex count, then stride40 vertices|
|Vertex+0|Float32 XYZ local position|
|Vertex+12|12 bytes often unused/uninitialized, even NaN; not trusted as normals|
|Vertex+24|Float32 UV0, retained unchanged (including tiling)|
|Vertex+32|UV1 for named CompleteMap baked preview (original shader not reproduced)|
|After vertices|UInt32 global index count; UInt16 triangle lists with part-local indices|
|After indices|Serialized material/state and collision structures; not geometry buffers|

Do not interpret header byte5 as triangle count: this happened to match the five-face first sample, but fails other cells. Do not search arbitrary floats and auto-generate faces. Use each declared part's own local indices and transform. Convert source Z-up to viewer Y-up as(x,z,-y); retain source triangle order, derive preview normals from actual transformed triangles and retain source vertex/index byte offsets. Source handedness/culling and original shader behavior still need visual comparison.

## Integration and checks

- [x] src/raycity_cell.h: bounded native scan/decode with counts, range coverage, finite transforms/positions, conservative bounds and local-index validation; cancellation and source-patch/stability checks. Invalid UV0 disables UV preview while preserving valid geometry. Uninitialized normal fields are ignored.
- [x] Connected discoverAssets -> meshCandidates -> MeshProfiles::decode. Render Default Set/Selected and normal mesh exports use the existing path. Recognized SCRIPTCE files never enter generic VIF/float-geometry fallback, including unsupported v3 vehicle/object serialization.
- [x] Focused4 passed/0 failed/no skips (setup/cleanup included): synthetic multi-part placements/local indices/provenance, NaN normals ignored, invalid UV fallback, truncation/invalid ranges/index/cancellation/stale candidates, RAM discovery/production decode, and real35-cell production scan/decode/GLB export. Corpus:448 parts,279,747 vertices,158,186 triangles. All declared vertices/indices accounted for in every cell; every exported GLB has the expected face count. Evidence: build/windows-local/raycity-focused.txt and ignored build/raycity-corpus.json.
- [x] First export check caught zero generated normals on degenerate/unused geometry. Added finite unit fallback for undefined normals, preserved original triangles, and a synthetic degenerate-face regression; final checks above pass.
- [x] Full window suite140 passed/0 failed/87 optional skips (build/windows-local/raycity-full.txt). Development EXE rebuilt successfully (build/raycity-app-build.log). Explicitly refreshed main.cpp timestamp because old Ninja dependency state had not rebuilt its object when included headers changed; no content changes to previous main.cpp work.
- [x] Live updated 9XD: opened f1/f1_1000_1000.sce, Scan for3D Data produced25 direct mesh candidates; first candidate auto-previewed8,004 vertices/5,040 triangles. Render Default Set loaded24 more +1 existing,0 failed/0 omitted, total34,035 vertices/20,374 triangles, visible in Vulkan.24 texture warnings are expected for newly loaded untextured parts. Left this cell rendered; no manual decoder setup or AI required.
- [x] Texture-bank inventory found229 DDS signatures with valid legacy header size:123 DXT1 and106 DXT3. This is an inventory, not verified material coverage. UV0 has zero nonfinite components across the whole corpus.
- [x] Named material graph and automatic local/shared bank binding integrated below. Original shader state, collision, v3 car serialization and broader versions/vehicles remain pending.

## References

Public research reference consulted: https://github.com/xpoi5010/JMDReader (GPL3). Its JMD XOR/directory code is not an SCE mesh reader and was not imported. Its serializer observations were cross-checked against local AA47 object records. Native geometry layout/reader above was independently derived from the owner's binary corpus. No third-party runtime or copied library is required. No game binaries/textures are added to the repository.

## Reproduce

Open C:/Users/Jarky/Documents/study cases/raycity/f1/f1_1000_1000.sce in the updated local EXE. Go to3D, Scan for3D Data, then Render Default Set.25 parts/34,035 vertices/20,374 triangles expected. Cells can be exported with the existing model exporter; all35 were regression-tested as GLB. For corpus tests set WORKBENCH_RAYCITY_ROOT to the study folder and run window_tests rayCityCellContracts rayCityCellCorpus. Optional WORKBENCH_RAYCITY_REPORT writes a counts-only JSON receipt. Original inputs are read only.

No in-game visual reference was supplied; silhouette/culling and original shader fidelity still need owner review. terra.sce v3 is a car, as clarified by the owner; lack of a geometry result reflects unsupported serialization. Whole-map deduplication across neighboring cells and other versions/vehicles are unverified. No source assets, commit, push or release.


## Named texture integration (2026-11-14, native exports and live render verified)

Native implementation: src/raycity_cell.h. Flat v1 archive only:32-byte descriptors begin0x100, XOR each LE32 word with0xdfa47b56*(wordIndex+1). Root IDffffffff, offset in256-byte blocks, stored size, decoded size, mode5, checksum, two reserved words. Texture entries mode0/plain DDS; descriptor ranges/IDs and directory file sizes checked. Root directory key is recovered for the measured zero-folder variant: first ciphertext DWORD is seed; 16-word repeating key is seed+0x8473fbc1*i. Require Adler32(initial0) match and complete structural parse. UTF16LE NUL base names followed by extension, property, ID and size; only DDS accepted. This is bounded independent local implementation; no GPL library imported.

Material serializer: first class0x1a790496 after index buffer, count UInt16 immediately preceding (includes null slot0). AA47 defines objects, BB47 references objects. Two optional texture strings with AA27 definitions/BB27 references, then53 bytes of state not yet interpreted. All35 cells parse412 nonnull records, all448 part selectors fit. Names match local banks and the supplied client's field/common_f1/tex.sce (60 shared named textures).

Primary uses UV0; *_CompleteMap is a full-color baked atlas (one extracted image visually inspected), previewed unlit with UV1. This is a baked preview, not proof of original shader reproduction; both named images are retained when found, provenance records role/channel/source bank and gameShaderApplied=false. No UV mirroring is introduced. Automatic external lookup is relative to the opened cell; copied study cases have their sibling banks but not the game's shared bank. No machine-specific client path hardcoded. RAM-only documents preserve geometry without guessing texture directories.

Focused regression5 passed/0 failed/no skips, includes bank checksums/truncation/cancel/named lookup, actual material references, missing-sibling geometry preservation, native35-cell textured GLB exports and13 car DDS production decodes. Study copy423/448 textured previews (94.4%),185 primary textures; absent25 pieces are shared-only. Client70 files match study bytes; automatic ../common_f1/tex.sce gives448/448 textured previews and448 primary image matches. Receipt build/raycity-textured-client-corpus.json uses the historical key texturedBakedPreview for total previews, including primary-only draws. No claim of100% shader reproduction.

EXE rebuilt (build/raycity-app-build.log). Live client central cell25 candidates,24 added+1 existing,0 failed/omitted/texture warnings,34,035 vertices/20,374 triangles visible with textures in Vulkan. Left open for owner review. Full suite141 passed/0 failed/87 optional fixture skips (build/windows-local/raycity-materials-full.txt); git diff --check passed. No commit/push/release. terra.sce has13 ordinary DDS images but v3 geometry/material directory remains pending; never associate those13 images by ordinal. Trace SCE-specific version dispatch next. Client x86 archive routines inspected at VA0xd0c90a (key expansion),0xd0c94d (XOR),0xd142e3 (archive header validation); their header interpretation is not yet validated for SCEv3 and is not production code.

## Owner visual rejection and car follow-up (2026-11-14)

Owner reports previous map textures incorrect. The448/448 statistic was file availability, not appearance validation. Primary texture+UV0 is now prioritized; CompleteMap+UV1 is fallback only. Original shader still pending; correction under focused test, owner comparison required.

Car diagnostics: ignored build/raycity-car-diagnostics/terra-textures.png and13 separately decoded PNGs. The1024x1024 DDS at0x150f00 is a channel-coded mask; raw DDS decode success does not imply albedo. Root directory contains terra_base.pmt/terra_color.pmt and corresponding terra_base_s.tds/terra_color_s.tds. Do not use masks as diffuse paint. Native geometry/material rendering of the car remains unsupported.

Root directory0x1400:10 folders (dooropen,frontbumper,headlight,hood,icon,mainspoiler,rearbumper,rearlight,roof,skirt),8 files (body_0/body_1/body_2.0m,mesh.xml,terra_base.pmt,terra_base_s.tds,terra_color.pmt,terra_color_s.tds). Directory first-word recovery requires known/structurally validated folder count10; key reproduces archive filename key0x4acf0fe1. Frontbumper directory0x1a00 has default_0/1/2 and rbrc_001_0/1/2 variants plus list.xml and material/mask pairs. Protected table wrapping remains undecoded: do not use recovered names to guess offsets by ordinal.

Actual mesh objects AA47/class0x0fdb0347 are present in plain car payloads; prior unsupported status does not mean every geometry byte is encrypted. Simple variant at0x1d00: count UInt16+11, descriptors28bytes from+13; descriptor material UInt32, vertices/indices UInt16, then five presence/start UInt16 pairs. AABB six Float32 follows descriptors; positions/count, normals/count, optional array/count (zero in first object), UV/count and local indices/count follow. First block428 vertices/1032 indices, XYZ bounds[-.9472457,-2.2316484,.067954]..[.9472464,-.7673981,.824585], positions0x1d43,UV0x4569,indices0x52cb,end0x5adb. Identical geometry repeats0x5b00/0x9900. Counted multi-material/transformed variants and protected body need separate handling; no native adapter or complete car exported. Useful next offsets0x29a00 (2 descriptors),0xb2100 (3),0xd6400 (transform),0xf6800 (protected main body). Never concatenate repeated variants into one claimed complete vehicle.

Primary-preview correction validation: focused5/0/no skips with client35 cells/448 texture lookups, UV0 verification and textured GLB exports (build/windows-local/raycity-primary-focused.txt). Owner closed9XD; local EXE relinked successfully, git diff --check clean. No new live appearance confirmation or complete car render. Full suite was not repeated for this priority-only change.

Owner confirmed vertical UV inversion: format-local V=1-V added for UV0 and selected UV1, unchanged U and unchanged DDS pixels. Applied to preview/export; sourceBindings.flipV=true. Existing loaded meshes require re-rendering. Explicit unsupported v3 evidence row added to3D scanner; no car mesh importer claimed. Focused5/0/no skips with real35-cell corpus, unchanged U/inverted V and explicit unsupported v3 evidence assertions; textured GLB export and counts preserved. EXE linked while9XD closed, diff check clean. No live appearance comparison yet.
