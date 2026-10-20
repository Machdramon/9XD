# 9XD

<p align="center"><img src="assets/logo-sticker.png" alt="9XD logo" width="520"></p>

9XD is a desktop workspace for inspecting and editing binary files, interpreting texture data, and working with 3D geometry and scripts. It is built with C++23, Qt 6 and Vulkan. The interface is in English.

The project is under active development. Some tools and format variants are still incomplete; console texture support is specific to the formats and memory layouts listed below.

## Download and run

Get the latest published **Windows x64 Preview** portable ZIP from [Releases](https://github.com/Machdramon/9XD/releases). See the [0.3.0 release notes](docs/releases/v0.3.0.md) for the upcoming update.

1. Extract the **entire ZIP** to a writable folder.
2. Open `9XD.exe` inside the extracted folder.
3. Keep its DLLs and the `platforms`, `imageformats`, `python` and other bundled folders beside it.

Windows 10/11 x64 is required. The portable package includes Qt, the Python runtime and standard library, the Vulkan loader and Visual C++ runtime files. Vulkan 3D requires a compatible graphics driver. The application is currently unsigned. No separate Qt, Python or Vulkan SDK installation is needed to run the Windows package.

## Workspace

Each major module can be dragged out into a separate window for multiple monitors. Tool panels dock within their own module. Open binary documents use individual tabs, and workspace layout and settings persist between sessions. New users start with the bundled standard panel arrangement; upgrades preserve personal layouts. Use Windows > Reset Panel Layout to restore the standard arrangement.

| Module | Current tools |
| --- | --- |
| Hex Viewer | Main viewer, data inspector, pattern search and results, formatting rules, statistics, comparison and checksum inspectors |
| 3D | Textured geometry view, mesh collection, coordinate transforms and binary-sourced XYZ placement |
| UV / 2D | Image view, texture/image collection and image calculator |
| Scripting | Script editor, console and library management |
| Other Tools | Programmer calculator, thread activity and settings |

Settings provides light/dark/system appearance, a [custom theme editor](docs/themes.md) with preview, grouped color overrides and theme import/export, plus editor preferences. Additional interface languages remain planned.

## Binary inspection and editing

- Open multiple files or create temporary RAM documents that grow as content is added.
- Navigate large files using 64-bit offsets, configurable bytes per row, selection and go-to-offset controls.
- Inspect text and numeric values with little-endian or big-endian interpretation, including ASCII, UTF-8, UTF-16 and CP932 text.
- Edit bytes, paste hexadecimal data, undo/redo and save through background operations. File-backed editing primarily overwrites bytes; general insertion/deletion is not implemented.
- Search patterns with case, overlap and alignment filters, inspect signatures and entropy, and apply named formatting rules to byte ranges. Navigate and copy search results. Text/regex is limited to 1 MiB per region; hex search streams the document.
- Calculate Checksum-8/16/32, CRC-32, MD5 and SHA-1/256/512 over a document, selection or range. Optionally compare with an expected result. Unsaved changes are included.
- Compare the current document (including RAM and unsaved changes) against a reference file, navigate differing ranges and copy results. Comparison is positional; inserted/deleted regions are not realigned. See [search and comparison](docs/search-compare.md).
- Zoom the hexadecimal viewer with Ctrl + mouse wheel.

The viewer reads data on demand through a shared bounded chunk cache rather than loading an entire file into memory. Large-file tests include sparse files; these tests are not disk-performance benchmarks.

## Textures and image interpretation

The image calculator uses the same binary document reader as the hexadecimal viewer, including RAM documents and unsaved edits. Open images or folders, interpret binary ranges with a recipe, generate image collections, and export PNG images or snapshot libraries (`.bwil`).

Supported decoding includes:

- **Standard images:** PNG, JPEG, BMP, TGA, TIFF and WebP.
- **Raw pixels and palettes:** RGB/BGR, RGBA/BGRA/ARGB/ABGR, grayscale, intensity/alpha, packed 16-bit formats and indexed pixels, with byte order, stride, palette and flip controls.
- **Block compression:** BC1–BC7, including supported signed BC4/BC5/BC6H variants. BC6H is tone-mapped for preview.
- **Console data:** PS1 BGR555; PS2 RGBA/indexed formats and CLUT options; NES CHR and SNES planar/Mode 7 data; GameCube/Wii intensity, palette, RGB and CMPR formats.
- **Memory layouts:** linear, square/rectangular Morton, 8×8 tiles, PS2 GS pages/16S, Xbox 360 tiled and PS4 1D thin layouts, subject to format and dimension restrictions.
- **Containers:** DDS, TIM, TIM2, TPL, GTF and a restricted GNF reader. Mipmap/surface selection depends on the container. GNF currently supports specific single-2D-texture layouts rather than all PS4 textures.

This is not a universal game-archive extractor or automatic decoder for every texture from those consoles. Raw data requires the correct dimensions, offsets, pixel format, palette and layout. Unsupported combinations produce an error. Images are limited to 8192 pixels per dimension, four million pixels total and 32 MiB of source image data per calculation.

## 3D and scripting

The 3D workspace provides a Vulkan geometry view, per-mesh image texture assignment and a textured UV view. Image recipes can be saved and loaded as .bwir, with console presets and additional UNORM/FLOAT raw and DDS formats. See [recipes and mesh textures](docs/image-recipes-and-textures.md) for supported paths and remaining console limitations. FBX and GLB exports support scene materials and textures when assigned.

Geometry settings support Int8/16/32, UInt8/16/32, Float16/32, SNorm/UNorm 8/16 and Fixed16.16 positions, plus XYZ and homogeneous XYZW. Transform axes, invert, scale and rotate each decoded part, then place it with an XYZ delta entered manually or read from a binary offset. Adjust placement later in Mesh Collection. Normals and face winding follow coordinate transformations; model export includes placement. Settings sections can be collapsed individually.

The [Scripting Library Manager](docs/scripting-libraries.md) lists the active runtime libraries and supports creating, editing, pasting, importing and enabling/disabling user Python modules/packages and C++ headers shared across documents. **Default Modules** and **My Modules** clearly separate included and user code; **Add Module...** unifies creation/paste and imports, with Documentation and Code views.

Python scripting uses the bundled interpreter and standard library in the Windows package. Additional third-party Python packages are not included. C++ scripting requires a compatible local compiler. Run scripts only when you trust their contents: scripts execute local code.

## New in 0.3.0

- Native **Scan File**, **Scan for 3D Data**, and **Scan for Textures / Images** inspectors share the binary document reader and include unsaved changes. Recognized regions can be marked in Structure/Formatting.
- Validated Genki PS2 and ARCP packet profiles reconstruct triangle strips directly. Tested originals recover 26,532 vertices / 21,942 triangles and 3,256 vertices / 2,222 triangles respectively. This is profile-specific support, not a universal console-model importer; hierarchy, part variants and material bindings remain unresolved.
- Bounded TIM2 discovery now lists all supported pictures up to 128 candidates; the Genki test fixture provides 40 decoded surfaces.
- Local **Ollama** assistant with user-installed models, permission-controlled document inspection, structure annotations and native preview tools. Ollama and models are installed separately; no cloud API key is bundled. See [AI assistance](docs/ai-assistance.md).
- Expanded 3D workspace: transform gizmos, split and detached viewports, six orthographic directions, reference grids, framing/visibility/isolation, bounding dimensions, rendering styles, face/vertex normal display, materials and FBX export. See [3D workspace](docs/3d-workspace.md).
- Updated 9XD identity and transparent application icon. Existing user settings and layouts are preserved.

See [0.3.0 release notes](docs/releases/v0.3.0.md) and [native scanning](docs/native-scanning.md) for current limits.

## Programmer calculator

HEX, DEC, OCT and BIN are displayed together with 8/16/32/64-bit words and optional signed two's-complement interpretation. Integer arithmetic, bitwise operations, shifts, rotations, memory and a clickable bit view work with both the keypad and keyboard. Enter calculates; Esc clears; Ctrl+C/Ctrl+V copy and paste. Operations run left to right and wrap to the selected word size. The borderless readings and grouped controls use the application's theme palette.

## Build from source

Dependencies: CMake 3.24+, a C++23 compiler, Qt 6.8+ with Core/Gui/Widgets/Concurrent/Network (and Test for tests), the matching Qt Image Formats module, Vulkan SDK and Python 3.10+ development/embed files.

For Windows, use MSVC x64 and the matching MSVC Qt build. Copy `CMakeUserPresets.example.json` to `CMakeUserPresets.json` and adjust local dependency paths. In CLion, use the Visual Studio toolchain with LLDB and enable the `windows-release` profile.

From a Visual Studio x64 developer terminal:

```sh
cmake --preset windows-release
cmake --build --preset windows-package
```

The `windows_package` target builds and checks the portable package and writes a ZIP plus SHA-256 checksum under `dist/<timestamp>/`. Building `binary_workbench` alone does not create a portable distribution. See [Windows release instructions](docs/windows-release.md).

Keep local presets, IDE configuration, SDKs and build outputs out of version control. Use separate development and Release build directories and avoid simultaneous configure/build operations in the same directory.

Windows x64 and macOS ARM64 have been built and tested during development. Linux has not yet been validated. The published binary package targets Windows x64.

## Validation and documentation

The latest Windows validation completed **45 reader tests and 59 interface tests**, with 13 optional checks skipped in the automated suite; native Vulkan textured rendering was also verified separately during development. Packaging checks the application, image handlers and bundled Python scripting with development Qt/Python paths removed from the environment. A separate clean-machine validation is still pending.

See [the user guide](docs/user-guide.md), [Windows build/release notes](docs/windows-release.md), and [third-party notices](tools/THIRD-PARTY.txt). Additional documentation may still contain older descriptions while the application evolves.
