#include "byte_format.h"
#include <QPainter>
#include <QJsonArray>
#include <QVariantMap>
#include <QtEndian>
#include <limits>
#include <algorithm>

bool FormatRule::matches(qint64 offset) const {
    if(!enabled)return false;
    if(!matchRanges.isEmpty()){
        auto it=std::upper_bound(matchRanges.begin(),matchRanges.end(),offset,[](qint64 value,const ByteRange &r){return value<r.offset;});
        if(it==matchRanges.begin())return false;--it;return offset-it->offset<it->length;
    }
    if(selectionRule)return selection.contains(offset);
    if(offset<start||length<=0||step<length||repetitions<=0)return false;
    const auto relative=offset-start;
    return relative/step<repetitions&&relative%step<length;
}
QList<DecodedToken> decodeTokens(const QByteArray &bytes,qint64 offset,const QString &encoding,const QString &type,bool bigEndian,bool normalizeControls){
    QList<DecodedToken> tokens;
    for(qsizetype i=0;i<bytes.size();){
        int width=1;
        if(type!="Text")width=numberWidth(type);
        else if(encoding=="UTF-8"){
            const auto lead=quint8(bytes[i]);
            int candidate=lead>=0xC2&&lead<=0xDF?2:lead>=0xE0&&lead<=0xEF?3:lead>=0xF0&&lead<=0xF4?4:1;
            bool valid=i+candidate<=bytes.size();
            for(int j=1;valid&&j<candidate;++j)valid=(quint8(bytes[i+j])&0xC0)==0x80;
            if(valid&&candidate>=3){const auto second=quint8(bytes[i+1]);valid=!(lead==0xE0&&second<0xA0)&&!(lead==0xED&&second>=0xA0)&&!(lead==0xF0&&second<0x90)&&!(lead==0xF4&&second>=0x90);}
            if(valid)width=candidate;
        }else if(encoding=="UTF-16LE"||encoding=="UTF-16BE"){
            width=2;
            if(i+4<=bytes.size()){
                const auto *data=reinterpret_cast<const uchar *>(bytes.constData()+i);
                auto a=encoding=="UTF-16LE"?qFromLittleEndian<quint16>(data):qFromBigEndian<quint16>(data);
                auto b=encoding=="UTF-16LE"?qFromLittleEndian<quint16>(data+2):qFromBigEndian<quint16>(data+2);
                if(a>=0xD800&&a<=0xDBFF&&b>=0xDC00&&b<=0xDFFF)width=4;
            }
        }else if(encoding=="CP932"){
            const auto lead=quint8(bytes[i]);
            if(((lead>=0x81&&lead<=0x9F)||(lead>=0xE0&&lead<=0xFC))&&i+1<bytes.size()){
                const auto trail=quint8(bytes[i+1]);if(trail>=0x40&&trail<=0xFC&&trail!=0x7F)width=2;
            }
        }
        width=qMin(width,int(bytes.size()-i));
        auto text=type=="Text"?decodeText(bytes.mid(i,width),encoding):decodeNumber(bytes.mid(i,width),type,bigEndian);
        if(type=="Text"&&text.startsWith("[incomplete"))text=QString(QChar::ReplacementCharacter);
        if(type=="Text"&&normalizeControls)for(auto &c:text)if(c.isSpace()||c.category()==QChar::Other_Control)c=QChar('.');
        tokens.append({text,offset+i,width});i+=width;
    }
    return tokens;
}
QJsonObject encodeRule(const FormatRule &r){
    auto color=[](const QColor &c){return c.isValid()?c.name(QColor::HexArgb):QString();};
    QJsonArray matches;for(const auto &range:r.matchRanges)matches.append(QJsonArray{QString::number(range.offset),QString::number(range.length)});
    return {{"jumpTo",QString::number(r.jumpTo)},{"underlineColor",color(r.style.underlineColor)},{"matches",matches},{"name",r.name},{"enabled",r.enabled},{"selectionRule",r.selectionRule},
        {"start",QString::number(r.start)},{"length",QString::number(r.length)},
        {"step",QString::number(r.step)},{"repetitions",QString::number(r.repetitions)},
        {"anchor",QString::number(r.selection.anchor)},{"cursor",QString::number(r.selection.cursor)},
        {"stride",r.selection.stride},{"rectangular",r.selection.rectangular},
        {"background",color(r.style.background)},{"foreground",color(r.style.foreground)},{"border",color(r.style.border)},
        {"bold",r.style.bold},{"italic",r.style.italic},{"underline",r.style.underline}};
}
bool decodeRule(const QJsonObject &j,FormatRule &r){
    r=FormatRule{};
    auto number=[&](const char *key,qint64 &out){bool ok=false;out=j.value(key).toString().toLongLong(&ok);return ok;};
    if(j.contains("matches")&&!j["matches"].isArray())return false;
    if(j["matches"].toArray().size()>10000)return false;
    qint64 previous=-1,previousEnd=-1;
    for(const auto &entry:j["matches"].toArray()){
        const auto a=entry.toArray();if(a.size()!=2)return false;bool ok1,ok2;
        auto start=a[0].toString().toLongLong(&ok1),length=a[1].toString().toLongLong(&ok2);
        if(!ok1||!ok2||start<0||length<=0||start>std::numeric_limits<qint64>::max()-length||start<=previous||start+length<previousEnd)return false;
        r.matchRanges.append({start,length});previous=start;previousEnd=start+length;
    }
    if(j.contains("jumpTo")&&(!number("jumpTo",r.jumpTo)||r.jumpTo < -1))return false;
    r.name=j["name"].toString();r.enabled=j["enabled"].toBool(true);r.selectionRule=j["selectionRule"].toBool();
    if(r.name.size()>200||!number("start",r.start)||!number("length",r.length)||!number("step",r.step)||!number("repetitions",r.repetitions)||!number("anchor",r.selection.anchor)||!number("cursor",r.selection.cursor))return false;
    r.selection.stride=j["stride"].toInt();r.selection.rectangular=j["rectangular"].toBool();
    if(r.selection.stride<1||r.selection.stride>256)return false;
    if(r.selectionRule){if(r.selection.anchor<0||r.selection.cursor<0)return false;}
    else if(r.start<0||r.length<1||r.step<r.length||r.repetitions<1||r.start>std::numeric_limits<qint64>::max()-r.length||r.repetitions-1>(std::numeric_limits<qint64>::max()-r.start-r.length)/r.step)return false;
    auto color=[&](const char *key,QColor &out){auto value=j[key].toString();out=QColor(value);return value.isEmpty()||out.isValid();};
    if(!color("background",r.style.background)||!color("foreground",r.style.foreground)||!color("border",r.style.border)||!color("underlineColor",r.style.underlineColor))return false;
    r.style.bold=j["bold"].toBool();r.style.italic=j["italic"].toBool();r.style.underline=j["underline"].toBool();return true;
}
void ByteFormatDelegate::paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index)const{
    const auto tokens=index.data(TokensRole).toList();
    if(tokens.isEmpty()){QStyledItemDelegate::paint(p,option,index);return;}
    p->save();p->setClipRect(option.rect);p->fillRect(option.rect,index.row()%2?option.palette.alternateBase():option.palette.base());
    const bool hex=index.data(HexRole).toBool();qreal x=option.rect.left()+4;
    for(qsizetype i=0;i<tokens.size();++i){
        const auto token=tokens[i].toMap();const auto offset=token["offset"].toLongLong(),length=token["length"].toLongLong();
        const auto resolved=resolver_(offset,length);const auto &s=resolved.style;
        QFont font=option.font;font.setBold(s.bold);font.setItalic(s.italic);font.setUnderline(s.underline&&!s.underlineColor.isValid());p->setFont(font);
        const QFontMetricsF metrics(font);const auto text=token["text"].toString();
        const qreal width=hex?option.rect.width():qMax(qreal(8),metrics.horizontalAdvance(text));
        QRectF rect(hex?option.rect.left():x,option.rect.top()+1,width,option.rect.height()-2);
        if(s.background.isValid())p->fillRect(rect,s.background);
        else if(resolved.selected)p->fillRect(rect,option.palette.highlight());
        p->setPen(s.foreground.isValid()?s.foreground:resolved.selected&&!s.background.isValid()?option.palette.highlightedText().color():option.palette.text().color());
        p->drawText(rect,Qt::AlignCenter,text);
        if(s.underline&&s.underlineColor.isValid()){p->setPen(QPen(s.underlineColor,1));const qreal half=metrics.horizontalAdvance(text)/2;const qreal y=rect.center().y()+(metrics.ascent()-metrics.descent())/2+metrics.underlinePos();p->drawLine(QPointF(rect.center().x()-half,y),QPointF(rect.center().x()+half,y));}
        if(s.border.isValid()){
            p->setPen(QPen(s.border,2));auto outline=rect.adjusted(1,1,-1,-1);
            p->drawLine(outline.topLeft(),outline.topRight());p->drawLine(outline.bottomLeft(),outline.bottomRight());
            const bool first=hex?index.data(RowFirstRole).toBool():i==0;
            const bool last=hex?index.data(RowLastRole).toBool():i==tokens.size()-1;
            const auto previous=resolver_(offset-1,1),next=resolver_(offset+length,1);
            if(first||previous.rule!=resolved.rule||previous.group!=resolved.group)p->drawLine(outline.topLeft(),outline.bottomLeft());
            if(last||next.rule!=resolved.rule||next.group!=resolved.group)p->drawLine(outline.topRight(),outline.bottomRight());
        }
        if(resolved.selected){p->setPen(QPen(option.palette.highlight().color().lighter(160),2));p->drawLine(QPointF(rect.left()+1,rect.bottom()-1),QPointF(rect.right()-1,rect.bottom()-1));}
        x+=width;
    }
    p->restore();
}
