from pathlib import Path
import struct,json,hashlib
root=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]');b=(root/'BUILD.TOC').read_bytes();count=struct.unpack_from('<I',b)[0];assert len(b)==16+count*16;emitted=[];bad=[]
for i in range(count):
 sector,packed,size,flag=struct.unpack_from('<4I',b,16+i*16);offset=sector*2048
 if offset+packed>(root/'BUILD.DAT').stat().st_size:bad.append(i)
 if size or (offset and packed):emitted.append({'tocIndex':i,'tocRecordOffset':16+i*16,'sector':sector,'offset':offset,'packedBytes':packed,'unpackedBytes':size,'flag':flag})
target=emitted[0x2a0];print('entries',count,'emitted',len(emitted),'bad',len(bad),'target',target)
with (root/'BUILD.DAT').open('rb') as f:f.seek(target['offset']);packed=f.read(target['packedBytes'])
print('packed head',packed[:96].hex());print('source', (root/'2022/000002a0.sdb').stat().st_size)
Path('txrd2-march-capture/toc-link-investigation.json').write_text(json.dumps({'tocEntries':count,'emittedEntries':len(emitted),'badRanges':bad,'ordinalCandidate':target,'packedHead':packed[:96].hex()},indent=2))
