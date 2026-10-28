import struct, numpy as np
SIMPLE={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x10,0x11,0x13,0x14,0x15,0x17}
FMT={0:'S',1:'V2',2:'V3',3:'V4'}
BITS={0:32,1:16,2:8,3:5}
def unpack_size(cmd,num,cl,wl):
    vn=(cmd>>2)&3;vl=cmd&3;n=num or 256
    inputs=n if wl<=cl else (n//wl)*cl+min(n%wl,cl)
    if vl==3: return ((inputs*2+3)//4)*4
    return ((inputs*(vn+1)*BITS[vl]+31)//32)*4, inputs
def code_len(b,p,end,cl,wl):
    """returns (kind,extra,info) or None if invalid at p"""
    w=struct.unpack_from('<I',b,p)[0];cmd=(w>>24)&0x7f;num=(w>>16)&0xff;imm=w&0xffff
    if cmd>=0x60:
        if cmd&3==3 and (cmd>>2)&3!=3: return None   # only V4-5 uses 5-bit
        if cl==0 or wl==0: return None
        r=unpack_size(cmd,num,cl,wl)
        size=r if isinstance(r,int) else r[0]
        if (imm&0x3ff)+((num or 256) if wl<=cl else 0)>1024+256: return None
        return ('U',size,(cmd,num or 256,imm))
    if cmd in SIMPLE:
        if cmd in (0x00,0x10,0x11,0x13) and (num or imm) and cmd!=0x00: return None
        if cmd==0x00 and num: return None
        if cmd==0x01 and not (1<=(imm&0xff)<=16 and 1<=(imm>>8)<=16): return None
        if cmd==0x05 and imm>3: return None
        if cmd in(0x02,0x03,0x04) and imm>0x3ff: return None
        return ('C',0,(cmd,num,imm))
    if cmd==0x20: return ('C',4,(cmd,num,imm))
    if cmd in(0x30,0x31): return ('C',16,(cmd,num,imm))
    if cmd==0x4a: return ('M',(num or 256)*8,(cmd,num,imm))
    if cmd in(0x50,0x51): return ('D',(imm or 65536)*16,(cmd,num,imm))
    return None
def is_dmatag(b,p,end):
    if p+16>end: return None
    w0,w1=struct.unpack_from('<II',b,p)
    tid=(w0>>28)&7;qwc=w0&0xffff
    if w0&0x8fff0000!=(tid<<28) : return None
    if tid not in(1,2,6,7) : return None
    if tid in(1,6,7) and w1!=0: return None
    if p+16+qwc*16>end: return None
    return tid,qwc
def walk(b,start,end,maxlen=1<<24,dma=True):
    """Walk a VIF stream from start. Returns (stop,events)."""
    p=start;cl=wl=1;ev=[];base=start
    while p+4<=end and p-start<maxlen:
        if dma and (p-base)%16==0:
            t=is_dmatag(b,p,end)
            if t and t[0]!=0:
                w0=struct.unpack_from('<I',b,p)[0]
                # DMA tags look like FLUSHE/UNPACK codes; accept as tag when qwc>0 or RET/END
                if (w0>>24)&0x7f not in (0x00,) :
                    ev.append(('DMA',p,t));p+=16;continue
        r=code_len(b,p,end,cl,wl)
        if r is None: break
        kind,extra,info=r
        if p+4+extra>end: break
        if kind=='C' and info[0]==0x01: cl=info[2]&0xff;wl=info[2]>>8
        ev.append((kind,p,info,cl,wl,p+4,extra))
        p+=4+extra
    return p,ev
def decode_unpack(b,cmd,n,imm,cl,wl,data):
    vn=((cmd>>2)&3)+1;vl=cmd&3;usn=(imm>>14)&1
    inputs=n if wl<=cl else (n//wl)*cl+min(n%wl,cl)
    if vl==0: a=np.frombuffer(b,'<u4',inputs*vn,data).reshape(inputs,vn)
    elif vl==1: a=np.frombuffer(b,'<u2' if usn else '<i2',inputs*vn,data).reshape(inputs,vn)
    elif vl==2: a=np.frombuffer(b,'u1' if usn else 'i1',inputs*vn,data).reshape(inputs,vn)
    else:
        v=np.frombuffer(b,'<u2',inputs,data).astype(np.uint32)
        a=np.stack([(v&31)<<3,((v>>5)&31)<<3,((v>>10)&31)<<3,(v>>15)<<7],1)
    return a
