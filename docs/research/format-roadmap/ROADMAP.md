# Format roadmap: study cases

Status of every format in `C:\Users\Jarky\Documents\study cases` that is not fully supported yet, with the
evidence already collected and the next concrete step. Written 2026-10-26 by Claude after the triage of
the unanalyzed folders. Complements `docs/research/format-inventory/IMPLEMENTATION-CANDIDATES.md` (ChatGPT).

Rules that apply to every item:

- **Native first.** The generic PS2 VIF analysis stays a fallback for unknown files. It is not a substitute
  for a reader.
- **Validate every reader on the real sample.** That means index ranges, triangle counts and a rendered
  check, plus a synthetic test in `window_tests` and an optional `WORKBENCH_STUDY_CASES` check.
- **Read-only inputs.** Game files and community scripts (xentax, Discord dump) are only read. They are
  never executed or bundled.

Effort scale: **S** = under a day of focused work. **M** = a few days. **L** = needs a container,
decompression or encryption step first.

## Current continuation order - 2026-10-26

This current summary overrides historical "Next up" rows below. ITC and RenderWare (D1GP/Super Trucks geometry) are already wired into discovery and native decoding. AFS is already supported by native_resources.h; Initial D SS is blocked by its inner SMD compression, not by missing AFS support. Do not reimplement these paths.

1. Native-reader/TXD automated validation passed; finish desktop visual checks (see COORDINATION-PLAN.md).
2. Material completion: PSP GIM, NFS MW TPK, external XTD/XPR2 and remaining MIX bank links. Primary PS2 TXD is implemented.
3. Selene part placement, NFS wheels and default variant selection; desktop visual/export checks.
4. Reuse parsed/decompressed data across scan/render by document revision; measure first.
5. Further families: Ridge Racer/NFS PSP, GT6, Blur. Initial D SS codec investigation remains separate.

No decoder is complete solely because its geometry renders. Record geometry, images, material assignment, placement, automated tests and visual checks separately. Existing effort labels below are planning notes, not validated delivery estimates.

## Done in this round

| Format | Game(s) | Result |
|---|---|---|
| PSP GMO (`OMG.00.1PSP`) | Initial D Street Stage, Street Supremacy | `gmo_core.h` / `gmo_model.h`. Both header variants. Geometry only; GIM textures are external and are reported by name. Initial D 6,131 tris, Street Supremacy 2,072. |
| Wangan Midnight PS3 XMD/XTD/X3D | WM PS3 | `wm_ps3_core.h` / `wm_ps3_xmd.h`. One candidate per geometry. Embedded GTFs are linked through the material → slot → bank chain (JZS161: 121 of 162 parts textured). External XTD banks are reported. |
| NFS Most Wanted PC `GEOMETRY.BIN` | NFS MW | `nfs_mw_core.h` / `nfs_mw_solid.h`. 291 solids (76,339 tris) on the 911 Turbo. Adds a stock-car assembly (21 LOD-A parts, 11,029 tris). LOD/damage/kit/decal/hood variants are listed separately. Wheels and brakes stay separate because they sit at the origin. |

### Follow-ups for the done formats

- **GMO**
  - Link GIM textures from sibling files. Needs a GIM decoder: PSP swizzle plus 4/8-bit palettes. **M**
  - The model name sometimes parses as one letter (`D`). This is cosmetic. **S**
- **WM PS3**
  - Resolve external XTD banks from course/common files. Needs the multi-file link already used for Genki
    track context. **M**
- **NFS MW**
  - Link `TEXTURES.BIN`. It is a TPK; the hashes are in 0x134012 and the per-submesh slots are at record
    +0x18. **M**
  - Place the wheels using the car's attribute data. **M**

## Next up (analysed enough to implement)

| # | Format | Game | Evidence | Next step | Effort |
|---|---|---|---|---|---|
| 1 | `XMD\0` + XPR2 | Import Tuner Challenge (X360) | Same XMD family as WM PS3, but with a different magic and LE/BE mix. Textures are in XPR2 (Xbox 360 tiled). | Reuse `wm_ps3_core` block parsing behind a variant flag. Add XPR2 header + X360 untile (DXT). | M |
| 2 | NFS PSP `GEOMETRY.BIN` | NFS (PSP) 240SX | Same 0x80134000 root. After 0x80134001 the file has `0x55441122` and `0x00097900` chunks instead of solids. | Map the PSP solid chunk ids. Vertices are likely GU vertex types, so the GMO layout code can be reused. | M |
| 3 | Super Trucks MIX | Super Trucks | Scene tags found during triage. | Dump the tag tree. Check whether the meshes are VIF (the generic fallback already gives a hint). | M |
| 4 | Initial D Special Stage AFS | Initial D SS (PS2) | AFS archive with named PARTS/PAC entries. | Add AFS as a container (list children, open a child as a document). Then the existing Genki/VIF paths apply per child. | S |
| 5 | GT6 PACB | Gran Turismo 6 | `MDL3`, `SHDS`, `PMSH` blocks, low entropy, so not compressed. | Walk MDL3 → mesh table. Check vertex declarations against PMSH. | M |
| 6 | D1GP | D1 Grand Prix (PS2) | Header table with 22 entries and `0x1c020037` markers. | Decode the table → per-entry VIF packets. Compare with the generic VIF result for the same file. | M |
| 7 | Ridge Racer PSP | Ridge Racer | `vhcl` / `modl` / `PXmtrl` chunks. | Chunk walker + GU vertex arrays (shared with GMO). | M |
| 8 | Blur | Blur (PC) | `CPModel` chunks. | Chunk walker; vertex declaration from the chunk header. | M |
| 9 | Gran Tour (CryEngine `CrCh`) | — | CryEngine chunk file (CGF/CGA). | Implement a CGF chunk-table reader (mesh, data streams, material). The format is documented. | M |
| 10 | NUD (`NUDP`) | — | Namco NUD (also used by the generic VIF sky-dome sample). | Read the NUD header → mesh groups → polygon descriptors. | M |
| 11 | Overdrive BDAE | Asphalt Overdrive | Zip containing `little_endian_quantized.bdae`. | Unzip in memory (we already depend on zlib via Qt), then the Gameloft BDAE reader (quantized positions). | M |

## Needs analysis first

| Format | Game | What we know | First analysis step |
|---|---|---|---|
| Auto Modellista | Auto Modellista (PS2) | In ChatGPT's candidate list; no layout confirmed yet. | Run the generic VIF analysis. If the coverage is low, look for the per-object header table. |
| TXR2 | Tokyo Xtreme Racer 2 (DC) | Dreamcast and not PS2, so VIF does not apply. | Look for PVR texture headers and strip lists (NJ/Ninja chunks such as `NJCM`). |
| F&F ARCP | Fast & Furious | `arcp_evidence.h` already finds point clouds. Faces are not linked yet. | Find the index stream that pairs with the point sets. |
| Driveclub `EVOSLITL` | Driveclub (PS4) | A community BMS script exists (read only). | Read the BMS logic for the container. Then dump one mesh and confirm the vertex format. |

## Blocked by encryption or compression

These need a container or crypto step before any geometry work. They are listed so nobody loses time
treating them as plain data (entropy ≈ 8.0).

| Format | Game | Blocker |
|---|---|---|
| GT4 | Gran Turismo 4 | Entropy 8.0. The volume is compressed or encrypted. Needs the GT4 volume/decompression step first. |
| PCARS | Project CARS | Encrypted/compressed BFF packs. |
| WMMT | Wangan Midnight Maximum Tune (arcade) | Encrypted arcade data. |
| MCLA | Midnight Club LA | RAGE resources, compressed. |
| Dacia | — | Compressed container. |

## Out of scope

| Format | Reason |
|---|---|
| Unity asset bundles | General-purpose engine format; dedicated tools exist. |
| MSFS packages | glTF-based. Use the official pipeline. |
| The Crew 2 `HSEM` | Large proprietary engine; no evidence base yet. Revisit only on request. |
| Digimon "test" | It is a plain BMP. Already viewable as an image. |

## Suggested order

1. **AFS container (#4).** Small, and it unlocks more Initial D files for the existing readers.
2. **Import Tuner Challenge (#1).** Shares most of the code with WM PS3.
3. **NFS PSP (#2) and Ridge Racer PSP (#7)** together, reusing the GMO GU-vertex code. Add a **GIM decoder**
   at the same time; it also completes GMO texturing.
4. **NFS MW `TEXTURES.BIN`.** TPK textures also apply to NFSU2.
5. Then GT6, D1GP, Blur, CryEngine, NUD and BDAE, in any order based on interest.

## Update 2026-10-26 (owner priority: ITC, Super Trucks, Initial D SS, GT6, D1GP, Blur, NUD)

| Format | Status | Notes / next step |
|---|---|---|
| Import Tuner Challenge `.sdb` | **Done** (`itc_core.h`, `itc_model.h`, `xpr2_textures.h`) | Wrapped BE model + XPR2. One candidate per material element, textured from the local XPR2 (5 of 6 elements on `000000ba.sdb`). The `.xmd` sample is a UI menu (X2D + XPR2): its 77 images now list through the generic XPR2 reader. Next: shared course textures; car files (not in the samples). |
| D1 Grand Prix `.BIN` / `.D1` | **Geometry + primary TXD** (`rw_core.h`, `rw_model.h`, `rw_txd.h`) | RenderWare 3.7 streams: clumps (frames applied) and BSP worlds. A31 car file 16,275 tris; HARE_USA track 40,239 tris. Primary TXD now decoded and linked by material name: 99.44%/99.52% of recovered car/track triangles textured. Remaining shader/material-state fidelity needs visual review. |
| Super Trucks `.MIX` | **Partial** (same RenderWare reader) | MIX = flat archive (directory at the end, names obfuscated). Scenes are Selene + RenderWare atomics with PS2-native VIF geometry. CARS.MIX 196,987 tris, ALBACETE.MIX 128,409 tris; the track is pre-placed. Missing: car-part placement. Each Selene object ends with an 80-byte node (rotation at +20, translation at +56; parents in `@HIERARCHY_DATA@`), and the wheels are placed at runtime. Primary TXD now supported on 44.76% of recovered CARS triangles; remaining bank-less scenes require validated shared-resource linkage. ALBACETE texture storage remains unidentified. |
| Initial D Special Stage | **Blocked by compression** | AFS (already opens) → PAC (`PAC\0`, 32-byte entries) → GIM textures (type 1) + compressed SMD models (type 3, header `12 3D DA 01`, raw size, packed size). The codec is LZ with 8-bit flags (1 = literal); the reference encoding is not resolved (not plain LZSS or PRS). Next: identify the codec (an emulator trace or another Sega AM2 title). |
| GT6 `PACB` | Analysis | The PACB directory (BE offset/size) holds MDL3 models. The Xentax Noesis script (`7682/gt6.7z`, read only) covers textures (TXS3), bones and FVF, but **not the meshes**. Next: map the MDL3 mesh table + PMSH. |
| NUD (`NUDP`) | Covered by the generic VIF | PS2 NUD: 3 records {offset, size} → VIF blocks (V3-32 position, V4-8 colour, V2-32 UV). A native reader would only add the part split and names. Low value. |
| Blur `CPModel` | Not started | Xentax `6778/blur_loc_ext.7z` holds only an extractor exe (not used). |

RenderWare is now a generic reader. Other study cases using RW 3.x (any `0x1C02xxxx` library stamp) will be picked up automatically.

## Joint validation follow-up - 2026-10-26

Configured baseline: reader 45/0; full window 109/0 (80 optional skipped); focused new-reader tests 7/0 with study-case environment enabled. Real RenderWare inspection found invalid source normals on CARS.MIX; Codex added a reported face-normal regeneration path and actual-corpus regression coverage, final rerun pending. ALBACETE currently yields 101,577 triangles, differing from the previous 128,409 report; this is an open coverage issue. See ../renderware/README.md and COORDINATION-PLAN.md for current results. TXD/GIM/TPK/external banks remain the next material work, not completed by these checks.

Final follow-up validation: reader 45/0, window 110/0 (81 optional skipped), focused 9/0/no skips. RenderWare repair verified on actual CARS: five normals regenerated, all 196,987 recovered triangles preserved; D1GP and ALBACETE scan/decode outputs have finite values and valid indices. Application build status is recorded in COORDINATION-PLAN.md. Next implementation: TXD images with per-material surface binding, followed by PSP GIM/NFS TPK and external bank contexts. ALBACETE full-coverage and live visual validation remain pending.

### TXD picked up - 2026-10-26

Codex implemented native PS2 TXD primary-image decode, scoped material surfaces, optional livery dictionaries and document-revision caching. Integration and real corpus tests are running; see ../renderware/README.md and COORDINATION-PLAN.md. Do not mark all RenderWare materials complete: ALBACETE textures, shader passes, Selene placement and runtime variants remain pending.

### TXD completion scope - 2026-10-26

Primary PS2 TXD decode/material partition and optional livery variants pass: 1,256 native images, triangle totals preserved, reader 45/0, window 111/0 (82 optional skipped), focused 11/0 and Genki corpus 7/0. Complete game rendering is not claimed: CARS material coverage is partial, ALBACETE texture storage/coverage and Selene placement remain open. See ../renderware/README.md for exact measured coverage and COORDINATION-PLAN.md for final application build status. Next material implementation: PSP GIM or NFS TPK; do not reimplement the completed native TXD base path.

## Super Trucks status supersession — 2026-10-26

Native MIX TOC/path linkage and bounded LZW TXD decompression are integrated and tested. CARS default PS2 set: 116/116 textured surfaces, 88,548/88,548 textured triangles. ALBACETE dry: 61/70 surfaces, 43,974/52,136 textured triangles (84.35%); all 236 embedded rasters decode. Remaining 8,162 dry triangles lack recovered UVs despite matching texture images. Earlier “unknown/absent Albacete banks” and CARS 44.76% notes are historical. BSP resource traversal restores the all-alternative ALBACETE total of 128,409 triangles; default set remains 52,136. Weather/platform/livery alternatives are optional, not duplicated by Render All. Updated dev executable builds; reader 45/0, full window 112/0 (83 optional skipped), focused 13/0/no skips, Genki 7/0/no skips. Next: native UV streams plus live visual validation; full shaders/Selene placement still pending. See ../renderware/README.md and COORDINATION-PLAN.md for evidence and exact scope.

## Initial D SS audit supersession — 2026-10-26

The codec blocker is historical. Native PAC/SMD/GIM are integrated. Independent Codex audit corrected masked VIF UVs (all 415,596 Akagi pairs were present but ignored), removed unproven W-bit-2 winding in favor of GIF triangle-strip order, split named LOD alternatives and enabled an explicitly approximate unlit base-texture preview. Akagi: 216,950 triangles = 158,296 main + 58,654 optional LOD; 122 default texture surfaces. Source vertex colors, fog/blend/alpha, V3-32 attributes, exact runtime LOD selection and active VU/front-face/basis verification remain pending. Evidence/hashes/static executable offsets: ../initial-d-ss/README.md. Focused 6/0, reader 45/0, full window 113/0 (84 skips), actual Genki 7/0; dev executable updated. No publication or game data added. Live visual comparison remains required.

### Initial D SS GIM correction — 2026-10-27

Native GIM now replays the declared PSMCT32 transfer into GS storage before reading PSMT4/PSMT8. This repairs 33 swizzled T4 Akagi images previously decoded linearly (trees/asphalt/course map); 106 other images unchanged. All 139 decode and checked image hashes pass. New automatic/manual viewport clipping and extended/fine zoom support close inspection. Targeted 9/0, reader 45/0, full window 115/0 (85 optional skips); dev executable updated. Shared GS address math is in ps2_gs_address.h, image_recipe.h keeps the compatibility wrapper. Current evidence and remaining effects/UI verification are in ../initial-d-ss/README.md and ../../CLAUDE-INITIAL-D-SS-HANDOFF.md.

## Blur reconnaissance supersession — 2026-10-27

Blur now has a read-only reconnaissance and ordered native implementation plan: [Blur roadmap](../blur/ROADMAP.md). CPModel framing and one inline DXT texture are verified; PAK directory/name parsing and one exact segmented decompression are proven by independent private probes. Full PC installation resources and showroom companions are available. ARCH references, vertex/index declarations and material links remain unimplemented; this does not add 9XD mesh support. Older 'not started' rows are historical. No automatic reuse of PGR layouts is authorized by shared studio identity.

### Blur first native slice — 2026-10-27

Bounded single-static-draw CPModel geometry and inline DXT image recipes are now integrated. Real envsphere/specsphere, malformed cases, GLB/FBX and scan-to-scene checks pass (6/0/no skips). Audi multi-buffer and skinned character meshes are still unsupported; seven inline images per model decode. See [Blur implementation receipt](../blur/ROADMAP.md). Full game/scene support is not claimed.
