# Genki 0IVX: topology isolation

Current update (2026-10-25): native TXRD2/v1 mode-dependent draw suppression and TXR Zero transfer-form TIM2 are now supported for the validated layouts below. Historical investigation sections describe the earlier restrictions; the current rules table and final TXRD2 section supersede those restrictions.

## Recognition is not connectivity

`0IVX00.1`, `LDMX00.1` / `LDMX00.2`, and `MRON00.1` / `MRON00.2` identify related structures. Neither those strings nor valid DMA/VIF syntax establish how faces are generated. A game name, filename or folder is not used to choose the face decoder.

The previous importer incorrectly applied a C1-derived interpretation of Y bit 0 and a packet-tag parity field to all matching v2 streams. The user's corpus audit found **nine non-C1 files** that received faces this way: two Kaido Racer, one TXR Zero, three TXR3 and three TXRD1 samples. Their finite vertices and syntactically valid packets were not sufficient evidence for those faces.

## Current rules

| Profile / case | Behavior |
|---|---|
| `c1-v2-ybit-tag-parity-v1` | Face decoding only after all model packets meet its explicit structural/control checks |
| `genki-v2-explicit-gif-strip-v1` | Four-vector VIF control with explicit, mutually checked GIF triangle-strip tags; source positions preserved |
| `txrd2-v1-mode-adc-strip-v1` | Separate v1 packet/group checks; normal-present mode uses Y bit 0, no-normal mode uses color/alpha bit 0. No C1 fallback |
| Other v2 layouts or failed controls | Positions are evidence only; no inferred C1 faces |
| Mixed/partial/unvalidated model sections | No automatic partial face reconstruction; the aggregate result has zero validated triangles |

The C1-v2 profile requires:
- A complete bounded source and model boundaries established at offset zero or by the validated outer section table.
- The expected `LDMX00.2` / `MRON00.2` version pair.
- The two-record V4-32 control block at the expected VU address/cycle, followed by the expected V3-32 position stream.
- Agreement between the control-block count, packet count, control sign and packet parity.
- Two suppressed starting records, consistent with the sampled C1 strip convention.
- The supported ordered optional UV/normal streams and final stream, with matching counts and VU addresses/cycles.
- Expected command ordering and completion command; finite bounded control/position values.

Only after admission may the decoder clear the embedded Y bit and use it as draw suppression. This is a narrowly defined, sample-supported profile, not a general Genki/VIF topology rule or proof that every unseen asset using similar commands is compatible.

## Scanner, preview and AI

**Scan for 3D Data** reports position records and topology support separately. Unknown Genki packet layouts do not fall through to the generic raw-index detector. Unsupported topology is not auto-previewed.

**Preview Points (No Faces)** is an explicit manual action. It leaves coordinate bits unchanged, emits no triangles and does not guess UVs or normals from an unsupported attribute layout. The scene name and completion status say `NO FACES` / raw point preview. The assistant receives the topology diagnostic and must not invoke the C1 decoder for v1/unknown variants.

The decoder independently repeats recognition/admission before generating faces, so an old or manually altered UI result cannot bypass the topology gate. Cached evidence is invalidated when documents change. Newly installed builds do not retroactively validate geometry previously exported using the old rule.

The scanner now records bounded 0IVX model sections rather than inspecting only the first model section. `modelSections` reports version, boundary status, position-packet count, matching C1 packets, rejected packets, topology profile and rejection reason. Some files contain multiple models; directory boundaries are distinguished from header candidates recovered inside another section.

## Validation

Local corpus: 23 files with 0IVX headers. The audit recognizes all 23; six C1-folder v2 examples meet the profile. All 17 other/v1 cases remain blocked from C1 face decoding, including the v1 file in the C1 folder. Some packets in other games happen to match part or all of the C1 signature; this does not admit their whole mixed model.

The known C1 `000004b8.dat` retains 26,532 vertices, 21,942 triangles and 867 packets. Its native preview and GLB/FBX export are regression tested. Other C1 examples can now include additional bounded model sections; face counts differ from the previous first-section-only scan. Their visual/ground-truth verification remains separate from structural admission.

Synthetic regressions reject changed parity, count, version, strip-start controls, attribute layout, command order, invalid bounds and mixed models. Unknown/v1 raw previews are checked byte-for-byte for unchanged coordinates, and UI tests confirm no automatic point/face preview before explicit selection.

`native_asset_audit <folder> <report.json> --genki` records compact production Genki evidence without writing extracted game assets or using AI. Detailed local before/after reports and fingerprints are stored under the task workspace's `study-case-audit/genki-topology/`.

## Still required for TXRD2

Its topology needs a separate validated decoder based on its actual control/attribute streams, reset behavior and winding. A known-correct importer, recipe or exported reference mesh would allow comparison of vertex correspondence, connectivity and normals. Matching visual shape alone is insufficient. No TXRD2 rule is inferred from C1's behavior or merely from the shared v1 version markers.

The archive-layer documentation at [GUTArchiveTools](https://github.com/igorciz777/GUTArchiveTools) is useful for container/game variants; its archive compatibility table is not a geometry/topology specification and is not used to select this face rule.
## TXRD2 pre-vertex flag investigation

The user's recollection of a byte/flag before vertices led to a useful **mode/attribute flag candidate**, distinct from a proven connectivity rule.

A candidate v1 mesh-group header 48 bytes before the first DMA packet contains three independently checked fields: total position-record count, packet count, and total packet span in 16-byte units. These match all 19 groups in `txrd2/00000000.xvi`, all 54 groups in `txrd2/car/000002a0.sdb`, and all 22 groups found in `txrd2/00004336.dat` (95 TXRD2 groups). The same checks match 52 groups in the v1 `c1 ps2/00001010.dat` sample. This also demonstrates why game-name dispatch would be insufficient.

For groups whose prefix stays inside their model and outside the preceding packet group, a word **0xD0 bytes before the first DMA packet** has this measured correlation:
- bit 0 clear: every packet in the group has the observed V3-8 attribute stream;
- bit 0 set: that stream is absent.

All 95 TXRD2 groups, and the 52 additional C1-v1 groups, match this correlation. This does not establish whether the bit controls lighting, an attribute-layout mode, a VU path or some combination. It is not yet a validated triangle primitive or restart selector.

Concrete examples from `txrd2/00000000.xvi`:

| Field | First group | First group without the V3-8 stream |
|---|---|---|
| Candidate flag offset/value | `0x110` / `0x450` | `0x9BF0` / `0x11` |
| First DMA packet | `0x1E0` | `0x9CC0` |
| First position buffer | `0x220` | `0x9D00` |
| Group position-record count | 126 | 214 |
| Group packet count | 3 | 5 |
| Group packet span (quadwords) | 173 | 200 |

The immediate pre-vertex VIF control block also contains a word equal to count + `0x8000` in the sampled v1 packets. Its other tested sign word correlates with the packet's word at +8. Counts, sign correlation and VU addressing are not enough to establish faces.

There is an important contradiction to the universal C1 interpretation: all 57 V3-8-bearing packets in the XVI have the two starting Y low bits set, while only 3 of its 9 packets without that attribute stream do. In the car SDB, the split is 265/265 versus 4/35. Therefore treating all Y low bits as C1-style suppression and clearing them in every v1 packet would alter unvalidated coordinate data and generate unproven connectivity.

Native evidence now includes bounded `v1MeshFlagGroups`, `v1FlagStatistics`, `controlStatistics`, sampled pre-vertex control words and exact offsets. **Mark Recognized Structures** can mark the candidate flag words/control blocks for inspection. The assistant receives a compact version. These additions expose the evidence; TXRD2 faces remain disabled until its separate mode-dependent connectivity, resets and winding can be verified.


## Native material inspection (2026-10-24)

`GenkiMaterials::inspect` recovers 160-byte material descriptors preceding a recognized packet group. It checks the packet count, every decoded packet boundary and the declared quadword span. Material recognition never enables another topology profile. TIM2 banks must be complete, bounded entries of the same validated container directory; arbitrary signature proximity is not a bank association.

C1-v2 material parts can be rendered individually or with **Render All**. A local bank/image reference is assigned only when a supported UV stream, region-clamp mode, zero clamp origin, exact image dimensions and plausible TEX0 storage dimensions agree. Cropped, non-power-of-two TIM2 images use TEX0 storage-to-image UV scaling. Source coordinates are retained; source shaders, hierarchy/transforms and full PS2 blending are not reconstructed. The stored top-left UV convention is explicit in export/render bindings.

Measured on local study cases:

| Sample | Local TIM2 banks | Bounded material parts | Automatically textured parts |
|---|---:|---:|---:|
| C1 `000004b8.dat` | 2 | 165 | 78 |
| TXR3 `000028ca.nfc` | 1 | 5 | 0 (bank semantics/topology unvalidated) |
| TXR3 `000000c4.nfc` | 1 | 55 | 0 (global/local bank semantics unvalidated) |
| TXR Zero `0x000000ba.dat` | 1 | 136 | 0 (variant unvalidated) |
| TXRD1 `00001363.dat` | 3 | 47 | 0 (variant unvalidated) |
| Kaido `000004ac.dat` | 3 | 46 | 0 (variant unvalidated) |
| TXRD2 `00004336.dat` | 2 | 0 | 0 (separate v1 material table still pending) |

C1's 165 parts cover all 867 validated packets: 26,532 position records and 21,942 triangles, with no aggregate duplicate offered by the scanner UI. Untextured parts are not necessarily missing assets: some have no supported UV stream or a color-only layout. Material rows retain exact offsets, bank/image indices and resolution status. For the other games this is inspectable evidence, not a claim of reconstructed materials.

Historic layout reference: user-provided Discord scripts `IVX_rework_rev0.77_local_read-C18E0.py` and `txr3_reworkglob-dump_rev1.32-0909B.py` (read only, not executed). Register bit layout checked against [ps2dev gsKit](https://github.com/ps2dev/gsKit/blob/master/ee/gs/include/gsInit.h) and [ps2sdk libgs](https://github.com/ps2dev/ps2sdk/blob/master/ee/libgs/include/libgs.h). No game assets are included in the repository.

Of the 165 C1 parts, 82 have no supported texture/UV binding and 83 are potential textured parts. Five of those 83 remain unresolved: bank 1 images 16, 17, 18, 19 and 20 are 256x64, while the referring descriptors clamp 256x128. They are deliberately not assigned/scaled speculatively. Descriptor offsets are 0x48ab0, 0x4b280, 0x4e960, 0x532b0 and 0x57050. This may need a separate atlas/animation/layout rule; the discrepancy is not proof that the image is absent.

Validation: seven Qt test functions/lifecycle checks passed, including scanner Render All, GLB/FBX export, invalid bank/span rejection, native TEX0 UV scaling and isolation over 23 local 0IVX files. Tests use optional external study files; no copyrighted game data is added to the source tree.


## Other PS2 variants: UV and image-reference pass

The VIF inspector now recognizes bounded Float32x2 (`0x64`) UV streams, records their byte stride and rejects nonfinite UV values for material/image association. This does not admit a triangle profile or change the raw position preview. C1's existing Int16 UV decoding and face gate remain unchanged.

The material inspector distinguishes `textureReferenceResolved`, `textureDecodeSupported` and `textureAssigned`. A compatible native bank/index reference can be inspectable and previewable even while the corresponding mesh topology is unsupported. These are per-material references, not counts of unique textures or proof of a reconstructed in-game shader.

| Study file | Compatible local material/image references | Decoded image preview |
|---|---:|---:|
| TXR Zero `0x000000ba.dat` | 14 | 0: nonstandard TIM2 storage pending |
| TXR3 `000000c4.nfc` | 18 | 18 |
| TXR3 `000028ca.nfc` | 5 | 5 |
| TXRD1 `00001363.dat` | 8 | 8 |
| Kaido `000004ac.dat` | 25 | 25 |

The indexed mapping is restricted to images in the active file's complete, bounded TIM2 banks, matching dimensions and a supported UV layout. No folder search substitutes unrelated global banks. Old type-0 material headers place CLAMP at +64; the type-4/type-6 layout uses +80. Explicit CLAMP origins and limits are retained. A Kaido reference at 0x3beb0 samples the 2x2 region (254,254)-(255,255) of its 256x256 image; the preview intentionally shows the entire source image and describes that sampling region.

In **Scan for 3D Data**, compatible rows expose **Preview Texture**, opening the image through the normal 2D decoder. They are not added to Render All as models. Automatic first-model preview does not unexpectedly switch to a material image. Unsupported or missing references retain their exact descriptor offset, UV samples and a resolution reason. Image banks that fail structural validation retain their slot rather than renumbering subsequent valid banks.

The separate 208-byte v1 material/state prefix is now exposed for 19 groups in `txrd2/00000000.xvi`, 54 in `txrd2/car/000002a0.sdb` and 22 embedded groups in `txrd2/00004336.dat`. Its texture-bank/index encoding and connectivity remain unresolved. Float opacity/state fields are not interpreted as image IDs. Capped v1 group evidence now retains up to 1024 groups; the assistant still receives a compact summary.

TXR Zero's supplied bank has 43 structurally bounded TIM2 pictures: 41 use PictFormat 0x10 and two use 0x30. The [TIM2 v4 specification](https://github.com/GirianSeed/tim2/blob/trunk/webtech/tim2v4b_e/tim2.txt) defines PictFormat as zero. A local linear decoding probe produced scrambled images, so the decoder was not loosened to accept unknown storage. Such pictures appear as **Image storage / Decoder required**, not automatic image recipes. Fourteen material/index/dimension references can still be inspected, but no decoded image is claimed.

The provided TXR2 Dreamcast `CARMODEL_00004.bin` and complete 72,738,816-byte `COURSE.AFS` contain no ordinary PVRT/GBIX headers. The latter has 2258 bounded AFS resources and is a different format family. No PS2/TIM2 material interpretation is applied to these Dreamcast resources; their texture encoding/linkage is still pending.

Validation: all eight Qt test/lifecycle checks passed, including real Preview Texture completion, all compatible source-image decodes, nonfinite UV and malformed bank rejection, unchanged C1 Render All/GLB/FBX export, and topology isolation across the 23 local 0IVX fixtures. No non-C1 face decoder was enabled by the material work.


## Explicit GIF strip profile (TXR Zero / TXR3 / TXRD1)

A separate native profile admits complete bounded v2 models whose every packet contains a four-vector V4-32 control block. It checks the packed GIF tags, triangle-strip primitive, vertex count, register list, paired parity, native bounding box and all vertex coordinates. Ordered Float32x2 UV, SNorm16x4 normal and V4-8 color streams are recognized. Strips restart at every DMA packet. This profile does not clear the low bit of Y and never applies the C1 suppression rule. Degenerate triangles are omitted; arbitrary long edges are not filtered.

Validated packets lacking a recognized material descriptor remain available as explicitly untextured packet parts. Render All therefore covers the whole admitted model without adding a duplicate aggregate. Supported local material references use native Float32 UV and decoded TIM2 images. Unresolved references are left untextured rather than matched to a guessed external bank. Image previews remain accessible alongside renderable models.

The initial corpus covers TXR Zero `0x000000ba.dat`, TXR3 `000000c4.nfc` and `000028ca.nfc`, and TXRD1 `00001363.dat`. Multi-model files with unestablished boundaries remain blocked, as do the supplied two-vector Kaido layouts and v1 TXRD2 connectivity. Game names are not admission criteria.

TXR Zero storage experiments indicate that the 0x10 indexed4 pictures resemble a GS16 transfer and 0x30 indexed8 pictures resemble a GS32 transfer. This remains research evidence, not an enabled general TIM2 decoder: transfer pitch, palette banks and platform-specific storage flags need independent validation. No unknown PictFormat is silently accepted.


Validation for the explicit GIF profile: ten Qt checks passed (including lifecycle), zero failures/skips. The four corpus cases were decoded and exported as both GLB and FBX; per-part face totals equal aggregate totals. The TXR3 wheel was loaded through the actual scanner/Render All path with five textured parts, then rendered again to verify deduplication. C1 retains 165 parts / 21,942 faces / 78 textured parts; the 23-file isolation corpus still passes.

| File | Render parts | Triangles | Textured parts |
|---|---:|---:|---:|
| TXR Zero `0x000000ba.dat` | 185 (49 without recognized descriptors) | 2,239 | 0 |
| TXR3 `000000c4.nfc` | 55 | 6,786 | 18 |
| TXR3 `000028ca.nfc` | 5 | 990 | 5 |
| TXRD1 `00001363.dat` | 47 | 5,312 | 8 |

Orthographic projections of exported geometry were inspected locally. This validates recognisable geometry and packet coverage, not complete game-scene placement, shader reproduction or all LOD/animation semantics. No game assets or generated exports are published.


## TXR Zero indexed TIM2 transfer storage

The decoder now supports the observed Genki single-level 128-byte picture-header layouts: PictFormat 0x10 / Indexed4 transferred through GS16, and 0x30 / Indexed8 transferred through GS32. It reconstructs GS local memory from the compact transfer then reads the indexed surface at TEX0 pitch 128. A written-byte map rejects uncovered texels instead of inventing zero pixels. Unknown flags and mismatched dimensions, TEX0 PSM/pitch, palette metadata, mip counts or payload sizes remain unsupported. This is a bounded Genki storage profile, not a universal meaning assigned to all nonzero TIM2 flags.

In `txr zero/0x000000ba.dat`, all 43 pictures decode (41 Indexed4, two Indexed8). The 14 compatible local material references now receive decoded images; the remaining material references still require independent bank/layout work. Palette alpha and the existing CSM1 index permutation are retained. Image scanning, Preview Texture, native mesh material binding and GLB/FBX export share this decoder.

The decoded contact sheet was inspected locally: logos, licence plate, lamps and effects are recognisable and readable. Truncated payload, unknown flag, altered TEX0 pitch and cancellation cases are covered by tests. No game textures are committed or uploaded.

A full native corpus audit also confirms the explicit GIF profile admits the directory-bounded multi-model TXR3 `0000603f.nfc` (1,402 triangles), TXRD1 `0000059f.nfc` (1,000) and `00000d09.nfc` (3,256). The raw concatenated TXR Zero `0x00000493.dat` and `0x000004d2.dat` still need independently established section boundaries. Kaido's two-vector controls and TXRD2's v1 controls remain separate unresolved face profiles; no C1 rule was substituted.


Final follow-up tests: 11 passed, zero failures/skips. All seven admitted TXR/TXRD1 files load and export GLB/FBX. Zero-face material/packet parts are omitted from render candidates; triangle coverage suppresses the aggregate duplicate without requiring degenerate packet records to become meshes. The additional three multi-model cases are currently geometry-only (material bindings unresolved).


## TXRD2 v1 draw suppression: current implementation

The user remembered a per-record byte marking skipped faces. Read-only review of `IVX_rework_rev0.77_local_read-C18E0.py` found an explicit TXRD2 track rule using the low bit of the fourth V4-8 byte. Its alternate attempt to split at the sign of a normal component is not used: normal signs describe direction, not a reliable strip restart. The older `nfc_fullspidey_car_txrd2_2024_rev1.1-4B4D6.py` simply chained each packet and did not implement per-record suppression.

The native v1 profile distinguishes two validated group modes:

- Mode flag bit 0 clear, V3-8 normals present: per-record position Y bit 0 suppresses the triangle. Clear only this embedded bit when decoding Y.
- Mode flag bit 0 set, no V3-8 normals: use bit 0 of V1-8 (`0x62`, one-byte stride) or the fourth byte of V4-8 (`0x6E`, four-byte stride). Preserve all coordinate bits.

A set draw bit omits the triangle **ending at that record**. It does not remove the vertex or reset parity. Sequential strip parity continues for every record; each DMA packet begins a separate strip, with its native control/tag parity checked. The first two draw markers must be set. Degenerate triangles are omitted, without arbitrary edge-length filtering.

Admission requires the complete v1 version pair, exact two-vector control/position VIF addresses/cycles, count/parity agreement, ordered supported optional streams, complete packet bounds/kick, and a matching group header. The group declares total vertices, consecutive packet count and quadword span; the mode bit must agree with normal presence in every packet. Every packet must belong to a validated group. Unknown/mixed/truncated variants remain point evidence only.

Nested relative offset tables in an already bounded container section establish the boundaries of the 22 embedded XVI models in `00004336.dat`. Counts, aligned ordered offsets and parent bounds are checked; searching for magic strings alone does not admit those sections.

| Study case | Native material groups | Triangles | Texture bindings |
|---|---:|---:|---|
| `txrd2/00000000.xvi` | 19 | 1,692 | Pending |
| `txrd2/car/000002a0.sdb` | 54 | 8,328 | 19 primary local texture bindings (development update 2026-10-25) |
| Embedded models in `txrd2/00004336.dat` | 22 | 352 | Pending |

The 8,328-triangle car contains 7,286 normal-bearing faces; all agree in orientation with their stored normals. In the XVI, 1,470 agree and 10 disagree, so stored normal agreement is corroborating evidence, not a universal face filter. Exported car silhouettes were inspected locally. The 352 embedded faces are small scattered resources, not a claim of a reconstructed complete track scene or hierarchy.

Tests cover a synthetic omitted-face sequence without parity reset, exact single-face removal by a draw-bit mutation, rejection of missing initial markers and mismatched mode, GLB/FBX export of all three files, and actual scanner Render All of the car. Existing C1/TXR profiles remain isolated. Material bank/index interpretation, some v1 UV encodings (V4-16 at 0x8003), external assets, hierarchy and game shaders remain separate work.

## Native primary texture update — 2026-10-25

The March v1 path now reads the bounded headerless Indexed8 bank with all six slot IDs preserved, resolves the primary namespace-2 material slot at prefix +68/+70 against TME (+72) and CLAMP (+88), and uses the first pair of V4-16 UV records at 0x8003 with stride 8. V2-16 UV records remain stride 4. Full-image zero-minimum CLAMP is carried to the native renderer and model exporters. Empty reference-only records are not pixel data and remain unresolved. The image scanner exposes only the three stored surfaces.

TIM2 RGB24 images/palettes use 3-byte colours. Verified transfer-form TIM2 layouts also admit observed 16-pixel sizes and RGB16 palette storage; reconstructed writes must cover every requested pixel byte. The Zero AE86 candidate decodes all 42 source images. Existing C1/GIF-strip/v1 face profiles remain separate; primary image support does not reconstruct secondary shader stages, visibility/LOD selection or external banks.

See [implementation and validation evidence](research/genki/checks/MATERIAL-INTEGRATION.md). These changes are in the development checkout and were made after the public 0.3.6 release.
