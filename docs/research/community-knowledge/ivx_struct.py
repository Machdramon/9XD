import struct,sys,os
def I(b,p,n=1): return struct.unpack_from('<%dI'%n,b,p)
def F(b,p,n=1): return struct.unpack_from('<%df'%n,b,p)
def models(b):
    return [a for a in range(0,len(b)-32,16) if b[a:a+8]==b'0IVX00.1']
def walk(b,a,end):
    p=a+32;ptr=I(b,p,4);p+=16
    count2,reps,_,size1=ptr;p+=size1
    p+=count2*12*4
    out=[]
    for sc in range(200):
        if p+0x100>end:break
        m=I(b,p,8);fl=F(b,p+32,12);loc=fl[0:3];p+=80
        second=None
        if m[0]==0: second=F(b,p+32,12);p+=80
        va=I(b,p,4);p+=16;bounds=F(b,p,8);p+=32
        cont=va[3]
        if cont>512: out.append(('stop',hex(p),va));break
        parts=[]
        ok=True
        for k in range(cont):
            p+=16+32+64
            inter=I(b,p,3);p+=16;p+=32
            reps,size=inter[1],inter[2]*16
            if reps>4096 or p+size>end: ok=False;break
            parts.append((reps,size));p+=size;p+=16
        out.append((sc,[round(x,3) for x in loc],m[0],cont,len(parts),ok))
        if not ok:break
    return out
root='/mnt/user-data/uploads/study cases/'
for rel in sys.argv[1:]:
    b=open(root+rel,'rb').read();ms=models(b)
    print('==',rel,'models',len(ms))
    for i,a in enumerate(ms[:3]):
        end=ms[i+1] if i+1<len(ms) else len(b)
        try:
            r=walk(b,a,end)
        except Exception as e: r=[('err',str(e))]
        print('  model',hex(a),'containers',len(r));[print('    ',x) for x in r[:8]]
