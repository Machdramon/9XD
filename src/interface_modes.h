#pragma once
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QSettings>
#include <QVariantMap>
#include <QPointer>
#include <QEvent>
#include <QMouseEvent>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <QAbstractItemView>
#include <QTimer>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QTabBar>
#include <QGuiApplication>
// Interface modes: how 9XD is presented, as three independent choices that can be combined freely.
//   style   - how menus, panel titles, buttons and bars are drawn, plus effects (Classic, Hackerman CRT, Aqua, Aero);
//   layout  - how panels are organised (Classic docks, Inspector stack; later Command Deck, Floating HUD, Pipeline);
// Colours always come from the theme the user picks (Themes page or Theme menu): styles only draw, so any theme works
// with any style. Styles derive their gradients from the palette and the theme's "Interface Styles" colours.
// Classic style + Classic layout is 0.3.7 exactly.
// Roadmap and per-mode work items: docs/ui-modes/ROADMAP.md.
namespace InterfaceModes {
struct Mode {QString id,name,summary,theme,status;};
inline const QList<Mode> &styles(){static const QList<Mode> modes{
    {"classic","Classic (0.3.7)","The current look: flat panels, rounded controls.","","stable"},
    {"hackerman-crt","Hackerman CRT","Terminal status line instead of the module rail, box-drawn panel titles, terminal menus with square corners, scanline texture and text glow. Pairs well with the Hackerman theme.","","experimental"},
    {"aqua-2003","Aqua 2003","Gel style from the early 2000s: glossy gel buttons, pinstripe backgrounds, task-pane panel titles and rail, plain menu and status bars, block progress bars. Pairs well with the Aqua 2003 theme.","","experimental"},
    {"aero-glass","Aero Glass","Glass style from the late 2000s: glossy glass menu, tool and panel bars, glass buttons that glow on hover, shiny progress bars and soft text glow. Pairs well with the Aero Glass theme.","","experimental"},
    {"glass-nines","Glass Nines","Frosted panels over the 3D viewport (needs the Vulkan overlay pass).","","planned"},
    {"carbon-pit","Carbon Pit","Carbon fibre and brushed aluminium with gauge-style status.","","planned"}
};return modes;}
inline const QList<Mode> &layouts(){static const QList<Mode> modes{
    {"classic","Classic (0.3.7)","Panels where you put them: tabbed groups, side and bottom docks.","","stable"},
    {"inspector-stack","Inspector stack","Each module's panels become one collapsible stack on the right, so the main view gets the full height. Click a panel title to open or fold it. Your Classic arrangement is kept apart and comes back when you switch back.","","experimental"},
    {"command-deck","Command Deck","A header with the modules as tabs (with counts), the command search and the open file; panels as a stack with summaries on the right; an activity strip at the bottom with recent messages, running progress and threads.","","experimental"},
    {"floating-hud","Floating HUD","The work area fills the window; panels become translucent floating cards that snap to the window edges and to each other and move with the window; modules, search and settings sit in a dock at the bottom. Drag the dock by its background to move it; double-click it to put it back.","","experimental"},
    {"pipeline","Pipeline","Source → Detect → Output columns with inline inspectors.","","planned"}
};return modes;}
inline const QList<Mode> &all(){return styles();}
inline const Mode *findIn(const QList<Mode> &list,const QString &id){for(const auto &mode:list)if(mode.id==id)return &mode;return nullptr;}
inline const Mode *find(const QString &id){return findIn(styles(),id);}
inline const Mode *findLayout(const QString &id){return findIn(layouts(),id);}
// Per-style and per-layout options live under interface/<id>/<key>; defaults are the intended look.
// Options shown as check boxes in Settings: {key, label, default}. The CRT scanline slider is handled apart.
struct Option {QString key,label;bool fallback;};
inline QList<Option> options(const QString &id){
    if(id=="hackerman-crt")return {{"statusLine","Terminal status line instead of the module rail (Alt+1…6 switch modes)",true},{"boxTitles","Box-drawn panel titles",true},{"terminalMenus","Terminal menus and square corners",true},{"glow","Phosphor glow on text",true}};
    if(id=="aqua-2003")return {{"gel","Glossy gel buttons",true},{"pinstripes","Pinstripe window background",true},{"taskPanes","Task-pane panel titles and rail, plain menu and status bars",true}};
    if(id=="aero-glass")return {{"glassBars","Glass menu, tool, status and panel bars",true},{"glassButtons","Glass buttons with hover glow, shiny progress bars",true},{"glow","Soft glow on text",true}};
    if(id=="inspector-stack")return {{"exclusive","One panel open at a time",true},{"collectBottom","Move bottom panels into the stack too",true}};
    if(id=="command-deck")return {{"exclusive","One panel open at a time",false},{"collectBottom","Move bottom panels into the stack too",true},{"summaries","Summaries in panel titles (item counts)",true},{"timeline","Activity strip at the bottom",true}};
    if(id=="floating-hud")return {{"translucent","Translucent cards",true},{"snap","Snap cards to the window edges and to each other",true},{"follow","Cards move with the window",true},{"modeDock","Mode dock at the bottom instead of the module rail",true},
        {"dockCollapse","Mode dock shows only the active module until the pointer is over it",false},{"dockAutoHide","Mode dock hides itself until the pointer comes near",false}};
    return {};
}
inline QVariantMap chromeFor(const QString &id,QSettings &settings){
    auto get=[&](const char *key,QVariant fallback){return settings.value("interface/"+id+"/"+key,fallback);};
    if(id=="aqua-2003")return {{"style","aqua"},{"gel",get("gel",true).toBool()},{"pinstripes",get("pinstripes",true).toBool()},{"taskPanes",get("taskPanes",true).toBool()}};
    if(id=="aero-glass")return {{"style","aero"},{"glassBars",get("glassBars",true).toBool()},{"glassButtons",get("glassButtons",true).toBool()},{"glow",get("glow",true).toBool()}};
    if(id!="hackerman-crt")return {};
    return {{"style","crt"},{"statusLine",get("statusLine",true).toBool()},{"boxTitles",get("boxTitles",true).toBool()},{"terminalMenus",get("terminalMenus",true).toBool()},
            {"squareCorners",get("terminalMenus",true).toBool()},{"glow",get("glow",true).toBool()},{"scanlines",qBound(0,get("scanlines",35).toInt(),100)/100.0}};
}
// The selected style, migrating the single 0.3.x-era "interface/mode" key.
inline QString styleId(QSettings &settings){
    QString id=settings.value("interface/style",settings.value("interface/mode","classic")).toString();const auto *mode=find(id);
    return !mode||mode->status=="planned"?QString("classic"):id;
}
inline QString layoutId(QSettings &settings){
    const QString id=settings.value("interface/layout","classic").toString();const auto *mode=findLayout(id);
    return !mode||mode->status=="planned"?QString("classic"):id;
}
// Reads the choices from settings and publishes them for the theme, the style and the shell.
inline void load(QSettings &settings){
    const auto id=styleId(settings),layout=layoutId(settings);
    auto chrome=chromeFor(id,settings);chrome["layout"]=layout;
    for(const auto &option:options(layout))chrome[option.key]=settings.value("interface/"+layout+"/"+option.key,option.fallback).toBool();
    qApp->setProperty("interfaceMode",id);qApp->setProperty("interfaceLayout",layout);qApp->setProperty("interfaceChrome",chrome);
}
inline bool stackLayout(const QString &layout){return layout=="inspector-stack"||layout=="command-deck";}
inline QVariantMap chrome(){return qApp->property("interfaceChrome").toMap();}

// Chrome that is not a stylesheet rule: panel title decoration. Box-drawn titles ("┤ INSPECTOR ├", CRT) and the
// stack disclosure arrow ("▾ Inspector", Inspector stack) are composed on the plain title, which is kept and restored.
// Titles that code changes later are decorated again; the Windows menu keeps the plain name.
class Chrome final : public QObject {
public:
    using QObject::QObject;
    static void apply(){
        static QPointer<Chrome> instance;if(!instance){instance=new Chrome(qApp);qApp->installEventFilter(instance);}
        for(auto *widget:QApplication::allWidgets())if(auto *dock=qobject_cast<QDockWidget *>(widget))decorate(dock);
    }
    static QString plainTitle(QDockWidget *dock){const auto plain=dock->property("modePlainTitle").toString();return plain.isEmpty()?dock->windowTitle():plain;}
    static void decorate(QDockWidget *dock){
        if(!dock->property("modeWatched").toBool()){dock->setProperty("modeWatched",true);QObject::connect(dock,&QWidget::windowTitleChanged,dock,[dock](const QString &title){if(dock->property("modeDecorating").toBool())return;dock->setProperty("modePlainTitle",title);decorate(dock);});}
        const QString plain=plainTitle(dock);if(plain.isEmpty())return;
        QString title=chrome().value("boxTitles").toBool()?QString(QChar(0x2524))+" "+plain.toUpper()+" "+QChar(0x251C):plain;
        const auto stack=dock->property("stackState").toString();
        if(!stack.isEmpty()){title=QString(stack=="closed"?QChar(0x25B8):QChar(0x25BE))+"  "+title;const auto summary=dock->property("stackSummary").toString();if(!summary.isEmpty())title+=QString("   \u00b7   ")+summary;}
        dock->setProperty("modeDecorating",true);
        dock->setProperty("modePlainTitle",title==plain?QVariant():QVariant(plain));if(dock->windowTitle()!=title)dock->setWindowTitle(title);
        dock->toggleViewAction()->setText(plain);dock->setProperty("modeDecorating",false);
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::Polish)if(auto *dock=qobject_cast<QDockWidget *>(object))decorate(dock);
        return QObject::eventFilter(object,event);
    }
};

// Inspector stack layout: a module's side and bottom panels in one column on the right, each folding to its title.
// Main work surfaces (3D viewport, UV canvas, script editor, reconstruction canvases, AI chat) stay where they are.
// The column is an ordinary dock arrangement, so it is saved per layout (interface/layouts/<layout>/<module>) and the
// Classic arrangement is never touched. Folding hides the panel's content and caps its height at the title.
class DockStack final : public QObject {
public:
    static DockStack *instance(){static QPointer<DockStack> stack;if(!stack)stack=new DockStack(qApp);return stack;}
    static bool workSurface(const QDockWidget *dock){static const QSet<QString> names{"vulkanDock","uvDock","scriptDock","scriptConsoleDock","videoReconstructionDock","splineModelingDock","aiChatDock"};return names.contains(dock->objectName());}
    QList<QDockWidget *> members(QMainWindow *host)const{
        const bool collect=chrome().value("collectBottom",true).toBool();QList<QDockWidget *> list;
        for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly)){
            if(dock->isFloating()||workSurface(dock))continue;const auto area=host->dockWidgetArea(dock);
            if(area==Qt::LeftDockWidgetArea||area==Qt::RightDockWidgetArea||(collect&&area==Qt::BottomDockWidgetArea))list<<dock;
        }
        auto rank=[host](QDockWidget *dock){const auto area=host->dockWidgetArea(dock);return area==Qt::RightDockWidgetArea?0:area==Qt::LeftDockWidgetArea?1:2;};
        // Within a tab group (same place), the raised tab comes first.
        auto hiddenTab=[](QDockWidget *dock){return dock->visibleRegion().isEmpty()?1:0;};
        std::stable_sort(list.begin(),list.end(),[&](QDockWidget *a,QDockWidget *b){if(rank(a)!=rank(b))return rank(a)<rank(b);if(a->geometry().top()!=b->geometry().top())return a->geometry().top()<b->geometry().top();if(a->geometry().left()!=b->geometry().left())return a->geometry().left()<b->geometry().left();return hiddenTab(a)<hiddenTab(b);});
        return list;
    }
    // Called while the Classic arrangement is still in place: the stack keeps its reading order (raised tabs first).
    void captureOrder(QMainWindow *host){QStringList names;for(auto *dock:members(host))names<<dock->objectName();if(!names.isEmpty())order_[host->objectName()]=names;}
    // build: lay the column out from the current arrangement (entering the layout without a saved one, or Reset Panel
    // Layout). Otherwise the user's own arrangement is kept and only the fold behaviour is applied.
    void arrange(QMainWindow *host,bool build){
        auto docks=members(host);if(docks.isEmpty())return;
        if(!build){apply(host,docks);return;}
        bool stacked=true;for(auto *dock:docks)stacked&=host->dockWidgetArea(dock)==Qt::RightDockWidgetArea&&host->tabifiedDockWidgets(dock).isEmpty();
        // Work surfaces docked at the top or bottom (the 3D viewport, UV canvas, script editor) move to the left area, so
        // the stack sits beside them instead of above.
        QList<QDockWidget *> work;for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly)){if(!workSurface(dock)||dock->isFloating())continue;const auto area=host->dockWidgetArea(dock);if(area==Qt::TopDockWidgetArea||area==Qt::BottomDockWidgetArea)work<<dock;}
        if(!work.isEmpty()){
            std::stable_sort(work.begin(),work.end(),[](QDockWidget *a,QDockWidget *b){return a->geometry().top()<b->geometry().top();});
            QHash<QDockWidget *,bool> hidden;for(auto *dock:work){hidden[dock]=dock->isHidden();host->removeDockWidget(dock);}
            for(auto *dock:work){host->addDockWidget(Qt::LeftDockWidgetArea,dock,Qt::Vertical);dock->setVisible(!hidden[dock]);}
            stacked=false;
        }
        // One column means one width; a restored state with nested splits is rebuilt.
        if(stacked){int width=-1;for(auto *dock:docks){if(dock->isHidden()||dock->width()<=0)continue;if(width<0)width=dock->width();else if(std::abs(dock->width()-width)>8){stacked=false;break;}}}
        if(!stacked){
            const auto order=order_.value(host->objectName());
            if(!order.isEmpty())std::stable_sort(docks.begin(),docks.end(),[&](QDockWidget *a,QDockWidget *b){auto at=[&](QDockWidget *d){const int i=order.indexOf(d->objectName());return i<0?int(order.size()):i;};return at(a)<at(b);});
            QHash<QDockWidget *,bool> hidden;for(auto *dock:docks){hidden[dock]=dock->isHidden();host->removeDockWidget(dock);}
            for(auto *dock:docks){host->addDockWidget(Qt::RightDockWidgetArea,dock,Qt::Vertical);dock->setVisible(!hidden[dock]);}
            QList<QDockWidget *> shown;for(auto *dock:docks)if(!dock->isHidden())shown<<dock;
            const int column=qMax(320,host->width()/4);if(!shown.isEmpty())host->resizeDocks({shown.first()},{column},Qt::Horizontal);
            QList<QDockWidget *> surfaces;for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly))if(workSurface(dock)&&!dock->isHidden()&&host->dockWidgetArea(dock)==Qt::LeftDockWidgetArea)surfaces<<dock;
            if(!surfaces.isEmpty()&&!host->centralWidget())host->resizeDocks({surfaces.first()},{qMax(200,host->width()-column-8)},Qt::Horizontal);
        }
        apply(host,docks);
    }
    // Panels in tab groups stay as tabs (never folded); single panels fold from their title.
    void apply(QMainWindow *host,QList<QDockWidget *> docks){
        QList<QDockWidget *> single;for(auto *dock:docks){if(!host->tabifiedDockWidgets(dock).isEmpty()){watch(dock);release(dock);}else single<<dock;}docks=single;
        const bool exclusive=chrome().value("exclusive",true).toBool();QDockWidget *open=nullptr;
        if(exclusive)for(auto *dock:docks)if(!dock->isHidden()&&opened_.contains(dock->objectName())){open=dock;break;}
        for(auto *dock:docks){
            watch(dock);
            setOpen(dock,exclusive?dock==open:opened_.contains(dock->objectName()));
        }
        ensureOneOpen(host);
    }
    // Back to Classic: every panel unfolded and undecorated before the Classic arrangement is restored.
    void clear(QMainWindow *host){for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly))release(dock);}
    static void release(QDockWidget *dock){
        if(!dock->property("stackState").isValid())return;
        if(auto *content=dock->widget())content->setMaximumHeight(content->property("stackContentMax").isValid()?content->property("stackContentMax").toInt():QWIDGETSIZE_MAX);dock->setMaximumHeight(QWIDGETSIZE_MAX);
        dock->setProperty("stackState",QVariant());dock->setProperty("stackSummary",QVariant());Chrome::decorate(dock);
    }
    // Command Deck: item counts of each panel's first list, table or tree ("195 items"). The byte Inspector is a fixed
    // table of types, so it gets none.
    static int itemCount(QDockWidget *dock){
        if(!dock->widget())return 0;
        for(auto *view:dock->widget()->findChildren<QAbstractItemView *>()){if(!view->model()||view->isHidden())continue;const int rows=view->model()->rowCount();if(rows>0)return rows;}
        return 0;
    }
    static QString summaryFor(QDockWidget *dock){
        if(dock->objectName()=="inspectorDock")return {};const int rows=itemCount(dock);
        return rows>0?QString("%L1 %2").arg(rows).arg(rows==1?"item":"items"):QString();
    }
    void refreshSummaries(QMainWindow *host){
        const bool on=chrome().value("summaries").toBool();
        for(auto *dock:members(host)){if(!dock->property("stackState").isValid())continue;const auto summary=on?summaryFor(dock):QString();if(dock->property("stackSummary").toString()!=summary){dock->setProperty("stackSummary",summary);Chrome::decorate(dock);}}
    }
    void watch(QDockWidget *dock){
        if(dock->property("stackFiltered").toBool())return;dock->setProperty("stackFiltered",true);dock->installEventFilter(this);
        // A panel pulled out becomes an ordinary floating panel (full content, no fold arrow, a usable size). One dropped
        // into a tab group stays a tab; one dropped on its own joins the stack, open.
        auto relocate=[this,guard=QPointer<QDockWidget>(dock)]{
            auto *panel=guard.data();if(!panel||!stackLayout(chrome().value("layout").toString()))return;
            if(panel->isFloating()){const bool folded=panel->property("stackState").isValid();release(panel);if(folded)panel->resize(qMax(panel->width(),320),qMax(panel->height(),360));return;}
            auto *host=qobject_cast<QMainWindow *>(panel->parentWidget());if(!host)return;
            if(!host->tabifiedDockWidgets(panel).isEmpty()){release(panel);for(auto *other:host->tabifiedDockWidgets(panel))release(other);}
            else{remember(panel,true);setOpen(panel,true);}
        };
        QObject::connect(dock,&QDockWidget::topLevelChanged,this,[this,relocate]{QTimer::singleShot(0,this,relocate);});
        QObject::connect(dock,&QDockWidget::dockLocationChanged,this,[this,relocate]{QTimer::singleShot(0,this,relocate);});
    }
    void toggle(QDockWidget *dock){
        auto *host=qobject_cast<QMainWindow *>(dock->parentWidget());if(!host)return;const bool wasOpen=dock->property("stackState").toString()=="open";
        if(chrome().value("exclusive",true).toBool()){if(wasOpen)return;for(auto *other:members(host))if(other!=dock&&!other->isHidden()&&other->property("stackState").isValid()){remember(other,false);setOpen(other,false);}}
        remember(dock,!wasOpen);setOpen(dock,!wasOpen);ensureOneOpen(host);
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        auto *dock=qobject_cast<QDockWidget *>(object);if(!dock||!dock->property("stackState").isValid()||dock->isFloating())return false;
        if(event->type()==QEvent::MouseButtonPress){auto *mouse=static_cast<QMouseEvent *>(event);if(mouse->button()==Qt::LeftButton&&mouse->position().y()<titleHeight(dock))press_={dock,mouse->position().toPoint()};else press_={};}
        else if(event->type()==QEvent::MouseButtonRelease&&press_.first==dock){
            auto *mouse=static_cast<QMouseEvent *>(event);const bool click=(mouse->position().toPoint()-press_.second).manhattanLength()<QApplication::startDragDistance();press_={};
            if(click)QMetaObject::invokeMethod(this,[this,guard=QPointer<QDockWidget>(dock)]{if(guard)toggle(guard);},Qt::QueuedConnection);
        }
        return false;
    }
private:
    using QObject::QObject;
    QSet<QString> opened_,closed_;QHash<QString,QStringList> order_;std::pair<QPointer<QDockWidget>,QPoint> press_;
    // The title bar height comes from the style sheet padding; it is measured while the content is shown.
    static int titleHeight(QDockWidget *dock){
        if(dock->widget()&&dock->widget()->isVisible()&&dock->widget()->height()>0&&dock->widget()->geometry().top()>0)dock->setProperty("stackTitleHeight",dock->widget()->geometry().top());
        return qMax(dock->fontMetrics().height()+16,dock->property("stackTitleHeight").toInt());
    }
    void remember(QDockWidget *dock,bool open){if(open){opened_.insert(dock->objectName());closed_.remove(dock->objectName());}else{closed_.insert(dock->objectName());opened_.remove(dock->objectName());}}
    void setOpen(QDockWidget *dock,bool open){
        // Folding collapses the content to zero height instead of hiding it: a hidden content widget would also give
        // the panel a zero maximum width, and the folded title would shrink to a stub.
        const int title=titleHeight(dock);dock->setProperty("stackState",open?"open":"closed");
        if(auto *content=dock->widget()){if(!content->property("stackContentMax").isValid())content->setProperty("stackContentMax",content->maximumHeight());content->setMaximumHeight(open?content->property("stackContentMax").toInt():0);}
        dock->setMaximumHeight(open?QWIDGETSIZE_MAX:title);Chrome::decorate(dock);
    }
    void ensureOneOpen(QMainWindow *host){
        QDockWidget *first=nullptr;for(auto *dock:members(host)){if(dock->isHidden()||!host->tabifiedDockWidgets(dock).isEmpty())continue;if(dock->property("stackState").toString()=="open")return;if(!first)first=dock;}
        if(first)setOpen(first,true);
    }
};

// Floating HUD layout: side and bottom panels float as translucent cards over the work area (floating docks are
// top-level windows, so they also sit above the native Vulkan viewport). Cards get a slim custom title bar, snap to the
// work area's edges and to each other when a drag ends, and move with the main window.
class HudCards final : public QObject {
public:
    static HudCards *instance(){static QPointer<HudCards> cards;if(!cards)cards=new HudCards(qApp);return cards;}
    void setFrame(const QRect &frame){frame_=frame;}
    QRect frame()const{return frame_;}
    // build: float the docked panels as cards (entering the layout without a saved one, or Reset Panel Layout).
    // Otherwise cards keep the places the user gave them, and panels the user docked stay docked.
    void arrange(QMainWindow *host,bool build){
        if(!build){for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly))if(dock->isFloating()&&!DockStack::workSurface(dock))style(dock);return;}
        // Tabs that were behind another tab stay closed as cards (reopen them from the Windows menu), so a module
        // starts with the panels that were in view rather than a column of every panel.
        QSet<QDockWidget *> behind;const auto docks=host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly);
        for(auto *bar:host->findChildren<QTabBar *>(QString(),Qt::FindDirectChildrenOnly))for(int i=0;i<bar->count();++i){if(i==bar->currentIndex())continue;for(auto *dock:docks)if(!dock->isFloating()&&Chrome::plainTitle(dock)==bar->tabText(i))behind.insert(dock);}
        QList<QDockWidget *> left,right;
        for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly)){
            if(DockStack::workSurface(dock))continue;
            if(!dock->isFloating()){const auto area=host->dockWidgetArea(dock);if(area==Qt::NoDockWidgetArea||area==Qt::TopDockWidgetArea)continue;(area==Qt::LeftDockWidgetArea?left:right)<<dock;}
            style(dock);
        }
        // The top band stays clear for view toolbars (3D, UV).
        QRect area=frame_.isValid()?frame_:QRect(host->mapToGlobal(QPoint(0,0)),host->size());area.adjust(gap,gap+40,-gap,-gap);
        const int width=qBound(260,area.width()/5,380);
        // Cards are floated first; their final places are set once they are shown (Qt positions a newly floated dock
        // itself), using each card's real height. A column that runs out of room continues in a new column inwards.
        struct Column {QList<QPointer<QDockWidget>> docks;int x,step,height;};QList<Column> columns;
        auto prepare=[&](const QList<QDockWidget *> &list,int x,int step){
            Column column{{},x,step,0};int shown=0;
            for(auto *dock:list){const bool hidden=dock->isHidden()||behind.contains(dock);dock->setFloating(true);dock->setVisible(!hidden);if(!hidden){column.docks<<dock;++shown;}}
            column.height=qBound(150,(area.height()-gap*qMax(0,shown-1))/qMax(1,shown),440);columns<<column;
        };
        prepare(right,area.right()-width+1,-(width+gap));prepare(left,area.left(),width+gap);
        QTimer::singleShot(0,this,[columns,area,width]{
            for(const auto &column:columns){
                int x=column.x,y=area.top();
                for(const auto &dock:column.docks){
                    if(!dock||!dock->isFloating())continue;
                    // The layout may grow a card to its content's minimum after this call, so use that minimum up front.
                    const int height=qMin(area.height(),std::max({column.height,dock->minimumSizeHint().height(),dock->minimumHeight()}));
                    dock->setGeometry(x,y,width,height);const int real=qMax(height,dock->frameGeometry().height());
                    if(y>area.top()&&y+real>area.bottom()+1){x+=column.step;y=area.top();dock->move(dock->pos()+QPoint(x,y)-dock->frameGeometry().topLeft());}
                    y+=real+gap;
                }
            }
        });
    }
    void clear(QMainWindow *host){for(auto *dock:host->findChildren<QDockWidget *>(QString(),Qt::FindDirectChildrenOnly))unstyle(dock);}
    // The main window moved: cards keep their place relative to it.
    void follow(QPoint delta){
        if(delta.isNull()||!chrome().value("follow",true).toBool())return;frame_.translate(delta);
        for(auto *widget:QApplication::topLevelWidgets())if(auto *dock=qobject_cast<QDockWidget *>(widget);dock&&dock->property("hudCard").toBool()&&dock->isFloating())dock->move(dock->pos()+delta);
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::Move)if(auto *dock=qobject_cast<QDockWidget *>(object);dock&&dock->property("hudCard").toBool()&&dock->isFloating()&&!snapping_&&chrome().value("snap",true).toBool()){moved_=dock;timer_.start();}
        return false;
    }
private:
    static constexpr int gap=12,reach=24;
    QRect frame_;QPointer<QDockWidget> moved_;QTimer timer_;bool snapping_=false;
    explicit HudCards(QObject *parent):QObject(parent){timer_.setSingleShot(true);timer_.setInterval(180);connect(&timer_,&QTimer::timeout,this,[this]{snap();});}
    // A card docked back into the window loses its card look; floated again (in the HUD) it gets it back.
    static void unstyle(QDockWidget *dock){
        if(!dock->property("hudCard").toBool())return;dock->setProperty("hudCard",QVariant());dock->setWindowOpacity(1);
        if(auto *bar=dock->titleBarWidget()){dock->setTitleBarWidget(nullptr);bar->deleteLater();}
        dock->style()->unpolish(dock);dock->style()->polish(dock);
    }
    void style(QDockWidget *dock){
        if(!dock->property("hudWatched").toBool()){
            dock->setProperty("hudWatched",true);
            QObject::connect(dock,&QDockWidget::topLevelChanged,this,[this,guard=QPointer<QDockWidget>(dock)](bool floating){
                auto *card=guard.data();if(!card)return;
                if(!floating)unstyle(card);else if(chrome().value("layout").toString()=="floating-hud")style(card);
            });
        }
        dock->setProperty("hudCard",true);dock->setWindowOpacity(chrome().value("translucent",true).toBool()?.93:1.);
        if(!dock->property("hudFiltered").toBool()){dock->setProperty("hudFiltered",true);dock->installEventFilter(this);}
        if(!dock->titleBarWidget()){
            // A slim title: name and close. It ignores presses, so dragging it moves the card as usual.
            auto *bar=new QWidget;bar->setObjectName("hudCardTitle");bar->setAttribute(Qt::WA_StyledBackground);auto *row=new QHBoxLayout(bar);row->setContentsMargins(10,4,4,4);row->setSpacing(4);
            auto *name=new QLabel(Chrome::plainTitle(dock));name->setObjectName("hudCardName");row->addWidget(name,1);
            auto *close=new QToolButton;close->setObjectName("hudCardClose");close->setText(QString(QChar(0x00D7)));close->setAutoRaise(true);close->setToolTip("Hide panel");row->addWidget(close);
            QObject::connect(close,&QToolButton::clicked,dock,&QWidget::close);
            QObject::connect(dock,&QWidget::windowTitleChanged,name,[name,dock]{name->setText(Chrome::plainTitle(dock));});
            dock->setTitleBarWidget(bar);
        }
        dock->style()->unpolish(dock);dock->style()->polish(dock);
    }
    void snap(){
        auto *dock=moved_.data();if(!dock||!dock->isFloating()||!dock->isVisible())return;
        if(QGuiApplication::mouseButtons()!=Qt::NoButton){timer_.start();return;}
        const QRect card=dock->frameGeometry();QList<int> xs,ys;
        if(frame_.isValid()){xs<<frame_.left()+gap<<frame_.right()-gap-card.width()+1;ys<<frame_.top()+gap<<frame_.bottom()-gap-card.height()+1;}
        for(auto *widget:QApplication::topLevelWidgets())if(auto *other=qobject_cast<QDockWidget *>(widget);other&&other!=dock&&other->property("hudCard").toBool()&&other->isFloating()&&other->isVisible()){
            const QRect r=other->frameGeometry();xs<<r.right()+gap+1<<r.left()-gap-card.width()<<r.left();ys<<r.bottom()+gap+1<<r.top()-gap-card.height()<<r.top();
        }
        auto nearest=[](int value,const QList<int> &targets){int best=value,distance=reach+1;for(int t:targets)if(std::abs(t-value)<distance){distance=std::abs(t-value);best=t;}return best;};
        const QPoint target(nearest(card.left(),xs),nearest(card.top(),ys));
        if(target!=card.topLeft()){snapping_=true;dock->move(dock->pos()+(target-card.topLeft()));snapping_=false;}
    }
};
}
