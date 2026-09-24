#pragma once
#include "interpretation.h"
#include <QColor>
#include <QJsonObject>
#include <QStyledItemDelegate>
#include <functional>

struct ByteStyle {
    QColor background, foreground, border;
    bool bold=false, italic=false, underline=false;
    QColor underlineColor;
};
struct FormatRule {
    QString name;
    qint64 jumpTo=-1; // -1 uses the first byte of the rule.
    bool enabled=true;
    ByteStyle style;
    // Selection rules retain their original rectangle stride.
    QVector<ByteRange> matchRanges;
    bool selectionRule=false;
    ByteSelection selection;
    qint64 start=0, length=1, step=1, repetitions=1;
    bool matches(qint64 offset) const;
};
struct StyledBytes { ByteStyle style; int rule=-1; qint64 group=-1; bool selected=false; };
struct DecodedToken { QString text; qint64 offset; qint64 length; };
QList<DecodedToken> decodeTokens(const QByteArray &bytes,qint64 offset,const QString &encoding,
                                const QString &type,bool bigEndian,bool normalizeControls=true);
QJsonObject encodeRule(const FormatRule &rule);
bool decodeRule(const QJsonObject &json,FormatRule &rule);

// Paints only visible cells. Decoded glyphs keep the source byte range they represent.
class ByteFormatDelegate final : public QStyledItemDelegate {
public:
    enum { TokensRole=Qt::UserRole+10, HexRole, RowFirstRole, RowLastRole };
    using Resolver=std::function<StyledBytes(qint64,qint64)>;
    ByteFormatDelegate(QObject *parent,Resolver resolver):QStyledItemDelegate(parent),resolver_(std::move(resolver)){}
    void paint(QPainter *,const QStyleOptionViewItem &,const QModelIndex &) const override;
private:
    Resolver resolver_;
};
