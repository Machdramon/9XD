#include "format_panel.h"
#include <QToolBar>
#include <QToolButton>
#include <QColorDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSignalBlocker>
#include <limits>
#include <QGroupBox>
#include <QGridLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QTimer>
#include <QCheckBox>
#include <QRegularExpression>

FormatPanel::FormatPanel(HexView *view,QWidget *parent):QWidget(parent),view_(view){
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(8,8,8,8);layout->setSpacing(6);
    toolbar_=new QToolBar(this);toolbar_->setObjectName("byteFormatToolbar");toolbar_->hide();
    auto *styleBox=new QGroupBox("Formato");auto *grid=new QGridLayout(styleBox);grid->setContentsMargins(6,6,6,6);
    auto check=[this,grid](QString label,QString name,int row,bool checked=false){
        auto *box=new QCheckBox(label);box->setObjectName(name);box->setChecked(checked);grid->addWidget(box,row,0);
        connect(box,&QCheckBox::toggled,this,[this]{if(!loadingStyle_)applySelection();});return box;
    };
    textEnabled_=check("Color:","formatText",0,true);fillEnabled_=check("Background:","formatBackground",1,true);
    borderEnabled_=check("Contorno:","formatBorder",2);underline_=check("Subrayado:","formatUnderline",3);
    QColor *colors[]={&foreground_,&background_,&border_,&underlineColor_};
    for(int i=0;i<4;++i){
        auto *field=new QLineEdit(colors[i]->name().toUpper());colorFields_[i]=field;field->setObjectName(QString("formatColor%1").arg(i));field->setMaxLength(7);field->setMaximumWidth(100);field->setToolTip("Color hexadecimal #RRGGBB");grid->addWidget(field,i,1);
        auto *swatch=new QToolButton;swatch->setObjectName(QString("formatSwatch%1").arg(i));swatch->setText("…");swatch->setToolTip("Elegir color");grid->addWidget(swatch,i,2);
        connect(field,&QLineEdit::textEdited,this,[this,i,value=colors[i]](const QString &text){bool valid=QRegularExpression("^#[0-9A-Fa-f]{6}$").match(text).hasMatch();colorFields_[i]->setStyleSheet(valid?QString():"border: 1px solid #D05050;");if(!valid)return;*value=QColor(text);findChild<QToolButton *>(QString("formatSwatch%1").arg(i))->setStyleSheet("background-color: "+value->name()+";");if(!loadingStyle_)applySelection();});
        connect(field,&QLineEdit::editingFinished,this,[this]{updateColorFields();});
        connect(swatch,&QToolButton::clicked,this,[this,value=colors[i]]{auto color=QColorDialog::getColor(*value,this,"Elegir color");if(!color.isValid())return;*value=color;updateColorFields();applySelection();});
    }
    bold_=check("Negrita / Bold","formatBold",4);italic_=check("Cursiva","formatItalic",5);
    jump_=new QLineEdit("0x0");jump_->setObjectName("formatJumpTo");jump_->setMaxLength(18);jump_->setToolTip("Destino hexadecimal de esta regla. Enter para saltar; no cambia sus rangos.");grid->addWidget(new QLabel("Jump to:"),6,0);grid->addWidget(jump_,6,1,1,2);
    layout->addWidget(styleBox);
    list_=new QTreeWidget;list_->setObjectName("formatRules");list_->setHeaderLabels({"Regla","Región"});list_->setRootIsDecorated(false);list_->setMinimumHeight(80);layout->addWidget(list_,1);
    connect(jump_,&QLineEdit::editingFinished,this,[this]{
        if(loadingStyle_||list_->selectedItems().isEmpty())return;int index=list_->indexOfTopLevelItem(list_->currentItem());if(index<0)return;
        QString text=jump_->text().trimmed();if(text.startsWith("0x",Qt::CaseInsensitive))text.remove(0,2);bool ok;auto offset=text.toLongLong(&ok,16);
        if(!ok||offset<0||offset>=view_->fileSize()){jump_->setStyleSheet("border: 1px solid #D05050;");return;}
        jump_->setStyleSheet({});auto rules=view_->formatRules();if(rules[index].jumpTo!=offset){rules[index].jumpTo=offset;publish(rules);}
    });
    connect(jump_,&QLineEdit::returnPressed,this,[this]{jump_->editingFinished();if(jump_->styleSheet().isEmpty())navigateSelected();});
    connect(list_,&QTreeWidget::itemClicked,this,[this](auto *,int){navigateSelected();});
    connect(list_,&QTreeWidget::itemActivated,this,[this](auto *,int){navigateSelected();});
    updateColorFields();
    auto button=[this](QHBoxLayout *row,QString text,QString tooltip,std::function<void()> fn){auto *b=new QToolButton;b->setText(text);b->setToolTip(tooltip);b->setAccessibleName(tooltip);row->addWidget(b);connect(b,&QToolButton::clicked,this,std::move(fn));};
    auto *controls=new QHBoxLayout;
    button(controls,"+","Añadir regla",[this]{beginRule(false);});button(controls,"−","Eliminar regla",[this]{int i=list_->indexOfTopLevelItem(list_->currentItem());if(i>=0){auto r=view_->formatRules();r.removeAt(i);publish(r);editor_->hide();}});
    button(controls,"Modificar","Modificar regla",[this]{beginRule(true);});
    for(int delta:{-1,1})button(controls,delta<0?"↑":"↓",delta<0?"Subir regla":"Bajar regla",[this,delta]{int i=list_->indexOfTopLevelItem(list_->currentItem());auto r=view_->formatRules();if(i>=0&&i+delta>=0&&i+delta<r.size()){r.swapItemsAt(i,i+delta);publish(r);list_->setCurrentItem(list_->topLevelItem(i+delta));}});
    controls->addStretch();layout->addLayout(controls);
    editor_=new QWidget;auto *editorLayout=new QVBoxLayout(editor_);editorLayout->setContentsMargins(0,0,0,0);auto *form=new QFormLayout;
    name_=new QLineEdit("Secuencia");name_->setMaxLength(200);start_=new QLineEdit("0x0");length_=new QLineEdit("4");step_=new QLineEdit("16");repetitions_=new QLineEdit("1");
    start_->setObjectName("ruleStart");length_->setObjectName("ruleLength");step_->setObjectName("ruleStep");repetitions_->setObjectName("ruleRepetitions");
    for(auto *f:{start_,length_,step_,repetitions_}){f->setMaxLength(20);f->setToolTip("Decimal o hexadecimal (0x…)");}
    form->addRow("Nombre",name_);form->addRow("Inicio",start_);form->addRow("Longitud",length_);form->addRow("Paso",step_);form->addRow("Repeticiones",repetitions_);editorLayout->addLayout(form);
    auto *editorButtons=new QHBoxLayout;
    button(editorButtons,"Selección","Usar selección como región",[this]{auto s=view_->selection();if(s.anchor<0)return;start_->setText(QString::number(qMin(s.anchor,s.cursor)));length_->setText(QString::number(qAbs(s.cursor-s.anchor)+1));step_->setText(length_->text());repetitions_->setText("1");});
    button(editorButtons,"Aceptar","Guardar regla",[this]{addRule(list_->currentItem()!=nullptr);});button(editorButtons,"Cancelar","Cancelar edición de regla",[this]{editor_->hide();});editorLayout->addLayout(editorButtons);layout->addWidget(editor_);editor_->hide();
    connect(list_,&QTreeWidget::itemSelectionChanged,this,[this]{loadSelected();});
    connect(list_,&QTreeWidget::itemDoubleClicked,this,[this]{beginRule(true);});
    connect(list_,&QTreeWidget::itemChanged,this,[this](QTreeWidgetItem *item,int column){if(column)return;auto r=view_->formatRules();int i=list_->indexOfTopLevelItem(item);if(i>=0){r[i].enabled=item->checkState(0)==Qt::Checked;publish(r);}});
    connect(view_,&HexView::formatRulesChanged,this,[this]{refresh();});
    auto *searchBox=new QGroupBox("Buscar patrones");auto *searchLayout=new QVBoxLayout(searchBox);auto *searchForm=new QFormLayout;
    searchMode_=new QComboBox;searchMode_->setObjectName("patternMode");searchMode_->addItems({"Hex + comodines","Regex de bytes","Texto literal","Regex de texto"});
    pattern_=new QLineEdit;pattern_->setObjectName("patternExpression");pattern_->setMaxLength(4096);pattern_->setPlaceholderText("48 8B ?? A?");
    searchScope_=new QComboBox;searchScope_->addItems({"Archivo completo (hex)","Región indicada"});
    searchStart_=new QLineEdit("0");searchLength_=new QLineEdit("4096");searchStart_->setObjectName("patternStart");searchLength_->setObjectName("patternLength");
    searchForm->addRow("Tipo",searchMode_);searchForm->addRow("Patrón",pattern_);searchForm->addRow("Ámbito",searchScope_);searchForm->addRow("Inicio",searchStart_);searchForm->addRow("Bytes",searchLength_);searchLayout->addLayout(searchForm);
    auto *hint=new QLabel("Hex: ?? = byte, A? = dígito variable. Regex de bytes: \\x48. Texto: codificación del visor. Texto/regex: región ≤ 1 MiB; ^ y $ son los límites de esa región.");hint->setWordWrap(true);searchLayout->addWidget(hint);
    auto *searchButtons=new QHBoxLayout;
    button(searchButtons,"Selección","Buscar en la selección",[this]{auto s=view_->selection();if(s.anchor>=0){searchScope_->setCurrentIndex(1);searchStart_->setText(QString::number(qMin(s.anchor,s.cursor)));searchLength_->setText(QString::number(qAbs(s.cursor-s.anchor)+1));}});
    button(searchButtons,"Buscar","Buscar patrón",[this]{startSearch();});button(searchButtons,"Detener","Cancelar búsqueda",[this]{view_->cancelPatternSearch();});searchLayout->addLayout(searchButtons);
    searchStatus_=new QLabel("Sin búsqueda");searchStatus_->setWordWrap(true);searchLayout->addWidget(searchStatus_);
    auto *matchesRow=new QHBoxLayout;matchIndex_=new QSpinBox;matchIndex_->setObjectName("matchIndex");matchIndex_->setRange(0,0);matchesRow->addWidget(matchIndex_);
    button(matchesRow,"←","Coincidencia anterior",[this]{matchIndex_->stepDown();});button(matchesRow,"→","Coincidencia siguiente",[this]{matchIndex_->stepUp();});
    button(matchesRow,"Marcar","Crear regla con las coincidencias",[this]{markMatches();});searchLayout->addLayout(matchesRow);layout->addWidget(searchBox);
    connect(matchIndex_,&QSpinBox::valueChanged,this,[this](int value){if(value>0&&value<=matches_.size())view_->selectMatch(matches_[value-1].first,matches_[value-1].second);});
    connect(searchMode_,&QComboBox::currentIndexChanged,this,[this](int mode){if(mode)searchScope_->setCurrentIndex(1);searchScope_->setEnabled(mode==0);pattern_->setPlaceholderText(mode==0?"48 8B ?? A?":mode==1?"\\x48.{2}\\x41":mode==2?"Texto que buscar":"Nombre[0-9]+ ");});
    connect(view_,&HexView::documentChanged,this,[this]{searchStale_=true;matches_.clear();matchIndex_->setRange(0,0);searchStatus_->setText("El contenido cambió; vuelve a buscar.");});
    connect(view_,&HexView::patternSearchFinished,this,[this](const ReadResult &r){
        if(searchStale_){searchStatus_->setText("Resultados descartados: el documento cambió.");return;}
        if(r.cancelled){searchStatus_->setText("Búsqueda cancelada.");return;}
        if(!r.error.isEmpty()){searchStatus_->setText(r.error);return;}
        matches_=r.matches;searchStatus_->setText(QString("%1 coincidencias%2").arg(matches_.size()).arg(r.truncated?" · límite de 10 000; búsqueda incompleta":""));
        QSignalBlocker block(matchIndex_);matchIndex_->setRange(matches_.isEmpty()?0:1,matches_.size());
        if(!matches_.isEmpty()){matchIndex_->setValue(1);view_->selectMatch(matches_[0].first,matches_[0].second);}
    });
    auto *timer=new QTimer(this);timer->setInterval(200);connect(timer,&QTimer::timeout,this,[this]{if(view_->searchBusy()&&!searchStale_)searchStatus_->setText(QString("Buscando… %1 MiB procesados").arg(double(view_->searchProgress())/(1024*1024),0,'f',2));});timer->start();
    auto *io=new QHBoxLayout;button(io,"Importar","Importar reglas",[this]{importRules();});button(io,"Exportar","Exportar reglas",[this]{exportRules();});io->addStretch();layout->addLayout(io);
    refresh();
}
void FormatPanel::beginRule(bool replace){
    if(replace){if(!list_->currentItem())return;loadSelected();}
    else {list_->setCurrentItem(nullptr);name_->setText("Secuencia");for(auto *f:{start_,length_,step_,repetitions_})f->setEnabled(true);}
    editor_->show();
}
void FormatPanel::startSearch(){
    if(view_->searchBusy()){searchStatus_->setText("Hay una búsqueda en curso. Deténla antes de iniciar otra.");return;}
    PatternSpec spec;int mode=searchMode_->currentIndex();spec.regex=mode==1||mode==3;spec.text=mode>=2;spec.encoding=view_->selectedEncoding();spec.expression=pattern_->text();
    if(mode==0&&searchScope_->currentIndex()==0){spec.begin=0;spec.length=view_->fileSize();}
    else{auto read=[](QString text,bool &ok){text=text.trimmed();return text.startsWith("0x",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,16):text.toLongLong(&ok,10);};bool a,b;spec.begin=read(searchStart_->text(),a);spec.length=read(searchLength_->text(),b);if(!a||!b){searchStatus_->setText("Inicio y longitud deben ser números válidos.");return;}}
    matches_.clear();matchIndex_->setRange(0,0);searchStale_=false;matchedPattern_=spec.expression;
    if(!view_->startPatternSearch(spec))searchStatus_->setText("Abre un archivo y espera a que termine el guardado.");else searchStatus_->setText("Buscando…");
}
void FormatPanel::markMatches(){
    if(matches_.isEmpty()||view_->searchBusy()||searchStale_)return;
    auto rules=view_->formatRules();qsizetype total=matches_.size();for(const auto &r:rules)total+=r.matchRanges.size();
    if(rules.size()>=256||total>10000){searchStatus_->setText("Máximo 256 reglas y 10 000 coincidencias guardadas. Elimina alguna regla antes de añadir otra.");return;}
    FormatRule r;r.name=QString("Patrón: %1").arg(matchedPattern_.left(180));r.style=selectedStyle();
    for(const auto &m:matches_)r.matchRanges.append({m.first,m.second});rules.append(r);publish(rules);
}

ByteStyle FormatPanel::selectedStyle()const{
    return {fillEnabled_->isChecked()?background_:QColor(),textEnabled_->isChecked()?foreground_:QColor(),borderEnabled_->isChecked()?border_:QColor(),bold_->isChecked(),italic_->isChecked(),underline_->isChecked(),underlineColor_};
}
void FormatPanel::publish(QList<FormatRule> rules){dirty_=true;view_->setFormatRules(std::move(rules));}
void FormatPanel::refresh(){
    QSignalBlocker block(list_);const int current=list_->indexOfTopLevelItem(list_->currentItem());list_->clear();
    for(const auto &r:view_->formatRules()){
        const auto region=!r.matchRanges.isEmpty()?QString("%1 coincidencias").arg(r.matchRanges.size()):r.selectionRule?QString("0x%1–0x%2%3").arg(qMin(r.selection.anchor,r.selection.cursor),0,16).arg(qMax(r.selection.anchor,r.selection.cursor),0,16).arg(r.selection.rectangular?" · rectangular":""):
            QString("0x%1 · %2 B × %3 · paso %4").arg(r.start,0,16).arg(r.length).arg(r.repetitions).arg(r.step);
        auto *item=new QTreeWidgetItem(list_,{r.name,region});item->setCheckState(0,r.enabled?Qt::Checked:Qt::Unchecked);item->setToolTip(1,region);
    }
    if(current>=0&&current<list_->topLevelItemCount())list_->setCurrentItem(list_->topLevelItem(current));
    list_->resizeColumnToContents(0);
    jump_->setEnabled(!list_->selectedItems().isEmpty());
}
void FormatPanel::applySelection(bool clear){
    if(loadingStyle_||list_->selectedItems().isEmpty())return;
    int index=list_->indexOfTopLevelItem(list_->currentItem());auto rules=view_->formatRules();
    if(index<0||index>=rules.size())return;
    rules[index].style=clear?ByteStyle():selectedStyle();publish(rules);

}
void FormatPanel::addRule(bool replace){
    const auto index=list_->indexOfTopLevelItem(list_->currentItem());
    if(replace&&index<0)return;
    if(!replace&&view_->formatRules().size()>=256){QMessageBox::warning(this,"Límite de reglas","Se admiten hasta 256 reglas por archivo.");return;}
    if(replace&&(view_->formatRules()[index].selectionRule||!view_->formatRules()[index].matchRanges.isEmpty())){
        auto rules=view_->formatRules();rules[index].style=selectedStyle();rules[index].name=name_->text();publish(rules);editor_->hide();return;
    }
    auto read=[](QLineEdit *field,qint64 &value){bool ok;auto text=field->text().trimmed();value=text.startsWith("0x",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,16):text.toLongLong(&ok,10);return ok;};
    FormatRule r;r.name=name_->text();r.style=selectedStyle();
    if(!read(start_,r.start)||!read(length_,r.length)||!read(step_,r.step)||!read(repetitions_,r.repetitions)||r.start<0||r.length<1||r.step<r.length||r.repetitions<1||view_->fileSize()<r.length||r.start>view_->fileSize()-r.length||(r.repetitions-1)>(view_->fileSize()-r.start-r.length)/r.step){
        QMessageBox::warning(this,"Regla no válida","Usa una longitud positiva, paso ≥ longitud y al menos una repetición. Todas las secuencias deben caber en el archivo.");return;
    }
    auto rules=view_->formatRules();if(replace){r.jumpTo=rules[index].jumpTo;rules[index]=r;}else rules.append(r);publish(rules);editor_->hide();
}
void FormatPanel::loadSelected(){
    const auto index=list_->indexOfTopLevelItem(list_->currentItem());jump_->setEnabled(!list_->selectedItems().isEmpty());if(index<0||list_->selectedItems().isEmpty())return;const auto &r=view_->formatRules()[index];
    loadingStyle_=true;
    name_->setText(r.name);
    for(auto *f:{start_,length_,step_,repetitions_})f->setEnabled(!r.selectionRule&&r.matchRanges.isEmpty());

    if(r.selectionRule){start_->setText(QString::number(qMin(r.selection.anchor,r.selection.cursor)));length_->setText(QString::number(r.selection.count(view_->fileSize())));step_->setText(length_->text());repetitions_->setText("1");}
    if(!r.selectionRule){start_->setText("0x"+QString::number(r.start,16));length_->setText(QString::number(r.length));step_->setText(QString::number(r.step));repetitions_->setText(QString::number(r.repetitions));}
    fillEnabled_->setChecked(r.style.background.isValid());textEnabled_->setChecked(r.style.foreground.isValid());borderEnabled_->setChecked(r.style.border.isValid());
    if(r.style.background.isValid())background_=r.style.background;if(r.style.foreground.isValid())foreground_=r.style.foreground;if(r.style.border.isValid())border_=r.style.border;
    bold_->setChecked(r.style.bold);italic_->setChecked(r.style.italic);underline_->setChecked(r.style.underline);
    underlineColor_=r.style.underlineColor.isValid()?r.style.underlineColor:(r.style.foreground.isValid()?r.style.foreground:QColor("#202020"));
    qint64 target=r.jumpTo>=0?r.jumpTo:!r.matchRanges.isEmpty()?r.matchRanges.first().offset:r.selectionRule?qMin(r.selection.anchor,r.selection.cursor):r.start;
    jump_->setText("0x"+QString::number(target,16).toUpper());jump_->setStyleSheet({});updateColorFields();loadingStyle_=false;
}
bool FormatPanel::exportRules(){
    const auto path=QFileDialog::getSaveFileName(this,"Exportar formato visual",{},"Reglas de formato (*.json)");if(path.isEmpty())return false;
    QJsonArray rules;for(const auto &r:view_->formatRules())rules.append(encodeRule(r));
    QSaveFile file(path);file.setDirectWriteFallback(false);
    const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"rules",rules}}).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){QMessageBox::warning(this,"No se pudo exportar",file.errorString());return false;}
    dirty_=false;return true;
}
void FormatPanel::importRules(){
    if(!view_->hasFile()){QMessageBox::information(this,"Abrir archivo","Abre primero el binario al que se aplicarán las reglas.");return;}
    const auto path=QFileDialog::getOpenFileName(this,"Importar formato visual",{},"Reglas de formato (*.json)");if(path.isEmpty())return;
    QFile file(path);QList<FormatRule> rules;bool valid=file.size()<=1024*1024&&file.open(QIODevice::ReadOnly);
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(valid?file.read(1024*1024+1):QByteArray(),&error);
    valid=valid&&error.error==QJsonParseError::NoError&&doc.isObject()&&doc["version"].toInt()==1&&doc["rules"].isArray()&&doc["rules"].toArray().size()<=256;
    if(valid)for(const auto &item:doc["rules"].toArray()){
        FormatRule r;if(!item.isObject()||!decodeRule(item.toObject(),r)){valid=false;break;}
        if(!r.matchRanges.isEmpty()){if(r.matchRanges.last().offset+r.matchRanges.last().length>view_->fileSize()){valid=false;break;}}
        else if(r.selectionRule){if(qMax(r.selection.anchor,r.selection.cursor)>=view_->fileSize()){valid=false;break;}}
        else if(r.start+r.length+(r.repetitions-1)*r.step>view_->fileSize()){valid=false;break;}
        rules.append(r);
    }
    qsizetype total=0;for(const auto &r:rules)total+=r.matchRanges.size();if(total>10000)valid=false;
    if(!valid){QMessageBox::warning(this,"Reglas no válidas","El archivo no contiene reglas válidas para este binario (máximo 256 reglas y 1 MiB). No se ha cambiado el formato.");return;}
    if(!confirmLeave())return;
    view_->setFormatRules(rules);dirty_=false;
}
bool FormatPanel::confirmLeave(){
    if(!dirty_)return true;
    const auto choice=QMessageBox::question(this,"Formato sin exportar","Las reglas visuales se guardan aparte del binario. ¿Quieres exportarlas antes de continuar?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if(choice==QMessageBox::Cancel)return false;
    if(choice==QMessageBox::Save)return exportRules();
    dirty_=false;return true;
}

bool FormatPanel::markExternalMatches(const QString &name,const QVector<ByteRange> &matches,QString &error){
    auto rules=view_->formatRules();qsizetype total=matches.size();for(const auto &r:rules)total+=r.matchRanges.size();
    if(matches.isEmpty()||rules.size()>=256||total>10000){error="Límite: 256 reglas y 10 000 posiciones marcadas. Elimina reglas o marca menos coincidencias.";return false;}
    FormatRule r;r.name=name.left(200);r.style=selectedStyle();r.matchRanges=matches;rules.append(r);publish(rules);return true;
}

void FormatPanel::updateColorFields(){
    const QColor colors[]={foreground_,background_,border_,underlineColor_};
    for(int i=0;i<4;++i){colorFields_[i]->setText(colors[i].name().toUpper());colorFields_[i]->setStyleSheet({});findChild<QToolButton *>(QString("formatSwatch%1").arg(i))->setStyleSheet("background-color: "+colors[i].name()+";");}
}
void FormatPanel::navigateSelected(){
    if(list_->selectedItems().isEmpty())return;int index=list_->indexOfTopLevelItem(list_->currentItem());if(index<0)return;
    const auto &r=view_->formatRules()[index];qint64 target=r.jumpTo>=0?r.jumpTo:!r.matchRanges.isEmpty()?r.matchRanges.first().offset:r.selectionRule?qMin(r.selection.anchor,r.selection.cursor):r.start;
    if(target>=0&&target<view_->fileSize())view_->jumpTo(target);
}
