#include "script_context.h"
#include <QJsonArray>
#include <cmath>
QJsonObject scriptMeshJson(const std::shared_ptr<const MeshData> &mesh){QJsonArray positions,normals,uv,indices;if(mesh){for(const auto &v:mesh->vertices){positions.append(QJsonArray{v.position[0],v.position[1],v.position[2]});normals.append(QJsonArray{v.normal[0],v.normal[1],v.normal[2]});uv.append(QJsonArray{v.uv[0],v.uv[1]});}for(auto i:mesh->triangles)indices.append(double(i));}return {{"vertices",positions},{"normals",normals},{"uvs",mesh&&!mesh->hasUvs?QJsonValue():QJsonValue(uv)},{"indices",indices}};}
std::shared_ptr<MeshData> scriptMeshFromJson(const QJsonObject &json,QString &error){
    auto fail=[&](QString text)->std::shared_ptr<MeshData>{error=text;return {};};
    if(!json["vertices"].isArray()||!json["indices"].isArray())return fail("La malla necesita vertices e indices.");auto positions=json["vertices"].toArray(),indices=json["indices"].toArray();
    if(positions.size()>100000||indices.size()>600000||indices.size()%3)return fail("Límite: 100 000 vértices y 600 000 índices en triángulos completos.");
    if(positions.isEmpty()!=indices.isEmpty())return fail("Malla incompleta.");
    auto mesh=std::make_shared<MeshData>();mesh->hasUvs=json.contains("uvs")&&!json["uvs"].isNull();mesh->vertices.resize(positions.size());
    auto values=[&](const QJsonValue &row,float *out,int n){if(!row.isArray()||row.toArray().size()!=n)return false;auto a=row.toArray();for(int j=0;j<n;++j){if(!a[j].isDouble()||!std::isfinite(a[j].toDouble())||!std::isfinite(float(a[j].toDouble())))return false;out[j]=float(a[j].toDouble());}return true;};
    for(int i=0;i<positions.size();++i)if(!values(positions[i],mesh->vertices[i].position,3))return fail("Posiciones XYZ inválidas.");
    for(auto v:indices){double i=v.toDouble(-1);if(!v.isDouble()||!std::isfinite(i)||i<0||i>=positions.size()||std::floor(i)!=i)return fail("Índice fuera del rango de vértices.");mesh->triangles.append(quint32(i));}
    bool hasNormals=json.contains("normals")&&!json["normals"].isNull();if(hasNormals){auto n=json["normals"].toArray();if(!json["normals"].isArray()||n.size()!=positions.size())return fail("Cantidad de normales incorrecta.");for(int i=0;i<n.size();++i)if(!values(n[i],mesh->vertices[i].normal,3))return fail("Normal inválida.");}
    else for(int i=0;i<mesh->triangles.size();i+=3){auto &a=mesh->vertices[mesh->triangles[i]],&b=mesh->vertices[mesh->triangles[i+1]],&c=mesh->vertices[mesh->triangles[i+2]];double u[3],v[3];for(int j=0;j<3;++j){u[j]=double(b.position[j])-a.position[j];v[j]=double(c.position[j])-a.position[j];}double n[]={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};double length=std::hypot(n[0],n[1],n[2]);if(length>0)for(auto k:{mesh->triangles[i],mesh->triangles[i+1],mesh->triangles[i+2]})for(int j=0;j<3;++j)mesh->vertices[k].normal[j]+=float(n[j]/length);}
    for(auto &v:mesh->vertices){double n=std::hypot(double(v.normal[0]),double(v.normal[1]),double(v.normal[2]));if(n>0)for(auto &f:v.normal)f=float(f/n);}
    if(json.contains("uvs")&&!json["uvs"].isNull()){auto uv=json["uvs"].toArray();if(!json["uvs"].isArray()||uv.size()!=positions.size())return fail("Cantidad de UV incorrecta.");for(int i=0;i<uv.size();++i)if(!values(uv[i],mesh->vertices[i].uv,2))return fail("UV inválida.");}return mesh;
}
