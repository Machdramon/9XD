#include "vulkan_panel.h"
#include "hex_view.h"
#include "local_config.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QThreadPool>
#include <QScrollArea>
#include <QFileInfo>
namespace {
class Controls final:public QWidget {
public:
    HexView *view;VulkanPanel *renderer;QThreadPool pool;QFutureWatcher<MeshResult> watcher;quint64 revision=0,started=0;
    std::shared_ptr<MeshData> targetSnapshot;int replaceId=-1;QString sourceName;QComboBox *destination;
    QLineEdit *vo,*vc,*vs,*io,*ic,*is,*uo,*us,*no,*ns;QComboBox *vt,*it,*ut,*nt,*topology,*endian;QCheckBox *uv,*normals;QDoubleSpinBox *scale;QLabel *status;QPushButton *render;
    Controls(VulkanPanel *r,HexView *v):view(v),renderer(r){setObjectName("meshControls");pool.setMaxThreadCount(1);auto *layout=new QVBoxLayout(this);
        auto section=[&](QString name){auto *box=new QGroupBox(name);layout->addWidget(box);return new QFormLayout(box);};
        auto field=[](QFormLayout *f,QString name,QString value,QString id){auto *w=new QLineEdit(value);w->setObjectName(id);w->setMaxLength(20);f->addRow(name,w);return w;};
        auto combo=[](QFormLayout *f,QString name,QStringList values){auto *w=new QComboBox;w->addItems(values);f->addRow(name,w);return w;};
        auto *f=section("Vértices · XYZ");vo=field(f,"Offset","0x80","meshVertexOffset");vc=field(f,"Cantidad","128","meshVertexCount");vs=field(f,"Stride (bytes)","12","meshVertexStride");vt=combo(f,"Tipo",{"Int32","Int16","Float32","Float16"});scale=new QDoubleSpinBox;scale->setDecimals(8);scale->setRange(.00000001,1000000);scale->setValue(.0001);f->addRow("Escala",scale);
        f=section("Caras");io=field(f,"Offset","0x680","meshIndexOffset");ic=field(f,"Cantidad de índices","402","meshIndexCount");is=field(f,"Stride (bytes)","2","meshIndexStride");it=combo(f,"Tipo",{"UInt16","UInt32"});topology=combo(f,"Topología",{"Triangle strip · reinicio FF","Lista de triángulos"});
        f=section("UV · un par por vértice");uv=new QCheckBox("Leer UV");uv->setChecked(true);f->addRow(uv);uo=field(f,"Offset","0x9B0","meshUvOffset");us=field(f,"Stride (bytes)","4","meshUvStride");ut=combo(f,"Tipo",{"Float16","Float32"});
        f=section("Normales · XYZ por vértice");normals=new QCheckBox("Leer normales (desmarcar para calcular)");normals->setChecked(true);f->addRow(normals);no=field(f,"Offset","0xBB0","meshNormalOffset");ns=field(f,"Stride (bytes)","12","meshNormalStride");nt=combo(f,"Tipo",{"Float32","SNorm16"});
        f=section("Lectura");endian=combo(f,"Orden",{"Little endian","Big endian"});
        auto *sample=new QPushButton("Abrir esfera de prueba (preset)");layout->addWidget(sample);connect(sample,&QPushButton::clicked,this,[this]{if(view->dirty()||view->saving()||view->directEditing()){status->setText("Guarda o descarta las ediciones antes de abrir el ejemplo desde Archivo.");return;}if(!view->formatRules().isEmpty()){status->setText("Abre sphere128.bin desde Archivo para conservar o exportar las reglas actuales.");return;}vo->setText("0x80");vc->setText("128");vs->setText("12");vt->setCurrentText("Int32");scale->setValue(.0001);io->setText("0x680");ic->setText("402");is->setText("2");it->setCurrentText("UInt16");topology->setCurrentIndex(0);uv->setChecked(true);uo->setText("0x9B0");us->setText("4");ut->setCurrentText("Float16");normals->setChecked(true);no->setText("0xBB0");ns->setText("12");nt->setCurrentText("Float32");endian->setCurrentIndex(0);view->openFile(WORKBENCH_SPHERE);status->setText("Esfera abierta. Pulsa Renderizar cuando termine la lectura.");});
        destination=new QComboBox;destination->setObjectName("meshDestination");destination->addItems({"Añadir a la colección","Actualizar malla activa"});layout->addWidget(destination);
        render=new QPushButton("Renderizar archivo actual");render->setObjectName("renderMesh");layout->addWidget(render);status=new QLabel("Offsets absolutos; stride = tamaño completo de cada registro. Reinicio: 0xFFFF / 0xFFFFFFFF según tipo.");status->setObjectName("meshStatus");status->setWordWrap(true);layout->addWidget(status);
        connect(render,&QPushButton::clicked,this,[this]{load();});
        connect(view,&HexView::documentChanged,this,[this]{++revision;renderer->scene()->markStale();status->setText("El archivo cambió; las mallas conservan su captura anterior. Renderiza para añadir o actualizar.");});
        for(auto *w:findChildren<QLineEdit *>())connect(w,&QLineEdit::textChanged,this,[this]{++revision;status->setText("Parámetros modificados; pulsa Renderizar para actualizar la vista.");});for(auto *w:findChildren<QComboBox *>())connect(w,&QComboBox::currentIndexChanged,this,[this]{++revision;});for(auto *w:findChildren<QCheckBox *>())connect(w,&QCheckBox::toggled,this,[this]{++revision;});connect(scale,&QDoubleSpinBox::valueChanged,this,[this]{++revision;});
        connect(&watcher,&QFutureWatcher<MeshResult>::finished,this,[this]{render->setEnabled(true);if(started!=revision){status->setText("Resultado descartado: cambiaron el archivo o los parámetros.");return;}auto result=watcher.result();if(!result.error.isEmpty()){status->setText(result.error);return;}if(replaceId>=0){auto *target=renderer->scene()->entry(replaceId);if(!target||target->data!=targetSnapshot){status->setText("El destino cambió durante la lectura; resultado descartado.");return;}if(!renderer->scene()->replace(replaceId,result.mesh,sourceName)){status->setText("La malla de destino fue eliminada; resultado descartado.");return;}}else renderer->scene()->add(result.mesh,{},sourceName);status->setText(QString("%1 vértices · %2 triángulos cargados. Abre Vista 3D · Vulkan.").arg(result.mesh->vertices.size()).arg(result.mesh->triangles.size()/3));});
    }
    ~Controls()override{watcher.waitForFinished();}
    void load(){if(watcher.isRunning()||view->saving()||view->directEditing()||!view->hasFile()){status->setText("Espera a la lectura/edición o abre un archivo.");return;}MeshSpec s;bool valid=true;auto read=[&](QLineEdit *w){bool ok;auto t=w->text().trimmed();auto value=t.toLongLong(&ok,t.startsWith("0x",Qt::CaseInsensitive)?16:10);valid&=ok&&value>=0;return value;};s.vertexOffset=read(vo);s.vertexCount=read(vc);s.vertexStride=read(vs);s.indexOffset=read(io);s.indexCount=read(ic);s.indexStride=read(is);s.uvOffset=read(uo);s.uvStride=read(us);s.normalOffset=read(no);s.normalStride=read(ns);if(!valid){status->setText("Introduce cantidades decimales u offsets 0x… válidos.");return;}s.positionType=vt->currentText();s.indexType=it->currentText();s.uvType=ut->currentText();s.normalType=nt->currentText();s.uv=uv->isChecked();s.normals=normals->isChecked();s.strip=topology->currentIndex()==0;s.bigEndian=endian->currentIndex()==1;s.positionScale=scale->value();if(destination->currentIndex()==1&&renderer->scene()->active()<0){status->setText("Selecciona una malla para actualizar.");return;}replaceId=destination->currentIndex()==1?renderer->scene()->active():-1;targetSnapshot=replaceId<0?nullptr:renderer->scene()->entry(replaceId)->data;auto source=view->meshSource();sourceName=source.path;auto patches=view->pendingPatches();started=revision;render->setEnabled(false);status->setText("Decodificando malla en segundo plano…");watcher.setFuture(QtConcurrent::run(&pool,[source,patches,s]{try{return readMesh(source,patches,s);}catch(const std::exception &e){return MeshResult{{},QString::fromUtf8(e.what())};}}));}
};
}
QWidget *VulkanPanel::createMeshControls(HexView *view){auto *container=new QWidget;auto *layout=new QVBoxLayout(container);auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);auto *controls=new Controls(this,view);scroll->setWidget(controls);layout->addWidget(scroll,1);layout->addWidget(controls->render);layout->addWidget(controls->status);return container;}
