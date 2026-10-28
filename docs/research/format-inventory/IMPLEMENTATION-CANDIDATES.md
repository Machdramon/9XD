# Additional study-case implementation candidates

Updated: 2026-10-26. Research triage only; no decoder added or legacy script executed. Cross-checked the current inventory, local script text/archive contents, and representative sample headers (first 2 MiB per sample). This is not a complete audit of the Discord/Xentax collections.

| Priority | Candidate / actual sample | Existing evidence | Proposed bounded next step | Remaining uncertainty |
|---|---|---|---|---|
| 1 | Wangan Midnight PS3: JZS161.CAR, AREA_9GO_BUIL_CUT_9GO_FREE_001.GRID | Discord wmps3_nines-DBE04.py reads geometry, normals, strip indices and multiple UV streams; Xentax 9072/Wangan Midnight.zip contains a 23,367-byte Wangan Midnight.ms reader with XMD/XTD dispatch, mesh and texture paths. GRID contains X3D0037 at byte 56. | Native XMD/XTD child adapter; expose embedded GTF to existing decoder, then bounded X3D mesh parts with material receipts. | Script/sample variants and relative pointer bases; complete material/UV associations not yet validated. Archive converters were not run and need not be bundled. |
| 2 | Import Tuner Challenge Xbox 360: 000000ba.sdb, 000000cf.sdb, 00001d7e.xmd | Discord itcnew-F3E7E.py has geometry, strip faces, three UV/material layers, common and XPR texture-name references. XPR2 occurs at 233168, 471184 and 108704 respectively. Additional fixitfelix scripts exist. | Independent native container/geometry reader, then XPR2 image storage and common/local name-based material mapping. | External common banks, Xbox tiling/endian/storage and layered shaders. Existing scripts reference external TGA paths; not a complete self-contained texture decoder. |
| 3 | NFS Most Wanted PC: car/GEOMETRY.BIN | Existing measured inventory finds NFSU2 chunk family and solid version 0x16, with MW-specific chunks. Current source does not expose explicit MW support. | Try native NFSU2 discovery in a regression harness, compare layouts and add an isolated MW variant only after bounds/index/UV validation. | Shared container family does not prove identical mesh layout. Xentax 533 ZIP has BIN.bms, but it is binary, not a readable QuickBMS script; do not treat it as a ready importer. |
| 4 | D1 Grand Prix PS2: A31_C00.BIN, GRANDPRIX_HARE_USA.D1 | Discord d1gp channel plus source D1 and OBJ attachments for Nikko/Odaiba/Sekia. A31 contains IMGF at 756900. | Identify tables/geometry packets and texture sections; use an exact matching D1/OBJ pair from references as validation before the USA sample. | Existing reference OBJs are different tracks, not ground truth for GRANDPRIX_HARE_USA. Full material/texture binding unproven. |
| 5 | Ridge Racer PSP: body.vhcl | Actual header vhcl followed by modl; Discord ridge-racer research channel and multipart archive attachments. | Index source notes/archives, then implement a bounded vhcl/modl walker and verify PSP vertex/index encodings. | No working matching reader was inspected in this triage; format signature alone is insufficient. |
| 6 | Auto Modellista PS2: car044.bin | Embedded TIM2 observed at 239636; inventory identifies several images. | Reuse image decoder and investigate bounded PS2 geometry packets/topology separately. | VIF-like data is not enough to reuse Genki face rules; geometry/material linkage still research. |

## Lower confidence / deferred

- TXR2 Dreamcast has actual CARMODEL_00004.bin and COURSE.AFS, but txr2_old_adapted_folder-92F16.py contains TXRD2/Zao paths and PS2 VIF 0x65/0x6e packet logic. Its filename is not evidence of a compatible Dreamcast importer. Find genuine matching source first.
- SBOL MMDL has structure notes and Python references, but no identified matching study-case sample in the current inventory.
- Blur CPModel/ARCH remains a research candidate; no new matching implementation recipe verified here.
- PCARS, WMMT, MCLA and other compressed/high-entropy families need their container/codec established first. High entropy alone does not establish encryption.
- Kaido admission, concatenated Genki boundaries/locators and WM PS2 height correction are already integrated in the development tree. Older inventory rows saying "blocked" are historical, not new opportunities to implement again.

## Proposed order

For strongest documentation plus useful new family: WM PS3, then ITC. For a potentially smaller compatibility extension: first test NFS MW with the existing native NFS pipeline. D1GP is attractive for validation because related source/OBJ pairs exist. Relative effort only; no completion or visual correctness is claimed.

All source dumps and game binaries stay read-only and local. Port documented logic into bounded native readers; do not execute attachment scripts or bundle converter executables. Update COORDINATION-PLAN.md when a candidate is picked up and distinguish geometry, texture decoding and material binding in acceptance checks.
