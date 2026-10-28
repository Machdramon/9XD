#pragma once
// Wangan Midnight PS3 XMD/XTD/X3D reader (Genki, big-endian). Layout from the Gato's Lair
// wmps3_nines research script and mariokart64n's Xentax MaxScript, verified on JZS161.CAR and
// AREA_9GO_BUIL_CUT_9GO_FREE_001.GRID. Pure C++ so the walk can be validated outside Qt.
//   XMD  "XMD\1": BE child count @12, BE child offsets @16 (relative), sizes after 16-byte padding.
//   XTD  "XTD\0": LE entry count @8, LE entry offsets @12. Entry: LE size, "GTF\0" or 0,
//        LE u16 bank @8, LE u16 index @10; real entries carry a GTF at +16, 16-byte entries are references.
//   X3D model: block table at +0x84 (BE count @+0x88, offsets @+0x90, relative to the model).
//        Block: BE u16 type, u16 string index (1-based), u32 size, u32, u32.
//        0x15 string, 0x07 material, 0x0C texture slot, 0x05 shape (+u3 bindings: material, geometry),
//        0x08 geometry: 80-byte header, then streams (8 BE u32 header, data at +32):
//        comp 0 u16 strip indices (0xFFFF restart), comp 2 float3 position, comp 4 float3 normal, comp 5 float2 UV (set in word 7).
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace WmPs3Core {
inline uint32_t be32(const uint8_t *d,int64_t p){return uint32_t(d[p])<<24|uint32_t(d[p+1])<<16|uint32_t(d[p+2])<<8|d[p+3];}
inline uint16_t be16(const uint8_t *d,int64_t p){return uint16_t(d[p]<<8|d[p+1]);}
inline uint32_t le32(const uint8_t *d,int64_t p){return uint32_t(d[p])|uint32_t(d[p+1])<<8|uint32_t(d[p+2])<<16|uint32_t(d[p+3])<<24;}
inline uint16_t le16(const uint8_t *d,int64_t p){return uint16_t(d[p]|d[p+1]<<8);}
inline float bef(const uint8_t *d,int64_t p){uint32_t v=be32(d,p);float f;std::memcpy(&f,&v,4);return f;}

struct Texture { int64_t entry=0,gtf=-1; int bank=-1,index=-1; int width=0,height=0,format=0,xtd=0; bool real()const{return gtf>=0;} };
struct Xtd { int64_t base=0; std::vector<int> entries; };
struct Stream { int64_t data=0; int comp=0,dtype=0,count=0,set=0; };
struct Geometry { int64_t block=0; int ordinal=-1,material=-1,shape=-1; std::string name; Stream position,normal,uv,indices; bool hasNormal=false,hasUv=false; };
struct Material { int64_t block=0; std::string name; int textureSlot=-1; };
struct Shape { std::string name; std::vector<std::pair<int,int>> bindings; };
struct Model {
    int64_t base=0; int localXtd=-1; std::vector<std::string> strings; std::vector<Material> materials;
    std::vector<int> slotEntries; std::vector<std::string> slotNames; std::vector<Geometry> geometries; std::vector<Shape> shapes;
};
struct Library { std::vector<Model> models; std::vector<Texture> textures; std::vector<Xtd> xtds; std::string error; };

struct Reader {
    const uint8_t *d; int64_t size; Library lib;
    bool in(int64_t p,int64_t n)const{return p>=0&&n>=0&&p<=size&&n<=size-p;}
    void fail(const std::string &e){if(lib.error.empty())lib.error=e;}
    void xtd(int64_t base,int64_t end){
        if(!in(base,12))return fail("Truncated XTD header.");
        const uint32_t n=le32(d,base+8);if(n>4096||!in(base+12,int64_t(n)*4))return fail("Invalid XTD entry table.");
        Xtd x;x.base=base;const int id=int(lib.xtds.size());
        for(uint32_t i=0;i<n;++i){
            const int64_t e=base+le32(d,base+12+4*i);if(!in(e,16)||e>=end)return fail("XTD entry outside its container.");
            Texture t;t.entry=e;t.xtd=id;t.bank=le16(d,e+8);t.index=le16(d,e+10);
            const uint32_t total=le32(d,e);
            if(total>16&&std::memcmp(d+e+4,"GTF\0",4)==0&&in(e,total)&&total>=16+48){
                const int64_t g=e+16;
                if(be32(d,g)==0x01050000u&&be32(d,g+8)>=1){const int64_t gcm=g+12+12;t.gtf=g;t.format=d[gcm];t.width=be16(d,gcm+8);t.height=be16(d,gcm+10);}
            }
            x.entries.push_back(int(lib.textures.size()));lib.textures.push_back(t);
        }
        lib.xtds.push_back(x);
    }
    std::string cstr(int64_t p,int64_t end){std::string s;while(p<end&&p<size&&d[p]&&s.size()<256)s+=char(d[p++]);return s;}
    void model(int64_t base,int64_t end,int localXtd){
        if(!in(base,0x90)||std::memcmp(d+base+8,"X3D",3)!=0)return fail("Unrecognized XMD model block.");
        const uint32_t count=be32(d,base+0x88);if(count>65536||!in(base+0x90,int64_t(count)*4))return fail("Invalid X3D block table.");
        Model m;m.base=base;m.localXtd=localXtd;
        struct Block{int64_t at;int type,string;uint32_t size,u2,u3;};std::vector<Block> blocks;
        for(uint32_t i=0;i<count;++i){const int64_t o=base+be32(d,base+0x90+4*i);if(!in(o,16)||o>=end)return fail("X3D block outside the model.");blocks.push_back({o,be16(d,o),be16(d,o+2),be32(d,o+4),be32(d,o+8),be32(d,o+12)});}
        for(auto &b:blocks)if(b.type==0x15)m.strings.push_back(cstr(b.at+16,end));
        auto name=[&](int i){return i>0&&size_t(i)<=m.strings.size()?m.strings[i-1]:std::string();};
        for(auto &b:blocks){
            if(b.type==0x07){
                Material mat;mat.block=b.at;mat.name=name(b.string);
                if(!in(b.at+16,16))return fail("Truncated material.");
                const uint32_t entries=be32(d,b.at+28);int64_t p=b.at+32;int t9=-1;
                for(uint32_t k=0;k<entries&&k<256;++k){
                    if(!in(p,16))return fail("Truncated material entry.");
                    const uint32_t type=be32(d,p),sz=be32(d,p+4),u1=be32(d,p+8);if(sz<16||!in(p,sz))return fail("Invalid material entry size.");
                    if(type==8&&mat.textureSlot<0&&cstr(p+16,p+sz)=="tx_name_0")mat.textureSlot=int(u1);
                    if(type==9&&t9<0)t9=int(u1);
                    p+=sz;
                }
                if(mat.textureSlot<0)mat.textureSlot=t9;
                m.materials.push_back(mat);
            }else if(b.type==0x0C){
                if(!in(b.at+16,4))return fail("Truncated texture slot.");
                m.slotEntries.push_back(int(be32(d,b.at+16)));m.slotNames.push_back(name(b.string));
            }else if(b.type==0x05){
                Shape s;s.name=name(b.string);int64_t p=b.at+b.size;
                for(uint32_t k=0;k<b.u3&&k<4096;++k){
                    if(!in(p,16))return fail("Truncated shape binding.");
                    const uint32_t sz=be32(d,p+4);if(sz<16||!in(p,sz))return fail("Invalid shape binding.");
                    if(be16(d,p)==6)s.bindings.push_back({int(be32(d,p+8)),int(be32(d,p+12))});
                    p+=sz;
                }
                m.shapes.push_back(s);
            }else if(b.type==0x08){
                Geometry g;g.block=b.at;g.ordinal=int(b.u3);g.name=name(b.string);
                int64_t p=b.at+16;if(!in(p,80))return fail("Truncated geometry header.");
                const uint32_t streams=be32(d,p+48+12);p+=80;
                for(uint32_t k=0;k<streams&&k<64;++k){
                    if(!in(p,32))return fail("Truncated geometry stream.");
                    const uint32_t sz=be32(d,p+4);Stream s;s.comp=int(be32(d,p+8));s.dtype=int(be32(d,p+12));s.count=int(be32(d,p+20));s.set=int(be32(d,p+28));s.data=p+32;
                    if(sz<32||!in(p,sz)||s.count<0)return fail("Invalid geometry stream size.");
                    const int64_t need=s.dtype==12?int64_t(s.count)*2:s.dtype==26?int64_t(s.count)*12:s.dtype==25?int64_t(s.count)*8:0;
                    if(32+need>sz)return fail("Geometry stream data exceeds its block.");
                    if(s.comp==0&&s.dtype==12)g.indices=s;
                    else if(s.comp==2&&s.dtype==26&&g.position.count==0)g.position=s;
                    else if(s.comp==4&&s.dtype==26&&!g.hasNormal){g.normal=s;g.hasNormal=true;}
                    else if(s.comp==5&&s.dtype==25&&(!g.hasUv||s.set<g.uv.set)){g.uv=s;g.hasUv=true;}
                    p+=sz;
                }
                if(g.hasNormal&&g.normal.count!=g.position.count)g.hasNormal=false;
                if(g.hasUv&&g.uv.count!=g.position.count)g.hasUv=false;
                m.geometries.push_back(g);
            }
        }
        for(int s=0;s<int(m.shapes.size());++s)for(auto &[material,geometry]:m.shapes[s].bindings)for(auto &g:m.geometries)if(g.ordinal==geometry){g.material=material;g.shape=s;}
        lib.models.push_back(std::move(m));
    }
    void xmd(int64_t base,int64_t end,int depth){
        if(depth>8)return fail("XMD nesting is too deep.");
        if(!in(base,16)||std::memcmp(d+base,"XMD\1",4)!=0)return fail("Missing XMD header.");
        const uint32_t n=be32(d,base+12);if(n<1||n>256||!in(base+16,int64_t(n)*4))return fail("Invalid XMD child table.");
        const int64_t sizes=(base+16+4*int64_t(n)+15)&~int64_t(15);if(!in(sizes,int64_t(n)*4))return fail("Truncated XMD size table.");
        std::vector<std::pair<int64_t,int64_t>> children;
        for(uint32_t i=0;i<n;++i){const int64_t o=base+be32(d,base+16+4*i),s=be32(d,sizes+4*i);if(!in(o,s)||o+s>end||o<base+16)return fail("XMD child outside its parent.");children.push_back({o,o+s});}
        // Textures first, so models can resolve slots against their sibling XTD.
        int local=-1;
        for(auto &[o,e]:children)if(std::memcmp(d+o,"XTD\0",4)==0){xtd(o,e);local=int(lib.xtds.size())-1;}
        for(auto &[o,e]:children){
            if(std::memcmp(d+o,"XMD\1",4)==0)xmd(o,e,depth+1);
            else if(std::memcmp(d+o,"XTD\0",4)!=0)model(o,e,local);
            if(!lib.error.empty())return;
        }
    }
};
inline Library read(const uint8_t *d,int64_t size){
    Reader r{d,size,{}};
    if(size<16||std::memcmp(d,"XMD\1",4)!=0){r.lib.error="Not an XMD file.";return r.lib;}
    r.xmd(0,size,0);
    if(!r.lib.error.empty()){r.lib.models.clear();}
    return r.lib;
}
// Material slot -> sibling XTD entry -> (bank,index) reference -> real GTF anywhere in the file.
inline int resolveTexture(const Library &lib,const Model &m,int material){
    if(material<0||material>=int(m.materials.size()))return -1;
    const int slot=m.materials[material].textureSlot;if(slot<0||slot>=int(m.slotEntries.size())||m.localXtd<0)return -1;
    const auto &entries=lib.xtds[m.localXtd].entries;const int e=m.slotEntries[slot];if(e<0||e>=int(entries.size()))return -1;
    const auto &t=lib.textures[entries[e]];if(t.real())return entries[e];
    for(int i=0;i<int(lib.textures.size());++i)if(lib.textures[i].real()&&lib.textures[i].bank==t.bank&&lib.textures[i].index==t.index)return i;
    return -1;
}
// Strip indices with 0xFFFF restarts; winding alternates from each restart; degenerate triangles dropped.
inline std::vector<std::array<uint32_t,3>> faces(const uint8_t *d,const Geometry &g){
    std::vector<std::array<uint32_t,3>> out;std::vector<uint32_t> run;
    auto flush=[&]{for(size_t i=2;i<run.size();++i){uint32_t a=run[i-2],b=run[i-1],c=run[i];if(i%2)std::swap(a,b);if(a==b||b==c||a==c)continue;if(a<uint32_t(g.position.count)&&b<uint32_t(g.position.count)&&c<uint32_t(g.position.count))out.push_back({a,b,c});}run.clear();};
    for(int i=0;i<g.indices.count;++i){const uint16_t v=be16(d,g.indices.data+2*int64_t(i));if(v==0xffff)flush();else run.push_back(v);}
    flush();return out;
}
}
