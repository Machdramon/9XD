import os,sys,collections
import numpy as np
from vif import *
from census import find_streams
from extract import batches,classify,flag_sources,tris_strip,quality,clean
def sig(c): return tuple(sorted((u['addr'],u['vn'],u['vl'],u['usn']) for u in c['streams']))
def rules_for(c):
    out={'strip':tris_strip(c['N'])}
    for k,f in flag_sources(c):
        out['strip/'+k]=tris_strip(c['N'],f)
        out['strip/!'+k]=tris_strip(c['N'],~f)
    if c['N']%3==0: out['list']=np.arange(c['N']).reshape(-1,3)
    return out
def analyse(path,qmin=0.75):
    b=open(path,'rb').read();st=find_streams(b)
    cs=[c for c in (classify(bt) for bt in batches(b,st)) if c]
    groups=collections.defaultdict(list)
    for c in cs: groups[sig(c)].append(c)
    V=[];F=[];M=[];base=0;report=[];acc=[]
    for s,g in groups.items():
        allP=np.concatenate([c['pos'][1] for c in g]);diag=np.linalg.norm(np.ptp(allP,axis=0))
        steps=np.concatenate([np.linalg.norm(np.diff(c['pos'][1],axis=0),axis=1) for c in g])
        if diag==0 or np.median(steps)/diag>0.08:
            report.append(dict(sig=s,batches=len(g),kept=0,rule='incoherent',tris=0,pos=''));continue
        # position stream: same addr across group -> choose by largest summed spread
        score=collections.Counter();per=[]
        for c in g:
            Nrm=c['nrm'][1] if c['nrm'] else None
            r={}
            for name,T in rules_for(c).items():
                T=clean(c['pos'][1],T);q,good=quality(c['pos'][1],T,Nrm);r[name]=(q,T);score[name]+=good-3*(len(T)-good)
            per.append(r)
        best=max(score,key=lambda k:(score[k]+(0.01*score['strip'] if k=='strip' else 0)))
        kept=[]
        for c,r in zip(g,per):
            q,T=r.get(best,(0,None))
            if T is None or q<qmin: continue
            kept.append((c,T))
        u=g[0]['pos'][0]
        scale=float(np.median(np.abs(allP)))+1e-12
        acc.append(dict(sig=s,batches=len(g),kept=kept,rule=best,tris=sum(len(t) for _,t in kept),scale=scale,pos=f"a{u['addr']} V{u['vn']}-{[32,16,8,5][u['vl']]}"))
    rets=ret_offsets(b)
    if acc:
        ref=max(acc,key=lambda a:a['tris'])['scale']
        for a in acc:
            ok=0.05<a['scale']/ref<20
            if not ok: a['rule']+=' (other scale, dropped)'
            for c,T in (a['kept'] if ok else []):
                P=c['pos'][1];V.append(P);F.append(T+base);base+=len(P)
                off=c['pos'][0]['off'];M.append(int(np.searchsorted(rets,off)))
            report.append(dict(sig=a['sig'],batches=a['batches'],kept=len(a['kept']) if ok else 0,rule=a['rule'],tris=a['tris'] if ok else 0,pos=a['pos']))
    return dict(V=V,F=F,M=M,rets=rets,report=report,batches=len(cs))
def ret_offsets(b):
    a=np.frombuffer(b,'<u4',len(b)//16*4).reshape(-1,4)
    idx=np.nonzero((a[:,0]==0x60000000)&(a[:,1]==0)&(a[:,2]==0)&(a[:,3]==0))[0]
    return idx*16
if __name__=='__main__':
    from render import render
    root='/mnt/user-data/uploads/study cases'
    for rel in sys.argv[1:]:
        r=analyse(os.path.join(root,rel))
        nv=sum(len(v) for v in r['V']);nf=sum(len(f) for f in r['F'])
        print(f"== {rel}: batches {r['batches']} verts {nv} tris {nf}")
        for g in sorted(r['report'],key=lambda g:-g['batches'])[:5]: print('   ',g['batches'],'kept',g['kept'],g['rule'],g['pos'],'tris',g['tris'])
        render(r['V'],r['F'],'out/'+rel.replace('/','_').replace(' ','_')+'.png',rel)
