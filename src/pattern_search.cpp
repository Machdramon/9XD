#include "pattern_search.h"
#include "byte_format.h"
#include <QRegularExpression>

ReadResult searchPattern(const ReadRequest &source,const PatternSpec &spec,
                         std::atomic<bool> &cancel,std::atomic<qint64> &progress){
    ReadResult out;out.request=source;
    constexpr qint64 block=1024*1024;
    const qsizetype maxMatches=qBound(1,spec.maxMatches,10000);
    if(spec.alignment<1||spec.alignment>1048576){out.error="Alignment must be between 1 and 1048576 bytes.";return out;}
    qint64 nextAllowed=spec.begin;
    if(spec.begin<0||spec.length<=0||spec.begin>source.expectedSize||spec.length>source.expectedSize-spec.begin){out.error="Invalid search scope.";return out;}
    if(spec.expression.isEmpty()){out.error="Enter a pattern.";return out;}
    auto read=[&](qint64 offset,qint64 length){auto r=source;r.offset=offset;r.length=length;auto result=readPage(r);applyPatches(result.bytes,offset,spec.patches);return result;};
    auto add=[&](qint64 offset,qint64 length){
        if(offset%spec.alignment||(!spec.overlap&&offset<nextAllowed))return true;
        nextAllowed=offset+length;
        if(out.matches.size()==maxMatches){out.truncated=true;return false;}
        out.matches.append({offset,length});return true;
    };
    if(spec.regex||spec.text){
        if(spec.length>block){out.error="Text/regex supports regions of up to 1 MiB. Select a smaller region or use hexadecimal wildcards for the entire file.";return out;}
        if(spec.expression.size()>4096){out.error="The expression exceeds 4096 characters.";return out;}
        // PCRE limits bound backtracking on untrusted expressions. Latin-1 preserves byte offsets.
        QRegularExpression regex("(*LIMIT_MATCH=100000)(*LIMIT_DEPTH=1000)"+(spec.regex?spec.expression:QRegularExpression::escape(spec.expression)),QRegularExpression::DotMatchesEverythingOption|(spec.caseSensitive?QRegularExpression::NoPatternOption:QRegularExpression::CaseInsensitiveOption));
        if(!regex.isValid()){out.error="Invalid regex: "+regex.errorString();return out;}
        if(cancel.load()){out.cancelled=true;return out;}
        auto data=read(spec.begin,spec.length);if(!data.error.isEmpty())return data;
        QString text;QVector<qint64> starts,ends;
        if(spec.text){
            for(const auto &token:decodeTokens(data.bytes,spec.begin,spec.encoding,"Text",false,false)){
                auto value=token.text;
                if(spec.encoding=="ASCII")value=QString::fromLatin1(data.bytes.mid(token.offset-spec.begin,token.length));
                for(qsizetype i=0;i<value.size();++i){starts.append(token.offset);ends.append(token.offset+token.length);}
                text+=value;
            }
        }else text=QString::fromLatin1(data.bytes);
        qsizetype offset=0;
        while(offset<=text.size()){
            if(cancel.load()){out.cancelled=true;out.matches.clear();return out;}
            const auto match=regex.match(text,offset);
            if(!match.isValid()){out.error="The regex exceeded the engine limits. Simplify the expression or reduce the region.";out.matches.clear();return out;}
            if(!match.hasMatch())break;
            const auto start=match.capturedStart(),length=match.capturedLength();
            bool accepted=false;
            if(length>0){
                const auto byteStart=spec.text?starts[start]:spec.begin+start;
                const auto byteLength=spec.text?ends[start+length-1]-byteStart:length;
                if(out.matches.isEmpty()||out.matches.last().first!=byteStart){auto before=out.matches.size();if(!add(byteStart,byteLength))break;accepted=out.matches.size()>before;}
            }
            offset=start+(spec.overlap||!accepted?1:qMax(qsizetype(1),length));
        }
        progress.store(spec.length);
    }else{
        QString expression=spec.expression;expression.remove(QRegularExpression("\\s"));
        if(expression.isEmpty()||expression.size()%2||expression.size()>8192||expression.contains(QRegularExpression("[^0-9a-fA-F?]"))){
            out.error="Use hexadecimal pairs: 48 8B ?? A?. ? matches one digit; ?? matches one byte. Maximum 4096 bytes.";return out;
        }
        QByteArray values,masks;
        for(qsizetype i=0;i<expression.size();i+=2){unsigned value=0,mask=0;
            for(int j=0;j<2;++j){value<<=4;mask<<=4;if(expression[i+j]!='?'){value|=QString(expression[i+j]).toUInt(nullptr,16);mask|=15;}}
            values.append(char(value));masks.append(char(mask));
        }
        QByteArray tail;const auto end=spec.begin+spec.length;
        for(qint64 position=spec.begin;position<end;){
            if(cancel.load()){out.cancelled=true;out.matches.clear();return out;}
            const auto length=qMin(block,end-position);auto data=read(position,length);if(!data.error.isEmpty())return data;
            QByteArray bytes=tail+data.bytes;const auto base=position-tail.size();
            for(qsizetype i=0;i+values.size()<=bytes.size();++i){
                if((i&4095)==0&&cancel.load()){out.cancelled=true;out.matches.clear();return out;}
                bool matches=true;for(qsizetype j=0;j<values.size();++j)if((quint8(bytes[i+j])&quint8(masks[j]))!=quint8(values[j])){matches=false;break;}
                if(matches&&!add(base+i,values.size()))break;
            }
            position+=length;progress.store(position-spec.begin);
            if(out.truncated)break;
            tail=bytes.right(values.size()-1);
        }
    }
    auto metadata=read(spec.begin,0);if(!metadata.error.isEmpty())return metadata;
    out.size=metadata.size;out.modified=metadata.modified;return out;
}
