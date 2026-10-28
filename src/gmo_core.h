#pragma once
// Sony PSP GMO ("OMG.00.1PSP") model reader. Two header variants seen in the study cases:
//   Initial D Street Stage: block = u16 type, u16 header size, u32 size, name; data chunk header 8 bytes.
//   Street Supremacy (Genki): block = u16 type, u16 0, u32 size, u32 header size, u32; data chunk u16 type, u16 8, u32 size.
// Blocks: 0x02/0x00 file, 0x03 model, 0x04 bone, 0x05 part, 0x06 mesh, 0x07 arrays (vertex data),
// 0x08 material, 0x09 layer, 0x0A texture (0x8012 file name). Data chunks: 0x8015 vertex offset/scale
// (size 0x24), 0x8061 set material, 0x8066 draw arrays {arrays ref, primitive, vertices per primitive,
// primitive count[, first vertex]}. Arrays: {GU vertex type, count, morphs, 0} then interleaved
// vertices in GU order (weights, texture, colour, normal, position) with natural alignment.
// Pure C++ so the walk can be validated outside Qt.
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace GmoCore {
inline uint32_t le32(const uint8_t *d,int64_t p){return uint32_t(d[p])|uint32_t(d[p+1])<<8|uint32_t(d[p+2])<<16|uint32_t(d[p+3])<<24;}
inline uint16_t le16(const uint8_t *d,int64_t p){return uint16_t(d[p]|d[p+1]<<8);}
inline float lef(const uint8_t *d,int64_t p){uint32_t v=le32(d,p);float f;std::memcpy(&f,&v,4);return f;}

struct Layout { int stride=0,tex=-1,texSize=0,normal=-1,normalSize=0,position=-1,positionSize=0; bool valid=false; };
inline Layout layout(uint32_t format){
    Layout l;const int tex=format&3,col=(format>>2)&7,nrm=(format>>5)&3,pos=(format>>7)&3,w=(format>>9)&3,weights=((format>>14)&7)+1;
    static const int size[4]={0,1,2,4};int off=0,align=1;
    auto place=[&](int bytes,int count){off=(off+bytes-1)/bytes*bytes;const int at=off;off+=bytes*count;if(bytes>align)align=bytes;return at;};
    if(col&&(col<4))return l;
    if(w)place(size[w],weights);
    if(tex){l.tex=place(size[tex],2);l.texSize=size[tex];}
    if(col)place(col==7?4:2,1);
    if(nrm){l.normal=place(size[nrm],3);l.normalSize=size[nrm];}
    if(!pos)return l;
    l.position=place(size[pos],3);l.positionSize=size[pos];
    l.stride=(off+align-1)/align*align;l.valid=true;return l;
}
struct Arrays { int64_t data=0; uint32_t format=0; int count=0; Layout layout; };
struct Draw { int arrays=-1,primitive=0,perPrimitive=0,primitives=0,first=0,material=-1; };
struct Model {
    std::string name; std::array<double,3> offset{0,0,0},scale{1,1,1}; bool scaled=false;
    std::vector<Arrays> arrays; std::vector<Draw> draws; std::vector<std::string> materialTextures;
};
struct File { std::vector<Model> models; std::vector<std::string> textures; int64_t base=-1; std::string error; bool variantB=false; };

struct Reader {
    const uint8_t *d; int64_t size; File file; int depth=0;
    struct Chunk{int64_t at; int type; int64_t head,size;};
    bool chunk(int64_t p,int64_t end,Chunk &c){
        if(p+8>end)return false;
        c.at=p;c.type=le16(d,p);const int h16=le16(d,p+2);c.size=le32(d,p+4);
        if(c.size<8||c.size>end-p)return false;
        if(c.type&0x8000)c.head=8;
        else if(h16)c.head=h16;
        else{if(c.size<16)return false;c.head=le32(d,p+8);file.variantB=true;}
        return c.head>=8&&c.head<=c.size;
    }
    std::string name(const Chunk &c){if(file.variantB||c.head<=8)return {};std::string s;for(int64_t p=c.at+8;p<c.at+c.head&&d[p];++p)s+=char(d[p]);return s;}
    void model(const Chunk &m){
        Model out;out.name=name(m);
        std::vector<int> materialTexture; // material index -> texture index
        int64_t p=m.at+m.head;Chunk c;
        // First pass: model-level data, materials and textures (referenced by index from draws).
        for(;chunk(p,m.at+m.size,c);p+=c.size){
            if(c.type==0x8015&&c.size==0x24){for(int k=0;k<3;++k){out.offset[k]=lef(d,c.at+12+4*k);out.scale[k]=lef(d,c.at+24+4*k);}out.scaled=true;}
            else if(c.type==0x0008){int tex=-1;Chunk l;for(int64_t q=c.at+c.head;chunk(q,c.at+c.size,l);q+=l.size)if(l.type==0x0009){Chunk t;for(int64_t r=l.at+l.head;chunk(r,l.at+l.size,t);r+=t.size)if(t.type==0x8091&&tex<0)tex=int(le32(d,t.at+8)&0xfff);}materialTexture.push_back(tex);}
            else if(c.type==0x000A){Chunk t;std::string n;for(int64_t q=c.at+c.head;chunk(q,c.at+c.size,t);q+=t.size)if(t.type==0x8012)for(int64_t r=t.at+8;r<t.at+t.size&&d[r];++r)n+=char(d[r]);file.textures.push_back(n);}
        }
        for(int tex:materialTexture)out.materialTextures.push_back(tex>=0&&size_t(tex)<file.textures.size()?file.textures[tex]:std::string());
        // Second pass: parts, meshes, arrays and draws.
        for(p=m.at+m.head;chunk(p,m.at+m.size,c);p+=c.size)if(c.type==0x0005)part(c,out);
        file.models.push_back(std::move(out));
    }
    void addArrays(const Chunk &c,Model &out,std::vector<int> &local){
        if(c.size-c.head<16)return;
        Arrays a;a.format=le32(d,c.at+c.head);a.count=int(le32(d,c.at+c.head+4));a.data=c.at+c.head+16;a.layout=layout(a.format&0xffffff);
        if(!a.layout.valid||a.count<1||a.count>1000000||int64_t(a.count)*a.layout.stride>c.size-c.head-16){if(file.error.empty())file.error="GMO arrays with an unsupported vertex type or truncated data.";return;}
        local.push_back(int(out.arrays.size()));out.arrays.push_back(a);
    }
    void part(const Chunk &partChunk,Model &out){
        std::vector<int> local;std::vector<Draw> draws;Chunk c;
        auto mesh=[&](const Chunk &m){int material=-1;Chunk x;for(int64_t q=m.at+m.head;chunk(q,m.at+m.size,x);q+=x.size){
            if(x.type==0x8061)material=int(le32(d,x.at+8)&0xfff);
            else if(x.type==0x8066&&x.size>=0x1c){Draw w;const uint32_t ref=le32(d,x.at+8);w.arrays=int(ref&0xfff)|((ref>>16)==7?0x40000000:0);w.primitive=int(le32(d,x.at+12)&0xff);w.perPrimitive=int(le32(d,x.at+16));w.primitives=int(le32(d,x.at+20));w.first=file.variantB?0:int(le32(d,x.at+24));w.material=material;draws.push_back(w);}
            else if(x.type==0x0007)addArrays(x,out,local);}};
        for(int64_t p=partChunk.at+partChunk.head;chunk(p,partChunk.at+partChunk.size,c);p+=c.size){
            if(c.type==0x0006)mesh(c);else if(c.type==0x0007)addArrays(c,out,local);
        }
        // Part-local references (type 7 tag) index this part's arrays; plain indices are file-global.
        for(auto w:draws){const bool partLocal=w.arrays&0x40000000;const int i=w.arrays&0xfff;w.arrays=partLocal?(i<int(local.size())?local[i]:-1):(i<int(out.arrays.size())?i:-1);if(w.arrays>=0)out.draws.push_back(w);}
    }
};
inline File read(const uint8_t *d,int64_t size){
    Reader r{d,size,{}};
    int64_t base=-1;for(int64_t p=0;p+16<=size&&p<=4096;p+=4)if(std::memcmp(d+p,"OMG.00.1PSP",11)==0){base=p;break;}
    if(base<0){r.file.error="No GMO header.";return r.file;}
    r.file.base=base;Reader::Chunk f;
    for(int64_t p=base+16;r.chunk(p,size,f);p+=f.size){
        if(f.type!=0x0002&&f.type!=0x0000)continue;
        Reader::Chunk m;for(int64_t q=f.at+f.head;r.chunk(q,f.at+f.size,m);q+=m.size)if(m.type==0x0003)r.model(m);
    }
    if(r.file.models.empty()&&r.file.error.empty())r.file.error="GMO file contains no models.";
    return r.file;
}
inline std::array<double,3> position(const uint8_t *d,const Model &m,const Arrays &a,int i){
    const int64_t p=a.data+int64_t(i)*a.layout.stride+a.layout.position;std::array<double,3> v{};
    for(int k=0;k<3;++k){
        if(a.layout.positionSize==4)v[k]=lef(d,p+4*k);
        else{const double raw=a.layout.positionSize==2?double(int16_t(le16(d,p+2*k)))/32768.0:double(int8_t(d[p+k]))/128.0;v[k]=m.scaled?raw*m.scale[k]+m.offset[k]:raw;}
    }
    return v;
}
inline std::array<double,3> normal(const uint8_t *d,const Arrays &a,int i){
    std::array<double,3> n{};if(a.layout.normal<0)return n;const int64_t p=a.data+int64_t(i)*a.layout.stride+a.layout.normal;
    for(int k=0;k<3;++k)n[k]=a.layout.normalSize==4?double(lef(d,p+4*k)):a.layout.normalSize==2?double(int16_t(le16(d,p+2*k)))/32767.0:double(int8_t(d[p+k]))/127.0;
    return n;
}
// GU primitives: 3 triangles, 4 triangle strip, 5 triangle fan. Indices are relative to the arrays block.
inline std::vector<std::array<uint32_t,3>> faces(const Arrays &a,const Draw &w){
    std::vector<std::array<uint32_t,3>> out;
    for(int k=0;k<w.primitives;++k){
        const int64_t s=int64_t(w.first)+int64_t(k)*w.perPrimitive;if(s<0||s+w.perPrimitive>a.count)break;
        for(int i=2;i<w.perPrimitive;++i){
            uint32_t x,y,z;
            if(w.primitive==4){x=uint32_t(s+i-2);y=uint32_t(s+i-1);z=uint32_t(s+i);if(i%2)std::swap(x,y);}
            else if(w.primitive==3){if(i%3!=2)continue;x=uint32_t(s+i-2);y=uint32_t(s+i-1);z=uint32_t(s+i);}
            else if(w.primitive==5){x=uint32_t(s);y=uint32_t(s+i-1);z=uint32_t(s+i);}
            else break;
            out.push_back({x,y,z});
        }
    }
    return out;
}
}
