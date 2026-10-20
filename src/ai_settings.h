#pragma once
#include "ollama_client.h"
#include <QApplication>
#include <QSettings>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QBoxLayout>
#include <QJsonArray>
inline QSettings aiSettings(){auto file=qApp->property("workspaceSettingsFile").toString();if(file.isEmpty())return QSettings("BinaryWorkbench","Workspace");return QSettings(file,QSettings::IniFormat);}
inline QStringList ollamaLocalModels(const QJsonObject &response){QStringList result;for(auto value:response["models"].toArray()){auto model=value.toObject();auto name=model["name"].toString();if(!name.isEmpty()&&name.size()<=128&&!name.contains(":cloud",Qt::CaseInsensitive)&&model["remote_host"].toString().isEmpty()&&model["remote_model"].toString().isEmpty()&&result.size()<128)result.append(name);}return result;}
class AiSettingsPage final:public QWidget {
public:
    AiSettingsPage(QSettings &settings,QWidget *parent=nullptr):QWidget(parent){auto *layout=new QVBoxLayout(this);auto *form=new QFormLayout;layout->addLayout(form);auto *provider=new QComboBox;provider->addItem("Ollama · local (no API key)");form->addRow("Provider",provider);endpoint=new QLineEdit(settings.value("ai/endpoint","http://127.0.0.1:11434").toString());endpoint->setObjectName("aiEndpoint");form->addRow("Local address",endpoint);model=new QComboBox;model->setObjectName("aiModel");auto saved=settings.value("ai/model").toString();if(!saved.isEmpty())model->addItem(saved);form->addRow("Model",model);auto *test=new QPushButton("Connect / Refresh Models");form->addRow(test);context=new QSpinBox;context->setRange(4096,16384);context->setSingleStep(4096);context->setValue(settings.value("ai/context",8192).toInt());form->addRow("Context tokens",context);auto *replyHelp=new QLabel("Short answers: Finding, Evidence, Next step. Up to 80 words requested; 1,536 internal tokens per step.");replyHelp->setWordWrap(true);form->addRow("Response style",replyHelp);instructions=new QPlainTextEdit;instructions->setObjectName("aiInstructions");instructions->setPlaceholderText("Optional project purpose or instructions. Included with every question.");instructions->setPlainText(settings.value("ai/instructions").toString().left(4096));layout->addWidget(new QLabel("Additional project context"));layout->addWidget(instructions,1);status=new QLabel("Install Ollama and a local model separately. No model or runtime is bundled. Cloud providers are not connected in this version.");status->setWordWrap(true);layout->addWidget(status);connect(test,&QPushButton::clicked,this,[this,test]{test->setEnabled(false);client.request(endpoint->text(),"/api/tags",{},false,[this](QJsonObject response){auto previous=model->currentText();model->clear();model->addItems(ollamaLocalModels(response));int index=model->findText(previous);if(index>=0)model->setCurrentIndex(index);status->setText(model->count()?"Connected. Local models available: "+QString::number(model->count()):"Connected, but no local models found.");},[this,test](QString error){test->setEnabled(true);if(!error.isEmpty())status->setText(error);});});}
    bool save(QSettings &settings,QString &error){if(!localOllamaUrl(QUrl(endpoint->text()))){error="AI Assistance requires a local HTTP address (for example http://127.0.0.1:11434).";return false;}if(instructions->toPlainText().size()>4096){error="Additional AI context is limited to 4096 characters.";return false;}settings.setValue("ai/endpoint",endpoint->text());settings.setValue("ai/model",model->currentText());settings.setValue("ai/context",context->value());settings.setValue("ai/instructions",instructions->toPlainText());return true;}
    void defaults(){endpoint->setText("http://127.0.0.1:11434");context->setValue(8192);instructions->clear();}
    QLineEdit *endpoint;QComboBox *model;QSpinBox *context;QPlainTextEdit *instructions;QLabel *status;OllamaClient client;
};
