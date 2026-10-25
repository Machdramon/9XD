# March HK11 capture / TXRD2

The user selected a yellow Nissan March G# HK11 in the TXRD2 model selection screen. Captured with PCSX2 2.8.2 using Shift+F8. Capture serial: SLUS-21394.

Original: `C:\Users\Jarky\Documents\PCSX2\snaps\Tokyo Xtreme Racer Drift 2_SLUS-21394_20261025165628.gs.zst`

Compressed SHA-256: `1c33568a0c210f3ccb4cd2caaef5d46a0494d31612420ae94b2f4dc6d302ab67`

Local evidence: `march.gs.zst`, decompressed `march.gs`, `march-reference.png`, `gs-state.bin`, `gif-register-audit.json`.

## Source correlation

Source candidate: `F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]\2022\000002a0.sdb`.

Its complete file SHA-256 equals the existing `study cases/txrd2/car/000002a0.sdb` test fixture. This is the fixture with 54 geometry groups / 8,328 admitted triangles.

The second section has six bounded headerless records: three with pixels/palettes and three 48-byte reference-only records; 16 bytes remain at the end. The records must keep their source slots. They are not ordinary complete TIM2 files.

Two base-level surfaces exactly match the initial captured GS VRAM after GS indexed address decoding:

| File picture offset | Dimensions | Indexed-pixel SHA-256 |
|---|---|---|
| 0x44140 | 512 x 512 | `4189a06a123dc63ad0957ed09b9b8ac267801183b385f6ba7962b41cf148666e` |
| 0x88aa0 | 256 x 256 | `a94e388671503dd7a8553ecf6a1c9a46ccc6de4935a986dd467521086ceee687` |

Both corresponding 256-entry RGBA32 palettes also match byte-for-byte, using the captured CSM1/PSMCT32 palette descriptors. See `palette-comparison.json` for hashes. The pixel and palette addresses share a relocation delta of 0x3300 GS blocks (256 bytes each) in this capture. This is a measured runtime relocation, NOT a constant to hardcode into a general decoder.

This ties stored image data to the visible March family. Shared images do not uniquely establish which of the family's geometry/upgrade/LOD variants was loaded. The original file being identical to our fixture does not by itself prove a complete per-mesh runtime binding.

## Limits / next work

- The third stored 128x128 image and the three reference-only records did not match an observed same-size relocated TEX0 binding in this capture. They are unresolved, not proven absent or generated.
- Captured TEX0 configurations include the menu/floor/effects, not just the car.
- Chronological transfer/draw tracking is still needed for complete material assignment, tint, reflection and transparency behavior.
- Add native headerless-bank parsing and bind verified materials in 9XD; no 9XD source changes or public uploads were performed in this capture step.
- A Hakone daytime capture can provide a contrasting track sample once the user loads it.
