#pragma once
// Need for Speed: Most Wanted (PC) solid reader for GEOMETRY.BIN chunk trees (0x80134000 > 0x80134010 solids).
// Differs from the NFSU2 layout handled by nfsu2_scene.h: mesh header 0x134900 is 0x30-0x3C bytes (NFSU2: 0x44) and the
// mesh chunk 0x80134100 holds
//   0x134B02  0x68-byte submesh records after 0x11 padding: +0x38 flags, +0x3C vertex count,
//             +0x40 triangle count, +0x44 first index, +0x5C index count (= triangles * 3)
//   0x134B03  u16 triangle-list indices (optionally preceded by 0x1111 padding), solid-global
//   0x134B01  36-byte vertices after 0x11 padding: float3 position, float3 normal, RGBA8, float2 UV
//   0x134C02  one name per submesh (material/part label)
// The solid name follows the 0xA0-byte header of chunk 0x134011. Validated on the study-case
// Porsche 911 Turbo: 291 solids, 769 submeshes, 76,339 triangles, every index in range.
// Pure C++ so it can be validated outside Qt.
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace NfsMwCore {
inline uint32_t le32(const uint8_t *d,int64_t p){return uint32_t(d[p])|uint32_t(d[p+1])<<8|uint32_t(d[p+2])<<16|uint32_t(d[p+3])<<24;}
inline uint16_t le16(const uint8_t *d,int64_t p){return uint16_t(d[p]|d[p+1]<<8);}
inline float lef(const uint8_t *d,int64_t p){uint32_t v=le32(d,p);float f;std::memcpy(&f,&v,4);return f;}
inline constexpr int VertexStride=36,RecordSize=0x68;

struct Submesh { int64_t indexStart=0,indexCount=0; int vertexCount=0; uint32_t flags=0; std::string label; };
struct Solid {
    std::string name; int64_t at=0,vertexData=0,indexData=0; int vertexCount=0; std::vector<Submesh> submeshes;
    int64_t triangles() const {int64_t n=0;for(const auto &s:submeshes)n+=s.indexCount/3;return n;}
};
struct File { std::vector<Solid> solids; int rejected=0; bool container=false; };

struct Reader {
    const uint8_t *d; int64_t size; File file;
    int64_t skipPad(int64_t p,int64_t end,int step){while(p+step<=end&&d[p]==0x11&&d[p+1]==0x11&&(step==2||(d[p+2]==0x11&&d[p+3]==0x11)))p+=step;return p;}
    bool solid(int64_t at,int64_t end){
        Solid s;s.at=at-8;int64_t meshAt=-1,meshEnd=-1;
        for(int64_t p=at;p+8<=end;){const uint32_t t=le32(d,p),n=le32(d,p+4);if(n>uint64_t(end-p-8))return false;
            if(t==0x134011&&n>0xA0)for(int64_t q=p+8+0xA0;q<p+8+n&&d[q];++q)s.name+=char(d[q]);
            if(t==0x80134100){meshAt=p+8;meshEnd=p+8+n;}
            p+=8+n;}
        if(meshAt<0)return true; // solids without a mesh (placeholders) are legal and simply skipped
        int64_t header=-1,records=-1,recordsEnd=0,vertices=-1,verticesEnd=0,indices=-1,indicesEnd=0;std::vector<std::string> labels;
        for(int64_t p=meshAt;p+8<=meshEnd;){const uint32_t t=le32(d,p),n=le32(d,p+4);if(n>uint64_t(meshEnd-p-8))return false;const int64_t a=p+8,e=a+n;
            if(t==0x134900){if(n<0x30)return false;header=a;}
            else if(t==0x134B02){records=skipPad(a,e,4);recordsEnd=e;}
            else if(t==0x134B01&&vertices<0){vertices=skipPad(a,e,4);verticesEnd=e;}
            else if(t==0x134B03&&indices<0){indices=skipPad(a,e,2);indicesEnd=e;}
            else if(t==0x134C02){std::string l;for(int64_t q=a;q<e&&d[q];++q)l+=char(d[q]);labels.push_back(l);}
            p=e;}
        if(header<0||records<0||vertices<0||indices<0)return false;
        if((recordsEnd-records)%RecordSize||(verticesEnd-vertices)%VertexStride)return false;
        s.vertexData=vertices;s.vertexCount=int((verticesEnd-vertices)/VertexStride);s.indexData=indices;
        const int64_t indexTotal=(indicesEnd-indices)/2;int64_t vertexSum=0;
        for(int64_t r=records,i=0;r<recordsEnd;r+=RecordSize,++i){
            Submesh m;m.flags=le32(d,r+0x38);m.vertexCount=int(le32(d,r+0x3C));const int64_t tris=le32(d,r+0x40);m.indexStart=le32(d,r+0x44);m.indexCount=le32(d,r+0x5C);
            if(m.indexCount!=tris*3||m.indexStart+m.indexCount>indexTotal)return false;
            for(int64_t k=0;k<m.indexCount;++k)if(le16(d,indices+2*(m.indexStart+k))>=s.vertexCount)return false;
            if(size_t(i)<labels.size()){m.label=labels[i];}vertexSum+=m.vertexCount;s.submeshes.push_back(m);
        }
        if(vertexSum!=s.vertexCount)return false;
        if(s.triangles())file.solids.push_back(std::move(s));
        return true;
    }
    void walk(int64_t p,int64_t end,int depth){
        while(p+8<=end){const uint32_t t=le32(d,p),n=le32(d,p+4);if(n>uint64_t(end-p-8))return;
            if(t==0x80134010){if(!solid(p+8,p+8+n))++file.rejected;}
            else if((t&0x80000000)&&depth<6)walk(p+8,p+8+n,depth+1);
            p+=8+n;}
    }
};
inline File read(const uint8_t *d,int64_t size){
    Reader r{d,size,{}};if(size<16||le32(d,0)!=0x80134000)return r.file;
    r.file.container=true;r.walk(0,size,0);return r.file;
}
inline uint16_t index(const uint8_t *d,const Solid &s,int64_t i){return le16(d,s.indexData+2*i);}
// LOD suffix letter ('A' is the most detailed) or 0 when the name has none.
inline char lod(const std::string &name){const auto n=name.size();return n>2&&name[n-2]=='_'&&name[n-1]>='A'&&name[n-1]<='E'?name[n-1]:0;}
// Parts that belong in the stock assembly: highest LOD, stock kit, not damage/decal/alternate hoods.
// Wheels and brakes are modelled at the origin and placed at runtime, so they are kept separate.
inline bool variant(const std::string &name){
    const char l=lod(name);if(l&&l!='A')return true;
    for(auto key:{"_DAMAGE","_DECAL","_STYLE"})if(name.find(key)!=std::string::npos)return true;
    const auto kit=name.find("_KIT");return kit!=std::string::npos&&name.compare(kit,6,"_KIT00")!=0;
}
inline bool wheel(const std::string &name){return name.find("_TIRE")!=std::string::npos||name.find("_BRAKE_")!=std::string::npos||name.find("_WHEEL")!=std::string::npos||name.find("_RIM")!=std::string::npos;}
inline bool assembled(const std::string &name){return !variant(name)&&!wheel(name);}
}
