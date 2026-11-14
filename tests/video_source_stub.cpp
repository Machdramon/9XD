#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);auto args=app.arguments();auto mode=qEnvironmentVariable("NINES_VIDEO_STUB_MODE");
    auto command=args.value(1);auto option=[&](const QString &name){int i=args.indexOf(name);return i>=0?args.value(i+1):QString();};
    if(QStringList{"feature_extractor","sequential_matcher","exhaustive_matcher","mapper","model_converter","delaunay_mesher","image_undistorter","patch_match_stereo","stereo_fusion","poisson_mesher"}.contains(command)){
        if(mode=="fail")return 2;if(mode=="noinitialpair"&&command=="mapper"){std::fputs("No good initial image pair found.\nFailed to create any sparse model\n",stderr);return 2;}if(mode=="hold-sfm"){QTimer::singleShot(30000,&app,&QCoreApplication::quit);return app.exec();}
        auto write=[](const QString &path,const QByteArray &data){QDir().mkpath(QFileInfo(path).absolutePath());QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(data)==data.size();};
        auto cloud=[](){QByteArray b="ply\nformat ascii 1.0\nelement vertex 20\nproperty float x\nproperty float y\nproperty float z\nend_header\n";for(int i=0;i<20;++i)b+=QByteArray::number(i%2)+" "+QByteArray::number((i/2)%2)+" "+QByteArray::number(i/4)+"\n";return b;};
        auto mesh=[](){return QByteArray("ply\nformat ascii 1.0\nelement vertex 4\nproperty float x\nproperty float y\nproperty float z\nelement face 4\nproperty list uchar int vertex_indices\nend_header\n0 0 0\n1 0 0\n0 1 0\n0 0 1\n3 0 2 1\n3 0 1 3\n3 1 2 3\n3 2 0 3\n");};
        if(command=="mapper"){QByteArray count(8,0);count[0]=3;write(option("--output_path")+"/0/images.bin",count);}
        if(command=="model_converter"){
            auto output=option("--output_path");if(option("--output_type")=="PLY")write(output,cloud());else{
                QByteArray images,points;for(int i=0;i<3;++i)images+=QByteArray::number(i+1)+" 1 0 0 0 "+QByteArray::number(i)+" 0 0 1 frame-00000"+QByteArray::number(i+1)+".png\n\n";
                for(int i=0;i<20;++i)points+=QByteArray::number(i+1)+" 0 0 0 255 255 255 0.2 1 0 2 0\n";
                write(output+"/images.txt",images);write(output+"/points3D.txt",points);write(output+"/cameras.txt","1 SIMPLE_RADIAL 32 32 30 16 16 0\n");
            }
        }
        if(command=="patch_match_stereo"&&args.contains("-h"))std::puts(mode=="nocuda"?"COLMAP without GPU support":"COLMAP with CUDA");
        if(command=="stereo_fusion")write(option("--output_path"),cloud());
        if(command=="delaunay_mesher"||command=="poisson_mesher")write(option("--output_path"),mesh());
        return 0;
    }
    if(mode=="fail"){std::fputs("fixture downloader failure\n",stderr);return 2;}
    if(mode=="hold-inspect"||(mode=="hold-download"&&!args.contains("--dump-single-json"))){QTimer::singleShot(30000,&app,&QCoreApplication::quit);return app.exec();}
    if(args.contains("--dump-single-json")){
        if(mode=="malformed"){std::puts("not-json");return 0;}
        QJsonObject j{{"id","BaW_jenozKc"},{"title","Synthetic test video"},{"duration",4},{"width",320},{"height",240},{"is_live",mode=="live"}};
        auto json=QJsonDocument(j).toJson(QJsonDocument::Compact);std::puts(json.constData());return 0;
    }
    int at=args.indexOf("--output");if(at<0||at+1>=args.size())return 3;
    auto output=args[at+1];output.replace("%(ext)s","mp4");
    if(mode=="oversize"||mode=="oversize-final"){QFile f(output+(mode=="oversize"?".part":""));if(!f.open(QIODevice::WriteOnly))return 4;f.resize(18*1024*1024);f.close();if(mode=="oversize-final"){auto line=("9XD_FILE:"+output).toUtf8();std::puts(line.constData());return 0;}QTimer::singleShot(30000,&app,&QCoreApplication::quit);return app.exec();}
    if(!QFile::copy(qEnvironmentVariable("NINES_VIDEO_STUB_FILE"),output))return 5;
    std::puts("9XD_PROGRESS:100:100");auto line=("9XD_FILE:"+output).toUtf8();std::puts(line.constData());return 0;
}
