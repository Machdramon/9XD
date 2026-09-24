#pragma once
#include <QWidget>
class HexView;
class EntropyPanel final:public QWidget {
public:
    explicit EntropyPanel(HexView *,QWidget *parent=nullptr);
};
