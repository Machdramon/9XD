from pathlib import Path
import json,hashlib
from gs_capture_checks import run,source_pictures
base=Path(r'F:\copia de seguridad 08.04.2019\downloads\Kaido Racer [NTSC-PAL] [Es,Fr,De,En,It]\Kaido Racer [NTSC-PAL] [Es,Fr,De,En,It]\Kaido Racer\out');files=[];sources=[]
for i in range(0x442,0x44c):
 for f in base.glob(f'{i:08x}.*'):
  if f.stat().st_size>2000000:continue
  files.append(f);sources+=source_pictures(f)
r=run(Path('txrd1-alto-capture'),sources);r['sourceFileSha256']={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in files};Path('capture-checks/alto-europe-family.json').write_text(json.dumps(r,indent=2));m=[x for x in r['matches'] if x['stage']=='draw-binding'];print('Sources',len(files),'pictures',len(sources),'matched payloads',len({x['pixelSha256'] for x in m}))
for f in files:
 a=[x for x in m if x['source']==str(f)];print(f.name,'pictures',len({x['pictureOffset'] for x in a}),'full pal',len({x['pictureOffset'] for x in a if x['storedPaletteEqual']}))
