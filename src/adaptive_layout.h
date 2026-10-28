#pragma once
#include <QApplication>
#include <QWidget>
#include <QBoxLayout>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPointer>
#include <QEvent>
#include <functional>
// Panels that work both wide (bottom docks) and narrow (side columns, the Inspector stack, HUD cards):
//   flip()    switches a box layout or splitter between side by side and stacked as its owner narrows. The width
//             is one setting for the whole application (Settings › Appearance › Interface mode, interface/sideBySide);
//   section() wraps part of a panel in a titled section that folds to its title ("▾ Matches");
//   tool()    a small text/icon button for compact action rows.
namespace AdaptiveLayout {
// Panels are side by side from this width (in pixels) up; narrower panels stack.
inline int sideBySideWidth(){const int width=qApp?qApp->property("adaptiveSideBySide").toInt():0;return width>0?width:640;}
class Flipper final : public QObject {
public:
    Flipper(QWidget *owner,QBoxLayout *box,QSplitter *split,int offset):QObject(owner),owner_(owner),box_(box),split_(split),offset_(offset){owner->installEventFilter(this);all().append(this);apply();}
    ~Flipper()override{all().removeAll(this);}
    static QList<QPointer<Flipper>> &all(){static QList<QPointer<Flipper>> list;return list;}
    void apply(){
        if(!owner_)return;const bool narrow=owner_->width()<sideBySideWidth()+offset_;
        if(box_){
            const auto direction=narrow?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight;if(box_->direction()!=direction)box_->setDirection(direction);
            // Stacked, the first part (the query or settings) takes only the height it needs; the rest goes to results.
            if(compactFirst&&box_->count()>0){if(firstStretch_<0)firstStretch_=box_->stretch(0);box_->setStretch(0,narrow?0:firstStretch_);if(auto *first=box_->itemAt(0)->widget())first->setSizePolicy(QSizePolicy::Preferred,narrow?QSizePolicy::Maximum:QSizePolicy::Preferred);}
        }
        if(split_){const auto orientation=narrow?Qt::Vertical:Qt::Horizontal;if(split_->orientation()!=orientation)split_->setOrientation(orientation);}
        if(onChange&&int(narrow)!=narrow_)onChange(narrow);narrow_=int(narrow);
    }
    std::function<void(bool)> onChange;bool compactFirst=false;
protected:
    bool eventFilter(QObject *,QEvent *event)override{if(event->type()==QEvent::Resize||event->type()==QEvent::Show)apply();return false;}
private:
    QPointer<QWidget> owner_;QPointer<QBoxLayout> box_;QPointer<QSplitter> split_;int offset_;int narrow_=-1;int firstStretch_=-1;
};
// offset: added to the global width for panels that need more (or less) room before going side by side.
inline Flipper *flip(QWidget *owner,QBoxLayout *box,int offset=0){return new Flipper(owner,box,nullptr,offset);}
// As flip(), for a query/settings part followed by results: stacked, the first part keeps its natural height.
inline Flipper *flipColumns(QWidget *owner,QBoxLayout *box,int offset=0){auto *flipper=new Flipper(owner,box,nullptr,offset);flipper->compactFirst=true;flipper->apply();return flipper;}
inline Flipper *flip(QWidget *owner,QSplitter *split,int offset=0){return new Flipper(owner,nullptr,split,offset);}
// After the setting changes.
inline void refreshAll(){for(const auto &flipper:Flipper::all())if(flipper)flipper->apply();}
// A titled, foldable section. Folded sections shrink to their title so the open ones get the room.
inline QWidget *section(const QString &title,QWidget *content,const QString &name,bool open=true){
    auto *box=new QWidget;box->setObjectName(name);auto *layout=new QVBoxLayout(box);layout->setContentsMargins(0,0,0,0);layout->setSpacing(2);
    auto *toggle=new QToolButton;toggle->setObjectName("inspectorDisclosure");toggle->setProperty("sectionToggle",name);toggle->setCheckable(true);toggle->setChecked(open);toggle->setAutoRaise(true);toggle->setToolButtonStyle(Qt::ToolButtonTextOnly);
    layout->addWidget(toggle,0,Qt::AlignLeft);layout->addWidget(content,1);
    auto refresh=[toggle,content,box,title](bool shown){
        toggle->setText(QString(shown?QChar(0x25BE):QChar(0x25B8))+" "+title);content->setVisible(shown);
        box->setSizePolicy(shown?QSizePolicy::Preferred:QSizePolicy::Maximum,shown?QSizePolicy::Expanding:QSizePolicy::Maximum);
    };
    refresh(open);QObject::connect(toggle,&QToolButton::toggled,box,refresh);
    return box;
}
// Opens a folded section (for example when an action needs what it holds).
inline void open(QWidget *section){if(!section)return;for(auto *toggle:section->findChildren<QToolButton *>("inspectorDisclosure",Qt::FindDirectChildrenOnly))toggle->setChecked(true);}
inline QToolButton *tool(QHBoxLayout *row,const QString &text,const QString &tooltip){
    auto *button=new QToolButton;button->setText(text);button->setToolTip(tooltip);button->setAccessibleName(tooltip);button->setProperty("keepTooltip",true);button->setProperty("compactTool",true);
    row->addWidget(button);return button;
}
}
