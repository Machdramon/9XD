#include "native_resources.h"
#include "geometry_candidates.h"
#include "genki_mesh_evidence.h"
#include <QCoreApplication>
#include <QDirIterator>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTextStream>

// Optional developer corpus runner: production readers/codecs, no AI or extracted files.
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);auto args=app.arguments();if(args.size()<3){QTextStream(stderr)<<"Usage: native_asset_audit <file-or-folder> <report.json> [--verify] [--patterns] [--genki]\n";return 2;}QStringList paths;QFileInfo root(args[1]);if(root.isFile())paths.append(root.absoluteFilePath());else if(root.isDir()){QDirIterator it(root.absoluteFilePath(),QDir::Files,QDirIterator::Subdirectories);while(it.hasNext())paths.append(it.next());}else return 2;
    std::atomic_bool cancel=false;QJsonArray files;int recognized=0,failed=0,verified=0,entryCount=0;bool verify=args.contains("--verify"),patterns=args.contains("--patterns");
    for(const auto &path:paths){if(QFileInfo(args[2]).absoluteFilePath()==QFileInfo(path).absoluteFilePath())continue;QFileInfo info(path);ReadRequest source{path,0,info.size(),info.lastModified(),0};auto resources=NativeResources::inspect(source,{},cancel);QJsonObject row{{"path",root.isDir()?QDir(root.absoluteFilePath()).relativeFilePath(path):info.fileName()},{"bytes",QString::number(info.size())},{"modified",info.lastModified().toString(Qt::ISODateWithMs)},{"container",resources}};auto entries=resources["entries"].toArray();entryCount+=entries.size();if(!resources["format"].toString().isEmpty())++recognized;if(resources.contains("error"))++failed;
        if(verify){QJsonArray checks;qint64 total=0;for(auto entry:entries){auto e=entry.toObject();QJsonObject check{{"name",e["name"]}};auto decoded=e["decodedBytes"].toString().toLongLong();if(!e["supported"].toBool()||decoded>NativeResources::MaxOutput||(decoded>=0?decoded:NativeResources::MaxOutput)>512*1024*1024-total){check["status"]="skipped: unsupported or per-file verification budget";}else{auto output=NativeResources::extract(source,{},e,cancel);if(!output.error.isEmpty()){check["error"]=output.error;++failed;}else{++verified;total+=output.bytes.size();check["status"]="decoded";check["bytes"]=QString::number(output.bytes.size());check["headHex"]=QString::fromLatin1(output.bytes.left(32).toHex(' '));}}checks.append(check);}row["verification"]=checks;}
        if(patterns&&entries.isEmpty()&&!resources.contains("error")){QString error;auto bytes=NativeResources::read(source,{},0,qMin(source.expectedSize,qint64(8*1024*1024)),cancel,error);row["geometrySearch"]=error.isEmpty()?GeometryCandidates::scan(bytes,cancel):QJsonObject{{"error",error}};}
        if(args.contains("--genki")){QString error;auto bytes=NativeResources::read(source,{},0,qMin(source.expectedSize,qint64(32*1024*1024)),cancel,error);auto evidence=error.isEmpty()?genkiMeshEvidence(bytes,source.expectedSize,&cancel):QJsonObject{{"error",error}};if(!evidence.isEmpty()){evidence["chunkCount"]=evidence["previewChunks"].toArray().size();evidence.remove("previewChunks");evidence.remove("regions");row["genki"]=evidence;}}
        files.append(row);
    }
    QJsonObject report{{"format","9XD Native Asset Audit"},{"version",1},{"files",files},{"summary",QJsonObject{{"files",files.size()},{"recognizedContainers",recognized},{"entries",entryCount},{"verifiedResources",verified},{"errors",failed}}},{"limits","128 MiB packed/decoded per entry, 512 MiB decoded verification per file; pattern search first 8 MiB only. Header/codec success is not proof of geometry correctness."}};QSaveFile output(args[2]);auto bytes=QJsonDocument(report).toJson(QJsonDocument::Indented);if(!output.open(QIODevice::WriteOnly)||output.write(bytes)!=bytes.size()||!output.commit())return 3;QTextStream(stdout)<<QJsonDocument(report["summary"].toObject()).toJson(QJsonDocument::Compact)<<"\n";return failed?1:0;
}
