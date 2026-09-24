#include "pattern_search.h"
#include "byte_format.h"
#include <QRegularExpression>

ReadResult searchPattern(const ReadRequest &source,const PatternSpec &spec,
                         std::atomic<bool> &cancel,std::atomic<qint64> &progress){
    ReadResult out;out.request=source;
    constexpr qint64 block=1024*1024;
    constexpr qsizetype maxMatches=10000;
    if(spec.begin<0||spec.length<=0||spec.begin>source.expectedSize||spec.length>source.expectedSize-spec.begin){out.error="El ámbito de búsqueda no es válido.";return out;}
    if(spec.expression.isEmpty()){out.error="Introduce un patrón.";return out;}
    auto read=[&](qint64 offset,qint64 length){auto r=source;r.offset=offset;r.length=length;auto result=readPage(r);applyPatches(result.bytes,offset,spec.patches);return result;};
    auto add=[&](qint64 offset,qint64 length){
        if(out.matches.size()==maxMatches){out.truncated=true;return false;}
        out.matches.append({offset,length});return true;
    };
    if(spec.regex||spec.text){
        if(spec.length>block){out.error="Regex admite una región de hasta 1 MiB. Selecciona una región menor o utiliza comodines hexadecimales para todo el archivo.";return out;}
        if(spec.expression.size()>4096){out.error="La expresión supera 4096 caracteres.";return out;}
        // PCRE limits bound backtracking on untrusted expressions. Latin-1 preserves byte offsets.
        QRegularExpression regex("(*LIMIT_MATCH=100000)(*LIMIT_DEPTH=1000)"+(spec.regex?spec.expression:QRegularExpression::escape(spec.expression)),QRegularExpression::DotMatchesEverythingOption);
        if(!regex.isValid()){out.error="Regex no válida: "+regex.errorString();return out;}
        if(cancel.load()){out.cancelled=true;return out;}
        auto data=read(spec.begin,spec.length);if(!data.error.isEmpty())return data;
        QString text;QVector<qint64> starts,ends;
        if(spec.text){
            for(const auto &token:decodeTokens(data.bytes,spec.begin,spec.encoding,"Texto",false,false)){
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
            if(!match.isValid()){out.error="La regex excedió los límites del motor. Simplifica la expresión o reduce la región.";out.matches.clear();return out;}
            if(!match.hasMatch())break;
            const auto start=match.capturedStart(),length=match.capturedLength();
            if(length>0){
                const auto byteStart=spec.text?starts[start]:spec.begin+start;
                const auto byteLength=spec.text?ends[start+length-1]-byteStart:length;
                if(out.matches.isEmpty()||out.matches.last().first!=byteStart)if(!add(byteStart,byteLength))break;
            }
            offset=start+qMax(qsizetype(1),length);
        }
        progress.store(spec.length);
    }else{
        QString expression=spec.expression;expression.remove(QRegularExpression("\\s"));
        if(expression.isEmpty()||expression.size()%2||expression.size()>8192||expression.contains(QRegularExpression("[^0-9a-fA-F?]"))){
            out.error="Usa pares hexadecimales: 48 8B ?? A?. ? sustituye un dígito; ?? un byte. Máximo 4096 bytes.";return out;
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
