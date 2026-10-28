import os
import struct,sys,os,collections,json
def u32(b,p): return struct.unpack_from('<I',b,p)[0]
def f32(b,p): return struct.unpack_from('<f',b,p)[0]
def models(b):
    out=[];s=0
    while True:
        a=b.find(b'0IVX00.1',s)
        if a<0: break
        s=a+8
        if a%16 or a>len(b)-32 or u32(b,a+8) or u32(b,a+12) or b[a+16:a+20]!=b'LDMX' or b[a+24:a+28]!=b'MRON': continue
        out.append(a)
    return out
def packets(b,base,end):
    a=base+32
    while a+16<=end:
        tag=u32(b,a);kind=(tag>>28)&7;qwc=tag&0xffff;nv=u32(b,a+12)
        if not((kind in(1,7)) and tag==((kind<<28)|qwc) and 1<=qwc<=2048 and u32(b,a+4)==0 and u32(b,a+8)<=1 and 3<=nv<=256):
            a+=16;continue
        pend=a+16+qwc*16
        if pend>end: a+=16;continue
        cl=wl=1;p=a+16;streams=[];cmds=[];ok=True;kick=False
        while p<pend:
            w=u32(b,p);cmd=(w>>24)&127;num=(w>>16)&255;imm=w&0xffff;p+=4;extra=0;cmds.append(cmd)
            if cmd==1: cl=imm&255;wl=imm>>8
            elif cmd==0x20: extra=4
            elif cmd in(0x30,0x31): extra=16
            elif cmd==0x4a: extra=(num or 256)*8
            elif cmd in(0x50,0x51): extra=imm*16
            elif cmd>=0x60:
                comps=((cmd>>2)&3)+1;bits=32>>(cmd&3);n=num or 256
                if cl==0 or wl==0: ok=False;break
                inputs=n if wl<=cl else (n//wl)*cl+min(n%wl,cl)
                extra=((inputs*2+3)//4)*4 if bits==4 else ((inputs*comps*bits+31)//32)*4
                streams.append(dict(cmd=cmd,n=n,imm=imm,cl=cl,wl=wl,data=p,size=extra))
            elif cmd==0x17: kick=True
            elif cmd not in(0,2,3,4,5,6,7,0x10,0x11,0x13,0x14,0x15): ok=False;break
            if extra>pend-p: ok=False;break
            p+=extra
        yield dict(dma=a,end=pend,nv=nv,parity=u32(b,a+8),ok=ok and p==pend,streams=streams,kick=kick,cmds=cmds)
        a=pend
def sig(pk):
    return ' '.join(f"{s['cmd']:02x}/{s['n'] if s['n']!=pk['nv'] else 'N'}@{s['imm']:04x}" + (f"c{s['cl']}w{s['wl']}" if (s['cl'],s['wl'])!=(4,1) else '') for s in pk['streams'])
if __name__=='__main__':
    root=os.environ.get('STUDY_CASES',r'C:\Users\Jarky\Documents\study cases')
    allsig=collections.Counter();per={}
    for dp,dn,fn in os.walk(root):
        for f in sorted(fn):
            p=os.path.join(dp,f);b=open(p,'rb').read()
            ms=models(b)
            if not ms: continue
            rel=os.path.relpath(p,root);per[rel]=[]
            for i,m in enumerate(ms):
                end=ms[i+1] if i+1<len(ms) else len(b)
                ver=b[m+16:m+24].decode()+'/'+b[m+24:m+32].decode()
                c=collections.Counter();n=0;bad=0
                for pk in packets(b,m,end):
                    n+=1
                    if not pk['ok']: bad+=1;continue
                    c[sig(pk)]+=1;allsig[(ver,sig(pk))]+=1
                per[rel].append((hex(m),ver,n,bad,c.most_common(4)))
    for rel,ms in per.items():
        print(rel)
        for m in ms: print('  ',m[0],m[1],'packets',m[2],'bad',m[3]);[print('       ',k,v) for k,v in m[4]]
    print('\n== signature census')
    for (v,s),n in allsig.most_common(40): print(f'{n:6} {v} {s}')
