"""Research admission prototype for the verified March/AZ1 headerless bank only."""
from pathlib import Path
import struct,hashlib,json

def parse_bank(b):
 if len(b)<48 or struct.unpack_from('<I',b)[0]!=2:raise ValueError('Expected the observed two-section container')
 first,start=struct.unpack_from('<II',b,4)
 if not(16<=first<start<=len(b)-48 and first%16==0 and start%16==0 and b[first:first+8]==b'0IVX00.1' and b[first+16:first+24]==b'LDMX00.1'):raise ValueError('Unverified container/model boundary')
 p=start;records=[]
 while len(b)-p>16:
  if len(b)-p<48:raise ValueError('Truncated picture header')
  total,pal,size,hs,nc,pf,lv,cl,ty,w,h=struct.unpack_from('<IIIHHBBBBHH',b,p)
  if total<48 or total>len(b)-p or len(records)>=4096:raise ValueError('Unbounded picture span')
  value=struct.unpack_from('<Q',b,p+24)[0]
  if (pf,lv,cl,ty,nc)!=(0,1,3,5,256) or not(1<=w<=2048 and 1<=h<=2048) or 1<<((value>>26)&15)!=w or 1<<((value>>30)&15)!=h or ((value>>20)&63)!=19:raise ValueError('Outside observed indexed8 profile')
  kind='reference-only' if (total,pal,size)==(48,0,0) else 'pixels'
  if kind=='reference-only':
   if hs not in(48,128):raise ValueError('Unknown alias header declaration')
  elif not(hs==128 and size==w*h and pal==1024 and total==hs+size+pal):raise ValueError('Unverified stored image layout')
  r={'slot':len(records),'offset':p,'total':total,'declaredHeaderSize':hs,'kind':kind,'width':w,'height':h,'TEX0':hex(value)}
  if kind=='pixels':r.update(imageOffset=p+hs,imageBytes=size,paletteOffset=p+hs+size,paletteBytes=pal,pixelSha256=hashlib.sha256(b[p+hs:p+hs+size]).hexdigest(),paletteSha256=hashlib.sha256(b[p+hs+size:p+total]).hexdigest())
  records.append(r);p+=total
 if len(b)-p!=16 or any(b[p:]):raise ValueError('Expected observed 16-byte zero trailer')
 return {'bankOffset':start,'records':records,'trailerOffset':p,'trailerBytes':16,'scope':'Observed layout only. Empty alias slots stay unresolved; no image substitution or mesh binding inferred.'}

if __name__=='__main__':
 src=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]\2022\000002a0.sdb');original=src.read_bytes();positive=parse_bank(original);assert len(positive['records'])==6;assert [r['slot'] for r in positive['records'] if r['kind']=='pixels']==[0,3,4];assert [r['total'] for r in positive['records'] if r['kind']=='reference-only']==[48]*3
 cases=[]
 def altered(name,at,value,fmt='<I'):
  b=bytearray(original);struct.pack_into(fmt,b,at,value);cases.append((name,bytes(b)))
 start=positive['bankOffset'];alias=positive['records'][1]['offset']
 altered('bad directory count',0,3);altered('bank beyond file',8,len(original)+16);altered('zero total size',start,0);altered('huge total size',start,0xffffffff);altered('short indexed pixels',start+8,128);altered('short palette',start+4,768);altered('unknown picture format',start+16,1,'<B');altered('unexpected mip count',start+17,2,'<B');altered('wrong image dimensions',start+20,256,'<H');altered('alias with image payload',alias+8,1);altered('alias with unsafe header',alias+12,255,'<H');altered('nonzero trailer',len(original)-4,1);cases.append(('truncated payload',original[:-32]));cases.append(('truncated trailer',original[:-1]));passed=[]
 for name,b in cases:
  try:parse_bank(b)
  except ValueError:passed.append(name)
  else:raise AssertionError('Malformed case accepted: '+name)
 report={'source':str(src),'sourceSha256':hashlib.sha256(original).hexdigest(),'marchPositive':positive,'negativeChecksPassed':passed,'negativeChecks':len(passed),'nativeIntegration':False};Path('capture-checks/march-bank-validation.json').write_text(json.dumps(report,indent=2));print('March positive bank and',len(passed),'negative boundary/layout cases passed')
