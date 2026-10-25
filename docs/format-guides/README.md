# MSR / PGR format guides

These six JSON documents preserve measured fields, native decoder IDs, dependencies, sample headers and SHA-256 identities. They can guide further native implementation and AI analysis. They are **research/configuration documents**, not generic `9XD Mesh Profile` files; do not import them into the existing generic profile editor.

| Family | Usable now | Pending |
|---|---|---|
| MSR alpha | Scan, bulk preview and GLB export of Alfa 156 primary LOD and Asakusa, Shibuya and Shinjuku track nodes; independent strips, centers, per-corner UVs. | Embedded/sibling Dreamcast textures linked; further vehicle LODs, secondary masks and original shaders remain pending. |
| MSR retail / late Pak | Native bounded Bizarre resource extraction. | Retail geometry/material declarations and non-Pak wrappers. |
| PGR1 | Native GOTHPOK LZ extraction; 15 local resources pass native extraction and hash validation. | NRM/NRM_V/NRM_RES geometry/material assembly; track resource links. |
| PGR2 | Five indexed archives / 65 named sections; 240Z MESH vehicle first-LOD groups with compact coordinates and UVs. | WMSH track/CTH geometry, raw sequential CTH container, sibling PAK_STREAM references, state selection and original paint/glass shaders; supported MAT/TEXT BC3 diffuse bindings are integrated. |
| PGR3 | Bounded ENDI/ENSW resources and supported strict SCNE GPU streams. | Declarations, stream pairings, topology, transforms, materials. |
| PGR4 | Bounded ENDI/ENSW resources and supported strict SCNE GPU streams, separately surveyed. | BPR/STREAM2 and version-specific scene/model assembly. |

## Measured outputs

- `msr alpha/156.NRM`: 155 material groups, 6,506 expanded corners and 4,432 triangles. The main vehicle LOD is imported; later LODs are not combined. Independently rendered for geometry review.
- MSR alpha tracks, day and night: Asakusa 37,635 triangles; Shibuya 91,252; Shinjuku 44,730. Named child nodes (+64), repeated strip corners and 12-byte trailers are supported. All six decode/export checks and Asakusa bulk UI preview passed on 2026-10-23. The old 31,944-triangle partial Asakusa result is superseded.
- Some MSR alpha source UV fields contain NaNs. Affected groups retain geometry and explicitly omit UVs; valid groups retain their source UVs. Texture references are kept without claiming an assigned texture.
- `pgr2/240Z.pak_hrd`: 79 first-LOD subdraws, 12,038 triangles. Shared GPU vertices are compacted per exported draw. Position scale is verified against INFO/geometry bounds; original packed normals are not decoded and normals are calculated from faces. State/component alternatives remain separate and may overlap.
- PGR2 compressed sections use bounded sync-flush framing without a conventional final Adler trailer. Only the dedicated adapter accepts it; generic zlib handling remains strict. The declared frame capacity is not treated as an exact output size.

## References

The PGR2 documentation at https://github.com/x11x00x00x/pgr2_pak_file_format was consulted as a research reference. Its table/stream and scale descriptions were checked against the actual local files; conflicting assumptions were not accepted as validation. No external importer implementation or game data is included in these guides. Older scripts from the user's research archive were only read as references.

PGR1 track integration (2026-10-23): original P/T/V Asakusa day/night scanning, 470 linked material candidates, shared texture cache, bulk UI preview and GLB/FBX export pass native tests. Per-variant supported export is 120,695 triangles; two invalid-UV triangles omitted and two unsupported texture materials marked magenta. Original vertex lighting and exact alpha/shader behavior remain pending.

Material integration update: MSR alpha primary-LOD cars use embedded Dreamcast textures; tracks use matching DYT/NTT companions. PGR2 MESH vehicles resolve MAT/TEXT references and apply conventional BC3 diffuse textures. Paint/environment flags retain neutral surfaces and source references until their shaders are reconstructed. MSR commercial/late Pak resource extraction now supports big-endian LZ controls/tokens, independently of the alpha mesh layout.
