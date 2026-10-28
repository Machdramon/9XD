import os
import struct,os,collections,math,sys
sys.path.insert(0,os.path.dirname(__file__))
from genki_vif import *
root=os.environ.get('STUDY_CASES',r'C:\Users\Jarky\Documents\study cases')
def vec(b,p,clear=False):
    x,y,z=struct.unpack_from('<fff',b,p)
    if clear:
        yi=struct.unpack_from('<I',b,p+4)[0]&~1;y=struct.unpack('<f',struct.pack('<I',yi))[0]
    return (x,y,z)
def nrm(b,s,i):
    if s['cmd']==0x6a: return struct.unpack_from('<bbb',b,s['data']+i*3)
    if s['cmd']==0x6d: return struct.unpack_from('<hhh',b,s['data']+i*8)
    if s['cmd']==0x6e: return None
def evaluate(path,rule):
    b=open(os.path.join(root,path),'rb').read();ms=models(b)
    agree=dis=deg=faces=0;skipped=0;longest=[]
    for mi,m in enumerate(ms):
        end=ms[mi+1] if mi+1<len(ms) else len(b)
        for pk in packets(b,m,end):
            s=pk['streams'];n=pk['nv'];par=pk['parity']
            pos=[x for x in s if x['cmd']==0x68][0]['data']
            ns=[x for x in s if x['cmd'] in(0x6a,0x6d) and x['n']==n]
            yb=[u32(b,pos+i*12+4)&1 for i in range(n)]
            P=[vec(b,pos+i*12,rule=='ybit') for i in range(n)]
            N=[nrm(b,ns[0],i) for i in range(n)] if ns else None
            for i in range(2,n):
                if rule=='ybit' and yb[i]: skipped+=1;continue
                a,d,c=i-2,i-1,i
                if ((i-2)&1)^par: a,d=d,a
                u=[P[d][k]-P[a][k] for k in range(3)];v=[P[c][k]-P[a][k] for k in range(3)]
                fx=u[1]*v[2]-u[2]*v[1];fy=u[2]*v[0]-u[0]*v[2];fz=u[0]*v[1]-u[1]*v[0]
                L=math.sqrt(fx*fx+fy*fy+fz*fz)
                if L<=1e-10: deg+=1;continue
                faces+=1
                if N:
                    sx=sum(N[j][0] for j in(a,d,c));sy=sum(N[j][1] for j in(a,d,c));sz=sum(N[j][2] for j in(a,d,c))
                    dot=fx*sx+fy*sy+fz*sz
                    if dot>0: agree+=1
                    elif dot<0: dis+=1
    tot=agree+dis
    return dict(rule=rule,faces=faces,degenerate=deg,suppressed=skipped,withNormals=tot,agree=round(agree/tot,4) if tot else None)
for p in ['c1 ps2/000004b8.dat','txrd2/car/000002a0.sdb','kaido racer/000004ac.dat','txr3/000000c4.nfc','wm ps2/000000a3.0iv','txr zero/0x000000ba.dat','txrd1/00001363.dat']:
    for r in ['plain','ybit']:
        print(p,evaluate(p,r))
