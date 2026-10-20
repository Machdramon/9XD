#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QJsonDocument>
#include <QMessageBox>
#include <QSaveFile>
#include "file_dialogs.h"
#include <cmath>
class MeshSetupSession final:public QObject {
public:
    explicit MeshSetupSession(QWidget *root):QObject(root),root_(root){setObjectName("meshSetupSession");reset();
        for(auto *w:root_->findChildren<QLineEdit *>())connect(w,&QLineEdit::textChanged,this,[this]{update();});
        for(auto *w:root_->findChildren<QComboBox *>())connect(w,&QComboBox::currentIndexChanged,this,[this]{update();});
        for(auto *w:root_->findChildren<QCheckBox *>())connect(w,&QCheckBox::toggled,this,[this]{update();});
        for(auto *w:root_->findChildren<QDoubleSpinBox *>())connect(w,&QDoubleSpinBox::valueChanged,this,[this]{update();});
    }
    QJsonObject state()const{QJsonObject result;for(auto *w:root_->findChildren<QWidget *>()){
        auto key=w->objectName();if(!key.startsWith("mesh")||key=="meshDestination")continue;
        if(auto *v=qobject_cast<QLineEdit *>(w))result[key]=v->text();else if(auto *v=qobject_cast<QComboBox *>(w))result[key]=v->currentText();
        else if(auto *v=qobject_cast<QCheckBox *>(w))result[key]=v->isChecked();else if(auto *v=qobject_cast<QDoubleSpinBox *>(w))result[key]=v->value();}return result;}
    bool empty()const{return state()==blank_;}
    bool dirty()const{return state()!=saved_;}
    QString path()const{return path_;}
    void markLoaded(QString path){path_=std::move(path);saved_=state();ignored_={};update();}
    void reset(){
        for(auto *w:root_->findChildren<QLineEdit *>())if(w->objectName().startsWith("mesh"))w->clear();
        for(auto *w:root_->findChildren<QCheckBox *>())if(w->objectName().startsWith("mesh"))w->setChecked(false);
        for(auto *w:root_->findChildren<QDoubleSpinBox *>())if(w->objectName().startsWith("mesh"))w->setValue(w->objectName().contains("Scale")?1:0);
        for(auto *w:root_->findChildren<QComboBox *>())if(w->objectName().startsWith("mesh")){
            auto key=w->objectName();bool unknown=QStringList{"meshPositionType","meshIndexType","meshTopology","meshUvType","meshNormalType"}.contains(key);
            w->setPlaceholderText("Select...");w->setCurrentIndex(unknown?-1:0);}
        blank_=saved_=state();path_.clear();ignored_={};update();
    }
    bool save(QString path={}){
        if(path.isEmpty())path=path_;if(path.isEmpty())path=QFileDialog::getSaveFileName(root_,"Save Mesh Setup",WorkbenchFiles::lastDirectory()+"/mesh-setup.bwmesh","Mesh setup (*.bwmesh *.json)");
        if(path.isEmpty())return false;
        auto bytes=QJsonDocument(QJsonObject{{"format","BinaryWorkbench Mesh Setup"},{"version",1},{"controls",state()}}).toJson();QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){QMessageBox::warning(root_,"Save Mesh Setup","Could not save the mesh setup.");return false;}
        markLoaded(path);return true;
    }
    bool loadDraft(const QJsonObject &json,QString &error){
        if(json["format"]!="BinaryWorkbench Mesh Setup"||json["version"]!=1||!json["controls"].isObject()){error="Unsupported mesh setup format/version.";return false;}
        auto values=json["controls"].toObject();auto known=state();
        for(auto i=values.begin();i!=values.end();++i){if(!known.contains(i.key())){error="Unknown mesh field: "+i.key();return false;}auto *w=root_->findChild<QWidget *>(i.key());bool valid=false;
            if(auto *v=qobject_cast<QLineEdit *>(w))valid=i->isString()&&i->toString().size()<=v->maxLength();
            else if(auto *v=qobject_cast<QComboBox *>(w))valid=i->isString()&&(i->toString().isEmpty()||v->findText(i->toString())>=0);
            else if(qobject_cast<QCheckBox *>(w))valid=i->isBool();
            else if(auto *v=qobject_cast<QDoubleSpinBox *>(w))valid=i->isDouble()&&std::isfinite(i->toDouble())&&i->toDouble()>=v->minimum()&&i->toDouble()<=v->maximum();
            if(!valid){error="Invalid mesh field: "+i.key();return false;}}
        reset();for(auto i=values.begin();i!=values.end();++i){auto *w=root_->findChild<QWidget *>(i.key());if(auto *v=qobject_cast<QLineEdit *>(w))v->setText(i->toString());else if(auto *v=qobject_cast<QComboBox *>(w))v->setCurrentIndex(i->toString().isEmpty()?-1:v->findText(i->toString()));else if(auto *v=qobject_cast<QCheckBox *>(w))v->setChecked(i->toBool());else if(auto *v=qobject_cast<QDoubleSpinBox *>(w))v->setValue(i->toDouble());}return true;
    }
    bool confirmClose(){if(!dirty()||ignored_==state())return true;
        auto answer=QMessageBox::warning(root_,"Unsaved 3D Setup","Save the 3D setup before closing this document?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
        if(answer==QMessageBox::Cancel)return false;if(answer==QMessageBox::Save)return save();ignored_=state();return true;}
private:
    void update(){if(auto *label=root_->findChild<QLabel *>("meshSetupState"))label->setText(dirty()?"3D setup · Unsaved changes":path_.isEmpty()?"3D setup · Empty":"3D setup · "+QFileInfo(path_).fileName());}
    QWidget *root_;QJsonObject blank_,saved_,ignored_;QString path_;
};
inline MeshSetupSession *meshSetupSession(QWidget *root){if(!root)return nullptr;return dynamic_cast<MeshSetupSession *>(root->findChild<QObject *>("meshSetupSession"));}
