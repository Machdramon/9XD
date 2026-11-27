# Blur PC native scene geometry

Date: 2026-11-17. Owner: Codex. Native focused validation passed; development executable updated.

## Recovered structure

Original executable SHA2566832d82378a6e857515d4294788dcfdae913d78e8cad5c5fe74fbbec17824594. Resource reader0x6ea2a0 calls vertex/index/node arrays. Vertex body0x6e6ec0 reads a framed declaration, stride, attribute tuples and offsets, vertex count and aligned blob. Index body0x6dc160 reads count, type0u16/1u32 and aligned blob. Base variant resource is accepted only after empty node and texture arrays are consumed; subsequent unsupported records stop that block prefix.

RenderList body0x6ea950, modern branch0x6eab91: inherited CullNode, descriptor/parameter layouts, four packet arrays, second parameter values and post tables. Native parser follows this sequence through index/vertex pair tables (+0xa0/+0xa8). File primitive enum1 maps to original triangle-strip mode5. Binding records serialize four32-bit fields plus a framed parameter reference; stream0 identifies position buffer. Original in-memory binding packs its first two members into16-bit halves; treating its memory stride as12 instead of16 was incorrect and is not used natively.

Each range(first,count,min,vertexSpan) is validated against the exact index buffer. Triangles reset parity at each range and omit degenerate indices. Position declaration must be Float3(type6, semantic0,index0), offset within stride. Optional UV0 is type8 half2, semantic5,index0; source coordinates retained. Instance parameter must provide exactly an identity g_mWorld. Unsupported transformations are excluded rather than guessed.

## Showroom evidence

Independent sequential Python reader matches original isolated-reader reference tables for all four RenderLists. Six complete buffer groups(1,2,3,4,7,11). First packet arrays179 draws;173 Float3 sections yield36,750 faces. Six Float4/billboard sections excluded. No material textures assigned by proximity or name. Largest texture variant remains image-inspection only.

Production source: src/blur_scene_geometry.inc included from blur_scene.h. Discovery appends candidates in mesh mode; mesh_profile_decode.h dispatches native-blur-scene. Decoder reads complete current stream and sibling levelrender with existing file revision checks, reparses candidate identity and compacts vertices. Requires filesystem source and same-folder sibling; RAM-only context is explicitly unsupported. Input cap128MiB additionally constrained by memory budget.

## Pending and validation

Build and focused tests pending at initial documentation write; final receipt appended after execution. Additional47 fourth-array packets,138 alternate packets, global/mixed texture resources, nonidentity matrices and race-map coverage remain pending. Partial static scene support must not be described as a complete map or original game renderer.

Private reproducible research scripts: mixed_buffers.py, parse_scene_draws.py, validate_scene_geometry.py in Codex workspace blur-research. Snapshots and source game binaries stay outside repository. No interactive application launch authorized/needed for this integration.

## Final validation / development build

- Focused Qt run:15 passed,0 failed,0 skipped,16,631ms (13 test methods plus setup/cleanup). Log: build/windows-local/blur-scene-geometry-tests.txt.
- Real showroom discovery173 candidates; exact36,750 faces. Five representative sections decoded through MeshProfiles; finite positions, generated normals, unassigned textures. Corrupt used indices, cancellation, nonexistent candidate identity and RAM context rejection covered.
- Offscreen UI test opens showroom, scans, presses Render All and confirms all173 meshes /36,750 faces in MeshScene; repeats Render All and confirms no duplicates. This validates the application pipeline; it is not a Vulkan screenshot or original shader comparison.
- Existing static/split image/multi-draw/UI regressions pass. Full repository suite not rerun. First test pass exposed a test-target mistake (corruption outside admitted ranges); corrected to mutate the first admitted draw's actual first index, then full focused set rerun.
- Development binary rebuilt after main.cpp timestamp-only touch to work around stale header dependency tracking. Embedded native-blur-scene marker and SHA receipt verified: build/windows-local/blur-scene-geometry-receipt.json. Application left closed; no commit/push/release.
- Usage: open levels/showroom/level.levelstream with same-folder level.levelrender available, choose Scan for 3D Data, then Render All. Filesystem source only;128MiB stream cap plus memory budget. Whole race-map support and scene materials remain pending.
