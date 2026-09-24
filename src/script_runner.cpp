#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "script_context.h"
#include "workbench_script_api.h"
#include "local_config.h"
#include <QCoreApplication>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLibrary>
#include <iostream>
namespace {
ReadRequest source;PatchList patches;QJsonObject context;QByteArray lastError;std::shared_ptr<MeshData> published,initial;
int readBytes(int64_t offset,int64_t count,uint8_t *output){if(offset<0||count<0||count>1048576||offset>source.expectedSize||count>source.expectedSize-offset){lastError="Lectura fuera del archivo o superior a 1 MiB.";return 0;}auto request=source;request.offset=offset;request.length=count;auto result=readPage(request);if(!result.error.isEmpty()){lastError=result.error.toUtf8();return 0;}applyPatches(result.bytes,offset,patches);if(count)std::memcpy(output,result.bytes.constData(),size_t(count));return 1;}
PyObject *pyRead(PyObject *,PyObject *args){long long offset,count;if(!PyArg_ParseTuple(args,"LL",&offset,&count))return nullptr;if(count<0||count>1048576){PyErr_SetString(PyExc_ValueError,"read: máximo 1 MiB");return nullptr;}auto *bytes=PyBytes_FromStringAndSize(nullptr,count);if(!bytes)return nullptr;if(!readBytes(offset,count,reinterpret_cast<uint8_t *>(PyBytes_AS_STRING(bytes)))){Py_DECREF(bytes);PyErr_SetString(PyExc_ValueError,lastError.constData());return nullptr;}return bytes;}
PyObject *pyPublish(PyObject *,PyObject *args){const char *text;Py_ssize_t length;if(!PyArg_ParseTuple(args,"s#",&text,&length))return nullptr;if(length>64*1024*1024){PyErr_SetString(PyExc_ValueError,"Malla demasiado grande");return nullptr;}QString error;auto doc=QJsonDocument::fromJson(QByteArray(text,length));auto mesh=scriptMeshFromJson(doc.object(),error);if(!mesh){PyErr_SetString(PyExc_ValueError,error.toUtf8().constData());return nullptr;}published=mesh;Py_RETURN_NONE;}
PyMethodDef methods[]={{"read",pyRead,METH_VARARGS,"read(offset, count): bytes, hasta 1 MiB; incluye ediciones pendientes."},{"_publish",pyPublish,METH_VARARGS,nullptr},{nullptr,nullptr,0,nullptr}};
PyModuleDef module={PyModuleDef_HEAD_INIT,"workbench",nullptr,-1,methods};
PyObject *initModule(){return PyModule_Create(&module);}
int publishCpp(const BWVertex *vertices,uint64_t count,const uint32_t *indices,uint64_t n){if(count>100000||n>600000||(count&&!vertices)||(n&&!indices)){lastError="Malla demasiado grande o punteros inválidos.";return 0;}auto m=std::make_shared<MeshData>();m->vertices.resize(count);m->triangles.resize(n);if(count)std::memcpy(m->vertices.data(),vertices,size_t(count)*sizeof(BWVertex));if(n)std::memcpy(m->triangles.data(),indices,size_t(n)*4);QString error;auto validated=scriptMeshFromJson(scriptMeshJson(m),error);if(!validated){lastError=error.toUtf8();return 0;}published=validated;return 1;}
}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);std::cout.setf(std::ios::unitbuf);std::cerr.setf(std::ios::unitbuf);auto args=app.arguments();if(args.size()!=5){std::cerr<<"Uso: runner python|cpp context script result\n";return 2;}
    try{
        QFile input(args[2]);if(!input.open(QIODevice::ReadOnly)||input.size()>64*1024*1024)throw std::runtime_error("Contexto no disponible o demasiado grande.");auto doc=QJsonDocument::fromJson(input.readAll());if(!doc.isObject())throw std::runtime_error("Contexto inválido.");context=doc.object();source={context["path"].toString(),0,context["size"].toString().toLongLong(),QDateTime::fromString(context["modified"].toString(),Qt::ISODateWithMs),0};for(auto v:context["patches"].toArray()){auto p=v.toObject();patches.append({p["offset"].toString().toLongLong(),QByteArray::fromBase64(p["bytes"].toString().toLatin1())});}QString error;initial=scriptMeshFromJson(context["mesh"].toObject(),error);if(!initial)throw std::runtime_error(error.toStdString());
        if(args[1]=="python"){
            if(PyImport_AppendInittab("workbench",initModule)<0)throw std::runtime_error("No se pudo registrar workbench.");PyConfig config;PyConfig_InitPythonConfig(&config);config.parse_argv=0;config.buffered_stdio=0;config.install_signal_handlers=0;auto set=PyConfig_SetBytesString(&config,&config.program_name,WORKBENCH_PYTHON_EXECUTABLE);if(PyStatus_Exception(set)){PyConfig_Clear(&config);throw std::runtime_error("Ruta de Python inválida.");}auto status=Py_InitializeFromConfig(&config);PyConfig_Clear(&config);if(PyStatus_Exception(status))throw std::runtime_error(status.err_msg?status.err_msg:"No se pudo inicializar Python.");
            auto *main=PyImport_AddModule("__main__");auto *globals=PyModule_GetDict(main);auto json=QJsonDocument(context).toJson(QJsonDocument::Compact);auto *value=PyUnicode_DecodeUTF8(json.constData(),json.size(),"strict");if(!value)throw std::runtime_error("Contexto Python inválido.");PyDict_SetItemString(globals,"_context_json",value);Py_DECREF(value);
            const char *bootstrap=R"PY(
import json, copy, workbench
_context = json.loads(_context_json)
workbench.size = int(_context['size'])
workbench.path = _context['path']
workbench.selection = dict(_context['selection'])
workbench.selection['start'] = int(workbench.selection['start'])
workbench.selection['end'] = int(workbench.selection['end'])
workbench.mesh = lambda: copy.deepcopy(_context['mesh'])
def _set_mesh(vertices, indices, normals=None, uvs=None):
    workbench._publish(json.dumps(dict(vertices=vertices, indices=indices, normals=normals, uvs=uvs), allow_nan=False))
workbench.set_mesh = _set_mesh
)PY";
            if(PyRun_SimpleString(bootstrap)!=0){Py_FinalizeEx();return 1;}QFile script(args[3]);if(!script.open(QIODevice::ReadOnly))throw std::runtime_error("Script no disponible.");auto code=script.readAll();auto *compiled=Py_CompileString(code.constData(),args[3].toUtf8().constData(),Py_file_input);auto *result=compiled?PyEval_EvalCode(compiled,globals,globals):nullptr;bool success=result!=nullptr;if(!success)PyErr_Print();Py_XDECREF(result);Py_XDECREF(compiled);if(Py_FinalizeEx()<0)success=false;if(!success)return 1;
        }else if(args[1]=="cpp"){
            static_assert(sizeof(BWVertex)==sizeof(MeshVertex));std::vector<BWVertex> vertices(initial->vertices.size());if(!vertices.empty())std::memcpy(vertices.data(),initial->vertices.constData(),vertices.size()*sizeof(BWVertex));QByteArray path=source.path.toUtf8();BWApi api{1,source.expectedSize,context["selection"].toObject()["start"].toString().toLongLong(),context["selection"].toObject()["end"].toString().toLongLong(),path.constData(),vertices.data(),uint64_t(initial->vertices.size()),initial->triangles.constData(),uint64_t(initial->triangles.size()),readBytes,publishCpp,[]()->const char *{return lastError.constData();}};
            QLibrary library(args[3]);auto run=reinterpret_cast<void (*)(const BWApi *)>(library.resolve("bw_run"));if(!run)throw std::runtime_error(library.errorString().toStdString());run(&api);
        }else throw std::runtime_error("Lenguaje no admitido.");
        QJsonObject output{{"ok",true}};if(published)output["mesh"]=scriptMeshJson(published);auto bytes=QJsonDocument(output).toJson(QJsonDocument::Compact);QSaveFile file(args[4]);if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())throw std::runtime_error("No se pudo devolver el resultado.");return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}catch(...){std::cerr<<"Excepción C++ desconocida\n";return 1;}
}
