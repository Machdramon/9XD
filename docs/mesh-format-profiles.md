# Reusable 3D format profiles

Profiles are JSON configuration files executed by **Scan for 3D Data**, not saved absolute-offset mesh setups. A profile describes how to find object records and derive buffers in any file matching that format. New compatible layouts can be introduced without changing C++.

## Use a profile

1. Open the binary, then click **Scan for 3D Data**.
2. Open **Profiles...** beside the scanner. Choose **Automatic** or a particular installed profile under **Profile used for scanning**.
3. The included **NFS Underground 2 — PC solids** profile is read-only. Use **Copy to My Profiles** to edit a user copy, or **Import JSON** to load a configuration supplied by an AI or another user.
4. **Validate / Test File** checks the draft against the current document, including pending edits, without rendering or installing it. Schema validation alone does not prove file compatibility.
5. **Save Profile** installs the user configuration and selects it for future scans. **Export JSON** shares the helper file. Rescan after changing or selecting a profile.
6. Matching objects appear by name in the scanner and use the normal 3D collection/export path. The first supported result previews automatically when enabled. **Load Settings** exposes the raw local-space recipe; **Preview** additionally applies the object's matrix.
7. Filter names, for example `KIT00 _A`, then use Ctrl/Shift to select parts. **Preview Selected** loads up to 64 selected results in sequence. Do not select every kit and LOD expecting a resolved car: alternatives can overlap. Each selected object remains a separate mesh for visibility and export.

## AI workflow

- **Copy for AI** copies the JSON and instructions for refining it from binary evidence. Paste the returned JSON into a user draft or import its file, then validate/test before saving.
- The local assistant exposes `get_mesh_profiles`, `validate_mesh_profile` and `save_mesh_profile` for explicit requests mentioning profiles. Editing requires **Edit settings / profiles** permission. Included profiles cannot be overwritten; custom identifiers start with `custom-`.
- Saving via AI installs/selects the user profile and invalidates cached scan results. A subsequent `inspect_asset` or `decode_asset` runs the same native discovery engine. A valid schema is not a successful model import; only decoder completion receipts confirm rendering.

## Version 1 execution model

`format: "9XD Mesh Profile"`, `version: 1`, `parser: "chunked-indexed-v1"`.

- Little-endian chunk records: UInt32 ID followed by UInt32 payload length. The high bit identifies a child container; zero-ID chunks are padding. Chunk IDs and alignments come from the profile.
- `rootChunk` recognizes the container; `objectChunk` locates models. `header` specifies a version byte, bounded object name and optional Float32 affine 4x4 matrix. `descriptor` provides UInt32 vertex/triangle count fields.
- `vertices` declares a full record stride and Float32 XYZ/normal/UV offsets; optional normals/UVs use -1. `indices` selects UInt16 or UInt32 triangle lists.
- Optional material-table rules describe fixed records with index start/count, texture slot and shader fields. Groups are validated and retained as scan metadata; original texture bindings and shader behavior are not reconstructed.
- Optional `rotationDegrees: [x, y, z]` specifies output orientation after the object matrix; the NFSU2 example uses -90 degrees around X to display source Z-up data in the Y-up viewer. Object matrices are applied in profile previews; normals use inverse transpose and reflections reverse winding. Singular/nonfinite transforms are rejected.
- No scripts, dynamic code, shell commands, network requests or arbitrary expressions are executed by profiles. Schema fields and numeric limits are validated before reading.

Limits: 64 KiB per profile; 32 user profiles; a 32 MiB document scan; depth 12; 200,000 chunk records; 2,048 discovered objects total; 512 material records per object; 100,000 vertices / 200,000 triangles per decoded object; bounded names, alignments and strides. Profile tests and scans can be canceled; document changes invalidate results.

This initial parser does not support decompression, skeletal animation, general pointer graphs, packed attributes, strips or all console layouts. Those need additional bounded parser/decoder capabilities. Existing Genki and ARCP native importers continue alongside configurable profiles.

## NFSU2 PC example

The included JSON lives at `profiles/nfsu2-pc.json`. It recognizes solid-header version 0x16, finds platform vertex/index chunks, validates 36-byte vertex records and UInt16 triangle indices, reads named objects/material groups and applies object transforms. Tested on the user's GEOMETRY.BIN: 609 objects, 135,031 vertices and 99,885 triangles across kit/LOD alternatives. These are measured totals, not a guarantee that all NFS variants share this layout.
