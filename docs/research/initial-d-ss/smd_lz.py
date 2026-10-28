import struct
def decompress(b):
    raw=struct.unpack_from('<I',b,4)[0];src=b[12:];win=bytearray(0x10000);pos=0xFEFD;out=bytearray();p=0;flags=0
    while len(out)<raw:
        flags>>=1
        if not flags&0x100:flags=src[p]|0xFF00;p+=1
        if flags&1:c=src[p];p+=1;out.append(c);win[pos]=c;pos=(pos+1)&0xFFFF
        else:
            w=src[p]|src[p+1]<<8|src[p+2]<<16;p+=3;off=w&0xFFFF
            for i in range((w>>16)+4):c=win[(off+i)&0xFFFF];out.append(c);win[pos]=c;pos=(pos+1)&0xFFFF
    return bytes(out[:raw]),p
