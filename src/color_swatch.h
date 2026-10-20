#pragma once
#include <QColor>
#include <cmath>
#include <QPushButton>
inline QColor swatchTextColor(const QColor &color){
    auto linear=[](double c){return c<=.04045?c/12.92:std::pow((c+.055)/1.055,2.4);};
    double light=.2126*linear(color.redF())+.7152*linear(color.greenF())+.0722*linear(color.blueF());
    return light>.179?QColor(Qt::black):QColor(Qt::white);
}
inline void showColorSwatch(QPushButton *button,const QColor &color){
    button->setText(color.name());button->setToolTip("Click to choose a color");button->setMinimumHeight(26);
    button->setStyleSheet(QString("QPushButton {background-color: %1; color: %2; border: 1px solid palette(mid); border-radius: 3px; padding: 4px 10px;}").arg(color.name(),swatchTextColor(color).name()));
}
