from pathlib import Path
import struct,json,collections
root=Path('txrd1-hakone-capture');b=(root/'hakone.gs').read_bytes();hs=struct.unpack_from('<I',b,4)[0];info=json.loads((root/'capture-info.json').read_text());p=8+hs+info['stateSize']+8192;paths=collections.defaultdict(bytearray);records=collections.Counter()
while p<len(b):
 kind=b[p];p+=1;records[kind]+=1
 if kind==0:
  path=b[p];n=struct.unpack_from('<I',b,p+1)[0];p+=5;assert n<=len(b)-p;paths[path]+=b[p:p+n];p+=n
 elif kind==1:p+=1
 elif kind==2:p+=4
 elif kind==3:p+=8192
 else:raise ValueError((kind,p))
assert p==len(b)
regs=collections.Counter();tex=collections.Counter();summary=[]
for path,data in paths.items():
 q=0;tags=0;imagebytes=0
 while q<len(data):
  assert q+16<=len(data),(path,q,'tag truncated');tag,registers=struct.unpack_from('<QQ',data,q);q+=16;loops=tag&32767;flg=(tag>>58)&3;nreg=(tag>>60)&15 or 16;tags+=1
  size=loops*16 if flg>=2 else loops*nreg*(16 if flg==0 else 8);padded=(size+15)&~15;assert q+padded<=len(data),(path,q,size,len(data))
  if flg>=2:imagebytes+=size
  else:
   for i in range(loops*nreg):
    reg=(registers>>((i%nreg)*4))&15;value=struct.unpack_from('<Q',data,q+i*(16 if flg==0 else 8))[0]
    if flg==0 and reg==14:reg=data[q+i*16+8]&127
    regs[reg]+=1
    if reg in (6,7):tex[value]+=1
  q+=padded
 summary.append({'path':path,'bytes':len(data),'gifTags':tags,'imageTransferBytes':imagebytes})
textures=[]
for value,count in tex.most_common():
 textures.append({'TEX0':hex(value),'writes':count,'baseBytes':(value&16383)*256,'pitch':((value>>14)&63)*64,'psm':(value>>20)&63,'width':1<<((value>>26)&15),'height':1<<((value>>30)&15),'paletteBaseBytes':((value>>37)&16383)*256,'palettePsm':(value>>51)&15,'paletteMode':(value>>55)&1,'paletteBank':(value>>56)&31})
report={'capture':'hakone.gs.zst','serial':info['serial'],'records':dict(records),'paths':summary,'uniqueTex0Bindings':len(textures),'textures':textures,'scope':'Register observations only; texture memory must be replayed at each draw before mapping to source assets.'};(root/'gif-register-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k!='textures'}));print('texture dimensions',dict(collections.Counter((t['psm'],t['width'],t['height']) for t in textures)))
