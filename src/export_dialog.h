#pragma once
#include <QDialog>
#include <QThreadPool>
#include <QFutureWatcher>
#include "model_export.h"
#include "mesh_scene.h"
class QComboBox;class QDoubleSpinBox;class QCheckBox;class QTreeWidget;class QLabel;class QPushButton;class QProgressBar;class QDialogButtonBox;
class ExportDialog final:public QDialog {
public:
    ExportDialog(MeshScene *,QString currentBinary,QColor defaultColor,QWidget *parent=nullptr);
    ~ExportDialog()override;
    ModelExportOptions options()const;
    QVector<ExportMesh> selectedMeshes()const;
    // Shared by the save dialog and integration tests. The caller handles overwrite confirmation.
    void startExport(const QString &path);
    void reject()override;
private:
    QVector<MeshEntry> entries_;
    QStringList protectedPaths_;
    QColor color_;
    QComboBox *format_,*axes_,*normals_;
    QDoubleSpinBox *scale_;
    QCheckBox *negate_[3],*uvs_,*flipV_,*reverse_,*material_,*doubleSided_;
    QTreeWidget *meshes_;
    QLabel *summary_,*status_;
    QPushButton *save_,*colorButton_;QWidget *settings_;
    QProgressBar *progressBar_;
    QThreadPool pool_;QFutureWatcher<ModelExportResult> watcher_;
    std::shared_ptr<ExportProgress> progress_;
    void refresh();
};
