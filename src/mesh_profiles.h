#pragma once
#include <QJsonDocument>
#include "memory_budget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QMap>
#include <QVector>
#include <QSet>
#include <cstring>
#include <QSettings>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QtEndian>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
namespace MeshProfiles {
inline qint64 integer(QJsonValue value,qint64 fallback=-1){if(value.isDouble()){double n=value.toDouble();return std::isfinite(n)&&n==std::floor(n)&&n>=-1&&n<=4294967295.?qint64(n):fallback;}if(value.isString()){bool ok=false;auto s=value.toString();auto n=s.toLongLong(&ok,s.startsWith("0x",Qt::CaseInsensitive)?16:10);if(ok&&n>=0&&n<=4294967295LL)return n;}return fallback;}
inline bool validate(const QJsonObject &p,QString &error){
    auto fail=[&](QString s){error=s;return false;};if(QJsonDocument(p).toJson(QJsonDocument::Compact).size()>65536)return fail("Profile exceeds 64 KiB.");
    const QStringList top{"format","version","id","name","parser","rootChunk","objectChunk","header","descriptor","vertices","indices","materials","applyTransform","rotationDegrees"};for(auto i=p.begin();i!=p.end();++i)if(!top.contains(i.key()))return fail("Unknown profile field: "+i.key());
    if(p["format"]!="9XD Mesh Profile"||integer(p["version"])!=1||p["parser"]!="chunked-indexed-v1")return fail("Unsupported profile format, version or parser.");
    if(!QRegularExpression("^[a-z0-9][a-z0-9_-]{0,63}$").match(p["id"].toString()).hasMatch()||p["name"].toString().trimmed().isEmpty()||p["name"].toString().size()>120)return fail("Use a short name and an id containing lowercase letters, numbers, underscores or hyphens.");
    if(integer(p["rootChunk"])<0||integer(p["objectChunk"])<0||!p["applyTransform"].isBool())return fail("Missing chunk IDs or applyTransform boolean.");
    const QMap<QString,QStringList> groups{{"header",{"chunk","alignment","versionOffset","versionValue","nameOffset","nameBytes","matrixOffset"}},{"descriptor",{"chunk","alignment","vertexCountOffset","triangleCountOffset"}},{"vertices",{"chunk","alignment","stride","positionOffset","normalOffset","uvOffset"}},{"indices",{"chunk","alignment","bytes"}},{"materials",{"chunk","alignment","stride","countOffset","startOffset","textureSlotOffset","shaderOffset"}}};
    for(auto group=groups.begin();group!=groups.end();++group){if(group.key()=="materials"&&!p.contains("materials"))continue;if(!p[group.key()].isObject())return fail("Missing object: "+group.key());auto g=p[group.key()].toObject();for(auto i=g.begin();i!=g.end();++i)if(!group.value().contains(i.key()))return fail("Unknown "+group.key()+" field: "+i.key());for(auto key:group.value())if(!g.contains(key)||integer(g[key],-2)<(key=="matrixOffset"||key=="normalOffset"||key=="uvOffset"?-1:0))return fail("Invalid "+group.key()+"."+key);auto a=integer(g["alignment"]);if(a<1||a>256||(a&(a-1)))return fail("Alignment must be a power of two from 1 to 256.");}
    auto h=p["header"].toObject(),v=p["vertices"].toObject();auto stride=integer(v["stride"]);if(stride<12||stride>256||integer(h["nameBytes"])<1||integer(h["nameBytes"])>128||integer(h["versionValue"])>255)return fail("Invalid stride, name length or version byte.");for(auto key:{"positionOffset","normalOffset","uvOffset"}){auto n=integer(v[key]);if(n>=0&&n+(QString(key)=="uvOffset"?8:12)>stride)return fail("Vertex attribute exceeds its record: "+QString(key));}
    if(integer(p["indices"].toObject()["bytes"])!=2&&integer(p["indices"].toObject()["bytes"])!=4)return fail("Index bytes must be 2 or 4.");
    if(p.contains("rotationDegrees")){auto values=p["rotationDegrees"].toArray();if(values.size()!=3)return fail("rotationDegrees must contain XYZ angles.");for(auto value:values)if(!value.isDouble()||!std::isfinite(value.toDouble())||std::abs(value.toDouble())>360)return fail("Profile rotations must be finite angles from -360 to 360.");}
    if(p.contains("materials")){auto m=p["materials"].toObject();auto n=integer(m["stride"]);if(n<4||n>256)return fail("Invalid material stride.");for(auto key:{"countOffset","startOffset","textureSlotOffset","shaderOffset"})if(integer(m[key])+4>n)return fail("Material field exceeds its record.");}
    return true;
}
inline QSettings settings(){auto file=QCoreApplication::instance()?QCoreApplication::instance()->property("workspaceSettingsFile").toString():QString();if(file.isEmpty())return QSettings("BinaryWorkbench","Workspace");return QSettings(file,QSettings::IniFormat);}
inline QJsonArray builtins(){QFile f(":/mesh-profiles/nfsu2-pc.json");if(!f.open(QIODevice::ReadOnly))return {};auto p=QJsonDocument::fromJson(f.readAll()).object();QString error;return validate(p,error)?QJsonArray{p}:QJsonArray();}
inline QJsonArray users(){return QJsonDocument::fromJson(settings().value("profiles/mesh/v1").toByteArray()).array();}
inline QJsonArray installed(){auto out=builtins();for(auto v:users()){QString error;auto p=v.toObject();if(validate(p,error)&&p["id"].toString().startsWith("custom-"))out.append(p);}return out;}
inline QJsonArray active(){auto id=settings().value("profiles/mesh/selected").toString();auto all=installed();if(!id.isEmpty())for(auto value:all)if(value.toObject()["id"].toString()==id)return {value};return all;}
inline bool save(const QJsonObject &p,QString &error){if(!validate(p,error))return false;if(!p["id"].toString().startsWith("custom-")){error="User profile IDs must start with custom-. Included profiles are read-only; make a copy.";return false;}auto all=users();bool replaced=false;for(int i=0;i<all.size();++i)if(all[i].toObject()["id"]==p["id"]){all[i]=p;replaced=true;break;}if(!replaced){if(all.size()>=32){error="At most 32 user profiles can be installed.";return false;}all.append(p);}auto s=settings();s.setValue("profiles/mesh/v1",QJsonDocument(all).toJson(QJsonDocument::Compact));s.setValue("profiles/mesh/selected",p["id"].toString());s.sync();if(s.status()!=QSettings::NoError){error="Could not save profile settings.";return false;}return true;}
struct Chunk {quint32 id; qint64 start,end;};
inline QJsonObject scan(const QByteArray &bytes,const QJsonArray &profiles,const std::atomic_bool &cancel){
    QJsonArray candidates,notes,matches,regions;QSet<QString> seen;if(bytes.size()<8)return {{"candidates",candidates}};
    auto u32=[&](qint64 p){return qFromLittleEndian<quint32>(bytes.constData()+p);};
    for(auto value:profiles){auto profile=value.toObject();QString error;if(!validate(profile,error)){notes.append(error);continue;}if(u32(0)!=quint32(integer(profile["rootChunk"])))continue;
        QVector<Chunk> chunks;bool valid=true;std::function<void(qint64,qint64,int)> walk=[&](qint64 p,qint64 end,int depth){if(depth>12){valid=false;return;}while(p<end&&valid){if(cancel.load()){valid=false;return;}if(end-p<8||chunks.size()>=200000){valid=false;return;}quint32 id=u32(p);qint64 stop=p+8+qint64(u32(p+4));if(stop>end){valid=false;return;}chunks.append({id,p+8,stop});if((id&0x80000000)&&stop>p+8)walk(p+8,stop,depth+1);p=stop;}};walk(0,bytes.size(),0);if(!valid){notes.append(profile["name"].toString()+": incomplete, oversized or invalid chunk tree.");continue;}
        auto header=profile["header"].toObject(),desc=profile["descriptor"].toObject(),vertex=profile["vertices"].toObject(),index=profile["indices"].toObject(),material=profile["materials"].toObject();int accepted=0,rejected=0;quint64 vertices=0,triangles=0;
        for(auto object:chunks){if(cancel.load())break;if(object.id!=quint32(integer(profile["objectChunk"])))continue;if(accepted+rejected>=2048||candidates.size()>=2048){notes.append("Profile object limit reached (2048).");break;}
            auto locate=[&](const QJsonObject &g)->Chunk{auto id=quint32(integer(g["chunk"]));Chunk found{0,-1,-1};for(auto c:chunks)if(c.id==id&&c.start>object.start&&c.end<=object.end){if(found.start>=0)return {0,-1,-1};found=c;}if(found.start>=0){auto a=integer(g["alignment"]);found.start=(found.start+a-1)&~(a-1);if(found.start>found.end)found.start=-1;}return found;};
            auto h=locate(header),d=locate(desc),v=locate(vertex),idx=locate(index);auto fits=[](Chunk c,qint64 n,qint64 width){return c.start>=0&&n>=0&&width>=0&&n<=c.end-c.start&&width<=c.end-c.start-n;};
            auto readField=[&](Chunk c,const QJsonObject &g,const char *key)->qint64{auto offset=integer(g[key]);return fits(c,offset,4)?qint64(u32(c.start+offset)):-1;};
            if(!fits(h,integer(header["versionOffset"]),1)||quint8(bytes[h.start+integer(header["versionOffset"])])!=integer(header["versionValue"])){++rejected;continue;}
            auto count=readField(d,desc,"vertexCountOffset"),faces=readField(d,desc,"triangleCountOffset"),stride=integer(vertex["stride"]),width=integer(index["bytes"]);if(count<3||count>AssetMemory::vertices()||faces<1||faces>AssetMemory::indices()/3||!fits(v,0,count*stride)||v.end-v.start!=count*stride||!fits(idx,0,faces*3*width)){++rejected;continue;}
            bool sane=true;for(qint64 n=0;n<count&&sane;++n)for(auto attribute:{"positionOffset","normalOffset","uvOffset"}){auto offset=integer(vertex[attribute]);if(offset<0)continue;for(int a=0;a<(QString(attribute)=="uvOffset"?2:3);++a){auto bits=u32(v.start+n*stride+offset+a*4);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::abs(double(f))>1e8)sane=false;}}
            for(qint64 n=0;n<faces*3&&sane;++n){quint32 i=width==2?qFromLittleEndian<quint16>(bytes.constData()+idx.start+n*2):u32(idx.start+n*4);if(i>=count)sane=false;}
            QJsonArray groups;if(!material.isEmpty()){auto m=locate(material);auto ms=integer(material["stride"]);if(m.start<0||(m.end-m.start)%ms||(m.end-m.start)/ms>512)sane=false;else{qint64 consumed=0;for(qint64 p=m.start;p<m.end&&sane;p+=ms){auto length=qint64(u32(p+integer(material["countOffset"]))),start=qint64(u32(p+integer(material["startOffset"])));if(start!=consumed||length%3||start>faces*3||length>faces*3-start)sane=false;else groups.append(QJsonObject{{"indexStart",start},{"indexCount",length},{"textureSlot",qint64(u32(p+integer(material["textureSlotOffset"])))},{"shader",qint64(u32(p+integer(material["shaderOffset"])))}});consumed+=length;}if(consumed!=faces*3)sane=false;}}
            QJsonArray matrix;auto mo=integer(header["matrixOffset"]);if(profile["applyTransform"].toBool()&&mo>=0){if(!fits(h,mo,64))sane=false;else for(int a=0;a<16;++a){auto bits=u32(h.start+mo+a*4);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::abs(double(f))>1e8)sane=false;matrix.append(f);}if(matrix.size()==16&&(std::abs(matrix[3].toDouble())>1e-6||std::abs(matrix[7].toDouble())>1e-6||std::abs(matrix[11].toDouble())>1e-6||std::abs(matrix[15].toDouble()-1)>1e-6))sane=false;}
            if(!sane){++rejected;continue;}auto no=integer(header["nameOffset"]);QString name;if(fits(h,no,1)){auto raw=bytes.mid(h.start+no,qMin(integer(header["nameBytes"]),h.end-h.start-no));auto zero=raw.indexOf('\0');if(zero>=0)raw.truncate(zero);name=QString::fromLatin1(raw).trimmed();}if(name.isEmpty())name=QString("Object at 0x%1").arg(object.start-8,0,16);
            auto pos=v.start+integer(vertex["positionOffset"]),normal=integer(vertex["normalOffset"]),uv=integer(vertex["uvOffset"]);
            QJsonObject settings{{"vertexOffset",QString::number(pos)},{"vertexCount",count},{"vertexStride",stride},{"positionType","Float32"},{"positionScale",1},{"indexOffset",QString::number(idx.start)},{"indexCount",faces*3},{"indexStride",width},{"indexType",width==2?"UInt16":"UInt32"},{"topology","Triangle list"},{"uv",uv>=0},{"uvOffset",QString::number(v.start+qMax(qint64(0),uv))},{"uvStride",stride},{"uvType","Float32"},{"normals",normal>=0},{"normalOffset",QString::number(v.start+qMax(qint64(0),normal))},{"normalStride",stride},{"normalType","Float32"}};
            auto key=QString::number(pos)+":"+QString::number(idx.start)+":"+QString::number(count)+":"+QString::number(faces);if(seen.contains(key))continue;seen.insert(key);
            candidates.append(QJsonObject{{"name",name},{"profileId",profile["id"]},{"profileName",profile["name"]},{"objectOffset",QString::number(object.start-8)},{"sourceTransform",matrix},{"profileRotation",profile["rotationDegrees"]},{"materialGroups",groups},{"settings",settings},{"basis","Validated configuration profile; object matrix applied by profile preview. Material/texture assignments unresolved; alternatives may overlap."}});++accepted;vertices+=count;triangles+=faces;
            if(regions.size()<24)regions.append(QJsonObject{{"name",profile["name"].toString()+" / "+name},{"offset",QString::number(object.start-8)},{"length",QString::number(object.end-object.start+8)},{"step",QString::number(object.end-object.start+8)},{"repetitions",1},{"evidence","Matched bounded object, counts, attribute and index validation"}});
        }
        matches.append(QJsonObject{{"id",profile["id"]},{"name",profile["name"]},{"objects",accepted},{"rejectedObjects",rejected},{"vertices",qint64(vertices)},{"triangles",qint64(triangles)}});
    }
    return {{"candidates",candidates},{"matches",matches},{"notes",notes},{"regions",regions}};
}
}
