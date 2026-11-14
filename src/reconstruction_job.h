#pragma once
#include "mesh_data.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QFutureWatcher>
#include <QTimer>
#include <functional>
#include <atomic>
namespace Reconstruction {
struct ReconstructionOptions {int threads=4,imageSize=1600,denseSize=1024,overlap=10,poissonDepth=8;double cacheGiB=2,poissonTrim=4;bool exhaustive=false,singleCamera=true;int maxVertices=2000000,maxFaces=4000000;};
struct SparseSummary {int images=0,points=0,components=0;double meanError=0;QJsonArray cameras;QString error;QJsonObject json()const;};
struct ReconstructionOutput {QString run,model,cloud,surface,mode,error;SparseSummary summary;std::shared_ptr<MeshData> geometry;bool canceled=false;};
struct PlyResult {std::shared_ptr<MeshData> mesh;QString error;bool colors=false;};
PlyResult readReconstructionPly(const QString &,int maxVertices,int maxFaces,const std::atomic_bool &cancel);
SparseSummary readSparseSummary(const QString &,int components,const std::atomic_bool &cancel);
QString preferredColmapPath();
class ReconstructionJob final:public QObject {
public:
    explicit ReconstructionJob(QObject *parent=nullptr);
    ~ReconstructionJob()override;
    bool busy()const{return busy_;}
    std::function<void(const QString &)> log;
    std::function<void(const QString &,int,int)> progress;
    std::function<void(const ReconstructionOutput &)> finished;
    bool sparse(const QString &tool,const QString &job,const ReconstructionOptions &options);
    bool surface(const QString &tool,const ReconstructionOutput &sparse,const ReconstructionOptions &options,bool dense);
    void cancel();
private:
    struct Command {QString title;QStringList args;};
    void begin(const QString &tool);void next();void ended(int code,QProcess::ExitStatus);void conclude(QString error={});void validate();bool save();
    QProcess *process_;QTimer *deadline_;QFutureWatcher<ReconstructionOutput> *validation_;
    bool busy_=false,sparse_=false,exporting_=false;std::atomic_bool canceled_=false;QString tool_,job_,denseDir_;ReconstructionOptions options_;ReconstructionOutput output_;QList<Command> commands_;int done_=0,total_=0;QJsonObject manifest_;QJsonArray history_;QByteArray tail_;
};
}
