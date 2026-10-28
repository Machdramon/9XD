import os,sys,collections,struct
from vif import *
root='/mnt/user-data/uploads/study cases'
def find_streams(b,minunpacks=3):
    covered=0;out=[];L=len(b)
    a=np.frombuffer(b,'<u4',L//4)
    cmd=(a>>24)&0x7f
    seeds=np.nonzero((cmd>=0x60))[0]*4
    for s in seeds:
        s=int(s)
        if s<covered: continue
        best=None
        for back in range(64,-4,-4):
            st=s-back
            if st<covered or st<0: continue
            stop,ev=walk(b,st,L,maxlen=back+8,dma=False)
            if any(e[1]==s for e in ev if e[0]!='DMA') : best=st;break
        if best is None: continue
        stop,ev=walk(b,best,L)
        nu=sum(1 for e in ev if e[0]=='U')
        if nu>=minunpacks and stop-best>=64:
            out.append((best,stop,ev));covered=stop
        else: covered=max(covered,s+4)
    return out
if __name__=='__main__':
    for g in sys.argv[1:]:
        for dp,dn,fn in os.walk(os.path.join(root,g)):
            for f in sorted(fn):
                p=os.path.join(dp,f)
                if os.path.getsize(p)>12_000_000: print(os.path.relpath(p,root),'skipped (size)');continue
                b=open(p,'rb').read();st=find_streams(b)
                tot=sum(s[1]-s[0] for s in st)
                c=collections.Counter();k=collections.Counter()
                for s in st:
                    for e in s[2]:
                        if e[0]=='U': c[f"{FMT[(e[2][0]>>2)&3]}-{BITS[e[2][0]&3]}"]+=1
                        elif e[0]=='DMA': k['dma']+=1
                        elif e[0]=='C' and e[2][0] in(0x14,0x15,0x17): k['kick']+=1
                        elif e[0]=='D': k['direct']+=1
                print(f"{os.path.relpath(p,root)}: {len(b)}B streams {len(st)} cover {100*tot/max(1,len(b)):.1f}% {dict(k)} {c.most_common(6)}")
