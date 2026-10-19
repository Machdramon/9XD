# Scripting Library Manager

Open **Scripting > Library Manager**.

The tree separates **Default Modules** (application, runtime and compiler libraries) from **My Modules** (only code added by you). Select an entry to read its **Documentation**, including origin and usage, or inspect its **Code**.

## Runtime and bundled libraries

The manager queries the same `script_runner` and Python runtime used by scripts. It lists the Python version, discoverable standard-library modules, installed distributions and their versions, plus the Workbench API. These entries are read-only; distribution names do not always equal Python import names. The C++ standard library comes from the local compiler toolchain.

Additional application-shipped source libraries can be placed beside the executable under `libraries/python` and `libraries/cpp`. They appear as bundled, read-only entries. User libraries are stored separately under the user's standard data location, in `BinaryWorkbench/libraries/python` and `BinaryWorkbench/libraries/cpp`.

## Create or paste your own library

1. Click **Add Module...**, choose Python or C++, and select **Create module / paste code**. Enter a module name such as `my_helpers`.
2. Write or paste code into the dialog.
3. Click **Add Module**. Python creates `my_helpers.py`; C++ creates `my_helpers.hpp`.
4. Use `import my_helpers` in Python, or `#include "my_helpers.hpp"` in a C++ script.

C++ libraries here are header-based: use inline/template implementations, or explicitly include source as appropriate. This manager does not automatically compile/link separate C++ libraries or install native `.lib`/DLL dependencies.

Saved user libraries are shared across documents and remain available after restarting. Changes are picked up by subsequent runs. Existing scripts retain normal Python import caching and compiler behavior while running.

## Import existing code

- **Add Module... > Import source file** copies a Python module or C++ header into the managed folder.
- **Add Module... > Import package / header folder** copies a package/header folder. Select Python or C++ before importing. Python package folders must contain `__init__.py`; relative imports are preserved.
- Imports do not overwrite an existing library. Source files are limited to 1 MiB; folders to 1024 supported files and 32 MiB. Symlinks and compiled artifacts are not imported. Supported package files include source and common text-data files; this is not a wheel/native-package installer.

The editor saves UTF-8 source. Runtime/bundled entries and unreadable or non-UTF-8 source remain read-only. Copy Code lets you reuse source in a new library.

## Enable, disable and edit

Select a user library and edit its source, then **Save Changes**. **Enable Module / Disable Module** renames a top-level module or package with a `.disabled` suffix; enabling restores its original name. Disabled libraries remain on disk. The next run no longer finds them under their normal import/include name.

The manager asks before discarding unsaved code and checks whether a saved library changed on disk before overwriting it. Use Refresh to update the list and runtime inventory. Third-party packages already installed in the active Python runtime are displayed but cannot be installed/uninstalled through this interface.

Standard Python paths remain ahead of custom paths; own libraries should use unique module names. The Workbench module/header name is reserved.

