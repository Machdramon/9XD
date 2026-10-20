#pragma once
#include "hex_view.h"
#include <QWidget>
class QToolBar;
class QTreeWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QLabel;
class FormatPanel final : public QWidget {
public:
    explicit FormatPanel(HexView *view,QWidget *parent=nullptr);
    QToolBar *toolbar() const { return toolbar_; }
    bool confirmLeave();
    void appendAnalysisRules(const QList<FormatRule> &rules){auto combined=view_->formatRules();combined.append(rules);publish(std::move(combined));}
    bool markExternalMatches(const QString &name,const QVector<ByteRange> &matches,QString &error);
private:
    ByteStyle selectedStyle() const;
    void refresh();
    void applySelection(bool clear=false);
    void addRule(bool replace);
    void beginRule(bool replace);
    void startSearch();
    void markMatches();
    void loadSelected();
    void navigateSelected();
    void updateColorFields();
    bool exportRules();
    void importRules();
    void publish(QList<FormatRule> rules);
    HexView *view_;
    QToolBar *toolbar_;
    QTreeWidget *list_;
    QLineEdit *name_,*start_,*length_,*step_,*repetitions_;
    QCheckBox *bold_,*italic_,*underline_,*borderEnabled_,*fillEnabled_,*textEnabled_;
    QLineEdit *jump_,*colorFields_[4];
    bool loadingStyle_=false;
    QColor underlineColor_{"#202020"};
    QColor background_{"#F6D365"},foreground_{"#202020"},border_{"#E89323"};
    QWidget *editor_;
    QComboBox *searchMode_,*searchScope_;
    QLineEdit *pattern_,*searchStart_,*searchLength_;
    QSpinBox *matchIndex_,*alignment_,*resultLimit_;
    QCheckBox *caseSensitive_,*overlap_;
    QTreeWidget *searchResults_;
    QLabel *searchStatus_;
    QList<QPair<qint64,qint64>> matches_;
    QString matchedPattern_;
    bool searchStale_=false;
    bool dirty_=false;
};
