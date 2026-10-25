"""Stage optional reconstruction tools without changing the application runtime."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
CACHE = ROOT / 'build' / 'reconstruction-tools'
CACHE.mkdir(parents=True, exist_ok=True)
receipt = {}

def fetch(url):
    return urllib.request.urlopen(urllib.request.Request(url, headers={'User-Agent': '9XD-dependency-setup'}), timeout=120)

def archive(name, url, expected):
    target = CACHE / (name + '.zip')
    if not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest() != expected:
        partial = target.with_suffix('.part')
        with fetch(url) as source, partial.open('wb') as output:
            while chunk := source.read(1024 * 1024):
                output.write(chunk)
        if hashlib.sha256(partial.read_bytes()).hexdigest() != expected:
            raise RuntimeError('Checksum mismatch: ' + name)
        partial.replace(target)
    destination = CACHE / name
    with zipfile.ZipFile(target) as bundle:
        for entry in bundle.infolist():
            resolved = (destination / entry.filename).resolve()
            if not resolved.is_relative_to(destination.resolve()):
                raise RuntimeError('Unsafe archive path')
        bundle.extractall(destination)
    receipt[name] = {'url': url, 'sha256': expected, 'directory': str(destination)}
    (CACHE / 'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(name + ': checksum verified and extracted', flush=True)

with fetch('https://api.github.com/repos/colmap/colmap/releases/latest') as response:
    release = json.load(response)
asset = next(a for a in release['assets'] if a['name'] == 'colmap-x64-windows-nocuda.zip')
digest = asset.get('digest', '')
if not digest.startswith('sha256:'):
    raise RuntimeError('Official COLMAP asset has no SHA-256 digest')
archive('colmap-' + release['tag_name'], asset['browser_download_url'], digest[7:])
with fetch('https://api.github.com/repos/GyanD/codexffmpeg/releases/latest') as response:
    ff_release = json.load(response)
ff_asset = next(a for a in ff_release['assets'] if a['name'].endswith('-essentials_build.zip'))
if not ff_asset.get('digest', '').startswith('sha256:'):
    raise RuntimeError('Official FFmpeg mirror has no SHA-256 digest')
archive('ffmpeg-essentials', ff_asset['browser_download_url'], ff_asset['digest'][7:])
wheel_dir = CACHE / 'wheels'
wheel_dir.mkdir(exist_ok=True)
subprocess.run([sys.executable, '-m', 'pip', 'download', '--only-binary=:all:', '--no-deps',
                '--dest', str(wheel_dir), 'open3d==0.19.0', 'onnxruntime==1.23.2'], check=True)
receipt['wheels'] = [{'name': p.name, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                     for p in sorted(wheel_dir.glob('*.whl'))]
(CACHE / 'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
print('Core wheels staged; dependencies and SAM encoder/model setup remain pending.', flush=True)
