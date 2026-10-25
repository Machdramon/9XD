#pragma once
#include <QKeySequenceEdit>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSettings>
#include <QSet>
#include <QVector3D>
#include <QJsonArray>
#include <QtMath>
#include <cmath>
#include <atomic>
#include <array>
namespace CameraNavigation {
inline constexpr std::array<int,8> defaults{Qt::Key_W,Qt::Key_S,Qt::Key_A,Qt::Key_D,Qt::Key_Q,Qt::Key_E,Qt::Key_R,Qt::Key_F};
inline const std::array<const char *,8> labels{"Forward","Backward","Left","Right","Turn left","Turn right","Up","Down"};
inline std::array<std::atomic<int>,8> keys{Qt::Key_W,Qt::Key_S,Qt::Key_A,Qt::Key_D,Qt::Key_Q,Qt::Key_E,Qt::Key_R,Qt::Key_F};
inline std::atomic<double> speed{1.0};
inline void load(QSettings &settings){QSet<int> used;std::array<int,8> loaded{};bool valid=true;for(int i=0;i<8;++i){loaded[i]=settings.value(QString("3d/navigation/key%1").arg(i),defaults[i]).toInt();valid&=loaded[i]>0&&!(loaded[i]&Qt::KeyboardModifierMask)&&loaded[i]!=Qt::Key_Escape&&loaded[i]!=Qt::Key_Shift&&!used.contains(loaded[i]);used.insert(loaded[i]);}for(int i=0;i<8;++i)keys[i].store(valid?loaded[i]:defaults[i]);speed.store(qBound(.05,settings.value("3d/navigation/speed",1.0).toDouble(),20.0));}
inline bool handles(int key){for(auto &value:keys)if(value.load()==key)return true;return false;}
inline void step(QVector3D &target,float &yaw,float pitch,float distance,const QSet<int> &held,double seconds,bool fast,bool orthographic=false){
    seconds=qBound(0.,seconds,.1);auto down=[&](int i){return held.contains(keys[i].load())?1.f:0.f;};float angle=qDegreesToRadians(yaw),elevation=qDegreesToRadians(pitch);QVector3D outward(std::cos(elevation)*std::sin(angle),std::sin(elevation),std::cos(elevation)*std::cos(angle));auto eye=target+outward*distance;
    float turn=orthographic?0:down(4)-down(5);yaw+=turn*float(seconds)*75.f;angle=qDegreesToRadians(yaw);QVector3D forward(-std::cos(elevation)*std::sin(angle),-std::sin(elevation),-std::cos(elevation)*std::cos(angle));if(turn)target=eye+forward*distance;
    QVector3D right(std::cos(angle),0,-std::sin(angle));if(orthographic)forward=QVector3D::crossProduct(right,forward).normalized();auto movement=forward*(down(0)-down(1))+right*(down(3)-down(2))+QVector3D(0,down(6)-down(7),0);if(movement.lengthSquared()>0)target+=movement.normalized()*float(seconds*speed.load()*(fast?4:1))*qMax(.05f,distance*.5f);
}
class Page:public QWidget {
public:
    std::array<QKeySequenceEdit *,8> edits{};QDoubleSpinBox *movement;
    explicit Page(QSettings &settings){auto *form=new QFormLayout(this);auto *note=new QLabel("Click a 3D viewport to use these keys. Hold Shift to move faster. Controls stop when the viewport loses focus. Use one distinct key per action; shortcuts with modifiers are reserved for the application.");note->setWordWrap(true);form->addRow(note);for(int i=0;i<8;++i){edits[i]=new QKeySequenceEdit(QKeySequence(keys[i].load()));edits[i]->setMaximumSequenceLength(1);edits[i]->setObjectName(QString("settingsCameraKey%1").arg(i));form->addRow(labels[i],edits[i]);}movement=new QDoubleSpinBox;movement->setObjectName("settingsCameraSpeed");movement->setRange(.05,20);movement->setSingleStep(.25);movement->setValue(speed.load());form->addRow("Movement speed",movement);Q_UNUSED(settings);}
    void restore(){for(int i=0;i<8;++i)edits[i]->setKeySequence(QKeySequence(defaults[i]));movement->setValue(1);}
    bool validate(QString &error)const{QSet<int> used;for(int i=0;i<8;++i){auto sequence=edits[i]->keySequence();int key=sequence.isEmpty()?0:sequence[0].toCombined();if(!key||(key&Qt::KeyboardModifierMask)||key==Qt::Key_Escape||key==Qt::Key_Shift||used.contains(key)){error="Assign one unique key to each camera action, without Ctrl, Alt, Shift or Meta. Escape is reserved.";return false;}used.insert(key);}return true;}
    bool save(QSettings &settings,QString &error){if(!validate(error))return false;for(int i=0;i<8;++i)settings.setValue(QString("3d/navigation/key%1").arg(i),edits[i]->keySequence()[0].toCombined());settings.setValue("3d/navigation/speed",movement->value());return true;}
};
}
