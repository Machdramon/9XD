from pathlib import Path
import json
from gs_capture_checks import run,source_pictures
base=Path(r'F:\copia de seguridad 08.04.2019\downloads\Kaido Racer [NTSC-PAL] [Es,Fr,De,En,It]\Kaido Racer [NTSC-PAL] [Es,Fr,De,En,It]\Kaido Racer\out');files=[];sources=[]
for f in base.iterdir():
 if not f.is_file() or not 500000<=f.stat().st_size<=16*1024*1024:continue
 try:
  with f.open('rb') as stream:head=stream.read(512)
 except OSError:continue
 if b'TIM2' not in head:continue
 pics=source_pictures(f)
 if pics:files.append(f);sources+=pics
print('Candidate banks',len(files),'pictures',len(sources),flush=True)
r=run(Path('txrd1-hakone-capture'),sources);Path('capture-checks/hakone-europe-bank-search.json').write_text(json.dumps(r,indent=2));m=[x for x in r['matches'] if x['stage']=='draw-binding'];print('Matched pixel payloads',len({x['pixelSha256'] for x in m}),flush=True)
for f in files:
 a=[x for x in m if x['source']==str(f)]
 if a:print(f.name,'pictures',len({x['pictureOffset'] for x in a}),'full pal',len({x['pictureOffset'] for x in a if x['storedPaletteEqual']}),flush=True)
