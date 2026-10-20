#include "fbx_export.h"
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <stdexcept>
#ifdef WORKBENCH_FBX_SDK
#include <fbxsdk.h>
#endif
bool fbxExportAvailable(){
#ifdef WORKBENCH_FBX_SDK
    return true;
#else
    return false;
#endif
}
QByteArray encodeFbx(const QVector<ExportMesh> &meshes,const ModelExportOptions &options,const std::shared_ptr<ExportProgress> &progress){
#ifndef WORKBENCH_FBX_SDK
    throw std::runtime_error("FBX export requires a build configured with the Autodesk FBX SDK.");
#else
    auto check=[&]{if(progress&&progress->cancel.load())throw std::runtime_error("Export canceled; the destination was not replaced.");};
    QTemporaryDir directory;if(!directory.isValid())throw std::runtime_error("Could not create the FBX staging directory.");
    auto *manager=FbxManager::Create();if(!manager)throw std::runtime_error("Could not create the FBX manager.");
    struct Cleanup {FbxManager *p;~Cleanup(){p->Destroy();}} cleanup{manager};
    auto *io=FbxIOSettings::Create(manager,IOSROOT);manager->SetIOSettings(io);io->SetBoolProp(EXP_FBX_EMBEDDED,true);
    auto *scene=FbxScene::Create(manager,"Binary Workbench");scene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::OpenGL);scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
    auto *info=FbxDocumentInfo::Create(manager,"SceneInfo");info->mTitle="Binary Workbench scene";info->Original_ApplicationName="Binary Workbench";scene->SetSceneInfo(info);
    int index=0;for(const auto &item:meshes){check();const auto &data=*item.data;auto name=item.name.toUtf8();auto *mesh=FbxMesh::Create(scene,name.constData());auto *node=FbxNode::Create(scene,name.constData());node->SetNodeAttribute(mesh);scene->GetRootNode()->AddChild(node);
        mesh->InitControlPoints(int(data.vertices.size()));auto *positions=mesh->GetControlPoints();for(int i=0;i<data.vertices.size();++i){if(i%1024==0)check();const auto &v=data.vertices[i];positions[i]=FbxVector4(v.position[0],v.position[1],v.position[2]);}
        if(options.normals!=ExportNormals::Omit){auto *normals=mesh->CreateElementNormal();normals->SetMappingMode(FbxGeometryElement::eByControlPoint);normals->SetReferenceMode(FbxGeometryElement::eDirect);for(const auto &v:data.vertices)normals->GetDirectArray().Add(FbxVector4(v.normal[0],v.normal[1],v.normal[2]));}
        if(data.hasUvs){auto *uv=mesh->CreateElementUV("UVMap");uv->SetMappingMode(FbxGeometryElement::eByControlPoint);uv->SetReferenceMode(FbxGeometryElement::eDirect);for(const auto &v:data.vertices)uv->GetDirectArray().Add(FbxVector2(v.uv[0],v.uv[1]));}
        for(int i=0;i<data.triangles.size();i+=3){if(i%3072==0)check();mesh->BeginPolygon(options.sceneMaterials?0:-1);for(int j=0;j<3;++j)mesh->AddPolygon(data.triangles[i+j]);mesh->EndPolygon();}
        if(options.sceneMaterials){auto *mat=FbxSurfaceLambert::Create(scene,item.surface.name.toUtf8().constData());auto color=item.surface.baseColor;mat->Diffuse.Set(FbxDouble3(color.redF(),color.greenF(),color.blueF()));mat->DiffuseFactor.Set(item.surface.unlit?0.:1.);if(item.surface.unlit){mat->Emissive.Set(mat->Diffuse.Get());mat->EmissiveFactor.Set(1.);}node->AddMaterial(mat);
            auto *binding=mesh->CreateElementMaterial();binding->SetMappingMode(FbxGeometryElement::eAllSame);binding->SetReferenceMode(FbxGeometryElement::eIndexToDirect);binding->GetIndexArray().Add(0);
            if(options.textures&&data.hasUvs&&!item.texture.isNull()){check();auto path=directory.filePath(QString("texture_%1.png").arg(index));if(!item.texture.save(path,"PNG"))throw std::runtime_error("Could not encode the assigned texture.");auto *texture=FbxFileTexture::Create(scene,"BaseColor");texture->SetFileName(path.toUtf8().constData());texture->SetRelativeFileName(QFileInfo(path).fileName().toUtf8().constData());texture->SetTextureUse(FbxTexture::eStandard);texture->SetMappingType(FbxTexture::eUV);texture->UVSet.Set("UVMap");mat->Diffuse.ConnectSrcObject(texture);if(item.surface.unlit)mat->Emissive.ConnectSrcObject(texture);}
        }++index;
    }
    check();auto path=directory.filePath("scene.fbx");auto *exporter=FbxExporter::Create(manager,"");auto registry=manager->GetIOPluginRegistry();int writer=registry->GetNativeWriterFormat();
    exporter->SetProgressCallback([](void *arg,float percentage,const char *){auto *p=static_cast<ExportProgress *>(arg);if(!p)return true;p->percent.store(90+int(percentage*.09f));return !p->cancel.load();},progress.get());
    if(!exporter->Initialize(path.toUtf8().constData(),writer,io)||!exporter->Export(scene))throw std::runtime_error(exporter->GetStatus().GetErrorString());
    check();QFile file(path);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("Could not read the exported FBX.");return file.readAll();
#endif
}
