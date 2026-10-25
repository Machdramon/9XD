# Fast and Furious: original Xbox prototype static geometry

The native `native-faf-arcx` decoder reads uncompressed Eutechnyx ARCX banks directly. Open an ARC in the game's `FAF/xbox/data` directory, use **Scan for 3D Data**, then **Render All**. No AI or external converter is required at runtime. `03IS300_LOD0_GRA.ARC` is a convenient initial visual check; `TOKYO004_1A.ARC` is a supported track sector.

## Supported data

- Bounded 128-byte header, 16-byte entry directory, native entry-name table and relative payload offsets.
- Mesh type 9, index-buffer type 15 and vertex-buffer type 16. Explicit material groups, buffer references, draw counts and original names are retained.
- Xbox primitive 5 (triangle list) and 6 (triangle strip), including per-draw winding and degenerate-triangle removal. No strips cross draw boundaries.
- Verified position prefixes for native layout/stride pairs (2,20), (3,24), (8,20), (8,28), and (6,24). Source positions must agree with the mesh bounds and every index must stay within its declared group range.
- Static object, multi-object and hierarchy nodes. Sound/light/FX nodes occupy the same parent-index space and are included as transform ancestors without converting effect payloads into meshes. Parent bounds and cycles are checked.
- Native hierarchy transforms are baked into extracted geometry, with reflected winding handled explicitly. Source position and index byte offsets remain available for inspection.
- Source mesh and material names. Normals are generated from recovered geometry. Primary color textures and signed-short UV0 are supported for layouts (2,20) and (3,24), normalized by 2048 with top-left origin. Morph animation, skinning, procedural sprites and external wheel assembly are not implemented by this decoder.

## Variants and loading

Some banks contain several body kits or LODs simultaneously. Source UPG variants remain visible and individually selectable; **Render All** excludes upgrade alternatives when stock UPG0 parts exist. Locally decoded meshes with no active placement in a bank that also contains placed objects are likewise kept for explicit inspection and excluded from automatic loading. This prevents unused LOD resources from appearing at the origin. Standalone geometry banks without any placements can still render in local coordinates.

Decoded catalogs are immutable and cached per document/revision, with source snapshot checks, explicit patch hashing, cancellation and memory-budget checks. Source files are read only. Repeating Render All skips existing unchanged parts through the shared preview registry.

## Validation

The local July 7, 2006 Xbox prototype contains 516 banks whose header declares static meshes. All 516 passed the bounded native parser after including effect nodes in hierarchy indexing. This means static geometry structures are supported, not that every runtime effect, external dependency or complete scene has been reconstructed.

IS300: 57 extracted material/draw parts and 5,312 nondegenerate triangles. Its native hierarchy produces a recognizable car body; complete tyres/rims depend on other resources. The scanner/UI test loads all parts, verifies duplicate suppression and exports GLB and FBX. Additional decoded fixtures include the NSX, Ford GT, Mustang, Supra, Silvia and several track sectors. Tokyo sector `TOKYO004_1A.ARC` is loaded through the UI and exported to GLB. Corrupt directory counts, out-of-range indices, nonfinite positions, cyclic parents and stale candidate identities are rejected.

`TUNESHOP_GRA.ARC` has no supported static mesh entries and is explicitly reported as needing a separate skinned/sprite resource decoder. No geometry hypotheses are substituted for recognized unsupported ARCX banks.

Optional tests use `WORKBENCH_FAF_ROOT` pointing at `xbox/data`. `WORKBENCH_FAF_EXPORT` can select a local folder for audit reports and review GLB/FBX files. No game data is bundled with the source.

## Research provenance

The prototype includes `XBOXArchiveSizes.txt`, listing runtime classes and sizes. The ARCX container and Xbox-specific mesh/buffer schemas were cross-checked against local files and the partial [ArcBank template published by Doliman100](https://reshax.com/topic/848-eutechnyx/). The [Technyx project](https://github.com/PredatorCZ/Technyx) documents the Eutechnyx format family; its public tool advertises PC mesh conversion, which is not equivalent to support for this Xbox prototype. Reference files were read for format research and are not executed or distributed as dependencies.

## Materials and images

Image entries (1) and external references (52) share an ordered index space. Material entries (2) retain their names and pass count; the verified primary image index at byte 57 is resolved relative to the source image base. Repeat/clamp addressing is retained in the Vulkan viewport, GLB and FBX. GLB supports mirror addressing as well; FBX falls back to repeat for mirror. Unsupported material layouts are reported rather than assigned a guessed image.

Xbox BC1/BC2/BC3, swizzled BGRA8, ARGB1555, ARGB4444, A8 and indexed-eight-bit images are decoded with bounded spans. Indexed images preserve their source palette banks; the base palette is used on meshes. The 2D scan exposes native images and alternative palettes separately. Local image decoding is cached per document, so shared draw instances do not repeatedly decode pixels.

Only verified static UV layouts receive a texture. Reflection/generated-coordinate layouts remain untextured. Source shader passes, dynamic paint, environment reflections and externally referenced images are not emulated; a recovered base-color image is not a complete recreation of the game's material. GLB and FBX embed the recovered images; FBX stages PNG files temporarily during export.

The material validation run decoded 2,887 distinct embedded images without errors. IS300 loaded 54 textured UV draws out of 57; Tokyo sector TOKYO004_1A loaded 981 textured draws out of 981. These counts describe those fixtures, not guaranteed coverage for every bank.
