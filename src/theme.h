#pragma once
#include <QApplication>
#include <QWidget>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QStyle>
#include <QStyleHints>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <memory>
#include "theme_tokens.h"
#include "default_appearance.h"
#include "workbench_icons.h"

inline QColor themeColor(const QString &key,QColor fallback){auto colors=qApp->property("workbenchThemeColors").toMap();auto value=QColor(colors.value(key).toString());return value.isValid()?value:fallback;}
inline bool workbenchDark(){return qApp->palette().color(QPalette::Window).lightness()<128;}
inline QColor graphicsBackground(){return themeColor("graphics.background",workbenchDark()?QColor("#151B25"):QColor("#E7ECF3"));}
inline QColor graphicsGrid(){return themeColor("graphics.grid",workbenchDark()?QColor("#536174"):QColor("#A6B3C3"));}
inline QColor graphicsSelection(){return themeColor("graphics.selection",workbenchDark()?QColor("#F7C965"):QColor("#A65B00"));}
inline QPalette workbenchThemePalette(const QString &name,const QJsonObject &colors={}){
    const bool dark=name=="dark"||(name=="system"&&qApp->styleHints()->colorScheme()==Qt::ColorScheme::Dark);
    std::unique_ptr<QStyle> style(QStyleFactory::create("Fusion"));QPalette p=style->standardPalette();
    if(dark){
        p.setColor(QPalette::Window,QColor("#161B22"));p.setColor(QPalette::WindowText,QColor("#E6EAF0"));
        p.setColor(QPalette::Base,QColor("#12171E"));p.setColor(QPalette::AlternateBase,QColor("#1A2029"));
        p.setColor(QPalette::Text,QColor("#E6EAF0"));p.setColor(QPalette::Button,QColor("#1F2630"));
        p.setColor(QPalette::ButtonText,QColor("#E6EAF0"));p.setColor(QPalette::Highlight,QColor("#315C89"));
        p.setColor(QPalette::HighlightedText,Qt::white);p.setColor(QPalette::ToolTipBase,QColor("#252D38"));
        p.setColor(QPalette::ToolTipText,QColor("#F0F3F8"));p.setColor(QPalette::Mid,QColor("#2E3846"));
        p.setColor(QPalette::Link,QColor("#8EB8FF"));p.setColor(QPalette::Light,QColor("#2E3846"));
    }else{
        // Set every role: Fusion's standard palette can follow a dark system scheme.
        p.setColor(QPalette::Window,QColor("#EEF1F5"));p.setColor(QPalette::WindowText,QColor("#1B2430"));
        p.setColor(QPalette::Base,QColor("#FFFFFF"));p.setColor(QPalette::AlternateBase,QColor("#F3F6FA"));
        p.setColor(QPalette::Text,QColor("#1B2430"));p.setColor(QPalette::Button,QColor("#F8FAFC"));
        p.setColor(QPalette::ButtonText,QColor("#1B2430"));p.setColor(QPalette::Highlight,QColor("#C7DCF7"));
        p.setColor(QPalette::HighlightedText,QColor("#102B4A"));p.setColor(QPalette::ToolTipBase,QColor("#FFFFFF"));
        p.setColor(QPalette::ToolTipText,QColor("#1B2430"));p.setColor(QPalette::Mid,QColor("#CDD5DF"));
        p.setColor(QPalette::Link,QColor("#2457B8"));p.setColor(QPalette::Light,QColor("#FFFFFF"));
        p.setColor(QPalette::Midlight,QColor("#E3E8EF"));p.setColor(QPalette::Dark,QColor("#B8C2CE"));
    }
    for(auto role:{QPalette::WindowText,QPalette::Text,QPalette::ButtonText})p.setColor(QPalette::Disabled,role,dark?QColor("#8490A1"):QColor("#76808B"));
    p.setColor(QPalette::PlaceholderText,p.color(QPalette::Disabled,QPalette::Text));
    for(const auto &token:themeTokens())if(token.role>=0){QColor color(colors[token.key].toString());if(color.isValid()){p.setColor(QPalette::ColorRole(token.role),color);if(token.key=="text.primary"){p.setColor(QPalette::Text,color);p.setColor(QPalette::ButtonText,color);p.setColor(QPalette::ToolTipText,color);}}}
    QColor disabled(colors["text.disabled"].toString());if(!disabled.isValid())disabled=dark?QColor("#8490A1"):QColor("#76808B");for(auto role:{QPalette::Text,QPalette::WindowText,QPalette::ButtonText})p.setColor(QPalette::Disabled,role,disabled);
    QColor border(colors["border.color"].toString());if(border.isValid()){p.setColor(QPalette::Dark,border);p.setColor(QPalette::Light,border);p.setColor(QPalette::Midlight,border);}
    return p;
}
inline QColor resolvedThemeColor(const QString &key,const QPalette &p,const QJsonObject &colors={}){
    QColor custom(colors[key].toString());if(custom.isValid())return custom;
    for(auto &token:themeTokens())if(token.key==key&&token.role>=0)return p.color(QPalette::ColorRole(token.role));
    bool dark=p.color(QPalette::Window).lightness()<128;
    if(key=="text.disabled")return p.color(QPalette::Disabled,QPalette::Text);
    if(key=="accent.hover")return dark?p.color(QPalette::Button).lighter(125):p.color(QPalette::Button).darker(106);if(key=="accent.focus")return dark?QColor("#5B9BFF"):QColor("#2F6FDB");
    if(key=="accent.primary")return QColor("#2F6FDB");
    if(key=="hex.modified")return dark?QColor("#F5C87A"):QColor("#9B4F08");
    if(key=="graphics.background")return dark?QColor("#151B25"):QColor("#E7ECF3");if(key=="graphics.grid")return dark?QColor("#536174"):QColor("#A6B3C3");if(key=="graphics.selection")return dark?QColor("#F7C965"):QColor("#A65B00");
    if(key=="graphics.model")return QColor("#78B4E8");if(key=="graphics.vertices")return QColor("#FFD060");if(key=="graphics.wire")return dark?QColor("#93A8C1"):QColor("#33465E");
    if(key.endsWith(".background"))return p.color(QPalette::Base);
    if(key=="calculator.hex"||key=="calculator.bits")return p.color(QPalette::Link);
    return p.color(QPalette::Text);
}
inline void applyWorkbenchTheme(const QString &requested){
    auto library=QJsonDocument::fromJson(qApp->property("workbenchThemeLibrary").toByteArray()).object();auto theme=requested=="default"?standardAppearance():library[requested].toObject();QString name=theme.isEmpty()?requested:theme["base"].toString("dark");if(name!="dark"&&name!="light"&&name!="system")name="dark";
    auto colors=theme["colors"].toObject();auto options=theme["options"].toObject();qApp->setProperty("workbenchThemeColors",colors.toVariantMap());qApp->setProperty("workbenchThemeBase",name);
    WorkbenchIconDecorator::install();
    qApp->setStyle(QStyleFactory::create("Fusion"));auto p=workbenchThemePalette(name,colors);qApp->setPalette(p);
    auto color=[&](QString key,QColor fallback){return themeColor(key,fallback).name();};
    auto token=[&](const QString &key){return resolvedThemeColor(key,p,colors).name();};
    const bool darkBase=p.color(QPalette::Window).lightness()<128;
    const QString text=p.color(QPalette::Text).name(),muted=p.color(QPalette::Disabled,QPalette::Text).name(),window=p.color(QPalette::Window).name();
    const QString base=p.color(QPalette::Base).name(),button=p.color(QPalette::Button).name(),border=p.color(QPalette::Mid).name();
    const QString hover=token("accent.hover"),focus=token("accent.focus"),primary=token("accent.primary");
    const QString primaryHover=QColor(primary).lighter(112).name(),primaryPressed=QColor(primary).darker(115).name();
    const QString pressed=darkBase?QColor(button).lighter(140).name():QColor(button).darker(112).name();
    const QString handle=darkBase?QColor(border).lighter(130).name():QColor(border).darker(110).name();
    // Main actions of each panel use the accent; [primary="true"] marks new ones.
    QStringList primaryList{"QPushButton[primary=\"true\"]"};for(const auto &name:workbenchPrimaryButtons())primaryList<<"QPushButton#"+name;const QString primaries=primaryList.join(", ");
    auto states=[&](const QString &state){auto list=primaries.split(", ");for(auto &item:list)item+=state;return list.join(", ");};
    QString css="QToolBar { spacing: 4px; padding: 3px; border: none; }";
    css+="QPushButton { background: "+button+"; color: "+text+"; border: 1px solid "+border+"; border-radius: 6px; padding: 5px 12px; min-height: 18px; }";
    css+="QPushButton:hover { background: "+hover+"; } QPushButton:pressed, QPushButton:checked { background: "+pressed+"; }";
    css+="QPushButton:focus { border-color: "+focus+"; } QPushButton:disabled { color: "+muted+"; background: transparent; }";
    css+=primaries+" { background: "+primary+"; color: #FFFFFF; border: 1px solid "+primary+"; font-weight: 600; }";
    css+=states(":hover")+" { background: "+primaryHover+"; border-color: "+primaryHover+"; }";
    css+=states(":pressed")+" { background: "+primaryPressed+"; }";
    css+=states(":focus")+" { border-color: "+focus+"; }";
    css+=states(":disabled")+" { background: transparent; color: "+muted+"; border-color: "+border+"; font-weight: 400; }";
    css+="QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 3px 6px; }";
    css+="QToolButton:hover { background: "+hover+"; border-color: "+border+"; } QToolButton:pressed, QToolButton:checked { background: "+pressed+"; }";
    css+="QLineEdit { background: "+base+"; border: 1px solid "+border+"; border-radius: 6px; padding: 4px 8px; }";
    // Line edits inside combo and spin boxes keep the box's own frame.
    css+="QComboBox QLineEdit, QAbstractSpinBox QLineEdit { border: none; border-radius: 0; padding: 0; background: transparent; }";
    css+="QTabBar::tab { background: transparent; color: "+muted+"; border: none; border-bottom: 2px solid transparent; padding: 6px 12px; }";
    css+="QTabBar::tab:hover { color: "+text+"; background: "+hover+"; } QTabBar::tab:selected { color: "+text+"; border-bottom: 2px solid "+primary+"; }";
    // Tabified panels scroll instead of truncating their titles.
    css+="QMainWindow > QTabBar { qproperty-elideMode: 3; qproperty-usesScrollButtons: 1; }";
    css+="QTabWidget::pane { border: 1px solid "+border+"; border-radius: 8px; top: -1px; }";
    css+="QDockWidget::title { background: "+window+"; padding: 7px 8px; border-bottom: 1px solid "+border+"; }";
    css+="QHeaderView::section { background: "+button+"; color: "+muted+"; border: none; border-bottom: 1px solid "+border+"; border-right: 1px solid "+border+"; padding: 4px 6px; }";
    css+="QScrollBar:vertical { background: transparent; width: 11px; margin: 0; } QScrollBar:horizontal { background: transparent; height: 11px; margin: 0; }";
    css+="QScrollBar::handle:vertical { background: "+handle+"; border-radius: 4px; min-height: 28px; margin: 2px; } QScrollBar::handle:horizontal { background: "+handle+"; border-radius: 4px; min-width: 28px; margin: 2px; }";
    css+="QScrollBar::handle:hover { background: "+muted+"; }";
    css+="QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; border: none; background: none; } QScrollBar::add-page, QScrollBar::sub-page { background: none; }";
    css+="QMenu { background: "+base+"; border: 1px solid "+border+"; border-radius: 8px; padding: 4px; } QMenu::item { padding: 6px 24px 6px 12px; border-radius: 5px; }";
    css+="QMenu::item:selected { background: "+p.color(QPalette::Highlight).name()+"; color: "+p.color(QPalette::HighlightedText).name()+"; } QMenu::separator { height: 1px; background: "+border+"; margin: 4px 8px; }";
    css+="QToolTip { background: "+p.color(QPalette::ToolTipBase).name()+"; color: "+p.color(QPalette::ToolTipText).name()+"; border: 1px solid "+border+"; border-radius: 6px; padding: 6px 8px; }";
    // 3D module: toolbar and tool column as rounded bars, segmented render styles, section cards.
    const QString surface=darkBase?QColor(window).lighter(118).name():base;
    css+="QWidget#viewportToolbar, QWidget#viewportTransformBar { background: "+surface+"; border: 1px solid "+border+"; border-radius: 10px; }";
    css+="QFrame#viewportToolbarSeparator { background: "+border+"; border: none; }";
    css+="QToolButton[segment=\"true\"] { padding: 4px 8px; color: "+muted+"; } QToolButton[segment=\"true\"]:hover { color: "+text+"; }";
    css+="QToolButton[segment=\"true\"]:checked { background: "+primary+"; color: #FFFFFF; font-weight: 600; border-color: "+primary+"; }";
    css+="QWidget#viewportTransformBar QToolButton:checked { background: "+p.color(QPalette::Highlight).name()+"; border-color: "+focus+"; }";
    css+="QLabel#vulkanStatus { background: "+surface+"; border: 1px solid "+border+"; border-radius: 10px; padding: 4px 10px; color: "+muted+"; }";
    css+="QFrame#meshSectionCard { background: "+surface+"; border: 1px solid "+border+"; border-radius: 9px; }";
    css+="QPushButton[compact=\"true\"] { padding: 2px 8px; min-height: 14px; font-size: 11px; }";
    css+="QLabel#meshSectionBadge, QLabel#meshCountBadge { background: "+button+"; color: "+muted+"; border: 1px solid "+border+"; border-radius: 7px; padding: 1px 8px; font-size: 11px; }";
    css+="QTreeWidget#meshCollection { border: 1px solid "+border+"; border-radius: 8px; } QTreeWidget#meshCollection::item { padding: 5px 2px; }";
    css+="QLabel#meshBoundsDimensions, QLabel#meshPlacementNote, QLabel#meshStatus, QLabel#meshSetupState { color: "+muted+"; font-size: 11px; }";
    css+="QLabel#axisTagX { color: #FF7A7A; font-weight: 700; } QLabel#axisTagY { color: "+QString(darkBase?"#6BE09A":"#1E8F4E")+"; font-weight: 700; } QLabel#axisTagZ { color: "+QString(darkBase?"#6AA6FF":"#2457B8")+"; font-weight: 700; }";
    // Module rail, command palette, segmented inspector tabs and the status bar.
    const QString rail=darkBase?QColor(window).darker(125).name():QColor(window).darker(104).name();
    css+="QWidget#moduleRail { background: "+rail+"; border-right: 1px solid "+border+"; }";
    css+="QLabel#railLogo { background: #F5C451; color: #1A1300; border-radius: 9px; font-weight: 700; font-size: 12px; }";
    css+="QWidget#moduleRail QToolButton { border-radius: 10px; } QWidget#moduleRail QToolButton:hover { background: "+hover+"; border-color: transparent; }";
    css+="QDialog#commandPalette { background: "+base+"; border: 1px solid "+border+"; border-radius: 12px; }";
    css+="QListWidget#commandPaletteList { border: none; background: transparent; } QListWidget#commandPaletteList::item { padding: 7px 8px; border-radius: 6px; }";
    css+="QListWidget#commandPaletteList::item:selected { background: "+p.color(QPalette::Highlight).name()+"; color: "+p.color(QPalette::HighlightedText).name()+"; }";
    css+="QLabel#commandPaletteEmpty { color: "+muted+"; padding: 24px; }";
    css+="QTabWidget#meshInspectorTabs QTabBar::tab { border: 1px solid transparent; border-radius: 7px; padding: 5px 12px; margin: 2px; }";
    css+="QTabWidget#meshInspectorTabs QTabBar::tab:selected { background: "+button+"; border-color: "+border+"; color: "+text+"; }";
    css+="QStatusBar { color: "+muted+"; border-top: 1px solid "+border+"; } QStatusBar QLabel { color: "+muted+"; }";
    css+="QToolButton#threadActivityToggle { border: 1px solid "+border+"; border-radius: 9px; padding: 1px 9px; color: "+muted+"; }";
    css+="QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid "+color("accent.focus",QColor(focus))+"; }";
    if(!options.value("fieldBackground").toBool(true))css+="QLineEdit, QSpinBox, QDoubleSpinBox { background: transparent; }";
    if(!options.value("borders").toBool(true))css+="QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QGroupBox { border: none; }";
    for(auto pair:{std::pair{"scriptEditor","script"}, {"scriptConsole","console"}})css+=QString("QPlainTextEdit#%1 { background: %2; color: %3; }").arg(pair.first,color(QString(pair.second)+".background",p.color(QPalette::Base)),color(QString(pair.second)+".text",p.color(QPalette::Text)));
    qApp->setStyleSheet(css);qApp->setProperty("workbenchTheme",theme.isEmpty()?name:requested);
    // Tool colors can change while the base palette stays identical.
    qApp->paletteChanged(p);for(auto *widget:QApplication::allWidgets())widget->update();
}
