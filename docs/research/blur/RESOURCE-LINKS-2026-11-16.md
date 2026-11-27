# Blur showroom resource and material-value links — 2026-11-16

Continues NODE-PARAMETER-PROGRESS-2026-11-16.md. Research only: no application source/build,
install, UI launch, commit or push. Same executable SHA-256 as the previous receipts.

## Concrete result

The file-level values and reference scopes can now be distinguished. The first showroom
LightFlare_LN.fx g_tColour value is1, indexing local texture-reference slot1, which contains
(-1,0). The original resolver's negative-group branch uses the scene global texture array.
Its first serialized entry is named lightflare_diff. This is a named resource link, not a
GPU/visual comparison or a claim of complete shader reproduction.

## Four RenderListNode_Common file records consumed

| Source start, decimal | End, exclusive | Array records, stride48/20/36/48 |
|---|---|---|
| 254 | 9632 | 36 /0 /0 /0 |
| 10494 | 19887 | 25 /0 /0 /0 |
| 20734 | 57533 | 95 /0 /0 /0 |
| 59646 | 100413 | 23 /138 /0 /47 |

This includes226 stride48 records and138 stride20 records. The old signature probe found226
single-range packet candidates and missed the other array representation. These counts are
serialized record counts, NOT364 distinct meshes or proof that the full showroom is exported.
The semantics of the138 alternate records still need tracing, including instancing/LOD/state.

262 parameter-value blocks were decoded from their own per-list descriptors (not a hardcoded
first-list schema). Every block's wire extent agrees with8+4*count+declaredValueBytes.
All329 texture-named type7 values fit their local texture-reference table; their referenced
group IDs fit the scene's145 allocated resource groups or the global texture scope. Global
texture indexes fit the declared16 entries. These are reference consistency checks, not329
verified image assignments or a material-coverage percentage.

The per-list dependency records also contain explicit redundant triples(group,index,localSlot).
All815 texture-dependency triples reproduce the pair at the indicated local texture slot.
Do NOT label dependency-record field0 a shader index: its semantics are not yet established.
It is separate from the scene shader-name table and from parameter-layout descriptors.

## Original resource resolver: the missing scope

Runtime instruction sequence0x6ce5e0..0x6ce614:

- RenderList+0xb0 is the pair array; +0xb4 is its count. Pair stride8.
- Pair.first is interpreted as a signed group selector. Observed0xffffffff uses global scope.
- Nonnegative group: scene+0x98 points to records of stride0x84 (132 bytes).
- The resolver checks group+0x6c against-1 (availability), then uses the pointer array at
  group+0x54 and indexes it with Pair.second.
- Global branch uses the pointer array at scene+0xc8, indexed by Pair.second.
- Resulting texture objects supply their device handle at+8 and another signed field at+0x20.
  Their names are used for an override lookup; retain original identity before any override.

0x6ce4e1..0x6ce52e applies the same resolution while iterating the texture dependency triples.
0x6cdd20..0x6cdd52 initializes the globally scoped entries. The last pair member is an index
within its resource group, NOT a mip level, format flag or globally unique image hash.
The split DXT key previously studied cannot replace this two-level identity.

## Scene header now traced to resource tables

Reader body0x6ec040: after mode/base metadata it reads declaration array at scene+0xa4,
shader-name array at+0xb0, then the resource-group count, allocates the+0x98 table, and
reads globals at+0xc8. The root scene's+0xb0 shader-name array is a DIFFERENT object/role
from RenderList+0xb0's texture-reference array: do not merge offsets across types.

The exact showroom header parsing starts at90 (outer version1 frame is at86):

- Resource-group count145, source u32 at2258.
- 23 shader names recovered, including Road_LN_ND_NG_NW.fx, LightFlare_LN.fx,
  default_LN_NB_NL.fx and MeshEffect_LN.fx.
- Global texture wrapper begins2262; mode2 at2270, count16 at2274.
- First global name's length prefix at2278 and bytes at2282: lightflare_diff.

The scene header harness currently stops inside first texture loading at2376, before an
unimplemented runtime lookup/storage path. It has NOT decoded all16 globals, populated all145
resource groups, or traversed the entire root scene. Group allocation count does not mean
their contents have been reconstructed. Source group-to-stream offset tables are the next target.

## Validation and harness limitations

Evidence JSON: SHOWROOM-RESOURCE-LINK-VALIDATION-2026-11-16.json. Private reproducible scripts:
emulate_renderlist.py, validate_renderlists.py, material_bindings.py, link_inspect.py,
validate_bindings.py, emulate_scene.py, scene_tables.py, resource_accesses.py, stride_probe.py.
Per-list raw snapshots and original instruction traces are in the workspace blur-research folder.

RenderList runs use Unicorn with original reading instructions, synthetic bounded storage,
Python lifting of generic parameter decoding, skipped render-state registration, and skipped
runtime-only conversion/TLS cleanup. All skips are recorded. Thus a return means the fixture's
file-reading path completed under this harness; it does not validate runtime/GPU object creation.
The old TLS boundary at1151 is superseded by these four file-decoding results. No original
game entrypoint, operating-system import or GPU was executed by the harness.

No blanket UV flip or ambiguous last-key-wins fallback was introduced. No production tests
were run because no production code changed. Private checks executed all four lists, parameter
byte extents, reference bounds, global bounds and815 dependency equalities without failures.

## Next work — Codex/native analysis

1. Locate source resource-group metadata and recover each group's stream offset/length and
   buffer/texture arrays. Preserve pair(group,index) identities and distinguish unloaded groups.
2. Parse the145 groups' actual contents and all16 global texture entries with bounded readers.
3. Trace each packet/alternate record's index/vertex buffers and parameter-block selectors;
   validate one visible textured draw, then group-level/whole-scene rendering.
4. Validate the138 alternate records before aggregating them: do not assume all are extra meshes.
5. Integrate proven native paths after independent fixture comparisons. Complete maps and
   application material/image coverage remain pending.
