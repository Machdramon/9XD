#pragma once
#include <QDialog>
#include "theme.h"
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QBoxLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <array>

// Integer arithmetic deliberately wraps to the selected word size.
class ProgrammerCalculator final : public QDialog {
public:
    explicit ProgrammerCalculator(QWidget *parent=nullptr):QDialog(parent){
        setObjectName("programmerCalculator");setWindowTitle("Programmer Calculator");resize(440,650);
        setStyleSheet(R"(
            QDialog#programmerCalculator { background: palette(window); }
            QDialog#programmerCalculator QLineEdit { background: transparent; border: none; padding: 4px 2px; color: palette(window-text); selection-background-color: palette(highlight); selection-color: palette(highlighted-text); }
            QDialog#programmerCalculator QLabel[calculatorRole="caption"] { color: palette(placeholder-text); }
            QDialog#programmerCalculator QPushButton { border: 1px solid transparent; border-radius: 5px; padding: 6px; background: transparent; color: palette(window-text); }
            QDialog#programmerCalculator QPushButton[calculatorRole="hex"], QDialog#programmerCalculator QPushButton[calculatorRole="bits"] { color: palette(link); }
            QDialog#programmerCalculator QPushButton[calculatorRole="operator"], QDialog#programmerCalculator QPushButton[calculatorRole="edit"] { background: palette(alternate-base); }
            QDialog#programmerCalculator QPushButton:hover { background: palette(button); }
            QDialog#programmerCalculator QPushButton:pressed, QDialog#programmerCalculator QPushButton:checked { background: palette(highlight); color: palette(highlighted-text); }
            QDialog#programmerCalculator QPushButton:focus { border-color: palette(highlight); }
            QDialog#programmerCalculator QPushButton[calculatorRole="equals"] { background: palette(highlight); color: palette(highlighted-text); font-weight: bold; }
            QDialog#programmerCalculator QPushButton[calculatorRole="equals"]:hover { border-color: palette(window-text); }
            QDialog#programmerCalculator QPushButton:disabled { color: palette(placeholder-text); background: transparent; }
        )");
        auto *layout=new QVBoxLayout(this);layout->setSpacing(10);layout->setContentsMargins(18,16,18,16);auto *options=new QHBoxLayout;
        base_=new QComboBox;base_->setObjectName("calculatorBase");for(auto pair:{std::pair{"HEX",16}, {"DEC",10},{"OCT",8},{"BIN",2}})base_->addItem(pair.first,pair.second);base_->setCurrentIndex(1);
        word_=new QComboBox;word_->setObjectName("calculatorWord");for(int bits:{64,32,16,8})word_->addItem(QString::number(bits)+" bit",bits);
        signed_=new QCheckBox("Signed");signed_->setObjectName("calculatorSigned");options->addWidget(base_);options->addWidget(word_);options->addWidget(signed_);options->addStretch();layout->addLayout(options);
        expression_=new QLabel;expression_->setObjectName("calculatorExpression");layout->addWidget(expression_);
        display_=new QLineEdit("0");display_->setObjectName("calculatorDisplay");display_->setReadOnly(true);display_->setFrame(false);display_->setAlignment(Qt::AlignRight);auto font=display_->font();font.setPointSize(font.pointSize()+8);display_->setFont(font);layout->addWidget(display_);
        auto *conversions=new QGridLayout;int row=0;for(auto name:{"HEX","DEC","OCT","BIN"}){auto *label=new QLabel(name);label->setProperty("calculatorRole","caption");conversions->addWidget(label,row,0);auto *value=new QLineEdit;value->setReadOnly(true);value->setFrame(false);value->setAlignment(Qt::AlignRight);value->setObjectName(QString("calculator%1").arg(name));conversions->addWidget(value,row,1);values_[row++]=value;}layout->addLayout(conversions);
        auto *memoryTitle=new QLabel("MEMORY");memoryTitle->setProperty("calculatorRole","caption");layout->addWidget(memoryTitle);auto *memory=new QHBoxLayout;for(auto text:{"MC","MR","MS","M+","M−","Copy"}){auto *button=new QPushButton(text);button->setAutoDefault(false);button->setProperty("calculatorRole","memory");memory->addWidget(button);connect(button,&QPushButton::clicked,this,[this,text]{QString key=text;if(key=="Copy")QApplication::clipboard()->setText(display_->text());else if(key=="MC")memory_=0;else if(key=="MR"){value_=memory_&mask();fresh_=true;}else if(key=="MS")memory_=value_;else if(key=="M+")memory_=(memory_+value_)&mask();else memory_=(memory_-value_)&mask();update();});}layout->addLayout(memory);
        auto *bitTitle=new QLabel("BIT OPERATIONS");bitTitle->setProperty("calculatorRole","caption");layout->addWidget(bitTitle);
        auto *bitKeys=new QGridLayout;bitKeys->setSpacing(5);auto *grid=new QGridLayout;grid->setHorizontalSpacing(5);grid->setVerticalSpacing(5);
        QStringList bitActions={"AND","OR","XOR","NOT","<<",">>","ROL","ROR","<< 1",">> 1"};
        QStringList keys={"A","B","7","8","9","÷","C (hex)","D","4","5","6","×","E","F","1","2","3","−","±","⌫","0","%","=","+"};
        auto addKey=[this](QGridLayout *target,const QString &text,int index,const QString &role){auto *button=new QPushButton(text);button->setMinimumHeight(36);button->setAutoDefault(false);target->addWidget(button,index/6,index%6);QString key=text=="C (hex)"?"C":text=="C"?"Clear":text;button->setObjectName("calculatorKey_"+key);button->setProperty("calculatorRole",role);connect(button,&QPushButton::clicked,this,[this,key]{press(key);});if(key.size()==1&&QString("0123456789ABCDEF").contains(key))digits_.append(button);};
        int i=0;for(const auto &text:bitActions)addKey(bitKeys,text,i++,"bits");addKey(bitKeys,"C",10,"edit");addKey(bitKeys,"CE",11,"edit");layout->addLayout(bitKeys);
        auto *numberTitle=new QLabel("NUMBERS & ARITHMETIC");numberTitle->setProperty("calculatorRole","caption");layout->addWidget(numberTitle);
        i=0;for(const auto &text:keys){QString role="operator";if(text=="=")role="equals";else if(text=="⌫")role="edit";else if(text=="C (hex)"||(text.size()==1&&QString("ABCDEF").contains(text)))role="hex";else if(text.size()==1&&text[0].isDigit())role="digit";addKey(grid,text,i++,role);}
        layout->addLayout(grid);auto *bitsButton=new QPushButton("Show / hide bits");layout->addWidget(bitsButton);bits_=new QWidget;auto *bitGrid=new QGridLayout(bits_);bitGrid->setSpacing(2);for(int bit=63;bit>=0;--bit){auto *button=new QPushButton("0");button->setCheckable(true);button->setMinimumWidth(0);button->setToolTip("Bit "+QString::number(bit));bitButtons_[bit]=button;bitGrid->addWidget(button,(63-bit)/16,(63-bit)%16);connect(button,&QPushButton::clicked,this,[this,bit]{value_^=quint64(1)<<bit;fresh_=true;update();});}layout->addWidget(bits_);bits_->hide();connect(bitsButton,&QPushButton::clicked,this,[this]{bits_->setVisible(bits_->isHidden());});
        auto *hint=new QLabel("Enter = result · Esc = clear · Backspace = delete\n+ − * / % · & | ^ ~ · < > shift · Ctrl+C / Ctrl+V\nOperations run from left to right. Results wrap to the word size.");hint->setProperty("calculatorRole","caption");hint->setWordWrap(true);layout->addWidget(hint);
        connect(base_,&QComboBox::currentIndexChanged,this,[this]{fresh_=true;update();});connect(word_,&QComboBox::currentIndexChanged,this,[this]{value_&=mask();left_&=mask();memory_&=mask();fresh_=true;update();});connect(signed_,&QCheckBox::toggled,this,[this]{update();});
        auto baseStyle=styleSheet();auto refreshColors=[this,baseStyle]{QString css=baseStyle;for(auto role:{"digit","hex","operator","bits","memory"}){auto key=QString("calculator.")+role;auto color=resolvedThemeColor(key,qApp->palette(),QJsonObject::fromVariantMap(qApp->property("workbenchThemeColors").toMap()));css+=QString("QDialog#programmerCalculator QPushButton[calculatorRole=\"%1\"]:enabled { color: %2; }").arg(role,color.name());}setStyleSheet(css);};connect(qApp,&QApplication::paletteChanged,this,[refreshColors](const QPalette &){refreshColors();});refreshColors();
        installEventFilter(this);for(auto *widget:findChildren<QWidget *>())widget->installEventFilter(this);update();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::KeyPress){auto *key=static_cast<QKeyEvent *>(event);if(key->modifiers()&Qt::ControlModifier){if(key->key()==Qt::Key_C){QApplication::clipboard()->setText(display_->text());return true;}if(key->key()==Qt::Key_V){paste();return true;}return QDialog::eventFilter(object,event);}
            QString action;if(key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter)action="=";else if(key->key()==Qt::Key_Escape)action="Clear";else if(key->key()==Qt::Key_Backspace)action="⌫";else if(key->key()==Qt::Key_Delete)action="CE";else{auto text=key->text().toUpper();if(text.size()==1&&QString("0123456789ABCDEF+-*/%&|^~<>=").contains(text))action=text;}
            if(!action.isEmpty()){press(action);return true;}}
        return QDialog::eventFilter(object,event);
    }
private:
    int width()const{return word_->currentData().toInt();}int radix()const{return base_->currentData().toInt();}
    quint64 mask()const{return width()==64?~quint64(0):(quint64(1)<<width())-1;}
    bool negative(quint64 value)const{return signed_->isChecked()&&(value&(quint64(1)<<(width()-1)));}
    QString decimal(quint64 value)const{return negative(value)?"-"+QString::number((~value+1)&mask()):QString::number(value);}
    void update(){value_&=mask();display_->setText(radix()==10?decimal(value_):QString::number(value_,radix()).toUpper());values_[0]->setText(QString::number(value_,16).toUpper());values_[1]->setText(decimal(value_));values_[2]->setText(QString::number(value_,8));values_[3]->setText(QString::number(value_,2));expression_->setText(error_.isEmpty()?(pending_.isEmpty()?QString():QString::number(left_,radix()).toUpper()+" "+pending_):error_);for(auto *button:digits_){auto text=button->text();int digit=text=="C (hex)"?12:QString("0123456789ABCDEF").indexOf(text);button->setEnabled(digit<radix());}for(int bit=0;bit<64;++bit){bitButtons_[bit]->setEnabled(bit<width());bitButtons_[bit]->setText((value_&(quint64(1)<<bit))?"1":"0");bitButtons_[bit]->setChecked(value_&(quint64(1)<<bit));}}
    bool calculate(){auto right=value_;if((pending_=="÷"||pending_=="%")&&right==0){error_="Cannot divide by zero";pending_.clear();fresh_=true;return false;}if(pending_=="+")value_=left_+right;else if(pending_=="−")value_=left_-right;else if(pending_=="×")value_=left_*right;else if(pending_=="÷"||pending_=="%"){
            bool ln=negative(left_),rn=negative(right);auto a=ln?(~left_+1)&mask():left_;auto b=rn?(~right+1)&mask():right;auto result=pending_=="÷"?a/b:a%b;bool neg=pending_=="÷"?ln!=rn:ln;value_=neg?~result+1:result;
        }else if(pending_=="AND")value_=left_&right;else if(pending_=="OR")value_=left_|right;else if(pending_=="XOR")value_=left_^right;else if(pending_=="<<")value_=right>=quint64(width())?0:left_<<right;else if(pending_==">>")value_=shiftRight(left_,right);value_&=mask();return true;}
    quint64 shiftRight(quint64 value,quint64 count)const{if(count>=quint64(width()))return negative(value)?mask():0;if(!count)return value;auto result=value>>count;if(negative(value))result|=mask()^(mask()>>count);return result;}
    void paste(){auto text=QApplication::clipboard()->text().trimmed();bool neg=text.startsWith('-');if(neg)text.remove(0,1);int base=radix();if(text.startsWith("0x",Qt::CaseInsensitive)){base=16;text.remove(0,2);}else if(text.startsWith("0b",Qt::CaseInsensitive)){base=2;text.remove(0,2);}else if(text.startsWith("0c",Qt::CaseInsensitive)){base=8;text.remove(0,2);}bool ok=false;auto value=text.toULongLong(&ok,base);if(!ok){error_="Invalid integer for the selected base";}else{error_.clear();value_=(neg?~value+1:value)&mask();fresh_=true;}update();}
    void press(QString key){if(key=="-")key="−";if(key=="*")key="×";if(key=="/")key="÷";if(key=="&")key="AND";if(key=="|")key="OR";if(key=="^")key="XOR";if(key=="~")key="NOT";if(key=="<")key="<<";if(key==">")key=">>";error_.clear();int digit=key.size()==1?QString("0123456789ABCDEF").indexOf(key):-1;
        if(digit>=0){if(digit>=radix())return;if(fresh_){value_=0;fresh_=false;}value_=(value_*radix()+digit)&mask();}
        else if(key=="Clear"){value_=left_=0;pending_.clear();fresh_=true;}else if(key=="CE"){value_=0;fresh_=true;}else if(key=="⌫"){value_/=radix();fresh_=false;}else if(key=="±"){value_=(~value_+1)&mask();}else if(key=="NOT")value_=~value_&mask();else if(key=="<< 1")value_=(value_<<1)&mask();else if(key==">> 1")value_=shiftRight(value_,1);else if(key=="ROL"){value_=((value_<<1)|(value_>>(width()-1)))&mask();}else if(key=="ROR"){value_=(value_>>1)|((value_&1)<<(width()-1));}
        else if(key=="="){if(!pending_.isEmpty())calculate();pending_.clear();fresh_=true;}else{if(!pending_.isEmpty()&&!fresh_&&!calculate()){update();return;}left_=value_;pending_=key;fresh_=true;}update();
    }
    QComboBox *base_,*word_;QCheckBox *signed_;QLabel *expression_;QLineEdit *display_;std::array<QLineEdit *,4> values_;QList<QPushButton *> digits_;std::array<QPushButton *,64> bitButtons_;QWidget *bits_;quint64 value_=0,left_=0,memory_=0;QString pending_,error_;bool fresh_=true;
};
