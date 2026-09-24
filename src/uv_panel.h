#pragma once
#include <QWidget>
#include "vulkan_panel.h"
class UvCanvas:public QWidget {
    Q_OBJECT
public:
    explicit UvCanvas(VulkanPanel *,QWidget *parent=nullptr);
    QPointF uvToScreen(QPointF uv)const;
    void fit();
    bool activeOnly=true,labels=false;
protected:
    void paintEvent(QPaintEvent *)override;
    void mousePressEvent(QMouseEvent *)override;
    void mouseMoveEvent(QMouseEvent *)override;
    void mouseReleaseEvent(QMouseEvent *)override;
    void wheelEvent(QWheelEvent *)override;
private:
    VulkanPanel *renderer_;MeshScene *scene_;
    QPointF center_{.5,.5},press_,last_;double scale_=300;int cycle_=0;
    void pick(QPointF pixel,bool cycle);
};
class UvPanel:public QWidget {
public:
    explicit UvPanel(VulkanPanel *,QWidget *parent=nullptr);
};
