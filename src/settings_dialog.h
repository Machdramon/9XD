#pragma once
#include "theme.h"
#include "theme_editor.h"
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
    QJsonObject themes;QString error;auto saved=QJsonDocument::fromJson(settings.value("themes/v1").toByteArray()).object();for(auto it=saved.begin();it!=saved.end();++it)if(it.key().startsWith("custom:")&&validateTheme(it.value().toObject(),error))themes[it.key()]=it.value();qApp->setProperty("workbenchThemeLibrary",QJsonDocument(themes).toJson(QJsonDocument::Compact));
    applyWorkbenchTheme(settings.value("theme","dark").toString());
    qApp->setProperty("workbenchLanguage",settings.value("language","system").toString());
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
        auto *pages=new QStackedWidget;pages->setObjectName("settingsPages");body->addWidget(sections);body->addWidget(pages,1);
        connect(sections,&QTreeWidget::currentItemChanged,pages,[pages](QTreeWidgetItem *item){if(item)pages->setCurrentIndex(item->data(0,Qt::UserRole).toInt());});
        auto *general=new QWidget;auto *generalForm=new QFormLayout(general);
        language_=new QComboBox;language_->setObjectName("settingsLanguage");
        language_->addItem("System default (English)","system");language_->addItem("English","en");
        generalForm->addRow("Language",language_);
        auto *languageHelp=new QLabel("English is currently the available interface language. Additional languages will appear here when translation packs are added.");
        languageHelp->setWordWrap(true);generalForm->addRow(languageHelp);pages->addWidget(general);
        auto *appearance=new QWidget;auto *appearanceLayout=new QVBoxLayout(appearance);theme_=new QComboBox;theme_->setObjectName("settingsTheme");theme_->addItem("Dark","dark");theme_->addItem("Light","light");theme_->addItem("System","system");themeEditor_=new ThemeEditor(theme_,settings);appearanceLayout->addWidget(themeEditor_);pages->addWidget(appearance);
        auto *editorOverview=new QWidget;auto *overviewLayout=new QVBoxLayout(editorOverview);
        auto *overview=new QLabel("Select an editor on the left to configure its display preferences.");overview->setWordWrap(true);overviewLayout->addWidget(overview);overviewLayout->addStretch();pages->addWidget(editorOverview);
        auto *editor=new QWidget;auto *editorForm=new QFormLayout(editor);
        fontSize_=new QSpinBox;fontSize_->setObjectName("settingsEditorFontSize");fontSize_->setRange(8,24);fontSize_->setSuffix(" pt");
        alternating_=new QCheckBox("Alternate row backgrounds");alternating_->setObjectName("settingsAlternatingRows");
        editorForm->addRow("Viewer text size",fontSize_);editorForm->addRow(alternating_);
        auto *editorHelp=new QLabel("Ctrl + mouse wheel also changes the text size in the main hexadecimal viewer. These display preferences apply to hexadecimal bytes and decoded text in all documents. They do not modify file contents.");
        editorHelp->setWordWrap(true);editorForm->addRow(editorHelp);pages->addWidget(editor);sections->setCurrentItem(generalSection);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|QDialogButtonBox::RestoreDefaults);
        buttons->setObjectName("settingsButtons");layout->addWidget(buttons);
        theme_->setCurrentIndex(qMax(0,theme_->findData(settings.value("theme","dark"))));
        language_->setCurrentIndex(qMax(0,language_->findData(settings.value("language","system"))));
        fontSize_->setValue(qBound(8,settings.value("editor/fontSize",10).toInt(),24));
        alternating_->setChecked(settings.value("editor/alternatingRows",true).toBool());
        connect(buttons,&QDialogButtonBox::accepted,this,[this]{if(apply())accept();});
        connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
        connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this]{apply();});
        connect(buttons->button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,this,[this]{
            themeEditor_->restoreDefaults();language_->setCurrentIndex(language_->findData("system"));fontSize_->setValue(10);alternating_->setChecked(true);
        });
    }
private:
    bool apply(){
        themeEditor_->save(settings_);settings_.setValue("theme",theme_->currentData());settings_.setValue("language",language_->currentData());
        settings_.setValue("editor/fontSize",fontSize_->value());settings_.setValue("editor/alternatingRows",alternating_->isChecked());
        settings_.sync();
        if(settings_.status()!=QSettings::NoError){
            QMessageBox::warning(this,"Settings","The preferences could not be saved. Check that the settings location is writable.");return false;
        }
        loadWorkbenchPreferences(settings_);if(applied_)applied_();return true;
    }
    QSettings &settings_;std::function<void()> applied_;
    ThemeEditor *themeEditor_;QComboBox *language_,*theme_;QSpinBox *fontSize_;QCheckBox *alternating_;
};
