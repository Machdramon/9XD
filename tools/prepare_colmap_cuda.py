"""Stage optional pinned COLMAP 4.2.1 CUDA tools for Windows, with upstream SHA-256.
No global PATH changes. Not bundled into published packages by this helper.
"""
from pathlib import Path
import urllib.request,hashlib,json,zipfile,subprocess,os
repo=Path(__file__).resolve().parents[1];root=repo/'build/reconstruction-tools';root.mkdir(parents=True,exist_ok=True);url='https://github.com/colmap/colmap/releases/download/4.2.1/colmap-x64-windows-cuda.zip';digest='e9c5cbd84c2ea986d2e970a2473fc2d2e6b34a2cdcf5d3df2765c319a63af881'
f=root/'colmap-cuda-4.2.1.zip';dest=root/'colmap-cuda-4.2.1'
if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest()!=digest:
 req=urllib.request.Request(url,headers={'User-Agent':'9XD-reconstruction'})
 with urllib.request.urlopen(req,timeout=120) as inp,f.with_suffix('.part').open('wb') as out:
  while chunk:=inp.read(1024*1024):out.write(chunk)
 assert hashlib.sha256(f.with_suffix('.part').read_bytes()).hexdigest()==digest
 f.with_suffix('.part').replace(f)
with zipfile.ZipFile(f) as z:
 for n in z.namelist():
  if not (dest/n).resolve().is_relative_to(dest.resolve()):raise RuntimeError('Unsafe archive path')
 z.extractall(dest)
exe=dest/'bin/colmap.exe';env=dict(os.environ);env['PATH']=str(exe.parent)+';'+env['PATH'];env['QT_PLUGIN_PATH']=str(dest/'plugins');env.pop('QT_QPA_PLATFORM_PLUGIN_PATH',None)
p=subprocess.run([str(exe),'--help'],env=env,capture_output=True,timeout=30,check=True)
r={'version':'4.2.1','url':url,'sha256':digest,'bytes':f.stat().st_size,'executable':str(exe),'help':(p.stdout+p.stderr).decode('utf-8',errors='replace')[:250]};(dest/'receipt.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
