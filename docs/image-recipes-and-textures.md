# Image recipes, console presets and mesh textures

## Reusable recipes

The 2D Settings / Image Calculator provides **Save Recipe...** and **Load Recipe...** (`.bwir`). Recipes store all interpretation fields plus batch count and step, including exact 64-bit offsets. They do not embed source binaries or automatically open source paths. Loading validates the version, types, ranges and supported layouts before changing controls.

Choose the intended source after loading, then Calculate / Add to Library. Existing `.bwil` libraries still store decoded image snapshots.

## Presets

The Preset list covers standard/container images, PC RGBA/BGRA, PS1 BGR555/indexed, PS2 RGBA/GS/CSM1, PS3 Morton BC1, PS4 thin BC7, Xbox Morton, Xbox 360 tiled BC1, NES CHR, SNES planar, GameCube CMPR and Wii RGBA8.

Presets are starting points rather than format detection. Check dimensions, source/palette offsets, row pitch, palette encoding and layout against the actual file. They do not claim support for every game texture on a console.

Raw and DDS DX10 decoding additionally supports RG8; R16/RG16/RGBA16 UNORM; R16/RG16/RGBA16 FLOAT; and R32/RGBA32 FLOAT. Floating-point previews clamp channels to 0–1 and convert to 8-bit display values. Raw data honors byte order. Existing limits (32 MiB of source image data, four million output pixels) still apply.

## Assign a texture

1. Select the target mesh in Mesh Collection. It must have UV coordinates.
2. Open or calculate an image in Texture / Image Collection and select the decoded image.
3. Click **Assign to active mesh**. Its texture appears in Mesh Collection, behind the active mesh's UVs and on its faces in the Vulkan 3D view.
4. Use **Remove active mesh texture** to clear the assignment.

Each mesh holds its own image snapshot, independent of library selection and other meshes. Removing an image from the library does not remove a texture already assigned to a mesh. Scene texture images are limited to 64 MiB. Geometry and textures remain session data; this feature does not add scene-file persistence.

Rendering uses nearest filtering, repeated UVs and V-up UV coordinates. Transparency is basic alpha blending with near-zero-alpha discard; translucent faces are not depth-sorted. This is not a full material/PBR system. Existing model export does not yet embed these image textures.

## Console support boundaries

| Family | Available paths | Remaining limitations |
| --- | --- | --- |
| PS1 | TIM, BGR555 and indexed palettes | Game-specific archives and layouts require their own recipes/readers |
| PS2 | TIM2, RGBA/indexed, GS pages/16S and CLUT controls | Unrecognized TIM2/GS variants are rejected |
| PS3 | GTF 2D, raw/Morton and BC data | Volume/cubemap GTF variants are not supported |
| PS4 | Single-texture GNF base level, linear/1D thin; raw BC1–7 | GNF arrays, cubemaps, mips and macro tiling remain unsupported |
| Xbox | Raw/Morton and DDS | Game archives and proprietary resource containers need separate readers |
| Xbox 360 | Raw Xenos tiled and byte-swap controls; DDS | Packed resource containers and hardware-specific variants need samples |
| NES / SNES | CHR/planar/Mode7 plus supplied RGB palettes | NES PPU color IDs are not automatically converted into RGB palettes |
| GameCube / Wii | TPL, native tiled pixels/palettes and CMPR | Other resource/archive containers are not automatically unpacked |

Validation includes recipe round-trips and rejection of invalid files, raw/DDS pixel fixtures, independent mesh assignments, the existing decode suite, and a native Vulkan textured-render test with captured GPU frames.
