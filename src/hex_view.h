#pragma once
#include "binary_reader.h"
#include "interpretation.h"
#include "chunk_cache.h"
#include "binary_document.h"
#include "byte_format.h"
#include "pattern_search.h"
#include "magic_bytes.h"
#include "entropy.h"
#include "checksum.h"
#include "direct_edit.h"
#include <memory>
#include <QWidget>
class QTableWidget;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollBar;
class QSpinBox;
class QComboBox;
class DataInspector;

class HexView final : public QWidget {
    Q_OBJECT
public:
    explicit HexView(QWidget *parent = nullptr);
    void applyDisplayPreferences();
    int offsetBase()const{return offsetBase_;}
    int columnMode()const{return columnMode_;}
    int byteGroup()const{return byteGroup_;}
    void setOffsetBase(int base);
    void setColumnMode(int mode);
    void setByteGroup(int bytes);
    void focusOffsetHeader();
    void chooseBytesPerRow();
    void chooseByteGroup();
    void openFile(const QString &path);
    void newMemoryDocument();
    bool isMemoryDocument()const{return bool(memory_);}
    bool insertMode()const{return insertMode_;}
    void setInsertMode(bool value){insertMode_=value;}
    QString filePath()const{return path_;}
    ReadRequest sourceRequest(qint64 offset=0,qint64 length=PageBytes)const{return {path_,offset,size_,modified_,length,memory_};}
    bool spliceMemory(qint64 offset,qint64 remove,const QByteArray &insert,QString &error);
    void deleteSelection();
    void cutSelection();
    void selectWholeDocument(){if(size_>0)selectMatch(0,size_);}
    void insertBytes();
    BinaryReader *reader() { return &reader_; }
    QList<ReaderActivity> workerActivity() const {
        return {reader_.activity(), inspectorReader_.activity(), copyReader_.activity(), writer_.activity(), searchReader_.activity(), magicReader_.activity(), entropyReader_.activity(), directReader_.activity(),checksumReader_.activity()};
    }
    bool startEntropy(EntropySpec spec);
    bool startChecksum(ChecksumSpec spec);
    bool checksumBusy()const{return checksumReader_.busy();}
    void cancelChecksum(){checksumState_->cancel.store(true);}
    qint64 checksumProgress()const{return checksumState_->progress.load();}
    bool entropyBusy()const{return entropyReader_.busy();}
    void cancelEntropy(){entropyState_->cancel.store(true);}
    qint64 entropyProgress()const{return entropyState_->progress.load();}
    bool magicBusy() const { return magicReader_.busy(); }
    bool startMagicScan(MagicSpec spec);
    void cancelMagicScan() { magicState_->cancel.store(true); }
    qint64 magicProgress() const { return magicState_->progress.load(); }
    QVector<ByteRange> markedRegions(QString &error) const;
    QString selectedEncoding() const;
    bool searchBusy() const { return searchReader_.busy(); }
    bool startPatternSearch(PatternSpec spec);
    void cancelPatternSearch() { searchState_->cancel.store(true); }
    qint64 searchProgress() const { return searchState_->progress.load(); }
    void selectMatch(qint64 offset,qint64 length);
    bool directEditing()const{return directReader_.busy()||!directKeys_.isEmpty();}
    bool busy() const;
    void shutdown();
    QWidget *inspector() const;
    void copySelection();
    void addStatusWidget(QWidget *widget);
    QWidget *statusFooter()const{return statusFooter_;}
    void restoreStatusFooter();
    void editSelection();
    void pasteSelection();
    bool replaceSelection(const QByteArray &bytes, QString &error);
    void discardEdits();
    void undo();
    void redo();
    bool dirty() const { return memory_ ? memory_!=savedMemory_ : edits_.canUndo(); }
    bool canRedo() const { return memory_ ? memoryCursor_<memoryHistory_.size() : edits_.canRedo(); }
    bool saving() const { return writer_.busy(); }
    ReadRequest meshSource()const{return sourceRequest(0,0);}
    PatchList pendingPatches()const{return edits_.patches();}
    bool hasFile() const { return size_ >= 0; }
    bool saveFile(const QString &destination);
    qint64 saveProgress() const { return saveState_->progress.load(std::memory_order_relaxed); }
    qint64 fileSize() const { return size_; }
    void selectPage();
    void focusInterpretation();
    qint64 firstVisibleOffset() const { return loadedOffset_; }
    qint64 cachedBytes() const { return cache_->retainedBytes(); }
    ByteSelection selection() const { return selection_; }
    const QList<FormatRule> &formatRules() const { return formatRules_; }
    void setFormatRules(QList<FormatRule> rules);
    StyledBytes formatAt(qint64 offset,qint64 length=1) const;
    void jumpTo(qint64 offset);

signals:
    void viewerFontSizeChanged(int points);
    void checksumFinished(const ReadResult &result);
    void entropyFinished(const ReadResult &result);
    void magicScanFinished(const ReadResult &result);
    void patternSearchFinished(const ReadResult &result);
    void documentChanged();
    void formatRulesChanged();
    void editStateChanged();
    void saveFinished(bool success);
    void fileOpened(const QString &path);
    void idle();
    void selectionAvailable(bool available);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    int offsetBase_=16,columnMode_=0,byteGroup_=1;
    QLineEdit *offsetEditor_;
    QString offsetText(qint64 offset,bool padded=false)const;
    bool goToText(const QString &text);
    void updateOffsetHeader();
    int zoomRemainder_=0;
    void refreshMemory();
    void typeMemory(QString character);
    std::shared_ptr<const MemoryData> memory_,savedMemory_;
    struct MemoryChange {std::shared_ptr<const MemoryData> before,after;qint64 cost;};
    QList<MemoryChange> memoryHistory_;
    qsizetype memoryCursor_=0;
    bool insertMode_=false;
    int pendingNibble_=-1;
    void edited();
    void resetDirectEdit();
    void queueDirectEdit(QString character,bool broadcast);
    void startDirectEdit();
    QList<QPair<QString,bool>> directKeys_;
    int directDigit_=0;
    bool interpretedFocus_=false,directBroadcast_=false;
    QMap<qint64,QString> directDrafts_;
    std::shared_ptr<DirectEditSpec> directSpec_;

    QByteArray interpretedBytes(const ReadResult &result) const;
    void navigate(qint64 offset);
    void showResult(const ReadResult &result);
    void setNavigationEnabled(bool enabled);
    void rebuild();
    void updateSelection();
    void updateInspector();
    void selectByte(qint64 offset, bool extend, bool rectangle);
    qint64 byteAt(const QPoint &point) const;
    qint64 pageCapacity() const;
    int visibleRows() const;
    qint64 maxTopRow() const;
    int scrollValue(qint64 row) const;
    void configureScrollBar();
    void scrollRows(qint64 rows);
    void requestViewport();
    std::shared_ptr<ChunkCache> cache_;
    EditHistory edits_;
    std::shared_ptr<SaveState> saveState_;
    std::shared_ptr<PatternState> searchState_;
    std::shared_ptr<MagicState> magicState_;
    std::shared_ptr<EntropyState> entropyState_;
    std::shared_ptr<ChecksumState> checksumState_;
    BinaryReader reader_;
    BinaryReader inspectorReader_;
    BinaryReader copyReader_;
    BinaryReader writer_;
    BinaryReader searchReader_;
    BinaryReader magicReader_;
    BinaryReader entropyReader_;
    BinaryReader directReader_;
    BinaryReader checksumReader_;
    class QHBoxLayout *statusLayout_;
    QWidget *statusFooter_;
    QTableWidget *table_;
    QLabel *status_;
    QScrollBar *scrollbar_;
    QSpinBox *stride_;
    QComboBox *encoding_;
    QComboBox *type_;
    QComboBox *endian_;
    DataInspector *inspector_;
    QString path_;
    qint64 size_ = -1;
    qint64 desiredOffset_ = 0;
    qint64 loadedOffset_ = 0;
    QDateTime modified_;
    QByteArray bytes_;
    ByteSelection selection_;
    ByteSelection copying_;
    QList<FormatRule> formatRules_;
    bool dragging_ = false;
    bool resizePending_ = false;
    int wheelRemainder_ = 0;
    int pixelRemainder_ = 0;
    qint64 viewportRequestLength_ = 0;
};
