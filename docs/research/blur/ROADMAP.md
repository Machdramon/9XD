# Blur native analysis roadmap

Updated: 2026-10-28. Owner: Codex/native analysis; UI refinement may be handled by Claude.
Status: first bounded native single-static-draw profile and inline images integrated; broad ARCH/multibuffer/scene support still pending. See the implementation receipt at the end.

## Scope and evidence

Read-only study samples: `C:/Users/Jarky/Documents/study cases/blur`.
Read-only full PC installation: `F:/copia de seguridad 08.04.2019/downloads/blur/Blur`.
Private probe scripts/reports/previews: `C:/Users/Jarky/.codex/.chatgpt-projects/g-p-6aafac188d248191a492c99f58d0a766/blur-research/`.
Source game data, decoded images and third-party executables are not included in this repository.

| Fixture | Bytes | SHA-256 |
|---|---:|---|
| audi_s3_stock.model | 4,343,421 | ecd921ef2281b64c2974948a155c80ba780ddba14360a2d38def767e2fe7b521 |
| igm_japan_minor2.model | 233,288 | 1702bf3556907c8c3a85efe118acd5e04a7311fedd6de45197c420bedb3f7cd3 |
| level.levelstream | 24,725,504 | 7c0d17eb35c3a0969f7903a97fc478cf7ddb6c88d562d4f8da985c36c24541ee |

The study stream is byte-identical to the installation's `levels/showroom/level.levelstream`.
It is the showroom, not evidence of a complete race track. The named minor character is a skinned
female character, not an ideal first static-mesh fixture. The installation also has a separately
named wheel resource for the Audi: do not confuse the body and wheel files sharing one basename.

## Format inventory

Counts are files in this particular installation, not counts of supported assets.

| Format/family | Count | Observed structure / purpose | Current result and remaining work |
|---|---:|---|---|
| cache/*.pak | 7 | All observed `2KAP`; six v2, shaders-speed v1; alignment 2048 | Name recovery and two directories validated; one v2 model decompressed exactly. Full native adapter and codec variants pending. |
| *.model | 1,544 | 1,495 start with `  CPModel`; 49 start with `Model` | Two CPModel chunk trees traversed to EOF. Parse raw Model variant separately; no meshes decoded yet. |
| *.texture | 301 | `Texture` chunk prefix | Standalone texture schema needs validation; do not assume DDS wrapper. |
| *.dds | 139 | Existing standard image family | Candidate for existing DDS pipeline; no corpus decode performed this session. |
| *.level | 19 | `  CPLevel` | Metadata/container, not standalone mesh stream. Showroom chunks validated to EOF. |
| *.levelrender | 19 | `  CPRender`, Scene with ARCH data | Shader/material names and 15 bounded inline DXT records in showroom; 184,287 trailing bytes need interpretation. |
| *.levelstream | 19 | ARCH serialization, repeated aligned segments | Showroom has 31 paired ARCH signatures; starts aligned to 2048. Segment lengths/references and streaming payload association unresolved. |
| *.levelphysics | 19 | `  CPPhysic`, Header/Main/Routes | Showroom outer chunks validated. Collision reader must remain distinct from visual geometry. |
| *.skeleton | 20 | Binary hierarchy candidates; observed leading versions 6/7 | Bone names also visible in character ARCH data; hierarchy/bind-pose layout pending. |
| *.cycle | 805 | Observed leading integers 14/15 | Animation candidates; channel/keyframe layout pending. |
| *.bpr | 4 | `2RZB` | UI resource family by paths; investigate separately, not as CPModel geometry. |
| *.dat | 367 | Mostly `JBOA`, plus unrelated files | Serialized gameplay/config candidates; classify by magic/path, never extension alone. |
| *.ipf | 75 | UI-associated paths, binary headers | Low-priority UI resource research. |
| *.baf / *.bik | 126 / 31 | Audio-associated BAF paths / video files | Outside initial 3D extraction scope. |
| *.pf / *.particlemesh | 5 / 1 | Particle-related candidates | Defer until static meshes/materials work. |
| shaders-speed.pak | included above | 1,256 declared entries, recovered names include shader files | Later shader role/constant research; no shaders executed/decompiled here. |

## What is actually verified

### PAK container and compression

- Read local `cache/blur.bms` (QuickBMS Blur script 0.3.3) as a reference, not executed.
- Independent bounded Python probe recovered readable name tables for all seven PAK headers.
- Native data directory parsing verified 3,601 entries in root.pak (3,388 data-bearing) and 931
  in boot.pak (718 data-bearing): name offsets/null terminators and data start offsets checked.
  This does not yet verify every compressed payload extent or CRC.
- A segmented deflate/zlib path recovered igm_japan_minor2.model from root.pak to exactly 233,288 bytes,
  matching both the extracted file and study fixture SHA-256. This is one successful payload,
  not proof that every compression variant is solved.
- Opposite-endian `PAK2`/XMem variants are described by the local script but NOT observed or validated here.
- The Xentax attachment `6778/blur_loc_ext.7z` lists only blur_loc_ext.exe; it was not executed.
  It is not a mesh import recipe.

### CPModel and ARCH

The two CPModel samples have a 20-byte outer header; child records have an 8-byte padded tag,
little-endian total length including the 16-byte child header, and a flags word. Their children
cover the entire file with bounds checked. Nested children were admitted only when they cover
the complete parent payload; arbitrary embedded strings were not accepted as chunks.

Audi top-level children: Header, MdlDat, Render, Coll, MtaDbg. Character has the same family.
MdlDat exposes StrTab/Models/Element/Constr; Render contains nested Render/Header/Scene and ARCH.
Keep chunk framing separate from ARCH object serialization; `RA` markers are not proven mesh tags.

Audi metadata includes body, door, wheel, light and interior names and veh_Paint/Glass/Light/Alloy
shader families. Showroom includes RenderingData::CullNode/RenderListNode_Common, g_mWorld,
g_vUVOffset, g_tAlbedo, g_tLightmap, g_tNormal and effect names. These are useful evidence for
scene/material links, not a validated assignment algorithm. Never equate a string occurrence
with a complete buffer descriptor or reuse PGR sampler/layout rules automatically.

### Images

A conservative structural probe validates seven inline DXT records in each study model and 15
in showroom levelrender. Only records with plausible power-of-two dimensions, depth 1, bounded
mip count, exact declared size and complete block payload are counted. Other DXT occurrences
remain unresolved; these counts are NOT the total texture count or material coverage.

At the observed inline FOURCC offset F: hash at F-24, record size F-20, width F-16, height F-12,
depth F-8, mip count F-4; data begins F+12. Declared size equals 28 plus complete mip-chain bytes.
Use this only after the enclosing serializer confirms the record; signature scanning is research.
The first character texture (128x128 DXT1, 8 mips) was wrapped in a temporary DDS and decoded;
face/hair atlas preview is coherent. Private output remains outside repo.

Showroom stream references instead declare size 28 with a sentinel at F+4; there is NO inline
pixel payload under this schema. Resolve external/streamed payloads before decoding them.
Do not treat bytes following the header as pixels, or guess textures by nearest offset.

## Ordered implementation checklist

1. [x] Inventory study fixtures and full-installation formats, preserve hashes and private probes.
2. [x] Verify CPModel outer framing, conservative inline DXT layout and one actual decoded texture.
3. [x] Verify PAK names/directories and exact decompression of one model without old executables.
4. [ ] Native bounded CP chunk reader: distinguish CPModel/Model/CPLevel/CPRender/CPPhysic variants;
   preserve unknown chunks and reject overflow/truncation. Investigate levelrender tail.
5. [ ] ARCH serializer: map IDs/references, object types, arrays, versioning and segment boundaries.
   Record source offsets and avoid a generic scan of float triples as an importer.
6. [ ] First static mesh: use system envsphere.model or another small prop, then Audi. Recover
   vertex declarations/strides, index type, draw ranges, primitive topology/restarts, bounds,
   coordinate basis, UVs and normals. Validate each index; do not inherit PGR layouts blindly.
7. [ ] Material graph: draw -> material -> named shader inputs -> image identity -> pixel payload.
   Resolve standalone Texture records, inline versus streamed records, mips and alpha/samplers.
   Start with albedo only; retain lightmap/normal/environment roles separately.
8. [ ] Audi assembly: preserve named parts, LOD/damage alternatives, wheel companions, local matrices.
   Compare stock body and wheels; no automatic loading of every overlapping variant.
9. [ ] Scene assembly: join the showroom level/levelrender/levelstream by validated references,
   then one race track. Recover per-instance matrices, streaming resource IDs and LOD selection.
10. [ ] Optional native PAK reader: port only observed variants, retain source paths and bounded
    RAM resources, add limits/cancellation/source identity checks. Full decompression corpus next.
11. [ ] Wire verified meshes directly into Scan for 3D Data / Render All and normal export path;
    images into 2D scan. No AI or external converter required for recognized layouts.
12. [ ] Synthetic malformed-input tests plus actual static prop, Audi/wheels, showroom and one
    track; finite vertices/UVs, indices, no cross-draw faces/duplicate LODs, GLB/FBX and live Vulkan.
13. [ ] Later: character skinning, skeleton/cycle animation, original shaders, collision and particles.

## Acceptance and current limits

No vertex or triangle reconstruction, material application percentage, 9XD rendering or export
is claimed in this research pass. No application code changed or build was needed. Geometry
and material binding are the main next tasks; full-installation raw resources mean PAK extraction
does not have to block the first mesh prototype. Existing BC codecs/shared renderer are reusable;
Blur serialization, declarations and links require their own validated adapter.

For future progress use separate columns: vertices, faces, compression, material-assigned parts,
decoded images, placements/LODs, export and live validation. Material percentage must specify
fixture and denominator; decoding a texture is not assigning it to a model.

## First native integration receipt — 2026-10-27

This supersedes the reconnaissance-only status above without marking the full ordered checklist complete.

- Implemented `src/blur_model.h`, discovery in `src/asset_discovery.h` and dispatch in
  `src/mesh_profile_decode.h`. Recognizes complete CPModel framing and a Scene ARCH v1 carrying
  exactly one supported typed stride-20 position buffer, UInt16 index buffer and a verified
  full-buffer primitive-5 draw. Padding is bounded and the vertex blob's typed successor is
  checked, avoiding ambiguous marker matches after zero-prefix vertices.
- Finite positions, valid indices, bounded input/memory, patch-aware reads, cancellation and
  source snapshot recheck; degenerate strip triangles omitted with parity retained. Source basis
  preserved, normals generated; UVs, original packed normals and material binding NOT implemented.
- Real envsphere.model and specsphere.model: each 401 vertices, 1,090 indices and 760 triangles.
  Independent cross-product/centroid probe finds all 760 sphere triangles outward. Other ARCH
  layouts, multi-buffer Audi and skinned character are deliberately not admitted as meshes.
- Inline DXT1/3/5 records exposed to 2D image scanning using existing codecs, bounded parent Scene,
  dimensions and exact mip-chain size. Seven images each on Audi/character decode at expected
  dimensions in production decoder. No images assigned speculatively to material slots.
- Focused suite: 6 passed / 0 failed / 0 skipped (four test functions plus lifecycle), log
  `build/windows-local/blur-tests.txt`. Synthetic malformed/truncated/nonfinite/index/cancel/stale
  candidate checks; actual sphere geometry, GLB/FBX export; actual 14 image decodes and malformed
  image header; asynchronous scanner-to-scene load and repeat-preview deduplication.
- These UI checks use Qt offscreen, not a live Vulkan framebuffer comparison. Full regression
  and application build receipts are recorded in COORDINATION-PLAN.md.

### How to try the first slice

Open installation `cache/root/dontship/models/system/envsphere.model` (or specsphere.model),
then Scan for 3D Data; the single mesh can be previewed and exported. Open the original Audi
or character model and Scan for Textures / Images to inspect the seven supported inline images.
Their 3D scan reports unsupported layout until the multibuffer/skinning stages are implemented.

### Next implementation priority

Map ARCH declaration/buffer identities and per-draw vertex/index/material references for the
Audi before supporting multiple buffers. Do not pair arrays merely by proximity, ignore LOD
boundaries, or report a car as supported based on these sphere tests. Native PAK integration,
standalone Texture, streamed payloads, scene assembly, skinning and shader fidelity remain open.
Final validation: full window suite 124 passed / 0 failed / 86 optional skips; development binary_workbench.exe rebuilt successfully. A prior About icon fallback regression was repaired without changing Claude's animation work. No live Vulkan screenshot comparison, release, commit or push performed in this slice.

## Live scenery props — 2026-10-27

The existing reader was tested live via Scan for 3D Data in 9XD's RTX 4060 Ti Vulkan viewport:

| Shared construction resource | Vertices | Triangles | Live result |
|---|---:|---:|---|
| trashcontainer_small.model | 100 | 52 | Closed bin, coherent geometry, rendered |
| dam_dirtyconstructionequipment_b.model | 385 | 302 | Open construction skip with lifting eyes, coherent geometry, rendered |
| concreteblock1.model | 24 | 12 | Private structural probe only; not loaded live |

Source folder: installation cache/root/models/objects/shared/construction. The app remains on
its second rendered object. No base images are assigned yet; the viewport explicitly warns of
missing source color texture. These are scenery props, not extracted track surface or proven
placements in a particular course. This supersedes the earlier lack of live Vulkan validation
for this narrow static profile, not the outstanding full-scene work.

A search of 117 eligible scenery resources stopped at three full-single-draw candidates.
Japanese lamposta/b and several other props have plausible buffers but no full-draw tuple;
actual lamposta scan correctly yields no mesh. Extend per-draw semantics before admitting them.
No decoder change or rebuild was needed for this live check.
## ARCH research continuation — 2026-10-27

See [draw/declaration/texture findings](ARCH-FINDINGS-2026-10-27.md). This corrects the earlier
external-only hypothesis for size-28 texture records: many have locally resident split RA blobs.
Independent probe decodes 854 showroom base images (including repeated records), with 213
full-chain/641 base-only stored layouts; 17 DXT occurrences remain unresolved. Candidate keys
are not globally unique: 42 key/format/dimension combinations produce different RGBA pixels.
No speculative binding added.

Lamposta: nine ranges exactly partition 5,292 indices; minimumVertex must not be added as a
base vertex. Stream byte offsets resolve its 3,055-vertex pool; independent strip decoding gives
2,063 triangles, removing 16 spurious boundary faces from aggregate interpretation. Audi contains
100 bounded packets of four ranges, showroom 226 single-range candidates; these are structural
counts, not admitted native meshes or a rendered scene. Several typed vertex declarations and
coherent half UV streams are documented. Production reader remains unchanged in this pass.

Next: native split-image support with bounded tests, counted draw arrays and exact object/buffer
references, source material mapping with namespaces, then showroom assembly. Keep all LOD and
state alternatives separate until their selection is known.
## Named-albedo preview and split images — 2026-10-28

Implemented for the already admitted single-static-draw profile:

- Float16x2 UV0 at vertex offset16/stride20, source coordinates retained. Invalid/nonfinite UVs
  preserve geometry but disable UV/texture assignment; no blanket U/V flip introduced.
- Image names are tied to their exact serialized image object boundary (RA v0/type2 counted
  string followed by RA v1 image definition), not to an untrusted global 32-bit identifier.
- If and only if the file has one unambiguous image named *_diff / *_diffuse / *_albedo and the
  admitted mesh has finite UVs, it is used as a named color preview. **This is an approximation
  from resource naming, not a recovered original GPU binding.** Metadata explicitly records
  textureAssignmentVerified=false, gameShaderApplied=false and the original name/offset.
  The preview is unlit; no ipr/normal/environment map is substituted. Ambiguous names stay unbound.
- The raw ARCH stream image scanner now reads bounded split blobs, either full resident mip
  chains or base-only payloads. First 128 image recipes are listed, preserving offset identity;
  this is not a claim that all 854 privately decoded records are listed or mapped to materials.
- No map geometry, per-instance assembly, multi-draw or skinned mesh support added in this slice.

Changed src/blur_model.h and tests/window_tests.cpp. Focused suite: 8 passed/0 failed/no skips,
including named diffuse images on both construction props, finite UVs, invalid-UV geometry
preservation, ambiguity rejection, textured GLB export, three actual split-image decodes, corrupt
length rejection and cancellation, plus existing geometry/images/UI tests. Log:
`build/windows-local/blur-material-preview-tests.txt`. Full regression/build/live receipt follows
in COORDINATION-PLAN.md. Source game files unchanged; no release/publication.
Final validation: full window suite 126 passed/0 failed/86 optional skipped; development executable
rebuilt. Live normal Scan for 3D Data loaded dam_dirtyconstructionequipment_b.model in Vulkan
with visible color texture, 385 vertices/302 triangles. Left open for owner review. Original
GPU bindings and complete scene assembly remain pending. Next native owner: Codex, counted
draw arrays and exact buffer/object/instance references before full-map material assignment.

## Audi UV comparison follow-up — 2026-10-28

Owner flags prop UV appearance. Audi is present but still not renderable by the current native
profile. Broader read-only attribute scan finds 12 declarations (strides12–40), including
UV semantic5 type9/channel0 and additional channels; type9 encoding and blob/reference
boundaries remain unverified. Seven exact printable image names cover tyres/brakes/damage
resources rather than one car-wide diffuse map. Do not apply the prop stride20/offset16 rule
or assign one image to the whole car. Next: exact typed blob and draw/material references,
then car UV checker/source-texture comparison. Prop UV concern remains open. No app changes.

Additional live UV fixtures (2026-10-28): small bin textured 100 vertices/52 triangles;
concreteblock1 textured 24/12, left visible. Owner finds bin plausibly correct. Concrete
appearance coherent but insufficiently asymmetric to establish orientation. Shader 1-V
versus missing Blur sourceUvTopLeft flag is a hypothesis to validate against material/game
reference, not an applied fix. No code/build change; preserve existing source UVs for now.

## Pallet and standing-light admission — 2026-10-28

No native scan results because current profile requires one full-buffer draw. Pallet variants
1–4 have 9/9/10/9 draw ranges; standinglight and standinglight2 have 10 each. Private validation
confirms exact index partitions, minimum/count and byte-offset bounds. Per-draw strip resets
yield 526/738/1268/1930 pallet triangles and 2610/2730 light triangles; an aggregate strip
adds 16–18 false boundary faces. Next: tested counted draws plus exact object/state/material
references. Geometry is present; fully assembled textured preview not yet supported. No app change.

## Native local-section support - 2026-10-28

Implemented per-draw candidates for a single typed stride20 vertex/index buffer: exact index
partition, minimum/count checks, bounded stream-byte offsets and independent strip parity.
Unused vertices are compacted for correct section framing/export; source vertex offsets remain.
Render All loads sections independently with unique source identities. Multi-draw textures
remain unassigned: original material links, dynamic object transforms/states and LOD selection
are still pending. This is a local-section preview, not proven complete game-object assembly.
Focused suite: 10 passed/0 failed/no skips, six actual models plus malformed range/overlap/offset
rejection, stale candidate, existing single-draw behavior and scanner Render All/dedup test.
Log build/windows-local/blur-multidraw-tests.txt. Full regression and live validation pending.

Final validation: 128 passed/0 failed/86 optional skips, executable rebuilt/installed. Live
Vulkan scan + Render All verified pallet 9 entries/526 triangles and standinglight 10/2610,
zero failed or omitted, existing first section deduplicated. Both recognizable; lamp active
and pallet in adjacent document. Lamp all-sections view overlays bent/alternative components,
so dynamic state selection is explicitly unresolved. Expected texture warnings: multi-draw
material assignment remains pending. No complete-state/full-map fidelity claimed or release made.
