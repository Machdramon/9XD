import numpy as np, os, sys, collections
PSM={0:'CT32',1:'CT24',2:'CT16',0xA:'CT16S',0x13:'T8',0x14:'T4',0x1B:'T8H',0x24:'T4HL',0x2C:'T4HH',0x30:'Z32',0x31:'Z24',0x32:'Z16',0x3A:'Z16S'}
def tex0(v):
    v=int(v)
    return dict(tbp=v&0x3fff,tbw=(v>>14)&63,psm=(v>>20)&63,tw=(v>>26)&15,th=(v>>30)&15,tcc=(v>>34)&1,tfx=(v>>35)&3,cbp=(v>>37)&0x3fff,cpsm=(v>>51)&15,csm=(v>>55)&1,csa=(v>>56)&31,cld=(v>>61)&7)
def valid_tex0(t):
    return t['psm'] in PSM and t['psm']<0x30 and 1<=t['tw']<=10 and 1<=t['th']<=10 and 1<=t['tbw']<=32 and t['cpsm'] in (0,2,0xA) and (t['psm'] in (0x13,0x14,0x1B,0x24,0x2C) or t['cbp']==0 or True)
def bitblt(v):
    v=int(v);return dict(sbp=v&0x3fff,sbw=(v>>16)&63,spsm=(v>>24)&63,dbp=(v>>32)&0x3fff,dbw=(v>>48)&63,dpsm=(v>>56)&63)
def scan(b):
    out=collections.defaultdict(list)
    for off in (0,4,8,12):
        n=(len(b)-off)//16
        if n<=0: continue
        a=np.frombuffer(b,dtype='<u8',count=n*2,offset=off).reshape(n,2)
        for reg in (6,7,0x50):
            idx=np.nonzero(a[:,1]==reg)[0]
            for i in idx:
                v=a[i,0]
                if reg in(6,7):
                    t=tex0(v)
                    if valid_tex0(t): out['TEX0'].append((off+i*16,t))
                else:
                    t=bitblt(v)
                    if t['dbw'] and t['dpsm'] in PSM: out['BITBLT'].append((off+i*16,t))
    return out
def tim2(b):
    res=[];s=0
    while True:
        p=b.find(b'TIM2',s)
        if p<0:break
        s=p+4
        if p+16>len(b):break
        cnt=int.from_bytes(b[p+6:p+8],'little');q=p+16 if b[p+5]==0 else p+128
        for k in range(min(cnt,64)):
            if q+48>len(b):break
            tot=int.from_bytes(b[q:q+4],'little');w=int.from_bytes(b[q+0x10:q+0x12],'little');h=int.from_bytes(b[q+0x12:q+0x14],'little')
            g=int.from_bytes(b[q+0x18:q+0x20],'little')
            res.append((p,q,w,h,tex0(g)))
            if tot<48:break
            q+=tot
    return res
if __name__=='__main__':
    root='/mnt/user-data/uploads/study cases'
    for g in sys.argv[1:]:
        tot=collections.Counter();ex={}
        for dp,dn,fn in os.walk(os.path.join(root,g)):
            for f in fn:
                p=os.path.join(dp,f);b=open(p,'rb').read();r=scan(b);t=tim2(b)
                rel=os.path.relpath(p,root)
                print(f"{rel}: TEX0 {len(r['TEX0'])} BITBLT {len(r['BITBLT'])} TIM2pics {len(t)}")
                for off,t0 in r['TEX0'][:3]: print('   tex0',hex(off),{k:t0[k] for k in('tbp','tbw','psm','tw','th','cbp','cpsm')})
                for q in t[:2]: print('   tim2',hex(q[1]),q[2],'x',q[3],{k:q[4][k] for k in('tbp','tbw','psm','tw','th','cbp','cpsm')})
