#include "file_dialogs.h"
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
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include "adaptive_layout.h"
namespace {QString swatchStyle(const QColor &color){return "QToolButton { background-color: "+color.name()+"; border: 1px solid rgba(128,128,128,160); border-radius: 4px; }";}}

FormatPanel::FormatPanel(HexView *view,QWidget *parent):QWidget(parent),view_(view){
    // Structure: rules (what is marked), the rule editor (where) and the appearance (how). Side by side when the panel
    // is wide, stacked and foldable when it is narrow.
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(8,8,8,8);layout->setSpacing(6);
    toolbar_=new QToolBar(this);toolbar_->setObjectName("byteFormatToolbar");toolbar_->hide();
    auto button=[this](QHBoxLayout *row,QString text,QString tooltip,std::function<void()> fn){auto *b=AdaptiveLayout::tool(row,text,tooltip);connect(b,&QToolButton::clicked,this,std::move(fn));return b;};
    // Appearance: one row per effect (check, swatch, value), then text style and the jump target.
    auto *styleBox=new QWidget;styleBox->setObjectName("structureAppearance");auto *grid=new QGridLayout(styleBox);grid->setContentsMargins(0,0,0,0);grid->setHorizontalSpacing(8);grid->setVerticalSpacing(4);grid->setColumnStretch(2,1);
    auto check=[this,grid](QString label,QString name,int row,bool checked=false,int column=0,int span=1){
        auto *box=new QCheckBox(label);box->setObjectName(name);box->setChecked(checked);grid->addWidget(box,row,column,1,span);
        connect(box,&QCheckBox::toggled,this,[this]{if(!loadingStyle_)applySelection();});return box;
    };
    textEnabled_=check("Text","formatText",0,true);fillEnabled_=check("Background","formatBackground",1,true);
    borderEnabled_=check("Outline","formatBorder",2);underline_=check("Underline","formatUnderline",3);
    QColor *colors[]={&foreground_,&background_,&border_,&underlineColor_};
    for(int i=0;i<4;++i){
        auto *swatch=new QToolButton;swatch->setObjectName(QString("formatSwatch%1").arg(i));swatch->setFixedSize(22,22);swatch->setToolTip("Choose Color");swatch->setProperty("keepTooltip",true);grid->addWidget(swatch,i,1);
        auto *field=new QLineEdit(colors[i]->name().toUpper());colorFields_[i]=field;field->setObjectName(QString("formatColor%1").arg(i));field->setMaxLength(7);field->setMaximumWidth(90);field->setToolTip("Hexadecimal color #RRGGBB");grid->addWidget(field,i,2,Qt::AlignLeft);
        connect(field,&QLineEdit::textEdited,this,[this,i,value=colors[i]](const QString &text){bool valid=QRegularExpression("^#[0-9A-Fa-f]{6}$").match(text).hasMatch();colorFields_[i]->setStyleSheet(valid?QString():"border: 1px solid #D05050;");if(!valid)return;*value=QColor(text);findChild<QToolButton *>(QString("formatSwatch%1").arg(i))->setStyleSheet(swatchStyle(*value));if(!loadingStyle_)applySelection();});
        connect(field,&QLineEdit::editingFinished,this,[this]{updateColorFields();});
        connect(swatch,&QToolButton::clicked,this,[this,value=colors[i]]{auto color=QColorDialog::getColor(*value,this,"Choose Color");if(!color.isValid())return;*value=color;updateColorFields();applySelection();});
    }
    bold_=check("Bold","formatBold",4);italic_=check("Italic","formatItalic",4,false,1,2);
    jump_=new QLineEdit("0x0");jump_->setObjectName("formatJumpTo");jump_->setMaxLength(18);jump_->setMaximumWidth(120);jump_->setToolTip("Hexadecimal destination for this rule. Press Enter to jump; its ranges are not changed.");grid->addWidget(new QLabel("Jump to"),5,0);grid->addWidget(jump_,5,1,1,2,Qt::AlignLeft);
    // Rules: the list with a compact tool row.
    auto *rulesBox=new QWidget;auto *rulesLayout=new QVBoxLayout(rulesBox);rulesLayout->setContentsMargins(0,0,0,0);rulesLayout->setSpacing(4);
    list_=new QTreeWidget;list_->setObjectName("formatRules");list_->setHeaderLabels({"Rule","Region"});list_->setRootIsDecorated(false);list_->setMinimumHeight(80);list_->setToolTip("Double-click a rule to edit it.");rulesLayout->addWidget(list_,1);
    connect(jump_,&QLineEdit::editingFinished,this,[this]{
        if(loadingStyle_||list_->selectedItems().isEmpty())return;int index=list_->indexOfTopLevelItem(list_->currentItem());if(index<0)return;
        QString text=jump_->text().trimmed();if(text.startsWith("0x",Qt::CaseInsensitive))text.remove(0,2);bool ok;auto offset=text.toLongLong(&ok,16);
        if(!ok||offset<0||offset>=view_->fileSize()){jump_->setStyleSheet("border: 1px solid #D05050;");return;}
        jump_->setStyleSheet({});auto rules=view_->formatRules();if(rules[index].jumpTo!=offset){rules[index].jumpTo=offset;publish(rules);}
    });
    connect(jump_,&QLineEdit::returnPressed,this,[this]{jump_->editingFinished();if(jump_->styleSheet().isEmpty())navigateSelected();});
    connect(list_,&QTreeWidget::itemClicked,this,[this](auto *,int){navigateSelected();});
    connect(list_,&QTreeWidget::itemActivated,this,[this](auto *,int){navigateSelected();});
    auto *controls=new QHBoxLayout;controls->setSpacing(2);rulesLayout->addLayout(controls);
    button(controls,"+","Add Rule",[this]{beginRule(false);});button(controls,QString(QChar(0x2212)),"Delete Rule",[this]{int i=list_->indexOfTopLevelItem(list_->currentItem());if(i>=0){auto r=view_->formatRules();r.removeAt(i);publish(r);editor_->hide();}});
    button(controls,QString(QChar(0x270E)),"Edit Rule",[this]{beginRule(true);});
    for(int delta:{-1,1})button(controls,delta<0?"↑":"↓",delta<0?"Move Rule Up":"Move Rule Down",[this,delta]{int i=list_->indexOfTopLevelItem(list_->currentItem());auto r=view_->formatRules();if(i>=0&&i+delta>=0&&i+delta<r.size()){r.swapItemsAt(i,i+delta);publish(r);list_->setCurrentItem(list_->topLevelItem(i+delta));}});
    controls->addStretch();button(controls,"Import","Import Rules",[this]{importRules();});button(controls,"Export","Export Rules",[this]{exportRules();});
    // Rule editor: shown by + and by double-clicking a rule; it opens Appearance so every effect is at hand.
    editor_=new QFrame;editor_->setObjectName("inspectorCard");auto *editorLayout=new QVBoxLayout(editor_);editorLayout->setContentsMargins(8,6,8,8);editorLayout->setSpacing(4);
    auto *editorTitle=new QLabel("New rule");editorTitle->setObjectName("ruleEditorTitle");editorTitle->setProperty("cardTitle",true);editorLayout->addWidget(editorTitle);
    name_=new QLineEdit("Sequence");name_->setMaxLength(200);start_=new QLineEdit("0x0");length_=new QLineEdit("4");step_=new QLineEdit("16");repetitions_=new QLineEdit("1");
    start_->setObjectName("ruleStart");length_->setObjectName("ruleLength");step_->setObjectName("ruleStep");repetitions_->setObjectName("ruleRepetitions");
    for(auto *f:{start_,length_,step_,repetitions_}){f->setMaxLength(20);f->setToolTip("Decimal or hexadecimal (0x…)");}
    auto *fields=new QGridLayout;fields->setHorizontalSpacing(6);fields->setVerticalSpacing(4);
    fields->addWidget(new QLabel("Name"),0,0);fields->addWidget(name_,0,1,1,3);
    fields->addWidget(new QLabel("Start"),1,0);fields->addWidget(start_,1,1);fields->addWidget(new QLabel("Length"),1,2);fields->addWidget(length_,1,3);
    fields->addWidget(new QLabel("Step"),2,0);fields->addWidget(step_,2,1);fields->addWidget(new QLabel("Repeat"),2,2);fields->addWidget(repetitions_,2,3);
    editorLayout->addLayout(fields);
    auto *editorButtons=new QHBoxLayout;editorButtons->setSpacing(4);
    button(editorButtons,"Selection","Use selection as region",[this]{auto s=view_->selection();if(s.anchor<0)return;start_->setText(QString::number(qMin(s.anchor,s.cursor)));length_->setText(QString::number(qAbs(s.cursor-s.anchor)+1));step_->setText(length_->text());repetitions_->setText("1");});
    editorButtons->addStretch();button(editorButtons,"Apply","Save Rule",[this]{addRule(list_->currentItem()!=nullptr);})->setProperty("primaryTool",true);button(editorButtons,"Cancel","Cancel rule editing",[this]{editor_->hide();});editorLayout->addLayout(editorButtons);editor_->hide();
    connect(list_,&QTreeWidget::itemSelectionChanged,this,[this]{loadSelected();});
    connect(list_,&QTreeWidget::itemDoubleClicked,this,[this]{beginRule(true);});
    connect(list_,&QTreeWidget::itemChanged,this,[this](QTreeWidgetItem *item,int column){if(column)return;auto r=view_->formatRules();int i=list_->indexOfTopLevelItem(item);if(i>=0){r[i].enabled=item->checkState(0)==Qt::Checked;publish(r);}});
    connect(view_,&HexView::formatRulesChanged,this,[this]{refresh();});
    auto *body=new QHBoxLayout;body->setSpacing(10);layout->addLayout(body,1);
    body->addWidget(AdaptiveLayout::section("Rules",rulesBox,"structureRulesSection"),3);
    auto *side=new QWidget;auto *sideLayout=new QVBoxLayout(side);sideLayout->setContentsMargins(0,0,0,0);sideLayout->setSpacing(6);
    sideLayout->addWidget(editor_);sideLayout->addWidget(AdaptiveLayout::section("Appearance",styleBox,"structureAppearanceSection"));sideLayout->addStretch();
    body->addWidget(side,2);AdaptiveLayout::flip(this,body);
    updateColorFields();// The swatches are found through this panel, so only once the sections are in it.
    // Find: pattern and Find on top, options folded, results with a compact navigation row. Results sit beside the
    // query when the panel is wide.
    auto *searchBox=new QWidget;searchBox->setObjectName("patternSearchPanel");auto *searchOuter=new QHBoxLayout(searchBox);searchOuter->setContentsMargins(8,8,8,8);searchOuter->setSpacing(10);
    auto *query=new QWidget;auto *searchLayout=new QVBoxLayout(query);searchLayout->setContentsMargins(0,0,0,0);searchLayout->setSpacing(6);
    searchMode_=new QComboBox;searchMode_->setObjectName("patternMode");searchMode_->addItems({"Hex + wildcards","Byte regex","Literal text","Text regex"});
    pattern_=new QLineEdit;pattern_->setObjectName("patternExpression");pattern_->setMaxLength(4096);pattern_->setPlaceholderText("48 8B ?? A?");
    pattern_->setToolTip("Hex: ?? = byte, A? = wildcard digit. Byte regex: \\x48. Text uses the viewer encoding. Text/regex: region ≤ 1 MiB; ^ and $ are the region boundaries.");
    auto *queryRow=new QHBoxLayout;queryRow->setSpacing(4);queryRow->addWidget(searchMode_);queryRow->addWidget(pattern_,1);searchLayout->addLayout(queryRow);
    auto *searchButtons=new QHBoxLayout;searchButtons->setSpacing(4);
    button(searchButtons,"Find","Find Pattern",[this]{startSearch();})->setProperty("primaryTool",true);button(searchButtons,"Stop","Cancel search",[this]{view_->cancelPatternSearch();});
    searchButtons->addStretch();button(searchButtons,"Selection","Search selection",[this]{auto s=view_->selection();if(s.anchor>=0){searchScope_->setCurrentIndex(1);searchStart_->setText(QString::number(qMin(s.anchor,s.cursor)));searchLength_->setText(QString::number(qAbs(s.cursor-s.anchor)+1));}});
    searchLayout->addLayout(searchButtons);
    auto *options=new QWidget;auto *searchForm=new QFormLayout(options);searchForm->setContentsMargins(0,0,0,0);searchForm->setVerticalSpacing(4);
    searchScope_=new QComboBox;searchScope_->setObjectName("patternScope");searchScope_->addItems({"Entire document","Specified region","Contiguous selection","Cursor to end"});
    searchStart_=new QLineEdit("0");searchLength_=new QLineEdit("4096");searchStart_->setObjectName("patternStart");searchLength_->setObjectName("patternLength");
    auto *region=new QHBoxLayout;region->setSpacing(4);region->addWidget(searchStart_);region->addWidget(new QLabel("Bytes"));region->addWidget(searchLength_);
    searchForm->addRow("Scope",searchScope_);searchForm->addRow("Start",region);
    caseSensitive_=new QCheckBox("Case sensitive");caseSensitive_->setChecked(true);caseSensitive_->setEnabled(false);caseSensitive_->setObjectName("patternCaseSensitive");
    overlap_=new QCheckBox("Overlapping matches");overlap_->setChecked(false);overlap_->setObjectName("patternOverlap");
    auto *flags=new QHBoxLayout;flags->addWidget(caseSensitive_);flags->addWidget(overlap_);flags->addStretch();searchForm->addRow(flags);
    alignment_=new QSpinBox;alignment_->setRange(1,1048576);alignment_->setObjectName("patternAlignment");alignment_->setToolTip("Absolute byte alignment");
    resultLimit_=new QSpinBox;resultLimit_->setRange(1,10000);resultLimit_->setValue(10000);resultLimit_->setObjectName("patternResultLimit");resultLimit_->setToolTip("Maximum results");
    auto *limits=new QHBoxLayout;limits->setSpacing(4);limits->addWidget(alignment_);limits->addWidget(new QLabel("Max"));limits->addWidget(resultLimit_);searchForm->addRow("Align",limits);
    searchLayout->addWidget(AdaptiveLayout::section("Options",options,"patternOptionsSection",false));searchLayout->addStretch();
    auto *found=new QWidget;auto *foundLayout=new QVBoxLayout(found);foundLayout->setContentsMargins(0,0,0,0);foundLayout->setSpacing(4);
    searchStatus_=new QLabel("No search");searchStatus_->setObjectName("inspectorHint");searchStatus_->setWordWrap(true);foundLayout->addWidget(searchStatus_);
    searchResults_=new QTreeWidget;searchResults_->setObjectName("patternResults");searchResults_->setHeaderLabels({"Offset","Bytes"});searchResults_->setRootIsDecorated(false);foundLayout->addWidget(searchResults_,1);
    connect(searchResults_,&QTreeWidget::currentItemChanged,this,[this](QTreeWidgetItem *item){if(item)matchIndex_->setValue(searchResults_->indexOfTopLevelItem(item)+1);});
    auto *matchesRow=new QHBoxLayout;matchesRow->setSpacing(2);
    button(matchesRow,"←","Previous match",[this]{matchIndex_->stepDown();});matchIndex_=new QSpinBox;matchIndex_->setObjectName("matchIndex");matchIndex_->setRange(0,0);matchesRow->addWidget(matchIndex_);button(matchesRow,"→","Next match",[this]{matchIndex_->stepUp();});
    matchesRow->addStretch();button(matchesRow,"Mark","Create rule from matches",[this]{markMatches();});
    button(matchesRow,"Copy","Copy results",[this]{QString text="Offset\tBytes\n";for(auto m:matches_)text+=QString("0x%1\t%2\n").arg(m.first,0,16).arg(m.second);QApplication::clipboard()->setText(text);});
    foundLayout->addLayout(matchesRow);
    searchOuter->addWidget(query,2);searchOuter->addWidget(found,3);AdaptiveLayout::flipColumns(searchBox,searchOuter);layout->addWidget(searchBox);
    connect(matchIndex_,&QSpinBox::valueChanged,this,[this](int value){if(value>0&&value<=matches_.size()){QSignalBlocker block(searchResults_);searchResults_->setCurrentItem(searchResults_->topLevelItem(value-1));view_->selectMatch(matches_[value-1].first,matches_[value-1].second);}});
    connect(searchMode_,&QComboBox::currentIndexChanged,this,[this](int mode){caseSensitive_->setEnabled(mode!=0);pattern_->setPlaceholderText(mode==0?"48 8B ?? A?":mode==1?"\\x48.{2}\\x41":mode==2?"Text to find":"Name[0-9]+");});
    auto invalidate=[this]{searchStale_=true;view_->cancelPatternSearch();matches_.clear();searchResults_->clear();matchIndex_->setRange(0,0);searchStatus_->setText("Options changed; search again.");};
    for(auto *field:{pattern_,searchStart_,searchLength_})connect(field,&QLineEdit::textChanged,this,invalidate);
    for(auto *box:{searchMode_,searchScope_})connect(box,&QComboBox::currentIndexChanged,this,invalidate);
    for(auto *box:{caseSensitive_,overlap_})connect(box,&QCheckBox::toggled,this,invalidate);
    for(auto *spin:{alignment_,resultLimit_})connect(spin,&QSpinBox::valueChanged,this,invalidate);
    connect(view_,&HexView::documentChanged,this,[this]{searchStale_=true;matches_.clear();searchResults_->clear();matchIndex_->setRange(0,0);searchStatus_->setText("The content changed; search again.");});
    connect(view_,&HexView::patternSearchFinished,this,[this](const ReadResult &r){
        if(searchStale_){searchStatus_->setText("Results discarded: the document changed.");return;}
        if(r.cancelled){searchStatus_->setText("Search canceled.");return;}
        if(!r.error.isEmpty()){searchStatus_->setText(r.error);return;}
        matches_=r.matches;searchResults_->clear();for(auto m:matches_)new QTreeWidgetItem(searchResults_,{QString("0x%1").arg(m.first,0,16),QString::number(m.second)});searchStatus_->setText(QString("%1 matches%2").arg(matches_.size()).arg(r.truncated?" · Result limit reached; search incomplete":""));
        QSignalBlocker block(matchIndex_);matchIndex_->setRange(matches_.isEmpty()?0:1,matches_.size());
        if(!matches_.isEmpty()){matchIndex_->setValue(1);view_->selectMatch(matches_[0].first,matches_[0].second);}
    });
    auto *timer=new QTimer(this);timer->setInterval(200);connect(timer,&QTimer::timeout,this,[this]{if(view_->searchBusy()&&!searchStale_)searchStatus_->setText(QString("Searching… %1 MiB processed").arg(double(view_->searchProgress())/(1024*1024),0,'f',2));});timer->start();
    refresh();
}
void FormatPanel::beginRule(bool replace){
    if(replace){if(!list_->currentItem())return;loadSelected();}
    else {list_->setCurrentItem(nullptr);name_->setText("Sequence");for(auto *f:{start_,length_,step_,repetitions_})f->setEnabled(true);}
    if(auto *title=findChild<QLabel *>("ruleEditorTitle"))title->setText(replace?"Edit rule":"New rule");
    AdaptiveLayout::open(findChild<QWidget *>("structureAppearanceSection"));editor_->show();name_->setFocus();name_->selectAll();
}
void FormatPanel::startSearch(){
    if(view_->searchBusy()){searchStatus_->setText("A search is running. Stop it before starting another.");return;}
    PatternSpec spec;int mode=searchMode_->currentIndex();spec.regex=mode==1||mode==3;spec.text=mode>=2;spec.encoding=view_->selectedEncoding();spec.expression=pattern_->text();
    spec.caseSensitive=caseSensitive_->isChecked();spec.overlap=overlap_->isChecked();spec.alignment=alignment_->value();spec.maxMatches=resultLimit_->value();
    if(searchScope_->currentIndex()==0){spec.begin=0;spec.length=view_->fileSize();}
    else if(searchScope_->currentIndex()==2){auto selection=view_->selection();if(selection.rectangular||selection.anchor<0){searchStatus_->setText("Select a contiguous byte range.");return;}spec.begin=qMin(selection.anchor,selection.cursor);spec.length=selection.count(view_->fileSize());}
    else if(searchScope_->currentIndex()==3){spec.begin=qMax(qint64(0),view_->selection().cursor);spec.length=view_->fileSize()-spec.begin;}
    else{auto read=[](QString text,bool &ok){text=text.trimmed();return text.startsWith("0x",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,16):text.startsWith("0c",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,8):text.toLongLong(&ok,10);};bool a,b;spec.begin=read(searchStart_->text(),a);spec.length=read(searchLength_->text(),b);if(!a||!b){searchStatus_->setText("Start and length must be valid numbers.");return;}}
    matches_.clear();searchResults_->clear();matchIndex_->setRange(0,0);searchStale_=false;matchedPattern_=spec.expression;
    if(!view_->startPatternSearch(spec))searchStatus_->setText("Open a file and wait for saving to finish.");else searchStatus_->setText("Searching…");
}
void FormatPanel::markMatches(){
    if(matches_.isEmpty()||view_->searchBusy()||searchStale_)return;
    auto rules=view_->formatRules();qsizetype total=matches_.size();for(const auto &r:rules)total+=r.matchRanges.size();
    if(rules.size()>=256||total>10000){searchStatus_->setText("Maximum 256 rules and 10,000 saved matches. Delete a rule before adding another.");return;}
    FormatRule r;r.name=QString("Pattern: %1").arg(matchedPattern_.left(180));r.style=selectedStyle();
    for(const auto &m:matches_)r.matchRanges.append({m.first,m.second});rules.append(r);publish(rules);
}

ByteStyle FormatPanel::selectedStyle()const{
    return {fillEnabled_->isChecked()?background_:QColor(),textEnabled_->isChecked()?foreground_:QColor(),borderEnabled_->isChecked()?border_:QColor(),bold_->isChecked(),italic_->isChecked(),underline_->isChecked(),underlineColor_};
}
void FormatPanel::publish(QList<FormatRule> rules){dirty_=true;view_->setFormatRules(std::move(rules));}
void FormatPanel::refresh(){
    QSignalBlocker block(list_);const int current=list_->indexOfTopLevelItem(list_->currentItem());list_->clear();
    for(const auto &r:view_->formatRules()){
        const auto region=!r.matchRanges.isEmpty()?QString("%1 matches").arg(r.matchRanges.size()):r.selectionRule?QString("0x%1–0x%2%3").arg(qMin(r.selection.anchor,r.selection.cursor),0,16).arg(qMax(r.selection.anchor,r.selection.cursor),0,16).arg(r.selection.rectangular?" · rectangular":""):
            QString("0x%1 · %2 B × %3 · step %4").arg(r.start,0,16).arg(r.length).arg(r.repetitions).arg(r.step);
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
    if(!replace&&view_->formatRules().size()>=256){QMessageBox::warning(this,"Rule Limit","Up to 256 rules are supported per file.");return;}
    if(replace&&(view_->formatRules()[index].selectionRule||!view_->formatRules()[index].matchRanges.isEmpty())){
        auto rules=view_->formatRules();rules[index].style=selectedStyle();rules[index].name=name_->text();publish(rules);editor_->hide();return;
    }
    auto read=[](QLineEdit *field,qint64 &value){bool ok;auto text=field->text().trimmed();value=text.startsWith("0x",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,16):text.startsWith("0c",Qt::CaseInsensitive)?text.mid(2).toLongLong(&ok,8):text.toLongLong(&ok,10);return ok;};
    FormatRule r;r.name=name_->text();r.style=selectedStyle();
    if(!read(start_,r.start)||!read(length_,r.length)||!read(step_,r.step)||!read(repetitions_,r.repetitions)||r.start<0||r.length<1||r.step<r.length||r.repetitions<1||view_->fileSize()<r.length||r.start>view_->fileSize()-r.length||(r.repetitions-1)>(view_->fileSize()-r.start-r.length)/r.step){
        QMessageBox::warning(this,"Invalid Rule","Use a positive length, step ≥ length and at least one repetition. All sequences must fit in the file.");return;
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
    const auto path=QFileDialog::getSaveFileName(this,"Export Visual Formatting",{},"Formatting rules (*.json)");if(path.isEmpty())return false;
    QJsonArray rules;for(const auto &r:view_->formatRules())rules.append(encodeRule(r));
    QSaveFile file(path);file.setDirectWriteFallback(false);
    const auto bytes=QJsonDocument(QJsonObject{{"version",1},{"rules",rules}}).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){QMessageBox::warning(this,"Could Not Export",file.errorString());return false;}
    dirty_=false;return true;
}
void FormatPanel::importRules(){
    if(!view_->hasFile()){QMessageBox::information(this,"Open File","First open the binary file to which the rules will apply.");return;}
    const auto path=WorkbenchFiles::getOpenFileName(this,"Import Visual Formatting",{},"Formatting rules (*.json)");if(path.isEmpty())return;
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
    if(!valid){QMessageBox::warning(this,"Invalid Rules","The file contains no valid rules for this binary (maximum 256 rules and 1 MiB). Formatting was not changed.");return;}
    if(!confirmLeave())return;
    view_->setFormatRules(rules);dirty_=false;
}
bool FormatPanel::confirmLeave(){
    if(!dirty_)return true;
    const auto choice=QMessageBox::question(this,"Unexported Formatting","Visual rules are saved separately from the binary. Export them before continuing?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if(choice==QMessageBox::Cancel)return false;
    if(choice==QMessageBox::Save)return exportRules();
    dirty_=false;return true;
}

bool FormatPanel::markExternalMatches(const QString &name,const QVector<ByteRange> &matches,QString &error){
    auto rules=view_->formatRules();qsizetype total=matches.size();for(const auto &r:rules)total+=r.matchRanges.size();
    if(matches.isEmpty()||rules.size()>=256||total>10000){error="Limit: 256 rules and 10,000 marked offsets. Delete rules or mark fewer matches.";return false;}
    FormatRule r;r.name=name.left(200);r.style=selectedStyle();r.matchRanges=matches;rules.append(r);publish(rules);return true;
}

void FormatPanel::updateColorFields(){
    const QColor colors[]={foreground_,background_,border_,underlineColor_};
    for(int i=0;i<4;++i){colorFields_[i]->setText(colors[i].name().toUpper());colorFields_[i]->setStyleSheet({});findChild<QToolButton *>(QString("formatSwatch%1").arg(i))->setStyleSheet(swatchStyle(colors[i]));}
}
void FormatPanel::navigateSelected(){
    if(list_->selectedItems().isEmpty())return;int index=list_->indexOfTopLevelItem(list_->currentItem());if(index<0)return;
    const auto &r=view_->formatRules()[index];qint64 target=r.jumpTo>=0?r.jumpTo:!r.matchRanges.isEmpty()?r.matchRanges.first().offset:r.selectionRule?qMin(r.selection.anchor,r.selection.cursor):r.start;
    if(target>=0&&target<view_->fileSize())view_->jumpTo(target);
}
