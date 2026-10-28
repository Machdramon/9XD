#pragma once
#include <QApplication>
#include <QAbstractButton>
#include <QToolButton>
#include <QToolTip>
#include <QTabBar>
#include <QEvent>
// Tooltips are minimal by default: only buttons that show just an icon keep theirs (there the tooltip is the label),
// plus tab bars (full document names) and widgets marked keepTooltip. Settings > General can restore every tooltip.
// Help for complex views lives in the views themselves (for example the 3D viewport's Controls overlay).
class TooltipPolicy final : public QObject {
public:
    using QObject::QObject;
    static void install(){static bool installed=false;if(installed||!qApp)return;installed=true;qApp->installEventFilter(new TooltipPolicy(qApp));}
    static bool keeps(const QWidget *widget){
        for(auto *w=widget;w;w=w->parentWidget())if(w->property("keepTooltip").toBool())return true;
        if(qobject_cast<const QTabBar *>(widget))return true;
        if(auto *tool=qobject_cast<const QToolButton *>(widget))return tool->toolButtonStyle()==Qt::ToolButtonIconOnly||tool->text().trimmed().isEmpty()||tool->property("iconOnly").toBool();
        if(auto *button=qobject_cast<const QAbstractButton *>(widget))return button->text().trimmed().isEmpty()&&!button->icon().isNull();
        return false;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::ToolTip&&!qApp->property("workbenchDetailedTooltips").toBool()&&object->isWidgetType()&&!keeps(static_cast<QWidget *>(object))){QToolTip::hideText();return true;}
        return QObject::eventFilter(object,event);
    }
};
