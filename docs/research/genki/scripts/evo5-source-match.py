from pathlib import Path
import struct,json,hashlib,collections
root=Path('txr3-evo5-capture');sourceRoot=Path(r'C:/Users/Jarky/Documents/study cases/txr3');records=[];files=[]
for key in ['000000c4']:
 for file in sourceRoot.glob(key+'.*'):
  b=file.read_bytes();start=0;count=0
  while True:
   at=b.find(b'TIM2',start)
   if at<0:break
   start=at+4
   if at%16 or at+16>len(b):continue
   align=b[at+5];n=struct.unpack_from('<H',b,at+6)[0]
   if align>1 or not 1<=n<=4096:continue
   p=at+(128 if align else 16)
   for i in range(n):
    if p+48>len(b):break
    total,pal,size,hs,nc,pf,lv,cl,ty,w,h=struct.unpack_from('<IIIHHBBBBHH',b,p)
    if total<48 or p+total>len(b) or hs<48 or hs+pal+size>total or not(0<w<=2048 and 0<h<=2048):break
    record={'file':str(file),'bankOffset':at,'surface':i,'pictureOffset':p,'total':total,'imageOffset':p+hs,'imageBytes':size,'paletteOffset':p+hs+size,'paletteBytes':pal,'width':w,'height':h,'type':ty,'format':pf,'levels':lv,'clut':cl,'TEX0':hex(struct.unpack_from('<Q',b,p+24)[0])}
    records.append(record);p+=total;count+=1
  files.append({'file':str(file),'pictures':count})
(root/'source-banks.json').write_text(json.dumps({'files':files,'pictures':records},indent=2));print(json.dumps(files));print('layouts',dict(collections.Counter((e['format'],e['type']) for e in records)))
state=(root/'gs-state.bin').read_bytes();assert len(state)==4194813;vram=state[-84-4194304:-84];textures=json.loads((root/'gif-register-audit.json').read_text())['textures'];own=Path('txr-analysis/texture_probe.py').read_text();exec(own[own.index('def gs('):own.index('def morton(')]);cache={};sourceCache={};matches=[]
for e in records:
 if e['format'] or e['type'] not in (4,5):continue
 w,h=e['width'],e['height'];bits=4 if e['type']==4 else 8;size=w*h*bits//8
 if size>e['imageBytes']:continue
 b=sourceCache.setdefault(e['file'],Path(e['file']).read_bytes());raw=b[e['imageOffset']:e['imageOffset']+size];target=hashlib.sha256(raw).hexdigest()
 for t in textures:
  if t['psm']!=(20 if bits==4 else 19) or t['width']!=w or t['height']!=h or not t['pitch']:continue
  key=(t['baseBytes'],t['pitch'],w,h,bits)
  if key not in cache:
   out=bytearray(size)
   for y in range(h):
    for x in range(w):
     a=(t['baseBytes']*8+gs(x,y,t['pitch'],bits))%(4194304*8);value=(vram[a//8]>>(a%8))&((1<<bits)-1);dest=(y*w+x)*bits;out[dest//8]|=value<<(dest%8)
   cache[key]=hashlib.sha256(out).hexdigest()
  if cache[key]!=target:continue
  match={**e,'runtimeTEX0':t['TEX0'],'runtimePixelBase':t['baseBytes'],'indexSha256':target,'paletteIdentical':None}
  entries=16 if bits==4 else 256
  if t['palettePsm']==0 and t['paletteMode']==0 and t['paletteBank']==0 and e['paletteBytes']>=entries*4:
   pal=b[e['paletteOffset']:e['paletteOffset']+entries*4];captured=bytearray()
   for i in range(entries):
    a=(t['paletteBaseBytes']+gs(i%16,i//16,64,32)//8)%4194304;captured+=vram[a:a+4]
   match['paletteIdentical']=pal==captured;match['paletteEqualEntries']=sum(pal[i:i+4]==captured[i:i+4] for i in range(0,len(pal),4))
  matches.append(match)
(root/'exact-vram-matches.json').write_text(json.dumps({'matches':matches,'scope':'Exact initial GS VRAM pixel comparisons; current-frame uploads and per-mesh bindings require chronological replay.'},indent=2));print('exact pixel matches',len(matches),'palette verified',sum(e['paletteIdentical']==True for e in matches));print('by source',dict(collections.Counter(Path(e['file']).name for e in matches)))
