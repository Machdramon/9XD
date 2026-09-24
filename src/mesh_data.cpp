#include "mesh_data.h"
#include <QtEndian>
#include <bit>
#include <cmath>
#include <algorithm>
namespace {
int width(const QString &type){return type.endsWith("16")?2:4;}
quint32 integer(const char *p,int bytes,bool big){quint32 value=0;for(int i=0;i<bytes;++i)value|=quint32(quint8(p[i]))<<(8*(big?bytes-1-i:i));return value;}
float component(const char *p,const QString &type,bool big){auto u=integer(p,width(type),big);if(type=="Float32")return std::bit_cast<float>(u);if(type=="Float16"){int sign=(u&0x8000)?-1:1,e=(u>>10)&31,m=u&1023;if(e==31)return m?NAN:sign*INFINITY;return sign*std::ldexp(float(e?1024+m:m),e?e-25:-24);}if(type=="SNorm16")return std::max(-1.f,float(qint16(u))/32767.f);if(type=="Int16")return qint16(u);return float(std::bit_cast<qint32>(u));}
}
MeshResult readMesh(const ReadRequest &source,const PatchList &patches,const MeshSpec &s){
    MeshResult result;auto fail=[&](QString error){result.error=error;result.mesh.reset();return result;};
    if(s.vertexCount<1||s.vertexCount>100000||s.indexCount<3||s.indexCount>600000||!std::isfinite(s.positionScale)||s.positionScale<=0)return fail("Límites: 100 000 vértices, 600 000 índices y escala positiva finita.");
    if(!QStringList{"Int16","Int32","Float16","Float32"}.contains(s.positionType)||!QStringList{"UInt16","UInt32"}.contains(s.indexType)||!QStringList{"Float16","Float32"}.contains(s.uvType)||!QStringList{"Float32","SNorm16"}.contains(s.normalType))return fail("Tipo de componente no admitido.");
    QString error;qint64 budget=0;
    auto block=[&](qint64 offset,qint64 count,qint64 stride,int item){QByteArray bytes;if(offset<0||offset>source.expectedSize||stride<item||stride>65536||count<1||source.expectedSize-offset<item||(count-1)>(source.expectedSize-offset-item)/stride){error="Una sección queda fuera del archivo o tiene stride insuficiente.";return bytes;}qint64 length=(count-1)*stride+item;budget+=length;if(budget>64*1024*1024){error="Las secciones abarcan más de 64 MiB; reduce la región.";return bytes;}for(qint64 done=0;done<length;){auto r=source;r.offset=offset+done;r.length=qMin(qint64(1048576),length-done);auto data=readPage(r);if(!data.error.isEmpty()){error=data.error;return QByteArray();}applyPatches(data.bytes,r.offset,patches);bytes+=data.bytes;done+=data.bytes.size();}return bytes;};
    auto positions=block(s.vertexOffset,s.vertexCount,s.vertexStride,3*width(s.positionType));if(!error.isEmpty())return fail(error);
    auto indices=block(s.indexOffset,s.indexCount,s.indexStride,width(s.indexType));if(!error.isEmpty())return fail(error);
    QByteArray uv,normals;if(s.uv)uv=block(s.uvOffset,s.vertexCount,s.uvStride,2*width(s.uvType));if(!error.isEmpty())return fail(error);if(s.normals)normals=block(s.normalOffset,s.vertexCount,s.normalStride,3*width(s.normalType));if(!error.isEmpty())return fail(error);
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=s.uv;mesh->vertices.resize(s.vertexCount);
    for(qint64 i=0;i<s.vertexCount;++i){mesh->vertexOffsets.append(s.vertexOffset+i*s.vertexStride);auto &v=mesh->vertices[i];for(int j=0;j<3;++j){v.position[j]=float(component(positions.constData()+i*s.vertexStride+j*width(s.positionType),s.positionType,s.bigEndian)*s.positionScale);v.normal[j]=s.normals?component(normals.constData()+i*s.normalStride+j*width(s.normalType),s.normalType,s.bigEndian):0;if(!std::isfinite(v.position[j])||!std::isfinite(v.normal[j]))return fail("Posición o normal no finita.");}for(int j=0;j<2;++j){v.uv[j]=s.uv?component(uv.constData()+i*s.uvStride+j*width(s.uvType),s.uvType,s.bigEndian):0;if(!std::isfinite(v.uv[j]))return fail("UV no finita.");}}
    QVector<quint32> run;QVector<qint64> runSlots;int stripId=0;const quint32 restart=s.indexType=="UInt16"?0xffff:0xffffffff;
    for(qint64 i=0;i<s.indexCount;++i){
        quint32 value=integer(indices.constData()+i*s.indexStride,width(s.indexType),s.bigEndian);
        if(s.strip&&value==restart){run.clear();runSlots.clear();++stripId;continue;}
        if(value>=s.vertexCount)return fail("Índice de cara fuera del rango de vértices.");run.append(value);runSlots.append(i);
        if(s.strip&&run.size()>=3){auto n=run.size();quint32 a=run[n-3],b=run[n-2],c=run[n-1];std::array<qint64,3> indexSlots{runSlots[n-3],runSlots[n-2],runSlots[n-1]};if((n-3)%2){std::swap(a,b);std::swap(indexSlots[0],indexSlots[1]);}
            if(a!=b&&b!=c&&a!=c){mesh->triangles<<a<<b<<c;mesh->faceSources.append({{s.indexOffset+indexSlots[0]*s.indexStride,s.indexOffset+indexSlots[1]*s.indexStride,s.indexOffset+indexSlots[2]*s.indexStride},indexSlots,stripId});}
        }else if(!s.strip&&run.size()==3){mesh->triangles+=run;std::array<qint64,3> indexSlots{runSlots[0],runSlots[1],runSlots[2]};mesh->faceSources.append({{s.indexOffset+indexSlots[0]*s.indexStride,s.indexOffset+indexSlots[1]*s.indexStride,s.indexOffset+indexSlots[2]*s.indexStride},indexSlots,-1});run.clear();runSlots.clear();}
    }
    if(!s.strip&&!run.isEmpty())return fail("La lista de triángulos tiene índices incompletos.");if(mesh->triangles.isEmpty())return fail("No hay triángulos válidos.");
    if(!s.normals){for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];float u[3],v[3];for(int j=0;j<3;++j){u[j]=b.position[j]-a.position[j];v[j]=c.position[j]-a.position[j];}float n[]={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(auto index:{mesh->triangles[i],mesh->triangles[i+1],mesh->triangles[i+2]})for(int j=0;j<3;++j)mesh->vertices[index].normal[j]+=n[j];}}
    for(auto &v:mesh->vertices){double n=std::hypot(double(v.normal[0]),double(v.normal[1]),double(v.normal[2]));if(!std::isfinite(n))return fail("Normal fuera de rango.");if(n>0)for(auto &c:v.normal)c=float(c/n);}
    auto end=source;end.offset=0;end.length=0;auto check=readPage(end);if(!check.error.isEmpty())return fail(check.error);result.mesh=mesh;return result;
}
