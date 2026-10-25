#pragma once
#include "native_resources.h"
#include "mesh_data.h"
#include "memory_budget.h"
#include <array>
#include <cmath>
#include <cstring>

namespace RTunedCollision {
struct Layout {QString kind,error; qint64 records=0,positions=0,normals=0,vertices=0,triangles=0; bool recognized=false,valid=false;};
using Vec=std::array<double,3>;
inline quint32 word(const QByteArray &b,qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);}
inline double number(const QByteArray &b,qint64 p){auto bits=word(b,p);float f;std::memcpy(&f,&bits,4);return f;}
inline Vec vector(const QByteArray &b,qint64 p){return {number(b,p),number(b,p+4),number(b,p+8)};}
inline Vec subtract(Vec a,Vec b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline double oriented(Vec a,Vec b,Vec c,Vec n){auto x=subtract(b,a),y=subtract(c,a);return (x[1]*y[2]-x[2]*y[1])*n[0]+(x[2]*y[0]-x[0]*y[2])*n[1]+(x[0]*y[1]-x[1]*y[0])*n[2];}
inline qint64 position(const Layout &l,qint64 i,int corner){return l.kind=="COLI0200"?l.positions+i*128+corner*12:40+i*132+corner*24;}
inline qint64 normal(const Layout &l,qint64 i,int corner){return l.kind=="COLI0200"?l.normals+i*48+corner*12:52+i*132+corner*24;}
// The four stored corners describe the polygon boundary. Choose the internal diagonal
// whose two triangles agree with the source normals, including concave polygons.
inline int diagonal(const std::array<Vec,4> &p,Vec n,int corners,bool reversed){
    auto sign=reversed?-1.:1.;auto positive=[&](int a,int b,int c){return sign*oriented(p[a],p[b],p[c],n)>1e-12;};
    if(corners==3)return positive(0,1,2)?0:-1;
    if(positive(0,1,2)&&positive(0,2,3))return 0;
    return positive(0,1,3)&&positive(1,2,3)?1:-1;
}
inline Layout inspect(const QByteArray &b,const std::atomic_bool &cancel){
    Layout l;auto fits=[&](qint64 p,qint64 n){return p>=0&&n>=0&&p<=b.size()&&n<=b.size()-p;};
    if(b.startsWith("COLI0200")){
        l.recognized=true;l.kind="COLI0200";if(b.size()<64){l.error="Incomplete COLI0200 header.";return l;}l.records=word(b,8);l.positions=word(b,28);l.normals=word(b,32);
        qint64 offsets[8];for(int i=0;i<8;++i)offsets[i]=word(b,16+i*4);
        bool ordered=offsets[0]==64;for(int i=1;i<8;++i)ordered&=offsets[i]>=offsets[i-1];
        if(!ordered||l.records<1||l.records!=word(b,12)||l.normals-l.positions!=l.records*128||qint64(word(b,36))!=((l.normals+l.records*48+31)&~qint64(31))||!fits(l.positions,l.records*128)||!fits(l.normals,l.records*48)||!fits(offsets[7],32)){l.error="Invalid COLI0200 tables or record ranges.";return l;}
    }else{
        if(b.size()<164||word(b,4)!=32)return l;for(int i=8;i<32;++i)if(b[i])return l;
        l.records=word(b,0);qint64 end=32+l.records*132;
        if(l.records<1||end>b.size()||((end+31)&~qint64(31))!=b.size())return l;
        for(qint64 p=end;p<b.size();++p)if(b[p])return l;
        l.kind="Collision132";l.recognized=true;l.positions=40;
    }
    auto fail=[&](QString error){l.error=std::move(error);return l;};
    if(!AssetMemory::fits(l.records*4,l.records*6))return fail("Collision mesh exceeds the configured memory budget.");
    for(qint64 i=0;i<l.records;++i){
        if(cancel.load())return fail("Collision scan canceled.");int corners=4;
        if(l.kind!="COLI0200"){auto type=word(b,32+i*132);if(type>1||word(b,36+i*132))return fail("Unsupported collision polygon flags.");corners=type?4:3;}
        std::array<Vec,4> p;Vec n{};
        for(int c=0;c<4;++c){p[c]=vector(b,position(l,i,c));auto v=vector(b,normal(l,i,c));double length=0;for(int a=0;a<3;++a){if(!std::isfinite(p[c][a])||std::abs(p[c][a])>1e8||!std::isfinite(v[a]))return fail("Nonfinite collision attributes.");length+=v[a]*v[a];n[a]+=v[a];}if(length<.9||length>1.1)return fail("Invalid collision normal length.");}
        if(corners==3&&p[2]!=p[3])return fail("Triangle collision record has an unexpected fourth corner.");
        if(diagonal(p,n,corners,l.kind=="COLI0200")<0)return fail("Collision polygon has unsupported or degenerate winding.");
        l.vertices+=corners;l.triangles+=corners-2;
    }
    l.valid=true;return l;
}
inline QJsonObject candidate(const Layout &l){
    return {{"name",l.kind=="COLI0200"?"R-Tuned COLI0200 surfaces":"R-Tuned collision surfaces"},{"profileId","native-rtuned-collision"},{"profileName",l.kind},{"nativeDecoder",true},{"collisionLayout",l.kind},{"collisionRecords",QString::number(l.records)},{"objectOffset","0"},{"settings",QJsonObject{{"vertexOffset",QString::number(l.positions)},{"vertexCount",l.vertices},{"indexCount",l.triangles*3}}},{"basis","Collision geometry, not the visual track model. Stored polygon corners and source normals; no inferred point connectivity. Original coordinates preserved; materials/textures are not present in this import."}};
}
inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel){
    auto l=inspect(b,cancel);if(!l.recognized){
        if(b.size()>=250032&&word(b,0)==62500&&word(b,4)==32&&word(b,32)==250032){bool valid=true;quint32 previous=250032;for(int i=8;i<32;++i)valid&=b[i]==0;for(int i=0;i<62500&&valid;++i){if(cancel.load())return {};auto offset=word(b,32+i*4);valid=offset>=previous&&offset<=quint64(b.size());previous=offset;}if(valid)return {{"notes",QJsonArray{"R-Tuned spatial block index recognized. This resource indexes collision surfaces; it is not a separate mesh. Preview the sibling _colli resource or scan the original FArC."}},{"regions",QJsonArray{QJsonObject{{"name","R-Tuned collision spatial index"},{"offset","32"},{"length","4"},{"step","4"},{"repetitions",62500},{"evidence","Bounded monotonic cell pointer table"}}}}};}
        return {};
    }if(!l.valid)return {{"notes",QJsonArray{l.error}}};
    return {{"candidates",QJsonArray{candidate(l)}},{"notes",QJsonArray{"R-Tuned collision surfaces reconstructed. Route variants may overlap; use visibility/isolation to review them."}},{"regions",QJsonArray{QJsonObject{{"name","R-Tuned collision header"},{"offset","0"},{"length",l.kind=="COLI0200"?"64":"32"},{"step","1"},{"repetitions",1},{"evidence","Validated collision polygon records, normals and winding"}}}}};
}
inline MeshResult decodeBytes(const QByteArray &b,const QJsonObject &c,const std::atomic_bool &cancel){
    MeshResult out;auto l=inspect(b,cancel);if(!l.valid||l.kind!=c["collisionLayout"].toString()||QString::number(l.records)!=c["collisionRecords"].toString()){out.error=l.error.isEmpty()?"Collision layout no longer matches the scan.":l.error;return out;}
    out.mesh=std::make_shared<MeshData>();out.mesh->hasUvs=false;out.mesh->vertices.reserve(l.vertices);out.mesh->triangles.reserve(l.triangles*3);
    for(qint64 i=0;i<l.records;++i){
        if(cancel.load()){out.mesh.reset();out.error="Collision decoding canceled.";return out;}
        int count=l.kind=="COLI0200"||word(b,32+i*132)?4:3;std::array<Vec,4> p;Vec n{};auto base=quint32(out.mesh->vertices.size());
        for(int a=0;a<4;++a){p[a]=vector(b,position(l,i,a));auto normalValue=vector(b,normal(l,i,a));for(int j=0;j<3;++j)n[j]+=normalValue[j];if(a>=count)continue;MeshVertex v{};for(int j=0;j<3;++j){v.position[j]=p[a][j];v.normal[j]=normalValue[j];}out.mesh->vertices.append(v);out.mesh->vertexOffsets.append(position(l,i,a));}
        auto emitFace=[&](int a,int d,int e){if(l.kind=="COLI0200")std::swap(d,e);out.mesh->triangles<<base+a<<base+d<<base+e;out.mesh->faceSources.append({{position(l,i,a),position(l,i,d),position(l,i,e)},{-1,-1,-1},int(i)});};
        if(count==3)emitFace(0,1,2);else if(diagonal(p,n,count,l.kind=="COLI0200")==0){emitFace(0,1,2);emitFace(0,2,3);}else{emitFace(0,1,3);emitFace(1,2,3);}
    }
    return out;
}
inline QJsonArray archiveCandidates(ReadRequest source,const PatchList &patches,const QJsonObject &resources,const std::atomic_bool &cancel){
    QJsonArray out;if(!resources["format"].toString().startsWith("FArC"))return out;qint64 work=0;
    for(auto value:resources["entries"].toArray()){
        if(cancel.load())break;auto row=value.toObject();auto name=row["name"].toString();if(!name.contains("_COL_")||!(name.endsWith("_colli.bin")||name.contains("_COL_LEN_")))continue;
        auto size=row["decodedBytes"].toString().toLongLong();if(size<1||size>NativeResources::MaxOutput||size>qMin(AssetMemory::bytes()/4,qint64(128*1024*1024))-work)continue;work+=size;
        auto bytes=NativeResources::extract(source,patches,row,cancel);if(!bytes.error.isEmpty())continue;auto l=inspect(bytes.bytes,cancel);if(!l.valid)continue;
        auto c=candidate(l);c["name"]=name+" / collision";c["profileId"]="native-rtuned-collision-resource";c["collisionResource"]=row;c["objectOffset"]=row["offset"];auto settings=c["settings"].toObject();settings["vertexOffset"]=row["offset"];c["settings"]=settings;c["basis"]=c["basis"].toString()+" Decoded child offsets do not map directly to packed archive bytes.";out.append(c);
    }
    return out;
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &c,const std::atomic_bool &cancel){
    NativeResources::Output bytes;bool child=c.contains("collisionResource");if(child)bytes=NativeResources::extract(source,patches,c["collisionResource"].toObject(),cancel);else bytes.bytes=NativeResources::read(source,patches,0,source.expectedSize,cancel,bytes.error);
    if(!bytes.error.isEmpty())return {nullptr,bytes.error};auto out=decodeBytes(bytes.bytes,c,cancel);
    if(out.mesh&&child){out.mesh->vertexOffsets.clear();out.mesh->faceSources.clear();}
    return out;
}
}
