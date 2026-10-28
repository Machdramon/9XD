#pragma once
// Initial D Special Stage (PS2) PAC archives: SMD models (compressed) and GIM textures.
// Format notes: docs/research/initial-d-ss/README.md. Independent implementation from that description.
//   PAC: "PAC\0", u32 count, u32, u32 table offset; 32-byte entries {name[16], offset, size, type, flags}.
//        type 1 = GIM texture, type 3 = SMD model; flags bit 0 = compressed.
//   Compression: {u32 id, u32 raw size, u32 packed size}; LSB-first flags fetched lazily (bit 1 = literal),
//        references are 24-bit {u16 absolute window position, u8 length - 4}; 64 KiB zeroed window from 0xFEFD.
//   SMD: "SMD\0", "0.00", u32 triangles, u32 vertices, u32 textures, u32 records, u32 name table, u32 record table,
//        bbox; 16-byte texture names; 32-byte records {offset, qword count, texture, -1, 0, triangles, vertices, 0}
//        pointing at VIF packets: V4-32 position (W bit 15 = no kick; GIF PRIM=triangle strip), V2-32 UV, V4-8 colour, per MSCAL batch.
//   GIM: "GIM\0", name[16]; +0x30 u16 width, height, u8 GS psm (0x13 T8 / 0x14 T4), transfer width/height (u16,
//        at +0x36/+0x38), u32 pixel offset (+0x3C); +0x40 palette width/height, u32 palette offset (+0x4C).
//        8-bit images uploaded as a half-size PSMCT32 rectangle are GS-swizzled and are unswizzled here.
#include "ps2_gs_address.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace IdssCore {
inline uint32_t le32(const uint8_t *d,int64_t p){return uint32_t(d[p])|uint32_t(d[p+1])<<8|uint32_t(d[p+2])<<16|uint32_t(d[p+3])<<24;}
inline uint16_t le16(const uint8_t *d,int64_t p){return uint16_t(d[p]|d[p+1]<<8);}
inline float lef(const uint8_t *d,int64_t p){uint32_t v=le32(d,p);float f;std::memcpy(&f,&v,4);return f;}

struct Entry { std::string name; int64_t offset=0,size=0; uint32_t type=0,flags=0; };
inline std::vector<Entry> pac(const uint8_t *d,int64_t size){
    std::vector<Entry> out;if(size<16||std::memcmp(d,"PAC\0",4))return out;const uint32_t n=le32(d,4),table=le32(d,12);
    if(n<1||n>65536||table<16||table+int64_t(n)*32>size)return {};
    for(uint32_t i=0;i<n;++i){const int64_t p=table+32*int64_t(i);Entry e;for(int k=0;k<16&&d[p+k];++k)e.name+=char(d[p+k]);e.offset=le32(d,p+16);e.size=le32(d,p+20);e.type=le32(d,p+24);e.flags=le32(d,p+28);
        if(e.offset<0||e.size<0||e.offset+e.size>size)return {};out.push_back(e);}
    return out;
}
inline bool decompress(const uint8_t *d,int64_t size,std::vector<uint8_t> &out,int64_t maxRaw=64<<20){
    if(size<12)return false;const uint32_t raw=le32(d,4),packed=le32(d,8);if(raw>maxRaw||int64_t(packed)>size-12)return false;
    const uint8_t *s=d+12;std::vector<uint8_t> win(0x10000,0);uint32_t pos=0xFEFD,flags=0;int64_t p=0;out.clear();out.reserve(raw);
    while(out.size()<raw){
        flags>>=1;if(!(flags&0x100)){if(p>=packed)return false;flags=s[p++]|0xFF00u;}
        if(flags&1){if(p>=packed)return false;const uint8_t c=s[p++];out.push_back(c);win[pos]=c;pos=(pos+1)&0xFFFF;}
        else{if(p+3>packed)return false;const uint32_t w=s[p]|s[p+1]<<8|s[p+2]<<16;p+=3;const uint32_t at=w&0xFFFF,len=(w>>16)+4;
            for(uint32_t i=0;i<len&&out.size()<raw;++i){const uint8_t c=win[(at+i)&0xFFFF];out.push_back(c);win[pos]=c;pos=(pos+1)&0xFFFF;}}
    }
    return true;
}
struct Vertex { std::array<float,3> position{}; std::array<float,2> uv{}; };
struct Record { int texture=-1; std::vector<Vertex> vertices; std::vector<std::array<uint32_t,3>> triangles; uint32_t declaredTriangles=0; };
struct Model { std::vector<std::string> textures; std::vector<Record> records; uint32_t triangles=0,vertices=0; std::string error; };
inline Model smd(const uint8_t *d,int64_t size){
    Model m;auto fail=[&](const char *e){m.error=e;m.records.clear();return m;};
    if(size<0x40||std::memcmp(d,"SMD\0",4))return fail("Not an SMD model.");
    m.triangles=le32(d,8);m.vertices=le32(d,12);const uint32_t nt=le32(d,16),nr=le32(d,20),names=le32(d,24),table=le32(d,28);
    if(nt>4096||nr>65536||names+int64_t(nt)*16>size||table+int64_t(nr)*32>size)return fail("Invalid SMD tables.");
    for(uint32_t i=0;i<nt;++i){std::string s;for(int k=0;k<16&&d[names+16*i+k];++k)s+=char(d[names+16*i+k]);m.textures.push_back(s);}
    for(uint32_t r=0;r<nr;++r){
        const int64_t q=table+32*int64_t(r);const int64_t off=le32(d,q),bytes=int64_t(le32(d,q+4))*16;Record rec;rec.texture=int(le32(d,q+8));rec.declaredTriangles=le32(d,q+20);
        if(off+bytes>size)return fail("SMD record outside the model.");
        std::vector<std::array<float,3>> pos;std::vector<uint32_t> w;std::vector<std::array<float,2>> uv;
        auto flush=[&]{const uint32_t base=uint32_t(rec.vertices.size());for(size_t i=0;i<pos.size();++i){Vertex v;v.position=pos[i];if(i<uv.size())v.uv=uv[i];rec.vertices.push_back(v);}
            for(size_t i=2;i<pos.size();++i){if(w[i]&0x8000)continue;uint32_t a=base+uint32_t(i-2),b=base+uint32_t(i-1);if(i&1)std::swap(a,b);/* Standard GIF triangle-strip parity; ADC suppresses a kick, not a strip vertex. */rec.triangles.push_back({a,b,base+uint32_t(i)});}
            pos.clear();w.clear();uv.clear();};
        for(int64_t p=off,end=off+bytes;p+4<=end;){
            const uint32_t code=le32(d,p);const int cmd=(code>>24)&0x7F,num=(code>>16)&0xFF;
            if(cmd>=0x60){const int vn=(cmd>>2)&3,vl=cmd&3,n=num?num:256;const int64_t len=(int64_t(32>>vl)*(vn+1)*n+31)/32*4,a=p+4;if(a+len>end)break;
                if(cmd==0x6C&&n>1){for(int i=0;i<n;++i){pos.push_back({lef(d,a+16*i),lef(d,a+16*i+4),lef(d,a+16*i+8)});w.push_back(le32(d,a+16*i+12));}}
                else if((cmd&~0x10)==0x64){for(int i=0;i<n;++i)uv.push_back({lef(d,a+8*i),lef(d,a+8*i+4)});}
                p=a+len;continue;}
            if(cmd==0x14||cmd==0x15||cmd==0x17)flush();
            p+=cmd==0x20?8:(cmd==0x30||cmd==0x31)?20:4;
        }
        flush();m.records.push_back(std::move(rec));
    }
    return m;
}
struct Image { int width=0,height=0; std::vector<uint32_t> rgba; std::string error; }; // rgba packed 0xAARRGGBB
// 8-bit pixels uploaded as a PSMCT32 rectangle of half width/height are stored in GS column order.
inline uint8_t swizzled8(const uint8_t *s,int x,int y,int w){
    const int block=(y&~0xF)*w+(x&~0xF)*2,swap=(((y+2)>>2)&1)*4,row=(((y&~3)>>1)+(y&1))&7;
    return s[block+row*w*2+((x+swap)&7)*4+((y>>1)&1)+((x>>2)&2)];}
inline Image gim(const uint8_t *d,int64_t size){
    Image im;auto fail=[&](const char *e){im.error=e;im.rgba.clear();return im;};
    if(size<0x50||std::memcmp(d,"GIM\0",4))return fail("Not a GIM texture.");
    const int w=le16(d,0x30),h=le16(d,0x32),psm=d[0x34],tw=le16(d,0x36);const int th=le16(d,0x38),transferPsm=le16(d,0x3A);const int64_t px=le32(d,0x3C),pal=le32(d,0x4C);
    if(w<1||h<1||w>4096||h>4096||(psm!=0x13&&psm!=0x14))return fail("Unsupported GIM layout.");
    const int colours=psm==0x13?256:16;const int64_t pixelBytes=psm==0x13?int64_t(w)*h:(int64_t(w)*h+1)/2;
    if(px+pixelBytes>size||pal+int64_t(colours)*4>size)return fail("GIM data is truncated.");
    const bool swizzled=transferPsm==0;
    std::vector<uint8_t> indices(size_t(w)*h);
    if(swizzled){
        if(tw<1||th<1||int64_t(tw)*th*4!=pixelBytes||px+int64_t(tw)*th*4>size)return fail("Invalid GIM PSMCT32 transfer dimensions.");
        std::vector<uint8_t> memory(4*1024*1024,0),written(memory.size(),0);
        for(int y=0;y<th;++y)for(int x=0;x<tw;++x){auto a=Ps2Gs::bitAddress(x,y,tw,32)/8,source=px+int64_t(y*tw+x)*4;if(a<0||a+4>int64_t(memory.size()))return fail("GIM transfer exceeds GS storage.");for(int k=0;k<4;++k){memory[a+k]=d[source+k];written[a+k]=1;}}
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto a=Ps2Gs::bitAddress(x,y,w,psm==0x13?8:4);if(a<0||a/8>=int64_t(memory.size())||!written[a/8])return fail("GIM texture reads unwritten GS storage.");indices[size_t(y)*w+x]=(memory[a/8]>>(a%8))&((1<<(psm==0x13?8:4))-1);}
    }else{
        if(transferPsm!=psm||tw!=w||th!=h)return fail("Unsupported GIM transfer layout.");
        for(int64_t i=0;i<int64_t(w)*h;++i)indices[i]=psm==0x14?(d[px+i/2]>>(4*(i&1)))&15:d[px+i];
    }
    std::vector<uint32_t> palette(colours);
    for(int i=0;i<colours;++i){const int src=psm==0x13?((i&~24)|((i&8)<<1)|((i&16)>>1)):i;const uint8_t *c=d+pal+4*src;palette[i]=uint32_t(std::min(255,c[3]*2))<<24|uint32_t(c[0])<<16|uint32_t(c[1])<<8|c[2];}
    im.width=w;im.height=h;im.rgba.resize(size_t(w)*h);
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){int v;const int64_t i=int64_t(y)*w+x;
        v=indices[i];
        im.rgba[i]=palette[v];}
    return im;
}
}
