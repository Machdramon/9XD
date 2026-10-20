"""Create and verify a relocatable Windows ZIP from an existing Release build."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile


def run(command, **kwargs):
    completed = subprocess.run([str(part) for part in command], capture_output=True,
                               text=True, errors='replace', timeout=120, **kwargs)
    if completed.stdout:
        print(completed.stdout, flush=True)
    if completed.stderr:
        print(completed.stderr, flush=True)
    completed.check_returncode()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--qt-bin', type=Path, required=True)
    parser.add_argument('--python', type=Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--vulkan-loader', type=Path, required=True)
    parser.add_argument('--vulkan-license', type=Path, required=True)
    options = parser.parse_args()
    source = Path(__file__).resolve().parents[1]
    build = options.build_dir.resolve()
    cache = (build / 'CMakeCache.txt').read_text(encoding='utf-8')
    if 'CMAKE_BUILD_TYPE:STRING=Release' not in cache:
        raise RuntimeError('Select the windows-release profile before building windows_package.')
    name = f'9XD-{options.version}-windows-x64'
    output = source / 'dist' / datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    stage = output / name
    stage.mkdir(parents=True)
    licenses = stage / 'licenses'
    licenses.mkdir()
    for filename in ('binary_workbench.exe', 'script_runner.exe'):
        shutil.copy2(build / filename, stage / ('9XD.exe' if filename == 'binary_workbench.exe' else filename))
    fbx_runtime = build / 'libfbxsdk.dll'
    fbx_enabled = any(line.startswith('WORKBENCH_FBX_SDK_ROOT:PATH=') and line.split('=', 1)[1].strip()
                      for line in cache.splitlines())
    if fbx_enabled:
        if not fbx_runtime.is_file() or not (build / 'licenses/Autodesk-FBX-SDK.rtf').is_file():
            raise RuntimeError('FBX-enabled builds require the SDK runtime and license notice.')
        shutil.copy2(fbx_runtime, stage / fbx_runtime.name)
        shutil.copy2(build / 'licenses/Autodesk-FBX-SDK.rtf', licenses / 'Autodesk-FBX-SDK.rtf')
    run([options.qt_bin / 'windeployqt.exe', '--release', '--no-translations',
         '--no-compiler-runtime', '--no-opengl-sw', stage / '9XD.exe',
         stage / 'script_runner.exe'])
    # Use the redistributable CRT directory, never developer/compiler DLLs.
    redist = Path(os.environ.get('VCToolsRedistDir', ''))
    crt = redist / 'x64' / 'Microsoft.VC143.CRT'
    if not crt.is_dir():
        vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*',
                                      '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                      '-property', 'installationPath'], text=True).strip()
        candidates = sorted((Path(vs) / 'VC/Redist/MSVC').glob('*/x64/Microsoft.VC143.CRT'))
        if not candidates:
            raise RuntimeError('MSVC x64 redistributable files were not found.')
        crt = candidates[-1]
        redist = crt.parents[1]
    for dll in crt.glob('*.dll'):
        shutil.copy2(dll, stage / dll.name)
    installer = redist / 'vc_redist.x64.exe'
    if installer.is_file():
        (stage / 'prerequisites').mkdir()
        shutil.copy2(installer, stage / 'prerequisites' / installer.name)
    shutil.copy2(options.vulkan_loader, stage / 'vulkan-1.dll')
    shutil.copy2(options.vulkan_license, licenses / 'Vulkan-Loader.txt')
    shutil.copytree(source / 'docs', stage / 'docs')
    shutil.copy2(source / 'src/bcdec-LICENSE.txt', licenses / 'bcdec.txt')
    qt_license = source / 'tools/LGPL-3.0.txt'
    if qt_license.is_file():
        shutil.copy2(qt_license, licenses / qt_license.name)
    shutil.copy2(source / 'tools/THIRD-PARTY.txt', licenses / 'THIRD-PARTY.txt')
    (stage / 'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\n', encoding='utf-8')
    # Bundle the matching interpreter and standard library; exclude user packages.
    python_root = options.python.resolve().parent
    python_stage = stage / 'python'
    python_stage.mkdir()
    shutil.copy2(options.python, python_stage / 'python.exe')
    version = subprocess.check_output([str(options.python), '-c',
                                      'import sys;print(str(sys.version_info.major)+str(sys.version_info.minor))'], text=True).strip()
    for filename in (f'python{version}.dll', 'python3.dll'):
        shutil.copy2(python_root / filename, python_stage / filename)
        shutil.copy2(python_root / filename, stage / filename)
    shutil.copy2(python_root / 'LICENSE.txt', licenses / 'Python.txt')
    shutil.copytree(python_root / 'DLLs', python_stage / 'DLLs',
                    ignore=shutil.ignore_patterns('*.pdb', '__pycache__'))
    if (python_root / 'tcl').is_dir():
        shutil.copytree(python_root / 'tcl', python_stage / 'tcl')
    with zipfile.ZipFile(python_stage / f'python{version}.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in (python_root / 'Lib').rglob('*'):
            relative = path.relative_to(python_root / 'Lib')
            if path.is_file() and not set(relative.parts).intersection(
                    {'site-packages', '__pycache__', 'test', 'tests', 'ensurepip', 'idlelib'}):
                archive.write(path, relative.as_posix())
    # Local DLLs let python.exe work as well as the linked script runner.
    for dll in crt.glob('*.dll'):
        shutil.copy2(dll, python_stage / dll.name)
    (stage / 'README.txt').write_text(
        '9XD\n\nExtract this entire folder before running 9XD.exe.\n'
        'Keep the DLLs, imageformats, platforms and python folders beside the executable.\n'
        'Windows 10/11 x64. A compatible graphics driver is required for Vulkan 3D.\n'
        'Python scripting includes the standard library, without third-party packages.\n'
        'C++ scripting requires a local compatible compiler/toolchain.\n'
        'If Windows reports a missing Visual C++ runtime, use prerequisites/vc_redist.x64.exe.\n'
        'This build is unsigned. See docs/user-guide.md and licenses/THIRD-PARTY.txt.\n', encoding='utf-8')
    env = {key: value for key, value in os.environ.items()
           if key.upper() not in {'PYTHONHOME', 'PYTHONPATH', 'QTDIR', 'QT_PLUGIN_PATH',
                                  'QT_QPA_PLATFORM_PLUGIN_PATH', 'QT_QPA_PLATFORM'}}
    env['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32')
    hidden = {'creationflags': subprocess.CREATE_NO_WINDOW}
    run([stage / '9XD.exe', '--deployment-check'], env=env, cwd=stage, **hidden)
    run([python_stage / 'python.exe', '-I', '-c',
         'import json,struct,zlib,ssl,sqlite3;assert struct.calcsize("Q")==8'], env=env, cwd=stage, **hidden)
    with tempfile.TemporaryDirectory() as directory:
        directory = Path(directory)
        context = {'path': '', 'size': '0', 'modified': '', 'memoryBlocks': [], 'patches': [],
                   'selection': {'start': '0', 'end': '0'}, 'mesh': {'vertices': [], 'indices': []}}
        (directory / 'context.json').write_text(json.dumps(context), encoding='utf-8')
        (directory / 'check.py').write_text('import workbench, json, ssl, struct\nassert workbench.read(0,0)==b""\n', encoding='utf-8')
        run([stage / 'script_runner.exe', 'python', directory / 'context.json',
             directory / 'check.py', directory / 'result.json'], env=env, cwd=stage, **hidden)
        if not json.loads((directory / 'result.json').read_text())['ok']:
            raise RuntimeError('Bundled Python runner verification failed.')
    archive_path = Path(shutil.make_archive(str(output / name), 'zip', output, name))
    digest = hashlib.file_digest(archive_path.open('rb'), 'sha256').hexdigest()
    archive_path.with_suffix('.zip.sha256').write_text(f'{digest}  {archive_path.name}\n', encoding='ascii')
    print(f'Verified Release: {archive_path}', flush=True)


if __name__ == '__main__':
    main()
