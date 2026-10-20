#pragma once
#include "hex_view.h"
#include "format_panel.h"
#include "theme.h"
#include <QJsonArray>
#include <cmath>
namespace AiStructure {
inline bool number(QJsonValue v,qint64 &n){if(!v.isString())return false;auto text=v.toString().trimmed();bool ok=false;n=text.toLongLong(&ok,text.startsWith("0x",Qt::CaseInsensitive)?16:10);return ok&&n>=0;}
inline QJsonObject apply(HexView *view,FormatPanel *panel,const QJsonObject &args,bool metadata=false,QString origin="AI"){
    if(!view->hasFile()||view->saving()||view->directEditing())return {{"error","Open a document and wait for editing/saving to finish."}};
    if(args.size()!=1||!args["regions"].isArray())return {{"error","Expected a regions array."}};auto regions=args["regions"].toArray();
    if(regions.isEmpty()||regions.size()>32)return {{"error","Use 1-32 regions per batch."}};
    QList<FormatRule> additions;auto existing=view->formatRules();QJsonArray applied;int skipped=0;
    for(auto value:regions){if(!value.isObject())return {{"error","Each region must be an object."}};auto region=value.toObject();for(auto key:region.keys())if(!QStringList{"name","offset","length","step","repetitions","evidence"}.contains(key))return {{"error","Unknown structure field: "+key}};
        auto label=region["name"].toString().trimmed(),evidence=region["evidence"].toString().trimmed();if(label.isEmpty()||label.size()>70||evidence.isEmpty()||evidence.size()>90||label.contains(QChar('\n'))||evidence.contains(QChar('\n')))return {{"error","Provide a short name (70 characters) and evidence (90 characters)."}};
        FormatRule rule;if(!number(region["offset"],rule.start)||!number(region["length"],rule.length)||rule.length<1)return {{"error","Offsets and positive lengths must be decimal or 0x strings."}};
        rule.step=rule.length;if(region.contains("step")&&!number(region["step"],rule.step))return {{"error","Invalid repeat step."}};
        if(region.contains("repetitions")){double count=region["repetitions"].toDouble(-1);if(!region["repetitions"].isDouble()||!std::isfinite(count)||count!=std::floor(count)||count<1||count>1000000000)return {{"error","Invalid repetition count."}};rule.repetitions=qint64(count);}
        auto size=view->fileSize();if(rule.step<rule.length||rule.start>size||rule.length>size-rule.start||(rule.repetitions-1)>(size-rule.start-rule.length)/rule.step)return {{"error","Structure range/repetitions exceed the document or overlap records."}};
        rule.name=QString("%1 / %2 / %3 | %4").arg(origin,metadata?"metadata":"hypothesis",label,evidence);rule.jumpTo=rule.start;
        rule.style.border=themeColor("accent.focus",view->palette().color(QPalette::Highlight));rule.style.underline=true;rule.style.underlineColor=rule.style.border;rule.style.italic=!metadata;
        bool duplicate=false;for(const auto &candidate:existing+additions)if(candidate.name==rule.name&&candidate.start==rule.start&&candidate.length==rule.length&&candidate.step==rule.step&&candidate.repetitions==rule.repetitions){duplicate=true;break;}
        if(duplicate){++skipped;continue;}additions.append(rule);applied.append(QJsonObject{{"name",rule.name},{"offset",QString::number(rule.start)},{"length",QString::number(rule.length)},{"step",QString::number(rule.step)},{"repetitions",rule.repetitions}});
    }
    if(existing.size()+additions.size()>256)return {{"error","256-rule document limit reached. Consolidate repeated fields or remove unwanted rules."}};
    if(!additions.isEmpty())panel->appendAnalysisRules(additions);
    return {{"status","Structure/formatting rules added; binary bytes unchanged. Rules require separate export."},{"added",additions.size()},{"duplicatesSkipped",skipped},{"regions",applied}};
}
}
