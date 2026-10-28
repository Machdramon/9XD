# Wangan Midnight PS3 (XMD / XTD / X3D)

Updated: 2026-10-26. Author: Claude. Status: native reader in 9XD (`src/wm_ps3_core.h`, `src/wm_ps3_xmd.h`, profile `native-wm-ps3-xmd`).

Sources (read only, not executed or bundled): Gato's Lair Discord `python/nines_py` → `wmps3_nines-DBE04.py` (2021, geometry/normals/strips/UV sets, XTD→DDS); Xentax attachment `9072/Wangan Midnight.zip` → `Wangan Midnight.ms` by mariokart64n (2015, XMD/XTD dispatch, block types, material/texture names, GTF→DDS). Samples: `study cases/wm ps3/JZS161.CAR`, `AREA_9GO_BUIL_CUT_9GO_FREE_001.GRID`.

## Layout (all big-endian except the XTD tables)

| Structure | Fields |
| --- | --- |
| XMD `XMD\1 001\0` | u32 @8, child count @12, child offsets @16 (relative to the XMD), sizes after 16-byte alignment. Children: X3D model, XTD, nested XMD. |
| XTD `XTD\0 002\0` | **little-endian** entry count @8, entry offsets @12. Entry: u32 size, `GTF\0` (or 0), u16 bank @8, u16 index @10. Real entries carry a standard PS3 GTF at +16; 16-byte entries are references to (bank, index). |
| X3D model | `X3D0037` @+8; block table: count @+0x88, offsets @+0x90 (relative to the model). Block header: u16 type, u16 string index (1-based), u32 size, u32, u32. |
| 0x15 | name string |
| 0x07 material | 4 u32 (entry count last), entries {type, size, u1, u2}: type 8 `tx_name_0`/`tx_name_1`/`ref_name` → u1 = texture slot (courses); type 9 → u1 = texture slot (cars); type 2 colours, 6 named parameters, 12 environment |
| 0x0C texture slot | u32 = entry index in the sibling XTD |
| 0x05 shape | `u3` sub-blocks after the block: type 6 {material index, geometry ordinal} |
| 0x08 geometry | 80-byte header (stream count at +60), streams: 8 u32 header {0x000A0000, size, component, data type, ?, count, ordinal, UV set}, data at +32. Component 0/type 12 = u16 strip indices with 0xFFFF restart; 2/26 position float3; 4/26 normal float3; 5/25 UV float2 (several sets); 3/27 colour |
| 0x04 node | transform hierarchy (identity on the car sample) |

Texture chain: material → slot → sibling XTD entry → if reference, the real entry with the same (bank, index) anywhere in the file → GTF (DXT1/3/5) decoded by the existing GTF reader.

## Results

| File | Geometries | Vertices | Triangles | Textured from embedded GTF |
| --- | --- | --- | --- | --- |
| JZS161.CAR (Toyota Aristo) | 162 in 103 shapes | 35,648 | 46,376 | 121 (8 GTFs: 6 × 512/256 px, 2 × 1024 px); body/mirror/inner/shadows have no texture slot |
| AREA_9GO_BUIL_CUT_9GO_FREE_001.GRID | 40 | 10,485 | 6,078 | 0: all 78 slots reference external course banks (banks 1/2) |

Checked live in 9XD: the car renders complete (Render All, 0 failures) and its interior, lights and plate textures decode. Neon/LED/spot tuning shapes are marked as variants (selected explicitly).

## Open

- Course banks: load the course's shared XTD (same bank numbering) to texture GRID files; needs a sample folder.
- Several UV sets (lightmaps, `tx_name_1`, `ref_name`) and material parameters are recorded but only UV set 0 / the first texture is used.
- Customization variants (muffler/aero alternatives) overlap; selecting a default set needs the car setup data.
- Node transforms are not applied (identity on the sample); skinned/character models untested.
