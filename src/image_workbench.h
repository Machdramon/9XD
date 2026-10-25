#pragma once
#include "empty_state.h"
#include "file_dialogs.h"
#include "hex_view.h"
#include "image_decode.h"
#include "image_recipe_io.h"
#include "memory_budget.h"
#include "ai_decoder_setup.h"
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QListWidget>
#include <QFileDialog>
#include <QDirIterator>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QTabWidget>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

class ImagePreview final:public QGraphicsView {
public:
    explicit ImagePreview(QWidget *parent=nullptr):QGraphicsView(parent){setObjectName("imagePreview");setScene(new QGraphicsScene(this));addEmptyState(this,"image","No image to preview","Select an image in the collection, or calculate one with the settings on the right.");setDragMode(ScrollHandDrag);setTransformationAnchor(AnchorUnderMouse);setBackgroundBrush(palette().brush(QPalette::Dark));}
    void showImage(const QImage &image){scene()->clear();if(!image.isNull())scene()->addPixmap(QPixmap::fromImage(image));scene()->setSceneRect(QRectF(QPointF(0,0),image.size()));resetTransform();if(!image.isNull())fitInView(scene()->sceneRect(),Qt::KeepAspectRatio);}
protected:
    void wheelEvent(QWheelEvent *event)override{const double factor=std::exp(event->angleDelta().y()*.001);const auto next=transform().m11()*factor;if(next>=.01&&next<=128)scale(factor,factor);event->accept();}
};

// One collection per document; workers only receive immutable source/recipe copies.
class ImageWorkbench final:public QObject {
public:
    struct Entry{QString name;ReadRequest source;PatchList patches;ImageRecipe recipe;QImage image;bool document=false;};
    explicit ImageWorkbench(HexView *view,QObject *parent):QObject(parent),view_(view),state_(std::make_shared<Job>()),reader_(nullptr,[state=state_](const ReadRequest &source){state->decoded=decodeBinaryImage(source,state->patches,state->recipe,state->cancel);ReadResult result;result.error=state->decoded.error;result.cancelled=state->decoded.cancelled;return result;}){
        setObjectName("imageWorkbench");preview=new ImagePreview;
        collection=new QWidget;collection->setObjectName("imageCollectionPanel");auto *layout=new QVBoxLayout(collection);
        auto *open=new QPushButton("Open Folder...");open->setObjectName("imageOpenFolder");layout->addWidget(open);
        auto *openFiles=new QPushButton("Open Files...");openFiles->setObjectName("imageOpenFiles");layout->addWidget(openFiles);
        filter_=new QLineEdit;filter_->setPlaceholderText("Filter library...");layout->addWidget(filter_);
        list_=new QListWidget;list_->setObjectName("imageLibrary");list_->setIconSize({64,64});layout->addWidget(list_,1);addEmptyState(list_,"image","No images in the collection","Open files or a folder, or calculate an image from the current document.");
        auto *exportImage=new QPushButton("Export Selected PNG...");layout->addWidget(exportImage);
        auto *save=new QPushButton("Save Library...");auto *load=new QPushButton("Open Library...");auto *libraryButtons=new QHBoxLayout;libraryButtons->addWidget(save);libraryButtons->addWidget(load);layout->addLayout(libraryButtons);
        auto *remove=new QPushButton("Remove Selected");layout->addWidget(remove);
        auto *libraryHint=new QLabel("Select a file, then calculate with the settings on the right. Generated images are snapshots. Libraries save image snapshots, not source binaries.");libraryHint->setWordWrap(true);layout->addWidget(libraryHint);
        settings=new QWidget;settings->setObjectName("imageCalculatorPanel");auto *controls=new QVBoxLayout(settings);auto *form=new QFormLayout;controls->addLayout(form);
        auto *presets=new QComboBox;presets->setObjectName("imagePreset");presets->addItem("Custom settings");for(const auto &preset:imagePresets())presets->addItem(preset.name);form->addRow("Preset",presets);
        auto *recipeButtons=new QHBoxLayout;auto *saveRecipeButton=new QPushButton("Save Recipe..."),*loadRecipeButton=new QPushButton("Load Recipe...");recipeButtons->addWidget(saveRecipeButton);recipeButtons->addWidget(loadRecipeButton);controls->addLayout(recipeButtons);
        connect(saveRecipeButton,&QPushButton::clicked,this,[this]{auto path=QFileDialog::getSaveFileName(settings,"Save image recipe",{},"Image recipe (*.bwir)");if(!path.isEmpty())saveRecipe(path);});
        connect(loadRecipeButton,&QPushButton::clicked,this,[this]{auto path=WorkbenchFiles::getOpenFileName(settings,"Load image recipe",{},"Image recipe (*.bwir)");if(!path.isEmpty())loadRecipe(path);});
        source_=new QComboBox;source_->setObjectName("imageSource");source_->addItems({"Active document","Selected library file"});form->addRow("Source",source_);
        format_=new QComboBox;format_->setObjectName("imageFormat");format_->addItems(imageFormats());format_->setCurrentText("RGBA8");format_->setMaxVisibleItems(16);form->addRow("Pixel format",format_);
        memoryLayout_=new QComboBox;memoryLayout_->setObjectName("imageMemoryLayout");memoryLayout_->addItems({"Linear","Morton Z-order","Morton rectangular","Tiled 8x8","PS2 GS pages","PS2 GS 16S","Xbox 360 tiled","PS4 1D thin"});form->addRow("Memory layout / Unswizzle",memoryLayout_);
        auto *formatsLabel=new QLabel("Encoded images: "+QString::fromLatin1(QImageReader::supportedImageFormats().join(", "))+", tga, dds, tim, tm2, tpl, gtf, gnf");formatsLabel->setWordWrap(true);form->addRow(formatsLabel);
        container_=new QComboBox;container_->setObjectName("imageContainer");container_->addItems(imageContainers());form->addRow("Container hint",container_);
        mip_=number(0,13,0,"imageMipLevel");surface_=number(0,2047,0,"imageSurface");form->addRow("Container mip level",mip_);form->addRow("Image / surface / page",surface_);
        byteSwap_=new QComboBox;byteSwap_->setObjectName("imageByteSwap");byteSwap_->addItems({"None","Swap 8 in 16","Swap 8 in 32","Swap 16 in 32"});form->addRow("Raw byte swap",byteSwap_);
        offset_=field("0","imageOffset");form->addRow("Start offset",offset_);
        auto *selection=new QPushButton("Use Hex Selection Start");form->addRow(selection);
        width_=number(1,8192,256,"imageWidth");height_=number(1,8192,256,"imageHeight");form->addRow("Width",width_);form->addRow("Height",height_);
        stride_=field("0","imageRowStride");stride_->setToolTip("Bytes between row starts. 0 = tightly packed.");form->addRow("Row stride",stride_);
        endian_=new QComboBox;endian_->addItems({"Little Endian","Big Endian"});form->addRow("16-bit byte order",endian_);
        palette_=field("0","imagePaletteOffset");form->addRow("Palette offset",palette_);
        paletteFormat_=new QComboBox;paletteFormat_->setObjectName("imagePaletteFormat");paletteFormat_->addItems(imagePaletteFormats());form->addRow("Palette format",paletteFormat_);
        paletteBank_=number(0,4095,0,"imagePaletteBank");paletteEntries_=number(0,16384,0,"imagePaletteEntries");paletteEntries_->setSpecialValueText("Automatic");form->addRow("Palette bank",paletteBank_);form->addRow("Palette entries",paletteEntries_);
        lowNibble_=new QCheckBox("Low nibble first (4 bpp)");lowNibble_->setObjectName("imageLowNibble");clut_=new QCheckBox("PS2 CSM1 palette order");clut_->setObjectName("imagePs2Clut");alpha_=new QCheckBox("PS2 alpha range (0–128)");form->addRow(lowNibble_);form->addRow(clut_);form->addRow(alpha_);
        auto *paletteHint=new QLabel("Raw indexed/planar textures use a palette at the supplied offset. Bank selects a group of colors. NES palettes here contain RGB colors, not PPU color IDs. Offsets and strides accept decimal or 0x hex.");paletteHint->setWordWrap(true);form->addRow(paletteHint);
        flipX_=new QCheckBox("Flip horizontally");flipY_=new QCheckBox("Flip vertically");form->addRow(flipX_);form->addRow(flipY_);
        count_=number(1,128,1,"imageCount");form->addRow("Images to generate",count_);step_=field("0","imageStep");step_->setToolTip("Bytes between image starts. 0 = calculated image span.");form->addRow("Image step",step_);
        footprint_=new QLabel;footprint_->setWordWrap(true);form->addRow(footprint_);
        auto *row=new QHBoxLayout;calculate_=new QPushButton("Calculate / Add to Library");calculate_->setObjectName("imageCalculate");cancel_=new QPushButton("Cancel");cancel_->setObjectName("imageCancel");row->addWidget(calculate_);row->addWidget(cancel_);controls->addLayout(row);
        progress_=new QProgressBar;progress_->setObjectName("imageProgress");controls->addWidget(progress_);status_=new QLabel("Choose a source and pixel layout.");status_->setObjectName("imageStatus");status_->setWordWrap(true);controls->addWidget(status_);controls->addStretch();
        auto *assign=new QPushButton("Assign to active mesh");assign->setObjectName("imageAssignTexture");auto *unassign=new QPushButton("Remove active mesh texture");unassign->setObjectName("imageRemoveTexture");layout->addWidget(assign);layout->addWidget(unassign);
        connect(assign,&QPushButton::clicked,this,[this]{int row=list_->currentRow();if(row<0||entries_[row].image.isNull()){status_->setText("Select a decoded image first.");return;}if(assignTexture)status_->setText(assignTexture(entries_[row].image,entries_[row].name)?"Texture assigned to the active mesh.":"Choose a mesh with UVs and enough texture memory in Settings > Performance.");});
        connect(unassign,&QPushButton::clicked,this,[this]{if(assignTexture)assignTexture({},{});});
        connect(presets,&QComboBox::activated,this,[this](int index){if(index<1||running_)return;auto recipe=imagePresets()[index-1].recipe;recipe.width=width_->value();recipe.height=height_->value();bool ok;recipe.offset=parse(offset_->text(),ok);recipe.paletteOffset=parse(palette_->text(),ok);applyRecipe(recipe);status_->setText("Preset loaded. Check dimensions, offsets and palette against your source before calculating.");});
        connect(openFiles,&QPushButton::clicked,this,[this]{const auto paths=WorkbenchFiles::getOpenFileNames(collection,"Open images or binary files",{},"Images (*.png *.jpg *.jpeg *.bmp *.tga *.gif *.ico *.pbm *.pgm *.ppm *.xbm *.xpm *.tif *.tiff *.webp *.dds *.tim *.tm2 *.tpl *.gtf *.gnf);;All files (*)");for(const auto &path:paths)addFile(path);});
        connect(open,&QPushButton::clicked,this,[this]{const auto path=WorkbenchFiles::getExistingDirectory(collection,"Open image / binary folder");if(!path.isEmpty())openFolder(path);});
        connect(filter_,&QLineEdit::textChanged,this,[this](const QString &text){for(int i=0;i<list_->count();++i)list_->item(i)->setHidden(!list_->item(i)->text().contains(text,Qt::CaseInsensitive));});
        connect(list_,&QListWidget::currentRowChanged,this,[this](int row){if(row<0||row>=entries_.size())return;const auto &entry=entries_[row];source_->setCurrentIndex(1);preview->showImage(entry.image);if(!entry.image.isNull()&&imageSelected)imageSelected(entry.image);if(!entry.image.isNull())status_->setText(QString("%1 · %2 × %3 · %4").arg(entry.name).arg(entry.image.width()).arg(entry.image.height()).arg(entry.recipe.format));else {if(QImageReader::supportedImageFormats().contains(QFileInfo(entry.source.path).suffix().toLatin1().toLower())||QStringList{"tga","dds","tim","tm2","tpl","gtf","gnf"}.contains(QFileInfo(entry.source.path).suffix().toLower()))format_->setCurrentText("Encoded image");status_->setText("Source selected. Choose a pixel format and click Calculate.");}});
        connect(remove,&QPushButton::clicked,this,[this]{if(running_)return;int row=list_->currentRow();if(row<0)return;retained_-=entries_[row].image.sizeInBytes();entries_.removeAt(row);delete list_->takeItem(row);if(entries_.isEmpty())preview->showImage({});});
        connect(exportImage,&QPushButton::clicked,this,[this]{const int row=list_->currentRow();if(row<0||entries_[row].image.isNull())return;const auto path=QFileDialog::getSaveFileName(collection,"Export image",entries_[row].name+".png","PNG image (*.png)");if(!path.isEmpty())status_->setText(entries_[row].image.save(path,"PNG")?"Image exported.":"Could not export the image.");});
        connect(save,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getSaveFileName(collection,"Save image library",{},"Image library (*.bwil)");if(!path.isEmpty())saveLibrary(path);});
        connect(load,&QPushButton::clicked,this,[this]{const auto path=WorkbenchFiles::getOpenFileName(collection,"Open image library",{},"Image library (*.bwil)");if(!path.isEmpty())loadLibrary(path);});
        connect(selection,&QPushButton::clicked,this,[this]{auto s=view_->selection();if(s.anchor>=0){offset_->setText(QString("0x%1").arg(qMin(s.anchor,s.cursor),0,16));source_->setCurrentIndex(0);}});
        connect(calculate_,&QPushButton::clicked,this,[this]{calculate();});connect(cancel_,&QPushButton::clicked,this,[this]{state_->cancel.store(true);queue_.clear();status_->setText("Canceling...");});
        connect(view_,&HexView::documentChanged,this,[this]{if(running_&&documentJob_){state_->cancel.store(true);queue_.clear();}status_->setText("Document changed. Existing images are snapshots; calculate again to refresh.");});
        connect(&reader_,&BinaryReader::ready,this,[this](const ReadResult &result){if(!result.error.isEmpty())state_->decoded.error=result.error;finish();});
        auto refresh=[this]{QString error;auto recipe=currentRecipe(error);const auto span=imageSpan(recipe,error);footprint_->setText(error.isEmpty()?QString("%1 bits/pixel · %2 bytes/image\nBC6H preview uses tone mapping. Console layouts must match the source.").arg(imagePixelBits(recipe.format)).arg(span):error);bool encoded=recipe.format=="Encoded image";memoryLayout_->setEnabled(!encoded&&!running_);width_->setEnabled(!encoded&&!running_);height_->setEnabled(!encoded&&!running_);stride_->setEnabled(!encoded&&!running_);const bool indexed=imageIndexed(recipe.format);palette_->setEnabled(indexed&&!running_);paletteFormat_->setEnabled(indexed&&!running_);paletteBank_->setEnabled((indexed||encoded)&&!running_);paletteEntries_->setEnabled(indexed&&!running_);endian_->setEnabled(!encoded&&!running_);container_->setEnabled(encoded&&!running_);mip_->setEnabled(encoded&&!running_);surface_->setEnabled(encoded&&!running_);};
        refresh_ = refresh;connect(memoryLayout_,&QComboBox::currentIndexChanged,this,refresh);connect(format_,&QComboBox::currentIndexChanged,this,refresh);connect(width_,&QSpinBox::valueChanged,this,refresh);connect(height_,&QSpinBox::valueChanged,this,refresh);connect(stride_,&QLineEdit::textChanged,this,refresh);refresh();setRunning(false);
    }
    ~ImageWorkbench()override{state_->cancel.store(true);reader_.shutdown();}
    ImagePreview *preview;QWidget *collection,*settings;
    std::function<void(const QImage &)> imageSelected;
    std::function<bool(const QImage &,const QString &)> assignTexture;
    std::function<void(bool,const QString &)> calculationFinished;
    bool addDecodedTexture(const QImage &image,const QString &name,const QString &source,bool select=false){
        if(image.isNull()||!imageValidSize(image.width(),image.height()))return false;for(int i=0;i<entries_.size();++i){const auto &e=entries_[i];if(e.name==name&&e.source.path==source&&e.image==image){if(select)list_->setCurrentRow(i);return true;}}
        if(entries_.size()>=qBound(qint64(512),AssetMemory::textureBytes()/4096,qint64(10000))||retained_+image.sizeInBytes()>AssetMemory::textureBytes())return false;Entry e;e.name=name;e.source.path=source;e.source.expectedSize=-1;e.recipe.format="Encoded image";e.image=image;append(e);if(select)list_->setCurrentRow(entries_.size()-1);return true;
    }
    int imageCount()const{return entries_.size();}
    const Entry &entry(int index)const{return entries_[index];}
    bool busy()const{return running_;}
    void applyRecipe(const ImageRecipe &r){
        format_->setCurrentText(r.format);memoryLayout_->setCurrentText(r.layout);paletteFormat_->setCurrentText(r.paletteFormat);byteSwap_->setCurrentText(r.byteSwap);container_->setCurrentText(r.container);width_->setValue(r.width);height_->setValue(r.height);offset_->setText(QString::number(r.offset));stride_->setText(QString::number(r.rowStride));palette_->setText(QString::number(r.paletteOffset));mip_->setValue(r.mipLevel);surface_->setValue(r.surface);paletteBank_->setValue(r.paletteBank);paletteEntries_->setValue(r.paletteEntries);endian_->setCurrentIndex(r.bigEndian?1:0);flipX_->setChecked(r.flipX);flipY_->setChecked(r.flipY);lowNibble_->setChecked(r.lowNibbleFirst);clut_->setChecked(r.ps2Clut);alpha_->setChecked(r.ps2Alpha);if(refresh_)refresh_();
    }
    QJsonObject setupSnapshot(){QString error;auto r=currentRecipe(error);bool ok=false;auto step=parse(step_->text(),ok);return error.isEmpty()&&ok?encodeImageRecipe(r,count_->value(),step):QJsonObject{{"error","Current image settings are incomplete."}};}
    QJsonObject applyAssistantSetup(const QJsonObject &args){
        if(running_||view_->saving()||view_->directEditing())return {{"error","Wait for image calculation / document editing to finish."}};
        ImageRecipe r;int count;qint64 step;QString error;if(!AiDecoderSetup::image(args,view_->fileSize(),r,count,step,error))return {{"error",error}};
        applyRecipe(r);source_->setCurrentIndex(0);count_->setValue(count);step_->setText(QString::number(step));
        status_->setText("Assistant filled image settings. Review, then click Calculate / Add to Library.");
        return {{"status","Image settings filled for the active document; nothing calculated. Review and click Calculate / Add to Library."},{"settings",encodeImageRecipe(r,count,step)}};
    }
    QJsonObject calculateActiveDocument(){
        if(running_)return {{"error","An image calculation is already running."}};
        source_->setCurrentIndex(0);calculate();
        return running_?QJsonObject{{"started",true}}:QJsonObject{{"error",status_->text()}};
    }
    bool saveRecipe(const QString &path){
        if(running_)return false;QString error;auto r=currentRecipe(error);imageSpan(r,error);bool ok=false;auto step=parse(step_->text(),ok);if(!error.isEmpty()||!ok){status_->setText(error.isEmpty()?"Invalid image step.":error);return false;}auto bytes=QJsonDocument(encodeImageRecipe(r,count_->value(),step)).toJson();QSaveFile file(path);bool saved=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();status_->setText(saved?"Recipe saved; source bytes are not included.":"Could not save recipe.");return saved;
    }
    bool loadRecipe(const QString &path){
        if(running_)return false;QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>65536){status_->setText("Cannot open recipe (maximum 64 KiB).");return false;}QJsonParseError parseError;auto document=QJsonDocument::fromJson(file.readAll(),&parseError);ImageRecipe r;int count;qint64 step;QString error;if(parseError.error!=QJsonParseError::NoError||!document.isObject()||!decodeImageRecipe(document.object(),r,count,step,error)){status_->setText(error.isEmpty()?"Invalid recipe JSON.":error);return false;}applyRecipe(r);count_->setValue(count);step_->setText(QString::number(step));status_->setText("Recipe loaded. Choose its source and calculate.");return true;
    }
    void addFile(const QString &path){if(running_||entries_.size()>=512)return;QFileInfo info(path);if(!info.isFile())return;Entry entry;entry.name=info.fileName();entry.source={info.absoluteFilePath(),0,info.size(),info.lastModified(),0,{}};entry.recipe.format="Encoded image";append(entry);}
    void openFolder(const QString &path){
        if(running_)return;QDirIterator it(path,QDir::Files|QDir::NoDotAndDotDot,QDirIterator::NoIteratorFlags);int added=0;
        while(it.hasNext()&&entries_.size()<512){const auto name=it.next();QFileInfo info(name);Entry entry;entry.name=info.fileName();entry.source={info.absoluteFilePath(),0,info.size(),info.lastModified(),0,{}};entry.recipe.format="Encoded image";append(entry);++added;}
        status_->setText(QString("Added %1 files (folder only, up to 512 library entries). Select a file to interpret it.").arg(added));
    }
    void calculate(){
        if(running_)return;QString error;auto recipe=currentRecipe(error);const auto span=imageSpan(recipe,error);bool ok=false;auto step=parse(step_->text(),ok);if(!ok)error="Image step must be a nonnegative decimal or hexadecimal number.";
        if(!error.isEmpty()){status_->setText(error);return;}if(!step)step=span;
        Entry base;base.recipe=recipe;documentJob_=source_->currentIndex()==0;
        if(documentJob_){if(!view_->hasFile()||view_->saving()||view_->directEditing()){status_->setText("Open a document and wait for editing or saving to finish.");return;}base.source=view_->sourceRequest();base.patches=view_->pendingPatches();base.document=true;base.name=view_->isMemoryDocument()?"RAM image":QFileInfo(view_->filePath()).fileName();}
        else{const int row=list_->currentRow();if(row<0){status_->setText("Select a library file first.");return;}base.source=entries_[row].source;base.patches=entries_[row].patches;base.name=entries_[row].name;if(base.source.expectedSize<0){status_->setText("This saved snapshot has no binary source. Select another source.");return;}}
        if(recipe.format=="Encoded image"&&count_->value()!=1){status_->setText("Encoded images are decoded individually. Use a raw format for a batch.");return;}
        if(entries_.size()+count_->value()>512){status_->setText("Library limit: 512 entries. Remove entries first.");return;}
        const qint64 output=recipe.format=="Encoded image"?16*1024*1024:qint64(recipe.width)*recipe.height*4;
        if(output*count_->value()>AssetMemory::textureBytes()-retained_){status_->setText("The batch exceeds the configured image memory budget. Raise Settings > Performance, reduce dimensions/count or remove images.");return;}
        for(int i=0;i<count_->value();++i){auto entry=base;if(recipe.offset>base.source.expectedSize||qint64(i)> (base.source.expectedSize-recipe.offset)/qMax(qint64(1),step)){queue_.clear();status_->setText("The image batch extends outside the source.");return;}entry.recipe.offset=recipe.offset+i*step;if(recipe.format!="Encoded image"&&span>base.source.expectedSize-entry.recipe.offset){queue_.clear();status_->setText("The image batch extends outside the source.");return;}entry.name+=QString(" @0x%1").arg(entry.recipe.offset,0,16);queue_.append(entry);}
        completed_=0;total_=queue_.size();progress_->setRange(0,total_);progress_->setValue(0);setRunning(true);next();
    }
    void saveLibrary(const QString &path){
        if(running_)return;QJsonArray images;for(const auto &entry:entries_)if(!entry.image.isNull()){QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);entry.image.save(&buffer,"PNG");images.append(QJsonObject{{"name",entry.name},{"png",QString::fromLatin1(png.toBase64())}});}
        QSaveFile file(path);const auto data=QJsonDocument(QJsonObject{{"version",1},{"images",images}}).toJson(QJsonDocument::Compact);status_->setText(file.open(QIODevice::WriteOnly)&&file.write(data)==data.size()&&file.commit()?"Library saved (image snapshots).":"Could not save the library.");
    }
    void loadLibrary(const QString &path){
        if(running_)return;QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>192*1024*1024){status_->setText("Cannot open library (maximum 192 MiB).");return;}const auto root=QJsonDocument::fromJson(file.readAll()).object();if(root.value("version").toInt()!=1||!root.value("images").isArray()){status_->setText("Invalid image library.");return;}auto array=root.value("images").toArray();QList<Entry> imported;qint64 bytes=0;
        if(entries_.size()+array.size()>512){status_->setText("Library exceeds 512 entries.");return;}
        for(const auto &value:array){auto object=value.toObject();auto png=QByteArray::fromBase64(object.value("png").toString().toLatin1());QBuffer buffer(&png);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");auto size=reader.size();if(!size.isValid()||size.width()>8192||size.height()>8192||qint64(size.width())*size.height()>4*1024*1024||(bytes+=qint64(size.width())*size.height()*4)>AssetMemory::textureBytes()-retained_){status_->setText("Invalid image or library memory limit exceeded.");return;}Entry entry;entry.source.expectedSize=-1;entry.name=object.value("name").toString();entry.recipe.format="Encoded image";entry.image=reader.read().convertToFormat(QImage::Format_ARGB32);if(entry.image.isNull()){status_->setText("Invalid library image.");return;}imported.append(entry);}
        for(const auto &entry:imported)append(entry);status_->setText(QString("Loaded %1 image snapshots.").arg(imported.size()));
    }
private:
    struct Job{ImageRecipe recipe;PatchList patches;std::atomic_bool cancel{false};DecodedImage decoded;};
    static QLineEdit *field(const QString &text,const QString &name){auto *widget=new QLineEdit(text);widget->setObjectName(name);widget->setMaxLength(20);return widget;}
    static QSpinBox *number(int low,int high,int value,const QString &name){auto *widget=new QSpinBox;widget->setObjectName(name);widget->setRange(low,high);widget->setValue(value);return widget;}
    static qint64 parse(QString value,bool &ok){value=value.trimmed();auto n=value.startsWith("0x",Qt::CaseInsensitive)?value.mid(2).toLongLong(&ok,16):value.toLongLong(&ok,10);ok=ok&&n>=0;return n;}
    ImageRecipe currentRecipe(QString &error){ImageRecipe r;bool a,b,c;r.offset=parse(offset_->text(),a);r.rowStride=parse(stride_->text(),b);r.paletteOffset=parse(palette_->text(),c);if(!a||!b||!c)error="Offsets and strides must be nonnegative decimal or hexadecimal numbers.";r.width=width_->value();r.height=height_->value();r.format=format_->currentText();r.layout=memoryLayout_->currentText();r.paletteFormat=paletteFormat_->currentText();r.byteSwap=byteSwap_->currentText();r.container=container_->currentText();r.mipLevel=mip_->value();r.surface=surface_->value();r.paletteBank=paletteBank_->value();r.paletteEntries=paletteEntries_->value();r.lowNibbleFirst=lowNibble_->isChecked();r.ps2Clut=clut_->isChecked();r.ps2Alpha=alpha_->isChecked();r.bigEndian=endian_->currentIndex()==1;r.flipX=flipX_->isChecked();r.flipY=flipY_->isChecked();return r;}
    void setRunning(bool busy){running_=busy;calculate_->setEnabled(!busy);cancel_->setEnabled(busy);for(auto *widget:settings->findChildren<QWidget *>())if(widget!=cancel_&&widget!=progress_&&widget!=status_)widget->setEnabled(!busy);calculate_->setEnabled(!busy);cancel_->setEnabled(busy);if(refresh_)refresh_();}
    void append(const Entry &entry){entries_.append(entry);retained_+=entry.image.sizeInBytes();auto *item=new QListWidgetItem(entry.name,list_);if(!entry.image.isNull())item->setIcon(QPixmap::fromImage(entry.image.scaled(64,64,Qt::KeepAspectRatio,Qt::FastTransformation)));item->setToolTip(entry.source.path+QString("\n%1 · 0x%2").arg(entry.recipe.format).arg(entry.recipe.offset,0,16));item->setHidden(!entry.name.contains(filter_->text(),Qt::CaseInsensitive));}
    void next(){if(queue_.isEmpty()){setRunning(false);status_->setText(QString("Generated %1 image(s). Existing images are snapshots.").arg(completed_));if(calculationFinished)calculationFinished(true,status_->text());return;}active_=queue_.takeFirst();state_->recipe=active_.recipe;state_->patches=active_.patches;state_->cancel.store(false);status_->setText(QString("Calculating image %1 / %2...").arg(completed_+1).arg(total_));reader_.request(active_.source);}
    void finish(){if(state_->cancel.load()||state_->decoded.cancelled||!state_->decoded.error.isEmpty()){queue_.clear();setRunning(false);status_->setText(state_->cancel.load()||state_->decoded.cancelled?"Calculation canceled. Completed images remain in the library.":state_->decoded.error);if(calculationFinished)calculationFinished(false,status_->text());return;}active_.image=state_->decoded.image;append(active_);list_->setCurrentRow(entries_.size()-1);++completed_;progress_->setValue(completed_);next();}
    std::function<void()> refresh_;
    HexView *view_;std::shared_ptr<Job> state_;BinaryReader reader_;QList<Entry> entries_,queue_;Entry active_;qint64 retained_=0;bool running_=false,documentJob_=false;int completed_=0,total_=0;
    QListWidget *list_;QLineEdit *filter_,*offset_,*stride_,*palette_,*step_;QSpinBox *width_,*height_,*count_,*mip_,*surface_,*paletteBank_,*paletteEntries_;QComboBox *source_,*format_,*endian_,*memoryLayout_,*paletteFormat_,*byteSwap_,*container_;QCheckBox *flipX_,*flipY_,*lowNibble_,*clut_,*alpha_;QPushButton *calculate_,*cancel_;QLabel *status_,*footprint_;QProgressBar *progress_;
};
