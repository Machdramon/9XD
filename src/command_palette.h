#pragma once
#include "workbench_icons.h"
#include <QAction>
#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QVBoxLayout>
#include <functional>

// Ctrl+K search over every menu command plus extra entries such as module switching.
class CommandPalette final : public QDialog {
public:
    struct Entry {QString label;std::function<void()> run;};
    CommandPalette(QWidget *parent,QList<Entry> entries):QDialog(parent,Qt::Popup|Qt::FramelessWindowHint),entries_(std::move(entries)){
        setObjectName("commandPalette");setAttribute(Qt::WA_DeleteOnClose);setAttribute(Qt::WA_StyledBackground);
        auto *layout=new QVBoxLayout(this);layout->setContentsMargins(10,10,10,10);layout->setSpacing(8);
        search_=new QLineEdit;search_->setObjectName("commandPaletteSearch");search_->setPlaceholderText("Search commands and tools");search_->setClearButtonEnabled(true);
        search_->addAction(workbenchIcon("search"),QLineEdit::LeadingPosition);layout->addWidget(search_);
        list_=new QListWidget;list_->setObjectName("commandPaletteList");list_->setUniformItemSizes(true);layout->addWidget(list_,1);
        empty_=new QLabel("No matching commands");empty_->setObjectName("commandPaletteEmpty");empty_->setAlignment(Qt::AlignCenter);layout->addWidget(empty_);
        connect(search_,&QLineEdit::textChanged,this,[this]{filter();});
        connect(list_,&QListWidget::itemActivated,this,[this](QListWidgetItem *item){run(item);});
        search_->installEventFilter(this);resize(560,420);filter();
    }
    // Collects enabled leaf actions from a menu bar, labelled "Menu › Submenu › Command".
    static QList<Entry> menuEntries(QMenuBar *bar){
        QList<Entry> entries;
        std::function<void(QMenu *,QString)> walk=[&](QMenu *menu,QString path){
            for(auto *action:menu->actions()){
                if(action->isSeparator()||!action->isVisible())continue;
                const auto text=QString(action->text()).remove('&').remove(QChar(0x2026)).remove("...").trimmed();if(text.isEmpty())continue;
                if(action->menu()){walk(action->menu(),path+" › "+text);continue;}
                if(!action->isEnabled())continue;
                QPointer<QAction> target(action);entries.append({path+" › "+text,[target]{if(target)target->trigger();}});
            }
        };
        for(auto *action:bar->actions())if(action->menu()&&action->isVisible())walk(action->menu(),QString(action->text()).remove('&'));
        return entries;
    }
    void popup(){
        if(auto *owner=parentWidget()){const auto frame=owner->geometry();move(frame.center().x()-width()/2,frame.top()+qMax(40,frame.height()/6));}
        show();search_->setFocus();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(object==search_&&event->type()==QEvent::KeyPress){
            auto *key=static_cast<QKeyEvent *>(event);
            if(key->key()==Qt::Key_Down||key->key()==Qt::Key_Up){step(key->key()==Qt::Key_Down?1:-1);return true;}
            if(key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter){run(list_->currentItem());return true;}
            if(key->key()==Qt::Key_Escape){close();return true;}
        }
        return QDialog::eventFilter(object,event);
    }
private:
    void filter(){
        list_->clear();const auto words=search_->text().split(' ',Qt::SkipEmptyParts);
        for(int i=0;i<entries_.size();++i){
            bool match=true;for(const auto &word:words)match&=entries_[i].label.contains(word,Qt::CaseInsensitive);
            if(!match)continue;auto *item=new QListWidgetItem(entries_[i].label,list_);item->setData(Qt::UserRole,i);
        }
        list_->setCurrentRow(list_->count()?0:-1);empty_->setVisible(list_->count()==0);list_->setVisible(list_->count()>0);
    }
    void step(int delta){if(!list_->count())return;list_->setCurrentRow(qBound(0,list_->currentRow()+delta,list_->count()-1));}
    void run(QListWidgetItem *item){
        if(!item)return;const auto action=entries_.value(item->data(Qt::UserRole).toInt()).run;
        close();if(action)action();
    }
    QList<Entry> entries_;QLineEdit *search_;QListWidget *list_;QLabel *empty_;
};
