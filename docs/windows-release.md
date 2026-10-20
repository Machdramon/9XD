# Windows builds and releases

Use the **Visual Studio / MSVC x64** toolchain with the MSVC build of Qt. Do not
mix the MinGW toolchain with MSVC Qt libraries or an existing MSVC build cache.
The local CMake preset explicitly selects the `Visual Studio` CLion toolchain.

In CLion, reload the CMake project after changing presets. Enable the
`windows-release` profile under Settings → Build, Execution, Deployment → CMake.
Select `windows-release` and build the **windows_package** target. The ordinary
`binary_workbench` target builds the application but does not package it.

From a Visual Studio x64 developer terminal, the equivalent commands are:

```
cmake --preset windows-release
cmake --build --preset windows-package
```

`CMakeUserPresets.json` contains this machine's Qt/Python paths and is intentionally
not committed. For another machine, copy `CMakeUserPresets.example.json` to
`CMakeUserPresets.json` and adjust the dependency paths. Install Qt Image Formats
for the same Qt version to provide TIFF/WebP. A compatible Vulkan SDK and Python
development installation are required to compile; end users do not need them.

The verified ZIP is created in `dist/<timestamp>/`. Each build uses a new output
folder. The package includes Qt, image/platform plugins, matching Python and its
standard library, the Vulkan loader, redistributable MSVC runtime files, the
documentation and third-party notices. Its SHA-256 checksum is written beside it.

Publish the ZIP, not the bare executable. Users must extract the entire folder
and run `9XD.exe` with its DLLs and subfolders still beside it.
The package is unsigned. Vulkan 3D still requires a compatible GPU driver;
C++ scripting requires a local compiler. Third-party Python packages are not
included. Qt and the bundled Python runner are checked with Qt/Python removed
from PATH before the ZIP is produced.

If Ninja reports `failed recompaction: Permission denied`, stop simultaneous
build/configure operations against the same build folder and reload CMake.
Use separate build folders for `windows-local` and `windows-release`. The
MSVC include-prefix override must match the compiler output language; do not
change VSLANG independently of that override.
