#pragma once
#include "theme.h"
#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QBoxLayout>
#include <QColorDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QSaveFile>
#include <QFile>
#include <QSignalBlocker>
#include <QUuid>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QFontDatabase>
#include <QMessageBox>
#include <cmath>

class ThemePreview final:public QWidget {
public:
    QJsonObject theme;
    explicit ThemePreview(QWidget *parent=nullptr):QWidget(parent){setObjectName("themePreview");setMinimumSize(245,485);setAccessibleName("Theme preview: controls, hex bytes, geometry and code");}
    void paintEvent(QPaintEvent *)override{
        auto colors=theme["colors"].toObject();auto palette=workbenchThemePalette(theme["base"].toString("dark"),colors);auto c=[&](QString key){return resolvedThemeColor(key,palette,colors);};auto options=theme["options"].toObject();QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),palette.color(QPalette::Window));int w=QWidget::width();
        auto text=[&](int x,int y,QString text,QColor color){p.setPen(color);p.drawText(x,y,text);};
        text(12,25,"LIVE PREVIEW",palette.color(QPalette::PlaceholderText));
        p.fillRect(QRect(12,38,w-24,31),palette.color(QPalette::Button));p.fillRect(QRect(12,38,75,31),palette.highlight());text(21,59,"Selected",palette.color(QPalette::HighlightedText));text(99,59,"Other tab",palette.color(QPalette::Text));
        if(options.value("fieldBackground").toBool(true))p.fillRect(QRect(12,81,w-24,31),palette.base());if(options.value("borders").toBool(true)){p.setPen(palette.color(QPalette::Mid));p.drawRect(QRect(12,81,w-25,31));}text(21,102,"Input field",palette.color(QPalette::Text));
        p.fillRect(QRect(12,127,w-24,76),palette.base());p.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));text(18,147,"0x0010",c("hex.offset"));text(88,147,"48 65 6C",c("hex.bytes"));text(18,170,"Modified",c("hex.modified"));text(120,170,"Hello",c("hex.decoded"));p.fillRect(QRect(18,180,65,18),palette.highlight());text(22,194,"41 42",palette.color(QPalette::HighlightedText));
        p.fillRect(QRect(12,215,w-24,100),c("graphics.background"));p.setPen(c("graphics.grid"));for(int i=0;i<5;++i)p.drawLine(15,231+i*17,w-15,231+i*17);p.setPen(QPen(c("graphics.model"),2));p.drawLine(w/2,225,45,302);p.drawLine(45,302,w-45,302);p.drawLine(w-45,302,w/2,225);p.setPen(QPen(c("graphics.selection"),4));p.drawPoint(w/2,225);
        p.fillRect(QRect(12,327,w-24,56),c("script.background"));text(18,349,"value = read(0, 16)",c("script.text"));text(18,371,"> ready",c("console.text"));
        text(18,410,"7   8   9",c("calculator.digit"));text(118,410,"A F",c("calculator.hex"));text(180,410,"+ ×",c("calculator.operator"));text(18,440,"AND XOR",c("calculator.bits"));text(125,440,"MS MR",c("calculator.memory"));p.setFont(font());text(12,472,"Disabled",palette.color(QPalette::Disabled,QPalette::Text));
    }
};
class ThemeEditor final:public QWidget {
public:
    ThemeEditor(QComboBox *selector,QSettings &settings,QWidget *parent=nullptr):QWidget(parent),selector_(selector){
        setObjectName("themeEditor");resize(900,550);QString ignored;auto raw=QJsonDocument::fromJson(settings.value("themes/v1").toByteArray()).object();for(auto it=raw.begin();it!=raw.end();++it)if(it.key().startsWith("custom:")&&validateTheme(it.value().toObject(),ignored))library_[it.key()]=it.value();
        auto *layout=new QVBoxLayout(this);auto *top=new QHBoxLayout;top->addWidget(new QLabel("Theme"));top->addWidget(selector_,1);base_=new QComboBox;base_->setObjectName("themeBase");base_->addItem("Dark base","dark");base_->addItem("Light base","light");base_->addItem("System base","system");top->addWidget(base_);layout->addLayout(top);
        auto *actions=new QHBoxLayout;auto action=[&](QString label,auto callback){auto *button=new QPushButton(label);actions->addWidget(button);connect(button,&QPushButton::clicked,this,callback);return button;};
        action("New...",[this]{bool ok;auto name=QInputDialog::getText(this,"New Theme","Theme name",QLineEdit::Normal,"My Theme",&ok);if(ok&&!name.trimmed().isEmpty())createTheme(name,false);});
        action("Duplicate...",[this]{bool ok;auto name=QInputDialog::getText(this,"Duplicate Theme","Theme name",QLineEdit::Normal,selector_->currentText()+" Copy",&ok);if(ok&&!name.trimmed().isEmpty())createTheme(name,true);});
        rename_=action("Rename...",[this]{bool ok;auto name=QInputDialog::getText(this,"Rename Theme","Theme name",QLineEdit::Normal,current()["name"].toString(),&ok);if(ok&&!name.trimmed().isEmpty()&&name.trimmed().size()<=64){auto theme=current();theme["name"]=name.trimmed();library_[id()]=theme;reload(id());}});
        remove_=action("Delete",[this]{if(QMessageBox::question(this,"Delete Theme","Delete this custom theme?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes)return;library_.remove(id());reload("dark");});
        action("Import...",[this]{auto path=QFileDialog::getOpenFileName(this,"Import Theme",{},"Workbench theme (*.bwtheme *.json)");if(path.isEmpty())return;QString error;if(!importTheme(path,error))QMessageBox::warning(this,"Import Theme",error);});
        action("Export...",[this]{auto path=QFileDialog::getSaveFileName(this,"Export Theme",current()["name"].toString()+".bwtheme","Workbench theme (*.bwtheme)");if(path.isEmpty())return;QString error;if(!exportTheme(path,error))QMessageBox::warning(this,"Export Theme",error);});layout->addLayout(actions);
        auto *split=new QSplitter;layout->addWidget(split,1);categories_=new QTreeWidget;categories_->setObjectName("themeCategories");categories_->setHeaderHidden(true);categories_->setMaximumWidth(170);for(auto group:{"Backgrounds","Text","Accent & Highlights","Borders & Separators","Tool Colors"})new QTreeWidgetItem(categories_,{group});split->addWidget(categories_);
        auto *controls=new QWidget;auto *middle=new QVBoxLayout(controls);tokens_=new QTreeWidget;tokens_->setObjectName("themeTokens");tokens_->setHeaderLabels({"Element","Color"});tokens_->setRootIsDecorated(false);middle->addWidget(tokens_,1);inherit_=new QCheckBox("Use theme default");inherit_->setObjectName("themeUseDefault");middle->addWidget(inherit_);color_=new QPushButton("Choose Color...");color_->setObjectName("themeColor");middle->addWidget(color_);
        fieldBackground_=new QCheckBox("Background behind input fields");fieldBackground_->setObjectName("themeFieldBackground");borders_=new QCheckBox("Show control borders");borders_->setObjectName("themeBorders");middle->addWidget(fieldBackground_);middle->addWidget(borders_);auto *reset=new QPushButton("Reset Group");reset_=reset;middle->addWidget(reset);help_=new QLabel;help_->setWordWrap(true);middle->addWidget(help_);split->addWidget(controls);preview_=new ThemePreview;split->addWidget(preview_);split->setSizes({160,310,270});
        connect(selector_,&QComboBox::currentIndexChanged,this,[this]{showTheme();});connect(categories_,&QTreeWidget::currentItemChanged,this,[this]{showTokens();});connect(tokens_,&QTreeWidget::currentItemChanged,this,[this]{showColor();});
        connect(base_,&QComboBox::currentIndexChanged,this,[this]{if(loading_||!custom())return;auto theme=current();theme["base"]=base_->currentData().toString();library_[id()]=theme;showTheme();});
        connect(inherit_,&QCheckBox::toggled,this,[this](bool checked){if(loading_||!tokens_->currentItem()||!custom())return;setOverride(token(),checked?QColor():resolvedThemeColor(token(),workbenchThemePalette(current()["base"].toString(),current()["colors"].toObject()),current()["colors"].toObject()));});
        connect(color_,&QPushButton::clicked,this,[this]{auto theme=current();auto colors=theme["colors"].toObject();auto color=QColorDialog::getColor(resolvedThemeColor(token(),workbenchThemePalette(theme["base"].toString(),colors),colors),this,"Choose Theme Color");if(color.isValid())setOverride(token(),color);});
        auto options=[this]{if(loading_||!custom())return;auto theme=current();theme["options"]=QJsonObject{{"fieldBackground",fieldBackground_->isChecked()},{"borders",borders_->isChecked()}};library_[id()]=theme;preview_->theme=theme;preview_->update();};connect(fieldBackground_,&QCheckBox::toggled,this,options);connect(borders_,&QCheckBox::toggled,this,options);
        connect(reset,&QPushButton::clicked,this,[this]{if(!custom())return;auto theme=current();auto colors=theme["colors"].toObject();for(auto &t:themeTokens())if(t.group==group())colors.remove(t.key);theme["colors"]=colors;if(group()=="Backgrounds"){auto options=theme["options"].toObject();options.remove("fieldBackground");theme["options"]=options;}if(group()=="Borders & Separators"){auto options=theme["options"].toObject();options.remove("borders");theme["options"]=options;}library_[id()]=theme;showTheme();});
        categories_->setCurrentItem(categories_->topLevelItem(0));reload(settings.value("theme","dark").toString());
    }
    QString createTheme(QString name,bool duplicate=true){if(name.trimmed().isEmpty()||name.trimmed().size()>64)return {};auto theme=current();if(!duplicate){theme["colors"]=QJsonObject();theme["options"]=QJsonObject();}theme["name"]=name.trimmed();QString key="custom:"+QUuid::createUuid().toString(QUuid::WithoutBraces);library_[key]=theme;reload(key);return key;}
    bool setOverride(const QString &key,QColor color){if(!custom())return false;bool known=false;for(auto &t:themeTokens())known|=t.key==key;if(!known)return false;auto theme=current();auto colors=theme["colors"].toObject();if(color.isValid())colors[key]=color.name();else colors.remove(key);theme["colors"]=colors;library_[id()]=theme;showTokens();preview_->theme=theme;preview_->update();return true;}
    bool importTheme(const QString &path,QString &error){QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>65536){error="Cannot read theme (maximum 64 KiB).";return false;}auto doc=QJsonDocument::fromJson(file.readAll());if(!doc.isObject()||!validateTheme(doc.object(),error)){if(error.isEmpty())error="Invalid theme JSON.";return false;}QString key="custom:"+QUuid::createUuid().toString(QUuid::WithoutBraces);library_[key]=doc.object();reload(key);return true;}
    bool exportTheme(const QString &path,QString &error)const{QSaveFile file(path);auto bytes=QJsonDocument(current()).toJson();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){error="Could not save theme.";return false;}return true;}
    void save(QSettings &settings){settings.setValue("themes/v1",QJsonDocument(library_).toJson(QJsonDocument::Compact));}
    void restoreDefaults(){selector_->setCurrentIndex(selector_->findData("dark"));}
    QJsonObject current()const{auto theme=library_[id()].toObject();if(theme.isEmpty())theme=QJsonObject{{"format","binary-workbench-theme"},{"version",1},{"name",selector_->currentText()},{"base",id()},{"colors",QJsonObject()},{"options",QJsonObject()}};return theme;}
private:
    QString id()const{return selector_->currentData().toString();}bool custom()const{return library_.contains(id());}QString group()const{return categories_->currentItem()?categories_->currentItem()->text(0):"Backgrounds";}QString token()const{return tokens_->currentItem()?tokens_->currentItem()->data(0,Qt::UserRole).toString():QString();}
    void reload(const QString &selected){QSignalBlocker block(selector_);selector_->clear();selector_->addItem("Dark","dark");selector_->addItem("Light","light");selector_->addItem("System","system");for(auto it=library_.begin();it!=library_.end();++it)selector_->addItem(it.value().toObject()["name"].toString()+" · Custom",it.key());selector_->setCurrentIndex(qMax(0,selector_->findData(selected)));showTheme();}
    void showTheme(){loading_=true;auto theme=current();base_->setCurrentIndex(qMax(0,base_->findData(theme["base"].toString())));base_->setEnabled(custom());rename_->setEnabled(custom());reset_->setEnabled(custom());remove_->setEnabled(custom());fieldBackground_->setChecked(theme["options"].toObject().value("fieldBackground").toBool(true));borders_->setChecked(theme["options"].toObject().value("borders").toBool(true));loading_=false;showTokens();preview_->theme=theme;preview_->update();}
    void showTokens(){QString previous=token();QSignalBlocker block(tokens_);tokens_->clear();auto theme=current();auto colors=theme["colors"].toObject();auto p=workbenchThemePalette(theme["base"].toString(),colors);QTreeWidgetItem *selected=nullptr;for(auto &t:themeTokens())if(t.group==group()){auto color=resolvedThemeColor(t.key,p,colors);auto *item=new QTreeWidgetItem(tokens_,{t.label,colors.contains(t.key)?color.name():"Default"});item->setData(0,Qt::UserRole,t.key);QPixmap swatch(14,14);swatch.fill(color);{QPainter painter(&swatch);painter.setPen(p.color(QPalette::PlaceholderText));painter.drawRect(0,0,13,13);}item->setIcon(1,QIcon(swatch));item->setToolTip(1,color.name());if(t.key==previous)selected=item;}tokens_->resizeColumnToContents(0);if(!selected&&tokens_->topLevelItemCount())selected=tokens_->topLevelItem(0);tokens_->setCurrentItem(selected);showColor();}
    void showColor(){loading_=true;auto colors=current()["colors"].toObject();inherit_->setChecked(!colors.contains(token()));inherit_->setEnabled(custom()&&!token().isEmpty());color_->setEnabled(custom()&&!token().isEmpty());fieldBackground_->setVisible(group()=="Backgrounds");borders_->setVisible(group()=="Borders & Separators");fieldBackground_->setEnabled(custom());borders_->setEnabled(custom());auto p=workbenchThemePalette(current()["base"].toString(),colors);auto color=resolvedThemeColor(token(),p,colors);color_->setText("Choose Color... "+color.name());help_->setText(custom()?"Changes stay in this preview until Apply or OK. Default colors follow the base palette; explicit formatting rules and graphics overrides stay independent.":"Included theme — duplicate it to customize colors. Import/export uses versioned .bwtheme files.");loading_=false;}
    QComboBox *selector_,*base_;QTreeWidget *categories_,*tokens_;QCheckBox *inherit_,*fieldBackground_,*borders_;QPushButton *color_,*rename_,*remove_,*reset_;QLabel *help_;ThemePreview *preview_;QJsonObject library_;bool loading_=false;
};
