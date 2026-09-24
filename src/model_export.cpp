#include "model_export.h"
#include <QSaveFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QtEndian>
#include <cmath>
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace {
struct Cancelled {};
void checkpoint(const std::shared_ptr<ExportProgress> &p){if(p&&p->cancel.load())throw Cancelled{};}
void fail(const QString &s){throw std::runtime_error(s.toStdString());}
void u32(QByteArray &b,quint32 value){quint32 le=qToLittleEndian(value);b.append(reinterpret_cast<const char *>(&le),4);}
void f32(QByteArray &b,float value){u32(b,std::bit_cast<quint32>(value));}
void write(QIODevice &f,const QByteArray &b){if(f.write(b)!=b.size())fail("No se pudo escribir el archivo: "+f.errorString());}
QString canonical(const QString &path){QFileInfo f(path);return f.exists()?f.canonicalFilePath():f.absoluteFilePath();}
QString number(float v){return QString::number(v,'g',9);}
QByteArray vectorLine(const char *prefix,const float *v,int n){QByteArray line(prefix);for(int j=0;j<n;++j){line+=' ';line+=number(v[j]).toLatin1();}return line+'\n';}
std::array<double,3> faceNormal(const MeshVertex &a,const MeshVertex &b,const MeshVertex &c){std::array<double,3> u,v;for(int j=0;j<3;++j){u[j]=double(b.position[j])-a.position[j];v[j]=double(c.position[j])-a.position[j];}return {u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};}
MeshData prepare(const MeshData &input,const ModelExportOptions &o,const std::shared_ptr<ExportProgress> &progress){
    if(input.vertices.isEmpty()||input.triangles.isEmpty()||input.triangles.size()%3)fail("Hay una malla vacía o con triángulos incompletos.");
    MeshData m;m.hasUvs=input.hasUvs&&o.uvs&&o.format!=ModelFormat::Stl;m.vertices=input.vertices;m.triangles=input.triangles;
    int inversions=0;for(int i=0;i<3;++i)for(int j=i+1;j<3;++j)inversions+=o.axes[i]>o.axes[j];for(bool n:o.negate)inversions+=n;
    bool flip=(inversions%2!=0)!=o.reverseFaces;
    for(qsizetype i=0;i<m.vertices.size();++i){if(i%1024==0)checkpoint(progress);auto original=m.vertices[i];auto &v=m.vertices[i];
        for(int j=0;j<3;++j){double value=double(original.position[o.axes[j]])*o.scale*(o.negate[j]?-1:1);v.position[j]=float(value);if(!std::isfinite(value)||!std::isfinite(v.position[j]))fail("Posición no finita o fuera del rango Float32 tras transformar.");v.normal[j]=original.normal[o.axes[j]]*(o.negate[j]?-1:1)*(o.reverseFaces?-1:1);}
        if(m.hasUvs){if(o.flipV)v.uv[1]=1-v.uv[1];for(float value:v.uv)if(!std::isfinite(value))fail("Coordenada UV no finita.");}
    }
    for(qsizetype i=0;i<m.triangles.size();i+=3){if(i%3072==0)checkpoint(progress);for(int j=0;j<3;++j)if(m.triangles[i+j]>=m.vertices.size())fail("Índice fuera de la malla.");if(flip)std::swap(m.triangles[i+1],m.triangles[i+2]);}
    if(o.format!=ModelFormat::Stl&&o.normals!=ExportNormals::Omit){
        if(o.normals==ExportNormals::Recalculate){QVector<std::array<double,3>> sums(m.vertices.size());for(qsizetype i=0;i<m.triangles.size();i+=3){if(i%3072==0)checkpoint(progress);auto n=faceNormal(m.vertices[m.triangles[i]],m.vertices[m.triangles[i+1]],m.vertices[m.triangles[i+2]]);for(int k=0;k<3;++k)for(int j=0;j<3;++j)sums[m.triangles[i+k]][j]+=n[j];}for(qsizetype i=0;i<m.vertices.size();++i){auto n=sums[i];double length=std::hypot(n[0],n[1],n[2]);if(length<1e-30||!std::isfinite(length))fail("No se puede calcular alguna normal (vértice aislado o caras degeneradas). Elige no exportar normales.");for(int j=0;j<3;++j)m.vertices[i].normal[j]=float(n[j]/length);}}
        else for(auto &v:m.vertices){double length=std::hypot(double(v.normal[0]),double(v.normal[1]),double(v.normal[2]));if(length<1e-30||!std::isfinite(length))fail("La malla contiene normales nulas/no válidas. Recalcula u omite las normales.");for(float &n:v.normal)n=float(n/length);}
    }return m;
}
}
ModelExportResult exportModel(const QString &path,const QVector<ExportMesh> &meshes,const ModelExportOptions &o,const std::shared_ptr<ExportProgress> &progress){
    ModelExportResult result;
    try{
        checkpoint(progress);if(meshes.isEmpty())fail("Selecciona al menos una malla.");
        if(!std::isfinite(o.scale)||o.scale<=0)fail("La escala debe ser positiva y finita.");auto axes=o.axes;std::sort(axes.begin(),axes.end());if(axes!=std::array<int,3>{0,1,2})fail("Los ejes deben ser una permutación de XYZ.");
        if(o.format!=ModelFormat::Obj&&o.format!=ModelFormat::Glb&&o.format!=ModelFormat::Stl)fail("Formato no admitido.");
        for(const auto &source:o.protectedPaths)if(!source.isEmpty()&&canonical(source)==canonical(path))fail("El destino coincide con un archivo binario abierto o usado por las mallas.");
        for(const auto &e:meshes){if(!e.source.isEmpty()&&canonical(e.source)==canonical(path))fail("No se puede sobrescribir el binario de origen al exportar.");if(!e.data)fail("Malla no disponible.");result.vertices+=e.data->vertices.size();result.faces+=e.data->triangles.size()/3;}
        if(result.vertices>2000000||result.faces>4000000)fail("Esta versión permite hasta 2 millones de vértices y 4 millones de triángulos por exportación.");
        QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))fail(file.errorString());
        QByteArray bin;QJsonArray nodes,gltfMeshes,accessors,views,sceneNodes;
        quint64 vertexBase=1,uvBase=1,normalBase=1;
        if(o.format==ModelFormat::Obj)write(file,"# Binary Workbench: triangulated geometry; no textures/materials\n");
        if(o.format==ModelFormat::Stl){QByteArray header(80,0);header.replace(0,16,"Binary Workbench");u32(header,quint32(result.faces));write(file,header);}
        auto accessor=[&](QByteArray bytes,int count,int components,bool indices,QJsonArray minimum={},QJsonArray maximum={}){
            while(bin.size()%4)bin.append('\0');int view=views.size();views.append(QJsonObject{{"buffer",0},{"byteOffset",double(bin.size())},{"byteLength",double(bytes.size())},{"target",indices?34963:34962}});bin+=bytes;
            QJsonObject a{{"bufferView",view},{"componentType",indices?5125:5126},{"count",count},{"type",components==1?"SCALAR":components==2?"VEC2":"VEC3"}};if(!minimum.isEmpty()){a["min"]=minimum;a["max"]=maximum;}int id=accessors.size();accessors.append(a);return id;
        };
        for(int meshIndex=0;meshIndex<meshes.size();++meshIndex){checkpoint(progress);const auto &e=meshes[meshIndex];auto m=prepare(*e.data,o,progress);bool normals=o.normals!=ExportNormals::Omit;QString name=e.name.isEmpty()?QString("Mesh_%1").arg(meshIndex):e.name;
            if(o.format==ModelFormat::Obj){name.replace(QRegularExpression("[\\s#\\\\]+"),"_");write(file,"o "+name.toUtf8()+"\n");QByteArray chunk;auto flush=[&]{if(chunk.size()>=65536){checkpoint(progress);write(file,chunk);chunk.clear();}};
                for(auto v:m.vertices){chunk+=vectorLine("v",v.position,3);flush();}if(m.hasUvs)for(auto v:m.vertices){chunk+=vectorLine("vt",v.uv,2);flush();}if(normals)for(auto v:m.vertices){chunk+=vectorLine("vn",v.normal,3);flush();}
                for(qsizetype i=0;i<m.triangles.size();i+=3){chunk+='f';for(int j=0;j<3;++j){auto index=m.triangles[i+j];chunk+=' ';chunk+=QByteArray::number(vertexBase+index);if(m.hasUvs||normals){chunk+='/';if(m.hasUvs)chunk+=QByteArray::number(uvBase+index);if(normals){chunk+='/';chunk+=QByteArray::number(normalBase+index);}}}chunk+='\n';flush();}write(file,chunk);vertexBase+=m.vertices.size();if(m.hasUvs)uvBase+=m.vertices.size();if(normals)normalBase+=m.vertices.size();
            }else if(o.format==ModelFormat::Stl){QByteArray chunk;for(qsizetype i=0;i<m.triangles.size();i+=3){if(i%3072==0)checkpoint(progress);const auto &a=m.vertices[m.triangles[i]],&b=m.vertices[m.triangles[i+1]],&c=m.vertices[m.triangles[i+2]];auto n=faceNormal(a,b,c);double length=std::hypot(n[0],n[1],n[2]);for(double v:n)f32(chunk,length>0?float(v/length):0);for(auto *vertex:{&a,&b,&c})for(float v:vertex->position)f32(chunk,v);chunk.append(2,'\0');if(chunk.size()>=65536){write(file,chunk);chunk.clear();}}write(file,chunk);
            }else{
                QByteArray positions,normalData,uvData,indexData;std::array<float,3> low{},high{};for(int j=0;j<3;++j)low[j]=high[j]=m.vertices.first().position[j];
                for(qsizetype i=0;i<m.vertices.size();++i){if(i%1024==0)checkpoint(progress);auto v=m.vertices[i];for(int j=0;j<3;++j){f32(positions,v.position[j]);low[j]=qMin(low[j],v.position[j]);high[j]=qMax(high[j],v.position[j]);if(normals)f32(normalData,v.normal[j]);}if(m.hasUvs)for(float value:v.uv)f32(uvData,value);}
                for(qsizetype i=0;i<m.triangles.size();++i){if(i%3072==0)checkpoint(progress);u32(indexData,m.triangles[i]);}
                QJsonObject attributes{{"POSITION",accessor(positions,m.vertices.size(),3,false,{low[0],low[1],low[2]},{high[0],high[1],high[2]})}};
                if(normals)attributes["NORMAL"]=accessor(normalData,m.vertices.size(),3,false);if(m.hasUvs)attributes["TEXCOORD_0"]=accessor(uvData,m.vertices.size(),2,false);
                QJsonObject primitive{{"attributes",attributes},{"indices",accessor(indexData,m.triangles.size(),1,true)},{"mode",4}};if(o.material)primitive["material"]=0;
                gltfMeshes.append(QJsonObject{{"name",name},{"primitives",QJsonArray{primitive}}});nodes.append(QJsonObject{{"name",name},{"mesh",meshIndex}});sceneNodes.append(meshIndex);
            }
            if(progress)progress->percent.store((meshIndex+1)*90/meshes.size());
        }
        if(o.format==ModelFormat::Glb){
            QJsonObject json{{"asset",QJsonObject{{"version","2.0"},{"generator","Binary Workbench"}}},{"scene",0},{"scenes",QJsonArray{QJsonObject{{"nodes",sceneNodes}}}},{"nodes",nodes},{"meshes",gltfMeshes},{"accessors",accessors},{"bufferViews",views},{"buffers",QJsonArray{QJsonObject{{"byteLength",double(bin.size())}}}}};
            if(o.material){auto linear=[](double c){return c<=.04045?c/12.92:std::pow((c+.055)/1.055,2.4);};json["materials"]=QJsonArray{QJsonObject{{"name","Color de prueba"},{"doubleSided",o.doubleSided},{"pbrMetallicRoughness",QJsonObject{{"baseColorFactor",QJsonArray{linear(o.color.redF()),linear(o.color.greenF()),linear(o.color.blueF()),1.0}},{"metallicFactor",0.0},{"roughnessFactor",1.0}}}}};}
            QByteArray text=QJsonDocument(json).toJson(QJsonDocument::Compact);while(text.size()%4)text+=' ';while(bin.size()%4)bin+='\0';QByteArray header;u32(header,0x46546c67);u32(header,2);u32(header,quint32(28+text.size()+bin.size()));u32(header,text.size());u32(header,0x4e4f534a);write(file,header);write(file,text);QByteArray binHeader;u32(binHeader,bin.size());u32(binHeader,0x004e4942);write(file,binHeader);for(qsizetype at=0;at<bin.size();at+=1048576){checkpoint(progress);write(file,bin.mid(at,1048576));}
        }
        checkpoint(progress);if(!file.commit())fail("No se pudo finalizar el archivo: "+file.errorString());result.ok=true;if(progress)progress->percent=100;
    }catch(const Cancelled &){result.cancelled=true;result.error="Exportación cancelada; no se ha sustituido el destino.";}catch(const std::exception &e){result.error=QString::fromUtf8(e.what());}
    return result;
}
