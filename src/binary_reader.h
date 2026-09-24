#pragma once
#include <QByteArray>
#include <QMap>
#include <QDateTime>
#include <QFutureWatcher>
#include <QThreadPool>
#include <QElapsedTimer>
#include <functional>
#include <optional>

inline constexpr qint64 PageBytes = 4096;
struct ReadRequest {
    QString path;
    qint64 offset = 0;
    qint64 expectedSize = -1;
    QDateTime expectedModified;
    qint64 length = PageBytes;
};
struct EntropyPoint { qint64 offset,length;double entropy,zeros,printable; };
struct ReadResult {
    QMap<qint64,QString> directDrafts;
    int directCycle=1,directWidth=1;
    QList<EntropyPoint> entropy;
    qint64 analysisWindow=0;
    ReadRequest request;
    qint64 size = 0;
    QDateTime modified;
    QByteArray bytes;
    QString error;
    QList<QPair<qint64,qint64>> matches;
    QList<int> matchTypes;
    bool truncated=false, cancelled=false;
};
ReadResult readPage(const ReadRequest &request);

struct ReaderActivity {
    bool running = false, pending = false, stopping = false;
    quint64 started = 0, completed = 0, discarded = 0, replaced = 0, errors = 0;
    qint64 elapsedMs = 0, lastMs = 0, offset = 0, length = 0;
};

// Only accessed by the GUI thread. Workers receive value copies, never widgets.
// One active job + one replaceable pending request bounds I/O and result memory.
class BinaryReader final : public QObject {
    Q_OBJECT
public:
    using ReadFunction = std::function<ReadResult(const ReadRequest &)>;
    explicit BinaryReader(QObject *parent = nullptr, ReadFunction read = readPage);
    void request(ReadRequest request);
    void shutdown();
    void invalidate();
    bool busy() const { return running_; }
    ReaderActivity activity() const;
signals:
    void ready(const ReadResult &result);
    void idle();
private:
    void start(ReadRequest request);
    ReaderActivity activity_;
    QElapsedTimer elapsed_;
    QThreadPool pool_;
    QFutureWatcher<ReadResult> watcher_;
    ReadFunction read_;
    std::optional<ReadRequest> pending_;
    quint64 generation_ = 0;
    quint64 activeGeneration_ = 0;
    bool running_ = false;
    bool stopping_ = false;
};
