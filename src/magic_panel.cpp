#include "magic_panel.h"
#include "hex_view.h"
#include "format_panel.h"
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QSaveFile>
#include <QTimer>
#include <QSignalBlocker>
#include <algorithm>

MagicPanel::MagicPanel(HexView *view,FormatPanel *format,QWidget *parent):QWidget(parent),view_(view),format_(format){
    setObjectName("magicPanel");auto *layout=new QVBoxLayout(this);
    auto *intro=new QLabel("Firmas compatibles: una coincidencia es una pista; no valida el formato ni determina dónde termina. El color se elige en Reglas de formato.");intro->setWordWrap(true);layout->addWidget(intro);
    auto *body=new QHBoxLayout;layout->addLayout(body);
    auto *left=new QVBoxLayout;body->addLayout(left,1);
    libraryList_=new QTreeWidget;libraryList_->setObjectName("magicLibrary");libraryList_->setHeaderLabels({"Firma activa","Bytes","Desplazamiento"});libraryList_->setRootIsDecorated(false);left->addWidget(libraryList_);
    auto *buttons=new QHBoxLayout;left->addLayout(buttons);
    auto button=[&](QString label,auto callback){auto *b=new QPushButton(label);buttons->addWidget(b);connect(b,&QPushButton::clicked,this,callback);return b;};
    button("+",[this]{edit(true);});button("−",[this]{int row=libraryList_->indexOfTopLevelItem(libraryList_->currentItem());if(row<0)return;invalidate();library_.removeAt(row);dirty_=true;rebuild();});button("Modificar",[this]{edit(false);});
    button("Importar",[this]{
        if(!confirmClose())return;auto path=QFileDialog::getOpenFileName(this,"Importar biblioteca de firmas",{},"Biblioteca JSON (*.json)");if(path.isEmpty())return;
        QFile file(path);QString error;QList<MagicSignature> loaded;
        if(!file.open(QIODevice::ReadOnly)||!decodeMagicLibrary(file.read(1024*1024+1),loaded,error)){QMessageBox::warning(this,"Importación",error.isEmpty()?file.errorString():error);return;}
        invalidate();library_=std::move(loaded);dirty_=false;rebuild();
    });button("Exportar",[this]{exportLibrary();});
    connect(libraryList_,&QTreeWidget::itemDoubleClicked,this,[this](auto *,int){edit(false);});
    connect(libraryList_,&QTreeWidget::itemChanged,this,[this](auto *item,int column){if(column!=0)return;int row=libraryList_->indexOfTopLevelItem(item);if(row<0)return;invalidate();library_[row].enabled=item->checkState(0)==Qt::Checked;dirty_=true;});
    auto *right=new QVBoxLayout;body->addLayout(right,2);
    auto *controls=new QHBoxLayout;right->addLayout(controls);scope_=new QComboBox;scope_->addItems({"Todo el archivo","Inicio del archivo","Inicio de la selección","Inicios de regiones marcadas"});controls->addWidget(scope_);
    start_=new QPushButton("Analizar");start_->setObjectName("magicScan");controls->addWidget(start_);connect(start_,&QPushButton::clicked,this,[this]{scan();});
    auto *cancel=new QPushButton("Cancelar");controls->addWidget(cancel);connect(cancel,&QPushButton::clicked,view_,&HexView::cancelMagicScan);
    results_=new QTableWidget(0,4);results_->setObjectName("magicResults");results_->setHorizontalHeaderLabels({"Firma compatible","Offset ↗","Inicio candidato","Bytes"});results_->setEditTriggers(QAbstractItemView::NoEditTriggers);results_->setSelectionBehavior(QAbstractItemView::SelectRows);results_->setSelectionMode(QAbstractItemView::ExtendedSelection);results_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);results_->horizontalHeader()->setStretchLastSection(true);right->addWidget(results_);
    auto navigate=[this](QTableWidgetItem *item){if(!item||item->row()>=matches_.size())return;auto r=matches_[item->row()];view_->selectMatch(r.offset,r.length);};
    connect(results_,&QTableWidget::itemClicked,this,navigate);connect(results_,&QTableWidget::itemActivated,this,navigate);
    auto *markButton=new QPushButton("Enmarcar filas seleccionadas con el estilo actual");right->addWidget(markButton);connect(markButton,&QPushButton::clicked,this,[this]{mark();});
    status_=new QLabel("Biblioteca editable · PNG, DDS y gzip como punto de partida.");status_->setWordWrap(true);layout->addWidget(status_);
    connect(view_,&HexView::documentChanged,this,[this]{invalidate();});
    connect(view_,&HexView::formatRulesChanged,this,[this]{if(view_->magicBusy()&&anchored_)invalidate();});
    connect(view_,&HexView::magicScanFinished,this,[this](const ReadResult &r){
        if(stale_)return;if(!r.error.isEmpty()){status_->setText(r.error);return;}if(r.cancelled){status_->setText("Análisis cancelado.");return;}
        matches_.clear();for(auto m:r.matches)matches_.append({m.first,m.second});types_=r.matchTypes;results_->setUpdatesEnabled(false);results_->setRowCount(matches_.size());
        for(int i=0;i<matches_.size();++i){const auto &s=scanned_[types_[i]];auto range=matches_[i];
            QStringList cells{s.name,"0x"+QString::number(range.offset,16).toUpper(),"0x"+QString::number(range.offset-s.relativeOffset,16).toUpper(),QString::number(range.length)};
            for(int c=0;c<4;++c){auto *item=new QTableWidgetItem(cells[c]);item->setToolTip(s.notes);if(c==1){auto font=item->font();font.setUnderline(true);item->setFont(font);item->setForeground(QColor(45,125,220));}results_->setItem(i,c,item);}
        }
        results_->setUpdatesEnabled(true);status_->setText(QString("%1 coincidencias%2. Pulsa una fila para ir a sus bytes; selecciona varias para enmarcarlas.").arg(matches_.size()).arg(r.truncated?" (límite de 10 000 alcanzado; análisis parcial)":""));
    });
    auto *timer=new QTimer(this);timer->setInterval(200);connect(timer,&QTimer::timeout,this,[this]{start_->setEnabled(view_->hasFile()&&!view_->saving()&&!view_->magicBusy());if(view_->magicBusy()&&!stale_)status_->setText(anchored_?QString("Analizando %1 / %2 regiones…").arg(view_->magicProgress()).arg(total_):QString("Analizando %1 / %2 MiB…").arg(double(view_->magicProgress())/1048576,0,'f',1).arg(double(total_)/1048576,0,'f',1));});timer->start();rebuild();
}
void MagicPanel::rebuild(){QSignalBlocker block(libraryList_);libraryList_->clear();for(const auto &s:library_){auto *item=new QTreeWidgetItem(libraryList_,{s.name,s.pattern,QString::number(s.relativeOffset)});item->setCheckState(0,s.enabled?Qt::Checked:Qt::Unchecked);item->setToolTip(0,s.notes);}libraryList_->resizeColumnToContents(0);}
void MagicPanel::invalidate(){view_->cancelMagicScan();stale_=true;matches_.clear();types_.clear();results_->setRowCount(0);status_->setText("Resultados pendientes de analizar.");}
void MagicPanel::edit(bool create){
    int row=libraryList_->indexOfTopLevelItem(libraryList_->currentItem());if(!create&&row<0)return;if(create&&library_.size()>=64){QMessageBox::information(this,"Biblioteca","Máximo 64 firmas por biblioteca.");return;}
    MagicSignature s=create?MagicSignature{}:library_[row];QDialog dialog(this);dialog.setWindowTitle(create?"Añadir firma":"Modificar firma");auto *form=new QFormLayout(&dialog);
    auto *name=new QLineEdit(s.name);name->setMaxLength(200);auto *pattern=new QLineEdit(s.pattern);pattern->setPlaceholderText("89 50 4E 47 · comodines: ?? A? ?F");auto *offset=new QLineEdit(QString::number(s.relativeOffset));auto *notes=new QPlainTextEdit(s.notes);notes->setMaximumHeight(90);
    form->addRow("Nombre",name);form->addRow("Bytes hexadecimales",pattern);form->addRow("Desplazamiento desde el inicio",offset);form->addRow("Notas",notes);auto *hint=new QLabel("Desplazamiento en decimal o 0x hexadecimal. En búsqueda completa se infiere el inicio restándolo al offset encontrado.");hint->setWordWrap(true);form->addRow(hint);
    auto *actions=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(actions);connect(actions,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(actions,&QDialogButtonBox::accepted,&dialog,[&]{bool ok=false;QString text=offset->text().trimmed();s.relativeOffset=text.toLongLong(&ok,text.startsWith("0x",Qt::CaseInsensitive)?16:10);s.name=name->text().trimmed();s.pattern=pattern->text();s.notes=notes->toPlainText();QString error;if(!ok||!validateMagic(s,error)){QMessageBox::warning(&dialog,"Firma no válida",ok?error:"Desplazamiento no válido.");return;}dialog.accept();});
    if(dialog.exec()!=QDialog::Accepted)return;invalidate();if(create)library_.append(s);else library_[row]=s;dirty_=true;rebuild();
}
bool MagicPanel::exportLibrary(){auto path=QFileDialog::getSaveFileName(this,"Exportar biblioteca de firmas","firmas.magic.json","Biblioteca JSON (*.json)");if(path.isEmpty())return false;QSaveFile file(path);auto bytes=encodeMagicLibrary(library_).toJson();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){QMessageBox::warning(this,"Exportación",file.errorString());return false;}dirty_=false;return true;}
bool MagicPanel::confirmClose(){if(!dirty_)return true;auto answer=QMessageBox::question(this,"Biblioteca modificada","¿Exportar los cambios de la biblioteca de firmas?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);if(answer==QMessageBox::Save)return exportLibrary();if(answer==QMessageBox::Discard){dirty_=false;return true;}return false;}
void MagicPanel::scan(){
    MagicSpec spec;spec.signatures=library_;spec.anchored=scope_->currentIndex()!=0;QString error;
    if(scope_->currentIndex()==1)spec.regions={{0,view_->fileSize()}};
    else if(scope_->currentIndex()==2){auto s=view_->selection();if(s.anchor<0||s.rectangular)error="Selecciona una región continua para comprobar su inicio.";else spec.regions={{qMin(s.anchor,s.cursor),qAbs(s.cursor-s.anchor)+1}};}
    else if(scope_->currentIndex()==3){spec.regions=view_->markedRegions(error);if(error.isEmpty()&&spec.regions.isEmpty())error="No hay regiones marcadas activas.";}
    if(!error.isEmpty()){status_->setText(error);return;}
    if(view_->magicBusy())return;invalidate();scanned_=library_;anchored_=spec.anchored;total_=anchored_?spec.regions.size():view_->fileSize();stale_=!view_->startMagicScan(std::move(spec));if(stale_)status_->setText("Abre un archivo y espera a que termine el guardado.");
}
void MagicPanel::mark(){
    QVector<ByteRange> selected;for(const auto &index:results_->selectionModel()->selectedRows())if(index.row()<matches_.size())selected.append(matches_[index.row()]);
    if(selected.isEmpty()){status_->setText("Selecciona una o varias filas de resultados.");return;}
    std::sort(selected.begin(),selected.end(),[](auto a,auto b){return a.offset<b.offset;});QVector<ByteRange> merged;
    for(auto r:selected){if(!merged.isEmpty()&&r.offset<=merged.last().offset+merged.last().length)merged.last().length=qMax(merged.last().offset+merged.last().length,r.offset+r.length)-merged.last().offset;else merged.append(r);}
    QString error;if(!format_->markExternalMatches("Magic bytes",merged,error))status_->setText(error);else status_->setText("Firmas enmarcadas en hexadecimal y en la representación interpretada. Guarda las reglas de formato para conservar las marcas.");
}
