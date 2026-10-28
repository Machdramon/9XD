#pragma once
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <memory>
// Optional local JSONL profiling. Decode timings include texture subset; GUI stages are separate.
namespace MeshLoadTrace {
struct Sample {QElapsedTimer clock;QString name;QMap<QString,qint64> ns;qint64 doneNs=0;Sample(QString n):name(n){clock.start();}};
inline thread_local Sample *current=nullptr;
inline std::shared_ptr<Sample> begin(QString name){return qEnvironmentVariableIsEmpty("WORKBENCH_MESH_TRACE")?nullptr:std::make_shared<Sample>(name);}
struct Bind {Sample *prior;explicit Bind(Sample *s):prior(current){current=s;}~Bind(){current=prior;}};
struct Stage {Sample *sample;QString name;QElapsedTimer timer;explicit Stage(QString n,Sample *s=current):sample(s),name(n){if(sample)timer.start();}~Stage(){if(sample)sample->ns[name]+=timer.nsecsElapsed();}};
inline void finish(const std::shared_ptr<Sample> &s,int vertices,int faces,bool textured){if(!s)return;static QElapsedTimer epoch;static qint64 previous=0;static QFile file;if(!epoch.isValid())epoch.start();auto path=qEnvironmentVariable("WORKBENCH_MESH_TRACE");if(file.fileName()!=path){file.close();file.setFileName(path);}if(!file.isOpen()&&!file.open(QIODevice::WriteOnly|QIODevice::Append))return;QJsonObject row{{"name",s->name},{"elapsed_ms",epoch.elapsed()},{"vertices",vertices},{"faces",faces},{"textured",textured},{"dispatch_gap_ms",previous?double(epoch.nsecsElapsed()-previous-s->clock.nsecsElapsed())/1e6:0.}};for(auto it=s->ns.cbegin();it!=s->ns.cend();++it)row[it.key()+"_ms"]=double(it.value())/1e6;row["worker_to_gui_ms"]=double(s->clock.nsecsElapsed()-s->doneNs-s->ns.value("scene")-s->ns.value("library")-s->ns.value("registry"))/1e6;row["draw_total_ms"]=double(s->clock.nsecsElapsed())/1e6;file.write(QJsonDocument(row).toJson(QJsonDocument::Compact)+"\n");file.flush();previous=epoch.nsecsElapsed();}
}
