# Blur PC executable serializer analysis — 2026-11-16

Status: research only. No new production reader, application build, or full-track render in this session.

## Source and method

Read-only installation: `F:/copia de seguridad 08.04.2019/downloads/blur/Blur`.
Blur.exe SHA-256: `6832d82378a6e857515d4294788dcfdae913d78e8cad5c5fe74fbbec17824594`.
32-bit PE, image base 0x400000. Addresses below are VAs for this exact executable.
Static disassembly only; neither the game entry point nor its wrapper was executed.
Private reproducible scripts/disassembly live in the project's `blur-research/` directory:
`exe_probe_20261116.py`, `schema_probe.py`, `node_helpers.py`, `cull-reader-20261116.txt`,
`cull-fields-20261116.txt`, `cull-base-20261116.txt`, `schema-probe.txt`, `node-helpers.txt`.

## Existing support and unresolved scope

The current native profile accepts bounded CPModel static resources with one unambiguous
stride-20 vertex pool and one u16 index pool. It does not assemble all ARCH level scenes.
Its uniquely named diffuse-image preview is not proof of the original material/shader binding.
The previous research counted 19 level quartets and 1,544 model files in this installation;
these are inventory counts, not supported-mesh counts.

Use the showroom quartet as the first graph fixture: level.levelrender (978,033 bytes)
and level.levelstream (24,725,504 bytes), with their level/physics siblings retained.
The previous 226 bounded draw/range candidates and 31 aligned paired ARCH signatures
are not 226 verified meshes or 31 proven complete scene partitions.

## New executable evidence

The executable contains registration strings for RenderingData::Node, CullNode, GroupNode,
LodNode, MaterialNode, RenderNode and RenderListNode. The CullNode name is at 0xe9b548.
Multiple adapters reference the same name: registration alone does not identify a reader.

- 0x6f82c0 adapts a call to 0x6e42a0, the framed CullNode reader.
- 0x6e42a0 reads two u16 fields, temporarily installs the serialized version in archive+4,
  calls body 0x6e1030 and restores the enclosing version.
- 0x6f5d50 instead adapts writer 0x6f27d0; it emits magic 0x4152 (`RA`) and version 2.
  Its body 0x6f2750 mirrors the reader's field order.
- Fast reader stream fields: archive+0 points to stream; stream+4 cursor, stream+8 end.
  Each fast read checks remaining bytes; fallback read helper is 0x402f50.

CullNode body 0x6e1030, object-relative destinations (semantic names remain provisional):

| Order | Reader | Destination | Version condition |
|---|---|---|---|
| 1 | 0x6dae90, framed base Node | object | always |
| 2 | 0x6d9cd0, framed helper | object+0x34 | always |
| 3 | 0x6daf20, framed helper | object+0x54 | always |
| 4 | u32 | object+0x74 | >0; otherwise -1 |
| 5 | 0x6daf20 | object+0x64 | >1 |

Base Node wrapper 0x6dae90 calls body 0x6d8fa0. That body visits object+0x10
via 0x6d6f00, object+4 via framed reader 0x6d79c0 (body 0x6d7550), then
object+0x20 and +0x28 via 0x6d5eb0. It temporarily sets versions for inherited fields.
0x6d6f00 reads a u32 length and that many bytes, using storage helper 0x6d64c0;
whether that byte sequence is a name must be established from fixtures, not assumed.
0x6daf20 wraps body 0x6d9ef0; that reads a u32, invokes framed helper 0x6d9000
(body 0x6d8280), and stores the u32 at destination+0xc. Do not yet label this
as a transform or bounding box solely from its memory size.

The showroom stream contains CullNode type text at offsets 69, 10309, 20549 and 59461;
the first occurrence ends at 92, followed by an RA version-2 frame and nested RA version-0 frame.
This agrees with the traced framing, but a complete record cursor endpoint and object-reference
resolution have NOT been validated. String/signature matches alone are not deserialization.

## Texture identity constraint (previous validated research, still applicable)

Split DXT records are present: 854 validated records (717 DXT1, 137 DXT5),
including 213 complete mip chains and 641 base-only records. Seventeen further candidates
were conservatively rejected. Pixel decoding was independently checked in earlier research.
The key 28 bytes before FOURCC is not a safe global image identity: prior analysis found
42 conflicting key/format/dimensions combinations; even coarse ARCH scopes retain collisions.
Do not implement last-key-wins material binding. Recover original object references/scopes.

## Ordered next work — Codex/native analysis

- [x] Locate original typed CullNode reader and distinguish it from writers/adapters.
- [x] Trace version-dependent body and inherited Node field order; retain exact addresses/hash.
- [ ] Finish bounded Node/helper schemas and parse one full CullNode to an exact cursor endpoint.
- [ ] Decode typed object-reference dictionary and archive-local lifetime/reset rules.
- [ ] Trace RenderListNode_Common / RenderNode to original vertex/index resource identities,
      stream byte offsets, draw ranges, transforms, and LOD/state selection.
- [ ] Trace MaterialNode to texture object identity, UV channel, sampler and blend/pass state.
- [ ] Validate one complete showroom graph against known simple CPModel draws, then a race level.
- [ ] Only then integrate a native scene adapter with cancellation/budgets and missing-reference
      diagnostics; test render-all without duplicate LODs or alternate/bent object states.

No native fixture tests or application regression run were warranted for this documentation-only
research update. Validation was static reader/writer comparison and source framing inspection.
Complete map extraction, Audi multibuffer support and verified per-draw texture binding remain pending.
