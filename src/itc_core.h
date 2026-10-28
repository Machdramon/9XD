#pragma once
// Import Tuner Challenge (Xbox 360) model sections, as found in the study-case .sdb track files.
// File wrapper (little endian): u32 count, u32 offsets[count]; one section is the big-endian model, one is an
// XPR2 texture package. Model header: +12 mesh count, +28 mesh records (0x220 bytes each), +32 first element
// segment, +64 base for index offsets. Mesh record: name; +240 three stream descriptors {count, type, stride,
// bytes}; +384 three stream offsets. Stream types: 11 float3 position, 13 half4 normal, 14/15/16 RGBA8 colour
// followed by 1/2/3 half2 UV sets. Segment: {element count, segment length, 8 bytes}, then 0x200-byte elements:
// +0x80 texture layer count, +0xB0 first texture index (local XPR2 entry when in range, else a shared/common
// texture), +0x150 index count, +0x178 index offset; u16 triangle strips with 0xFFFF restart.
// Layout from the community MRP scripts (dualmaybe/fixitfelix, read only) and validated against both samples.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace ItcCore {
inline uint32_t be32(const uint8_t *d,int64_t p){return uint32_t(d[p])<<24|uint32_t(d[p+1])<<16|uint32_t(d[p+2])<<8|uint32_t(d[p+3]);}
inline uint16_t be16(const uint8_t *d,int64_t p){return uint16_t(d[p]<<8|d[p+1]);}
inline uint32_t le32(const uint8_t *d,int64_t p){return uint32_t(d[p])|uint32_t(d[p+1])<<8|uint32_t(d[p+2])<<16|uint32_t(d[p+3])<<24;}
inline float bef(const uint8_t *d,int64_t p){uint32_t v=be32(d,p);float f;std::memcpy(&f,&v,4);return f;}
inline float half(uint16_t h){const int e=(h>>10)&31,m=h&1023;float v=e==0?std::ldexp(float(m),-24):e==31?0.f:std::ldexp(1.f+m/1024.f,e-15);return (h&0x8000)?-v:v;}
inline constexpr int MeshRecord=0x220,ElementSize=0x200;

struct Stream { int type=0,stride=0; int64_t offset=-1; };
struct Element { int layers=0,texture=-1; int64_t indexData=0,indexCount=0; };
struct Mesh { std::string name; int vertexCount=0,uvSets=0; Stream position,colourUv,normal; std::vector<Element> elements; };
struct File { int64_t model=-1,modelEnd=0,xpr2=-1; std::vector<Mesh> meshes; std::string error; };

struct Reader {
    const uint8_t *d; int64_t size; File file;
    bool model(int64_t base,int64_t end){
        if(end-base<0x80)return false;
        auto u=[&](int64_t p){return be32(d,base+p);};
        const int64_t span=end-base;const uint32_t meshes=u(12),records=u(28),segments=u(32),indexBase=u(64);
        if(meshes<1||meshes>4096||records<0x44||records+int64_t(meshes)*MeshRecord>span||segments>=span||indexBase>=span)return false;
        std::vector<Mesh> out;int64_t segment=segments;
        for(uint32_t i=0;i<meshes;++i){
            const int64_t r=records+int64_t(i)*MeshRecord;Mesh m;
            for(int64_t p=r;p<r+64&&d[base+p];++p)m.name+=char(d[base+p]);
            m.vertexCount=int(u(r+240));if(m.vertexCount<3||m.vertexCount>1000000)return false;
            for(int k=0;k<3;++k){
                const int64_t s=r+240+16*k;const uint32_t count=u(s),type=u(s+4),stride=u(s+8),bytes=u(s+12),offset=u(r+384+4*k);
                if(count!=uint32_t(m.vertexCount)||int64_t(stride)*count!=bytes||offset>=span||bytes>span-offset)return false;
                Stream st{int(type),int(stride),int64_t(offset)};
                if(type==11&&stride==12)m.position=st;
                else if(type>=14&&type<=16&&stride==4+4*(type-13)){m.colourUv=st;m.uvSets=int(type)-13;}
                else if(type==13&&stride==16)m.normal=st;
            }
            if(m.position.offset<0)return false;
            if(segment+16>span)return false;
            const uint32_t elements=u(segment),length=u(segment+4);
            if(elements<1||elements>4096||length<16+int64_t(elements)*ElementSize||length>span-segment)return false;
            for(uint32_t e=0;e<elements;++e){
                const int64_t q=segment+16+int64_t(e)*ElementSize;Element x;
                x.layers=int(u(q+0x80));const uint32_t texture=u(q+0xB0);x.texture=x.layers>0&&texture!=0xFFFFFFFFu?int(texture):-1;
                x.indexCount=u(q+0x150);const int64_t at=int64_t(u(q+0x178))+indexBase;
                if(x.indexCount<3||at+x.indexCount*2>span)return false;
                x.indexData=base+at;
                for(int64_t k=0;k<x.indexCount;++k){const auto v=be16(d,x.indexData+2*k);if(v!=0xFFFF&&v>=m.vertexCount)return false;}
                m.elements.push_back(x);
            }
            m.position.offset+=base;if(m.colourUv.offset>=0)m.colourUv.offset+=base;if(m.normal.offset>=0)m.normal.offset+=base;
            segment+=length;out.push_back(std::move(m));
        }
        file.model=base;file.modelEnd=end;file.meshes=std::move(out);return true;
    }
};
inline File read(const uint8_t *d,int64_t size){
    Reader r{d,size,{}};if(size<0x100)return r.file;
    // Wrapped .sdb: small little-endian offset table.
    const uint32_t count=le32(d,0);
    if(count>=1&&count<=16&&4+4*int64_t(count)<=size){
        std::vector<int64_t> at;for(uint32_t i=0;i<count;++i)at.push_back(le32(d,4+4*i));
        bool ordered=at[0]>=4+4*int64_t(count);for(size_t i=0;i<at.size();++i)ordered=ordered&&at[i]<size&&(i==0||at[i]>at[i-1]);
        if(ordered)for(size_t i=0;i<at.size();++i){const int64_t end=i+1<at.size()?at[i+1]:size;
            if(std::memcmp(d+at[i],"XPR2",4)==0){if(r.file.xpr2<0)r.file.xpr2=at[i];}
            else if(r.file.model<0)r.model(at[i],end);}
    }
    if(r.file.model<0&&r.file.meshes.empty())r.file.error="No Import Tuner Challenge model section.";
    return r.file;
}
inline std::vector<std::array<uint32_t,3>> faces(const uint8_t *d,const Element &e){
    std::vector<std::array<uint32_t,3>> out;int run=0;uint32_t a=0,b=0;
    for(int64_t k=0;k<e.indexCount;++k){
        const uint32_t v=be16(d,e.indexData+2*k);if(v==0xFFFF){run=0;continue;}
        if(run>=2&&a!=b&&b!=v&&a!=v)out.push_back(run%2?std::array<uint32_t,3>{b,a,v}:std::array<uint32_t,3>{a,b,v});
        a=b;b=v;++run;
    }
    return out;
}
}
