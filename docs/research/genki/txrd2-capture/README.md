# TXRD2 / PCSX2 reference capture

Captured locally on 25 October 2026 with PCSX2 2.8.2. The visible game selection was a red Mazda AZ-1. No data was uploaded and no 9XD source files were changed in this capture task.

## Preserved evidence

- Original capture: `C:\Users\Jarky\Documents\PCSX2\snaps\Tokyo Xtreme Racer Drift 2_SLUS-21394_20261025164204.gs.zst`.
- Workspace copy: `az1.gs.zst` and screenshot `az1-reference.png`.
- Serial: SLUS-21394; GS state version 9.
- Compressed SHA-256: `2f7f287b74665db7f13e6010b77359cd5b18b33b51c13317879852cc6819c06d`.
- Decompressed size: 15,460,687 bytes. This is a graphics capture, not an EE/CPU RAM dump.
- Parsed 4,472 transfer records, 8,652 GIF tags and 4 VSync records without trailing parse errors.
- 50 distinct TEX0 register values. These are configurations, not a count of unique images or missing car textures; menu, floor and reflection draws are included.

## Original-file investigation

The old Discord car list associates AZ-1 with resource indices 295–305. This led to the candidate below, but the list-to-filename mapping and vehicle identity are not independently established by this observation:

`F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]\2022\00000127.sdb`

Its second bounded section contains seven headerless picture/reference records: four with pixel/palette data and three 48-byte records without pixels. Parsing reaches the last 16 bytes. This explains why searching for a TIM2 file signature alone finds no images in this candidate. The empty records must retain their original slots; their reference semantics are not yet resolved.

An independent local probe decoded the first 512x512 indexed atlas to `az1-source-picture-0.png`. That filename denotes a candidate from this capture investigation, not proof that the atlas is specific to the AZ-1.

One 256x256 indexed surface at source offset 0x969e0 exactly matches the initial GS VRAM at byte base 0x34c000 after GS address decoding. Indexed-pixel SHA-256:

`52e6e48074e18d7d797be66c9f03edced357f9491ff563cf8c4976543392418e`

The equality validates that pixel payload/layout. It does not establish palette identity, unique vehicle identity (the map could be shared), or per-mesh material assignment.

## Next implementation steps

1. Add a bounded headerless Genki image-bank reader that preserves pixel-less slots and rejects malformed sizes.
2. Replay texture uploads and palette/state changes in chronological order; initial VRAM alone is not sufficient for every draw.
3. Correlate TEX0/CLUT/CLAMP and material group metadata, then validate palette, UV orientation and modulation against the capture.
4. Turn verified relationships into native 9XD profiles, keeping unresolved references explicit. Emulator capture becomes reference evidence, not a runtime dependency.

Format references: [PCSX2 2.8.2 GS dump layout](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSDump.h), [capture writer](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSDump.cpp), [GS state serialization](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSState.cpp).
