# Project status — 9XD 0.3.0

Updated 20 October 2026. Windows x64 is the distributed platform; Linux and clean-machine Windows testing remain pending.

## Implemented

- Virtualized hexadecimal editor, byte interpretation, growing RAM documents, undo/redo, background saving, pattern search, comparison, checksums, entropy and visual formatting rules.
- Module-scoped docking and detachable Hex Viewer, 3D, UV/2D and Scripting workspaces; persistent user layout and themes.
- Shared bounded binary reader for manual decoding, native asset scanning and local AI tools, including pending byte edits.
- Native file/geometry/image scan inspectors, recognized structure annotations and real decoder preview completion.
- Validated Genki and ARCP packet reconstruction; bounded GLB and image/container discovery. TIM2 surface discovery walks declared pictures up to the configured candidate cap.
- Vulkan mesh/UV rendering, six orthographic directions, split/detached viewports, gizmos, transforms, binary-sourced placement, labels, normal display, rendering styles, grids, bounds, visibility and isolation.
- Materials and texture assignment; FBX, GLB, OBJ and STL export with format-appropriate attributes.
- Python/C++ scripting and a library manager separating included and user modules; programmable integer calculator.
- Optional local Ollama assistant with controlled reads, scan evidence, decoder recipes, native imports and formatting annotations. Ollama/models are separate user installations.
- Portable Windows packaging with Qt, Python standard library, Vulkan loader, Visual C++ and enabled FBX runtime dependencies.

## Known limits and next work

- Native packet import is profile-specific. Complete game hierarchy, part/LOD/visibility selection and source material-to-texture bindings are unresolved. All recovered local variants may overlap.
- Scanning examines at most 32 MiB and does not prove the absence of assets. Unsupported raw layouts require explicit recipes. Archive decompression is not implied by recognizing a signature.
- Disk-backed files use overwrite editing; growing/inserting/deleting is supported for RAM documents. Text/regex search is limited to 1 MiB per region.
- Meshes/images are session snapshots; a complete editable scene/project save format remains future work. UV coordinates use a shared per-vertex attribute model.
- Scripting executes local user code. C++ requires a compatible compiler; bundled Python does not include third-party packages.
- AI quality depends on the installed model. Native evidence and decoder validation remain authoritative; an inspection is not a completed render.
- The Windows preview is unsigned. Vulkan rendering requires a compatible graphics driver. A separate clean-machine deployment review and Linux validation are pending.

See [release notes](releases/v0.3.0.md), [native scanning](native-scanning.md), [AI assistance](ai-assistance.md), and [3D workspace](3d-workspace.md).
