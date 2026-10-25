#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QStringList>
#include <atomic>

namespace SegaA3da {
// Text animation/camera metadata is evidence of external object dependencies, not a mesh.
inline QJsonObject inspect(const QByteArray &bytes,const std::atomic_bool &cancel){
    if(!bytes.startsWith("#A3DA__________"))return {};
    QJsonArray references;QSet<QString> seen;QString filename,fps,duration,objects,cameras;int cameraFields=0;bool truncated=bytes.size()>2*1024*1024;
    auto text=bytes.left(2*1024*1024);
    for(auto raw:text.split('\n')){
        if(cancel.load())return {};auto line=raw.trimmed();auto equal=line.indexOf('=');if(equal<1)continue;auto key=line.left(equal),value=line.mid(equal+1);if(value.size()>1024)continue;
        auto v=QString::fromUtf8(value);if(key=="_.file_name")filename=v;else if(key=="play_control.fps")fps=v;else if(key=="play_control.size")duration=v;else if(key=="object.length")objects=v;else if(key=="camera_root.length")cameras=v;
        if(key.startsWith("camera_root."))++cameraFields;
        if((key.startsWith("object.")&&(key.endsWith(".name")||key.endsWith(".uid_name")))||(key.startsWith("object_list.")&&key!="object_list.length")){
            if(!seen.contains(v)&&!v.isEmpty()){seen.insert(v);if(references.size()<128)references.append(v);else truncated=true;}
        }
    }
    auto diagnostic=QString("SEGA A3DA animation/camera metadata%1. No mesh decoder applies to this text resource. Referenced geometry must be loaded from a separate object/geometry resource.").arg(filename.isEmpty()?QString():" — "+filename);
    if(!objects.isEmpty())diagnostic+=" Objects: "+objects+".";if(!cameras.isEmpty()||cameraFields)diagnostic+=" Camera data present.";if(!fps.isEmpty())diagnostic+=" FPS: "+fps+".";
    QStringList names;for(int i=0;i<qMin(6,int(references.size()));++i)names.append(references[i].toString());if(!names.isEmpty())diagnostic+=" Referenced objects: "+names.join(", ")+(references.size()>6?", …":"")+".";
    return {{"family","SEGA A3DA animation / cameras"},{"diagnostic",diagnostic},{"metadataOnly",true},{"previewSupported",false},{"renderSupported",false},{"filename",filename},{"fps",fps},{"durationFrames",duration},{"declaredObjects",objects},{"objectReferences",references},{"referencesTruncated",truncated},{"requiredDecoder","External geometry/object resource required; A3DA animation playback is not implemented."},{"regions",QJsonArray{QJsonObject{{"name","SEGA A3DA text header"},{"offset","0"},{"length","15"},{"step","15"},{"repetitions",1},{"evidence","Exact A3DA text signature; animation/camera key-value metadata"}}}}};
}
}
