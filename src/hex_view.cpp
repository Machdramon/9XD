#include "theme.h"
#include "hex_view.h"
#include "scan_sweep.h"
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
#include <QStatusBar>
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
#include <QFileInfo>

HexView::HexView(QWidget *parent) : QWidget(parent), directSpec_(std::make_shared<DirectEditSpec>()), cache_(std::make_shared<ChunkCache>()),
    saveState_(std::make_shared<SaveState>()),
    searchState_(std::make_shared<PatternState>()),
    magicState_(std::make_shared<MagicState>()),
    entropyState_(std::make_shared<EntropyState>()),
    checksumState_(std::make_shared<ChecksumState>()),
    reader_(this,[cache=cache_](const ReadRequest &r){return cache->read(r);}),
    inspectorReader_(this,[cache=cache_](const ReadRequest &r){return cache->read(r);}),
    copyReader_(this,[cache=cache_](const ReadRequest &r){return cache->read(r);}),
    writer_(this,[state=saveState_](const ReadRequest &r){
        return savePatchedFile(r,state->destination,state->patches,&state->progress);
    }),
    searchReader_(this,[state=searchState_](const ReadRequest &r){return searchPattern(r,state->spec,state->cancel,state->progress);}),
    magicReader_(this,[state=magicState_](const ReadRequest &r){return scanMagic(r,state->spec,state->cancel,state->progress);}),
    entropyReader_(this,[state=entropyState_](const ReadRequest &r){return analyzeEntropy(r,state->spec,state->cancel,state->progress);}),
    directReader_(this,[state=directSpec_](const ReadRequest &r){return directEdit(r,*state);}),
    checksumReader_(this,[state=checksumState_](const ReadRequest &r){return calculateChecksums(r,state->spec,state->cancel,state->progress);}) {
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(4,4,4,4);layout->setSpacing(4);
    auto *format=new QHBoxLayout;
    stride_=new QSpinBox; stride_->setObjectName("stride"); stride_->setRange(1,256); stride_->setValue(16);
    stride_->setKeyboardTracking(false); stride_->setSuffix(" bytes");
    stride_->setToolTip("Bytes per row. Changing the stride rearranges the view without modifying the file.");
    encoding_=new QComboBox; encoding_->setObjectName("encoding");
    encoding_->addItems({"ASCII","UTF-8","UTF-16LE","UTF-16BE","CP932"});
    encoding_->setItemText(4,"Japanese · CP932"); encoding_->setItemData(4,"CP932");
    type_=new QComboBox; type_->setObjectName("interpretation");
    type_->addItems({"Text","UInt8","Int8","UInt16","Int16","UInt32","Int32","UInt64","Int64","Float32","Float64"});
    endian_=new QComboBox; endian_->setObjectName("endian"); endian_->addItems({"Little Endian","Big Endian"});
    format->addWidget(new QLabel("Stride")); format->addWidget(stride_);
    format->addWidget(new QLabel("Text")); format->addWidget(encoding_);
    format->addWidget(new QLabel("Interpret")); format->addWidget(type_); format->addWidget(endian_); auto *scan=new QPushButton("Scan File");scan->setObjectName("hexScanFile");scan->setToolTip("Scan recognizable structures and assets in this document.");format->addWidget(scan);connect(scan,&QPushButton::clicked,this,&HexView::scanRequested);format->addStretch();
    layout->addLayout(format);
    table_=new QTableWidget; table_->setObjectName("bytes");
    auto font=qApp->font(); font.setPointSize(qApp->property("editorFontSize").isValid()?qBound(8,qApp->property("editorFontSize").toInt(),24):10); table_->setFont(font);table_->horizontalHeader()->setFont(font);table_->verticalHeader()->setFont(font);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);table_->setAlternatingRowColors(!qApp->property("editorAlternatingRows").isValid()||qApp->property("editorAlternatingRows").toBool());
    table_->setItemDelegate(new ByteFormatDelegate(table_,[this](qint64 offset,qint64 length){return formatAt(offset,length);}));
    table_->verticalHeader()->hide(); table_->setShowGrid(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    auto *header=table_->horizontalHeader();header->setSectionsClickable(true);
    table_->setFrameShape(QFrame::NoFrame);
    header->setHighlightSections(false);
    header->setStyleSheet("QHeaderView { background: palette(base); border: none; }"
        "QHeaderView::section, QHeaderView::section:hover, QHeaderView::section:pressed {"
        " background: palette(base); color: palette(text); border: none; padding: 3px; }");
    offsetEditor_=new QLineEdit(header->viewport());offsetEditor_->setObjectName("offsetHeaderEditor");offsetEditor_->hide();
    offsetEditor_->setToolTip("Decimal: no prefix. Hexadecimal: 0x. Octal: 0c.");
    connect(header,&QHeaderView::sectionClicked,this,[this](int section){if(section==0)focusOffsetHeader();});
    connect(offsetEditor_,&QLineEdit::returnPressed,this,[this]{if(goToText(offsetEditor_->text())){offsetEditor_->hide();table_->setFocus();}});
    connect(offsetEditor_,&QLineEdit::editingFinished,offsetEditor_,&QWidget::hide);
    offsetEditor_->installEventFilter(this);
    table_->viewport()->installEventFilter(this); table_->installEventFilter(this);
    table_->setMouseTracking(true);table_->setAttribute(Qt::WA_InputMethodEnabled,true);
    table_->verticalHeader()->setDefaultSectionSize(qMax(22,table_->fontMetrics().height()+6));
    table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *dataLayout=new QHBoxLayout; dataLayout->setSpacing(0);
    scrollbar_=new QScrollBar(Qt::Vertical);scrollbar_->setObjectName("fileScroll");
    scrollbar_->setRange(0,0);scrollbar_->setTracking(true);scrollbar_->installEventFilter(this);
    scrollbar_->setToolTip("Scroll through the entire file; the mouse wheel and arrow keys move by rows.");
    dataLayout->addWidget(table_,1);dataLayout->addWidget(scrollbar_);layout->addLayout(dataLayout,1);
    statusFooter_=new QWidget(this);statusFooter_->setObjectName("hexStatusFooter");
    statusLayout_=new QHBoxLayout(statusFooter_);statusLayout_->setContentsMargins(2,0,2,0);
    status_=new QLabel("Open a local file · Read and write");
    status_->setObjectName("fileStatus");status_->setWordWrap(false);status_->setMinimumWidth(0);status_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    statusLayout_->addWidget(status_,1);layout->addWidget(statusFooter_);
    inspector_=new DataInspector(this);
    connect(inspector_, &DataInspector::endianChanged, this, [this](bool big){endian_->setCurrentIndex(big?1:0);});
    inspector_->setPlainText("Select a byte to interpret it.\n\nDrag: sequential selection.\nAlt + drag: rectangular selection.\nShift + click: extend after scrolling.");
    inspector_->hide();
    setNavigationEnabled(false);
    connect(&reader_,&BinaryReader::ready,this,&HexView::showResult);
    for (auto *reader : {&reader_,&inspectorReader_,&copyReader_,&writer_,&searchReader_,&magicReader_,&entropyReader_,&directReader_,&checksumReader_})
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
        status_->setText(QString("Unsaved changes · %1 · write position %2").arg(directBroadcast_?"Shift: entire selection":"Direct Editing").arg(directDigit_+1));
    });
    connect(&entropyReader_,&BinaryReader::ready,this,&HexView::entropyFinished);
    connect(&checksumReader_,&BinaryReader::ready,this,&HexView::checksumFinished);
    connect(&magicReader_,&BinaryReader::ready,this,&HexView::magicScanFinished);
    connect(&searchReader_,&BinaryReader::ready,this,&HexView::patternSearchFinished);
    connect(&writer_,&BinaryReader::ready,this,[this](const ReadResult &r){
        if(!r.error.isEmpty()) {
            status_->setText("Save error · " + r.error);
            emit editStateChanged(); emit saveFinished(false); return;
        }
        reader_.invalidate(); inspectorReader_.invalidate(); copyReader_.invalidate();
        path_=saveState_->destination; size_=r.size; modified_=r.modified;
        edits_.clear(); saveState_->patches.clear(); cache_->reset(path_);
        if(memory_){savedMemory_=memory_;memoryHistory_.clear();memoryCursor_=0;}
        emit fileOpened(path_); emit editStateChanged();
        requestViewport(); updateInspector();
        status_->setText("File saved · Read and write");
        emit saveFinished(true);
    });
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
            if(part.size()!=range.length){status_->setText("Could not copy the entire selection.");return;}
            raw+=part; lines<<QString::fromLatin1(part.toHex(' ').toUpper());
        }
        auto *mime=new QMimeData; mime->setText(lines.join(copying_.rectangular?'\n':' '));
        mime->setData("application/octet-stream",raw); QApplication::clipboard()->setMimeData(mime);
        status_->setText(QString("Copied %1 bytes as hexadecimal. Rectangular selections use one line per row.").arg(raw.size()));
    });
}
int HexView::visibleRows() const {
    return qBound(1,table_->viewport()->height()/table_->verticalHeader()->defaultSectionSize(),512);
}
qint64 HexView::pageCapacity()const{return qint64(visibleRows())*stride_->value();}
qint64 HexView::maxTopRow()const{
    if(size_<=0)return 0;
    return qMax(qint64(0),(memory_?size_/stride_->value()+1:(size_-1)/stride_->value()+1)-visibleRows());
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
    reader_.request(sourceRequest(desiredOffset_,viewportRequestLength_));
}
bool HexView::busy()const{return reader_.busy()||inspectorReader_.busy()||copyReader_.busy()||writer_.busy()||searchReader_.busy()||magicReader_.busy()||entropyReader_.busy()||directReader_.busy()||checksumReader_.busy();}
void HexView::shutdown(){reader_.shutdown();inspectorReader_.shutdown();copyReader_.shutdown();writer_.shutdown();cancelPatternSearch();searchReader_.shutdown();cancelMagicScan();magicReader_.shutdown();cancelEntropy();entropyReader_.shutdown();resetDirectEdit();directReader_.shutdown();cancelChecksum();checksumReader_.shutdown();}
QWidget *HexView::inspector()const{return inspector_;}
void HexView::focusInterpretation(){type_->setFocus();type_->showPopup();}
void HexView::setNavigationEnabled(bool enabled){offsetEditor_->setEnabled(enabled);scrollbar_->setEnabled(enabled);}
void HexView::openFile(const QString &path){
    if(saving())return;setProperty("nativeResourceOrigin",QVariant());emit sourceReplaced();
    resetDirectEdit();cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    memory_.reset();savedMemory_.reset();memoryHistory_.clear();memoryCursor_=0;
    edits_.clear();
    inspectorReader_.invalidate(); copyReader_.invalidate(); dragging_=false;
    formatRules_.clear();emit formatRulesChanged();
    cache_->reset(path);
    path_=path;size_=-1;desiredOffset_=0;modified_={};bytes_.clear();selection_={};emit editStateChanged();
    setNavigationEnabled(false);table_->setRowCount(0);emit selectionAvailable(false);
    inspector_->setPlainText("Opening file…");status_->setText("Opening local file…");configureScrollBar();viewportRequestLength_=pageCapacity();reader_.request({path,0,-1,{},viewportRequestLength_});
}
void HexView::navigate(qint64 offset){
    if(size_<0||(!memory_&&size_==0))return; offset=qBound(qint64(0),offset,memory_?size_:size_-1);
    desiredOffset_=qMin(offset/stride_->value(),maxTopRow())*stride_->value();bytes_.clear();table_->setRowCount(0);
    configureScrollBar();
    status_->setText("Reading blocks…");requestViewport();
}
void HexView::showResult(const ReadResult &r){
    if(!r.error.isEmpty()){inspectorReader_.invalidate();bytes_.clear();table_->setRowCount(0);setNavigationEnabled(false);selection_={};emit selectionAvailable(false);status_->setText(r.error);inspector_->setPlainText(r.error);return;}
    const bool first=size_<0;
    size_=r.size;modified_=r.modified;desiredOffset_=r.request.offset;loadedOffset_=desiredOffset_;bytes_=r.bytes.left(pageCapacity());
    if(first&&size_>0)selection_={0,0,stride_->value(),false};
    if(first)emit fileOpened(path_);rebuild();setNavigationEnabled(size_>0||bool(memory_));
    configureScrollBar();
    updateOffsetHeader();
    status_->setText(size_==0?"Empty file · 0 bytes":QString("%1 bytes · View from 0x%2 · Cache: %3 / 20 blocks (%4 MiB) · %5").arg(size_).arg(desiredOffset_,0,16).arg(cache_->chunkCount()).arg(double(cache_->retainedBytes())/(1024*1024),0,'f',3).arg(dirty()?"Unsaved Changes":"Read and write"));
    emit editStateChanged();
    if(size_==0)inspector_->setPlainText("Empty file.");else if(first)updateInspector();
}
void HexView::applyDisplayPreferences(){
    auto font=table_->font();font.setPointSize(qBound(8,qApp->property("editorFontSize").toInt(),24));table_->setFont(font);table_->horizontalHeader()->setFont(font);table_->verticalHeader()->setFont(font);
    table_->verticalHeader()->setDefaultSectionSize(qMax(22,table_->fontMetrics().height()+6));
    table_->setAlternatingRowColors(qApp->property("editorAlternatingRows").toBool());
    if(size_>=0){rebuild();configureScrollBar();if(size_>0)navigate(desiredOffset_);}
}
void HexView::rebuild(){
    const int stride=stride_->value();QSignalBlocker block(table_);table_->setUpdatesEnabled(false);
    table_->clear();table_->setColumnCount(stride+2);table_->setRowCount(int((bytes_.size()+stride-1)/stride)+(memory_&&loadedOffset_+bytes_.size()==size_&&bytes_.size()%stride==0?1:0));
    QStringList labels{"Offset"};for(int i=0;i<stride;++i)labels<<QString::number(i,16).toUpper();labels<<"Decoded Text / Values";
    table_->setHorizontalHeaderLabels(labels);table_->setColumnWidth(0,qMax(150,table_->fontMetrics().horizontalAdvance("Offset (h): 0x"+QString(qMax(8,int(QString::number(qMax(qint64(0),size_),offsetBase_).size())),'0'))+22));
    for(int col=1;col<=stride;++col)table_->setColumnWidth(col,table_->fontMetrics().horizontalAdvance("00 ")+(byteGroup_>1&&col%byteGroup_==0?table_->fontMetrics().horizontalAdvance(" "):0));
    table_->setColumnWidth(stride+1,300);
    for(int col=1;col<=stride;++col)table_->setColumnHidden(col,columnMode_==2);
    table_->setColumnHidden(stride+1,columnMode_==1);
    const auto encoding=encoding_->currentIndex()==4?QString("CP932"):encoding_->currentText();
    auto displayed=bytes_; applyPatches(displayed,loadedOffset_,edits_.patches());
    for(int row=0;row<table_->rowCount();++row){
        const auto part=displayed.mid(row*stride,stride);
        table_->setItem(row,0,new QTableWidgetItem(offsetText(loadedOffset_+row*stride,true)));
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
            if(type_->currentText()!="Text"&&directDrafts_.contains(token.offset))token.text=directDrafts_.value(token.offset);
            tokens.append(tokenData(token.text,token.offset,token.length));
            if(!interpreted.isEmpty()&&type_->currentText()!="Text")interpreted+=" · ";interpreted+=token.text;
        }
        auto *item=new QTableWidgetItem(interpreted);
        item->setFont(table_->font());
        item->setData(ByteFormatDelegate::TokensRole,tokens);
        item->setToolTip(interpreted+"\nCharacter/value formatting covers all its bytes. When rules overlap, the last rule takes precedence.");table_->setItem(row,stride+1,item);
        qreal decodedWidth=12;
        for(const auto &entry:tokens){auto token=entry.toMap();auto style=formatAt(token["offset"].toLongLong(),token["length"].toLongLong()).style;auto font=table_->font();font.setBold(style.bold);font.setItalic(style.italic);decodedWidth+=qMax(qreal(8),QFontMetricsF(font).horizontalAdvance(token["text"].toString()));}
        table_->setColumnWidth(stride+1,qMax(table_->columnWidth(stride+1),int(decodedWidth)));
    }
    if(memory_&&loadedOffset_<=size_&&size_-loadedOffset_<pageCapacity()){
        const int row=int((size_-loadedOffset_)/stride),col=int((size_-loadedOffset_)%stride)+1;
        if(row<table_->rowCount())table_->setItem(row,col,new QTableWidgetItem("·"));
    }
    // Explicit fonts also cover offsets, headers and empty RAM insertion cells.
    // These otherwise use Qt's default item font instead of the decoded font.
    for(int col=0;col<table_->columnCount();++col)if(auto *item=table_->horizontalHeaderItem(col))item->setFont(table_->font());
    for(int row=0;row<table_->rowCount();++row)for(int col=0;col<table_->columnCount();++col)if(auto *item=table_->item(row,col))item->setFont(table_->font());
    table_->setUpdatesEnabled(true);updateSelection();
}
void HexView::updateSelection(){
    const int stride=stride_->value();
    updateOffsetHeader();
    for(int row=0;row<table_->rowCount();++row)for(int col=1;col<=stride;++col){
        auto *item=table_->item(row,col);if(!item)continue;const qint64 offset=loadedOffset_+row*stride+col-1;
        const bool selected=selection_.contains(offset);
        item->setBackground(selected?palette().highlight():QBrush());item->setForeground(selected?palette().highlightedText():palette().text());
        item->setFont(table_->font());
        item->setToolTip(QString("0x%1%2").arg(offset,0,16).arg(offset==selection_.anchor?" · selection anchor":""));
    }
    table_->viewport()->update();
    emit selectionAvailable(selection_.count(size_)>0);
}
void HexView::updateInspector(){
    if(selection_.anchor<0||size_<=0||selection_.anchor>=size_){if(memory_)inspector_->setPlainText("End of document · type hexadecimal bytes or decoded text to append.");return;}
    inspectorReader_.request(sourceRequest(selection_.anchor));
}
void HexView::selectByte(qint64 offset,bool extend,bool rectangle){
    resetDirectEdit();
    if(offset<0||offset>size_||(!memory_&&offset==size_))return;
    if(!extend||selection_.anchor<0)selection_={offset,offset,stride_->value(),rectangle};
    else{selection_.cursor=offset;selection_.rectangular=rectangle;}
    if(interpretedFocus_&&type_->currentText()!="Text"&&!selection_.rectangular&&offset>=selection_.anchor)selection_.cursor=qMin(size_-1,offset+numberWidth(type_->currentText())-1);
    updateSelection();updateInspector();
}
qint64 HexView::byteAt(const QPoint &point)const{
    auto index=table_->indexAt(point);if(!index.isValid())return -1;
    if(index.column()==stride_->value()+1){
        auto *item=table_->item(index.row(),index.column());if(!item)return -1;if(memory_&&item->data(ByteFormatDelegate::TokensRole).toList().isEmpty())return size_;qreal x=table_->visualRect(index).left()+4;
        for(const auto &v:item->data(ByteFormatDelegate::TokensRole).toList()){auto t=v.toMap();auto style=formatAt(t["offset"].toLongLong(),t["length"].toLongLong()).style;auto font=table_->font();font.setBold(style.bold);font.setItalic(style.italic);auto width=qMax(qreal(8),QFontMetricsF(font).horizontalAdvance(t["text"].toString()));if(point.x()>=x&&point.x()<x+width)return t["offset"].toLongLong();x+=width;}return -1;
    }
    if(index.column()<1||index.column()>stride_->value())return -1;
    const auto offset=loadedOffset_+index.row()*stride_->value()+index.column()-1;
    return offset<loadedOffset_+bytes_.size()||(memory_&&offset==size_)?offset:-1;
}
bool HexView::eventFilter(QObject *watched,QEvent *event){
    if(watched==offsetEditor_&&event->type()==QEvent::KeyPress&&static_cast<QKeyEvent *>(event)->key()==Qt::Key_Escape){offsetEditor_->hide();table_->setFocus();return true;}
    if(watched==table_&&event->type()==QEvent::InputMethod&&interpretedFocus_&&type_->currentText()=="Text"){
        auto *input=static_cast<QInputMethodEvent *>(event);for(char32_t c:input->commitString().toUcs4())queueDirectEdit(QString::fromUcs4(&c,1),QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier));input->accept();return true;
    }

    if(watched==table_->viewport()&&event->type()==QEvent::Resize&&!resizePending_){
        resizePending_=true;
        QTimer::singleShot(0,this,[this]{resizePending_=false;if(size_>0&&pageCapacity()!=viewportRequestLength_)navigate(desiredOffset_);});
    }
    if((watched==table_->viewport()||watched==scrollbar_)&&event->type()==QEvent::Wheel){
        auto *wheel=static_cast<QWheelEvent *>(event);
        if(watched==table_->viewport()&&wheel->modifiers().testFlag(Qt::ControlModifier)){
            const bool angles=!wheel->angleDelta().isNull();const int unit=angles?120:40;
            zoomRemainder_+=angles?wheel->angleDelta().y():wheel->pixelDelta().y();
            const int steps=zoomRemainder_/unit;zoomRemainder_%=unit;
            const int points=qBound(8,table_->font().pointSize()+steps,24);
            if(points!=table_->font().pointSize()){
                qApp->setProperty("editorFontSize",points);
                if(!qApp->property("editorAlternatingRows").isValid())qApp->setProperty("editorAlternatingRows",table_->alternatingRowColors());
                applyDisplayPreferences();emit viewerFontSizeChanged(points);
            }
            wheel->accept();return true;
        }
        zoomRemainder_=0;
        if(wheel->modifiers().testFlag(Qt::ShiftModifier)||qAbs(wheel->angleDelta().x())>qAbs(wheel->angleDelta().y()))return false;
        if(!wheel->pixelDelta().isNull()){
            pixelRemainder_+=wheel->pixelDelta().y();
            const auto height=table_->verticalHeader()->defaultSectionSize();const auto rows=pixelRemainder_/height;pixelRemainder_%=height;if(rows)scrollRows(-rows);
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
    if(watched==table_&&event->type()==QEvent::KeyPress&&(size_>0||memory_)){
        auto *key=static_cast<QKeyEvent *>(event);
        if(memory_&&key->key()==Qt::Key_Insert){insertMode_=!insertMode_;pendingNibble_=-1;refreshMemory();return true;}
        if(memory_&&key->key()==Qt::Key_Delete){deleteSelection();return true;}
        if(memory_&&key->key()==Qt::Key_Backspace){if(selection_.count(size_)<=1&&selection_.anchor>0)selection_={selection_.anchor-1,selection_.anchor-1,stride_->value(),false};deleteSelection();return true;}
        if(key->matches(QKeySequence::Paste)){pasteSelection();return true;}
        if(key->matches(QKeySequence::Undo)){undo();return true;}
        if(key->matches(QKeySequence::Redo)){redo();return true;}
        if(key->key()==Qt::Key_F2){editSelection();return true;}
        if(memory_&&key->matches(QKeySequence::Cut)){cutSelection();return true;}
        if(key->matches(QKeySequence::Copy)){copySelection();return true;}
        if(key->matches(QKeySequence::SelectAll)){selectWholeDocument();return true;}
        if(key->key()==Qt::Key_PageDown){scrollRows(visibleRows());return true;}
        if(key->key()==Qt::Key_PageUp){scrollRows(-visibleRows());return true;}
        if(key->key()==Qt::Key_Home){navigate(0);return true;}
        if(key->key()==Qt::Key_End){if(memory_){selection_={size_,size_,stride_->value(),false};navigate(size_);updateInspector();}else navigate(maxTopRow()*stride_->value());return true;}
        if(key->key()==Qt::Key_Escape){resetDirectEdit();return true;}
        if(!(key->modifiers()&(Qt::ControlModifier|Qt::MetaModifier|Qt::AltModifier))&&!key->text().isEmpty()){
            QString text=key->text();bool broadcast=key->modifiers().testFlag(Qt::ShiftModifier);
            if((!interpretedFocus_||type_->currentText()!="Text")&&key->key()>=Qt::Key_0&&key->key()<=Qt::Key_9)text=QString::number(key->key()-Qt::Key_0);
#ifdef Q_OS_MACOS
            // Cocoa can report the shifted symbol as both key() and text().
            // For numeric editing, recover the digit from the native number-row key.
            if(broadcast&&(!interpretedFocus_||type_->currentText()!="Text")){
                static constexpr quint32 numberKeys[]={29,18,19,20,21,23,22,26,28,25};
                for(int digit=0;digit<10;++digit)if(key->nativeVirtualKey()==numberKeys[digit]){text=QString::number(digit);break;}
            }
#endif
            if(!interpretedFocus_&&key->key()>=Qt::Key_A&&key->key()<=Qt::Key_F)text=QChar('A'+key->key()-Qt::Key_A);
            if(interpretedFocus_&&type_->currentText()!="Text"){
                if(key->key()==Qt::Key_Minus)text="-";if(key->key()==Qt::Key_Period)text=".";
            }
            queueDirectEdit(text,broadcast);return true;
        }
        qint64 delta=0;
        if(key->key()==Qt::Key_Left)delta=-1;else if(key->key()==Qt::Key_Right)delta=1;
        else if(key->key()==Qt::Key_Up)delta=-stride_->value();else if(key->key()==Qt::Key_Down)delta=stride_->value();
        if(delta){
            auto offset=selection_.cursor>=0?selection_.cursor:loadedOffset_;
            if(delta>0)offset+=qMin(delta,(memory_?size_:size_-1)-offset);else offset=qMax(qint64(0),offset+delta);
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
    if(ranges.isEmpty()){status_->setText("Select between 1 byte and 1 MiB to copy.");return;}
    const auto begin=ranges.first().offset;const auto end=ranges.last().offset+ranges.last().length;
    if(end-begin>limit){status_->setText("The selection exceeds 1 MiB. Copy smaller regions.");return;}
    copying_=selection_;status_->setText("Copying selection…");
    copyReader_.request(sourceRequest(begin,end-begin));
}

void HexView::addStatusWidget(QWidget *widget){statusLayout_->addWidget(widget);}
void HexView::restoreStatusFooter(){
    if(statusFooter_->parentWidget()==this)return;
    if(auto *bar=qobject_cast<QStatusBar *>(statusFooter_->parentWidget()))bar->removeWidget(statusFooter_);
    statusFooter_->setParent(this);layout()->addWidget(statusFooter_);statusFooter_->hide();
}
QByteArray HexView::interpretedBytes(const ReadResult &result) const {
    auto bytes=result.bytes; applyPatches(bytes,result.request.offset,edits_.patches()); return bytes;
}
void HexView::edited(){
    cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    rebuild(); updateInspector(); emit editStateChanged();
    status_->setText(dirty()?"Unsaved changes · Read and write":"No pending changes · Read and write");
}
void HexView::undo(){if(saving())return;resetDirectEdit();if(memory_){if(memoryCursor_>0){memory_=memoryHistory_[--memoryCursor_].before;selection_={0,0,stride_->value(),false};emit documentChanged();refreshMemory();emit editStateChanged();}}else{edits_.undo();edited();}}
void HexView::redo(){if(saving())return;resetDirectEdit();if(memory_){if(memoryCursor_<memoryHistory_.size()){memory_=memoryHistory_[memoryCursor_++].after;selection_={0,0,stride_->value(),false};emit documentChanged();refreshMemory();emit editStateChanged();}}else{edits_.redo();edited();}}
bool HexView::replaceSelection(const QByteArray &bytes,QString &error){
    resetDirectEdit();
    constexpr qint64 limit=1024*1024;
    if(saving()){error="Wait for saving to finish.";return false;}
    if(memory_){
        if(selection_.rectangular){error="Use a contiguous selection for RAM document edits.";return false;}
        if(bytes.isEmpty()||bytes.size()>limit){error="Enter between 1 byte and 1 MiB.";return false;}
        const auto offset=qMax(qint64(0),qMin(selection_.anchor,selection_.cursor));
        const auto count=selection_.count(size_);
        const auto remove=count>1?count:insertMode_?qint64(0):qMin(qint64(bytes.size()),size_-offset);
        return spliceMemory(offset,remove,bytes,error);
    }
    if(bytes.isEmpty()||bytes.size()>limit){error="Enter between 1 byte and 1 MiB per operation.";return false;}
    if(selection_.rectangular&&qAbs(selection_.anchor/selection_.stride-selection_.cursor/selection_.stride)>=4096){
        error="Rectangular editing supports up to 4096 rows per operation.";return false;
    }
    auto ranges=selection_.ranges(size_,limit);
    if(ranges.isEmpty()){error="Select up to 1 MiB of bytes to overwrite.";return false;}
    if(selection_.count(size_)==1&&!selection_.rectangular){
        if(bytes.size()>size_-selection_.anchor){error="The overwrite extends beyond the end of the file.";return false;}
        ranges={{selection_.anchor,bytes.size()}};
    }else if(selection_.count(size_)!=bytes.size()){
        error="The byte count must match the selection. Bytes are not inserted or deleted.";return false;
    }
    PatchList patches;qsizetype consumed=0;
    for(const auto &range:ranges){patches.append({range.offset,bytes.mid(consumed,range.length)});consumed+=range.length;}
    if(!edits_.push(std::move(patches),error))return false;
    edited();return true;
}
static QByteArray parseHexInput(QString text,QString &error){
    if(text.size()>3*1024*1024){error="The hexadecimal text exceeds the per-operation limit.";return {};}
    text.remove(QRegularExpression("\\s"));
    if(text.isEmpty()||text.size()%2||text.contains(QRegularExpression("[^0-9a-fA-F]"))){
        error="Use hexadecimal pairs, for example: 00 FF 2A. Only spaces are allowed between bytes.";return {};
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
    const auto text=QInputDialog::getMultiLineText(this,"Overwrite Bytes",
        "Hexadecimal (00 FF 2A). With a single byte selected, writing starts at that byte.\n"
        "For a larger selection, enter exactly the same number of bytes.",initial,&ok);
    if(!ok)return;
    QString error;const auto bytes=parseHexInput(text,error);
    if(error.isEmpty())replaceSelection(bytes,error);
    if(!error.isEmpty())QMessageBox::warning(this,"Could Not Edit",error);
}
void HexView::pasteSelection(){
    if(saving())return;
    const auto *mime=QApplication::clipboard()->mimeData();QString error;
    const auto bytes=mime->hasFormat("application/octet-stream")?mime->data("application/octet-stream"):parseHexInput(mime->text(),error);
    if(error.isEmpty())replaceSelection(bytes,error);
    if(!error.isEmpty())QMessageBox::warning(this,"Could Not Paste",error);
}
bool HexView::saveFile(const QString &destination){
    if(directReader_.busy()||!directKeys_.isEmpty()){status_->setText("Wait for direct editing to finish before saving.");return false;}
    if(saving()||size_<0||destination.isEmpty())return false;
    resetDirectEdit();cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    saveState_->destination=destination;saveState_->patches=edits_.patches();saveState_->progress.store(0);
    writer_.request(sourceRequest(0,size_));
    status_->setText("Saving in the background…");emit editStateChanged();return true;
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
    bool changed=memory_&&dirty();
    if(!memory_)for(const auto &patch:edits_.patches())if(offset<patch.offset+patch.bytes.size()&&offset+length>patch.offset){changed=true;break;}
    if(changed&&result.rule<0){result.style.foreground=themeColor("hex.modified",palette().color(QPalette::Base).lightness()<128?QColor("#F5C87A"):QColor("#9B4F08"));result.style.bold=true;}
    return result;
}

void HexView::jumpTo(qint64 offset){resetDirectEdit();if(size_>0){offset=qBound(qint64(0),offset,size_-1);selection_={offset,offset,stride_->value(),false};navigate(offset);updateInspector();}}

bool HexView::startPatternSearch(PatternSpec spec){
    if(searchBusy()||saving()||size_<=0)return false;
    spec.patches=edits_.patches();searchState_->spec=std::move(spec);
    searchState_->cancel.store(false);searchState_->progress.store(0);
    searchReader_.request(sourceRequest(0,0));return true;
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
    magicReader_.request(sourceRequest(0,size_));return true;
}
QVector<ByteRange> HexView::markedRegions(QString &error) const {
    QVector<ByteRange> regions;
    auto add=[&](qint64 offset,qint64 length){if(regions.size()>=10000){error="There are more than 10,000 regions. Disable some rules or analyze a selection.";return false;}if(offset>=0&&offset<size_&&length>0)regions.append({offset,qMin(length,size_-offset)});return true;};
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

bool HexView::startChecksum(ChecksumSpec spec){
    if(checksumBusy()||saving()||size_<0||directEditing())return false;
    spec.patches=edits_.patches();checksumState_->spec=std::move(spec);checksumState_->cancel.store(false);checksumState_->progress.store(0);
    checksumReader_.request(sourceRequest(0,0));return true;
}
bool HexView::startEntropy(EntropySpec spec){
    if(entropyBusy()||saving()||size_<=0)return false;
    spec.patches=edits_.patches();entropyState_->spec=std::move(spec);entropyState_->cancel.store(false);entropyState_->progress.store(0);
    entropyReader_.request(sourceRequest(0,size_));return true;
}

void HexView::resetDirectEdit(){pendingNibble_=-1;directReader_.invalidate();directKeys_.clear();directDigit_=0;directDrafts_.clear();}
void HexView::queueDirectEdit(QString character,bool broadcast){
    if(saving()||selection_.anchor<0)return;
    if(memory_){typeMemory(character);return;}
    if(!interpretedFocus_&&!QRegularExpression("^[0-9A-Fa-f]$").match(character).hasMatch())return;
    if(interpretedFocus_&&type_->currentText()!="Text"&&!QRegularExpression(type_->currentText().startsWith("Float")?"^[0-9eE+.-]$":type_->currentText().startsWith("UInt")?"^[0-9]$":"^[0-9-]$").match(character).hasMatch())return;
    if(character.toUcs4().size()!=1)return;
    if(directKeys_.size()>=256){status_->setText("Wait for pending keystrokes to finish.");return;}
    directKeys_.append({character,broadcast});startDirectEdit();
}
void HexView::startDirectEdit(){
    if(directReader_.busy()||directKeys_.isEmpty()||saving())return;
    auto key=directKeys_.takeFirst();if(key.second!=directBroadcast_){directDigit_=0;directDrafts_.clear();}directBroadcast_=key.second;
    DirectEditSpec spec;spec.character=key.first;spec.broadcast=key.second;spec.hex=!interpretedFocus_;spec.type=type_->currentText();spec.encoding=selectedEncoding();spec.bigEndian=endian_->currentIndex()==1;spec.digit=directDigit_;spec.drafts=directDrafts_;spec.patches=edits_.patches();
    if(spec.broadcast){
        if(selection_.rectangular&&qAbs(selection_.anchor/selection_.stride-selection_.cursor/selection_.stride)>=4096){directKeys_.clear();status_->setText("Maximum 4096 rows per operation.");return;}
        spec.ranges=selection_.ranges(size_,1048576);
    }else{qint64 start=qMin(selection_.anchor,selection_.cursor);int width=spec.hex?1:spec.type=="Text"?4:numberWidth(spec.type);spec.ranges={{start,qMin(qint64(width),size_-start)}};selection_={start,start,stride_->value(),false};}
    if(spec.ranges.isEmpty()){directKeys_.clear();status_->setText("Select up to 1 MiB per operation.");return;}
    auto begin=spec.ranges.first().offset,end=spec.ranges.last().offset+spec.ranges.last().length;
    if(end-begin>1048576){directKeys_.clear();status_->setText("The selection exceeds 1 MiB; split it into smaller regions.");return;}
    *directSpec_=std::move(spec);directReader_.request(sourceRequest(begin,end-begin));
}

void HexView::newMemoryDocument(const QByteArray &initialBytes){
    if(saving())return;setProperty("nativeResourceOrigin",QVariant());emit sourceReplaced();
    resetDirectEdit();cancelPatternSearch();cancelMagicScan();cancelEntropy();
    reader_.invalidate();inspectorReader_.invalidate();copyReader_.invalidate();
    auto initial=std::make_shared<MemoryData>();initial->size=initialBytes.size();for(qsizetype p=0;p<initialBytes.size();p+=MemoryData::BlockSize)initial->blocks.append(initialBytes.mid(p,MemoryData::BlockSize));
    memory_=initial;savedMemory_=memory_;memoryHistory_.clear();memoryCursor_=0;edits_.clear();
    path_.clear();size_=initialBytes.size();modified_={};desiredOffset_=loadedOffset_=0;selection_={0,0,stride_->value(),false};
    formatRules_.clear();cache_->reset({});insertMode_=false;
    emit documentChanged();emit formatRulesChanged();refreshMemory();emit fileOpened({});emit editStateChanged();
    table_->setFocus();
}
void HexView::refreshMemory(){
    size_=memory_->size;desiredOffset_=qMin(desiredOffset_,maxTopRow()*stride_->value());
    if(selection_.anchor>=desiredOffset_+pageCapacity())desiredOffset_=qMin(selection_.anchor/stride_->value(),maxTopRow())*stride_->value();
    if(selection_.anchor<desiredOffset_)desiredOffset_=qMax(qint64(0),selection_.anchor/stride_->value())*stride_->value();
    loadedOffset_=desiredOffset_;bytes_=memory_->read(loadedOffset_,pageCapacity());
    viewportRequestLength_=pageCapacity();configureScrollBar();setNavigationEnabled(true);rebuild();
    updateInspector();
    status_->setText(QString("%1 bytes · %2 · %3 · %4").arg(size_).arg(path_.isEmpty()?"RAM document":QFileInfo(path_).fileName(),dirty()?"Modified":"Unmodified",insertMode_?"Insert":"Overwrite"));
}
bool HexView::spliceMemory(qint64 offset,qint64 remove,const QByteArray &insert,QString &error){
    if(!memory_||saving()){error="This operation requires an editable RAM document.";return false;}
    if(offset<0||remove<0||offset>size_||remove>size_-offset||insert.size()>1048576){error="Invalid edit: use a valid range and at most 1 MiB of inserted data.";return false;}
    if(remove==0&&insert.isEmpty())return true;
    qint64 retained=0;for(qsizetype i=0;i<memoryCursor_;++i)retained+=memoryHistory_[i].cost;
    const qint64 cost=qMax(qint64(MemoryData::BlockSize),remove+insert.size());
    if(memoryCursor_>=1024||retained+cost>EditHistory::MaxBytes){error="Undo history is full. Save before continuing to edit.";return false;}
    try {
        auto next=memory_->splice(offset,remove,insert);
        while(memoryHistory_.size()>memoryCursor_)memoryHistory_.removeLast();
        memoryHistory_.append({memory_,next,cost});++memoryCursor_;memory_=std::move(next);
    }catch(const std::bad_alloc &){error="Not enough memory for this edit. The document was not changed.";return false;}
    reader_.invalidate();inspectorReader_.invalidate();copyReader_.invalidate();
    pendingNibble_=-1;selection_={offset+insert.size(),offset+insert.size(),stride_->value(),false};
    cancelPatternSearch();cancelMagicScan();cancelEntropy();emit documentChanged();
    refreshMemory();emit editStateChanged();return true;
}
void HexView::discardEdits(){
    if(saving())return;resetDirectEdit();
    if(memory_){memory_=savedMemory_;memoryHistory_.clear();memoryCursor_=0;selection_={0,0,stride_->value(),false};emit documentChanged();refreshMemory();emit editStateChanged();}
    else{edits_.clear();edited();}
}
void HexView::deleteSelection(){
    if(!memory_||saving())return;
    auto count=selection_.count(size_);if(count<=0||selection_.rectangular)return;
    QString error;if(!spliceMemory(qMin(selection_.anchor,selection_.cursor),count,{},error))status_->setText(error);
}
void HexView::insertBytes(){
    if(!memory_||saving())return;bool ok=false;
    auto text=QInputDialog::getMultiLineText(this,"Insert Bytes","Hexadecimal bytes to insert (for example: 00 FF 2A)",{},&ok);
    if(!ok)return;text.remove(QRegularExpression("\\s"));
    if(text.isEmpty()||text.size()%2||text.size()>2097152||text.contains(QRegularExpression("[^0-9A-Fa-f]"))){QMessageBox::warning(this,"Insert Bytes","Enter hexadecimal pairs, up to 1 MiB.");return;}
    QString error;if(!spliceMemory(qMax(qint64(0),qMin(selection_.anchor,selection_.cursor)),0,QByteArray::fromHex(text.toLatin1()),error))status_->setText(error);
}
void HexView::typeMemory(QString character){
    if(saving()||character.toUcs4().size()!=1)return;
    if(!interpretedFocus_){
        bool ok=false;int digit=character.toInt(&ok,16);if(!ok)return;
        if(pendingNibble_<0){pendingNibble_=digit;status_->setText(QString("Hex input: %1_ · type the second digit · Esc to cancel").arg(character.toUpper()));return;}
        QByteArray value(1,char(pendingNibble_*16+digit));pendingNibble_=-1;QString error;
        if(!replaceSelection(value,error))status_->setText(error);
    }else if(type_->currentText()=="Text"){
        auto value=encodeInputCharacter(character,selectedEncoding());QString error;
        if(value.isEmpty()){status_->setText("This character is not available in the selected encoding.");return;}
        if(!replaceSelection(value,error))status_->setText(error);
    }else{
        if(selection_.anchor>=size_){status_->setText("Append bytes using hexadecimal or text input first.");return;}
        DirectEditSpec spec;spec.character=character;spec.hex=false;spec.type=type_->currentText();spec.encoding=selectedEncoding();spec.bigEndian=endian_->currentIndex()==1;
        spec.digit=directDigit_;spec.drafts=directDrafts_;spec.ranges={{selection_.anchor,qMin(qint64(numberWidth(spec.type)),size_-selection_.anchor)}};
        auto result=directEdit(sourceRequest(selection_.anchor,spec.ranges.first().length),spec);
        if(!result.error.isEmpty()){status_->setText(result.error);return;}
        const auto anchor=selection_.anchor;QString error;
        if(spliceMemory(anchor,result.bytes.size(),result.bytes,error)){
            directDrafts_=result.directDrafts;++directDigit_;
            if(directDigit_<result.directCycle)selection_={anchor,anchor,stride_->value(),false};else{directDigit_=0;directDrafts_.clear();}
            refreshMemory();
        }else status_->setText(error);
    }
}

void HexView::cutSelection(){
    if(!memory_||saving()||selection_.rectangular)return;
    const auto count=selection_.count(size_);if(count<=0||count>1048576){status_->setText("Select up to 1 MiB to cut.");return;}
    const auto start=qMin(selection_.anchor,selection_.cursor);auto bytes=memory_->read(start,count);
    auto *mime=new QMimeData;mime->setData("application/octet-stream",bytes);mime->setText(QString::fromLatin1(bytes.toHex(' ').toUpper()));
    QApplication::clipboard()->setMimeData(mime);QString error;if(!spliceMemory(start,count,{},error))status_->setText(error);
}

QString HexView::offsetText(qint64 offset,bool padded)const{
    auto digits=QString::number(offset,offsetBase_).toUpper();
    if(padded)digits=digits.rightJustified(qMax(8,int(QString::number(qMax(qint64(0),size_),offsetBase_).size())),QChar('0'));
    return (offsetBase_==16?QString("0x"):offsetBase_==8?QString("0c"):QString())+digits;
}
void HexView::updateOffsetHeader(){
    const auto offset=qMax(qint64(0),selection_.cursor);
    if(auto *item=table_->horizontalHeaderItem(0)){
        item->setText(QString("Offset (%1): %2").arg(offsetBase_==16?"h":offsetBase_==8?"o":"d",offsetText(offset)));
        item->setToolTip("Click to enter an offset. Decimal has no prefix; hexadecimal uses 0x and octal uses 0c. Unprefixed input uses the selected base.");
    }
}
bool HexView::goToText(const QString &input){
    auto text=input.trimmed();int base=offsetBase_;
    for(auto entry:{qMakePair(QString("0x"),16),qMakePair(QString("0d"),10),qMakePair(QString("0c"),8)})if(text.startsWith(entry.first,Qt::CaseInsensitive)){base=entry.second;text=text.mid(2);break;}
    bool ok=false;const auto offset=text.toLongLong(&ok,base);
    if(!ok||offset<0||offset>size_||(!memory_&&offset==size_)){status_->setText("Invalid offset for this document.");return false;}
    resetDirectEdit();selection_={offset,offset,stride_->value(),false};navigate(offset);updateSelection();updateInspector();return true;
}
void HexView::setOffsetBase(int base){if(base!=8&&base!=10&&base!=16)return;offsetBase_=base;rebuild();}
void HexView::setColumnMode(int mode){if(mode<0||mode>2)return;columnMode_=mode;interpretedFocus_=mode==2;rebuild();}
void HexView::setByteGroup(int bytes){if(bytes<1||bytes>256)return;byteGroup_=bytes;rebuild();}
void HexView::chooseBytesPerRow(){bool ok=false;int n=QInputDialog::getInt(this,"Bytes per row","Bytes per row:",stride_->value(),1,256,1,&ok);if(ok)stride_->setValue(n);}
void HexView::chooseByteGroup(){bool ok=false;int n=QInputDialog::getInt(this,"Byte group","Bytes per group:",byteGroup_,1,256,1,&ok);if(ok)setByteGroup(n);}

void HexView::focusOffsetHeader(){
    if(size_<0)return;
    auto *header=table_->horizontalHeader();table_->horizontalScrollBar()->setValue(0);
    offsetEditor_->setGeometry(header->sectionViewportPosition(0),0,header->sectionSize(0),header->height());
    offsetEditor_->setFont(table_->font());offsetEditor_->setText(offsetText(qMax(qint64(0),selection_.cursor)));
    offsetEditor_->show();offsetEditor_->setFocus();offsetEditor_->selectAll();
}

// Cell colours are cached in the table items; refresh them when the theme (application palette) changes.
void HexView::changeEvent(QEvent *event){QWidget::changeEvent(event);if(event->type()==QEvent::PaletteChange&&table_)updateSelection();}
void HexView::setScanAnimation(bool active){
    if(!table_)return;
    if(!scanSweep_)scanSweep_=new ScanSweepOverlay(table_->viewport(),[this]{
        // Only the populated byte and text columns, not the offset column or the empty space below the last row.
        const int rows=table_->rowCount(),columns=table_->columnCount();if(!rows||columns<2)return QRect();auto *viewport=table_->viewport();
        const int left=table_->columnViewportPosition(1),right=table_->columnViewportPosition(columns-1)+table_->columnWidth(columns-1),bottom=table_->rowViewportPosition(rows-1)+table_->rowHeight(rows-1);
        return QRect(QPoint(qMax(0,left),0),QPoint(qMin(right,viewport->width())-1,qMin(bottom,viewport->height())-1));});
    active?scanSweep_->start():scanSweep_->stop();
}
