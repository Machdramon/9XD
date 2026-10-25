# NFS Underground 2 PC and R-Tuned materials

The native scanners expose material groups as directly renderable results. Use **Scan for 3D Data** and **Render All**. Each loaded collection entry has one base-color image, preserving the source draw boundaries; no texture atlas or guessed nearest image is used. Supported images are also available through **Scan for Textures / Images** and the ordinary image library. GLB and FBX retain supported material images through the normal export pipeline.

## NFSU2 PC

The solid texture table (0x134012) maps material texture slots to hashes. Each validated material's index start/count limits the faces receiving that image. Scenery instance transforms and the configured orientation are retained. Matching uncompressed version-5 TPK entries carry explicit hashes, image/palette placements, logical dimensions and 32-byte format records. The decoder supports observed DXT1, DXT3 and paletted images using the existing image codecs.

Images are resolved from the current binary, a sibling TEXTURES.BIN, CARS/TEXTURES.BIN when opening an installed car, and GLOBAL/GLOBALB.BUN in the corresponding installation root. Keep these files in their original game directory structure. External files are read only, bounded and checked for changes; a RAM document without a source path cannot discover installation companions. Conflicting same-hash images are left unresolved rather than selecting one arbitrarily. Missing images preserve untextured geometry with source hash metadata. Bounded JDLZ- and HUFF-compressed TPK records are supported. Vertex-color lighting, runtime paint/decal/light-state substitution and original shader layers remain separate work.

Whole-scene material results exclude the sky as before. Sector alternatives remain selectable and are excluded from Render All when a whole-scene result is available. Preview provenance tracks individual source instance/material index ranges, preventing whole-scene and sector selections from loading the same faces twice.

## R-Tuned

Open the original FArC containing geometry.bin, object.bin and texture.bin. Object materials use 1200-byte records, eight 120-byte texture states, and explicit per-draw material indices. Geometry draws retain their own independent triangle-strip boundaries. The 92-byte draw descriptor supplies index start/count; material assignment never joins strip histories across draws.

TXP3/4/2 records provide the base mip and source texture index. Observed DXT1, DXT5 and RGB8 images are decoded natively. Only the color role with verified UV0 is assigned as diffuse; normal, specular, environment and other roles are not substituted for a missing base color. UV-less pieces remain untextured. Texture addressing is retained; advanced blending, texture animation, shader lighting and runtime placement remain separate work. Backdrop and sky material groups are optional and excluded from Render All.

Validated fixtures include the RX-8 archive and course/stghkn.farc. The car exposes 19 material groups (18 with supported base images); the course exposes 217 groups (215 textured), retaining 892,949 source triangles across the course, backdrop and sky.

## Research references

Format definitions were checked against local files and primary project sources: [NFS-ModTools](https://github.com/NFSTools/NFS-ModTools/tree/master/Common/Textures) and [MikuMikuLibrary](https://github.com/blueskythlikesclouds/MikuMikuLibrary/tree/master/MikuMikuLibrary). These projects are references, not runtime dependencies. Game data, exported models and study images are not included in the repository.

## Validation — 25 October 2026

Twelve test cases/lifecycle checks passed with no failures or skips. Actual scanner-to-UI image extraction and Render All were checked for the RX-8 and STREAML4RH, including repeated loading without duplicates. Both fixtures export successfully to GLB and FBX. Hakone exports to textured GLB. Full native decoding of STREAML4RA yields 3,104,048 triangles across 1,561 material groups, 1,549 textured; this is not a complete GPU/large-map export test. The 240SX yields 150 groups, 55 textured, with 29 HUFF image records explicitly unsupported. Shared and unused image records are counted separately from textured groups.

NFSU2 material splits preserve the original solid's generated/source normals. Undefined zero sums receive an incident-face fallback, or an up normal if no face defines one; the fallback count is stored in material metadata. This avoids rejecting exports while preserving source topology. Unused palette-placement values are not interpreted as pointers when palette size is zero.

## Native HUFF decoder

`nfs_huff.h` implements the EA canonical Huffman alphabet, cyclic symbol deltas, escaped literals, repeated-byte commands and mandatory end marker. Both wrapper and inner output lengths must agree. Input bounds, output caps, invalid trees/deltas, missing history, truncated commands and cancellation are checked. Prefix-sum variants are covered by synthetic tests. No machine-code blob, external executable or QuickBMS runtime is used.

The local 240SX's 29 HUFF payloads decode completely, with matching image metadata/hash references. The larger local corpus checks 636 HUFF blocks across 32 car texture archives; every block uses the observed 0x30FB mode. Tests include a SHA-256 comparison against an independently implemented Python format probe, repeat/literal fixtures and malformed/truncated streams.

The remaining car references are not all missing stored images. The game's rendering pipeline includes dynamic replacement tables for paint, decals, glass, damage and lights. Applying one arbitrary stored map to every such reference would be incorrect, especially when different part styles share a placeholder. Source hashes remain inspectable and unassigned when that state cannot be established. Research reference: [NFSU2Unlimiter CarRenderInfo](https://github.com/nlgxzef/NFSU2Unlimiter/blob/master/CarRenderInfo.h). HUFF layout was investigated using local payloads and static analysis of the [EA HUFF reference](https://github.com/LittleBigBug/QuickBMS/blob/master/src/included/ea_huff.c); reference machine code is not included in 9XD.

### HUFF follow-up validation — 25 October 2026

The final suite passes 14 checks with no failures or skips. HUFF validation covers 636 source blocks in 32 installed car archives. The 240SX now decodes all 29 previously unsupported HUFF records; adding the shared CARS/TEXTURES.BIN raises its applied-image coverage from 55/150 to 72/150 material groups. All 830 available car/shared image records decode, with no unsupported-compression results. Available-image counts include unused images and are not applied-material counts.

A separate Windows/Vulkan test loads all 1,561 material groups of STREAML4RA (1,549 textured), waits for asynchronous scene preparation and GPU upload, and verifies a submitted frame with 3,104,048 triangles on the RTX 4060 Ti. Device-local geometry allocation is 1,558,366,080 bytes; texture preview divisor is 1. This does not establish game-identical shaders or a full-scene export guarantee.
