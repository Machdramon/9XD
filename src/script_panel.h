#pragma once
#include <QWidget>
#include <QProcess>
#include <QTemporaryDir>
#include <QStringDecoder>
#include <memory>
#include "script_context.h"
class ScriptLibraryManager;
class HexView;class VulkanPanel;class QComboBox;class QPlainTextEdit;class QPushButton;class QLabel;
class ScriptPanel final:public QWidget {
public:
    ScriptPanel(HexView *,VulkanPanel *,QWidget *parent=nullptr);
    ~ScriptPanel()override;
    bool confirmClose();
    QWidget *libraryManager()const;
    void refreshLibraries();
private:
    struct Document {QString text,path;bool dirty=false;};
    Document docs_[2];int current_=0;bool loading_=false,compiling_=false,stopped_=false;
    quint64 revision_=0,runRevision_=0;qsizetype outputBytes_=0;
    std::shared_ptr<const MeshData> runMesh_;int runMeshId_=-1;
    ScriptLibraryManager *libraries_;
    HexView *view_;VulkanPanel *renderer_;QComboBox *language_;QPlainTextEdit *editor_,*console_;QPushButton *run_,*stop_;QLabel *status_;
    QStringDecoder outputDecoder_{QStringDecoder::Utf8};
    QProcess process_;std::unique_ptr<QTemporaryDir> directory_;
    bool save(int language);
    void execute();void launchRunner();void output();void finish(int,QProcess::ExitStatus);void log(const QString &);
};
