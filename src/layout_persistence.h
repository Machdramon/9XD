#pragma once
#include <QObject>
#include <QByteArray>
#include <QTimer>
class QMainWindow;
class UiLayout final:public QObject {
public:
    UiLayout(QMainWindow *,QString filePath);
    static QString defaultPath();
    void saveAndStop();
    void reset();
protected:
    bool eventFilter(QObject *,QEvent *)override;
private:
    QMainWindow *window_;QString path_;QTimer timer_;
    QByteArray defaultState_,defaultGeometry_;
    bool enabled_=false,started_=false;
    void schedule();void save();void restore();void keepPanelsOnScreen();
};
