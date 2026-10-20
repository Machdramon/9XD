#pragma once
#include "model_export.h"
// Inputs have already passed the common export validation and axis conversion.
QByteArray encodeFbx(const QVector<ExportMesh> &,const ModelExportOptions &,const std::shared_ptr<ExportProgress> &);
