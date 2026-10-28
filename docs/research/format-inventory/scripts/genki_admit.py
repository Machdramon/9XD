import os
import struct,os,collections,math,sys
sys.path.insert(0,os.path.dirname(__file__))
from genki_vif import *
def f(b,p): return struct.unpack_from('<f',b,p)[0]
def c1(b,pk):
    s=pk['streams'];n=pk['nv'];par=pk['parity']
    if not pk['kick'] or len(s)<3: return 'kick/streams'
    c,p=s[0],s[1]
    if not(c['cmd']==0x6c and c['n']==2 and c['imm']==0x8000 and c['cl']==2 and c['wl']==2 and p['cmd']==0x68 and p['imm']==0x8002): return 'layout'
    if u32(b,c['data']+28)!=(0x8000|n) or u32(b,c['data']+12)!=(0xbf800000 if par else 0x3f800000): return 'count/parity'
    if not(u32(b,p['data']+4)&1 and u32(b,p['data']+16)&1): return 'first-two'
    last=1;color=False
    for a in s[2:]:
        slot=2 if a['cmd']==0x65 and a['imm']==0x8003 else 3 if a['cmd']==0x6a and a['imm']==0x8004 else 4 if a['cmd'] in(0x62,0x6e) and a['imm']==0xc005 else -1
        if slot<=last: return f"optional {a['cmd']:02x}@{a['imm']:04x}"
        last=slot;color|=slot==4
    if not color: return 'no-final-color'
    return 'ok'
def gif(b,pk):
    s=pk['streams'];n=pk['nv'];par=pk['parity']
    if not pk['kick'] or len(s)<2: return 'kick'
    c,p=s[0],s[1]
    if not(c['cmd']==0x6c and c['n']==4 and c['imm']==0x8000 and c['cl']==4 and c['wl']==4 and p['cmd']==0x68 and p['imm']==0x8004): return 'layout'
    w=lambda k:u32(b,c['data']+k)
    tag=struct.unpack_from('<Q',b,c['data']+48)[0];clip=struct.unpack_from('<Q',b,c['data'])[0]
    if (tag&0xffff)!=(0x8000|n) or ((tag>>46)&1)!=1 or ((tag>>47)&7)!=4 or (tag>>60)!=3 or (w(56)&0xfff)!=0x412 or clip!=(tag|(1<<47)) or w(8)!=w(56) or w(12)!=w(60) or w(60)!=(0xbf800001 if par else 0x3f800000): return 'tag'
    if w(28)!=0x3f800000 or w(44)!=0x3f800000: return 'bounds-ctl'
    for k in range(3):
        lo=f(b,c['data']+16+k*4);hi=f(b,c['data']+32+k*4)
        for i in range(n):
            v=f(b,p['data']+i*12+k*4)
            if not(lo-.001<=v<=hi+.001): return 'out-of-bounds'
    prev=1
    for a in s[2:]:
        slot=2 if a['cmd']==0x64 and a['imm']==0x8005 else 3 if a['cmd']==0x6d and a['imm']==0x8006 else 4 if a['cmd']==0x6e and a['imm']==0xc007 else -1
        if slot<=prev: return f"optional {a['cmd']:02x}@{a['imm']:04x}"
        prev=slot
    return 'ok'
def kaido2(b,pk):
    # candidate: C1-style 2-vector control with any ordered optional streams
    s=pk['streams'];n=pk['nv'];par=pk['parity']
    if not pk['kick'] or len(s)<2: return 'kick'
    c,p=s[0],s[1]
    if not(c['cmd']==0x6c and c['n']==2 and c['imm']==0x8000 and c['cl']==2 and p['cmd']==0x68 and p['imm']==0x8002): return 'layout'
    if u32(b,c['data']+28)!=(0x8000|n): return 'count'
    if u32(b,c['data']+12)!=(0xbf800000 if par else 0x3f800000): return 'parity'
    if not(u32(b,p['data']+4)&1 and u32(b,p['data']+16)&1): return 'first-two'
    return 'ok'
if __name__=='__main__':
    root=os.environ.get('STUDY_CASES',r'C:\Users\Jarky\Documents\study cases')
    for dp,dn,fn in os.walk(root):
        for fname in sorted(fn):
            path=os.path.join(dp,fname);b=open(path,'rb').read();ms=models(b)
            if not ms: continue
            print(os.path.relpath(path,root),'size',hex(len(b)))
            for i,m in enumerate(ms):
                end=ms[i+1] if i+1<len(ms) else len(b)
                pk=list(packets(b,m,end));ver=b[m+16:m+24].decode()
                r={'c1':collections.Counter(),'gif':collections.Counter(),'2vec':collections.Counter()}
                for k in pk:
                    r['c1'][c1(b,k)]+=1;r['gif'][gif(b,k)]+=1;r['2vec'][kaido2(b,k)]+=1
                last=pk[-1] if pk else None
                kinds=collections.Counter((u32(b,k['dma'])>>28)&7 for k in pk)
                gap=end-(last['end'] if last else m)
                print(f"   model {hex(m)}-{hex(end)} {ver} packets={len(pk)} dmaKinds={dict(kinds)} lastPacketEnd={hex(last['end']) if last else None} tailBytes={gap}")
                for name,cn in r.items(): print('      ',name,dict(cn.most_common(5)))
