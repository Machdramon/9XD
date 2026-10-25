#pragma once
#include <QTreeWidget>
#include <QHeaderView>
#include <QMenu>
#include <QToolButton>
#include <QSettings>
#include <QApplication>
#include <QTimer>
#include <QSignalBlocker>
#include <QCollator>
class AssetTreeItem:public QTreeWidgetItem {
public:using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem &other)const override{int column=treeWidget()?treeWidget()->sortColumn():0;bool a=false,b=false;auto left=text(column).toULongLong(&a,0),right=other.text(column).toULongLong(&b,0);if(a&&b)return left<right;static thread_local QCollator collator=[](){QCollator value;value.setNumericMode(true);value.setCaseSensitivity(Qt::CaseInsensitive);return value;}();return collator.compare(text(column),other.text(column))<0;}
};
inline QSettings assetTreeSettings(){auto file=qApp->property("workspaceSettingsFile").toString();if(file.isEmpty())return QSettings("BinaryWorkbench","Workspace");return QSettings(file,QSettings::IniFormat);}
inline QToolButton *configureAssetTree(QTreeWidget *tree,const QString &key){
    tree->setUniformRowHeights(true);tree->setTextElideMode(Qt::ElideMiddle);tree->setSortingEnabled(true);tree->sortItems(0,Qt::AscendingOrder);auto *header=tree->header();header->setSectionsMovable(true);header->setSectionResizeMode(QHeaderView::Interactive);header->setStretchLastSection(true);header->resizeSection(0,300);for(int i=1;i<tree->columnCount();++i)header->resizeSection(i,130);if(QGuiApplication::platformName()!="offscreen"||!qApp->property("workspaceSettingsFile").toString().isEmpty())header->restoreState(assetTreeSettings().value("lists/"+key).toByteArray());
    auto *menu=new QMenu(tree);QObject::connect(menu,&QMenu::aboutToShow,tree,[tree,menu]{menu->clear();for(int i=1;i<tree->columnCount();++i){auto *action=menu->addAction(tree->headerItem()->text(i));action->setCheckable(true);action->setChecked(!tree->isColumnHidden(i));QObject::connect(action,&QAction::toggled,tree,[tree,i](bool show){tree->setColumnHidden(i,!show);});}menu->addSeparator();auto *all=menu->addAction("Show all columns");QObject::connect(all,&QAction::triggered,tree,[tree]{for(int i=0;i<tree->columnCount();++i)tree->setColumnHidden(i,false);});});
    header->setContextMenuPolicy(Qt::CustomContextMenu);QObject::connect(header,&QWidget::customContextMenuRequested,tree,[header,menu](QPoint point){menu->popup(header->mapToGlobal(point));});auto *save=new QTimer(tree);save->setSingleShot(true);save->setInterval(200);QObject::connect(save,&QTimer::timeout,tree,[header,key]{if(QGuiApplication::platformName()=="offscreen"&&qApp->property("workspaceSettingsFile").toString().isEmpty())return;assetTreeSettings().setValue("lists/"+key,header->saveState());});QObject::connect(header,&QHeaderView::sectionResized,save,[save]{save->start();});QObject::connect(header,&QHeaderView::sectionMoved,save,[save]{save->start();});QObject::connect(header,&QHeaderView::sortIndicatorChanged,save,[save]{save->start();});auto *button=new QToolButton;button->setText("Columns");button->setObjectName(key+"Columns");button->setPopupMode(QToolButton::InstantPopup);button->setMenu(menu);return button;
}
inline void filterAssetTree(QTreeWidget *tree,const QString &query){QSignalBlocker block(tree);tree->setUpdatesEnabled(false);auto tokens=query.simplified().split(' ',Qt::SkipEmptyParts);for(int i=0;i<tree->topLevelItemCount();++i){auto *item=tree->topLevelItem(i);QString text=item->toolTip(0);for(int c=0;c<tree->columnCount();++c)text+=item->text(c)+' ';bool visible=true;for(auto token:tokens)visible&=text.contains(token,Qt::CaseInsensitive);item->setHidden(!visible);if(!visible)item->setSelected(false);}tree->setUpdatesEnabled(true);}
