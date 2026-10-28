import os
import struct,os,collections,sys
sys.path.insert(0,os.path.dirname(__file__))
from genki_vif import *
root=os.environ.get('STUDY_CASES',r'C:\Users\Jarky\Documents\study cases')
def search(path,only=None):
    b=open(os.path.join(root,path),'rb').read();ms=models(b)
    tot=collections.Counter();hit=collections.Counter();later=collections.Counter();lat_n=collections.Counter()
    for i,m in enumerate(ms):
        end=ms[i+1] if i+1<len(ms) else len(b)
        for pk in packets(b,m,end):
            n=pk['nv']
            for s in pk['streams'][1:]:
                comps=((s['cmd']>>2)&3)+1;bits=32>>(s['cmd']&3);es=comps*bits//8
                if s['n']!=n: continue
                key=(f"{s['cmd']:02x}@{s['imm']:04x}")
                for byte in range(es):
                    for bit in range(8):
                        k=(key,byte,bit);tot[k]+=1
                        g=lambda j:(b[s['data']+j*es+byte]>>bit)&1
                        if g(0) and g(1): hit[k]+=1
                        for j in range(2,n): later[k]+=g(j);lat_n[k]+=1
    res=[]
    for k in tot:
        if tot[k]<20: continue
        r=hit[k]/tot[k];lf=later[k]/max(1,lat_n[k])
        if r>=0.97 and 0.005<lf<0.6: res.append((r,round(lf,3),tot[k],k))
    res.sort(key=lambda x:(-x[0],x[1]))
    print(path)
    for x in res[:10]: print('   firstTwo=%.3f laterSet=%s packets=%d %s'%x)
for p in ['kaido racer/000004ac.dat','kaido racer/000031c4.nfc','c1 ps2/000004b8.dat','txrd2/car/000002a0.sdb','txr3/000000c4.nfc','wm ps2/000000a3.0iv']: search(p)
