import json,struct
from pathlib import Path
import numpy as np
from gs_capture_checks import Replay,gs,MEM
passed=[]
def fresh():
 b=bytearray(4194813);struct.pack_into('<I',b,0,9);return Replay(bytes(b),[])
for psm,bits in [(0,32),(1,24),(2,16),(19,8),(20,4)]:
 r=fresh();w,h=32,16;size=w*h*bits//8;raw=bytes((i*37+11)%256 for i in range(size));base=0x100*256
 r.register(0x50,(0x100<<32)|(2<<48)|(psm<<56));r.register(0x51,(7<<32)|(9<<48));r.register(0x52,w|(h<<32));r.register(0x53,0);r.image(raw[:16]);r.image(raw[16:]);assert len(r.uploads)==1 and r.uploads[0]['replayed'];y,x=np.mgrid[:h,:w];a=(base*8+gs(x+7,y+9,128,32 if bits==24 else bits))%(MEM*8)
 if bits==4:
  values=((r.memory[a//8]>>(a%8))&15).astype(np.uint8).ravel();out=(values[::2]|(values[1::2]<<4)).tobytes()
 else:out=r.memory[((a.ravel()//8)[:,None]+np.arange(bits//8))%MEM].tobytes()
 assert out==raw;passed.append('split host transfer PSM '+str(psm)+' exact write/read with nonzero origin')
r=fresh();r.register(0x50,(0x100<<32)|(2<<48));r.register(0x52,16|(16<<32));r.register(0x53,0);r.image(b'\x01'*16);r.finish_transfer();assert not r.uploads[-1]['replayed'] and r.unsupported['incomplete host transfer'];passed.append('reject incomplete transfer')
r=fresh();r.image(b'\x00'*16);assert r.unsupported['IMAGE without a captured host transfer start'];passed.append('reject image without start')
r=fresh();r.register(0x50,(0x100<<32)|(1<<48));r.register(0x52,16|(16<<32));r.register(0x53,2);assert r.valid[0]==255 and not np.all(r.valid[0x100*256:0x100*256+4096]);passed.append('local copy invalidates destination, not unrelated texture storage')
r=fresh();r.register(0x50,(0x100<<32)|(1<<48)|(10<<56));r.register(0x52,16|(16<<32));r.register(0x53,0);r.image(bytes(1024));r.finish_transfer();assert not r.uploads[-1]['replayed'];passed.append('unsupported pixel storage explicitly rejected')
Path('capture-checks/replay-validation.json').write_text(json.dumps({'passed':len(passed),'checks':passed,'scope':'Synthetic transfer and invalidation checks; not independent GPU validation or full GS conformance.'},indent=2));print(len(passed),'replay checks passed')
