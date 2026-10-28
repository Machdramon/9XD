#pragma once
#include <QWidget>
#include "interpretation.h"
class QLabel;
class QTableWidget;
class QComboBox;
class QCheckBox;
class DataInspector : public QWidget {
    Q_OBJECT
public:
    explicit DataInspector(QWidget *parent=nullptr);
    void setPlainText(const QString &message);
    QString toPlainText() const;
    void setData(const QByteArray &, const ByteSelection &, qint64 fileSize, const QString &encoding, bool bigEndian);
protected:
    void changeEvent(QEvent *event) override;
signals:
    void endianChanged(bool bigEndian);
private:
    void refresh();
    QLabel *position_;
    QTableWidget *values_=nullptr;
    QComboBox *endian_;
    QCheckBox *hex_;
    QByteArray bytes_;
    QString encoding_="ASCII";
};
