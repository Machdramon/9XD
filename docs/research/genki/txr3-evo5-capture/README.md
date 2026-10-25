# TXR3 Evolution V selector reference

The user selected a Mitsubishi Lancer Evolution V in the TXR3 Time Attack car selector. The model and Evolution V badge were visually verified before capture. PCSX2 2.8.2 Software renderer; GS state version 9; serial SLUS-20831.

Original capture: `C:\Users\Jarky\Documents\PCSX2\snaps\Tokyo Xtreme Racer 3_SLUS-20831_20261025171654.gs.zst`

Compressed SHA-256: `720d5d6f1ec3b7c233ec5627bbc17904b3ee53174da6fc0e87e5cdaa84918ef9`

Saved `evo5.gs.zst`, `evo5.gs`, `evo5-reference.png`, `gs-state.bin`, and parsed register/source comparison reports. No 9XD source files changed and no captures/game data were uploaded.

## Parsed evidence

9,528 transfer records, 19,232 GIF tags and 4 VSync records, with complete record parsing. 99 distinct TEX0 values; these include UI, floor and effect configurations and are not a unique-image count. IMAGE transfer payload totals 5,805,184 bytes, so initial VRAM alone cannot establish every texture used later in the dump.

## Comparison with the tested fixture

`C:\Users\Jarky\Documents\study cases\txr3\000000c4.nfc`

Fixture SHA-256: `a7314adc105f94adfe1f512903e8131e1eaad828bbf435272094ceb6964b7958`

The original fixture-only comparison is preserved in `study-only-matches.json`. Two source entries (surface 35 and 39) match one observed 64x64 image configuration exactly, with full palette equality. Their indexed-pixel payloads have 1 distinct hash. This is shared-image evidence, not full confirmation of this exact car/upgrade variant or the body atlas/material mapping.

The fixture's first atlas contains an Evolution V badge, but that alone does not uniquely identify the loaded variant. The selector reference makes the intended car family explicit.

## Source provenance warning

Inside the TXR3 game directory, `uclless/000000c4.sdb` and `uclless/000002a0.sdb` are byte-identical to the corresponding TXRD2 `2022` files. These mixed research-folder contents were excluded from TXR3 matching. Folder name or extension must not decide game identity.

The `Nueva carpeta working one/000000c4.nfc` file equals the study fixture. Other Lancer-family NFC candidates were inspected separately; matches to small shared images do not establish vehicle identity. The family scan overwrites `source-banks.json` and `exact-vram-matches.json`; use `study-only-matches.json` for the original fixture-only result.

## Next step

Replay host image uploads in chronological GIF order, tracking BITBLTBUF/TRXPOS/TRXREG/TRXDIR and TEX0/palette state. Compare decoded images at their draw/upload points, not solely against initial VRAM. Keep framebuffer-generated textures and unknown transfers explicit. Then map verified source material groups and any required shared banks. Additional captures are not needed merely to compensate for an initial-snapshot-only analysis.

Captured reference is sufficient for this investigation step; 9XD integration remains separate work. Raw game filenames and old lists remain hypotheses until supported by exact payload/palette or geometry correspondence.
