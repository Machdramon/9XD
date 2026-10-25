#pragma once
#include "image_recipe_io.h"
#include "mesh_data.h"
#include "memory_budget.h"
#include "mesh_setup_session.h"
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QJsonArray>
#include <QFile>
#include <cmath>

namespace AiDecoderSetup {
inline bool offset(const QJsonValue &value,qint64 &out){if(!value.isString())return false;auto t=value.toString().trimmed();bool ok=false;out=t.toLongLong(&ok,t.startsWith("0x",Qt::CaseInsensitive)?16:10);return ok&&out>=0;}
inline QJsonObject scalar(QString type,QString description={}){QJsonObject p{{"type",type}};if(!description.isEmpty())p["description"]=description;return p;}
inline QJsonObject choice(QStringList values){QJsonArray a;for(auto v:values)a.append(v);return {{"type","string"},{"enum",a}};}
inline QJsonObject imageProperties(){return {
    {"offset",scalar("string","Absolute byte offset, decimal or 0x hex")},{"width",scalar("integer")},{"height",scalar("integer")},
    {"format",choice(imageFormats())},{"container",choice(imageContainers())},{"layout",choice(imageLayouts())},{"paletteOffset",scalar("string")},
    {"paletteFormat",choice(imagePaletteFormats())},{"paletteEntries",scalar("integer")},{"paletteBank",scalar("integer")},
    {"rowStride",scalar("string","0 means packed rows")},{"lowNibbleFirst",scalar("boolean")},{"ps2Clut",scalar("boolean")},
    {"ps2Alpha",scalar("boolean")},{"bigEndian",scalar("boolean")},{"flipX",scalar("boolean")},{"flipY",scalar("boolean")},
    {"byteSwap",choice({"None","Swap 8 in 16","Swap 8 in 32","Swap 16 in 32"})},
    {"mipLevel",scalar("integer")},{"surface",scalar("integer")},{"count",scalar("integer")},{"step",scalar("string")}};}
inline QJsonObject meshProperties(){QJsonObject p;
    for(auto key:{"vertexOffset","indexOffset","uvOffset","normalOffset","deltaOffset"})p[key]=scalar("string","Absolute byte offset, decimal or 0x hex");
    for(auto key:{"vertexCount","vertexStride","indexCount","indexStride","uvStride","normalStride","deltaStride"})p[key]=scalar("integer");
    p["positionType"]=choice({"Float32","Float16","Int8","UInt8","Int16","UInt16","Int32","UInt32","SNorm8","SNorm16","UNorm8","UNorm16","Fixed16.16"});
    p["indexType"]=choice({"UInt16","UInt32"});p["uvType"]=choice({"Float16","Float32"});p["normalType"]=choice({"Float32","SNorm16"});p["deltaType"]=choice({"Float32","Float16","Int32","Int16"});
    p["topology"]=choice({"Triangle list","Triangle strip"});p["coordinates"]=choice({"XYZ","XYZW"});
    for(auto key:{"positionScale","deltaScale"})p[key]=scalar("number");
    for(auto key:{"uv","normals","bigEndian","deltaFromBinary","deltaBigEndian"})p[key]=scalar("boolean");
    return p;
}
inline bool checkKeys(const QJsonObject &args,const QJsonObject &properties,QString &error){
    for(auto i=args.begin();i!=args.end();++i){if(!properties.contains(i.key())){error="Unknown setting: "+i.key();return false;}
        auto definition=properties[i.key()].toObject();auto type=definition["type"].toString();bool valid=type=="string"?i->isString():type=="boolean"?i->isBool():i->isDouble()&&std::isfinite(i->toDouble())&&(type!="integer"||i->toDouble()==std::floor(i->toDouble()));
        if(!valid||definition.contains("enum")&&!definition["enum"].toArray().contains(i.value())){error="Invalid setting: "+i.key();return false;}}
    return true;
}
inline bool required(const QJsonObject &args,QStringList keys,QString &error){for(auto key:keys)if(!args.contains(key)){error="Missing setting: "+key;return false;}return true;}
inline bool image(const QJsonObject &args,qint64 size,ImageRecipe &r,int &count,qint64 &step,QString &error){
    if(!checkKeys(args,imageProperties(),error)||!required(args,{"offset","width","height","format"},error))return false;
    auto json=encodeImageRecipe(ImageRecipe{});for(auto i=args.begin();i!=args.end();++i)json[i.key()]=i.value();
    for(auto key:{"offset","paletteOffset","rowStride","step"}){qint64 n;if(!offset(json[key],n)){error="Invalid byte offset/stride: "+QString(key);return false;}json[key]=QString::number(n);}
    if(!decodeImageRecipe(json,r,count,step,error))return false;
    if(size<=0){error="Open a nonempty document first.";return false;}
    if(r.format=="Encoded image"){if(count!=1||r.rowStride||r.offset>=size||size-r.offset>32*1024*1024){error="Encoded images require an in-range offset, count 1, row stride 0 and at most 32 MiB of remaining input.";return false;}return true;}
    auto span=imageSpan(r,error);auto spacing=step?step:span;
    if(!error.isEmpty()||r.offset>size||span>size-r.offset||(count-1)>(size-r.offset-span)/qMax(qint64(1),spacing)){error="Image range extends outside the active document.";return false;}
    if(qint64(r.width)*r.height*4*count>128*1024*1024){error="Image batch exceeds 128 MiB.";return false;}
    if(imageIndexed(r.format)){auto paletteBytes=qint64(imagePaletteCount(r))*imagePixelBits(r.paletteFormat)/8;auto bank=paletteBytes*r.paletteBank;
        if(r.paletteOffset>size||bank>size-r.paletteOffset||paletteBytes>size-r.paletteOffset-bank){error="Palette range extends outside the active document.";return false;}}
    return true;
}
inline int width(QString type){return type=="Fixed16.16"?4:type.endsWith("8")?1:type.endsWith("16")?2:4;}
inline bool mesh(const QJsonObject &args,qint64 size,MeshSpec &s,QString &error){
    if(!checkKeys(args,meshProperties(),error)||!required(args,{"vertexOffset","vertexCount","vertexStride","positionType","positionScale","indexOffset","indexCount"},error))return false;
    s=MeshSpec{};s.uv=false;s.normals=false;s.strip=false;s.vertexCount=0;s.indexCount=0;s.positionScale=1;
    auto off=[&](const char *key,qint64 &out){if(args.contains(key)&&!offset(args[key],out)){error="Invalid byte offset: "+QString(key);return false;}return true;};
    if(!off("vertexOffset",s.vertexOffset)||!off("indexOffset",s.indexOffset)||!off("uvOffset",s.uvOffset)||!off("normalOffset",s.normalOffset)||!off("deltaOffset",s.deltaOffset))return false;
    auto number=[&](const char *key,qint64 &out){if(!args.contains(key))return true;auto n=args[key].toDouble();if(!std::isfinite(n)||n!=std::floor(n)||n<0||n>AssetMemory::indices()){error="Count/stride out of range: "+QString(key);return false;}out=qint64(n);return true;};
    if(!number("vertexCount",s.vertexCount)||!number("vertexStride",s.vertexStride)||!number("indexCount",s.indexCount)||!number("indexStride",s.indexStride)||!number("uvStride",s.uvStride)||!number("normalStride",s.normalStride)||!number("deltaStride",s.deltaStride))return false;
    auto str=[&](const char *key,QString &out){if(args.contains(key))out=args[key].toString();};str("positionType",s.positionType);str("indexType",s.indexType);str("uvType",s.uvType);str("normalType",s.normalType);str("deltaType",s.deltaType);
    auto boolean=[&](const char *key,bool &out){if(args.contains(key))out=args[key].toBool();};boolean("uv",s.uv);boolean("normals",s.normals);boolean("bigEndian",s.bigEndian);boolean("deltaFromBinary",s.deltaFromBinary);boolean("deltaBigEndian",s.deltaBigEndian);
    s.strip=args.value("topology").toString()=="Triangle strip";s.homogeneous=args.value("coordinates").toString()=="XYZW";
    s.positionScale=args["positionScale"].toDouble();s.deltaScale=args.value("deltaScale").toDouble(1);
    if(s.vertexCount<1||s.vertexCount>AssetMemory::vertices()||s.indexCount<3||s.indexCount>AssetMemory::indices()||(!s.strip&&s.indexCount%3)||s.positionScale<1e-8||s.positionScale>1e6||s.deltaScale<1e-8||s.deltaScale>1e6||(s.homogeneous&&s.positionType!="Float16"&&s.positionType!="Float32")){error="Invalid mesh counts, scale or coordinates.";return false;}
    if(s.uv&&!required(args,{"uvOffset","uvStride","uvType"},error))return false;
    if(s.normals&&!required(args,{"normalOffset","normalStride","normalType"},error))return false;
    if(s.deltaFromBinary&&!required(args,{"deltaOffset"},error))return false;
    qint64 budget=0;auto block=[&](qint64 offset,qint64 count,qint64 stride,int item){if(offset>size||stride<item||stride>65536||item>size-offset||(count-1)>(size-offset-item)/stride)return false;budget+=(count-1)*stride+item;return AssetMemory::fits(s.vertexCount,s.indexCount,budget);};
    if(size<=0||!block(s.vertexOffset,s.vertexCount,s.vertexStride,(s.homogeneous?4:3)*width(s.positionType))||!block(s.indexOffset,s.indexCount,s.indexStride,width(s.indexType))||
       s.uv&&!block(s.uvOffset,s.vertexCount,s.uvStride,2*width(s.uvType))||s.normals&&!block(s.normalOffset,s.vertexCount,s.normalStride,3*width(s.normalType))||s.deltaFromBinary&&!block(s.deltaOffset,3,s.deltaStride,width(s.deltaType))){error="Mesh range/stride is invalid or exceeds the active document / configured memory budget.";return false;}
    return true;
}
// Changes only known decoding controls. No file access, button clicks or rendering.
inline QJsonObject applyMesh(QWidget *root,const QJsonObject &args,qint64 size){
    MeshSpec s;QString error;if(!mesh(args,size,s,error))return {{"error",error}};
    auto *render=root->findChild<QPushButton *>("renderMesh");if(!render||!render->isEnabled())return {{"error","Wait for mesh rendering to finish."}};
    QMap<QString,QString> text{{"meshVertexOffset",QString::number(s.vertexOffset)},{"meshVertexCount",QString::number(s.vertexCount)},{"meshVertexStride",QString::number(s.vertexStride)},
       {"meshIndexOffset",QString::number(s.indexOffset)},{"meshIndexCount",QString::number(s.indexCount)},{"meshIndexStride",QString::number(s.indexStride)},
       {"meshUvOffset",QString::number(s.uvOffset)},{"meshUvStride",QString::number(s.uvStride)},{"meshNormalOffset",QString::number(s.normalOffset)},{"meshNormalStride",QString::number(s.normalStride)},
       {"meshDeltaOffset",QString::number(s.deltaOffset)},{"meshDeltaStride",QString::number(s.deltaStride)}};
    QMap<QString,QString> combos{{"meshPositionType",s.positionType},{"meshIndexType",s.indexType},{"meshUvType",s.uvType},{"meshNormalType",s.normalType},
       {"meshDeltaType",s.deltaType},{"meshEndian",s.bigEndian?"Big Endian":"Little Endian"},{"meshDeltaEndian",s.deltaBigEndian?"Big Endian":"Little Endian"},
       {"meshTopology",s.strip?"Triangle strip · FF restart":"Triangle list"},{"meshCoordinateLayout",s.homogeneous?"XYZW (divide by W)":"XYZ"},
       {"meshDeltaSource",s.deltaFromBinary?"Read from binary":"Manual"},{"meshDestination","Add to collection"},{"meshAxisOrder","XYZ"}};
    QMap<QString,double> doubles{{"meshPositionScale",s.positionScale},{"meshDeltaScale",s.deltaScale}};
    QMap<QString,bool> checks{{"meshReadUvs",s.uv},{"meshReadNormals",s.normals}};
    for(auto axis:{"X","Y","Z"}){doubles[QString("meshTransformScale")+axis]=1;doubles[QString("meshRotation")+axis]=0;doubles[QString("meshPositionDelta")+axis]=0;checks[QString("meshFlip")+axis]=false;}
    for(auto i=text.begin();i!=text.end();++i)if(!root->findChild<QLineEdit *>(i.key()))return {{"error","Mesh controls unavailable."}};
    for(auto i=combos.begin();i!=combos.end();++i){auto *w=root->findChild<QComboBox *>(i.key());if(!w||w->findText(i.value())<0)return {{"error","Mesh control/type unavailable."}};}
    for(auto i=doubles.begin();i!=doubles.end();++i)if(!root->findChild<QDoubleSpinBox *>(i.key()))return {{"error","Mesh controls unavailable."}};
    for(auto i=checks.begin();i!=checks.end();++i)if(!root->findChild<QCheckBox *>(i.key()))return {{"error","Mesh controls unavailable."}};
    for(auto i=text.begin();i!=text.end();++i)root->findChild<QLineEdit *>(i.key())->setText(i.value());
    for(auto i=combos.begin();i!=combos.end();++i)root->findChild<QComboBox *>(i.key())->setCurrentText(i.value());
    for(auto i=doubles.begin();i!=doubles.end();++i)root->findChild<QDoubleSpinBox *>(i.key())->setValue(i.value());
    for(auto i=checks.begin();i!=checks.end();++i)root->findChild<QCheckBox *>(i.key())->setChecked(i.value());
    if(auto *status=root->findChild<QLabel *>("meshStatus"))status->setText("Assistant filled mesh settings. Review, then click Render Current File.");
    return {{"status","Mesh settings filled; nothing rendered. Review and click Render Current File."},{"settings",args},{"readUVs",s.uv},{"readNormals",s.normals},{"vertexCount",s.vertexCount},{"transforms","Identity; manual delta zero unless binary delta enabled."}};
}
inline QJsonObject loadMeshFile(QWidget *root,const QString &path,qint64 size){
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>65536)return {{"error","Cannot open mesh setup (maximum 64 KiB)."}};
    QJsonParseError error;auto json=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!json.isObject())return {{"error","Invalid mesh setup JSON."}};
    if(json.object().contains("format")){auto *session=meshSetupSession(root);if(!session)return {{"error","Mesh session unavailable."}};QString error;if(!session->loadDraft(json.object(),error))return {{"error",error}};session->markLoaded(path);return {{"status","Mesh setup restored; incomplete fields may be finished before rendering."}};}
    auto result=applyMesh(root,json.object(),size);if(!result.contains("error"))if(auto *session=meshSetupSession(root))session->markLoaded(path);
    if(!result.contains("error"))if(auto *status=root->findChild<QLabel *>("meshStatus"))status->setText(QString("Mesh setup loaded: %1 vertices · UVs %2 · normals %3. Click Render Current File.").arg(result["vertexCount"].toInteger()).arg(result["readUVs"].toBool()?"on":"off",result["readNormals"].toBool()?"read":"calculated"));
    return result;
}

}
