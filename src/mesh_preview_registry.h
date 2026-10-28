#pragma once
#include "mesh_scene.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
// Per-document-revision preview provenance. Removing/replacing a mesh permits reloading it.
class MeshPreviewRegistry {
public:
    struct Plan {QJsonObject candidate;QByteArray key;QSet<QString> instances;int existing=-1;bool skip=false;bool refreshMaterial=false;};
    void clear(){records_.clear();byKey_.clear();}
    Plan prepare(const QJsonObject &candidate,const QByteArray &nativeKind,const MeshScene &scene){
        Plan plan;plan.candidate=candidate;
        bool linked=candidate["profileId"]=="native-linked-surface"&&candidate["linkedFamily"]=="nfsu2";if(candidate["profileId"]!="native-nfsu2-scene"&&!linked){
            plan.key=candidate.isEmpty()?nativeKind:QJsonDocument(QJsonObject{{"idssTexture",candidate["texture"]},{"idssLod",candidate["idssLod"]},{"rwMaterialKey",candidate["rwMaterialKey"]},{"rwTextureOffset",candidate["rwTextureOffset"]},{"linkedKey",candidate["linkedKey"]},{"settings",candidate["settings"]},{"sourceTransform",candidate["sourceTransform"]},{"profileRotation",candidate["profileRotation"]},{"profileId",candidate["profileId"]},{"objectOffset",candidate["objectOffset"]},{"rtunedGeometryIds",candidate["rtunedGeometryIds"]},{"fm1Resource",candidate["fm1Resource"]},{"fm1ArchiveResource",candidate["fm1ArchiveResource"]},{"fm1TrackPart",candidate["fm1TrackPart"]},{"msrMaterial",candidate["msrMaterial"]},{"pgr1Resources",candidate["pgr1Resources"]},{"fm1FlipU",candidate["fm1FlipU"]},{"fm1FlipV",candidate["fm1FlipV"]}}).toJson(QJsonDocument::Compact);
            auto found=byKey_.find(plan.key);if(found!=byKey_.end()){
                for(auto id=found->begin();id!=found->end();){auto record=records_.find(*id);const auto *entry=scene.entry(*id);if(record==records_.end()||!entry||entry->data!=record->data.lock()){records_.remove(*id);id=found->erase(id);continue;}bool pgr=candidate["profileId"]=="native-pgr34-track"||candidate["profileId"]=="native-pgr34-car";if(pgr&&candidate.contains("pgrMaterialTarget")&&entry->id!=candidate["pgrMaterialTarget"].toInt()){++id;continue;}bool changedPgr=pgr&&candidate["pgrMaterialSource"].toString()!=entry->material.sourceBindings["materialMapChoice"].toString();bool targetPgr=pgr&&candidate.contains("pgrMaterialTarget");if(entry->visible&&(targetPgr||changedPgr||(entry->positionDelta==std::array<double,3>{}&&entry->rotationDegrees==std::array<double,3>{}&&entry->scale==std::array<double,3>{1,1,1}))){plan.existing=*id;if(changedPgr||targetPgr){plan.refreshMaterial=true;return plan;}if(candidate["profileId"]=="native-genki-part"&&([&]{for(auto key:{"genkiTrackBinding","textureOffset","uvScaleU","uvScaleV","uvOffsetU","uvOffsetV","textureWrapU","textureWrapV"})if(candidate[key]!=entry->material.sourceBindings[key])return true;return false;})()){plan.refreshMaterial=true;return plan;}plan.skip=true;return plan;}++id;}
            }
            return plan;
        }
        QSet<QString> covered;QSet<int> visible;
        for(auto it=records_.begin();it!=records_.end();){auto *entry=scene.entry(it.key());if(!entry||entry->data!=it->data.lock()){it=records_.erase(it);continue;}if(entry->visible&&entry->positionDelta==std::array<double,3>{}&&entry->rotationDegrees==std::array<double,3>{}&&entry->scale==std::array<double,3>{1,1,1}){visible.insert(it.key());covered.unite(it->instances);}++it;}
        if(candidate["profileId"]=="native-nfsu2-scene"||linked){
            QJsonArray pending; qint64 vertices=0,indices=0;auto original=candidate[linked?"linkedDraws":"nfsu2Instances"].toArray();
            for(auto value:original){auto item=value.toObject();auto identity=item["instanceOffset"].toString()+":"+item["objectOffset"].toString();if(linked){auto settings=item["settings"].toObject();identity+=":"+settings["indexOffset"].toString()+":"+QString::number(settings["indexCount"].toInteger());}if(covered.contains(identity)||plan.instances.contains(identity))continue;plan.instances.insert(identity);pending.append(item);auto settings=item["settings"].toObject();vertices+=settings["vertexCount"].toInteger();indices+=settings["indexCount"].toInteger();}
            if(pending.isEmpty()&&!original.isEmpty()){plan.skip=true;for(auto it=records_.cbegin();it!=records_.cend();++it)if(visible.contains(it.key())&&!it->instances.isEmpty()){plan.existing=it.key();break;}return plan;}
            if(pending.size()!=original.size()){plan.candidate[linked?"linkedDraws":"nfsu2Instances"]=pending;auto settings=candidate["settings"].toObject();settings["vertexCount"]=vertices;settings["indexCount"]=indices;plan.candidate["settings"]=settings;plan.candidate["name"]=candidate["name"].toString()+" / remaining instances";}
            return plan;
        }
        return plan;
    }
    void added(int id,const Plan &plan,const std::shared_ptr<MeshData> &data){records_[id]={plan.key,plan.instances,data};if(!plan.key.isEmpty())byKey_[plan.key].insert(id);}
private:
    struct Record {QByteArray key;QSet<QString> instances;std::weak_ptr<MeshData> data;};QMap<int,Record> records_;QMap<QByteArray,QSet<int>> byKey_;
};
