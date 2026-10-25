#pragma once
#include "mesh_profiles.h"
#include "native_resources.h"
#include <algorithm>
// NFSU2 PC scenery: bounded chunk traversal, validated solids and explicit instances.
// Colors/shaders/textures are not yet represented by MeshData.
namespace Nfsu2Scene {
inline qint64 instanceLimit(){return AssetMemory::bytes()/16384;}
inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel){
    QJsonArray scenes,notes,regions; if(b.size()<8)return {};
    auto u=[&](qint64 p){return qFromLittleEndian<quint32>(b.constData()+p);};
    if(u(0)!=0&&u(0)!=0x80134000)return {};
    struct Chunk{quint32 id;qint64 offset,start,end;};QVector<Chunk> chunks;bool valid=true;
    std::function<void(qint64,qint64,int)> walk=[&](qint64 p,qint64 end,int depth){if(depth>12){valid=false;return;}while(p<end&&valid){if(cancel.load()||end-p<8||chunks.size()>=200000){valid=false;return;}auto id=u(p);qint64 stop=p+8+qint64(u(p+4));if(stop>end){valid=false;return;}if(id)chunks.append({id,p,p+8,stop});if(id&0x80000000)walk(p+8,stop,depth+1);p=stop;}};walk(0,b.size(),0);
    bool recognized=false;for(auto c:chunks)recognized|=c.id==0x80034100;if(!recognized)return {};
    if(!valid)return {{"recognized",true},{"error","NFSU2 scenery has an incomplete or oversized chunk tree."}};
    auto builtin=MeshProfiles::builtins();if(builtin.isEmpty())return {{"recognized",true},{"error","NFSU2 solid profile unavailable."}};
    QMap<quint32,QJsonObject> models;QSet<quint32> ambiguous;int rejected=0;QMap<quint32,QByteArray> signatures;
    for(auto c:chunks){if(cancel.load())return {{"error","NFSU2 scan canceled."}};if(c.id!=0x80134010)continue;
        if(models.size()>=20000)return {{"recognized",true},{"error","NFSU2 solid limit reached."}};
        qint64 prefix=c.offset%128;if(prefix<8)prefix+=128;QByteArray local(prefix,0);qToLittleEndian<quint32>(prefix-8,local.data()+4);local+=b.mid(c.offset,c.end-c.offset);QJsonObject candidate;
        for(int stride:{24,36}){auto profile=builtin.first().toObject();profile["rootChunk"]=0;profile["applyTransform"]=false;auto v=profile["vertices"].toObject();v["stride"]=stride;v["normalOffset"]=stride==24?-1:12;v["uvOffset"]=stride==24?16:28;profile["vertices"]=v;auto found=MeshProfiles::scan(local,{profile},cancel)["candidates"].toArray();if(!found.isEmpty()){candidate=found.first().toObject();break;}}
        if(candidate.isEmpty()){++rejected;continue;}
        qint64 h=-1;for(auto it=std::lower_bound(chunks.cbegin(),chunks.cend(),c.start,[](const Chunk &x,qint64 p){return x.offset<p;});it!=chunks.cend()&&it->offset<c.end;++it){auto x=*it;if(x.id!=0x134011||x.end>c.end)continue;h=(x.start+15)&~qint64(15);if(h+20>x.end)h=-1;break;}if(h<0){++rejected;continue;}
        auto settings=candidate["settings"].toObject();qint64 shift=c.offset-prefix;for(auto key:{"vertexOffset","indexOffset","uvOffset","normalOffset"})if(settings.contains(key))settings[key]=QString::number(settings[key].toString().toLongLong()+shift);candidate["settings"]=settings;candidate["objectOffset"]=QString::number(c.offset);candidate.remove("sourceTransform");
        auto key=u(h+16);auto signature=b.mid(settings["vertexOffset"].toString().toLongLong(),settings["vertexCount"].toInteger()*settings["vertexStride"].toInteger())+b.mid(settings["indexOffset"].toString().toLongLong(),settings["indexCount"].toInteger()*2)+QJsonDocument(candidate["materialGroups"].toArray()).toJson(QJsonDocument::Compact);
        if(models.contains(key)&&signatures[key]!=signature)ambiguous.insert(key);else {models[key]=candidate;signatures[key]=signature;}
    }
    auto cleanStart=[&](Chunk c,int alignment){auto s=c.start;while(s+4<=c.end&&u(s)==0x11111111)s+=4;return (s+alignment-1)&~qint64(alignment-1);};
    QJsonArray all; qint64 totalVertices=0,totalIndices=0;int unresolved=0,sectorCount=0;bool wholeFits=true;
    for(auto section:chunks){if(section.id!=0x80034100)continue;if(cancel.load())return {{"error","NFSU2 scan canceled."}};
        Chunk info{},instance{},header{};int ni=0,nn=0,nh=0;for(auto it=std::lower_bound(chunks.cbegin(),chunks.cend(),section.start,[](const Chunk &x,qint64 p){return x.offset<p;});it!=chunks.cend()&&it->offset<section.end;++it){auto c=*it;if(c.end>section.end)continue;if(c.id==0x34102){info=c;++ni;}if(c.id==0x34103){instance=c;++nn;}if(c.id==0x34101){header=c;++nh;}}
        if(ni!=1||nn!=1||nh!=1)return {{"recognized",true},{"error","NFSU2 scenery section has missing or duplicate tables."}};
        auto is=cleanStart(info,1),ns=cleanStart(instance,16),hs=cleanStart(header,1);if((info.end-is)%68||(instance.end-ns)%64||hs+16>header.end)return {{"recognized",true},{"error","Invalid NFSU2 scenery record sizes."}};
        auto number=u(hs+12);++sectorCount;QJsonArray entries;qint64 vertices=0,indices=0;int part=0;
        auto emitPart=[&]{if(entries.isEmpty())return;scenes.append(QJsonObject{{"name",QString("NFSU2 sector %1 / part %2").arg(number).arg(++part)},{"profileId","native-nfsu2-scene"},{"profileName","NFSU2 PC scenery"},{"nfsu2Instances",entries},{"settings",QJsonObject{{"vertexOffset",QString::number(section.offset)},{"vertexCount",vertices},{"indexCount",indices}}},{"basis","Assembled sector using stored mesh keys and instance transforms. Large sectors are split into complete instance batches. UVs retained; textures, vertex colors and shaders pending."}});entries={};vertices=0;indices=0;};
        for(qint64 p=ns;p<instance.end;p+=64){if(cancel.load())return {{"error","NFSU2 scan canceled."}};auto ix=qFromLittleEndian<quint16>(b.constData()+p+24);if(ix>=(info.end-is)/68)return {{"recognized",true},{"error","NFSU2 instance references an invalid definition."}};auto key=u(is+ix*68+32);if(!models.contains(key)||ambiguous.contains(key)){++unresolved;continue;}auto candidate=models[key];auto settings=candidate["settings"].toObject();QJsonArray matrix;bool sane=true;
            for(int row=0;row<4;++row)for(int col=0;col<4;++col){double value=0;if(row<3&&col<3)value=qFromLittleEndian<qint16>(b.constData()+p+44+(row*3+col)*2)/8192.;else if(row==3&&col<3){auto bits=u(p+32+col*4);float f;std::memcpy(&f,&bits,4);value=f;}else if(row==3&&col==3)value=1;sane&=std::isfinite(value)&&std::abs(value)<1e8;matrix.append(value);}
            if(!sane)return {{"recognized",true},{"error","Nonfinite NFSU2 instance transform."}};
            if(entries.size()>=instanceLimit()||!AssetMemory::fits(vertices+settings["vertexCount"].toInteger(),indices+settings["indexCount"].toInteger()))emitPart();
            candidate["sourceTransform"]=matrix;candidate["instanceOffset"]=QString::number(p);candidate["name"]=candidate["name"].toString()+QString(" @ %1").arg(p);entries.append(candidate);vertices+=settings["vertexCount"].toInteger();indices+=settings["indexCount"].toInteger();
            if(!candidate["name"].toString().startsWith("SKYDOME @")){totalVertices+=settings["vertexCount"].toInteger();totalIndices+=settings["indexCount"].toInteger();if(wholeFits){if(all.size()>=instanceLimit()||!AssetMemory::fits(totalVertices,totalIndices)){wholeFits=false;all={};}else all.append(candidate);}}

        }
        emitPart();
        if(regions.size()<24)regions.append(QJsonObject{{"name",QString("NFSU2 scenery sector %1").arg(number)},{"offset",QString::number(section.offset)},{"length",QString::number(section.end-section.offset)},{"step",QString::number(section.end-section.offset)},{"repetitions",1},{"evidence","Bounded scenery definitions and instance references"}});
    }
    if(wholeFits&&!all.isEmpty())scenes.prepend(QJsonObject{{"name","NFSU2 scene (sky excluded)"},{"profileId","native-nfsu2-scene"},{"profileName","NFSU2 PC scenery"},{"nfsu2Instances",all},{"settings",QJsonObject{{"vertexOffset","0"},{"vertexCount",totalVertices},{"indexCount",totalIndices}}},{"basis","Stored triangle lists assembled with per-instance transforms; sky excluded for useful framing. Sector results are alternatives, not additional geometry. Textures, vertex colors and shaders pending."}});
    notes.append(QString("NFSU2 scenery: %1 mesh keys, %2 sectors; %3 unsupported solids, %4 unresolved/ambiguous instances. Preview size follows Settings > Performance; sectors remain available for larger scenes.").arg(models.size()).arg(sectorCount).arg(rejected).arg(unresolved));
    if(!wholeFits)notes.append("The complete scene exceeds the single-preview budget. All supported sectors remain available; load selected sector parts instead. No global instance limit was applied to discovery.");
    return {{"recognized",true},{"candidates",scenes},{"notes",notes},{"regions",regions}};
}
inline QJsonObject inspect(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel){
    if(source.expectedSize>AssetMemory::bytes()/4)return {{"notes",QJsonArray{"NFSU2 stream exceeds the configured scan-input budget; raise Settings > Performance."}}};QString error;auto bytes=NativeResources::read(source,patches,0,source.expectedSize,cancel,error,AssetMemory::bytes()/4);if(!error.isEmpty())return {{"error",error}};return scan(bytes,cancel);
}
}
