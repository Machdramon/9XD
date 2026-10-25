#include "mesh_data.h"
#include "memory_budget.h"
#include <QtEndian>
#include <bit>
#include <cmath>
#include <algorithm>
namespace {
int width(const QString &type){return type=="Fixed16.16"?4:type.endsWith("8")?1:type.endsWith("16")?2:4;}
quint32 integer(const char *p,int bytes,bool big){quint32 value=0;for(int i=0;i<bytes;++i)value|=quint32(quint8(p[i]))<<(8*(big?bytes-1-i:i));return value;}
double component(const char *p,const QString &type,bool big){auto u=integer(p,width(type),big);if(type=="Float32")return std::bit_cast<float>(u);if(type=="Float16"){int sign=(u&0x8000)?-1:1,e=(u>>10)&31,m=u&1023;if(e==31)return m?NAN:sign*INFINITY;return sign*std::ldexp(double(e?1024+m:m),e?e-25:-24);}if(type=="SNorm8")return std::max(-1.,double(std::bit_cast<qint8>(quint8(u)))/127.);if(type=="SNorm16")return std::max(-1.,double(std::bit_cast<qint16>(quint16(u)))/32767.);if(type=="UNorm8")return double(u)/255.;if(type=="UNorm16")return double(u)/65535.;if(type=="Int8")return std::bit_cast<qint8>(quint8(u));if(type=="Int16")return std::bit_cast<qint16>(quint16(u));if(type=="UInt8"||type=="UInt16"||type=="UInt32")return u;if(type=="Fixed16.16")return double(std::bit_cast<qint32>(u))/65536.;return double(std::bit_cast<qint32>(u));}
const QStringList coordinateTypes{"Int8","UInt8","Int16","UInt16","Int32","UInt32","Float16","Float32","SNorm8","SNorm16","UNorm8","UNorm16","Fixed16.16"};
std::array<double,3> transformVector(std::array<double,3> input,const MeshSpec &s,bool normal){
    std::array<double,3> result;
    for(int axis=0;axis<3;++axis)result[axis]=input[s.axes[axis]]*(s.negate[axis]?-1:1)*(normal?1/s.axisScale[axis]:s.axisScale[axis]);
    // Fixed Euler order: local X, then Y, then Z.
    for(int axis=0;axis<3;++axis){int a=(axis+1)%3,b=(axis+2)%3;double angle=s.rotationDegrees[axis]*3.14159265358979323846/180.,c=std::cos(angle),t=std::sin(angle),x=result[a],y=result[b];result[a]=c*x-t*y;result[b]=t*x+c*y;}
    return result;
}
}
MeshResult readMesh(const ReadRequest &source,const PatchList &patches,const MeshSpec &s){
    MeshResult result;auto fail=[&](QString error){result.error=error;result.mesh.reset();return result;};
    if(s.vertexCount<1||s.vertexCount>AssetMemory::vertices()||s.indexCount<3||s.indexCount>AssetMemory::indices()||!std::isfinite(s.positionScale)||s.positionScale<=0)return fail("Mesh counts exceed the configured memory budget or scale is invalid.");
    if(!coordinateTypes.contains(s.positionType)||!QStringList{"UInt16","UInt32"}.contains(s.indexType)||!QStringList{"Float16","Float32"}.contains(s.uvType)||!QStringList{"Float32","SNorm16"}.contains(s.normalType))return fail("Unsupported component type.");
    auto order=s.axes;std::sort(order.begin(),order.end());if(order!=std::array<int,3>{0,1,2})return fail("Coordinate axes must be a permutation of XYZ.");
    int inversions=0;for(int i=0;i<3;++i){if(!std::isfinite(s.axisScale[i])||std::abs(s.axisScale[i])<1e-12||!std::isfinite(s.rotationDegrees[i]))return fail("Transform scale must be finite and nonzero; rotation must be finite.");inversions+=s.negate[i];inversions+=s.axisScale[i]<0;for(int j=i+1;j<3;++j)inversions+=s.axes[i]>s.axes[j];}
    if(s.homogeneous&&s.positionType!="Float16"&&s.positionType!="Float32")return fail("Homogeneous XYZW requires Float16 or Float32 coordinates.");
    qint64 previousStart=-1;for(auto start:s.stripStarts){if(!s.strip||start<0||start>=s.indexCount||start<=previousStart)return fail("Invalid independent strip boundaries.");previousStart=start;}
    QString error;qint64 budget=0;
    auto block=[&](qint64 offset,qint64 count,qint64 stride,int item){QByteArray bytes;if(offset<0||offset>source.expectedSize||stride<item||stride>65536||count<1||source.expectedSize-offset<item||(count-1)>(source.expectedSize-offset-item)/stride){error="A section is outside the file or its stride is too small.";return bytes;}qint64 length=(count-1)*stride+item;budget+=length;if(!AssetMemory::fits(s.vertexCount,s.indexCount,budget)){error="Mesh input and geometry exceed the configured memory budget in Settings > Performance.";return bytes;}for(qint64 done=0;done<length;){auto r=source;r.offset=offset+done;r.length=qMin(qint64(1048576),length-done);auto data=readPage(r);if(!data.error.isEmpty()){error=data.error;return QByteArray();}applyPatches(data.bytes,r.offset,patches);bytes+=data.bytes;done+=data.bytes.size();}return bytes;};
    result.positionDelta=s.positionDelta;
    if(s.deltaFromBinary){
        if(!coordinateTypes.contains(s.deltaType)||!std::isfinite(s.deltaScale)||s.deltaScale<=0)return fail("Invalid position delta type or scale.");
        auto bytes=block(s.deltaOffset,3,s.deltaStride,width(s.deltaType));if(!error.isEmpty())return fail("Position Delta: "+error);
        for(int axis=0;axis<3;++axis)result.positionDelta[axis]=component(bytes.constData()+axis*s.deltaStride,s.deltaType,s.deltaBigEndian)*s.deltaScale;
    }
    for(double value:result.positionDelta)if(!std::isfinite(value))return fail("Position Delta contains a nonfinite value.");
    auto positions=block(s.vertexOffset,s.vertexCount,s.vertexStride,(s.homogeneous?4:3)*width(s.positionType));if(!error.isEmpty())return fail(error);
    auto indices=block(s.indexOffset,s.indexCount,s.indexStride,width(s.indexType));if(!error.isEmpty())return fail(error);
    QByteArray uv,normals;if(s.uv)uv=block(s.uvOffset,s.vertexCount,s.uvStride,2*width(s.uvType));if(!error.isEmpty())return fail(error);if(s.normals)normals=block(s.normalOffset,s.vertexCount,s.normalStride,3*width(s.normalType));if(!error.isEmpty())return fail(error);
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=s.uv;mesh->vertices.resize(s.vertexCount);
    for(qint64 i=0;i<s.vertexCount;++i){mesh->vertexOffsets.append(s.vertexOffset+i*s.vertexStride);auto &v=mesh->vertices[i];std::array<double,3> position,normal;double w=s.homogeneous?component(positions.constData()+i*s.vertexStride+3*width(s.positionType),s.positionType,s.bigEndian):1;
        if(!std::isfinite(w)||w==0)return fail("Invalid homogeneous W coordinate (zero or nonfinite).");
        for(int axis=0;axis<3;++axis){position[axis]=component(positions.constData()+i*s.vertexStride+axis*width(s.positionType),s.positionType,s.bigEndian)/w*s.positionScale;normal[axis]=s.normals?component(normals.constData()+i*s.normalStride+axis*width(s.normalType),s.normalType,s.bigEndian):0;}
        position=transformVector(position,s,false);normal=transformVector(normal,s,true);
        for(int axis=0;axis<3;++axis){v.position[axis]=float(position[axis]);v.normal[axis]=float(normal[axis]);if(!std::isfinite(v.position[axis])||!std::isfinite(v.normal[axis]))return fail("Nonfinite position or normal after transformation.");}
        for(int axis=0;axis<2;++axis){v.uv[axis]=s.uv?component(uv.constData()+i*s.uvStride+axis*width(s.uvType),s.uvType,s.bigEndian):0;if(!std::isfinite(v.uv[axis]))return fail(QString("Nonfinite UV coordinate at vertex %1 (offset 0x%2). Check UV type/stride, or disable Read UVs for geometry-only decoding.").arg(i).arg(s.uvOffset+i*s.uvStride,0,16));}}
    QVector<quint32> run;QVector<qint64> runSlots;int stripId=0;const quint32 restart=s.indexType=="UInt16"?0xffff:0xffffffff;
    qsizetype nextStart=0;
    for(qint64 i=0;i<s.indexCount;++i){
        if(nextStart<s.stripStarts.size()&&i==s.stripStarts[nextStart]){run.clear();runSlots.clear();if(i)++stripId;++nextStart;}
        quint32 value=integer(indices.constData()+i*s.indexStride,width(s.indexType),s.bigEndian);
        if(s.strip&&value==restart){run.clear();runSlots.clear();++stripId;continue;}
        if(value>=s.vertexCount)return fail("Face index outside the vertex range.");run.append(value);runSlots.append(i);
        if(s.strip&&run.size()>=3){auto n=run.size();quint32 a=run[n-3],b=run[n-2],c=run[n-1];std::array<qint64,3> indexSlots{runSlots[n-3],runSlots[n-2],runSlots[n-1]};if((n-3)%2){std::swap(a,b);std::swap(indexSlots[0],indexSlots[1]);}
            if(a!=b&&b!=c&&a!=c){mesh->triangles<<a<<b<<c;mesh->faceSources.append({{s.indexOffset+indexSlots[0]*s.indexStride,s.indexOffset+indexSlots[1]*s.indexStride,s.indexOffset+indexSlots[2]*s.indexStride},indexSlots,stripId});}
        }else if(!s.strip&&run.size()==3){mesh->triangles+=run;std::array<qint64,3> indexSlots{runSlots[0],runSlots[1],runSlots[2]};mesh->faceSources.append({{s.indexOffset+indexSlots[0]*s.indexStride,s.indexOffset+indexSlots[1]*s.indexStride,s.indexOffset+indexSlots[2]*s.indexStride},indexSlots,-1});run.clear();runSlots.clear();}
    }
    if(!s.strip&&!run.isEmpty())return fail("The triangle list has incomplete indices.");if(mesh->triangles.isEmpty())return fail("No valid triangles.");
    if(inversions%2)for(int i=0;i<mesh->triangles.size();i+=3){std::swap(mesh->triangles[i+1],mesh->triangles[i+2]);auto &source=mesh->faceSources[i/3];std::swap(source.offsets[1],source.offsets[2]);std::swap(source.indexSlots[1],source.indexSlots[2]);}
    if(!s.normals){for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];float u[3],v[3];for(int j=0;j<3;++j){u[j]=b.position[j]-a.position[j];v[j]=c.position[j]-a.position[j];}float n[]={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(auto index:{mesh->triangles[i],mesh->triangles[i+1],mesh->triangles[i+2]})for(int j=0;j<3;++j)mesh->vertices[index].normal[j]+=n[j];}}
    for(auto &v:mesh->vertices){double n=std::hypot(double(v.normal[0]),double(v.normal[1]),double(v.normal[2]));if(!std::isfinite(n))return fail("Normal out of range.");if(n>0)for(auto &c:v.normal)c=float(c/n);}
    for(const auto &vertex:mesh->vertices)for(int axis=0;axis<3;++axis)if(!std::isfinite(float(double(vertex.position[axis])+result.positionDelta[axis])))return fail("Position Delta places a vertex outside the supported range.");
    auto end=source;end.offset=0;end.length=0;auto check=readPage(end);if(!check.error.isEmpty())return fail(check.error);result.mesh=mesh;return result;
}
