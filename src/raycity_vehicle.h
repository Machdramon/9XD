#pragma once
#include "raycity_cell.h"
#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QSet>
#include <functional>
namespace RayCityVehicle {
inline constexpr auto ProfileId="native-raycity-vehicle";
using RayCity::u16;using RayCity::u32;using RayCity::real;
struct Block {quint32 id=0,mode=0,checksum=0;qint64 offset=0,packed=0,size=0;};
struct File {QString name;quint32 id=0,property=0,key=0;qint64 size=0;};
struct Archive {QByteArray bytes;QMap<quint32,Block> blocks;QMap<QString,File> files;quint32 key=0;QString error;};
inline quint32 adler(const QByteArray &b){return quint32(adler32(0,reinterpret_cast<const Bytef *>(b.constData()),b.size()));}
inline QByteArray xorData(QByteArray b,quint32 key){quint32 seed=key^0x8473fbc1u;for(qsizetype i=0;i<b.size();++i){quint32 word=seed+quint32((i%64)/4)*0x8473fbc1u;b[i]=char(uchar(b[i])^uchar(word>>(8*(i%4))));}return b;}
inline QByteArray data(const Archive &a,quint32 id,quint32 key,QString &error,const std::atomic_bool &cancel){
    if(cancel.load()){error="RayCity operation canceled.";return {};}
    if(!a.blocks.contains(id)){error="Missing RayCity resource block.";return {};}
    const auto f=a.blocks[id];QByteArray d=a.bytes.mid(f.offset,f.packed);
    if(f.mode&2){QByteArray out(f.size,0);uLongf n=f.size;auto rc=uncompress(reinterpret_cast<Bytef *>(out.data()),&n,reinterpret_cast<const Bytef *>(d.constData()),d.size());if(rc!=Z_OK||n!=f.size){error="Invalid RayCity compressed resource.";return {};}d=out;}
    if(d.size()!=f.size){error="RayCity resource size mismatch.";return {};}
    if(f.mode&4)d=xorData(d,key);
    if(f.checksum&&adler(d)!=f.checksum){error="RayCity resource checksum mismatch.";return {};}
    if(f.mode==4&&a.blocks.contains(id+1)){auto next=a.blocks[id+1];if(next.mode||next.offset!=f.offset+f.packed||d.size()+next.size>qMin<qint64>(128*1024*1024,AssetMemory::bytes()/8)){error="Invalid RayCity split resource.";return {};}d+=a.bytes.mid(next.offset,next.packed);}
    return d;
}
inline bool text(const QByteArray &b,qint64 &p,QString &s){s.clear();for(int n=0;n<1024&&p+2<=b.size();++n){auto ch=u16(b,p);p+=2;if(!ch)return !s.isEmpty()&&!s.contains('/')&&!s.contains('\\')&&!s.contains("..")&&!s.contains(QChar(0));s+=QChar(ch);}return false;}
inline Archive inspect(const QByteArray &b,const std::atomic_bool &cancel){
    Archive a;a.bytes=b;auto fail=[&](QString e){a.error=e;a.files.clear();return a;};
    if(!b.startsWith(RayCity::magic(1))||b.size()<0x220)return fail("RayCity vehicle archive version is not supported.");
    auto descriptor=[&](qint64 p){std::array<quint32,8>d{};for(int j=0;j<8;++j)d[j]=u32(b,p+j*4)^(quint32(0xdfa47b56u)*quint32(j+1));return d;};
    auto root=descriptor(0x100);qint64 stop=qint64(root[1])*256;
    if(root[0]!=0xffffffffu||root[4]!=5||root[2]<8||root[2]>1024*1024||root[2]!=root[3]||stop<0x120||stop>b.size()-root[2]||stop>1024*1024)return fail("Invalid RayCity archive directory.");
    for(qint64 p=0x100;p+32<=stop;p+=32){if(cancel.load())return fail("RayCity archive canceled.");if(b.mid(p,32)==QByteArray(32,0))break;auto d=descriptor(p);Block f{d[0],d[4],d[5],qint64(d[1])*256,d[2],d[3]};if(a.blocks.contains(f.id)||f.mode>7||f.mode==1||f.mode==3||f.mode==6||f.offset<stop||f.offset>b.size()-f.packed||f.size<0||f.size>qMin<qint64>(128*1024*1024,AssetMemory::bytes()/8)||(!(f.mode&2)&&f.packed!=f.size))return fail("Invalid RayCity resource descriptor.");a.blocks[f.id]=f;}
    bool found=false;auto encrypted=b.mid(stop,root[2]);
    for(quint32 folders=0;folders<=128;++folders){quint32 seed=u32(encrypted,0)^folders;auto decoded=xorData(encrypted,seed^0x8473fbc1u);bool structure=true;qint64 q=4;QString value;
        if(adler(decoded)!=root[5]||u32(decoded,0)!=folders)continue;
        for(quint32 j=0;j<folders&&structure;++j){structure=text(decoded,q,value)&&q<=decoded.size()-4;q+=4;}
        if(!structure||q>decoded.size()-4)continue;auto count=u32(decoded,q);q+=4;if(count>8192)continue;
        for(quint32 j=0;j<count&&structure;++j){structure=text(decoded,q,value)&&q<=decoded.size()-16;q+=16;}
        if(structure&&q==decoded.size()){a.key=(seed^0x8473fbc1u)+0x41014ebfu;found=true;break;}}
    if(!found)return fail("RayCity root directory key/checksum did not validate.");
    QSet<quint32> visited;int total=0;std::function<bool(quint32,QString,int)> visit;
    visit=[&](quint32 id,QString prefix,int depth){if(depth>16||visited.contains(id)||visited.size()>1024||cancel.load()){a.error="RayCity directory cycle, depth limit or cancellation.";return false;}visited.insert(id);auto d=data(a,id,a.key-0x41014ebfu,a.error,cancel);if(!a.error.isEmpty()||d.size()<8)return false;qint64 p=4;quint32 folders=u32(d,0);if(folders>128)return false;QVector<QPair<QString,quint32>> children;
        for(quint32 i=0;i<folders;++i){QString name;if(!text(d,p,name)||p>d.size()-4)return false;children.append({name,u32(d,p)});p+=4;}
        if(p>d.size()-4)return false;auto files=u32(d,p);p+=4;if(files>8192||total+files>20000)return false;total+=files;
        for(quint32 i=0;i<files;++i){QString name;if(!text(d,p,name)||p>d.size()-16)return false;auto ext=u32(d,p),property=u32(d,p+4),index=u32(d,p+8),size=u32(d,p+12);p+=16;QByteArray extension;for(int j=0;j<4;++j){char ch=char(ext>>(j*8));if(!ch)break;if(!((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')))return false;extension+=ch;}if(extension.isEmpty()||!a.blocks.contains(index))return false;
            QByteArray encoded;for(QChar ch:name){char raw[2];qToLittleEndian<quint16>(ch.unicode(),raw);encoded.append(raw,2);}auto key=adler(encoded)+ext+a.key-0x7e2af33du;auto full=prefix+name+"."+QString::fromLatin1(extension);if(a.files.contains(full))return false;a.files[full]={full,index,property,key,size};}
        if(p!=d.size())return false;for(auto [name,child]:children)if(!visit(child,prefix+name+"/",depth+1))return false;return true;};
    if(!visit(0xffffffffu,{},0))return fail(a.error.isEmpty()?"Invalid RayCity named directory.":a.error);return a;
}
inline QByteArray fileData(const Archive &a,const QString &name,QString &error,const std::atomic_bool &cancel){if(!a.files.contains(name)){error="RayCity named file is missing.";return {};}auto f=a.files[name];auto d=data(a,f.id,f.key,error,cancel);if(error.isEmpty()&&d.size()!=f.size)error="RayCity named file size mismatch.";return error.isEmpty()?d:QByteArray();}
struct Part {quint16 vertices=0,indices=0,vertexStart=0,indexStart=0;quint8 render=0,flags=0,bone=0;};
struct Layout {bool valid=false;qint64 table=0,positions=0,normals=0,uv=0,indices=0;quint32 vertices=0,indexCount=0;QVector<Part> parts;};
inline Layout meshLayout(const QByteArray &b,const std::atomic_bool &cancel){
    Layout empty;if(!b.startsWith(QByteArray::fromHex("aa474703db0f")))return empty;
    for(qint64 p=10;p<qMin<qint64>(b.size()-10,65536);++p){if((p&255)==0&&cancel.load())return empty;quint32 n=u16(b,p);if(n<3)continue;qint64 normals=p+2+qint64(n)*12,tangent=normals+2+qint64(n)*12,uv=tangent+4,indices=uv+qint64(n)*8;if(indices>b.size()-2||u16(b,normals)!=n||u16(b,tangent)||u16(b,tangent+2)!=n)continue;quint32 ni=u16(b,indices);if(ni<3||ni%3||indices+2+qint64(ni)*2!=b.size())continue;
        for(int count=1;count<=qMin<qint64>(1024,(p-34)/28);++count){qint64 table=p-24-count*28-2;if(table<8||u16(b,table)!=count)continue;Layout r;r.table=table;r.positions=p+2;r.normals=normals+2;r.uv=uv;r.indices=indices+2;r.vertices=n;r.indexCount=ni;quint32 nextV=0,nextI=0;bool valid=true;
            for(int j=0;j<count&&valid;++j){auto d=table+2+j*28;Part part{u16(b,d+4),u16(b,d+6),u16(b,d+10),u16(b,d+26),quint8(b[d]),quint8(b[d+1]),quint8(b[d+2])};if(part.vertices<1||!part.indices||part.indices%3||part.vertexStart!=nextV||part.indexStart!=nextI||part.vertices>n-nextV||part.indices>ni-nextI||u16(b,d+14)!=part.vertexStart||u16(b,d+22)!=part.vertexStart)valid=false;
                for(quint32 k=0;k<part.indices&&valid;++k)if(u16(b,r.indices+qint64(part.indexStart+k)*2)>=part.vertices)valid=false;nextV+=part.vertices;nextI+=part.indices;r.parts.append(part);}
            if(!valid||nextV!=n||nextI!=ni)continue;
            for(int k=0;k<3;++k)if(!std::isfinite(real(b,p-24+k*4))||!std::isfinite(real(b,p-12+k*4))||real(b,p-24+k*4)>real(b,p-12+k*4))valid=false;
            for(quint32 j=0;j<n&&valid;++j){for(int k=0;k<3;++k){auto v=real(b,r.positions+qint64(j)*12+k*4);if(!std::isfinite(v)||std::abs(v)>1e7||v<real(b,p-24+k*4)-.02||v>real(b,p-12+k*4)+.02)valid=false;}for(int k=0;k<2;++k){auto v=real(b,r.uv+qint64(j)*8+k*4);if(!std::isfinite(v)||std::abs(v)>1e7)valid=false;}}
            if(valid){r.valid=true;return r;}
        }
    }return empty;
}
inline QMap<QString,QString> defaultFiles(const Archive &a,const std::atomic_bool &cancel){
    QMap<QString,QString> selected;for(int lod=2;lod>=0;--lod)if(a.files.contains(QString("body_%1.0m").arg(lod))){selected[QString("body_%1.0m").arg(lod)]={};break;}
    for(const auto &f:a.files)if(f.name.endsWith("/list.xml")){QString error;auto xml=fileData(a,f.name,error,cancel);if(!error.isEmpty())continue;QXmlStreamReader reader(xml);auto prefix=f.name.left(f.name.size()-8);
        while(!reader.atEnd()){reader.readNext();if(!reader.isStartElement()||reader.name()!=u"part")continue;auto attrs=reader.attributes();if(attrs.value("id")!=u"0")continue;auto model=attrs.value("mesh").toString(),texture=attrs.value("tex").toString();if(model.contains('/')||model.contains("..")||texture.contains('/')||texture.contains(".."))continue;for(int lod=2;lod>=0;--lod){auto name=prefix+model+QString("_%1.0m").arg(lod);if(a.files.contains(name)){selected[name]=prefix+texture+".png";break;}}}
    }return selected;
}
inline QJsonObject scan(const QByteArray &b,const std::atomic_bool &cancel,const QString &name){
    if(!b.startsWith(RayCity::magic(1)))return {};auto a=inspect(b,cancel);if(!a.error.isEmpty()||!a.files.contains("body_0.0m"))return {};QJsonArray rows,notes;auto files=defaultFiles(a,cancel);int missing=0;
    for(auto it=files.cbegin();it!=files.cend();++it){if(cancel.load())return {};QString error;auto raw=fileData(a,it.key(),error,cancel);auto layout=meshLayout(raw,cancel);if(!error.isEmpty()||!layout.valid){++missing;notes.append(it.key()+": unsupported vehicle mesh layout.");continue;}auto digest=QString::fromLatin1(QCryptographicHash::hash(raw,QCryptographicHash::Sha256).toHex());
        for(int i=0;i<layout.parts.size();++i){const auto &p=layout.parts[i];rows.append(QJsonObject{{"name",QString("RayCity / %1 / %2 / part %3").arg(name,it.key()).arg(i+1)},{"profileId",ProfileId},{"nativeDecoder",true},{"profileName","RayCity v1 vehicle rest pose"},{"vehicleFile",it.key()},{"vehiclePart",i},{"vehicleTexture",it.value()},{"vehicleDigest",digest},{"vertexCount",p.vertices},{"triangleCount",p.indices/3},{"objectOffset",QString::number(a.blocks[a.files[it.key()].id].offset)},{"settings",QJsonObject{{"vertexCount",p.vertices},{"indexCount",p.indices}}},{"basis","Named highest available LOD and part-list id 0. Rest-pose positions already include bind placement; local UInt16 triangle lists. Named XML PNG preview when available; body shader, masks, wheels and animation are not reconstructed."}});}
    }notes.append(QString("RayCity v1 vehicle: %1 default named components; %2 unsupported components. Source archive has %3 resources. Body shader/mask composition, wheels, upgrades and animation remain separate.").arg(files.size()).arg(missing).arg(a.files.size()));return {{"recognized",true},{"candidates",rows},{"notes",notes}};
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult out;auto fail=[&](QString e){out.mesh.reset();out.error=e;return out;};if(source.expectedSize<=0||source.expectedSize>qMin<qint64>(256*1024*1024,AssetMemory::bytes()/4))return fail("RayCity vehicle exceeds input budget.");QByteArray b;
    for(qint64 at=0;at<source.expectedSize;){if(cancel.load())return fail("RayCity decoding canceled.");auto q=source;q.offset=at;q.length=qMin<qint64>(1048576,source.expectedSize-at);auto read=readPage(q);if(!read.error.isEmpty()||read.bytes.size()!=q.length)return fail(read.error.isEmpty()?"Incomplete RayCity source.":read.error);applyPatches(read.bytes,at,patches);b+=read.bytes;at+=read.bytes.size();}
    auto a=inspect(b,cancel);if(!a.error.isEmpty())return fail(a.error);auto file=candidate["vehicleFile"].toString();QString error;auto raw=fileData(a,file,error,cancel);if(!error.isEmpty())return fail(error);if(QString::fromLatin1(QCryptographicHash::hash(raw,QCryptographicHash::Sha256).toHex())!=candidate["vehicleDigest"].toString())return fail("RayCity candidate changed. Scan again.");auto layout=meshLayout(raw,cancel);int index=candidate["vehiclePart"].toInt(-1);if(!layout.valid||index<0||index>=layout.parts.size())return fail("RayCity vehicle part is invalid.");auto p=layout.parts[index];auto mesh=std::make_shared<MeshData>();
    for(quint32 j=0;j<p.vertices;++j){auto n=p.vertexStart+j;MeshVertex v{};v.position[0]=real(raw,layout.positions+qint64(n)*12);v.position[1]=real(raw,layout.positions+qint64(n)*12+8);v.position[2]=-real(raw,layout.positions+qint64(n)*12+4);v.uv[0]=real(raw,layout.uv+qint64(n)*8);v.uv[1]=1.0-real(raw,layout.uv+qint64(n)*8+4);mesh->vertices.append(v);mesh->vertexOffsets.append(-1);}
    QVector<QVector3D> normals(p.vertices);for(quint32 j=0;j<p.indices;j+=3){if(cancel.load())return fail("RayCity decoding canceled.");quint32 indices[3];for(int k=0;k<3;++k){indices[k]=u16(raw,layout.indices+qint64(p.indexStart+j+k)*2);mesh->triangles.append(indices[k]);}auto v=[&](int k){auto &p=mesh->vertices[indices[k]];return QVector3D(p.position[0],p.position[1],p.position[2]);};auto n=QVector3D::crossProduct(v(1)-v(0),v(2)-v(0));for(auto i:indices)normals[i]+=n;}
    for(int j=0;j<mesh->vertices.size();++j){auto n=normals[j];if(!std::isfinite(n.lengthSquared())||n.lengthSquared()<1e-20)n=QVector3D(0,1,0);else n.normalize();for(int k=0;k<3;++k)mesh->vertices[j].normal[k]=n[k];}
    out.mesh=mesh;out.surface=std::make_shared<MeshSurface>();auto &s=*out.surface;s.material.name=file+QString(" / part %1").arg(index+1);s.material.sourceBindings={{"format","RayCity v1 vehicle"},{"resource",file},{"part",index},{"restPose",true},{"bindMatricesApplied",false},{"flipV",true},{"renderType",p.render},{"sourceFlags",p.flags},{"boneIndex",p.bone},{"gameShaderApplied",false},{"textureAssigned",false},{"sourceOffsetsUnavailable",true}};
    auto selected=defaultFiles(a,cancel);auto texture=selected.value(file);if(!texture.isEmpty()&&a.files.contains(texture)){auto png=fileData(a,texture,error,cancel);if(error.isEmpty()&&png.size()>=24&&png.startsWith(QByteArray::fromHex("89504e470d0a1a0a"))&&imageValidSize(qFromBigEndian<quint32>(png.constData()+16),qFromBigEndian<quint32>(png.constData()+20))){s.texture=QImage::fromData(png,"PNG");if(!s.texture.isNull()){s.textureName=texture;
                    // RayCity part PNG alpha is not a reliable opacity channel:
                    // e.g. rockstar rear bumper is entirely alpha-zero. Preserve
                    // source RGBA separately and use an opaque structural preview.
                    auto original=s.texture.convertToFormat(QImage::Format_ARGB32);s.textureLibrary[texture+" (source RGBA)"]=original;s.texture=original.copy();
                    for(int y=0;y<s.texture.height();++y){if(cancel.load())return fail("RayCity decoding canceled.");auto *row=reinterpret_cast<QRgb *>(s.texture.scanLine(y));for(int x=0;x<s.texture.width();++x)row[x]|=0xff000000u;}
                    s.textureLibrary[texture+" (opaque preview)"]=s.texture;s.material.sourceBindings["sourceAlphaPreserved"]=true;s.material.sourceBindings["previewAlphaPolicy"]="Opaque preview; source alpha is not interpreted as opacity. Original shader masks pending.";s.material.sourceBindings["textureAssigned"]=true;s.material.sourceBindings["textureBinding"]="Named part-list tex PNG preview; original material stages not reproduced.";}}}
    auto q=source;q.offset=0;q.length=0;auto final=readPage(q);if(!final.error.isEmpty())return fail(final.error);if(cancel.load())return fail("RayCity decoding canceled.");return out;
}
}
