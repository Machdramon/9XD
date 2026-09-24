#pragma once
#include "magic_bytes.h"
#include <QWidget>
class HexView; class FormatPanel; class QTreeWidget; class QTableWidget; class QComboBox; class QLabel; class QPushButton;
class MagicPanel final:public QWidget {
public:
    MagicPanel(HexView *view,FormatPanel *format,QWidget *parent=nullptr);
    bool confirmClose();
private:
    void rebuild();
    void edit(bool create);
    void invalidate();
    bool exportLibrary();
    void scan();
    void mark();
    HexView *view_;FormatPanel *format_;
    QList<MagicSignature> library_=defaultMagicLibrary(), scanned_;
    QTreeWidget *libraryList_;QTableWidget *results_;QComboBox *scope_;QLabel *status_;QPushButton *start_;
    QVector<ByteRange> matches_;QList<int> types_;
    bool dirty_=false,stale_=true,anchored_=false;
    qint64 total_=0;
};
