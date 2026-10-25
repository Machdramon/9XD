# NFS Underground 2 PC scenery

`Scan for 3D Data` recognizes complete, bounded NFSU2 chunk streams containing scenery sections. The native reader builds a solid catalog, resolves scenery definitions by mesh hash, and applies each instance's packed rotation and translation. The ordinary profile decoder validates each solid before scene assembly. No AI service is required.

## UI

Open `STREAML4RH.BUN`, then run **Scan for 3D Data**. The first result assembles the scene without the sky dome for useful framing. Additional results load individual sectors, including the sky sector. These results describe overlapping views. Loading now omits instances already visible from a previous supported preview, including the automatic first result. Selecting all therefore does not add the combined scene and its sectors twice. Use Mesh Collection to remove or hide previews when comparing alternatives.

Scene previews are currently flattened into one mesh per selected result. Instance records are retained in discovery metadata; interactive shared-instance scene entries are a future extension. Individual sectors remain selectable scan results. Load Settings is disabled because a scene cannot be represented by a single raw buffer setup. Preview and export use the existing scene and model export tools.

## Supported geometry

- Little-endian chunk trees, with padding and absolute 16/128-byte alignment.
- NFSU2 solid version 0x16; 24-byte position/color/UV and 36-byte position/normal/color/UV layouts.
- Explicit UInt16 triangle lists, validated counts and material index ranges.
- Scenery definitions (68 bytes) and instances (64 bytes).
- Rotation: nine Int16 components divided by 8192; row-vector position transform `p * R + t`.
- No additional solid pivot is baked into scenery instances. The existing car profile is unchanged.
- Conversion from source Z-up to viewer Y-up uses the existing NFSU2 profile orientation.
- Source offsets are preserved for hex navigation, including repeated instances.

## Limits and unresolved features

Full stream inspection uses one quarter of the asset memory budget in Settings > Performance (8 GiB total by default). Chunk traversal remains bounded to 200,000 nonzero chunks and depth 12; the solid catalog to 20,000 keys. Scene part sizes and instance metadata limits follow the configured budget. Large streams retain sector alternatives, while a full-scene result is offered when its estimate fits. Invalid buffers are rejected rather than silently truncated. See asset-memory-budget.md for the scope of this estimate.

Conflicting same-hash vertex/index/material payloads are treated as ambiguous, and those instances are skipped with a diagnostic. Missing/unsupported references are also reported. This does not yet resolve external resource dependencies, all LOD alternatives, collision models, compressed solids, or all NFS family variants.

UVs are preserved. Vertex colors, texture bindings and game shaders are not yet rendered. Normals are calculated for the 24-byte layout; the original engine's smoothing rules are not claimed. Textured/game-identical appearance remains future work.

## Validation

`WORKBENCH_NFSU2_SCENE=.../STREAML4RH.BUN` enables the optional `localNfsu2Scenery` test. It exercises production discovery, scene decoding, coordinate bounds, corruption handling, cancellation, UI results, and the actual asynchronous scan-to-preview wiring. The scene without sky contains 638 instances, 48,372 vertices and 31,060 triangle faces. Five sector results are available. `nfsu2SceneSafety` runs without proprietary data.

Reference structures were cross-checked against local binary lengths, indices, hashes and stored instance bounds:

- https://github.com/NFSTools/NFS-ModTools/blob/master/Common/Geometry/Underground2SolidReader.cs
- https://github.com/NFSTools/NFS-ModTools/blob/master/Common/Scenery/Underground2Scenery.cs
- https://github.com/NFSTools/NFS-ModTools/blob/master/Common/Scenery/Structures/PackedRotationMatrix.cs

No game assets are included in the source repository.

Large-map regression: `WORKBENCH_NFSU2_LARGE_SCENE=.../STREAML4RA.BUN` enables `localNfsu2LargeScenery`, checking more than 10,000 discovered instances, bounded sector parts and decoding of the largest returned part. The local fixture yields 48,224 resolved instances across 606 sectors.
