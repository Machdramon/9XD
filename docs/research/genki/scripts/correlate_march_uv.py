from pathlib import Path
import json,struct,math,collections
b=Path('txrd2-march-capture/march.gs').read_bytes();info=json.loads(Path('txrd2-march-capture/capture-info.json').read_text());p=8+struct.unpack_from('<I',b,4)[0]+info['stateSize']+8192;stream=bytearray()
while p<len(b):
 k=b[p];p+=1
 if k==0:n=struct.unpack_from('<I',b,p+1)[0];p+=5;stream+=b[p:p+n];p+=n
 elif k==1:p+=1
 elif k==2:p+=4
 elif k==3:p+=8192
q=0;tex=[0,0];prim=0;values=collections.defaultdict(set)
while q<len(stream):
 tag,regs=struct.unpack_from('<QQ',stream,q);q+=16;n=tag&32767;fl=(tag>>58)&3;nr=(tag>>60)&15 or 16;size=n*16 if fl>=2 else n*nr*(16 if fl==0 else 8)
 if fl==0 and (tag>>46)&1:prim=(tag>>47)&2047
 if fl==0:
  for i in range(n*nr):
   reg=(regs>>((i%nr)*4))&15;o=q+i*16
   if reg==14:
    r=stream[o+8]&127;v=struct.unpack_from('<Q',stream,o)[0]
    if r in(6,7):tex[r-6]=v
    if r==0:prim=v
   elif reg==2:
    s,t,z=struct.unpack_from('<fff',stream,o)
    if z and all(math.isfinite(x) for x in (s,t,z)):
     u,v=s/z*4096,t/z*4096
     if abs(u)<100000 and abs(v)<100000:values[tex[(prim>>9)&1]].add((round(u),round(v)))
 q+=(size+15)&~15
src=Path(r'C:\Users\Jarky\Documents\study cases\txrd2\car\000002a0.sdb').read_bytes();e=json.loads(Path('capture-checks/march-evidence.json').read_text());result=[]
for g in e['v1MeshFlagGroups']:
 first=int(g['firstDmaOffset'],0);end=int(g['end'],0);d=first-208;ref=struct.unpack_from('<I',src,d+68)[0];index=(ref>>16)&255;matches=[];sourceSets=[set(),set()];formats=collections.Counter()
 for chunk in e['previewChunks']:
  a=int(chunk['dmaOffset']);stop=int(chunk['dmaEnd']);count=chunk['vertexCount']
  if not first<=a<end:continue
  for cmd,step in [(0x65,4),(0x6d,8)]:
   at=src.find(bytes([3,0x80,count%256,cmd]),a+16,stop)
   if at<0 or at+4+count*step>stop:continue
   formats[hex(cmd)]+=1
   for i in range(count):
    sourceSets[0].add(struct.unpack_from('<hh',src,at+4+i*step))
    if step==8:sourceSets[1].add(struct.unpack_from('<hh',src,at+8+i*step))
 if sourceSets[0]:
  dims=[(v,texvalue) for texvalue,v in values.items() if (1<<((texvalue>>26)&15),1<<((texvalue>>30)&15))==((512,512) if index==0 else (256,256) if index==4 else (128,128))]
  captured=set().union(*(v for v,_ in dims)) if dims else set();counts=[len(s&captured) for s in sourceSets];result.append({'descriptor':hex(d),'image':index,'formats':dict(formats),'primaryUnique':len(sourceSets[0]),'secondaryUnique':len(sourceSets[1]),'primaryRuntimeMatches':counts[0],'secondaryRuntimeMatches':counts[1]})
print(json.dumps(result,indent=2));Path('capture-checks/march-uv-runtime-correlation.json').write_text(json.dumps(result,indent=2))
