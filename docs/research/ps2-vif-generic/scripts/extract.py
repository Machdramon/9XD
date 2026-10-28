import os,sys,collections,struct,json
import numpy as np
from vif import *
from census import find_streams
KICK={0x14,0x15,0x17}
def batches(b,streams):
    for st,stop,ev in streams:
        cur=[]
        for e in ev:
            if e[0]=='U':
                (cmd,n,imm),cl,wl,data,size=e[2],e[3],e[4],e[5],e[6]
                try: arr=decode_unpack(b,cmd,n,imm,cl,wl,data)
                except Exception: continue
                cur.append(dict(cmd=cmd,vn=((cmd>>2)&3)+1,vl=cmd&3,n=n,addr=imm&0x3ff,usn=(imm>>14)&1,flg=imm>>15,cl=cl,wl=wl,raw=arr,off=data))
            elif e[0]=='C' and e[2][0] in KICK:
                if cur: yield cur
                cur=[]
        if cur: yield cur
def as_float(a): return a.view('<f4') if a.dtype==np.uint32 else None
def classify(batch):
    ns=collections.Counter(u['n'] for u in batch if u['n']>=3 and u['vn']>=2)
    if not ns: return None
    N=ns.most_common(1)[0][0]
    if ns[N]<2: return None
    roles={}
    pos=[];nrm=[];uv=[];col=[]
    for u in batch:
        if u['n']!=N: continue
        a=u['raw']
        if u['vl']==0:
            f=as_float(a).astype(np.float64)
            if not np.all(np.isfinite(f)): continue
            nz=np.abs(f[f!=0])
            if nz.size and (nz.min()<1e-7 or nz.max()>1e6): continue
            if u['vn']>=3:
                l=np.linalg.norm(f[:,:3],axis=1)
                if np.all(np.abs(l-1)<0.05): nrm.append((u,f[:,:3]))
                else: pos.append((u,f[:,:3]))
            elif u['vn']==2: uv.append((u,f))
        elif u['vl']==1 and u['vn']>=3 and not u['usn']:
            f=a[:,:3].astype(np.float64)
            l=np.linalg.norm(f,axis=1)
            if l.mean()>0 and np.std(l)/l.mean()<0.05: nrm.append((u,f/l.mean()))
            else: pos.append((u,f))
        elif u['vl']==1 and u['vn']==2: uv.append((u,a.astype(np.float64)))
        elif u['vl']==2 and u['vn']>=3:
            f=a[:,:3].astype(np.float64)
            l=np.linalg.norm(f,axis=1)
            if not u['usn'] and l.mean()>40 and np.std(l)/l.mean()<0.15: nrm.append((u,f/l[:,None].clip(1)))
            else: col.append((u,a))
    if not pos: return None
    # pick position stream with largest spread
    def coherent(P):
        if len(P)<3: return False
        d=np.linalg.norm(np.diff(P,axis=0),axis=1);diag=np.linalg.norm(np.ptp(P,axis=0))
        return len(P)<24 or (diag>0 and np.median(d)/diag<0.2)
    pos=[t for t in pos if coherent(t[1])]
    if not pos: return None
    p=max(pos,key=lambda t:np.ptp(t[1],axis=0).sum())
    return dict(N=N,pos=p,nrm=nrm[0] if nrm else None,uv=uv[0] if uv else None,col=col[0] if col else None,streams=[u for u in batch if u['n']==N])
def flag_sources(c):
    """candidate per-vertex boolean flags from integer views of streams"""
    out=[]
    for u in c['streams']:
        a=u['raw']
        for comp in range(a.shape[1]):
            col=a[:,comp].astype(np.int64)&( (1<<32)-1 if u['vl']==0 else (1<<16)-1 if u['vl']==1 else 255)
            bits=[15] if u['vl']==1 else [15,0] if u['vl']==0 else [7]
            for bit in bits:
                fl=((col>>bit)&1).astype(bool)
                if 0<fl.sum()<len(fl): out.append((f"a{u['addr']}c{comp}b{bit}",fl))
    return out
def tris_strip(N,skip=None):
    i=np.arange(2,N)
    if skip is not None: i=i[~skip[2:]]
    t=np.stack([i-2,i-1,i],1)
    t[1::2]=t[1::2][:,[1,0,2]] if len(t)>1 else t[1::2]
    return t
def clean(P,T):
    if len(T)==0: return T
    a,b_,c=P[T[:,0]],P[T[:,1]],P[T[:,2]]
    d=(np.all(a==b_,1)|np.all(b_==c,1)|np.all(a==c,1))
    return T[~d]
def quality(P,T,Nrm=None):
    T=clean(P,T)
    if len(T)==0: return 0,0
    a,b_,c=P[T[:,0]],P[T[:,1]],P[T[:,2]]
    e=np.stack([np.linalg.norm(a-b_,axis=1),np.linalg.norm(b_-c,axis=1),np.linalg.norm(c-a,axis=1)],1)
    m=np.median(e)
    if m==0: return 0,0
    cr=np.cross(b_-a,c-a);area=np.linalg.norm(cr,axis=1)
    good=(e.max(1)<5*m)&(area>1e-12*m*m)
    s=good.mean()
    if Nrm is not None:
        fn=cr/area.clip(1e-30)[:,None];vn=(Nrm[T[:,0]]+Nrm[T[:,1]]+Nrm[T[:,2]]);vn/=np.linalg.norm(vn,axis=1).clip(1e-9)[:,None]
        agree=(np.abs((fn*vn).sum(1))>0.5)
        s=0.5*s+0.5*(agree&good).mean();good=good&agree
    return s,good.sum()
def topology(c):
    P=c['pos'][1];N=c['N'];Nrm=c['nrm'][1] if c['nrm'] else None
    cands=[('strip',None)]+[('strip/'+k,f) for k,f in flag_sources(c)]
    res=[]
    for name,fl in cands:
        T=tris_strip(N,fl);q,g=quality(P,T,Nrm);res.append((q,name,T))
    if N%3==0:
        T=np.arange(N).reshape(-1,3);q,g=quality(P,T,Nrm);res.append((q,'list',T))
    res.sort(key=lambda r:-r[0])
    return res
def analyse(path,limit=None):
    b=open(path,'rb').read();st=find_streams(b)
    out=[];rules=collections.Counter();V=[];F=[];base=0;qs=[]
    for bt in batches(b,st):
        c=classify(bt)
        if not c: continue
        res=topology(c);q,name,T=res[0]
        rules[name]+=1;qs.append(q)
        P=c['pos'][1]
        V.append(P);F.append(T+base);base+=len(P)
        out.append(dict(N=c['N'],q=q,rule=name,off=c['pos'][0]['off'],fmt=(c['pos'][0]['vn'],c['pos'][0]['vl'])))
    return dict(batches=out,rules=rules,V=V,F=F,q=np.array(qs))
def write_obj(path,V,F):
    with open(path,'w') as f:
        for P in V:
            for v in P: f.write(f"v {v[0]:.5f} {v[1]:.5f} {v[2]:.5f}\n")
        for T in F:
            for t in T: f.write(f"f {t[0]+1} {t[1]+1} {t[2]+1}\n")
if __name__=='__main__':
    root='/mnt/user-data/uploads/study cases'
    for rel in sys.argv[1:]:
        r=analyse(os.path.join(root,rel))
        nb=len(r['batches']);good=(r['q']>0.9).sum() if nb else 0
        nv=sum(len(v) for v in r['V']);nf=sum(len(f) for f in r['F'])
        print(f"{rel}: batches {nb} q>0.9 {good} verts {nv} tris {nf} mean q {r['q'].mean() if nb else 0:.3f} rules {r['rules'].most_common(4)}")
