from pathlib import Path
import json,hashlib
root=Path('txrd2-hakone-capture');r=json.loads((root/'exact-vram-matches.json').read_text());vram=(root/'gs-state.bin').read_bytes()[-84-4194304:-84];textures=json.loads((root/'gif-register-audit.json').read_text())['textures'];own=Path('txr-analysis/texture_probe.py').read_text();exec(own[own.index('def gs('):own.index('def morton(')]);files={}
for e in r['matches']:
 t=next(t for t in textures if t['TEX0']==e['runtimeTEX0']);entries=16 if e['type']==4 else 256;b=files.setdefault(e['file'],Path(e['file']).read_bytes());e['paletteIdentical']=None
 if t['palettePsm'] not in(0,1) or t['paletteMode']!=0 or e['paletteBytes']<entries*4:continue
 pal=b[e['paletteOffset']:e['paletteOffset']+entries*4];out=bytearray()
 for i in range(entries):
  ci=i+(t['paletteBank']&15)*16 if entries==16 else i
  physical=(ci&~24)|((ci&8)<<1)|((ci&16)>>1) if entries==16 else ci
  a=(t['paletteBaseBytes']+gs(physical%16,physical//16,64,32)//8)%4194304;out+=vram[a:a+4]
 e['paletteIdentical']=pal==out;e['paletteEqualEntries']=sum(pal[i:i+4]==out[i:i+4] for i in range(0,len(pal),4));e['paletteComparison']='I4 CLD=0 CSA slice of resident CSM1 palette' if entries==16 else'I8 CSM1 CBP 16x16 storage';e['paletteSha256']=hashlib.sha256(pal).hexdigest();e['runtimePaletteSha256']=hashlib.sha256(out).hexdigest()
r['paletteScope']='Initial snapshot CBP memory equality, using CSA slices for observed I4 CLD=0 bindings; not a replay of the draw-time CLUT cache.'
unique={}
for e in r['matches']:
 k=(e['file'],e['pictureOffset']);unique.setdefault(k,[]).append(e)
r['uniqueSourceImages']=len(unique);r['uniqueSourceImagesWithExactPalette']=sum(any(e['paletteIdentical']==True for e in group) for group in unique.values());(root/'exact-vram-matches.json').write_text(json.dumps(r,indent=2));print(r['uniqueSourceImages'],r['uniqueSourceImagesWithExactPalette'])
