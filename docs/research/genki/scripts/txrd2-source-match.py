from pathlib import Path
import struct,json,collections
root=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]\2022');capture=Path('txrd2-capture/az1.gs').read_bytes();report=[]
for index in range(295,306):
 candidates=list(root.glob(f'{index:08x}.*'))
 for file in candidates:
  if file.stat().st_size>32*1024*1024:continue
  b=file.read_bytes();pictures=[];start=0
  while True:
   start=b.find(b'TIM2',start)
   if start<0:break
   if start+16>len(b):break
   align=b[start+5];count=struct.unpack_from('<H',b,start+6)[0];p=start+(128 if align else 16);start+=4
   if align>1 or not 1<=count<=4096:continue
   for i in range(count):
    if p+48>len(b):break
    total,pal,size,hs,nc,pf,lv,cl,ty,w,h=struct.unpack_from('<IIIHHBBBBHH',b,p)
    if total<48 or p+total>len(b) or hs+size+pal>total:break
    raw=b[p+hs:p+hs+size];sample=raw[:min(256,len(raw))];match=capture.find(sample) if len(sample)>=64 and len(set(sample))>=16 else -2
    pictures.append({'pictureOffset':p,'width':w,'height':h,'type':ty,'pictureFormat':pf,'imageBytes':size,'prefixCaptureOffset':match});p+=total
  report.append({'file':str(file),'bytes':len(b),'modelSignatureOffset':b.find(b'0IVX00.1'),'pictures':pictures})
Path('txrd2-capture/az1-source-candidates.json').write_text(json.dumps(report,indent=2));print(json.dumps([{'name':Path(r['file']).name,'pictures':len(r['pictures']),'prefixMatches':sum(p['prefixCaptureOffset']>=0 for p in r['pictures'])} for r in report]))
