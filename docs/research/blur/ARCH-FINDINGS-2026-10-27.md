# Blur ARCH: draw ranges, declarations and split texture storage

Updated: 2026-10-27. Research owner: Codex. Read-only analysis; production decoder unchanged.
This extends ROADMAP.md; raw files, decoded images and full offset manifests remain private.

## Findings and confidence

| Area | Demonstrated on local files | Not yet demonstrated |
|---|---|---|
| Multiple draws | Lamposta has 9 bounded ranges exactly partitioning its 5,292 indices | All ARCH state/reference variants and model placement/LOD selection |
| Vertex stream offset | Per-draw byte offset fits the actual 3,055-vertex stride-20 pool for all 9 lamp ranges | Universal meaning across multi-stream bindings |
| Index metadata | Stored minimum/count equals observed min/max range on all 9 lamp draws | Do not reinterpret that minimum as a base-vertex addition |
| Strip topology | 2,063 nondegenerate faces with per-draw parity reset, versus 2,079 from joining the whole index pool | No source-game comparison or final material/LOD assembly |
| Audi subdraws | 100 bounded packet candidates each contain 4 structurally framed ranges (400 ranges) | Range-to-buffer binding and which ranges are LOD alternatives |
| Showroom | 226 bounded packet/range candidates, 31 aligned paired-ARCH starts | 226 is not a verified mesh count or scene completeness measure |
| Vertex layouts | Five fully bounded declaration/payload records found in showroom stream, strides 20/24 | Complete declaration coverage, packed-attribute semantics and bindings |
| Texture storage | 854 DXT records with bounded payloads decode successfully in independent DDS/Pillow probe | Native 9XD split-storage integration and exact material binding |

## Reproducibility

Private workspace: `C:/Users/Jarky/.codex/.chatgpt-projects/g-p-6aafac188d248191a492c99f58d0a766/blur-research/`.
Probes: draw_probe.py, draw_validation.py, declarations_probe.py, stream_textures_probe.py,
research_summary.py. Reports: draw-validation.private.json, all-primitives.private.json,
declarations.private.json, stream-textures.private.json, analysis-summary.private.json,
texture-scope.private.json. Originals unchanged. No external exe or community script executed.

### A. Lamp draw boundary and vertex-reference evidence

Source: installation `cache/root/models/objects/jpn_mountaintrack/lamposts/lamposta.model`.
One typed 20-byte vertex buffer holds 3,055 vertices; index buffer holds 5,292 UInt16 indices.
The primitive-5 packet wrapper has a counted array of 20-byte range records, each starting RA v0.
For the observed subset the four words in each range are:

1. first index slot;
2. index count;
3. minimum vertex index (NOT a value to add);
4. vertex range count.

Evidence: each stored minimum equals actual minimum index; maximum-minimum+1 equals stored
vertex range count on all nine ranges. Adding that minimum again would exceed the pool on
the first range. The field called `baseVertex` in early private probes is therefore a stale
research label: use `minimumVertex` in future implementation.

| First index | Count | Observed minimum | Vertex range count | Stream byte offset |
|---:|---:|---:|---:|---:|
| 3147 | 129 | 1544 | 410 | 0 |
| 0 | 3147 | 0 | 1889 | 0 |
| 5251 | 41 | 0 | 23 | 60640 |
| 5226 | 25 | 135 | 51 | 56920 |
| 4931 | 295 | 0 | 172 | 56920 |
| 4254 | 588 | 0 | 318 | 49600 |
| 4842 | 89 | 110 | 256 | 49600 |
| 4232 | 22 | 139 | 387 | 39080 |
| 3276 | 956 | 0 | 515 | 39080 |

These sorted ranges cover every index exactly once without overlap. Byte-addressing vertices
as `poolStart + streamByteOffset + sourceIndex * stride` stays within the pool and yields finite
positions on every range. In the single-stream one-range layout the byte offset appears at
packet+76; DO NOT hardcode this offset in the general parser: Audi has four range records and
variable/multiple subsequent bindings, shifting the layout.

Independent strips yield 2,063 nondegenerate triangles. Joining the whole index pool yields
2,079, introducing 16 boundary faces. This is a measured connectivity difference, not a
distance filter or a source-game visual validation. MdlDat names include separate elements;
selection/placement/LOD semantics must be resolved before rendering all parts together.

### B. Declarations and UV evidence

A bounded declaration walker validates stride, attribute count, each RA attribute record,
UInt16 attribute offsets, vertex count, blob byte size (stride*count), padding and successor.
No generic scan of float triples is used.

Observed simple declaration: serialized attributes `(6,0,0,0)`, `(11,0,3,0)`, `(8,0,5,0)`
at offsets 0,12,16 in stride 20. Float32x3 positions are already validated. Interpreting the
type-8/semantic-5 pair as Float16x2 produces finite coherent UV coordinates in lamp and several
showroom buffers. It is strong local layout evidence, not a complete enum specification.

Showroom validated payloads: 15 vertices/stride20; 14,420/stride24; 6,525/stride24;
8/stride24 (Float4 position candidate with two UV-like attributes); 15,722/stride24.
One uses a type-9 UV-like declaration, still unimplemented. No total-scene vertex count is
claimed: this conservative walker does not cover all stream/reference variants.

UV ranges are not forcibly normalized to [0,1]. Lamp repeats roughly U [-.998,.998],
V [-.939,3.311]; larger showroom buffers contain modest negative/>1 coordinates. Preserve
sampling/addressing and source UV transforms; do not introduce blanket flips/clamps.

### C. Split DXT storage: major correction to reconnaissance hypothesis

Earlier research treated size-28/sentinel headers as possibly external-only. The showroom
demonstrates a local split-storage form: at FOURCC offset F, declared size is 28, F+4 is -1,
F+8 is 3, and F+12 begins an RA v0 blob (byte length + alignment16), followed by bounded zero
padding and actual DXT bytes. Thus these headers do not imply missing pixel data.

The declared mip count is not sufficient to compute stored bytes blindly:

- 213 validated records store the whole declared mip chain;
- 641 store only the base level;
- total 854 decoded records: DXT1 717, DXT5 137;
- 17 other DXT occurrences did not pass the conservative padding/successor gate and remain unresolved.

Acceptance requires stored byte count equal to either exact base-level size or exact full-chain
size, plausible dimensions/depth/count, bounded payload and a recognized RA successor version
(0..4 in this probe). Production needs enclosing resource/ARCH validation rather than scanning
every possible signature. Base-only data is wrapped with one resident mip for the independent
DDS decoder, avoiding reading following archive metadata as extra mip levels.

All 854 decoded at declared base dimensions; selected private previews show a wall/grass strip,
a ventilation grille and a normal-map-like image coherently. This is not proof of UV orientation,
correct shader role or exact in-game material appearance. Nothing has been added to production
image scan in this research session; existing inline-only support remains as previously tested.

### D. Texture identity is NOT a global map

The candidate 32-bit key at F-28 takes 209 distinct values across 854 records. There are 258
distinct `(key,format,width,height)` combinations; 42 combinations contain different base pixel
payloads AND different decoded RGBA images. Therefore neither key alone nor key+dimensions is
a safe dictionary key. This may reflect reference namespaces/variants/serialized fields not yet
fully interpreted; do not call the field a proven content hash.

Merely scoping by the 31 aligned ARCH starts is also insufficient: 107 scoped combinations
still contain conflicting decoded pixels (a different grouping/count from the 42 global keys).
Preserve exact resource object/reference identity and source offset until serializer semantics
are understood. Never let the last matching key overwrite an earlier texture globally.

## Next implementation order

1. Add synthetic and real split-texture fixtures: full-chain, base-only, successor versions,
   malformed length/padding, cancellation and source identity; then native 2D exposure.
2. Implement a typed counted range-array reader; represent minVertex independently from any
   stream byte offset or true base-vertex field. Validate every referenced vertex and index.
3. Resolve multi-stream/object references before extending the Audi; its four ranges per packet
   could include LOD alternatives, so Render All must not automatically load all 400.
4. Preserve per-draw strip restart/parity and original vertex/index byte provenance. Use lamp's
   exact index partition and 16-boundary-face difference as regression fixtures.
5. Finish material/state and image-reference resolution (including UV transforms, role names,
   residency and conflicting resource keys) before automatic image assignment.
6. Assemble showroom instances from levelrender/levelstream; then verify one actual track.

No production reader, UI, app build or release changed in this research pass. Existing sphere
and shared construction-object support is unchanged. No claim of a complete showroom render.
