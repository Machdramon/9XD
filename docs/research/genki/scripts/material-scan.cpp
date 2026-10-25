#include "genki_materials.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>
int main(int argc,char **argv){QCoreApplication app(argc,argv);QFile f(QString::fromLocal8Bit(argv[1]));if(!f.open(QIODevice::ReadOnly))return 1;auto b=f.readAll();auto e=genkiMeshEvidence(b,b.size());std::atomic_bool cancel=false;auto m=GenkiMaterials::inspect(b,e,cancel);m["geometry"]=e;QFile o(QString::fromLocal8Bit(argv[2]));if(!o.open(QIODevice::WriteOnly))return 2;o.write(QJsonDocument(m).toJson());}
