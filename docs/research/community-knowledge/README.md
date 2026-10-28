# Community knowledge map (Gato's Lair Discord dump, Xentax dump)

Updated: 2026-10-26. Author: Claude. Sources: `F:\discord dump\gatos` (HTML channel exports + `assets/` attachments) and `F:\xentax` (1 GB `xentax.7z` + extracted folder, 13,628 entries, not yet indexed). Scripts are cited by attachment name; they are research references, not code to copy.

## What the Discord dump contains

| Channel | Formats | Useful attachments |
| --- | --- | --- |
| genki bs / 0ivx-script | Genki 0IVX (all PS2 Genki games) | `IVX_rework_rev0.591.py` (full model → container → part → packet walk) |
| genki bs / tokyo-xtreme-racer-3 | TXR3 cars, tracks (`.nfc`), global/local textures | `txr3_reworkglob-dump_rev1.32.py`, `flyingfree.py`, `file_list_txr3.txt`, `carlist.txt` |
| genki bs / tokyo-xtreme-racer-drift-2 | TXRD2 / Kaido Racer cars and tracks | `nfc_fullspidey_car_txrd2_2024_rev1.1.py`, `TXRD2-carfiles-V2.txt` (file roles), `current_folder_scan_jic.py` |
| genki bs / import-tuner-challenge | ITC (Xbox 360, XMD) | `itcnew.py`, `explanation.py`, `fixitfelix(_clean).py`, `current.py`, `dualmaybe.py` |
| genki bs / wangan-midnight | WM PS2/PS3 | `current.py`, `curwip.py`; `python/nines_py`: `wmps3_nines.py` |
| genki bs / shutokou-battle-online | SBOL PC MMDL/MIA | `MMDL_Structure.txt`, `mmdl.py`, `cur_sbo1.*.py`, logs |
| formats / ps2 | VIF UNPACK flag meanings | `message.txt` |
| python / nines_py | shared reader library | `nines.py`, `nines_nts.py`, `le_unpack_1.1.py` |
| research: d1gp | D1 Grand Prix | **OBJ exports of `GRANDPRIX_HARE_*`, `BATTLE_HARE_*`** (ground truth for our `d1gp` sample) |
| research: forza-motorsport-1, project-gotham-racing, ridge-racer, gran-turismo, r-tuned, driveclub, drift-spirits, sega-gt, grand-tour | other families | MaxScript/BMS/Python readers, unpackers, sample files |

## Genki 0IVX: hierarchy confirmed on our corpus

The community layout (from `IVX_rework`) parses every Genki file in the study cases without a single span error (`ivx_struct.py`):

```
0IVX00.1 | 0 | LDMX.. | MRON..           32 bytes
4 ints: count2, repeats, ?, size1         + size1 bytes + count2 × 12 ints
container (×N):
    matrix: 8 ints + 12 floats (floats 0..2 = locator translation); a second matrix follows when int[0] == 0
    4 ints (int[3] = part count) + 2 × vec4 bounds
    part (×count):
        8 shorts + 2 × vec4 + 4 × 8 shorts (+0x30 on TXRD2 cars)
        3 ints + float (int[1] = packet count, int[2] = span in qwords) + 2 × vec4
        packets (DMA cnt + VIF), then a 16-byte RET
```

Measured: TXR Zero `0x4d2` and WM PS2 `0x740` models carry world locators (e.g. (2837.5, 30.9, −6532.1)); the three "concatenated models" in those files are three 0IVX models of one placed track object. C1 track files use several containers per model with zero locators; TXR3 cars and Kaido have one container per car part (Kaido container 0 uses the two-matrix form).

## Gaps between this knowledge and 9XD today

| Rule from the community | 9XD today | Impact |
| --- | --- | --- |
| Model → container → part hierarchy with exact spans | Packet scan + heuristics; concatenated models "boundaries not established" | Exact part/model boundaries, names, no guessing |
| Container locator (and second matrix) | Not applied | Track pieces and placed objects end up at the origin |
| Part descriptor byte +13 = texture container (0 global common, 4 global animated, others local), +14 = image | Only local banks inside the same file | Global/animated track textures and cross-file banks unlinked |
| Tracks: global container per track/weather, local container files with up to 3 containers; 3 LODs (geometry, animation, distant) | Not modelled | Track assembly and LOD choice |
| UV scale differs per local segment; TXR3 track V is negated | Per-material scale only | Wrong UVs on some track segments |
| TXRD2 track: restart flag = parity of the last byte of the 0x6E stream | Separate TXRD2 profile (check) | Track faces |
| VIF 0x6C = positions with tristrip reset flag | Partially (Y-bit rules) | Unify topology rules |
| TXRD2/Kaido car file roles (aero/stock × menu/race/LOD1/LOD2, hitbox/shadow/muffler/light) | Not labelled | Show which file is which LOD/role |

Other opportunities: ITC (`itc 360` XMD), WM PS3 (`wm ps3`), TXR2 Dreamcast (`txr2`), D1GP (ground-truth OBJs available), SBOL MMDL (no sample in study cases).

## Xentax dump

`F:\xentax\xentax` holds 13,628 entries in one folder level, which the file bridge refuses to list (limit 8,000). A plain file list (`dir /s /b F:\xentax > xentax-list.txt`) is needed to pick the relevant threads.

## Native integration review — 2026-10-26

New local genki_hierarchy.h, Kaido admission and locator application have been reviewed: see docs/CLAUDE-REVIEW-2026-10-26.md. Hierarchy real-file test passes; damaged-C1 isolation fails, and nonuniform-scale normals need inverse-transpose treatment. Native coverage and tests require synchronization before release. Review made no application changes.
## Native correction result — 2026-10-26

The four review items are resolved: positive hierarchy/attribute Kaido admission, DMA tag parsing, generic coverage accounting and inverse-transpose normals (including singular/mirrored scale behavior). Real hierarchy/isolation/March run 6 passed/0 failed; broader Genki fixture run 14 passed/0 failed. Kaido support is explicitly allowed in the real corpus test; malformed C1 is still rejected. Remaining external material/LOD research stays pending.


## WM PS2 raw-track coordinates — 2026-10-26

Verified development Y reflection for bounded raw concatenated track carriers with placement trailer; WM 00000740.0iv preserves 2,488 vertices/1,826 triangles and X/Z, while correcting normals/winding. Standalone wheel and directory-wrapped cars excluded. See docs/research/wm-ps2/README.md for script evidence, tests and external texture-bank candidates; automatic material linkage remains pending.


## Track texture context investigation — 2026-10-26

See docs/research/genki-track-banks/README.md: original scripts/messages and two TXR3 oval contexts validated for 3,512 index-level references. Global/local banks are scoped to track condition and segment; 18/54 common global payloads differ between variants. Native linking and sampler/UV validation remain pending; prototype JSON is not yet application support. WM requires its own context; no TXR3 ID reuse.


## Native track-context support — 2026-10-26

Verified TXR3 Oval-A/B context selection is now integrated in scan/render/texture collection/GLB/FBX via fingerprinted profiles/genki-txr3-oval-contexts.json. Real model tests load/export 21 and 31 external textured parts; full suite passes. See docs/research/genki-track-banks/README.md for usage. WM/other-game context maps, exact sampling and animation remain pending; do not reuse TXR3 IDs. No new release.


## Automatic track context update — 2026-10-26

Verified TXR3 global/local contexts now load automatically from embedded profiles and validated companion locations. Manual JSON loading is no longer normal setup. Scan controls default to automatic; Texture options… remains an advanced override. Remembered contexts allow matching study-case copies. Real automatic tests and GLB/FBX pass; source/texture fingerprints and no-duplicate-material refresh remain required. Other game maps remain pending. See docs/research/genki-track-banks/README.md.
