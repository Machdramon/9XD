// Headless test adapter. Uses the same bounded reader and decoders as the UI.
#include "ai_decoder_setup.h"
#include "image_decode.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <iostream>
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QByteArray input;
    for(std::string line;std::getline(std::cin,line);){input+=QByteArray::fromStdString(line);if(input.size()>65536)break;}
    QJsonObject result;auto output=[&](int code){std::cout<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<std::endl;return code;};
    QJsonParseError parse;auto json=QJsonDocument::fromJson(input,&parse);if(input.size()>65536||parse.error!=QJsonParseError::NoError||!json.isObject()){result={{"error","Invalid probe request (maximum 64 KiB)."}};return output(1);}
    auto request=json.object();QFileInfo info(request["file"].toString());QDir out(request["output"].toString());
    if(!info.isFile()||info.isSymLink()||info.size()>256*1024*1024||request["output"].toString().isEmpty()||!QDir().mkpath(out.absolutePath())){result={{"error","Invalid input file or output directory."}};return output(1);}
    ReadRequest source{info.absoluteFilePath(),0,info.size(),info.lastModified(),0,{}};auto args=request["settings"].toObject();QString error;
    if(request["kind"]=="image"){
        ImageRecipe recipe;int count;qint64 step;
        if(!AiDecoderSetup::image(args,info.size(),recipe,count,step,error)){result={{"error",error},{"stage","validation"}};return output(1);}
        if(count!=1){result={{"error","Probe validates one image at a time; use count 1."}};return output(1);}
        std::atomic_bool cancel{false};auto decoded=decodeBinaryImage(source,{},recipe,cancel);
        if(decoded.image.isNull()||!decoded.error.isEmpty()){result={{"error",decoded.error},{"stage","decoder"}};return output(1);}
        auto image=decoded.image.convertToFormat(QImage::Format_RGBA8888);QCryptographicHash hash(QCryptographicHash::Sha256);for(int y=0;y<image.height();++y)hash.addData(QByteArrayView(reinterpret_cast<const char *>(image.constScanLine(y)),image.width()*4));
        auto preview=out.filePath("preview.png");if(!image.save(preview,"PNG")){result={{"error","Could not save preview."}};return output(1);}
        QFile recipeFile(out.filePath("image.bwir"));if(!recipeFile.open(QIODevice::WriteOnly)){result={{"error","Could not save image recipe."}};return output(1);}recipeFile.write(QJsonDocument(encodeImageRecipe(recipe)).toJson());
        result={{"accepted",true},{"kind","image"},{"width",image.width()},{"height",image.height()},{"pixelSha256",QString::fromLatin1(hash.result().toHex())},{"preview",preview},{"recipe",recipeFile.fileName()},{"assessment","Decoder accepted; visual/ground-truth verification still required."}};
    }else if(request["kind"]=="mesh"){
        MeshSpec spec;if(!AiDecoderSetup::mesh(args,info.size(),spec,error)){result={{"error",error},{"stage","validation"}};return output(1);}
        auto decoded=readMesh(source,{},spec);if(!decoded.mesh||!decoded.error.isEmpty()){result={{"error",decoded.error},{"stage","decoder"}};return output(1);}
        QJsonArray low,high;for(int axis=0;axis<3;++axis){double a=INFINITY,b=-INFINITY;for(auto &v:decoded.mesh->vertices){double p=v.position[axis]+decoded.positionDelta[axis];a=qMin(a,p);b=qMax(b,p);}low.append(a);high.append(b);}
        int degenerate=0;auto &m=*decoded.mesh;for(int i=0;i<m.triangles.size();i+=3){auto &a=m.vertices[m.triangles[i]],&b=m.vertices[m.triangles[i+1]],&c=m.vertices[m.triangles[i+2]];double u[3],v[3];for(int k=0;k<3;++k){u[k]=b.position[k]-a.position[k];v[k]=c.position[k]-a.position[k];}if(std::hypot(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])<1e-12)++degenerate;}
        QFile setup(out.filePath("mesh-settings.json"));if(!setup.open(QIODevice::WriteOnly)){result={{"error","Could not save mesh settings."}};return output(1);}setup.write(QJsonDocument(args).toJson());
        result={{"accepted",true},{"kind","mesh"},{"vertices",m.vertices.size()},{"triangles",m.triangles.size()/3},{"degenerateTriangles",degenerate},{"minimum",low},{"maximum",high},{"settings",setup.fileName()},{"assessment","Decoder accepted; plausible geometry is not proof of a correct format."}};
    }else{result={{"error","Kind must be image or mesh."}};return output(1);}
    auto check=readPage(source);if(!check.error.isEmpty()){result={{"error",check.error},{"stage","final source check"}};return output(1);}return output(0);
}
