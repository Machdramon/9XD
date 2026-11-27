# Blur bounded scene-node and parameter schema progress — 2026-11-16

Research only; no production changes, application build, install or launch.
Continues EXECUTABLE-SERIALIZERS-2026-11-16.md using the same executable SHA-256.

## Bounded Node/CullNode schema

Independent Python reader now consumes the complete named CullNode bodies, including the
base Node, pair arrays, eight raw float words and two version-dependent byte-payload wrappers.
All reads are source-bounded; unsupported versions and oversized arrays/blobs are rejected.
The earlier hypothesis that destination+0x54/+0x64 could be vector/bounds fields was NOT
confirmed: original body 0x6d9ef0 reads a u32 plus a framed counted BYTE blob via 0x6d8280.
Do not use these byte payloads as transforms. The eight floats at +0x34 remain raw fields;
their geometric interpretation is not established by this schema test alone.

Four showroom named bodies are 121 bytes each:

| Start (decimal) | End, exclusive | First pair | Next typed record text starts |
|---|---|---|---|
| 92 | 213 | 6,1 | 218 |
| 10332 | 10453 | 8,1 | 10458 |
| 20572 | 20693 | 10,1 | 20698 |
| 59484 | 59605 | 12,1 | 59610 |

After each body: byte 0x03, u32 string length 36, then RenderingData::RenderListNode_Common.
This describes the observed bytes, not a complete interpretation of the object-reference token.
Payload54 has tag40 and five bytes ffffffff0f; payload64 has tag0 and zero bytes in these fixtures.

An offline Unicorn run of original reader 0x6e42a0 returned at exactly the same four endpoints.
Only storage/allocation and raw-read services were supplied by the harness; the original Node
and CullNode parsing instructions ran in emulated memory. This is not native game execution,
GPU validation, or proof of every object-reference meaning. No original entrypoint was invoked.

## Broader validation

38 levelrender/levelstream files across 19 level directories were inspected read-only using mmap.
3,082 type-name-following CullNode bodies passed the bounded schema; zero structural rejects.
Per-file coverage is in CULL-NODE-COVERAGE-2026-11-16.csv.
These are named bodies located via type-name declarations, NOT all scene nodes, mesh counts,
complete archive traversal, texture coverage or proof that every object is supported.
Private full evidence: blur-research/node-validation.json; validate_nodes.py;
parse_cull.py; emulate_node.py; validation-log.txt. Their paths are relative to the Codex workspace.

## RenderListNode_Common and material parameter layout

String VA 0xe9b0cc; reader adapter 0x6ec000 -> framed reader 0x6ebd90 ->
another framed reader 0x6eb4d0 -> body 0x6ea950. Body first reads inherited CullNode.
It then branches on the enclosing version. The first showroom instance takes the >=2 branch.
At source offset375 its parameter-layout block is read by 0x6e8e60.

Recovered bounded descriptor array ends at1143. Descriptor reader chain:
0x6e7e30 -> 0x6e7220 (counted array, in-memory stride56) -> 0x6e65f0 -> 0x6e4660.
Field array chain: 0x6e13d0 -> 0x6dc030 (in-memory stride24) -> 0x6da1d0 -> 0x6d8860.
Each field records a counted name, u32 type and u32 element count. The descriptor also includes
u16 offsets, additional metadata and sizes. Raw metadata is preserved rather than overnamed.
Full four descriptors: SHOWROOM-PARAMETER-SCHEMAS-2026-11-16.json.

Examples:

| Layout | Parameter | Type/count | Offset in value block |
|---|---|---|---|
| LightFlare_LN.fx | g_tColour | 7 / 1 | 0 |
| LightFlare_LN.fx | g_vLightFlareColour | 6 / 4 | 4 |
| LightFlare_LN.fx | g_fLightFlareIntensity | 6 / 4 | 20 |
| MeshEffect_LN.fx | g_tTexture | 7 / 1 | 0 |
| MeshEffect_LN.fx | g_vColour | 6 / 4 | 4 |
| MeshEffect_LN.fx | g_vColourFresnel | 6 / 4 | 20 |

LightFlare has a52-byte layout; MeshEffect116 bytes. A shared layout includes g_mWorld,
g_vUVOffset, g_lodFade, g_tEnvironment and g_SHNode. These are recovered descriptions,
NOT demonstrated bindings for a specific draw, and not yet a rendered original shader.

Generic parameter reader 0x6dbcb0 reads a count and size, then descriptor indexes. The index
selects an entry in the 56-byte descriptor array; its low11 bits are copied into an internal handle.
0x6d96e0 decodes values using each descriptor's field type/count, rather than treating the block
as arbitrary floats. Switch at0x6d988c: types0/1 read1 byte,2/3 read2 bytes,
4/5/6/7/10 read4 bytes,8/9 use framed helper0x6d61c0. Thus the observed texture-named
type7 parameters are serialized32-bit values; their original resource lookup still needs tracing.
Do NOT confuse them with type8/9, whose helper0x6d5950 combines two wire u32 values as
(first & 0x0fffffff) | (second <<28). This is a different representation.

The isolated RenderList read reached offset1151, after the descriptor array and two zero u32s,
then stopped at an unmapped TlsGetValue import (IAT0xe530a4). No TLS/resource-pool/GPU behavior
was fabricated to call that a successful complete RenderList decode. This boundary is pending.

## Next action / owner: Codex native analysis

1. Lift remaining RenderList >=2 fields and generic parameter values into a bounded reader,
   separating file decoding from runtime thread-local allocation/registration.
2. Resolve type7 resource values through the original lookup and archive-local object tables.
3. Associate vertex/index buffers, draw ranges, transforms, LOD/state and parameter block identity.
4. Verify one textured showroom draw end-to-end, then aggregate showroom and one race track.
5. Integrate only proven paths into native Scan/Render All with diagnostics and budgets.

No full map, mesh material coverage percentage, verified texture assignment or new native
application support is claimed by these tests. Existing static CPModel support is unchanged.
