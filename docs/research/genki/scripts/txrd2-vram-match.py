from pathlib import Path
import struct,json,hashlib
root=Path('txrd2-capture');state=(root/'gs-state.bin').read_bytes();assert len(state)==4194813 and struct.unpack_from('<I',state)[0]==9
# Version 9 freeze tail: four GIF tag (16) + register cursor (4) pairs, then Q (4).
for i in range(4):assert struct.unpack_from('<I',state,len(state)-84+i*20+16)[0]<=15
vram=state[-84-4194304:-84];assert len(vram)==4194304
own=Path('txr-analysis/texture_probe.py').read_text();exec(own[own.index('def gs('):own.index('def morton(')])
bank=json.loads((root/'headerless-bank-audit.json').read_text());source=Path(bank['source']).read_bytes();bindings=json.loads((root/'gif-register-audit.json').read_text())['textures'];matches=[]
for entry in bank['entries']:
 if entry['kind']!='pixels':continue
 w,h=entry['width'],entry['height'];bits=4 if entry['type']==4 else 8;raw=source[entry['offset']+entry['headerSize']:entry['offset']+entry['headerSize']+entry['imageBytes']];target=hashlib.sha256(raw[:w*h*bits//8]).hexdigest()
 for t in bindings:
  if t['psm']!=(20 if bits==4 else 19) or t['width']!=w or t['height']!=h or not t['pitch']:continue
  out=bytearray(w*h*bits//8)
  for y in range(h):
   for x in range(w):
    a=(t['baseBytes']*8+gs(x,y,t['pitch'],bits))%(4194304*8);value=(vram[a//8]>>(a%8))&((1<<bits)-1);dest=(y*w+x)*bits;out[dest//8]|=value<<(dest%8)
  if hashlib.sha256(out).hexdigest()==target:matches.append({'sourcePictureOffset':entry['offset'],'width':w,'height':h,'TEX0':t['TEX0'],'gsBaseBytes':t['baseBytes'],'indexSha256':target})
report={'candidate':bank['source'],'matches':matches,'scope':'Exact base-level indexed-pixel equality against initial GS VRAM; palette identity and per-mesh material assignment are separate.'};(root/'exact-vram-matches.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
