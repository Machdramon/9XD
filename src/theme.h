#pragma once
#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QStyle>
#include <QStyleHints>

inline bool workbenchDark(){return qApp->palette().color(QPalette::Window).lightness()<128;}
inline QColor graphicsBackground(){return workbenchDark()?QColor("#151B25"):QColor("#E7ECF3");}
inline QColor graphicsGrid(){return workbenchDark()?QColor("#536174"):QColor("#A6B3C3");}
inline QColor graphicsSelection(){return workbenchDark()?QColor("#F7C965"):QColor("#A65B00");}
inline void applyWorkbenchTheme(const QString &name){
    qApp->setStyle(QStyleFactory::create("Fusion"));
    const bool dark=name=="dark"||(name=="system"&&qApp->styleHints()->colorScheme()==Qt::ColorScheme::Dark);
    QPalette p=qApp->style()->standardPalette();
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
    qApp->setPalette(p);
    qApp->setStyleSheet("QToolBar { spacing: 4px; padding: 2px; } QTabBar::tab { padding: 5px 12px; } QDockWidget::title { padding: 4px; } QHeaderView::section { padding: 3px; } QToolTip { padding: 5px; }");
    qApp->setProperty("workbenchTheme",name);
}
