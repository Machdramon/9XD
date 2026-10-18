# Binary Workbench

Binary Workbench is a desktop workspace for inspecting and editing binary files, interpreting texture data, and working with 3D geometry and scripts. It is built with C++23, Qt 6 and Vulkan. The interface is in English.

The project is under active development. Some tools and format variants are still incomplete; console texture support is specific to the formats and memory layouts listed below.

## Download and run

Get the Windows x64 portable ZIP from [Releases](https://github.com/Machdramon/binary-workbench/releases).

1. Extract the **entire ZIP** to a writable folder.
2. Open `binary_workbench.exe` inside the extracted folder.
3. Keep its DLLs and the `platforms`, `imageformats`, `python` and other bundled folders beside it.

Windows 10/11 x64 is required. The portable package includes Qt, the Python runtime and standard library, the Vulkan loader and Visual C++ runtime files. Vulkan 3D requires a compatible graphics driver. The application is currently unsigned. No separate Qt, Python or Vulkan SDK installation is needed to run the Windows package.

## Workspace

Each major module can be dragged out into a separate window for multiple monitors. Tool panels dock within their own module. Open binary documents use individual tabs, and workspace layout and settings persist between sessions.

| Module | Current tools |
| --- | --- |
| Hex Viewer | Main viewer, data inspector, pattern search and results, formatting rules, statistics and checksum inspector |
| 3D | Geometry view, mesh collection and geometry settings |
| UV / 2D | Image view, texture/image collection and image calculator |
| Scripting | Script editor, console and library management |
| Other Tools | Integer calculator, thread activity and settings |

Some planned actions, including Compare Data, are not yet implemented. Settings provides light/dark/system appearance and editor preferences; custom theme editing, theme import/export and additional interface languages are planned.

## Binary inspection and editing

- Open multiple files or create temporary RAM documents that grow as content is added.
- Navigate large files using 64-bit offsets, configurable bytes per row, selection and go-to-offset controls.
- Inspect text and numeric values with little-endian or big-endian interpretation, including ASCII, UTF-8, UTF-16 and CP932 text.
- Edit bytes, paste hexadecimal data, undo/redo and save through background operations. File-backed editing primarily overwrites bytes; general insertion/deletion is not implemented.
- Search patterns, inspect signatures and entropy, and apply named formatting rules to byte ranges.
- Calculate Checksum-8/16/32, CRC-32, MD5 and SHA-1/256/512 over a document, selection or range. Optionally compare with an expected result. Unsaved changes are included.
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

The 3D workspace provides a Vulkan geometry view and mesh-oriented tools. Texture-to-mesh workflows remain in development.

Python scripting uses the bundled interpreter and standard library in the Windows package. Additional third-party Python packages are not included. C++ scripting requires a compatible local compiler. Run scripts only when you trust their contents: scripts execute local code.

## Build from source

Dependencies: CMake 3.24+, a C++23 compiler, Qt 6.8+ with Core/Gui/Widgets/Concurrent (and Test for tests), the matching Qt Image Formats module, Vulkan SDK and Python 3.10+ development/embed files.

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

The latest Windows validation completed **38 reader tests and 29 interface tests**, with one optional graphics test skipped. Packaging checks the application, image handlers and bundled Python scripting with development Qt/Python paths removed from the environment. A separate clean-machine validation is still pending.

See [the user guide](docs/user-guide.md), [Windows build/release notes](docs/windows-release.md), and [third-party notices](tools/THIRD-PARTY.txt). Additional documentation may still contain older descriptions while the application evolves.
