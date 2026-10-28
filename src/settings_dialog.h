#pragma once
#include "theme.h"
#include "theme_editor.h"
#include "ai_settings.h"
#include "memory_budget.h"
#include "import_preferences.h"
#include "camera_navigation.h"
#include "hardware_performance.h"
#include "interface_modes.h"
#include "adaptive_layout.h"
#include <QStandardItemModel>
#include <QGroupBox>
#include <QHash>
#include <QFutureWatcher>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <functional>

inline void loadWorkbenchPreferences(QSettings &settings){
    CameraNavigation::load(settings);
    ImportPreferences::fm1FlipU.store(settings.value("3d/import/fm1FlipU",false).toBool());
    ImportPreferences::fm1FlipV.store(settings.value("3d/import/fm1FlipV",false).toBool());
    AssetMemory::setGiB(settings.value("performance/assetMemoryGiB",8).toInt());QThreadPool::globalInstance()->setMaxThreadCount(qBound(1,settings.value("performance/cpuWorkers",qMax(1,QThread::idealThreadCount()-1)).toInt(),qMax(1,QThread::idealThreadCount())));
    QJsonObject themes;QString error;auto saved=QJsonDocument::fromJson(settings.value("themes/v1").toByteArray()).object();for(auto it=saved.begin();it!=saved.end();++it)if(it.key().startsWith("custom:")&&validateTheme(it.value().toObject(),error))themes[it.key()]=it.value();qApp->setProperty("workbenchThemeLibrary",QJsonDocument(themes).toJson(QJsonDocument::Compact));
    {InterfaceModes::load(settings);applyWorkbenchTheme(settings.value("theme","default").toString());InterfaceModes::Chrome::apply();}
    qApp->setProperty("workbenchLanguage",settings.value("language","system").toString());
    qApp->setProperty("workbenchDetailedTooltips",settings.value("interface/detailedTooltips",false).toBool());
    qApp->setProperty("adaptiveSideBySide",qBound(360,settings.value("interface/sideBySide",640).toInt(),2000));AdaptiveLayout::refreshAll();
    qApp->setProperty("editorFontSize",qBound(8,settings.value("editor/fontSize",10).toInt(),24));
    qApp->setProperty("editorAlternatingRows",settings.value("editor/alternatingRows",true).toBool());
}

class SettingsDialog final : public QDialog {
public:
    SettingsDialog(QSettings &settings,std::function<void()> applied,QWidget *parent=nullptr)
        :QDialog(parent),settings_(settings),applied_(std::move(applied)){
        setObjectName("settingsDialog");setWindowTitle("Settings");resize(1120,760);setMinimumSize(1000,680);
        auto *layout=new QVBoxLayout(this);auto *body=new QHBoxLayout;layout->addLayout(body,1);
        auto *sections=new QTreeWidget;sections->setObjectName("settingsSections");sections->setAccessibleName("Settings sections");
        sections->setHeaderHidden(true);sections->setFixedWidth(190);sections->setRootIsDecorated(false);
        auto *generalSection=new QTreeWidgetItem(sections,{"General"});generalSection->setData(0,Qt::UserRole,0);
        auto *appearanceSection=new QTreeWidgetItem(sections,{"Appearance"});appearanceSection->setData(0,Qt::UserRole,1);auto *themesSection=new QTreeWidgetItem(appearanceSection,{"Themes"});themesSection->setData(0,Qt::UserRole,1);appearanceSection->setExpanded(true);
        auto *editorSection=new QTreeWidgetItem(sections,{"Editor"});editorSection->setData(0,Qt::UserRole,2);
        auto *hexSection=new QTreeWidgetItem(editorSection,{"Hexadecimal Viewer"});hexSection->setData(0,Qt::UserRole,3);editorSection->setExpanded(true);
        auto *aiSection=new QTreeWidgetItem(sections,{"AI Assistance"});aiSection->setData(0,Qt::UserRole,4);auto *pages=new QStackedWidget;pages->setObjectName("settingsPages");body->addWidget(sections);body->addWidget(pages,1);
        auto *performanceSection=new QTreeWidgetItem(sections,{"Performance"});performanceSection->setData(0,Qt::UserRole,5);
        auto *threeDSection=new QTreeWidgetItem(sections,{"3D"});threeDSection->setData(0,Qt::UserRole,6);auto *importSection=new QTreeWidgetItem(threeDSection,{"Import"});importSection->setData(0,Qt::UserRole,6);threeDSection->setExpanded(true);auto *navigationSection=new QTreeWidgetItem(threeDSection,{"Navigation"});navigationSection->setData(0,Qt::UserRole,7);
        connect(sections,&QTreeWidget::currentItemChanged,pages,[pages](QTreeWidgetItem *item){if(item)pages->setCurrentIndex(item->data(0,Qt::UserRole).toInt());});
        auto *general=new QWidget;auto *generalForm=new QFormLayout(general);
        language_=new QComboBox;language_->setObjectName("settingsLanguage");
        language_->addItem("System default (English)","system");language_->addItem("English","en");
        generalForm->addRow("Language",language_);
        auto *languageHelp=new QLabel("English is currently the available interface language. Additional languages will appear here when translation packs are added.");
        languageHelp->setWordWrap(true);generalForm->addRow(languageHelp);
        detailedTooltips_=new QCheckBox("Show detailed tooltips");detailedTooltips_->setObjectName("settingsDetailedTooltips");detailedTooltips_->setChecked(settings.value("interface/detailedTooltips",false).toBool());generalForm->addRow("Tooltips",detailedTooltips_);auto *tooltipHelp=new QLabel("Off: only icon buttons name themselves on hover. Views keep their own help, such as Controls at the bottom of the 3D viewport.");tooltipHelp->setWordWrap(true);generalForm->addRow(tooltipHelp);pages->addWidget(general);
        auto *appearance=new QWidget;auto *appearanceLayout=new QVBoxLayout(appearance);theme_=new QComboBox;theme_->setObjectName("settingsTheme");theme_->addItem("Dark","dark");theme_->addItem("Light","light");theme_->addItem("System","system");themeEditor_=new ThemeEditor(theme_,settings);appearanceLayout->addWidget(themeEditor_);pages->addWidget(appearance);
        {// Interface mode: style (how things are drawn) x panel layout x colours, combined freely. Classic + Classic is 0.3.7.
            auto *page=new QWidget;auto *layout=new QVBoxLayout(page);auto *form=new QFormLayout;layout->addLayout(form);
            auto fill=[](QComboBox *combo,const QList<InterfaceModes::Mode> &modes){auto *model=qobject_cast<QStandardItemModel *>(combo->model());for(const auto &mode:modes){combo->addItem(mode.name+(mode.status=="experimental"?QString(" — experimental"):mode.status=="planned"?QString(" — planned"):QString()),mode.id);if(mode.status=="planned"&&model)model->item(combo->count()-1)->setEnabled(false);}};
            interfaceMode_=new QComboBox;interfaceMode_->setObjectName("settingsInterfaceMode");fill(interfaceMode_,InterfaceModes::styles());
            form->addRow("Style",interfaceMode_);auto *summary=new QLabel;summary->setObjectName("settingsInterfaceSummary");summary->setWordWrap(true);form->addRow(summary);
            auto *colorsHelp=new QLabel("Colours always come from your theme (Themes page or the Theme menu), so any theme works with any style. To retune a style's bars, glass, panel titles or progress bars, duplicate a theme and edit its Interface Styles colours.");colorsHelp->setObjectName("settingsInterfaceColorsHelp");colorsHelp->setWordWrap(true);form->addRow(colorsHelp);
            interfaceLayout_=new QComboBox;interfaceLayout_->setObjectName("settingsInterfaceLayout");fill(interfaceLayout_,InterfaceModes::layouts());
            form->addRow("Panel layout",interfaceLayout_);auto *layoutSummary=new QLabel;layoutSummary->setObjectName("settingsInterfaceLayoutSummary");layoutSummary->setWordWrap(true);form->addRow(layoutSummary);
            auto *crt=new QGroupBox("Hackerman CRT");crt->setObjectName("settingsCrtOptions");crt->setProperty("modeId","hackerman-crt");auto *crtForm=new QFormLayout(crt);
            crtStatusLine_=new QCheckBox("Terminal status line instead of the module rail (Alt+1…6 switch modes)");crtBoxTitles_=new QCheckBox("Box-drawn panel titles");crtTerminalMenus_=new QCheckBox("Terminal menus and square corners");crtGlow_=new QCheckBox("Phosphor glow on text");
            crtScanlines_=new QSlider(Qt::Horizontal);crtScanlines_->setObjectName("settingsCrtScanlines");crtScanlines_->setRange(0,100);
            for(auto *box:{crtStatusLine_,crtBoxTitles_,crtTerminalMenus_,crtGlow_})crtForm->addRow(box);crtForm->addRow("Scanlines",crtScanlines_);
            auto value=[&](const char *key,QVariant fallback){return settings.value(QString("interface/hackerman-crt/")+key,fallback);};
            crtStatusLine_->setChecked(value("statusLine",true).toBool());crtBoxTitles_->setChecked(value("boxTitles",true).toBool());crtTerminalMenus_->setChecked(value("terminalMenus",true).toBool());crtGlow_->setChecked(value("glow",true).toBool());crtScanlines_->setValue(value("scanlines",35).toInt());
            layout->addWidget(crt);
            // The other experimental styles and layouts: one group of check boxes each, from InterfaceModes::options().
            for(const auto &mode:InterfaceModes::styles()+InterfaceModes::layouts()){if(mode.id=="hackerman-crt"||mode.id=="classic"||mode.status=="planned")continue;auto opts=InterfaceModes::options(mode.id);if(opts.isEmpty())continue;
                const bool isLayout=InterfaceModes::findLayout(mode.id)!=nullptr;
                auto *group=new QGroupBox(mode.name);group->setObjectName("settingsModeOptions_"+mode.id);group->setProperty(isLayout?"layoutId":"modeId",mode.id);auto *groupForm=new QFormLayout(group);
                for(const auto &option:opts){auto *box=new QCheckBox(option.label);box->setObjectName("settingsModeOption_"+mode.id+"_"+option.key);box->setChecked(settings.value("interface/"+mode.id+"/"+option.key,option.fallback).toBool());groupForm->addRow(box);modeOptionBoxes_.insert(mode.id+"/"+option.key,box);}
                layout->addWidget(group);}
            sideBySide_=new QSpinBox;sideBySide_->setObjectName("settingsSideBySideWidth");sideBySide_->setRange(360,2000);sideBySide_->setSingleStep(20);sideBySide_->setSuffix(" px");sideBySide_->setValue(settings.value("interface/sideBySide",640).toInt());
            form->addRow("Side by side from",sideBySide_);auto *sideHelp=new QLabel("Panels such as Structure, Find, Magic Bytes, Checksum and Compare Data stack their parts when narrower than this and place them side by side when wider.");sideHelp->setObjectName("settingsSideBySideHelp");sideHelp->setWordWrap(true);form->addRow(sideHelp);
            auto *note=new QLabel("Style (how menus, panels and buttons are drawn), panel layout and theme combine freely. Classic style with Classic layout is the 0.3.7 interface. Each panel layout keeps its own panel arrangement, so your Classic arrangement comes back unchanged. Nothing here changes files. Planned entries show what is coming; see docs/ui-modes/ROADMAP.md.");note->setWordWrap(true);layout->addWidget(note);layout->addStretch();
            auto sync=[this,summary,layoutSummary,page]{
                const auto *mode=InterfaceModes::find(interfaceMode_->currentData().toString());summary->setText(mode?mode->summary:QString());
                const auto *panels=InterfaceModes::findLayout(interfaceLayout_->currentData().toString());layoutSummary->setText(panels?panels->summary:QString());
                for(auto *group:page->findChildren<QGroupBox *>()){if(group->property("modeId").isValid())group->setVisible(group->property("modeId")==interfaceMode_->currentData());if(group->property("layoutId").isValid())group->setVisible(group->property("layoutId")==interfaceLayout_->currentData());}
            };
            connect(interfaceMode_,&QComboBox::currentIndexChanged,this,sync);connect(interfaceLayout_,&QComboBox::currentIndexChanged,this,sync);
            interfaceMode_->setCurrentIndex(qMax(0,interfaceMode_->findData(InterfaceModes::styleId(settings))));interfaceLayout_->setCurrentIndex(qMax(0,interfaceLayout_->findData(InterfaceModes::layoutId(settings))));sync();interfaceSync_=sync;
            interfaceModePage_=page;interfaceModeSection_=new QTreeWidgetItem(appearanceSection,{"Interface mode"});
        }
        auto *editorOverview=new QWidget;auto *overviewLayout=new QVBoxLayout(editorOverview);
        auto *overview=new QLabel("Select an editor on the left to configure its display preferences.");overview->setWordWrap(true);overviewLayout->addWidget(overview);overviewLayout->addStretch();pages->addWidget(editorOverview);
        auto *editor=new QWidget;auto *editorForm=new QFormLayout(editor);
        fontSize_=new QSpinBox;fontSize_->setObjectName("settingsEditorFontSize");fontSize_->setRange(8,24);fontSize_->setSuffix(" pt");
        alternating_=new QCheckBox("Alternate row backgrounds");alternating_->setObjectName("settingsAlternatingRows");
        editorForm->addRow("Viewer text size",fontSize_);editorForm->addRow(alternating_);
        auto *editorHelp=new QLabel("Ctrl + mouse wheel also changes the text size in the main hexadecimal viewer. These display preferences apply to hexadecimal bytes and decoded text in all documents. They do not modify file contents.");
        editorHelp->setWordWrap(true);editorForm->addRow(editorHelp);pages->addWidget(editor);aiPage_=new AiSettingsPage(settings);pages->addWidget(aiPage_);sections->setCurrentItem(generalSection);
        auto *performance=new QWidget;auto *performanceForm=new QFormLayout(performance);
        memorySlider_=new QSlider(Qt::Horizontal);memorySlider_->setObjectName("settingsAssetMemorySlider");memorySlider_->setRange(1,64);memorySlider_->setTickInterval(8);memorySlider_->setTickPosition(QSlider::TicksBelow);
        memoryValue_=new QSpinBox;memoryValue_->setObjectName("settingsAssetMemoryGiB");memoryValue_->setRange(1,64);memoryValue_->setSuffix(" GiB");auto *memoryRow=new QHBoxLayout;memoryRow->addWidget(memorySlider_,1);memoryRow->addWidget(memoryValue_);performanceForm->addRow("Scanning / geometry memory budget",memoryRow);
        connect(memorySlider_,&QSlider::valueChanged,memoryValue_,&QSpinBox::setValue);connect(memoryValue_,&QSpinBox::valueChanged,memorySlider_,&QSlider::setValue);memoryValue_->setValue(qBound(1,settings.value("performance/assetMemoryGiB",8).toInt(),64));
        auto *memoryHelp=new QLabel("Default: 8 GiB. Sets the estimated budget for scan input, geometry decoding, assembled scenes and preview batches. Texture and image libraries each receive one quarter of this budget; shared texture images count once. Memory is allocated only as needed. This is not a hard limit on total application RAM or GPU memory. Already loaded assets remain in memory; file validation and hardware limits still apply. Apply changes before starting a new scan or preview.");memoryHelp->setWordWrap(true);performanceForm->addRow(memoryHelp);
        workers_=new QSpinBox;workers_->setObjectName("settingsCpuWorkers");workers_->setRange(1,qMax(1,QThread::idealThreadCount()));workers_->setValue(qBound(1,settings.value("performance/cpuWorkers",qMax(1,QThread::idealThreadCount()-1)).toInt(),workers_->maximum()));performanceForm->addRow("Background CPU workers",workers_);
        auto *bench=new QPushButton("Detect Hardware / Benchmark CPU & RAM");bench->setObjectName("settingsHardwareBenchmark");performanceForm->addRow(bench);auto *summary=new QLabel("Measures a small synthetic workload. GPU memory is detected; GPU frame rate is not benchmarked. Recommendations remain optional.");summary->setObjectName("settingsHardwareSummary");summary->setWordWrap(true);performanceForm->addRow(summary);auto *recommend=new QPushButton("Use Recommended Values");recommend->setObjectName("settingsUseHardwareRecommendations");recommend->setEnabled(false);performanceForm->addRow(recommend);auto *watcher=new QFutureWatcher<QJsonObject>(this);
        connect(bench,&QPushButton::clicked,this,[bench,summary,watcher]{bench->setEnabled(false);summary->setText("Detecting hardware and measuring CPU / RAM...");watcher->setFuture(QtConcurrent::run([]{return HardwarePerformance::benchmark();}));});
        connect(watcher,&QFutureWatcher<QJsonObject>::finished,this,[bench,summary,recommend,watcher]{auto r=watcher->result();recommend->setProperty("recommendation",r);recommend->setEnabled(true);bench->setEnabled(true);summary->setText(QString("CPU: %1 logical processors | RAM: %2 GiB\nPreferred GPU: %3 | Device-local heap: %4 GiB%5\nSynthetic RAM copy: %6 GiB/s | Recommended: %7 workers, %8 GiB asset budget.\nApply recommendations to change settings. Dedicated GPU preference and decode reuse are automatic; GPU rendering throughput was not measured.").arg(r["logicalProcessors"].toInt()).arg(r["ramBytes"].toString().toDouble()/AssetMemory::GiB,0,'f',1).arg(r["gpu"].toString("Unavailable")).arg(r["deviceLocalBytes"].toString().toDouble()/AssetMemory::GiB,0,'f',1).arg(r["dedicatedGpu"].toBool()?"":" (may be shared RAM)").arg(r["ramCopyGiBPerSecond"].toDouble(),0,'f',1).arg(r["recommendedWorkers"].toInt()).arg(r["recommendedMemoryGiB"].toInt()));});
        connect(recommend,&QPushButton::clicked,this,[this,recommend]{auto r=recommend->property("recommendation").toJsonObject();workers_->setValue(r["recommendedWorkers"].toInt(1));memoryValue_->setValue(r["recommendedMemoryGiB"].toInt(8));});pages->addWidget(performance);
        auto *imports=new QWidget;auto *importsForm=new QFormLayout(imports);fm1FlipU_=new QCheckBox("Flip U for FM1 track UVs");fm1FlipU_->setObjectName("settingsFm1FlipU");fm1FlipU_->setChecked(settings.value("3d/import/fm1FlipU",false).toBool());importsForm->addRow(fm1FlipU_);fm1FlipV_=new QCheckBox("Flip V for FM1 track UVs");fm1FlipV_->setObjectName("settingsFm1FlipV");fm1FlipV_->setChecked(settings.value("3d/import/fm1FlipV",false).toBool());importsForm->addRow(fm1FlipV_);auto *importHelp=new QLabel("Independently flips horizontal U = 1 - U and vertical V = 1 - V for newly scanned FM1 track meshes. Texture pixels remain unchanged. Apply, remove previously loaded meshes, then scan and preview again to compare orientations.");importHelp->setWordWrap(true);importsForm->addRow(importHelp);pages->addWidget(imports);cameraPage_=new CameraNavigation::Page(settings);pages->addWidget(cameraPage_);interfaceModeSection_->setData(0,Qt::UserRole,pages->addWidget(interfaceModePage_));
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|QDialogButtonBox::RestoreDefaults);
        buttons->setObjectName("settingsButtons");layout->addWidget(buttons);
        theme_->setCurrentIndex(qMax(0,theme_->findData(settings.value("theme","default"))));

        language_->setCurrentIndex(qMax(0,language_->findData(settings.value("language","system"))));
        fontSize_->setValue(qBound(8,settings.value("editor/fontSize",10).toInt(),24));
        alternating_->setChecked(settings.value("editor/alternatingRows",true).toBool());
        connect(buttons,&QDialogButtonBox::accepted,this,[this]{if(apply())accept();});
        connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
        connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this]{apply();});
        connect(buttons->button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,this,[this]{
            cameraPage_->restore();fm1FlipU_->setChecked(false);fm1FlipV_->setChecked(false);aiPage_->defaults();themeEditor_->restoreDefaults();language_->setCurrentIndex(language_->findData("system"));detailedTooltips_->setChecked(false);interfaceMode_->setCurrentIndex(interfaceMode_->findData("classic"));interfaceLayout_->setCurrentIndex(interfaceLayout_->findData("classic"));sideBySide_->setValue(640);fontSize_->setValue(10);alternating_->setChecked(true);memoryValue_->setValue(8);workers_->setValue(qMax(1,QThread::idealThreadCount()-1));
        });
    }
private:
    bool apply(){
        QString cameraError;if(!cameraPage_->validate(cameraError)){QMessageBox::warning(this,"Camera controls",cameraError);return false;}
        QString aiError;if(!aiPage_->save(settings_,aiError)){QMessageBox::warning(this,"AI Assistance",aiError);return false;}cameraPage_->save(settings_,cameraError);themeEditor_->save(settings_);settings_.setValue("theme",theme_->currentData());settings_.setValue("language",language_->currentData());
        settings_.setValue("interface/detailedTooltips",detailedTooltips_->isChecked());settings_.setValue("interface/style",interfaceMode_->currentData());settings_.setValue("interface/mode",interfaceMode_->currentData());settings_.setValue("interface/layout",interfaceLayout_->currentData());settings_.setValue("interface/sideBySide",sideBySide_->value());settings_.setValue("interface/hackerman-crt/statusLine",crtStatusLine_->isChecked());settings_.setValue("interface/hackerman-crt/boxTitles",crtBoxTitles_->isChecked());settings_.setValue("interface/hackerman-crt/terminalMenus",crtTerminalMenus_->isChecked());settings_.setValue("interface/hackerman-crt/glow",crtGlow_->isChecked());settings_.setValue("interface/hackerman-crt/scanlines",crtScanlines_->value());for(auto it=modeOptionBoxes_.cbegin();it!=modeOptionBoxes_.cend();++it)settings_.setValue("interface/"+it.key(),it.value()->isChecked());settings_.setValue("editor/fontSize",fontSize_->value());settings_.setValue("editor/alternatingRows",alternating_->isChecked());
        settings_.setValue("performance/assetMemoryGiB",memoryValue_->value());settings_.setValue("performance/cpuWorkers",workers_->value());
        settings_.setValue("3d/import/fm1FlipU",fm1FlipU_->isChecked());
        settings_.setValue("3d/import/fm1FlipV",fm1FlipV_->isChecked());
        settings_.sync();
        if(settings_.status()!=QSettings::NoError){
            QMessageBox::warning(this,"Settings","The preferences could not be saved. Check that the settings location is writable.");return false;
        }
        loadWorkbenchPreferences(settings_);if(applied_)applied_();return true;
    }
    QSettings &settings_;std::function<void()> applied_;CameraNavigation::Page *cameraPage_;
    AiSettingsPage *aiPage_;ThemeEditor *themeEditor_;QComboBox *language_,*theme_;QSpinBox *fontSize_;QHash<QString,QCheckBox *> modeOptionBoxes_;QComboBox *interfaceMode_=nullptr,*interfaceLayout_=nullptr;QSpinBox *sideBySide_=nullptr;std::function<void()> interfaceSync_;QWidget *interfaceModePage_=nullptr;QTreeWidgetItem *interfaceModeSection_=nullptr;QCheckBox *crtStatusLine_=nullptr,*crtBoxTitles_=nullptr,*crtTerminalMenus_=nullptr,*crtGlow_=nullptr;QSlider *crtScanlines_=nullptr;QCheckBox *detailedTooltips_,*alternating_,*fm1FlipU_,*fm1FlipV_;
    QSlider *memorySlider_;QSpinBox *memoryValue_,*workers_;
};
