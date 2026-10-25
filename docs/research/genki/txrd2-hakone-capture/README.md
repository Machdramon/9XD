# TXRD2 Hakone downhill / fine weather capture

User selected Hakone downhill, fine weather, with the Nissan March HK11 at the start line. PCSX2 2.8.2 Software renderer. Captured locally with Shift+F8; no application source changes or public uploads in this capture step.

## Capture evidence

Original: `C:\Users\Jarky\Documents\PCSX2\snaps\Tokyo Xtreme Racer Drift 2_SLUS-21394_20261025170533.gs.zst`

Compressed SHA-256: `df5a440f792398c86a920eb96b053991c114783702db4285f15dfb3914177f0d`

Serial SLUS-21394; GS state version 9; decompressed 16,852,063 bytes.

Saved `hakone.gs.zst`, `hakone.gs`, `hakone-reference.png`, `gs-state.bin` and register/source comparison JSON reports.

The dump parses into 7,664 transfer records, 10,596 GIF tags and 4 VSync records, with no remaining parse bytes. There are 175 distinct TEX0 values, not that many unique image assets. Car, HUD and track resources coexist.

## Verified source correspondence

The old track table identifies `00001701.dat` as the Hakone global block, and `00001726.dat`, `00001759.dat`, `000017b8.dat` as local blocks. The available source files contain 152, 14, 13 and 12 bounded TIM2 pictures respectively.

The initial GS VRAM snapshot matches **30 distinct original image payloads** from `00001701.dat` byte-for-byte after GS index addressing. Duplicate TEX0 configurations account for 32 match records. For **29 distinct images**, the corresponding stored palette also matches exactly.

The confirmed RGB preview sheet is `matched-global-textures-rgb.png`. It includes start/finish signs, checkerboard, trees/cherry blossom, asphalt/ground surfaces, stone walls, road signs and a building visible in or relevant to the captured start area. This is an RGB texture sheet, not game shader reproduction or a complete per-mesh material mapping.

Most indexed4 draws use CLD=0 and nonzero CSA palette slots. Comparing only the first palette entries at CBP gives false mismatches. The probe compares the appropriate CSA slice of the resident CSM1 palette memory. Initial-memory equality corroborates storage and palette selection; complete draw-time CLUT cache history remains to be replayed.

One matched image has a 24-bit RGB palette (source CLUT type 2). Its indexed pixels match, but the probe has not yet validated that palette against captured storage. It is unresolved, not proof of a missing image.

No exact indexed-pixel match was found in this initial snapshot for the three local candidate banks. This neither proves absence from the game/capture transfers nor proves the weather/variant mapping is wrong. Later uploads, alternate dimensions and other variants require chronological replay.

## Reproduce and continue

1. `hakone-gs-probe.py` preserves/decompresses the latest captured file (select the intended source explicitly if further captures exist).
2. `hakone-gif-audit.py` parses recorded transfers and TEX0 observations.
3. `hakone-source-match.py` inventories the four known source files and compares indexed payloads against initial VRAM.
4. `hakone-palette-match.py` checks palette memory including CSA slices and deduplicates source-image counts.

Next: implement the bounded headerless image-bank reader established by the March capture; resolve material/group references using these pixel/palette ground truths, and trace upload/draw order for remaining textures. Keep runtime addresses and relocation values capture-specific rather than hardcoded. Check RGB24 TIM2 palette support separately.

Primary format reference: [PCSX2 2.8.2 GS capture writer](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSDump.cpp), [GS freeze serialization](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSState.cpp), [CLUT loading/cache semantics](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSClut.cpp).
