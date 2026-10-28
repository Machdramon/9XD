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
#include <cmath>
#include <QProxyStyle>
#include <QFontDatabase>
#include <QPainter>
#include <QImage>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QPainterPath>
#include <QStyleOption>
#include "theme_tokens.h"
#include "default_appearance.h"
#include "included_themes.h"
#include "dock_title_hover.h"
#include "tooltip_policy.h"
#include "workbench_icons.h"

inline QColor themeColor(const QString &key,QColor fallback){auto colors=qApp->property("workbenchThemeColors").toMap();auto value=QColor(colors.value(key).toString());return value.isValid()?value:fallback;}
inline bool workbenchDark(){return qApp->palette().color(QPalette::Window).lightness()<128;}
// 3D / UV background: the theme's "graphics.background", or a shade of its window colour so the views match the theme.
inline QColor graphicsBackground(){const QColor window=qApp->palette().color(QPalette::Window);return themeColor("graphics.background",workbenchDark()?window.darker(118):window.darker(104));}
// Themes with a window gradient ("background.gradient") carry it into the 3D / UV views: window colour to that colour.
inline bool graphicsGradient(QColor &top,QColor &bottom){const QColor end=themeColor("background.gradient",QColor());if(!end.isValid())return false;top=qApp->palette().color(QPalette::Window);bottom=end;return true;}
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
    // Interface style colours (interface_modes.h): derived from the palette unless the theme sets them.
    if(key=="style.bar")return dark?p.color(QPalette::Window).lighter(125):p.color(QPalette::Window).darker(104);
    if(key=="style.glass")return dark?p.color(QPalette::Window):p.color(QPalette::Window).darker(108);
    if(key=="style.panelTitle"||key=="style.progress")return resolvedThemeColor("accent.primary",p,colors);
    if(key=="style.neon")return resolvedThemeColor("accent.focus",p,colors);
    if(key=="background.gradient")return p.color(QPalette::Window);
    if(key=="hex.modified")return dark?QColor("#F5C87A"):QColor("#9B4F08");
    if(key=="graphics.background")return dark?QColor("#151B25"):QColor("#E7ECF3");if(key=="graphics.grid")return dark?QColor("#536174"):QColor("#A6B3C3");if(key=="graphics.selection")return dark?QColor("#F7C965"):QColor("#A65B00");
    if(key=="graphics.model")return QColor("#78B4E8");if(key=="graphics.vertices")return QColor("#FFD060");if(key=="graphics.wire")return dark?QColor("#93A8C1"):QColor("#33465E");
    if(key.endsWith(".background"))return p.color(QPalette::Base);
    if(key=="calculator.hex"||key=="calculator.bits")return p.color(QPalette::Link);
    return p.color(QPalette::Text);
}
// Fusion derives check box / radio outlines from a darkened Window colour, which vanishes on near-black themes.
// This proxy draws both indicators from theme roles instead: secondary text for the outline, the primary accent when checked.
inline double themeLuminance(QColor c){auto f=[](double v){return v<=.03928?v/12.92:std::pow((v+.055)/1.055,2.4);};return .2126*f(c.redF())+.7152*f(c.greenF())+.0722*f(c.blueF());}
inline QColor readableOn(QColor fill,QColor dark){const double l=themeLuminance(fill);return 1.05/(l+.05)>=(l+.05)/(themeLuminance(dark)+.05)?QColor(Qt::white):dark;}
class WorkbenchStyle final : public QProxyStyle {
public:
    WorkbenchStyle():QProxyStyle(QStyleFactory::create("Fusion")){}
    void drawPrimitive(PrimitiveElement element,const QStyleOption *option,QPainter *painter,const QWidget *widget)const override{
        if(element!=PE_IndicatorCheckBox&&element!=PE_IndicatorRadioButton){QProxyStyle::drawPrimitive(element,option,painter,widget);return;}
        const auto &pal=option->palette;const bool enabled=option->state&State_Enabled,on=option->state&State_On,partial=option->state&State_NoChange,hover=enabled&&(option->state&State_MouseOver);
        const QColor accent=themeColor("accent.primary",QColor("#2F6FDB")),focus=themeColor("accent.focus",accent);
        QColor outline=pal.color(QPalette::PlaceholderText);if(hover)outline=focus;if(on||partial)outline=accent;
        const int side=qMin(option->rect.width(),option->rect.height())-2;if(side<4){QProxyStyle::drawPrimitive(element,option,painter,widget);return;}
        QRectF box(option->rect.x()+(option->rect.width()-side)/2.0+.5,option->rect.y()+(option->rect.height()-side)/2.0+.5,side-1,side-1);
        painter->save();painter->setRenderHint(QPainter::Antialiasing);if(!enabled)painter->setOpacity(.45);painter->setPen(QPen(outline,1.2));
        if(element==PE_IndicatorRadioButton){painter->setBrush(pal.color(QPalette::Base));painter->drawEllipse(box);if(on){painter->setPen(Qt::NoPen);painter->setBrush(accent);painter->drawEllipse(box.adjusted(side*.25,side*.25,-side*.25,-side*.25));}painter->restore();return;}
        painter->setBrush(on||partial?accent:pal.color(QPalette::Base));painter->drawRoundedRect(box,3,3);
        QPen mark(readableOn(accent,pal.color(QPalette::Window)),qMax(1.6,side/7.0),Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);painter->setPen(mark);painter->setBrush(Qt::NoBrush);
        if(partial)painter->drawLine(QPointF(box.left()+side*.25,box.center().y()),QPointF(box.right()-side*.25,box.center().y()));
        else if(on){QPainterPath check;check.moveTo(box.left()+side*.22,box.top()+side*.52);check.lineTo(box.left()+side*.42,box.top()+side*.72);check.lineTo(box.left()+side*.78,box.top()+side*.30);painter->drawPath(check);}
        painter->restore();
    }
    // Interface-mode glow (Hackerman CRT): a faint halo behind labels, buttons, menus and tabs. Item views draw their own text.
    void drawItemText(QPainter *painter,const QRect &rect,int flags,const QPalette &pal,bool enabled,const QString &text,QPalette::ColorRole textRole=QPalette::NoRole)const override{
        // Glow: the theme's choice (options.glow: none, soft, strong) or, by default, the style's (soft when on). Text
        // always glows in its own colour; neon is for panel lines only (options.neon).
        QString glow=qApp->property("workbenchGlow").toString();if(glow.isEmpty()||glow=="style")glow=qApp->property("interfaceChrome").toMap().value("glow").toBool()?"soft":"none";
        if(enabled&&!text.isEmpty()&&glow!="none"){
            QColor halo=textRole==QPalette::NoRole?painter->pen().color():pal.color(textRole);
            const QList<QPoint> near{QPoint(1,0),QPoint(-1,0),QPoint(0,1),QPoint(0,-1)},corners{QPoint(1,1),QPoint(-1,1),QPoint(1,-1),QPoint(-1,-1)},far{QPoint(2,0),QPoint(-2,0),QPoint(0,2),QPoint(0,-2)};
            painter->save();auto pass=[&](const QList<QPoint> &offsets,int alpha){halo.setAlpha(alpha);painter->setPen(halo);for(auto offset:offsets)painter->drawText(rect.translated(offset),flags,text);};
            if(glow=="soft")pass(near,60);else{pass(far,30);pass(corners,60);pass(near,85);}
            painter->restore();
        }
        QProxyStyle::drawItemText(painter,rect,flags,pal,enabled,text,textRole);
    }
};
inline void applyWorkbenchTheme(const QString &requested){
    auto library=QJsonDocument::fromJson(qApp->property("workbenchThemeLibrary").toByteArray()).object();auto theme=requested=="default"?standardAppearance():includedThemes().contains(requested)?includedThemes()[requested].toObject():library[requested].toObject();QString name=theme.isEmpty()?requested:theme["base"].toString("dark");if(name!="dark"&&name!="light"&&name!="system")name="dark";
    auto colors=theme["colors"].toObject();auto options=theme["options"].toObject();qApp->setProperty("workbenchThemeColors",colors.toVariantMap());qApp->setProperty("workbenchThemeBase",name);
    WorkbenchIconDecorator::install();DockTitleHover::install();TooltipPolicy::install();
    qApp->setStyle(new WorkbenchStyle);auto p=workbenchThemePalette(name,colors);qApp->setPalette(p);
    {// options.monospace: terminal font for the whole interface (Hackerman). Other themes restore the original UI font.
        static const QFont original=QApplication::font();QFont font=original;
        if(options.value("monospace").toBool()){const QStringList families{"Cascadia Mono","Consolas","Lucida Console","DejaVu Sans Mono","Menlo"};const auto installed=QFontDatabase::families();font=QFontDatabase::systemFont(QFontDatabase::FixedFont);for(const auto &family:families)if(installed.contains(family)){font=QFont(family);break;}font.setStyleHint(QFont::TypeWriter);if(original.pointSizeF()>0)font.setPointSizeF(original.pointSizeF());}
        if(QApplication::font()!=font)QApplication::setFont(font);
    }
    auto color=[&](QString key,QColor fallback){return themeColor(key,fallback).name();};
    auto token=[&](const QString &key){return resolvedThemeColor(key,p,colors).name();};
    const bool darkBase=p.color(QPalette::Window).lightness()<128;
    const QString text=p.color(QPalette::Text).name(),muted=p.color(QPalette::Disabled,QPalette::Text).name(),window=p.color(QPalette::Window).name();
    const QString base=p.color(QPalette::Base).name(),button=p.color(QPalette::Button).name(),border=p.color(QPalette::Mid).name();
    const QString hover=token("accent.hover"),focus=token("accent.focus"),primary=token("accent.primary");
    // Text on accent-filled controls: white or the window colour, whichever reads better on this accent (olive needs dark text).
    const QString onPrimary=readableOn(QColor(primary),darkBase?QColor(window):QColor(text)).name();qApp->setProperty("workbenchOnPrimary",onPrimary);
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
    css+=primaries+" { background: "+primary+"; color: "+onPrimary+"; border: 1px solid "+primary+"; font-weight: 600; }";
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
    css+="QToolButton[segment=\"true\"]:checked { background: "+primary+"; color: "+onPrimary+"; font-weight: 600; border-color: "+primary+"; }";
    css+="QWidget#viewportTransformBar QToolButton:checked { background: "+p.color(QPalette::Highlight).name()+"; border-color: "+focus+"; }";
    css+="QFrame#viewportCameraPanel, QFrame#viewportGridPanel { background: "+base+"; border: 1px solid "+border+"; }";
    // Transform / Material card in the 3D viewport corner (a native child window: opaque, square corners).
    css+="QFrame#viewportInspectorCard { background: "+surface+"; border: 1px solid "+border+"; border-radius: 0px; } QToolButton#viewportInspectorToggle { font-weight: 600; color: "+text+"; padding: 2px 4px; }";
    css+="QLabel#vulkanStatus { background: "+surface+"; border: 1px solid "+border+"; border-radius: 10px; padding: 4px 10px; color: "+muted+"; }";
    css+="QFrame#meshSectionCard { background: "+surface+"; border: 1px solid "+border+"; border-radius: 9px; }";
    css+="QPushButton[compact=\"true\"] { padding: 2px 8px; min-height: 14px; font-size: 11px; }";
    css+="QLabel#meshSectionBadge, QLabel#meshCountBadge { background: "+button+"; color: "+muted+"; border: 1px solid "+border+"; border-radius: 7px; padding: 1px 8px; font-size: 11px; }";
    css+="QTreeWidget#meshCollection { border: 1px solid "+border+"; border-radius: 8px; } QTreeWidget#meshCollection::item { padding: 5px 2px; }";
    css+="QLabel#meshBoundsDimensions, QLabel#meshPlacementNote, QLabel#meshStatus, QLabel#meshSetupState { color: "+muted+"; font-size: 11px; }";
    css+="QLabel#axisTagX { color: #FF7A7A; font-weight: 700; } QLabel#axisTagY { color: "+QString(darkBase?"#6BE09A":"#1E8F4E")+"; font-weight: 700; } QLabel#axisTagZ { color: "+QString(darkBase?"#6AA6FF":"#2457B8")+"; font-weight: 700; }";
    // Reconstruction inspectors: section cards, pipeline steps, state-coloured status lines, drawing canvas.
    css+="QFrame#inspectorCard { background: "+surface+"; border: 1px solid "+border+"; border-radius: 9px; } QLabel#inspectorCardTitle { font-weight: 600; }";
    css+="QLabel#inspectorHint, QLabel#reconstructionVideoInfo, QLabel#reconstructionEstimate, QLabel#reconstructionResult, QLabel#reconstructionToolState, QLabel#splineImageInfo, QLabel#splinePointCount, QLabel#splineProject { color: "+muted+"; font-size: 11px; }";
    css+="QWidget#reconstructionPipeline { background: "+surface+"; border: 1px solid "+border+"; border-radius: 10px; } QLabel#pipelineArrow { color: "+muted+"; }";
    css+="QLabel#pipelineStep { padding: 3px 9px; border-radius: 7px; color: "+muted+"; } QLabel#pipelineStep[state=\"current\"] { background: "+primary+"; color: "+onPrimary+"; font-weight: 600; } QLabel#pipelineStep[state=\"done\"] { color: "+text+"; font-weight: 600; } QLabel#pipelineStep[state=\"unavailable\"] { color: "+muted+"; font-style: italic; }";
    css+="QLabel#reconstructionStatus, QLabel#splineStatus { background: "+button+"; border: 1px solid "+border+"; border-radius: 8px; padding: 6px 10px; }";
    css+="QLabel#reconstructionStatus[state=\"running\"] { border-color: "+primary+"; }";
    css+="QLabel#reconstructionStatus[state=\"done\"], QLabel#splineStatus[state=\"done\"], QLabel#reconstructionToolState[state=\"done\"] { border-color: "+QString(darkBase?"#6BE09A":"#1E8F4E")+"; color: "+QString(darkBase?"#6BE09A":"#1E8F4E")+"; }";
    css+="QLabel#reconstructionStatus[state=\"attention\"], QLabel#reconstructionStatus[state=\"canceled\"], QLabel#splineStatus[state=\"attention\"], QLabel#reconstructionToolState[state=\"attention\"] { border-color: "+QString(darkBase?"#F5C87A":"#9B4F08")+"; color: "+QString(darkBase?"#F5C87A":"#9B4F08")+"; }";
    css+="QLabel#reconstructionStatus[state=\"failed\"], QLabel#splineStatus[state=\"failed\"] { border-color: "+QString(darkBase?"#FF8A8A":"#C2413B")+"; color: "+QString(darkBase?"#FF8A8A":"#C2413B")+"; }";
    css+="QGraphicsView#splineCanvas { border: 1px solid "+border+"; border-radius: 8px; } QListWidget#reconstructionThumbnails { border: none; background: transparent; }";
    css+="QToolButton#inspectorDisclosure { font-weight: 600; color: "+muted+"; padding: 2px 4px; } QToolButton#inspectorDisclosure:hover { color: "+text+"; }";
    // Open files strip shown above non-Hex modules.
    css+="QWidget#openDocumentsBar { background: "+window+"; border-bottom: 1px solid "+border+"; } QLabel#openDocumentsCaption { color: "+muted+"; font-weight: 600; }";
    css+="QTabBar#openDocumentsStrip::tab { max-width: 180px; padding: 5px 10px; }";
    // Module rail, command palette, segmented inspector tabs and the status bar.
    const QString rail=darkBase?QColor(window).darker(125).name():QColor(window).darker(104).name();
    css+="QWidget#moduleRail { background: "+rail+"; border-right: 1px solid "+border+"; }";
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
    // Compact tool rows in panels (adaptive_layout.h) and primary tool buttons (Find, Apply).
    css+="QToolButton[compactTool=\"true\"] { padding: 2px 7px; border: 1px solid "+border+"; border-radius: 6px; background: "+button+"; } QToolButton[compactTool=\"true\"]:hover { background: "+hover+"; }";
    css+="QToolButton[primaryTool=\"true\"] { background: "+primary+"; color: "+onPrimary+"; border-color: "+primary+"; font-weight: 600; padding: 2px 12px; } QToolButton[primaryTool=\"true\"]:hover { background: "+primaryHover+"; }";
    css+="QLabel[cardTitle=\"true\"] { font-weight: 600; } QLabel#inspectorHint { color: "+muted+"; font-size: 11px; }";
    // Command Deck header and activity strip, Floating HUD cards and mode dock (interface_modes.h). Hidden in Classic.
    css+="QWidget#deckHeader { background: "+window+"; border-bottom: 1px solid "+border+"; } QWidget#deckFooter { background: "+window+"; border-top: 1px solid "+border+"; }";
    css+="QWidget#deckModes { background: "+surface+"; border: 1px solid "+border+"; border-radius: 10px; } QWidget#deckModes QToolButton { border: 1px solid transparent; border-radius: 8px; padding: 5px 12px; color: "+muted+"; }";
    css+="QWidget#deckModes QToolButton:hover { color: "+text+"; background: "+hover+"; } QWidget#deckModes QToolButton:checked { color: "+onPrimary+"; background: "+primary+"; font-weight: 600; }";
    css+="QPushButton#deckSearch { text-align: left; padding: 6px 12px; color: "+muted+"; background: "+base+"; border: 1px solid "+border+"; border-radius: 9px; font-weight: 400; } QPushButton#deckSearch:hover { border-color: "+focus+"; color: "+text+"; }";
    css+="QToolButton#deckDocument { border: 1px solid "+border+"; border-radius: 8px; padding: 5px 18px 5px 10px; background: "+surface+"; }";
    css+="QLabel#deckFooterCaption { color: "+muted+"; font-size: 10px; font-weight: 700; padding-right: 4px; }";
    css+="QLabel#deckChip, QWidget#deckChip { background: "+surface+"; border: 1px solid "+border+"; border-radius: 8px; padding: 3px 10px; color: "+text+"; font-size: 11px; } QWidget#deckChip QLabel { border: 0; background: transparent; padding: 0; font-size: 11px; }";
    css+="QProgressBar#deckProgressBar { border: 0; border-radius: 4px; background: "+border+"; } QProgressBar#deckProgressBar::chunk { border-radius: 4px; margin: 0; background: "+primary+"; }";
    css+="QDockWidget[hudCard=\"true\"] { border: 1px solid "+border+"; } QWidget#hudCardTitle { background: "+surface+"; border-bottom: 1px solid "+border+"; } QLabel#hudCardName { font-weight: 600; background: transparent; }";
    css+="QToolButton#hudCardClose { border: 0; padding: 0 6px; font-size: 14px; color: "+muted+"; background: transparent; } QToolButton#hudCardClose:hover { color: "+text+"; }";
    css+="QWidget#hudModeDock QToolButton { border: 1px solid transparent; border-radius: 10px; padding: 4px 6px; color: "+muted+"; font-size: 11px; } QWidget#hudModeDock QToolButton:hover { color: "+text+"; background: "+hover+"; }";
    css+="QWidget#hudModeDock QToolButton:checked { color: "+onPrimary+"; background: "+primary+"; } QFrame#hudDockDivider { color: "+border+"; } QLabel#hudDockGrip { color: "+muted+"; font-size: 14px; padding: 0 2px; }";
    {// Interface-mode chrome (see interface_modes.h). The terminal status line only exists in modes that show it.
        const auto chrome=qApp->property("interfaceChrome").toMap();
        css+="QWidget#statusLine { background: "+text+"; } QWidget#statusLine QLabel, QWidget#statusLine QToolButton { color: "+window+"; background: transparent; border: 0; border-radius: 0; padding: 3px 10px; font-weight: 600; font-family: \""+QApplication::font().family()+"\"; }";
        css+="QWidget#statusLine QToolButton:checked { background: "+window+"; color: "+text+"; } QWidget#statusLine QToolButton:hover { background: "+QColor(text).darker(130).name()+"; }";
        if(chrome.value("terminalMenus").toBool()){
            css+="QMenu { border-radius: 0; border: 1px solid "+text+"; padding: 2px; } QMenu::item { border-radius: 0; padding: 4px 22px 4px 10px; } QMenu::item:selected { background: "+text+"; color: "+window+"; }";
            css+="QMenuBar::item:selected { background: "+text+"; color: "+window+"; } QToolTip { border-radius: 0; border: 1px solid "+text+"; }";
        }
        // Combo and spin boxes are left alone: any border rule switches them to stylesheet drawing and drops their frame.
        if(chrome.value("squareCorners").toBool())css+="QPushButton, QToolButton, QLineEdit, QTabBar::tab, QTabWidget::pane, QScrollBar::handle, QFrame#inspectorCard, QFrame#meshSectionCard, QWidget#viewportToolbar, QWidget#viewportTransformBar, QLabel#vulkanStatus, QTreeWidget#meshCollection, QFrame#viewportCameraPanel, QFrame#viewportGridPanel, QDialog#commandPalette, QWidget#moduleRail QToolButton { border-radius: 0px; }";
    }
    {// Interface styles drawn with gradients (Aqua 2003, Aero Glass). Every colour is derived from the active palette and
     // the theme's "Interface Styles" colours (style.bar, style.glass, style.panelTitle, style.progress), so a style can
     // be worn by any theme: the Aqua and Aero palettes are just themes that set those colours.
        const auto chrome=qApp->property("interfaceChrome").toMap();const auto style=chrome.value("style").toString();
        auto gloss=[](QColor a,QColor b,QColor c,QColor d){return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "+a.name()+", stop:0.48 "+b.name()+", stop:0.52 "+c.name()+", stop:1 "+d.name()+")";};
        auto mix=[](QColor a,QColor b,double t){return QColor::fromRgbF(a.redF()+(b.redF()-a.redF())*t,a.greenF()+(b.greenF()-a.greenF())*t,a.blueF()+(b.blueF()-a.blueF())*t);};
        const QColor W(window),T(text),P(primary),F(focus),Bd(border),Bt(button),Bs(base),white(Qt::white),black(Qt::black);
        const QColor darkText=themeLuminance(T)<.3?T:QColor("#14202E");
        auto on=[&](QColor fill){return (themeLuminance(fill)>.36?darkText:white).name();};
        auto styleColor=[&](const char *key){return QColor(token(key));};
        auto horizontal=[](QColor a,QColor b){return "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 "+a.name()+", stop:1 "+b.name()+")";};
        auto vertical=[](QColor a,QColor b){return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "+a.name()+", stop:1 "+b.name()+")";};
        const QColor progress=styleColor("style.progress");
        if(style=="aqua"){
            if(chrome.value("gel").toBool()){
                const double hi=darkBase?.18:.85;auto gel=[&](QColor c){return gloss(mix(c,white,hi),mix(c,white,hi*.55),darkBase?c:c.darker(104),mix(c,white,hi*.7));};
                const QColor hoverFill=mix(Bt,F,.18),pressedFill=darkBase?Bt.lighter(125):mix(Bt,P,.2).darker(105);
                css+="QPushButton { border: 1px solid "+(darkBase?mix(Bd,white,.15):Bd.darker(118)).name()+"; border-radius: 11px; padding: 4px 14px; color: "+text+"; background: "+gel(Bt)+"; }";
                css+="QPushButton:hover { border-color: "+F.name()+"; background: "+gel(hoverFill)+"; } QPushButton:pressed { background: "+gloss(pressedFill.darker(108),pressedFill,pressedFill.darker(104),mix(pressedFill,white,.3))+"; }";
                auto blueGel=[&](QColor c){return gloss(mix(c,white,.5),mix(c,white,.2),c,mix(c,white,.3));};
                css+=primaries+" { color: "+on(P)+"; border: 1px solid "+P.darker(135).name()+"; font-weight: 700; background: "+blueGel(P)+"; }";
                css+=states(":hover")+" { border-color: "+P.darker(135).name()+"; background: "+blueGel(P.lighter(112))+"; }";
                css+=states(":pressed")+" { background: "+gloss(P.darker(110),P.darker(105),P.darker(118),P)+"; }";
                css+="QToolButton[segment=\"true\"]:checked { color: "+on(P)+"; border: 1px solid "+P.darker(135).name()+"; border-radius: 9px; background: "+blueGel(P)+"; }";
                css+="QTabBar::tab:selected { color: "+(darkBase?T:P.darker(150)).name()+"; border-bottom: 2px solid "+primary+"; background: "+gel(Bs)+"; }";
                // Small buttons (compact ones and those inside tables and trees) keep a tight gel so their text fits.
                css+="QPushButton[compact=\"true\"], QAbstractItemView QPushButton, QFrame#inspectorCard QPushButton[compact=\"true\"] { padding: 1px 8px; border-radius: 7px; min-height: 14px; }";
            }
            if(chrome.value("taskPanes").toBool()){
                const QColor title=styleColor("style.panelTitle"),bar=styleColor("style.bar");
                css+="QDockWidget { color: "+on(title)+"; font-weight: 700; } QDockWidget::title { padding: 6px 8px; border: 0; border-top-left-radius: 6px; border-top-right-radius: 6px; background: "+horizontal(title.darker(112),mix(title,white,.35))+"; }";
                css+="QWidget#hudCardTitle { border: 0; background: "+horizontal(title.darker(112),mix(title,white,.35))+"; } QLabel#hudCardName, QToolButton#hudCardClose { color: "+on(title)+"; font-weight: 700; }";
                css+="QWidget#moduleRail { border-right: 1px solid "+title.darker(130).name()+"; background: "+(darkBase?vertical(mix(W,title,.35),mix(W,title,.18)):vertical(mix(title,white,.45),mix(title,white,.25)))+"; }";
                css+="QMenuBar { background: "+bar.name()+"; border-bottom: 1px solid "+bar.darker(115).name()+"; color: "+on(bar)+"; } QMenuBar::item:selected { background: "+primary+"; color: "+on(P)+"; }";
                css+="QStatusBar { background: "+bar.name()+"; border-top: 1px solid "+mix(bar,white,.6).name()+"; color: "+on(bar)+"; } QStatusBar QLabel { color: "+on(bar)+"; }";
                // Table headers: a soft gel that stays readable on dark themes (only a faint highlight there).
                css+="QHeaderView::section { color: "+(darkBase?text:muted)+"; background: "+(darkBase?vertical(mix(Bs,white,.10),mix(Bs,Bd,.35)):vertical(mix(Bs,white,.7),mix(Bs,Bd,.3)))+"; }";
            }
            css+="QProgressBar { border: 1px solid "+(darkBase?Bd:mix(Bd,P,.3)).name()+"; border-radius: 3px; background: "+base+"; color: "+text+"; text-align: center; } QProgressBar::chunk { width: 8px; margin: 1px; background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "+mix(progress,white,.55).name()+", stop:0.6 "+progress.name()+", stop:1 "+mix(progress,white,.3).name()+"); }";
        }
        if(style=="aero"){
            const QColor G=styleColor("style.glass"),edge=darkBase?black:G.darker(140),barText=QColor(on(G));
            const QString glass=gloss(mix(G,white,.30),mix(G,white,.12),G,mix(G,white,.07)),blueGlass=gloss(mix(F,white,.4),P,P.darker(135),mix(P,F,.5)),blueEdge=mix(F,white,.3).name();
            if(chrome.value("glassBars").toBool()){
                css+="QMenuBar { background: "+glass+"; border-bottom: 1px solid "+edge.name()+"; } QMenuBar::item { background: transparent; color: "+barText.name()+"; padding: 5px 10px; }";
                css+="QMenuBar::item:selected { color: "+on(P)+"; border: 1px solid "+blueEdge+"; border-radius: 3px; background: "+blueGlass+"; }";
                css+="QToolBar { background: "+glass+"; border: 0; } QStatusBar { background: "+glass+"; color: "+mix(barText,G,.15).name()+"; } QStatusBar QLabel { color: "+mix(barText,G,.15).name()+"; }";
                css+="QDockWidget { color: "+barText.name()+"; } QDockWidget::title { padding: 6px 8px; border: 1px solid "+edge.name()+"; background: "+glass+"; }";
                css+="QWidget#hudCardTitle { border-bottom: 1px solid "+edge.name()+"; background: "+glass+"; } QLabel#hudCardName, QToolButton#hudCardClose { color: "+barText.name()+"; }";
                css+="QWidget#moduleRail { border-right: 1px solid "+mix(G,P,.25).name()+"; background: "+horizontal(mix(G,P,.30),darkBase?G.darker(110):G)+"; }";
                css+="QWidget#viewportToolbar, QWidget#viewportTransformBar { border: 1px solid "+edge.name()+"; background: "+glass+"; }";
                css+="QHeaderView::section { color: "+mix(barText,G,.15).name()+"; background: "+glass+"; }";
            }
            if(chrome.value("glassButtons").toBool()){
                const QColor hoverFill=mix(Bt,F,.45),pressedFill=hoverFill.darker(130);
                css+="QPushButton { border: 1px solid "+(darkBase?mix(Bd,black,.6):Bd.darker(130)).name()+"; border-radius: 4px; color: "+text+"; background: "+gloss(mix(Bt,white,.22),mix(Bt,white,.07),Bt.darker(125),Bt)+"; }";
                css+="QPushButton:hover { border-color: "+blueEdge+"; color: "+on(hoverFill)+"; background: "+gloss(mix(hoverFill,white,.3),hoverFill,hoverFill.darker(125),mix(hoverFill,F,.3))+"; } QPushButton:pressed { background: "+gloss(pressedFill,pressedFill.darker(110),pressedFill.darker(125),pressedFill.lighter(115))+"; }";
                css+=primaries+" { color: "+on(P)+"; border: 1px solid "+blueEdge+"; font-weight: 600; background: "+blueGlass+"; }";
                const QColor P2=P.lighter(115);css+=states(":hover")+" { border-color: "+mix(F,white,.6).name()+"; background: "+gloss(mix(F,white,.6),P2,P2.darker(130),mix(P2,F,.5))+"; }";
                css+="QToolButton[segment=\"true\"]:checked { color: "+on(P)+"; border: 1px solid "+blueEdge+"; background: "+blueGlass+"; }";
                css+="QProgressBar { border: 1px solid "+edge.name()+"; border-radius: 3px; background: "+(darkBase?Bs.darker(110):Bs).name()+"; color: "+text+"; text-align: center; } QProgressBar::chunk { border-radius: 2px; background: "+gloss(mix(progress,white,.6),progress,progress.darker(115),mix(progress,white,.3))+"; }";
            }
        }
    }
    {// Theme effects, available to every theme and style: options.roundness (corner radius in px for panels and
     // buttons), options.neon (glowing panel edges in the "style.neon" colour) and options.glow (text glow type).
        const QColor neon(token("style.neon"));qApp->setProperty("workbenchNeonColor",neon);qApp->setProperty("workbenchGlow",options.value("glow").toString());
        if(options.contains("roundness")){
            // Every radius scales with the chosen roundness; the stylesheet's reference is the 6 px button radius.
            const int roundness=qBound(0,options.value("roundness").toInt(),24);QString scaled;qsizetype last=0;
            static const QRegularExpression radius("(border(?:-top|-bottom)?(?:-left|-right)?-radius:\\s*)(\\d+)px");
            for(auto it=radius.globalMatch(css);it.hasNext();){const auto match=it.next();scaled+=css.mid(last,match.capturedStart()-last)+match.captured(1)+QString::number(qRound(match.captured(2).toInt()*roundness/6.0))+"px";last=match.capturedEnd();}
            css=scaled+css.mid(last);
        }
        if(options.value("neon").toBool()&&neon.isValid()){
            // Neon edges: a glowing rounded outline drawn as a nine-patch border image (Qt stylesheets have no shadows),
            // generated once per colour and corner size. The glow sits inside each frame's edge.
            const int corner=qBound(0,options.value("roundness").toInt(8),10);
            const QString path=QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QString("9xd-neon2-%1-%2.png").arg(neon.name().mid(1)).arg(corner));
            if(!QFileInfo::exists(path)){
                QImage image(48,48,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);painter.setBrush(Qt::NoBrush);
                const QRectF tube(4.5,4.5,39,39);for(auto [width,alpha]:std::initializer_list<std::pair<double,int>>{{10,40},{7,75},{4.5,130},{2.8,225}}){QColor c=neon;c.setAlpha(alpha);painter.setPen(QPen(c,width));painter.drawRoundedRect(tube,corner*2,corner*2);}
                QColor core=neon.lighter(135);painter.setPen(QPen(core,1.5));painter.drawRoundedRect(tube,corner*2,corner*2);painter.end();image.save(path);
            }
            const QString url=QDir::fromNativeSeparators(path),image="border-width: 5px; border-image: url(\""+url+"\") 10 10 10 10 stretch stretch; background-clip: padding;";
            css+="QFrame#inspectorCard, QFrame#meshSectionCard, QGroupBox, QTabWidget::pane, QWidget#viewportToolbar, QWidget#viewportTransformBar, QFrame#viewportInspectorCard, QWidget#deckModes, QMenu, QFrame#viewportCameraPanel, QFrame#viewportGridPanel, QDialog#commandPalette { "+image+" }";
            QColor dim=neon;dim.setAlpha(150);
            css+="QDockWidget::title { border-bottom: 2px solid "+neon.name()+"; } QAbstractItemView { border: 1px solid "+dim.name(QColor::HexArgb)+"; }";
            css+="QLineEdit:focus, QComboBox:focus, QAbstractSpinBox:focus, QPushButton:focus { border: 1px solid "+neon.name()+"; } QWidget#moduleRail { border-right: 2px solid "+neon.name()+"; }";
            css+="QHeaderView::section { border-bottom: 1px solid "+neon.name()+"; } QTabBar::tab:selected { border-bottom: 2px solid "+neon.name()+"; } QWidget#deckHeader { border-bottom: 2px solid "+neon.name()+"; } QWidget#deckFooter { border-top: 2px solid "+neon.name()+"; } QStatusBar { border-top: 1px solid "+neon.name()+"; }";
            css+="QSplitter::handle, QMainWindow::separator { background: "+dim.name(QColor::HexArgb)+"; }";
        }
    }
    // Window gradient: themes that set "background.gradient" fade the window colour into it (top to bottom-right).
    // Panels drawn on the window (module areas, docks, dialogs) show it; lists and fields keep their surface colour.
    if(colors.contains("background.gradient")){const QColor end(colors["background.gradient"].toString());if(end.isValid())css+="QMainWindow, QDialog, QDockWidget > QWidget, QWidget#deckHeader, QWidget#deckFooter { background: qlineargradient(x1:0, y1:0, x2:0.35, y2:1, stop:0 "+window+", stop:1 "+end.name()+"); }";}
    qApp->setStyleSheet(css);qApp->setProperty("workbenchTheme",theme.isEmpty()?name:requested);
    {// Interface-mode scanlines: a texture on window and list backgrounds (behind content). Brushes keep their colour, so
     // code that reads palette colours is unaffected. A full-window overlay is not used: the native Vulkan viewport
     // makes the window's containers native, which would hide (or blacken) a sibling overlay.
        const double strength=qApp->property("interfaceChrome").toMap().value("scanlines").toDouble();
        if(qApp->property("interfaceChrome").toMap().value("pinstripes").toBool()){// Aqua: 2 px light / 2 px slightly darker rows on window backgrounds.
            auto textured=p;const QColor base=p.color(QPalette::Window);QPixmap tile(4,4);tile.fill(base.lighter(102));{QPainter painter(&tile);painter.fillRect(0,2,4,2,base.darker(103));}textured.setBrush(QPalette::All,QPalette::Window,QBrush(base,tile));qApp->setPalette(textured);}
        else if(strength>0){auto textured=p;for(auto role:{QPalette::Window,QPalette::Base}){const QColor base=p.color(role),line=QColor::fromRgbF(base.redF()+(p.color(QPalette::Text).redF()-base.redF())*strength*.14,base.greenF()+(p.color(QPalette::Text).greenF()-base.greenF())*strength*.14,base.blueF()+(p.color(QPalette::Text).blueF()-base.blueF())*strength*.14);QPixmap tile(4,3);tile.fill(base);{QPainter painter(&tile);painter.fillRect(0,2,4,1,line);}textured.setBrush(QPalette::All,role,QBrush(base,tile));}qApp->setPalette(textured);}
    }
    // Tool colors can change while the base palette stays identical.
    qApp->paletteChanged(p);for(auto *widget:QApplication::allWidgets())widget->update();
}
