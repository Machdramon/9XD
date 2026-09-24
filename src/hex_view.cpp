#include "hex_view.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QWheelEvent>
#include <QTimer>
#include <climits>
#include <algorithm>
#include <QSpinBox>
#include <QComboBox>
#include "data_inspector.h"
#include <QVBoxLayout>
#include <QFontDatabase>
#include <QSignalBlocker>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QInputMethodEvent>

HexView::HexView(QWidget *parent) : QWidget(parent), directSpec_(std::make_shared<DirectEditSpec>()), cache_(std::make_shared<ChunkCache>()),
    saveState_(std::make_shared<SaveState>()),
    searchState_(std::make_shared<PatternState>()),
    magicState_(std::make_shared<MagicState>()),
    entropyState_(std::make_shared<EntropyState>()),
    reader_(this,[cache=cache_](const ReadRequest &r){return cache->read(r);}),
    inspectorReader_(this,[cache=cache_](const ReadRequest &r){return cache->read(r);}),
    copyReader_(this,[cache=cache_](const ReadRequest &r){return cache->read(r);}),
    writer_(this,[state=saveState_](const ReadRequest &r){
        return savePatchedFile(r,state->destination,state->patches,&state->progress);
    }),
    searchReader_(this,[state=searchState_](const ReadRequest &r){return searchPattern(r,state->spec,state->cancel,state->progress);}),
    magicReader_(this,[state=magicState_](const ReadRequest &r){return scanMagic(r,state->spec,state->cancel,state->progress);}),
    entropyReader_(this,[state=entropyState_](const ReadRequest &r){return analyzeEntropy(r,state->spec,state->cancel,state->progress);}),
    directReader_(this,[state=directSpec_](const ReadRequest &r){return directEdit(r,*state);}) {
    auto *layout=new QVBoxLayout(this);
    auto *format=new QHBoxLayout;
    stride_=new QSpinBox; stride_->setObjectName("stride"); stride_->setRange(1,256); stride_->setValue(16);
    stride_->setKeyboardTracking(false); stride_->setSuffix(" bytes");
    stride_->setToolTip("Bytes por fila. Cambiar el stride reorganiza la vista, no modifica el archivo.");
    encoding_=new QComboBox; encoding_->setObjectName("encoding");
    encoding_->addItems({"ASCII","UTF-8","UTF-16LE","UTF-16BE","CP932"});
    encoding_->setItemText(4,"Japonés · CP932"); encoding_->setItemData(4,"CP932");
    type_=new QComboBox; type_->setObjectName("interpretation");
    type_->addItems({"Texto","UInt8","Int8","UInt16","Int16","UInt32","Int32","UInt64","Int64","Float32","Float64"});
    endian_=new QComboBox; endian_->setObjectName("endian"); endian_->addItems({"Little endian","Big endian"});
    format->addWidget(new QLabel("Stride")); format->addWidget(stride_);
    format->addWidget(new QLabel("Texto")); format->addWidget(encoding_);
    format->addWidget(new QLabel("Interpretar")); format->addWidget(type_); format->addWidget(endian_); format->addStretch();
    layout->addLayout(format);
    auto *bar=new QHBoxLayout;
    address_=new QLineEdit; address_->setObjectName("address");
    address_->setPlaceholderText("Posición decimal o hexadecimal: 0x1000"); go_=new QPushButton("Ir");
    bar->addWidget(new QLabel("Ir a posición")); bar->addWidget(address_,1); bar->addWidget(go_); layout->addLayout(bar);
    table_=new QTableWidget; table_->setObjectName("bytes");
    auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont); font.setPointSize(12); table_->setFont(font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setItemDelegate(new ByteFormatDelegate(table_,[this](qint64 offset,qint64 length){return formatAt(offset,length);}));
    table_->verticalHeader()->hide(); table_->setShowGrid(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_->viewport()->installEventFilter(this); table_->installEventFilter(this);
    table_->setMouseTracking(true);table_->setAttribute(Qt::WA_InputMethodEnabled,true);
    table_->verticalHeader()->setDefaultSectionSize(26);
    table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *dataLayout=new QHBoxLayout; dataLayout->setSpacing(0);
    scrollbar_=new QScrollBar(Qt::Vertical);scrollbar_->setObjectName("fileScroll");
    scrollbar_->setRange(0,0);scrollbar_->setTracking(true);scrollbar_->installEventFilter(this);
    scrollbar_->setToolTip("Desplazamiento por todo el archivo; rueda y flechas avanzan por filas.");
    dataLayout->addWidget(table_,1);dataLayout->addWidget(scrollbar_);layout->addLayout(dataLayout,1);
    statusLayout_=new QHBoxLayout;
    status_=new QLabel("Abre un archivo local · Lectura y escritura");
    status_->setObjectName("fileStatus"); status_->setWordWrap(true);
    statusLayout_->addWidget(status_,1); layout->addLayout(statusLayout_);
    inspector_=new DataInspector(this);
    connect(inspector_, &DataInspector::endianChanged, this, [this](bool big){endian_->setCurrentIndex(big?1:0);});
    inspector_->setPlainText("Selecciona un byte para interpretarlo.\n\nArrastrar: selección secuencial.\nAlt + arrastrar: rectangular.\nShift + clic: extender tras desplazarte.");
    inspector_->hide();
    setNavigationEnabled(false);
    connect(&reader_,&BinaryReader::ready,this,&HexView::showResult);
    for (auto *reader : {&reader_,&inspectorReader_,&copyReader_,&writer_,&searchReader_,&magicReader_,&entropyReader_,&directReader_})
        connect(reader,&BinaryReader::idle,this,[this]{ if (!busy()) emit idle(); });
    connect(stride_,&QSpinBox::valueChanged,this,[this]{resetDirectEdit();});
    for(auto *box:{encoding_,type_,endian_})connect(box,&QComboBox::currentIndexChanged,this,[this]{resetDirectEdit();});
    connect(&directReader_,&BinaryReader::idle,this,[this]{startDirectEdit();});
    connect(&directReader_,&BinaryReader::ready,this,[this](const ReadResult &r){
        if(!r.error.isEmpty()){directKeys_.clear();status_->setText(r.error);return;}
        PatchList patches;qsizetype at=0;for(auto m:r.matches){patches.append({m.first,r.bytes.mid(at,m.second)});at+=m.second;}
        QString error;if(!edits_.push(patches,error)){directKeys_.clear();status_->setText(error);return;}
        directDrafts_=r.directDrafts;directDigit_++;
        if(directDigit_>=r.directCycle){directDigit_=0;directDrafts_.clear();if(!directBroadcast_){qint64 next=r.matches.first().first+r.directWidth;if(next<size_)selection_={next,next,stride_->value(),false};else directKeys_.clear();}}
        edited();if(selection_.anchor<loadedOffset_||selection_.anchor>=loadedOffset_+bytes_.size())navigate(selection_.anchor);
        status_->setText(QString("Cambios sin guardar · %1 · posición de escritura %2").arg(directBroadcast_?"Shift: toda la selección":"Edición directa").arg(directDigit_+1));
    });
    connect(&entropyReader_,&BinaryReader::ready,this,&HexView::entropyFinished);
    connect(&magicReader_,&BinaryReader::ready,this,&HexView::magicScanFinished);
    connect(&searchReader_,&BinaryReader::ready,this,&HexView::patternSearchFinished);
    connect(&writer_,&BinaryReader::ready,this,[this](const ReadResult &r){
        if(!r.error.isEmpty()) {
            status_->setText("Error de guardado · " + r.error);
            emit editStateChanged(); emit saveFinished(false); return;
        }
        reader_.invalidate(); inspectorReader_.invalidate(); copyReader_.invalidate();
        path_=saveState_->destination; size_=r.size; modified_=r.modified;
        edits_.clear(); saveState_->patches.clear(); cache_->reset(path_);
        emit fileOpened(path_); emit editStateChanged();
        requestViewport(); updateInspector();
        status_->setText("Archivo guardado · Lectura y escritura");
        emit saveFinished(true);
    });
    auto go=[this]{
        const auto text=address_->text().trimmed(); bool ok=false;
        const auto offset=text.startsWith("0x",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,16):text.toLongLong(&ok,10);
        if(!ok||offset<0||offset>=size_){status_->setText("Posición no válida para este archivo.");return;}
        selection_={offset,offset,stride_->value(),false}; navigate(offset); updateInspector();
    };
    connect(go_,&QPushButton::clicked,this,go); connect(address_,&QLineEdit::returnPressed,this,go);
    connect(scrollbar_,&QScrollBar::valueChanged,this,[this](int value){
        if(size_<=0)return;
        const auto maxRow=maxTopRow();
        const qint64 row=maxRow<=INT_MAX ? value :
            (maxRow/INT_MAX)*value+((maxRow%INT_MAX)*value)/INT_MAX;
        navigate(row*stride_->value());
    });
    connect(scrollbar_,&QScrollBar::actionTriggered,this,[this](int action){
        switch(action){
        case QAbstractSlider::SliderSingleStepAdd:scrollRows(1);break;
        case QAbstractSlider::SliderSingleStepSub:scrollRows(-1);break;
        case QAbstractSlider::SliderPageStepAdd:scrollRows(visibleRows());break;
        case QAbstractSlider::SliderPageStepSub:scrollRows(-visibleRows());break;
        case QAbstractSlider::SliderToMinimum:navigate(0);break;
        case QAbstractSlider::SliderToMaximum:navigate(maxTopRow()*stride_->value());break;
        default:break;
        }
    });
    connect(stride_,&QSpinBox::valueChanged,this,[this]{
        if(size_<=0)return;
        const auto anchor=selection_.anchor>=0?selection_.anchor:desiredOffset_;
        selection_={anchor,anchor,stride_->value(),false};
        navigate(anchor); updateInspector();
    });
    auto formatChanged=[this]{ rebuild(); updateInspector(); };
    connect(encoding_,&QComboBox::currentIndexChanged,this,formatChanged);
    connect(type_,&QComboBox::currentIndexChanged,this,formatChanged);
    connect(endian_,&QComboBox::currentIndexChanged,this,formatChanged);
    connect(&inspectorReader_,&BinaryReader::ready,this,[this](const ReadResult &r){
        if(r.request.path!=path_||r.request.offset!=selection_.anchor)return;
        if(!r.error.isEmpty()){inspector_->setPlainText(r.error);return;}
        const auto currentBytes=interpretedBytes(r);
        const auto encoding=encoding_->currentIndex()==4?QString("CP932"):encoding_->currentText();
        inspector_->setData(currentBytes, selection_, size_, encoding, endian_->currentIndex()==1);
    });
    connect(&copyReader_,&BinaryReader::ready,this,[this](const ReadResult &r){
        if(!r.error.isEmpty()){status_->setText(r.error);return;}
        const auto currentBytes=interpretedBytes(r);
        QByteArray raw; QStringList lines;
        for(const auto &range:copying_.ranges(r.size,1024*1024)){
            auto part=currentBytes.mid(range.offset-r.request.offset,range.length);
            if(part.size()!=range.length){status_->setText("No se pudo copiar la selección completa.");return;}
            raw+=part; lines<<QString::fromLatin1(part.toHex(' ').toUpper());
        }
        auto *mime=new QMimeData; mime->setText(lines.join(copying_.rectangular?'\n':' '));
        mime->setData("application/octet-stream",raw); QApplication::clipboard()->setMimeData(mime);
        status_->setText(QString("Copiados %1 bytes como hexadecimal. Rectángulos: una línea por fila.").arg(raw.size()));
    });
}
int HexView::visibleRows() const {
    return qBound(1,table_->viewport()->height()/table_->verticalHeader()->defaultSectionSize(),512);
}
qint64 HexView::pageCapacity()const{return qint64(visibleRows())*stride_->value();}
qint64 HexView::maxTopRow()const{
    if(size_<=0)return 0;
    return qMax(qint64(0),(size_-1)/stride_->value()+1-visibleRows());
}
int HexView::scrollValue(qint64 row)const{
    const auto maximum=maxTopRow();
    if(maximum<=INT_MAX)return int(row);
    return int((static_cast<long double>(row)/maximum)*INT_MAX);
}
void HexView::configureScrollBar(){
    QSignalBlocker block(scrollbar_);
    const auto maximum=maxTopRow();
    scrollbar_->setRange(0,int(qMin(maximum,qint64(INT_MAX))));
    scrollbar_->setPageStep(maximum<=INT_MAX?visibleRows():qMax(1,int(static_cast<long double>(visibleRows())/maximum*INT_MAX)));
    scrollbar_->setSingleStep(1);
    scrollbar_->setValue(scrollValue(desiredOffset_/stride_->value()));
    scrollbar_->setVisible(maximum>0);
}
void HexView::scrollRows(qint64 rows){
    if(size_<=0)return;
    auto row=desiredOffset_/stride_->value();
    if(rows>0)row+=qMin(rows,maxTopRow()-row);else row=qMax(qint64(0),row+rows);
    navigate(row*stride_->value());
}
void HexView::requestViewport(){
    viewportRequestLength_=pageCapacity();
    reader_.request({path_,desiredOffset_,size_,modified_,viewportRequestLength_});
}
bool HexView::busy()const{return reader_.busy()||inspectorReader_.busy()||copyReader_.busy()||writer_.busy()||searchReader_.busy()||magicReader_.busy()||entropyReader_.busy()||directReader_.busy();}
void HexView::shutdown(){reader_.shutdown();inspectorReader_.shutdown();copyReader_.shutdown();writer_.shutdown();cancelPatternSearch();searchReader_.shutdown();cancelMagicScan();magicReader_.shutdown();cancelEntropy();entropyReader_.shutdown();resetDirectEdit();directReader_.shutdown();}
QWidget *HexView::inspector()const{return inspector_;}
void HexView::focusInterpretation(){type_->setFocus();type_->showPopup();}
void HexView::setNavigationEnabled(bool enabled){address_->setEnabled(enabled);go_->setEnabled(enabled);scrollbar_->setEnabled(enabled);}
void HexView::openFile(const QString &path){
    if(saving())return;
    resetDirectEdit();cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    edits_.clear();
    inspectorReader_.invalidate(); copyReader_.invalidate(); dragging_=false;
    formatRules_.clear();emit formatRulesChanged();
    cache_->reset(path);
    path_=path;size_=-1;desiredOffset_=0;modified_={};bytes_.clear();selection_={};emit editStateChanged();
    setNavigationEnabled(false);table_->setRowCount(0);emit selectionAvailable(false);
    inspector_->setPlainText("Abriendo archivo…");status_->setText("Abriendo archivo local…");configureScrollBar();viewportRequestLength_=pageCapacity();reader_.request({path,0,-1,{},viewportRequestLength_});
}
void HexView::navigate(qint64 offset){
    if(size_<=0)return; offset=qBound(qint64(0),offset,size_-1);
    desiredOffset_=qMin(offset/stride_->value(),maxTopRow())*stride_->value();bytes_.clear();table_->setRowCount(0);
    configureScrollBar();
    status_->setText("Leyendo bloques…");requestViewport();
}
void HexView::showResult(const ReadResult &r){
    if(!r.error.isEmpty()){inspectorReader_.invalidate();bytes_.clear();table_->setRowCount(0);setNavigationEnabled(false);selection_={};emit selectionAvailable(false);status_->setText(r.error);inspector_->setPlainText(r.error);return;}
    const bool first=size_<0;
    size_=r.size;modified_=r.modified;desiredOffset_=r.request.offset;loadedOffset_=desiredOffset_;bytes_=r.bytes.left(pageCapacity());
    if(first&&size_>0)selection_={0,0,stride_->value(),false};
    if(first)emit fileOpened(path_);rebuild();setNavigationEnabled(size_>0);
    configureScrollBar();
    address_->setText("0x"+QString::number(desiredOffset_,16).toUpper());
    status_->setText(size_==0?"Archivo vacío · 0 bytes":QString("%1 bytes · Vista desde 0x%2 · Caché: %3 / 20 bloques (%4 MiB) · %5").arg(size_).arg(desiredOffset_,0,16).arg(cache_->chunkCount()).arg(double(cache_->retainedBytes())/(1024*1024),0,'f',3).arg(dirty()?"Cambios sin guardar":"Lectura y escritura"));
    emit editStateChanged();
    if(size_==0)inspector_->setPlainText("Archivo vacío.");else if(first)updateInspector();
}
void HexView::rebuild(){
    const int stride=stride_->value();QSignalBlocker block(table_);table_->setUpdatesEnabled(false);
    table_->clear();table_->setColumnCount(stride+2);table_->setRowCount(int((bytes_.size()+stride-1)/stride));
    QStringList labels{"Posición"};for(int i=0;i<stride;++i)labels<<QString::number(i,16).toUpper();labels<<"Interpretación";
    table_->setHorizontalHeaderLabels(labels);table_->setColumnWidth(0,155);
    for(int col=1;col<=stride;++col)table_->setColumnWidth(col,32);
    table_->setColumnWidth(stride+1,300);
    const auto encoding=encoding_->currentIndex()==4?QString("CP932"):encoding_->currentText();
    auto displayed=bytes_; applyPatches(displayed,loadedOffset_,edits_.patches());
    for(int row=0;row<table_->rowCount();++row){
        const auto part=displayed.mid(row*stride,stride);
        table_->setItem(row,0,new QTableWidgetItem(QString("%1").arg(loadedOffset_+row*stride,16,16,QChar('0')).toUpper()));
        auto tokenData=[](const QString &text,qint64 offset,qint64 length){return QVariantMap{{"text",text},{"offset",offset},{"length",length}};};
        for(int col=0;col<part.size();++col){
            auto *item=new QTableWidgetItem(QString("%1").arg(quint8(part[col]),2,16,QChar('0')).toUpper());
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(ByteFormatDelegate::TokensRole,QVariantList{tokenData(item->text(),loadedOffset_+row*stride+col,1)});
            item->setData(ByteFormatDelegate::HexRole,true);item->setData(ByteFormatDelegate::RowFirstRole,col==0);item->setData(ByteFormatDelegate::RowLastRole,col==part.size()-1);
            table_->setItem(row,col+1,item);
        }
        QVariantList tokens;QString interpreted;
        for(auto token:decodeTokens(part,loadedOffset_+row*stride,encoding,type_->currentText(),endian_->currentIndex()==1)){
            if(type_->currentText()!="Texto"&&directDrafts_.contains(token.offset))token.text=directDrafts_.value(token.offset);
            tokens.append(tokenData(token.text,token.offset,token.length));
            if(!interpreted.isEmpty()&&type_->currentText()!="Texto")interpreted+=" · ";interpreted+=token.text;
        }
        auto *item=new QTableWidgetItem(interpreted);
        item->setData(ByteFormatDelegate::TokensRole,tokens);
        item->setToolTip(interpreted+"\nEl formato de un carácter/valor abarca todos sus bytes. Si coinciden varias reglas, prevalece la última.");table_->setItem(row,stride+1,item);
        qreal decodedWidth=12;
        for(const auto &entry:tokens){auto token=entry.toMap();auto style=formatAt(token["offset"].toLongLong(),token["length"].toLongLong()).style;auto font=table_->font();font.setBold(style.bold);font.setItalic(style.italic);decodedWidth+=qMax(qreal(8),QFontMetricsF(font).horizontalAdvance(token["text"].toString())+4);}
        table_->setColumnWidth(stride+1,qMax(table_->columnWidth(stride+1),int(decodedWidth)));
    }
    table_->setUpdatesEnabled(true);updateSelection();
}
void HexView::updateSelection(){
    const int stride=stride_->value();
    for(int row=0;row<table_->rowCount();++row)for(int col=1;col<=stride;++col){
        auto *item=table_->item(row,col);if(!item)continue;const qint64 offset=loadedOffset_+row*stride+col-1;
        const bool selected=selection_.contains(offset);
        item->setBackground(selected?palette().highlight():QBrush());item->setForeground(selected?palette().highlightedText():palette().text());
        auto font=table_->font();font.setBold(offset==selection_.anchor);item->setFont(font);
        item->setToolTip(QString("0x%1%2").arg(offset,0,16).arg(offset==selection_.anchor?" · cursor inicial":""));
    }
    table_->viewport()->update();
    emit selectionAvailable(selection_.count(size_)>0);
}
void HexView::updateInspector(){
    if(selection_.anchor<0||size_<=0)return;
    inspectorReader_.request({path_,selection_.anchor,size_,modified_});
}
void HexView::selectByte(qint64 offset,bool extend,bool rectangle){
    resetDirectEdit();
    if(offset<0||offset>=size_)return;
    if(!extend||selection_.anchor<0)selection_={offset,offset,stride_->value(),rectangle};
    else{selection_.cursor=offset;selection_.rectangular=rectangle;}
    if(interpretedFocus_&&type_->currentText()!="Texto"&&!selection_.rectangular&&offset>=selection_.anchor)selection_.cursor=qMin(size_-1,offset+numberWidth(type_->currentText())-1);
    updateSelection();updateInspector();
}
qint64 HexView::byteAt(const QPoint &point)const{
    auto index=table_->indexAt(point);if(!index.isValid())return -1;
    if(index.column()==stride_->value()+1){
        auto *item=table_->item(index.row(),index.column());if(!item)return -1;qreal x=table_->visualRect(index).left()+4;
        for(const auto &v:item->data(ByteFormatDelegate::TokensRole).toList()){auto t=v.toMap();auto style=formatAt(t["offset"].toLongLong(),t["length"].toLongLong()).style;auto font=table_->font();font.setBold(style.bold);font.setItalic(style.italic);auto width=qMax(qreal(8),QFontMetricsF(font).horizontalAdvance(t["text"].toString())+4);if(point.x()>=x&&point.x()<x+width)return t["offset"].toLongLong();x+=width;}return -1;
    }
    if(index.column()<1||index.column()>stride_->value())return -1;
    const auto offset=loadedOffset_+index.row()*stride_->value()+index.column()-1;
    return offset<loadedOffset_+bytes_.size()?offset:-1;
}
bool HexView::eventFilter(QObject *watched,QEvent *event){
    if(watched==table_&&event->type()==QEvent::InputMethod&&interpretedFocus_&&type_->currentText()=="Texto"){
        auto *input=static_cast<QInputMethodEvent *>(event);for(char32_t c:input->commitString().toUcs4())queueDirectEdit(QString::fromUcs4(&c,1),QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier));input->accept();return true;
    }

    if(watched==table_->viewport()&&event->type()==QEvent::Resize&&!resizePending_){
        resizePending_=true;
        QTimer::singleShot(0,this,[this]{resizePending_=false;if(size_>0&&pageCapacity()!=viewportRequestLength_)navigate(desiredOffset_);});
    }
    if((watched==table_->viewport()||watched==scrollbar_)&&event->type()==QEvent::Wheel){
        auto *wheel=static_cast<QWheelEvent *>(event);
        if(wheel->modifiers().testFlag(Qt::ShiftModifier)||qAbs(wheel->angleDelta().x())>qAbs(wheel->angleDelta().y()))return false;
        if(!wheel->pixelDelta().isNull()){
            pixelRemainder_+=wheel->pixelDelta().y();
            const auto rows=pixelRemainder_/26;pixelRemainder_%=26;if(rows)scrollRows(-rows);
        }else{
            wheelRemainder_+=wheel->angleDelta().y();
            const auto steps=wheelRemainder_/120;wheelRemainder_%=120;if(steps)scrollRows(-steps*3);
        }
        wheel->accept();return true;
    }
    if(watched==table_->viewport()&&event->type()==QEvent::MouseButtonDblClick){
        auto *mouse=static_cast<QMouseEvent *>(event);
        const auto offset=byteAt(mouse->position().toPoint());
        if(mouse->button()==Qt::LeftButton&&offset>=0){
            dragging_=false; selectByte(offset,false,false); editSelection(); return true;
        }
    }
    if(watched==table_->viewport()&&(event->type()==QEvent::MouseButtonPress||event->type()==QEvent::MouseMove||event->type()==QEvent::MouseButtonRelease)){
        auto *mouse=static_cast<QMouseEvent *>(event);const auto offset=byteAt(mouse->position().toPoint());
        if(event->type()==QEvent::MouseButtonPress&&mouse->button()==Qt::LeftButton){
            interpretedFocus_=table_->indexAt(mouse->position().toPoint()).column()==stride_->value()+1;dragging_=offset>=0;table_->setFocus();if(dragging_)selectByte(offset,mouse->modifiers().testFlag(Qt::ShiftModifier),mouse->modifiers().testFlag(Qt::AltModifier));return dragging_;
        }
        if(event->type()==QEvent::MouseMove&&dragging_){if(offset>=0&&offset!=selection_.cursor)selectByte(offset,true,selection_.rectangular||mouse->modifiers().testFlag(Qt::AltModifier));return true;}
        if(event->type()==QEvent::MouseButtonRelease&&dragging_){if(offset>=0)selectByte(offset,true,selection_.rectangular);dragging_=false;return true;}
    }
    if(watched==table_&&event->type()==QEvent::KeyPress&&size_>0){
        auto *key=static_cast<QKeyEvent *>(event);
        if(key->matches(QKeySequence::Paste)){pasteSelection();return true;}
        if(key->matches(QKeySequence::Undo)){undo();return true;}
        if(key->matches(QKeySequence::Redo)){redo();return true;}
        if(key->key()==Qt::Key_F2){editSelection();return true;}
        if(key->matches(QKeySequence::Copy)){copySelection();return true;}
        if(key->matches(QKeySequence::SelectAll)){selectPage();return true;}
        if(key->key()==Qt::Key_PageDown){scrollRows(visibleRows());return true;}
        if(key->key()==Qt::Key_PageUp){scrollRows(-visibleRows());return true;}
        if(key->key()==Qt::Key_Home){navigate(0);return true;}
        if(key->key()==Qt::Key_End){navigate(maxTopRow()*stride_->value());return true;}
        if(key->key()==Qt::Key_Escape){resetDirectEdit();return true;}
        if(!(key->modifiers()&(Qt::ControlModifier|Qt::MetaModifier|Qt::AltModifier))&&!key->text().isEmpty()){
            QString text=key->text();bool broadcast=key->modifiers().testFlag(Qt::ShiftModifier);
            if((!interpretedFocus_||type_->currentText()!="Texto")&&key->key()>=Qt::Key_0&&key->key()<=Qt::Key_9)text=QString::number(key->key()-Qt::Key_0);
#ifdef Q_OS_MACOS
            // Cocoa can report the shifted symbol as both key() and text().
            // For numeric editing, recover the digit from the native number-row key.
            if(broadcast&&(!interpretedFocus_||type_->currentText()!="Texto")){
                static constexpr quint32 numberKeys[]={29,18,19,20,21,23,22,26,28,25};
                for(int digit=0;digit<10;++digit)if(key->nativeVirtualKey()==numberKeys[digit]){text=QString::number(digit);break;}
            }
#endif
            if(!interpretedFocus_&&key->key()>=Qt::Key_A&&key->key()<=Qt::Key_F)text=QChar('A'+key->key()-Qt::Key_A);
            if(interpretedFocus_&&type_->currentText()!="Texto"){
                if(key->key()==Qt::Key_Minus)text="-";if(key->key()==Qt::Key_Period)text=".";
            }
            queueDirectEdit(text,broadcast);return true;
        }
        qint64 delta=0;
        if(key->key()==Qt::Key_Left)delta=-1;else if(key->key()==Qt::Key_Right)delta=1;
        else if(key->key()==Qt::Key_Up)delta=-stride_->value();else if(key->key()==Qt::Key_Down)delta=stride_->value();
        if(delta){
            auto offset=selection_.cursor>=0?selection_.cursor:loadedOffset_;
            if(delta>0)offset+=qMin(delta,size_-1-offset);else offset=qMax(qint64(0),offset+delta);
            selectByte(offset,key->modifiers().testFlag(Qt::ShiftModifier),key->modifiers().testFlag(Qt::AltModifier));
            if(offset<loadedOffset_||offset>=loadedOffset_+bytes_.size())navigate(offset);
            else table_->scrollToItem(table_->item(int((offset-loadedOffset_)/stride_->value()),int((offset-loadedOffset_)%stride_->value())+1));
            return true;
        }
    }
    return QWidget::eventFilter(watched,event);
}
void HexView::selectPage(){resetDirectEdit();if(bytes_.isEmpty())return;selection_={loadedOffset_,loadedOffset_+bytes_.size()-1,stride_->value(),false};updateSelection();updateInspector();}
void HexView::copySelection(){
    constexpr qint64 limit=1024*1024;
    const auto ranges=selection_.ranges(size_,limit);
    if(ranges.isEmpty()){status_->setText("Selecciona entre 1 byte y 1 MiB para copiar.");return;}
    const auto begin=ranges.first().offset;const auto end=ranges.last().offset+ranges.last().length;
    if(end-begin>limit){status_->setText("La selección abarca más de 1 MiB. Divide la copia en regiones menores.");return;}
    copying_=selection_;status_->setText("Copiando selección…");
    copyReader_.request({path_,begin,size_,modified_,end-begin});
}

void HexView::addStatusWidget(QWidget *widget){statusLayout_->addWidget(widget);}
QByteArray HexView::interpretedBytes(const ReadResult &result) const {
    auto bytes=result.bytes; applyPatches(bytes,result.request.offset,edits_.patches()); return bytes;
}
void HexView::edited(){
    cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    rebuild(); updateInspector(); emit editStateChanged();
    status_->setText(dirty()?"Cambios sin guardar · Lectura y escritura":"Sin cambios pendientes · Lectura y escritura");
}
void HexView::undo(){if(!saving()){resetDirectEdit();edits_.undo();edited();}}
void HexView::redo(){if(!saving()){resetDirectEdit();edits_.redo();edited();}}
bool HexView::replaceSelection(const QByteArray &bytes,QString &error){
    resetDirectEdit();
    constexpr qint64 limit=1024*1024;
    if(saving()){error="Espera a que termine el guardado.";return false;}
    if(bytes.isEmpty()||bytes.size()>limit){error="Introduce entre 1 byte y 1 MiB por operación.";return false;}
    if(selection_.rectangular&&qAbs(selection_.anchor/selection_.stride-selection_.cursor/selection_.stride)>=4096){
        error="La edición rectangular admite hasta 4096 filas por operación.";return false;
    }
    auto ranges=selection_.ranges(size_,limit);
    if(ranges.isEmpty()){error="Selecciona hasta 1 MiB de bytes para sobrescribir.";return false;}
    if(selection_.count(size_)==1&&!selection_.rectangular){
        if(bytes.size()>size_-selection_.anchor){error="La sobrescritura supera el final del archivo.";return false;}
        ranges={{selection_.anchor,bytes.size()}};
    }else if(selection_.count(size_)!=bytes.size()){
        error="El número de bytes debe coincidir con la selección. No se insertan ni eliminan bytes.";return false;
    }
    PatchList patches;qsizetype consumed=0;
    for(const auto &range:ranges){patches.append({range.offset,bytes.mid(consumed,range.length)});consumed+=range.length;}
    if(!edits_.push(std::move(patches),error))return false;
    edited();return true;
}
static QByteArray parseHexInput(QString text,QString &error){
    if(text.size()>3*1024*1024){error="El texto hexadecimal supera el límite por operación.";return {};}
    text.remove(QRegularExpression("\\s"));
    if(text.isEmpty()||text.size()%2||text.contains(QRegularExpression("[^0-9a-fA-F]"))){
        error="Usa pares hexadecimales, por ejemplo: 00 FF 2A. Solo se admiten espacios entre bytes.";return {};
    }
    return QByteArray::fromHex(text.toLatin1());
}
void HexView::editSelection(){
    if(saving()||selection_.anchor<0)return;
    QString initial;
    if(selection_.count(size_)==1&&selection_.anchor>=loadedOffset_&&selection_.anchor<loadedOffset_+bytes_.size()){
        auto bytes=bytes_;applyPatches(bytes,loadedOffset_,edits_.patches());
        initial=QString::fromLatin1(bytes.mid(selection_.anchor-loadedOffset_,1).toHex().toUpper());
    }
    bool ok=false;
    const auto text=QInputDialog::getMultiLineText(this,"Sobrescribir bytes",
        "Hexadecimal (00 FF 2A). Con un solo byte seleccionado, se escribe desde él.\n"
        "Para una selección mayor, introduce exactamente el mismo número de bytes.",initial,&ok);
    if(!ok)return;
    QString error;const auto bytes=parseHexInput(text,error);
    if(error.isEmpty())replaceSelection(bytes,error);
    if(!error.isEmpty())QMessageBox::warning(this,"No se pudo editar",error);
}
void HexView::pasteSelection(){
    if(saving())return;
    const auto *mime=QApplication::clipboard()->mimeData();QString error;
    const auto bytes=mime->hasFormat("application/octet-stream")?mime->data("application/octet-stream"):parseHexInput(mime->text(),error);
    if(error.isEmpty())replaceSelection(bytes,error);
    if(!error.isEmpty())QMessageBox::warning(this,"No se pudo pegar",error);
}
bool HexView::saveFile(const QString &destination){
    if(directReader_.busy()||!directKeys_.isEmpty()){status_->setText("Espera a que termine la escritura directa antes de guardar.");return false;}
    if(saving()||size_<0||destination.isEmpty())return false;
    resetDirectEdit();cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    saveState_->destination=destination;saveState_->patches=edits_.patches();saveState_->progress.store(0);
    writer_.request({path_,0,size_,modified_,size_});
    status_->setText("Guardando en segundo plano…");emit editStateChanged();return true;
}

void HexView::setFormatRules(QList<FormatRule> rules){
    if(rules.size()>256)return;
    formatRules_=std::move(rules);rebuild();emit formatRulesChanged();
}
StyledBytes HexView::formatAt(qint64 offset,qint64 length) const {
    StyledBytes result;
    if(offset<0||offset>=size_)return result;
    length=qMin(length,size_-offset);
    for(qint64 byte=offset;byte<offset+length;++byte)result.selected|=selection_.contains(byte);
    for(int i=0;i<formatRules_.size();++i){
        for(qint64 byte=offset;byte<offset+length;++byte)if(formatRules_[i].matches(byte)){
            result.style=formatRules_[i].style;result.rule=i;result.group=formatRules_[i].selectionRule||!formatRules_[i].matchRanges.isEmpty()?0:(byte-formatRules_[i].start)/formatRules_[i].step;
            if(!formatRules_[i].matchRanges.isEmpty()){const auto &ranges=formatRules_[i].matchRanges;result.group=std::upper_bound(ranges.begin(),ranges.end(),byte,[](qint64 value,const ByteRange &r){return value<r.offset;})-ranges.begin()-1;}
            break;
        }
    }
    return result;
}

void HexView::jumpTo(qint64 offset){resetDirectEdit();if(size_>0){offset=qBound(qint64(0),offset,size_-1);selection_={offset,offset,stride_->value(),false};navigate(offset);updateInspector();}}

bool HexView::startPatternSearch(PatternSpec spec){
    if(searchBusy()||saving()||size_<=0)return false;
    spec.patches=edits_.patches();searchState_->spec=std::move(spec);
    searchState_->cancel.store(false);searchState_->progress.store(0);
    searchReader_.request({path_,0,size_,modified_,0});return true;
}
void HexView::selectMatch(qint64 offset,qint64 length){
    resetDirectEdit();
    if(offset<0||length<=0||offset>=size_||length>size_-offset)return;
    selection_={offset,offset+length-1,stride_->value(),false};navigate(offset);updateInspector();
}

QString HexView::selectedEncoding()const{return encoding_->currentIndex()==4?QString("CP932"):encoding_->currentText();}

bool HexView::startMagicScan(MagicSpec spec){
    if(magicBusy()||saving()||size_<0)return false;
    spec.patches=edits_.patches();magicState_->spec=std::move(spec);
    magicState_->cancel.store(false);magicState_->progress.store(0);
    magicReader_.request({path_,0,size_,modified_,size_});return true;
}
QVector<ByteRange> HexView::markedRegions(QString &error) const {
    QVector<ByteRange> regions;
    auto add=[&](qint64 offset,qint64 length){if(regions.size()>=10000){error="Hay más de 10 000 regiones. Desactiva algunas reglas o analiza una selección.";return false;}if(offset>=0&&offset<size_&&length>0)regions.append({offset,qMin(length,size_-offset)});return true;};
    for(const auto &r:formatRules_)if(r.enabled){
        if(!r.matchRanges.isEmpty()){for(const auto &range:r.matchRanges)if(!add(range.offset,range.length))return {};}
        else if(r.selectionRule){
            auto s=r.selection;if(!s.rectangular){if(!add(qMin(s.anchor,s.cursor),qAbs(s.cursor-s.anchor)+1))return {};}
            else{const auto left=qMin(s.anchor%s.stride,s.cursor%s.stride),width=qAbs(s.anchor%s.stride-s.cursor%s.stride)+1;
                for(qint64 row=qMin(s.anchor/s.stride,s.cursor/s.stride);row<=qMax(s.anchor/s.stride,s.cursor/s.stride)&&row<=((size_-1)/s.stride);++row)if(!add(row*s.stride+left,width))return {};}
        }else for(qint64 i=0;i<r.repetitions&&r.start+i*r.step<size_;++i)if(!add(r.start+i*r.step,r.length))return {};
    }
    std::sort(regions.begin(),regions.end(),[](auto a,auto b){return a.offset<b.offset||(a.offset==b.offset&&a.length<b.length);});
    regions.erase(std::unique(regions.begin(),regions.end(),[](auto a,auto b){return a.offset==b.offset&&a.length==b.length;}),regions.end());return regions;
}

bool HexView::startEntropy(EntropySpec spec){
    if(entropyBusy()||saving()||size_<=0)return false;
    spec.patches=edits_.patches();entropyState_->spec=std::move(spec);entropyState_->cancel.store(false);entropyState_->progress.store(0);
    entropyReader_.request({path_,0,size_,modified_,size_});return true;
}

void HexView::resetDirectEdit(){directReader_.invalidate();directKeys_.clear();directDigit_=0;directDrafts_.clear();}
void HexView::queueDirectEdit(QString character,bool broadcast){
    if(saving()||selection_.anchor<0)return;
    if(!interpretedFocus_&&!QRegularExpression("^[0-9A-Fa-f]$").match(character).hasMatch())return;
    if(interpretedFocus_&&type_->currentText()!="Texto"&&!QRegularExpression(type_->currentText().startsWith("Float")?"^[0-9eE+.-]$":type_->currentText().startsWith("UInt")?"^[0-9]$":"^[0-9-]$").match(character).hasMatch())return;
    if(character.toUcs4().size()!=1)return;
    if(directKeys_.size()>=256){status_->setText("Espera a que terminen las pulsaciones pendientes.");return;}
    directKeys_.append({character,broadcast});startDirectEdit();
}
void HexView::startDirectEdit(){
    if(directReader_.busy()||directKeys_.isEmpty()||saving())return;
    auto key=directKeys_.takeFirst();if(key.second!=directBroadcast_){directDigit_=0;directDrafts_.clear();}directBroadcast_=key.second;
    DirectEditSpec spec;spec.character=key.first;spec.broadcast=key.second;spec.hex=!interpretedFocus_;spec.type=type_->currentText();spec.encoding=selectedEncoding();spec.bigEndian=endian_->currentIndex()==1;spec.digit=directDigit_;spec.drafts=directDrafts_;spec.patches=edits_.patches();
    if(spec.broadcast){
        if(selection_.rectangular&&qAbs(selection_.anchor/selection_.stride-selection_.cursor/selection_.stride)>=4096){directKeys_.clear();status_->setText("Máximo 4096 filas por operación.");return;}
        spec.ranges=selection_.ranges(size_,1048576);
    }else{qint64 start=qMin(selection_.anchor,selection_.cursor);int width=spec.hex?1:spec.type=="Texto"?4:numberWidth(spec.type);spec.ranges={{start,qMin(qint64(width),size_-start)}};selection_={start,start,stride_->value(),false};}
    if(spec.ranges.isEmpty()){directKeys_.clear();status_->setText("Selecciona hasta 1 MiB por operación.");return;}
    auto begin=spec.ranges.first().offset,end=spec.ranges.last().offset+spec.ranges.last().length;
    if(end-begin>1048576){directKeys_.clear();status_->setText("La selección abarca más de 1 MiB; divídela en regiones menores.");return;}
    *directSpec_=std::move(spec);directReader_.request({path_,begin,size_,modified_,end-begin});
}
