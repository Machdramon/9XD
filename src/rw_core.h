#pragma once
// RenderWare 3.x binary streams (DFF clumps, BSP worlds, standalone atomics) as used by D1 Grand Prix and the
// Selene scenes inside Super Trucks .MIX archives. Chunks are {type, size, library stamp}; geometry is either
// platform-independent (vertex/UV/triangle arrays) or PS2 native (NativeData PLG holding VIF packets with
// V3/V4-32 positions, V2-32 UVs and V3/V4-8 normals or colours; triangle strips with the ADC flag in position W).
// Frames are composed through the clump frame list; world sectors are walked through the plane-sector tree.
// Pure C++ so the walk can be validated outside Qt.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace RwCore {
inline uint32_t le32(const uint8_t *d,int64_t p){return uint32_t(d[p])|uint32_t(d[p+1])<<8|uint32_t(d[p+2])<<16|uint32_t(d[p+3])<<24;}
inline uint16_t le16(const uint8_t *d,int64_t p){return uint16_t(d[p]|d[p+1]<<8);}
inline float lef(const uint8_t *d,int64_t p){uint32_t v=le32(d,p);float f;std::memcpy(&f,&v,4);return f;}
enum : uint32_t { Struct=1,String=2,Extension=3,Texture=6,Material=7,MaterialList=8,AtomicSector=9,PlaneSector=0xA,World=0xB,FrameList=0xE,Geometry=0xF,Clump=0x10,Atomic=0x14,TextureNative=0x15,TexDictionary=0x16,GeometryList=0x1A,BinMesh=0x50E,NativeData=0x510 };
inline uint32_t version(uint32_t stamp){return (stamp&0xFFFF0000u)?(((stamp>>14)&0x3FF00u)+0x30000u)|((stamp>>16)&0x3Fu):stamp<<8;}

struct Chunk { uint32_t type=0,size=0,stamp=0; int64_t at=0,data=0,end=0; uint32_t version() const {return RwCore::version(stamp);} };
struct Vertex { std::array<float,3> position{},normal{}; std::array<float,2> uv{}; bool hasNormal=false; };
struct Part {
    std::string name; std::vector<Vertex> vertices; std::vector<std::array<uint32_t,3>> triangles; std::vector<int> triangleMaterial;
    std::vector<std::string> textures; bool hasUv=false,native=false; int64_t source=0;
};
struct Group { std::string kind,name; int64_t at=0; std::vector<Part> parts; std::vector<std::string> textures; };
struct File { std::vector<Group> groups; std::vector<std::pair<std::string,int64_t>> textureNatives; int unsupportedNative=0; std::string error; };

struct Reader {
    const uint8_t *d; int64_t size; File file;
    bool chunk(int64_t p,int64_t end,Chunk &c){
        if(p<0||p+12>end)return false;c.type=le32(d,p);c.size=le32(d,p+4);c.stamp=le32(d,p+8);c.at=p;c.data=p+12;c.end=c.data+c.size;
        const auto v=c.version();return c.size<=uint64_t(end-p-12)&&v>=0x30000&&v<=0x3FFFF;
    }
    std::vector<Chunk> children(const Chunk &parent){std::vector<Chunk> out;Chunk c;for(int64_t p=parent.data;chunk(p,parent.end,c);p=c.end)out.push_back(c);return out;}
    const Chunk *find(const std::vector<Chunk> &list,uint32_t type,int nth=0){for(const auto &c:list)if(c.type==type&&nth--==0)return &c;return nullptr;}
    std::string string(const Chunk &c){std::string s;for(int64_t p=c.data;p<c.end&&d[p];++p)s+=char(d[p]);return s;}
    std::vector<std::string> materials(const Chunk &list){
        std::vector<std::string> out;auto kids=children(list);const Chunk *s=find(kids,Struct);if(!s||s->size<4)return out;
        const uint32_t count=le32(d,s->data);if(count>4096||4+int64_t(count)*4>s->size)return out;std::vector<int> refs;for(uint32_t i=0;i<count;++i)refs.push_back(int(le32(d,s->data+4+4*i)));
        int next=0;std::vector<std::string> unique;for(const auto &m:kids)if(m.type==Material){std::string name;auto mk=children(m);if(auto t=find(mk,Texture)){auto tk=children(*t);if(auto n=find(tk,String))name=string(*n);}unique.push_back(name);}
        for(uint32_t i=0;i<count;++i){const int r=refs[i];out.push_back(r<0?(next<int(unique.size())?unique[next++]:std::string()):(r<int(out.size())?out[r]:std::string()));}
        return out;
    }
    // PS2 native VIF packets: one call per batch (MSCAL/MSCNT); strips with ADC in position W (V4-32) .
    void nativeMesh(int64_t p,int64_t end,int material,Part &out){
        std::vector<std::array<float,4>> pos;std::vector<uint32_t> w;std::vector<std::array<float,2>> uv;std::vector<std::array<float,3>> nrm;bool v4=false,uvInvalid=false;int uvFormat=0;
        auto flush=[&]{
            if(pos.size()>=3){const uint32_t base=uint32_t(out.vertices.size());
                bool completeUv=!uvInvalid&&uv.size()==pos.size();for(const auto &v:uv)for(auto x:v)completeUv&=std::isfinite(x);
                out.hasUv=base?out.hasUv&&completeUv:completeUv;
                for(size_t i=0;i<pos.size();++i){Vertex v;for(int k=0;k<3;++k)v.position[k]=pos[i][k];if(completeUv){v.uv={uv[i][0],uv[i][1]};}if(i<nrm.size()){v.normal=nrm[i];v.hasNormal=true;}out.vertices.push_back(v);}
                for(size_t i=2;i<pos.size();++i){if(v4&&(w[i]&0x8000))continue;uint32_t a=base+uint32_t(i-2),b=base+uint32_t(i-1),c=base+uint32_t(i);if(i%2)std::swap(a,b);
                    const auto &A=out.vertices[a].position,&B=out.vertices[b].position,&C=out.vertices[c].position;if(A==B||B==C||A==C)continue;out.triangles.push_back({a,b,c});out.triangleMaterial.push_back(material);}}
            pos.clear();w.clear();uv.clear();nrm.clear();uvInvalid=false;uvFormat=0;};
        while(p+4<=end){
            const uint32_t code=le32(d,p);const int cmd=(code>>24)&0x7F,num=(code>>16)&0xFF,imm=code&0xFFFF;
            if(cmd>=0x60){
                const int vn=(cmd>>2)&3,vl=cmd&3,n=num?num:256,bits=32>>vl;const int64_t bytes=(int64_t(bits)*(vn+1)*n+31)/32*4,a=p+4;if(a+bytes>end){uvInvalid=true;break;}const int addr=imm&0x3FF;
                if(vl==0&&(vn==2||vn==3)&&addr==0){v4=vn==3;for(int i=0;i<n;++i){std::array<float,4> v{};for(int k=0;k<3;++k)v[k]=lef(d,a+int64_t(i)*4*(vn+1)+4*k);pos.push_back(v);w.push_back(v4?le32(d,a+int64_t(i)*16+12):0);}}
                else if(vl==0&&vn==1){if(uvFormat&&uvFormat!=2)uvInvalid=true;uvFormat=2;for(int i=0;i<n;++i)uv.push_back({lef(d,a+int64_t(i)*8),lef(d,a+int64_t(i)*8+4)});}
                // Super Trucks PS2uv2 cluster: V4-32 at VU address1; XY is the primary pair.
                // Address0 is position/ADC, other V4 streams must never be treated as UV.
                else if(vl==0&&vn==3&&addr==1){if(uvFormat&&uvFormat!=4)uvInvalid=true;uvFormat=4;for(int i=0;i<n;++i)uv.push_back({lef(d,a+int64_t(i)*16),lef(d,a+int64_t(i)*16+4)});}
                else if(vl==2&&(vn==2||vn==3)&&!(imm&0x4000)){for(int i=0;i<n;++i){std::array<float,3> v{};for(int k=0;k<3;++k)v[k]=float(int8_t(d[a+int64_t(i)*(vn+1)+k]))/127.f;nrm.push_back(v);}}
                p=a+bytes;continue;}
            if(cmd==0x14||cmd==0x15||cmd==0x17)flush();
            if(cmd==0x20)p+=8;else if(cmd==0x30||cmd==0x31)p+=20;else if(cmd==0x50||cmd==0x51)p+=4+int64_t(imm?imm:65536)*16;else if(cmd==0x4A)p+=4+int64_t(num?num:256)*8;else p+=4;
        }
        flush();
    }
    bool native(const Chunk &ext,const std::vector<int> &meshMaterials,Part &out){
        auto kids=children(ext);const Chunk *nd=find(kids,NativeData);if(!nd)return false;auto nk=children(*nd);const Chunk *s=find(nk,Struct);if(!s||s->size<4)return false;
        if(le32(d,s->data)!=4){++file.unsupportedNative;return true;} // not PS2
        int64_t p=s->data+4;out.native=true;
        for(size_t m=0;m<meshMaterials.size()&&p+8<=s->end;++m){const int64_t bytes=le32(d,p);const int64_t a=p+8;if(a+bytes>s->end)break;// Serialized PS2 DMA subroutine: tag low64 is not VIF. TTE starts at +8,
            // QWC bounds payload; trailing relocation records are not command data.
            const auto flags=le32(d,p+4);
            if(flags==1&&bytes>=16){const auto tag=le32(d,a);const int64_t packetEnd=a+16+int64_t(tag&65535)*16;
                if(((tag>>28)&7)==6&&le32(d,a+4)==0&&packetEnd<=a+bytes)nativeMesh(a+8,packetEnd,meshMaterials[m],out);
            }else if(flags==0)nativeMesh(a,a+bytes,meshMaterials[m],out);
            p=a+bytes;}
        return true;
    }
    std::vector<int> binMeshMaterials(const Chunk &ext){std::vector<int> out;auto kids=children(ext);const Chunk *b=find(kids,BinMesh);if(!b||b->size<12)return out;const uint32_t n=le32(d,b->data+4);int64_t p=b->data+12;const bool hasIndices=true;
        for(uint32_t i=0;i<n&&p+8<=b->end;++i){const uint32_t count=le32(d,p);out.push_back(int(le32(d,p+4)));p+=8;(void)hasIndices;
            // Indices are present for platform-independent geometry only; native data has none (the next record follows).
            if(p+int64_t(count)*4<=b->end&&!nativeFlag)p+=int64_t(count)*4;}
        return out;}
    bool nativeFlag=false;
    bool geometry(const Chunk &g,Part &out){
        auto kids=children(g);const Chunk *s=find(kids,Struct);if(!s||s->size<16)return false;const uint32_t ver=s->version();
        const uint32_t format=le32(d,s->data);const int tris=int(le32(d,s->data+4)),verts=int(le32(d,s->data+8)),morphs=int(le32(d,s->data+12));
        if(tris<0||verts<0||verts>1000000||tris>4000000||morphs<0||morphs>64)return false;
        const bool isNative=format&0x01000000;int texSets=int((format>>16)&0xFF);if(!texSets)texSets=(format&0x80)?2:(format&0x04)?1:0;
        int64_t p=s->data+16+(ver<0x34000?12:0);
        if(const Chunk *ml=find(kids,MaterialList))out.textures=materials(*ml);
        const Chunk *ext=find(kids,Extension);
        if(isNative){nativeFlag=true;auto mm=ext?binMeshMaterials(*ext):std::vector<int>();nativeFlag=false;if(!ext||!native(*ext,mm,out))return false;return true;}
        if(format&0x08)p+=int64_t(verts)*4;
        std::vector<std::array<float,2>> uv;if(texSets){if(p+int64_t(verts)*8>s->end)return false;for(int i=0;i<verts;++i)uv.push_back({lef(d,p+8*i),lef(d,p+8*i+4)});p+=int64_t(verts)*8*texSets;}
        if(p+int64_t(tris)*8>s->end)return false;const int64_t triAt=p;p+=int64_t(tris)*8;
        if(morphs<1||p+24>s->end)return false;const uint32_t hasV=le32(d,p+16),hasN=le32(d,p+20);p+=24;
        if(!hasV||p+int64_t(verts)*12>s->end)return false;const uint32_t base=uint32_t(out.vertices.size());
        for(int i=0;i<verts;++i){Vertex v;for(int k=0;k<3;++k)v.position[k]=lef(d,p+12*i+4*k);if(!uv.empty())v.uv=uv[i];out.vertices.push_back(v);}p+=int64_t(verts)*12;
        if(hasN&&p+int64_t(verts)*12<=s->end)for(int i=0;i<verts;++i){auto &v=out.vertices[base+i];for(int k=0;k<3;++k)v.normal[k]=lef(d,p+12*i+4*k);v.hasNormal=true;}
        out.hasUv=out.hasUv||!uv.empty();
        for(int t=0;t<tris;++t){const int64_t q=triAt+8*t;const uint32_t v2=le16(d,q),v1=le16(d,q+2),m=le16(d,q+4),v3=le16(d,q+6);if(int(v1)>=verts||int(v2)>=verts||int(v3)>=verts)return false;out.triangles.push_back({base+v1,base+v2,base+v3});out.triangleMaterial.push_back(int(m));}
        return true;
    }
    using Matrix=std::array<float,12>; // 3x3 rotation rows (right, up, at) + position
    static std::array<float,3> apply(const Matrix &m,const std::array<float,3> &v,bool direction=false){std::array<float,3> r{};for(int k=0;k<3;++k)r[k]=v[0]*m[k]+v[1]*m[3+k]+v[2]*m[6+k]+(direction?0:m[9+k]);return r;}
    static Matrix multiply(const Matrix &child,const Matrix &parent){Matrix r{};for(int row=0;row<4;++row){std::array<float,3> v{child[row*3],child[row*3+1],child[row*3+2]};auto o=apply(parent,v,row<3);for(int k=0;k<3;++k)r[row*3+k]=o[k];}return r;}
    void clump(const Chunk &c){
        auto kids=children(c);Group group;group.kind="clump";group.at=c.at;std::vector<Matrix> frames;std::vector<int> parents;
        if(const Chunk *fl=find(kids,FrameList)){auto fk=children(*fl);if(const Chunk *s=find(fk,Struct);s&&s->size>=4){const uint32_t n=le32(d,s->data);if(n<=4096&&4+int64_t(n)*56<=s->size)for(uint32_t i=0;i<n;++i){Matrix m{};for(int k=0;k<12;++k)m[k]=lef(d,s->data+4+56*i+4*k);frames.push_back(m);parents.push_back(int(le32(d,s->data+4+56*i+48)));}}}
        std::vector<Matrix> world(frames.size());for(size_t i=0;i<frames.size();++i){Matrix m=frames[i];int p=parents[i];for(int guard=0;p>=0&&p<int(frames.size())&&guard<64;++guard){m=multiply(m,frames[p]);p=parents[p];}world[i]=m;}
        std::vector<Part> geometries;if(const Chunk *gl=find(kids,GeometryList))for(const auto &g:children(*gl))if(g.type==Geometry){Part part;part.source=g.at;if(!geometry(g,part))part=Part{};geometries.push_back(part);}
        for(const auto &a:kids){if(a.type!=Atomic)continue;auto ak=children(a);const Chunk *s=find(ak,Struct);if(!s||s->size<8)continue;const uint32_t frame=le32(d,s->data),geo=le32(d,s->data+4);
            Part part;if(const Chunk *g=find(ak,Geometry)){part.source=g->at;if(!geometry(*g,part))continue;}else if(geo<geometries.size())part=geometries[geo];else continue;
            if(frame<world.size())for(auto &v:part.vertices){v.position=apply(world[frame],v.position);if(v.hasNormal)v.normal=apply(world[frame],v.normal,true);}
            if(!part.triangles.empty())group.parts.push_back(std::move(part));}
        if(!group.parts.empty())file.groups.push_back(std::move(group));
    }
    void sector(const Chunk &c,const std::vector<std::string> &mats,uint32_t worldFormat,Part &out,int depth){
        if(depth>64)return;auto kids=children(c);const Chunk *s=find(kids,Struct);if(!s)return;
        if(c.type==PlaneSector){for(const auto &k:kids)if(k.type==PlaneSector||k.type==AtomicSector)sector(k,mats,worldFormat,out,depth+1);return;}
        if(s->size<44)return;const uint32_t matBase=le32(d,s->data),tris=le32(d,s->data+4),verts=le32(d,s->data+8);if(verts>1000000||tris>4000000)return;
        const Chunk *ext=find(kids,Extension);
        if(worldFormat&0x01000000){nativeFlag=true;auto mm=ext?binMeshMaterials(*ext):std::vector<int>();nativeFlag=false;for(auto &m:mm)m+=int(matBase);if(ext)native(*ext,mm,out);return;}
        if(!verts||!tris)return;int texSets=int((worldFormat>>16)&0xFF);if(!texSets)texSets=(worldFormat&0x80)?2:(worldFormat&0x04)?1:0;
        int64_t p=s->data+44;const uint32_t base=uint32_t(out.vertices.size());if(p+int64_t(verts)*12>s->end)return;
        for(uint32_t i=0;i<verts;++i){Vertex v;for(int k=0;k<3;++k)v.position[k]=lef(d,p+12*i+4*k);out.vertices.push_back(v);}p+=int64_t(verts)*12;
        if(worldFormat&0x10){if(p+int64_t(verts)*4<=s->end)for(uint32_t i=0;i<verts;++i){auto &v=out.vertices[base+i];for(int k=0;k<3;++k)v.normal[k]=float(int8_t(d[p+4*i+k]))/127.f;v.hasNormal=true;}p+=int64_t(verts)*4;}
        if(worldFormat&0x08)p+=int64_t(verts)*4;
        if(texSets){if(p+int64_t(verts)*8<=s->end)for(uint32_t i=0;i<verts;++i)out.vertices[base+i].uv={lef(d,p+8*i),lef(d,p+8*i+4)};out.hasUv=true;p+=int64_t(verts)*8*texSets;}
        if(p+int64_t(tris)*8>s->end)return;
        // RW 3.4+ stores {v1, v2, v3, material}; older streams put the material first. Pick the layout whose
        // material field stays inside the material list.
        bool materialLast=true;for(uint32_t t=0;t<tris&&materialLast;++t)if(le16(d,p+8*t+6)+matBase>=mats.size()||le16(d,p+8*t)>=verts)materialLast=le16(d,p+8*t+6)+matBase<mats.size()&&le16(d,p+8*t)<verts;
        for(uint32_t t=0;t<tris;++t){const int64_t q=p+8*t;uint32_t a,b,cc,m;if(materialLast){a=le16(d,q);b=le16(d,q+2);cc=le16(d,q+4);m=le16(d,q+6);}else{m=le16(d,q);a=le16(d,q+2);b=le16(d,q+4);cc=le16(d,q+6);}
            if(a>=verts||b>=verts||cc>=verts)continue;out.triangles.push_back({base+a,base+b,base+cc});out.triangleMaterial.push_back(int(m+matBase));}
    }
    void world(const Chunk &c){
        auto kids=children(c);const Chunk *s=find(kids,Struct);if(!s||s->size<40)return;const uint32_t format=le32(d,s->data+36);
        Group group;group.kind="world";group.at=c.at;Part part;part.source=c.at;
        if(const Chunk *ml=find(kids,MaterialList))part.textures=materials(*ml);
        for(const auto &k:kids)if(k.type==PlaneSector||k.type==AtomicSector)sector(k,part.textures,format,part,0);
        if(!part.triangles.empty()){group.parts.push_back(std::move(part));file.groups.push_back(std::move(group));}
    }
    void top(int64_t p,int64_t end,int depth){
        Chunk c;
        while(chunk(p,end,c)){
            if(c.type==Clump)clump(c);
            else if(c.type==World)world(c);
            else if(c.type==Atomic){Group g;g.kind="atomic";g.at=c.at;auto ak=children(c);if(const Chunk *gc=find(ak,Geometry)){Part part;part.source=gc->at;if(geometry(*gc,part)&&!part.triangles.empty()){g.parts.push_back(std::move(part));file.groups.push_back(std::move(g));}}}
            else if(c.type==TexDictionary){for(const auto &t:children(c))if(t.type==TextureNative){auto tk=children(t);if(const Chunk *n=find(tk,String))file.textureNatives.push_back({string(*n),t.at});}}
            p=c.end;
        }
    }
};
// Reads every RenderWare stream found at the start of the data (and nothing else).
inline File read(const uint8_t *d,int64_t size){Reader r{d,size,{}};r.top(0,size,0);return r.file;}
inline File readAt(const uint8_t *d,int64_t size,int64_t at,int64_t end){Reader r{d,size,{}};r.top(at,end,0);return r.file;}
// Containers that embed RenderWare chunks between their own records (Selene .MIX scenes): every Atomic/Clump/World
// chunk inside [start, end) that parses cleanly.
inline File scanRange(const uint8_t *d,int64_t size,int64_t start,int64_t end){
    File out;Reader r{d,size,{}};
    for(int64_t p=start;p+12<=end;++p){Chunk c;const uint32_t t=le32(d,p);if((t!=Atomic&&t!=Clump&&t!=World)||!r.chunk(p,end,c))continue;
        auto f=readAt(d,size,p,c.end);for(auto &g:f.groups)out.groups.push_back(std::move(g));p=c.end-1;}
    return out;
}
}
