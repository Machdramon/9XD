# Blur native directory/named texture integration — 2026-11-16

## Implemented

- src/blur_scene.h: bounded/cancelable original stream directory and strict texture-only resource
  subrecord reader. Ordered(group,variant,index) identities, name inheritance, unique image choices;
  conflicting base names are excluded rather than last-copy-wins.
- src/asset_discovery.h: opening a complete filesystem level.levelstream automatically checks
  its same-folder level.levelrender. Sibling size/mtime checks, no symlink or memory-document
  path fallback. Candidate recipe offsets always refer to the current stream document.
- Named image candidates use existing BC1/BC2/BC3 production image decoding/export. The largest
  decoded variant is explicitly an image inspection choice, not the game's quality/LOD decision.
- Stream spans are exposed as structure regions, with summary/limitations in scan notes and
  blurScene metadata. A recognized paired scene does not fall through to generic geometry guessing.
- Missing/unsupported context retains old anonymous split-image behavior. CPModel geometry and
  its existing material preview paths remain covered by regression tests.

## Padding correction found during integration

The first native corpus test exposed a discrepancy: absolute minimum alignment is insufficient.
Some payloads have an extra16 bytes of zero fill. Use a unique16-aligned payload within the bounded
0..31 zero-padding window, followed by a framed successor or terminal zero block padding (<2048).
Reject ambiguous candidates. The first research fallback also mislocated two image payloads by16
bytes; the native test investigation corrected these. Pillow success alone did not establish the
right pixel start. The old20 complete/5 partial/582 image/663 inherited counts are superseded.

With corrected boundaries, both the research parser and native reader report:

| Check | Result |
|---|---|
| Directory |31 contiguous blocks, complete stream coverage|
| Complete texture-only blocks |25|
| Partial texture prefixes |0|
| Bounded nonempty image records |712|
| Inherited/no-update records |868|
| Named image identities offered |89|
| Research material texture slots resolved to names |96/329; not mesh coverage|

The six remaining blocks consist of the four scene-node blocks plus two mixed resource blocks;
they are not supported by this texture-only path. They are not counted as empty/corrupt resources.
SHOWROOM-TEXTURE-VARIANT-VALIDATION-2026-11-16.json refreshed with corrected offsets/counts.

## Validation

tests/window_tests.cpp adds blurSceneDirectoryContract and blurSceneNamedTextureCorpus.
The contract rejects truncation, wrong range/count and cancellation. The real showroom test checks
all counts above, unique identities, all89 image recipes through production decodeBinaryImage,
automatic sibling loading through discoverAssets, no fabricated mesh candidates, corrupted archive
rejection and no unrelated filesystem lookup for memory documents.

Focused result:12 passed/0 failed/0 skipped (10 methods plus setup/cleanup),5413ms.
Includes existing split/inline images, six pallet/lamp model cases, static model/corpus,
material previews and two UI scan/render tests under offscreen Qt.
Log: build/windows-local/blur-scene-native-tests.txt. Initial9/1 run identified the padding bug;
the corrected complete run above supersedes it. Full window suite was not rerun for this quick check.

Development EXE rebuilt and linked successfully: build/windows-local/binary_workbench.exe.
Build log: build/windows-local/blur-scene-app-build.txt. Embedded native feature marker verified;
SHA-256 receipt: build/windows-local/blur-scene-native-receipt.json. Executable left closed.
No interactive app launch, push, commit or release. Existing uncommitted RayCity/F&F/Auto Modellista
work preserved. Only main.cpp's mtime was touched to force its header-dependent recompilation,
because the initial incremental target did not rebuild that object; no main.cpp text change.

## Try it

Open levels/showroom/level.levelstream with level.levelrender beside it and run the texture scan.
Expect89 named candidates carrying their group/index and selected inspection variant. Preview/export
uses existing image tools. Scene geometry/material binding is still pending; Scan for3D Data does
not yet reconstruct this map. No whole-map render is claimed by this integration.

Next Codex: mixed vertex/index readers, remaining source resources and true draw/parameter/texture
association; verify an actual textured mesh with placement/UVs before whole-map/native scene rendering.
