# Study-case format inventory and gap analysis

Updated: 2026-10-26. Author: Claude. Research only: no application code changed by this report.

Sources: `C:\Users\Jarky\Documents\study cases` (967 entries, about 50 folders, about 1.3 GB) and the backup folder `F:\copia de seguridad 08.04.2019\downloads` (mostly ISOs and archives; extracted data only for Auto Modellista USA, MSR alpha, TXR2 Dreamcast and part of Midnight Club LA). 140 representative samples were copied to Claude's workspace and probed with the Python scripts in `scripts/`. Implementation status comes from the 9XD source and docs on `main` after 0.3.7, plus three live checks in the development build.

Evidence levels used below: **tested** = loaded in 9XD during this review; **measured** = computed by the probe scripts on the sample bytes; **documented** = stated in existing 9XD docs; **hypothesis** = consistent observation, not yet a rule.

## 1. Summary

| Area | Finding | Level |
|---|---|---|
| Wangan Midnight PS2 `.0iv` | Already decodes. `000000a3.0iv` (a wheel) passes the existing GIF-strip profile: 28 parts, 3,452 vertices, 2,880 triangles. Textures are referenced (bank 0, images 0/52/85/119) but not in this file. | tested |
| Kaido Racer | Blocked today; no profile admits it. A plain per-packet strip rule fits the data: 99.98 % of 5,072 normal-bearing faces agree with stored normals in `000004ac.dat`; zero orientation conflicts in both Kaido files. | measured |
| Concatenated 0IVX models (TXR Zero `0x493`/`0x4d2`, WM PS2 `0x740`) | Blocked because boundaries are "not established". Every 0IVX model in the corpus, including directory-bounded ones, ends with a DMA `RET` tag (`0x60000000`, qwc 0) right after its last packet. This is an in-format terminator that can establish boundaries. | measured |
| Data after the last model | A count followed by 16-byte records (UInt32 id + Float32 XYZ in world units) in the concatenated files. Likely placements/pivots. | hypothesis |
| Fast & Furious `fnf ps2/01mrszagato/*.ARC` | Decodes with the existing ARCX decoder (11 material draws in BODY_0; LOD0–3/UPG variants in BODY_X). Entry types and vertex layout match the Xbox prototype; the folder name says PS2. | tested |
| Genki next generation (ITC Xbox 360, WM PS3) | Unsupported. Big-endian `XMD.001` containers with nested `XMD`, `XTD.002` texture dictionaries holding PS3 `GTF` textures, `X3D0037` models, `X2D` and Xbox 360 `XPR2` packages. 9XD already decodes standalone GTF. | measured |
| NFS Most Wanted PC `GEOMETRY.BIN` | Same chunk family and solid version (0x16) as NFSU2, plus MW-only chunks 0x134013/17/18/19. Worth one test with the NFSU2 profile. | measured |

## 2. Genki PS2 family (0IVX / LDMX / MRON)

Two packet families exist, independent of game name.

| Family | Control unpack | Position | Optional streams (VU address) | Games in corpus |
|---|---|---|---|---|
| 2-vector | `6c` x2 @0x8000, cl2 wl2 | `68` @0x8002 | UV `65` Int16 or `64` Float32 @8003; normal `6a` V3-8 or `6d` V4-16 @8004; color `62`/`6e` @c005 | C1 (v2 and v1), TXRD2 (v1), Kaido (v2) |
| 4-vector (GIF tag) | `6c` x4 @0x8000, cl4 wl4 | `68` @0x8004 | UV `64` Float32 @8005; normal `6d` @8006; color `6e` @c007 | TXR3, TXRD1, TXR Zero, WM PS2 |

### Per-file status

| File | Layout | 9XD today | Textures | Gap / next step |
|---|---|---|---|---|
| C1 `000004b8.dat` | 2-vec v2, 867 packets | C1 profile, 21,942 tris | 78/165 parts textured | 5 references clamp 256x128 against 256x64 images (documented) |
| C1 `00000c49`, `00000c50`, `00001540`, `0000159f`, `00004151` | 2-vec v2, multi-model with directory | All packets pass C1 checks | Not measured | Confirm in app; material binding per model |
| C1 `00001010.dat` | 2-vec v1 | TXRD2/v1 profile candidate (323/345 packets pass basic checks) | — | Check admission in app |
| C1 `00003235.dat` | TIM2 bank only | Image scan | — | Texture bank, link to models |
| Kaido `000004ac.dat` | 2-vec v2, UV Float32 @8003, normal V4-16 @8004, mostly no color | **Blocked** | 25 references already decode | New profile (below) |
| Kaido `000031c4.nfc` | 2-vec v2, UV Float32 @8003, color @c005; 2 models with directory | **Blocked** | — | Same new profile |
| TXR3 `000000c4`, `000028ca`, `0000603f` | 4-vec | GIF-strip profile | 18/55, 5/5, 0 | Global/atlas banks; Evo V capture work |
| TXRD1 `00001363`, `0000059f`, `00000d09` | 4-vec | GIF-strip | 8/47; multi-model geometry only | Alto material/palette confirmation |
| TXR Zero `0x000000ba.dat` | 4-vec | GIF-strip | 14 references decode | Remaining bank/layout references |
| TXR Zero `0x00000493`, `0x000004d2` | 4-vec, concatenated, no directory | **Blocked** (boundaries) | — | RET terminator rule (below) |
| WM PS2 `000000a3.0iv` | 4-vec | **Works** (tested) | 20 unresolved references | Find the texture bank file |
| WM PS2 `00000740.0iv` | 4-vec, 3 concatenated models | **Blocked** (boundaries) | — | RET terminator rule; placement table |
| TXRD2 `car/000002a0.sdb`, `00000000.xvi`, `00004336.dat` | 2-vec v1 | v1 profile | March: 19 parts | XVI and 4336 materials pending (documented) |
| TXRD2 `00001aeb.sdb` | Genki 2-entry container, float records, no 0IVX | Not recognized | — | Probably not mesh packets (collision/course data?); identify |

### Proposed: Kaido 2-vector plain-strip profile

Evidence (measured on `000004ac.dat` and `000031c4.nfc`):

- Control block matches the C1 layout: count word (+28) = `0x8000 | n` and parity word (+12) = `±1.0` in every packet.
- No per-record draw marker exists. The Y low bit is set in only 416/917 strip starts and in about 60 % of all records; normal W is always 32767; colour alpha is always odd (255/43/39). A bit-by-bit search over every stream found no marker set at strip starts and sparse elsewhere. The same search finds C1's known Y-bit marker.
- Plain strips per DMA packet, winding from the packet parity, give 99.98 % agreement with stored normals (5,072 faces). For calibration, C1 scores 92.8 % with plain strips and 99.98 % with its verified Y-bit rule, so the test discriminates.
- Orientation check: 10,700 and 1,180 shared edges with consistent orientation, 0 conflicts. C1 shows 288 conflicts without its rule.

Admission sketch: complete bounded model; `LDMX00.2`/`MRON00.2`; 2-vector control at 0x8000 with matching count/parity; positions `68` @8002; ordered optional streams from {UV `64` @8003, normal `6d` @8004, color `6e` @c005}; kick present; no C1 Y-bit clearing; each packet an independent strip. Keep it isolated from C1 and TXRD2.

### Proposed: RET terminator for model boundaries

Every model in all 23 files ends with a DMA `RET` quadword (`00 00 00 60` followed by 12 zero bytes) directly after its last packet. In directory-bounded files the RET sits exactly at the directory boundary (plus zero padding), which cross-checks the rule. In concatenated files the next `0IVX00.1` header follows the RET immediately. Proposed admission: a model ends at the first RET quadword after a contiguous run of validated packets, and the next model header (or a known trailing table) must start at the following 16-byte boundary. This would unlock Zero `0x493`/`0x4d2` and WM PS2 `0x740` without guessing.

### Hypothesis: trailing placement table

After the last model's RET: UInt32 count, then count x 16-byte records `(UInt32 id, Float32 x, y, z)`. Zero `0x4d2`: 2 records with id 25; Zero `0x493`: 1 record with id 7; WM PS2 `0x740`: 7 records (ids 1 and 7). Coordinates are world-scale. The id is not a model index in these samples. Needs a capture or game-side check before use for scene assembly.

## 3. Other families

| Game / folder | Platform | Format | 9XD status | Missing (geometry / materials / textures) |
|---|---|---|---|---|
| FM1 | Xbox | CAB v15, CARBIN, XB2 | Cars primary set; tracks with textures (documented) | Car UVs/materials, alternate draw sets/LODs, damage; track special shaders |
| MSR alpha | Dreamcast | NRM/HRD, DYP/DYT/NTP/NTT | Cars + tracks with textures (documented) | Further LODs, secondary masks, lighting |
| MSR retail | Dreamcast | Pak (big-endian LZ) | Tracks; resource extraction | Retail vehicles; `yzb_` wrappers |
| PGR1 | Xbox | GOTHPOK, NRM/_v/_res, DyP/DyT/DyV | Cars + tracks textured | Roof/component alternates, shaders |
| PGR2 | Xbox | Indexed PAK, MESH/GPUD, WMSH, INST | Cars + tracks + placed objects textured | 378 command-layout INST refs, COLR lighting, extra streams |
| PGR3 / PGR4 | Xbox 360 | ENDI/ENSW, SCNE, STREAM2 | Cars and static tracks with base maps | Layered shaders (1,536 Tokyo draws), instanced objects, PGR4 STREAM2 v6 for vehicles |
| R-Tuned | Arcade | FArC/FArc, geometry/object/texture.bin | Car, course, collision, textures | object.bin placement, packed normals, A3DA animation |
| NFSU2 PC | PC | Chunked BUN/GEOMETRY.BIN, TPK | Scenery + cars with textures | Vertex colours, dynamic paint/decal tables |
| NFS MW PC | PC | Same chunk family as NFSU2 | Not tested | Test NFSU2 profile; MW chunks 0x134013/17/18/19 |
| NFS PSP | PSP | BUN, GEOMETRY.BIN with PSP chunks (0x55441122) | Not supported | PSP solid layout |
| Fast & Furious (`fnf ps2` ARCX) | Xbox-layout banks | ARCX | **Works** (tested) | Damage/skinned resources, external wheels |
| Fast & Furious ARCP (`fnf ps2/*.arc`) | PS2 | ARCP | Points + strips for `000000c4.arc` (documented) | Materials, hierarchy; other ARCP files not checked |
| ITC | Xbox 360 | Genki `.sdb` (big-endian, `mdl_Detail`) + `XMD.001` with `XPR2` | Not supported | Everything; XPR2 decoder |
| Wangan Midnight PS3 | PS3 | `XMD.001` → `XTD.002` (GTF) + `X3D0037` | Not supported | Container + model; GTF textures are a quick win |
| Street Supremacy | PSP | Genki `.sdb` | Not supported | Not probed in depth |
| TXR2 | Dreamcast | CARMODEL `.bin`, COURSE.AFS | AFS opens; models not decoded (documented) | Everything inside |
| Blur | 360/PC | `CPModel` `.model`, ARCH `.levelstream` | Not supported | Bizarre Creations; may share ideas with PGR4 |
| Auto Modellista | PS2 | `.bin` with 4 TIM2, `.adx` | TIM2 images only | Geometry |
| D1 Grand Prix | PS2 | `.BIN`/`.D1` with IMGF | Not supported | — |
| Initial D Special Stage | PS2 | AFS + TBL | AFS opens | Children unidentified |
| Initial D Street Stage (`initial d`) | PSP | `OMG.00.1PSP` | Not supported | — |
| R Racing Evolution | PS2 | Raw `Model`, `RoadPoly` etc. | Not supported | — |
| Ridge Racer | PSP | `vhcl`/`modl` chunks | Not supported | — |
| Overdrive City | Mobile | `.bdae` (ZIP + BRES), TGA | ZIP opens | BDAE model |
| Project CARS 2/3 | PC | `.meb` | Not supported (high entropy: compressed or encrypted) | — |
| Wangan Midnight Maximum Tune | Arcade | `.mdl`/`.tex`/`.bin`/`.nut` | Not supported (entropy ~7.97: encrypted or compressed) | Decryption first |
| Midnight Club LA | 360 | RSC `CSR` resources | Not supported (compressed) | Codec |
| Driveclub, GT4/GT6/GT PS4, The Crew 2, Super Trucks, Natsuiro, MSFS, Dacia AR, Unity bundle, NUD | various | various | Not supported | Outside current priorities |
| `initial d zero/*.tmr`, `natsuiro/*.tmr`, `fnf ps2/.../*.tmr`, `nfsu/*.mtl`, `nfs chinese/*.pskx`, `gothpokk/*` | — | User notes, exports or tests | n/a | Not game formats |

## 4. Recommended order

1. **Kaido profile** (2-vector plain strips). Strong evidence, small isolated change; Kaido's 25 decodable texture references then bind automatically.
2. **RET terminator** for concatenated 0IVX models (Zero, WM PS2). Cross-checked against every directory-bounded model.
3. **WM PS2 textures**: locate the bank file holding images 0/52/85/119 in the game data (only two `.0iv` samples here).
4. **XTD.002 → GTF adapter** for WM PS3 textures (decoder already exists), then `X3D0037` geometry.
5. **Quick tests in the app**: NFS MW PC with the NFSU2 profile; C1 `00001010.dat` v1; the other C1 multi-model files.
6. Later: XPR2 (ITC), Blur, placement-table research.

## 5. Reproduce

Scripts in `scripts/` read the study-case folder (set `STUDY_CASES` or edit `ROOT`):

- `survey.py`: headers, embedded signatures, entropy.
- `genki_vif.py`: 0IVX model/packet walker and layout census.
- `genki_admit.py`: per-packet C1 / GIF-strip / 2-vector admission checks and model tails.
- `flag_search2.py`: search for strip-start draw markers.
- `normal_agree.py`: face-normal agreement for plain vs Y-bit strips.

No game data is included.

## Follow-up triage - 2026-10-26

See [additional implementation candidates](IMPLEMENTATION-CANDIDATES.md) for cross-checked local references and current priority. Earlier Kaido/concatenated-model "blocked" rows are historical: these paths are now integrated in the development tree. WM PS2 external banks remain unresolved. This triage adds no new decoder support.

## RenderWare/TXD status refresh - 2026-10-26

Historical "D1GP unsupported" rows are superseded by the native rw_core/rw_model/rw_txd path: D1GP and Super Trucks recovered geometry plus supported primary TXD material surfaces. All 1,256 sample TXD rasters decode; D1GP car/track primary textured triangle coverage is 99.44%/99.52%, CARS 44.76%, ALBACETE remains untextured (storage unknown). See ../renderware/README.md for validation and limits; this is not complete runtime shader/scene reconstruction.

## Super Trucks status supersession — 2026-10-26

Native MIX TOC/path linkage and bounded LZW TXD decompression are integrated and tested. CARS default PS2 set: 116/116 textured surfaces, 88,548/88,548 textured triangles. ALBACETE dry: 61/70 surfaces, 43,974/52,136 textured triangles (84.35%); all 236 embedded rasters decode. Remaining 8,162 dry triangles lack recovered UVs despite matching texture images. Earlier “unknown/absent Albacete banks” and CARS 44.76% notes are historical. BSP resource traversal restores the all-alternative ALBACETE total of 128,409 triangles; default set remains 52,136. Weather/platform/livery alternatives are optional, not duplicated by Render All. Updated dev executable builds; reader 45/0, full window 112/0 (83 optional skipped), focused 13/0/no skips, Genki 7/0/no skips. Next: native UV streams plus live visual validation; full shaders/Selene placement still pending. See ../renderware/README.md and COORDINATION-PLAN.md for evidence and exact scope.

## Initial D SS audit supersession — 2026-10-26

The codec blocker is historical. Native PAC/SMD/GIM are integrated. Independent Codex audit corrected masked VIF UVs (all 415,596 Akagi pairs were present but ignored), removed unproven W-bit-2 winding in favor of GIF triangle-strip order, split named LOD alternatives and enabled an explicitly approximate unlit base-texture preview. Akagi: 216,950 triangles = 158,296 main + 58,654 optional LOD; 122 default texture surfaces. Source vertex colors, fog/blend/alpha, V3-32 attributes, exact runtime LOD selection and active VU/front-face/basis verification remain pending. Evidence/hashes/static executable offsets: ../initial-d-ss/README.md. Focused 6/0, reader 45/0, full window 113/0 (84 skips), actual Genki 7/0; dev executable updated. No publication or game data added. Live visual comparison remains required.

### Initial D SS GIM correction — 2026-10-27

Native GIM now replays the declared PSMCT32 transfer into GS storage before reading PSMT4/PSMT8. This repairs 33 swizzled T4 Akagi images previously decoded linearly (trees/asphalt/course map); 106 other images unchanged. All 139 decode and checked image hashes pass. New automatic/manual viewport clipping and extended/fine zoom support close inspection. Targeted 9/0, reader 45/0, full window 115/0 (85 optional skips); dev executable updated. Shared GS address math is in ps2_gs_address.h, image_recipe.h keeps the compatibility wrapper. Current evidence and remaining effects/UI verification are in ../initial-d-ss/README.md and ../../CLAUDE-INITIAL-D-SS-HANDOFF.md.
