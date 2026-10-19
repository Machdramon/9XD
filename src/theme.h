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

inline QColor themeColor(const QString &key,QColor fallback){auto colors=qApp->property("workbenchThemeColors").toMap();auto value=QColor(colors.value(key).toString());return value.isValid()?value:fallback;}
inline bool workbenchDark(){return qApp->palette().color(QPalette::Window).lightness()<128;}
inline QColor graphicsBackground(){return themeColor("graphics.background",workbenchDark()?QColor("#151B25"):QColor("#E7ECF3"));}
inline QColor graphicsGrid(){return themeColor("graphics.grid",workbenchDark()?QColor("#536174"):QColor("#A6B3C3"));}
inline QColor graphicsSelection(){return themeColor("graphics.selection",workbenchDark()?QColor("#F7C965"):QColor("#A65B00"));}
inline QPalette workbenchThemePalette(const QString &name,const QJsonObject &colors={}){
    const bool dark=name=="dark"||(name=="system"&&qApp->styleHints()->colorScheme()==Qt::ColorScheme::Dark);
    std::unique_ptr<QStyle> style(QStyleFactory::create("Fusion"));QPalette p=style->standardPalette();
    if(dark){
        p.setColor(QPalette::Window,QColor("#202630"));p.setColor(QPalette::WindowText,QColor("#E0E6EF"));
        p.setColor(QPalette::Base,QColor("#171C24"));p.setColor(QPalette::AlternateBase,QColor("#1C222C"));
        p.setColor(QPalette::Text,QColor("#E0E6EF"));p.setColor(QPalette::Button,QColor("#29313D"));
        p.setColor(QPalette::ButtonText,QColor("#E0E6EF"));p.setColor(QPalette::Highlight,QColor("#315C89"));
        p.setColor(QPalette::HighlightedText,Qt::white);p.setColor(QPalette::ToolTipBase,QColor("#303947"));
        p.setColor(QPalette::ToolTipText,QColor("#F0F3F8"));p.setColor(QPalette::Mid,QColor("#465165"));
        p.setColor(QPalette::Link,QColor("#8DBBFF"));p.setColor(QPalette::Light,QColor("#394455"));
    }else{p.setColor(QPalette::Base,QColor("#FCFDFE"));p.setColor(QPalette::AlternateBase,QColor("#F0F4F8"));p.setColor(QPalette::Highlight,QColor("#C7DCF7"));p.setColor(QPalette::HighlightedText,QColor("#102B4A"));}
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
    if(key=="accent.hover")return p.color(QPalette::Button).lighter(115);if(key=="accent.focus")return p.color(QPalette::Highlight);
    if(key=="hex.modified")return dark?QColor("#F5C87A"):QColor("#9B4F08");
    if(key=="graphics.background")return dark?QColor("#151B25"):QColor("#E7ECF3");if(key=="graphics.grid")return dark?QColor("#536174"):QColor("#A6B3C3");if(key=="graphics.selection")return dark?QColor("#F7C965"):QColor("#A65B00");
    if(key=="graphics.model")return QColor("#78B4E8");if(key=="graphics.vertices")return QColor("#FFD060");if(key=="graphics.wire")return dark?QColor("#93A8C1"):QColor("#33465E");
    if(key.endsWith(".background"))return p.color(QPalette::Base);
    if(key=="calculator.hex"||key=="calculator.bits")return p.color(QPalette::Link);
    return p.color(QPalette::Text);
}
inline void applyWorkbenchTheme(const QString &requested){
    auto library=QJsonDocument::fromJson(qApp->property("workbenchThemeLibrary").toByteArray()).object();auto theme=library[requested].toObject();QString name=theme.isEmpty()?requested:theme["base"].toString("dark");if(name!="dark"&&name!="light"&&name!="system")name="dark";
    auto colors=theme["colors"].toObject();auto options=theme["options"].toObject();qApp->setProperty("workbenchThemeColors",colors.toVariantMap());qApp->setProperty("workbenchThemeBase",name);
    qApp->setStyle(QStyleFactory::create("Fusion"));auto p=workbenchThemePalette(name,colors);qApp->setPalette(p);
    QString css="QToolBar { spacing: 4px; padding: 2px; } QTabBar::tab { padding: 5px 12px; } QDockWidget::title { padding: 4px; } QHeaderView::section { padding: 3px; } QToolTip { padding: 5px; }";
    auto color=[&](QString key,QColor fallback){return themeColor(key,fallback).name();};
    css+="QPushButton:hover, QToolButton:hover, QTabBar::tab:hover { background: "+color("accent.hover",p.color(QPalette::Button).lighter(115))+"; }";
    css+="QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid "+color("accent.focus",p.color(QPalette::Highlight))+"; }";
    if(!options.value("fieldBackground").toBool(true))css+="QLineEdit, QSpinBox, QDoubleSpinBox { background: transparent; }";
    if(!options.value("borders").toBool(true))css+="QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QGroupBox { border: none; }";
    for(auto pair:{std::pair{"scriptEditor","script"}, {"scriptConsole","console"}})css+=QString("QPlainTextEdit#%1 { background: %2; color: %3; }").arg(pair.first,color(QString(pair.second)+".background",p.color(QPalette::Base)),color(QString(pair.second)+".text",p.color(QPalette::Text)));
    qApp->setStyleSheet(css);qApp->setProperty("workbenchTheme",theme.isEmpty()?name:requested);
    // Tool colors can change while the base palette stays identical.
    qApp->paletteChanged(p);for(auto *widget:QApplication::allWidgets())widget->update();
}
