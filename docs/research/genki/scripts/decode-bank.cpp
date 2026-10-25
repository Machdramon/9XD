#include "image_containers.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
int main(int argc,char**argv){QCoreApplication app(argc,argv);QFile f(QString::fromLocal8Bit(argv[1]));if(!f.open(QIODevice::ReadOnly))return 1;auto b=f.readAll();auto start=b.indexOf("TIM2");if(start<0)return 2;auto bank=b.mid(start);int count=qFromLittleEndian<quint16>(bank.constData()+6);std::atomic_bool cancel=false;QJsonArray rows;for(int i=0;i<count;++i){ImageRecipe r;r.surface=i;QString error;auto image=decodeTim2(bank,r,error,cancel);rows.append(QJsonObject{{"surface",i},{"width",image.width()},{"height",image.height()},{"error",error},{"ok",!image.isNull()}});}QFile out(QString::fromLocal8Bit(argv[2]));if(!out.open(QIODevice::WriteOnly))return 3;out.write(QJsonDocument(rows).toJson());}
