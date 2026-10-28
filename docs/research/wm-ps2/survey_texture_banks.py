from pathlib import Path
import struct,json
root=Path(r'F:\copia de seguridad 08.04.2019\downloads\Wangan Midnight ps2\unpack\quickbms unpack')
results=[]
for p in root.rglob('*'):
 if not p.is_file() or p.stat().st_size>32*1024*1024: continue
 b=p.read_bytes();start=0
 while True:
  at=b.find(b'TIM2',start)
  if at<0: break
  start=at+4
  if at+16>len(b) or b[at+5]>1: continue
  count=struct.unpack_from('<H',b,at+6)[0]
  if not 120<=count<=4096: continue
  q=at+(128 if b[at+5] else 16);pictures=[];valid=True
  for i in range(count):
   if q+48>len(b):valid=False;break
   total,pal,pixels,header=struct.unpack_from('<IIIH',b,q);w,h=struct.unpack_from('<HH',b,q+20)
   if total<48 or q+total>len(b) or header<48 or header+pal+pixels>total or not w or not h: valid=False;break
   if i in (0,52,85,119):pictures.append({'index':i,'width':w,'height':h})
   q+=total
  if valid:results.append({'file':p.name,'offset':at,'count':count,'referencedImages':pictures})
Path('wm-texture-survey.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
print(json.dumps({'candidateBanks':len(results),'examples':results[:12]}))
