from pathlib import Path
import struct,json,collections
root=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer 3 [NTSC]\Tokyo Xtreme Racer 3 [NTSC]\Tokyo Xtreme Racer 3\Nueva carpeta working one')
def u(b,p):return struct.unpack_from('<I',b,p)[0]
def bank(file,section):
 b=(root/file).read_bytes();n=u(b,0);a=u(b,4+section*4);end=u(b,8+section*4) if section+1<n else len(b)
 assert b[a:a+4]==b'TIM2';q=a+(128 if b[a+5] else 16);images=[]
 for i in range(struct.unpack_from('<H',b,a+6)[0]):
  total,pal,pix,header=struct.unpack_from('<IIIH',b,q);assert total>=48 and q+total<=end and header>=48 and header+pal+pix<=total
  images.append(struct.unpack_from('<HH',b,q+20));q+=total
 return {'file':file,'section':section,'offset':a,'images':images}
variants=[('oval-A',0x4a5c,0x4ad4,0x4a5e,[0x4a5f,0x4a7c,0x4a9a,0x4ab7]),('oval-B',0x4ad5,0x4b4d,0x4ad7,[0x4ad8,0x4af5,0x4b13,0x4b30])]
reports=[]
for label,lo,hi,glob,locals in variants:
 globalbanks={0:bank(f'{glob:08x}.dat',0),4:bank(f'{glob:08x}.dat',1)}
 localbanks={i:{1:bank(f'{file:08x}.dat',3),2:bank(f'{file:08x}.dat',4)} for i,file in enumerate(locals)}
 counters=collections.Counter();refs=[]
 for ident in range(lo,hi+1):
  for file in root.glob(f'{ident:08x}.nfc'):
   b=file.read_bytes();segment=max((i for i,a in enumerate(locals) if a<ident),default=-1)
   for base in range(0,len(b)-48,16):
    if b[base:base+8]!=b'0IVX00.1':continue
    try:
     v1=b[base+16:base+24]==b'LDMX00.1';p=base+48+u(b,base+44)+48*u(b,base+32)
     for node in range(u(b,base+36)):
      flags=u(b,p);p+=80
      if not flags&0xf0:continue
      count=u(b,p+12);p+=48
      for part in range(count):
       descriptor=p;p+=112+(48 if v1 else 0);packets=u(b,p+4);span=u(b,p+8)*16;p+=48
       assert p+span+16<=len(b) and u(b,p+span)==0x60000000
       header=u(b,descriptor);ref=b[descriptor+13];image=b[descriptor+14]
       if header&0x20:
        chosen=globalbanks.get(ref) if ref in (0,4) else localbanks.get(segment,{}).get(ref)
        status='resolved-index' if chosen and image<len(chosen['images']) else 'unresolved'
        counters[status]+=1;counters['global' if ref in (0,4) else 'local']+=1
        clamp=struct.unpack_from('<Q',b,descriptor+(64 if header>>16==0 else 80))[0]
        dim=(((clamp>>14)&1023)+1,((clamp>>34)&1023)+1)
        if status=='resolved-index':counters['exact-clamp-size' if chosen['images'][image]==dim else 'clamp-size-differs']+=1
        refs.append({'file':file.name,'descriptor':descriptor,'segment':segment,'bankId':ref,'image':image,'status':status,'bankFile':chosen['file'] if chosen else None,'section':chosen['section'] if chosen else None,'dimensions':chosen['images'][image] if status=='resolved-index' else None,'clampExtent':dim})
       p+=span+16
    except (struct.error,AssertionError):counters['walk-error']+=1
 manifest={'schema':1,'game':'txr3','variant':label,'condition':'unlabelled','global':{str(k):{'path':v['file'],'section':v['section']} for k,v in globalbanks.items()},'segments':[{'startId':hex(a),'endId':hex(locals[i+1] if i+1<len(locals) else hi+1),'local':{str(k):{'path':v['file'],'section':v['section']} for k,v in localbanks[i].items()}} for i,a in enumerate(locals)]}
 reports.append({'context':manifest,'statistics':dict(counters),'references':refs})
Path('txr3-bank-link-validation.json').write_text(json.dumps(reports,indent=2),encoding='utf-8')
print(json.dumps([{'variant':r['context']['variant'],'statistics':r['statistics']} for r in reports]))
