import os,sys,json,time
from extract2 import analyse
from render import render
root='/mnt/user-data/uploads/study cases'
games=sys.argv[1:]
rows=[]
for g in games:
    for dp,dn,fn in os.walk(os.path.join(root,g)):
        for f in sorted(fn):
            p=os.path.join(dp,f);rel=os.path.relpath(p,root)
            if os.path.getsize(p)>12_000_000 or f.endswith(('.tmr','.txt')): continue
            t=time.time()
            try: r=analyse(p)
            except Exception as e: print(rel,'ERR',e);continue
            nv=sum(len(v) for v in r['V']);nf=sum(len(x) for x in r['F'])
            kept=sum(x['kept'] for x in r['report'])
            top=sorted(r['report'],key=lambda x:-x['tris'])[:1]
            rows.append(dict(file=rel,batches=r['batches'],kept=kept,verts=nv,tris=nf,rule=top[0]['rule'] if top else '',pos=top[0]['pos'] if top else '',sec=round(time.time()-t,1)))
            if nf: render(r['V'],r['F'],'out/'+rel.replace('/','_').replace(' ','_')+'.png',rel,r['M'])
            print(rows[-1],flush=True)
json.dump(rows,open('corpus.json','w'),indent=1)
