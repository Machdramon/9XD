#include "direct_edit.h"
#include "byte_format.h"
#include <QStringEncoder>
#include <QRegularExpression>
#include <bit>
#include <cmath>
#include <limits>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#else
#include <iconv.h>
#endif
namespace {
QByteArray encodeCharacter(const QString &text,const QString &encoding){
    if(encoding=="ASCII"){if(text.size()!=1||text[0].unicode()<32||text[0].unicode()>126)return {};return text.toLatin1();}
    if(encoding=="CP932"){
#ifdef Q_OS_WIN
        BOOL replaced=FALSE;QByteArray bytes(8,0);int n=WideCharToMultiByte(932,WC_NO_BEST_FIT_CHARS,reinterpret_cast<const wchar_t *>(text.utf16()),int(text.size()),bytes.data(),8,nullptr,&replaced);if(n<=0||replaced)return {};bytes.resize(n);return bytes;
#else
        auto converter=iconv_open("CP932","UTF-8");if(converter==(iconv_t)-1)return {};auto input=text.toUtf8();QByteArray bytes(16,0);char *in=input.data(),*out=bytes.data();size_t remaining=input.size(),capacity=bytes.size();bool ok=iconv(converter,&in,&remaining,&out,&capacity)!=size_t(-1);iconv_close(converter);if(!ok||remaining)return {};bytes.resize(bytes.size()-qsizetype(capacity));return bytes;
#endif
    }
    QStringEncoder encoder(encoding.toLatin1().constData(),QStringConverter::Flag::Stateless);if(!encoder.isValid())return {};QByteArray bytes=encoder(text);return encoder.hasError()?QByteArray():bytes;
}
QByteArray encodeNumber(const QString &text,const QString &type,bool big){
    bool ok=false;quint64 bits=0;int width=numberWidth(type);
    if(type.startsWith("Float")){
        if(!QRegularExpression("^[+-]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?$").match(text).hasMatch())return {};
        double value=text.toDouble(&ok);if(!ok||!std::isfinite(value))return {};
        if(width==4){float f=float(value);if(!std::isfinite(f))return {};bits=std::bit_cast<quint32>(f);}else bits=std::bit_cast<quint64>(value);
    }else if(type.startsWith("UInt")){
        if(!QRegularExpression("^[0-9]+$").match(text).hasMatch())return {};bits=text.toULongLong(&ok);if(!ok||(width<8&&bits>=(quint64(1)<<(width*8))))return {};
    }else{
        if(!QRegularExpression("^-?[0-9]+$").match(text).hasMatch())return {};qint64 value=text.toLongLong(&ok);if(!ok)return {};if(width<8){qint64 bound=qint64(1)<<(width*8-1);if(value < -bound||value>=bound)return {};}bits=quint64(value);
    }
    QByteArray bytes(width,0);for(int i=0;i<width;++i)bytes[i]=char(bits>>(8*(big?width-1-i:i)));return bytes;
}
}
ReadResult directEdit(const ReadRequest &source,const DirectEditSpec &spec){
    ReadResult result;result.request=source;
    auto fail=[&](QString error){result.error=error;result.bytes.clear();result.matches.clear();result.directDrafts.clear();return result;};
    if(source.length<=0||source.length>1048576||spec.ranges.isEmpty()||spec.ranges.size()>4096||spec.character.toUcs4().size()!=1)return fail("Selección no válida: máximo 1 MiB y 4096 filas por pulsación.");
    if(!spec.hex&&source.length>65536)return fail("La edición interpretada admite regiones de hasta 64 KiB por pulsación.");
    auto data=readPage(source);if(!data.error.isEmpty())return data;applyPatches(data.bytes,source.offset,spec.patches);
    result.size=data.size;result.modified=data.modified;result.directCycle=spec.hex?2:1;
    QByteArray replacement;if(!spec.hex&&spec.type=="Texto"){replacement=encodeCharacter(spec.character,spec.encoding);if(replacement.isEmpty())return fail("Carácter no representable en esta codificación.");}
    for(auto range:spec.ranges){
        if(range.offset<source.offset||range.length<=0||range.offset-source.offset>data.bytes.size()||range.length>data.bytes.size()-(range.offset-source.offset))return fail("La selección supera los bytes disponibles.");
        auto part=data.bytes.mid(range.offset-source.offset,range.length);qint64 consumed=0;
        if(spec.hex){bool ok;int digit=spec.character.toInt(&ok,16);if(!ok||digit<0||digit>15)return fail("Solo se admiten dígitos hexadecimales 0–9 y A–F.");
            for(auto &c:part)c=char(spec.digit%2?(quint8(c)&0xF0)|digit:(quint8(c)&0x0F)|(digit<<4));consumed=part.size();result.directWidth=1;
        }else{
            QByteArray changed;
            auto tokens=decodeTokens(part,range.offset,spec.encoding,spec.type,spec.bigEndian,false);
            for(const auto &token:tokens){
                QByteArray encoded;
                if(spec.type=="Texto"){
                    if(replacement.size()!=token.length)return fail("La sobrescritura debe conservar el tamaño del carácter. Usa hexadecimal para cambiar su codificación o longitud.");encoded=replacement;
                }else{
                    if(token.length!=numberWidth(spec.type))return fail("Selecciona elementos numéricos completos.");
                    QString text=spec.drafts.value(token.offset,token.text);if(text.isEmpty())return fail("Valor numérico no editable.");result.directCycle=qMax(result.directCycle,int(text.size()));
                    if(spec.digit<text.size())text[spec.digit]=spec.character[0];
                    encoded=encodeNumber(text,spec.type,spec.bigEndian);if(encoded.isEmpty())return fail("Ese dígito produciría un valor inválido o fuera de rango; no se ha aplicado la pulsación.");result.directDrafts.insert(token.offset,text);
                }
                changed+=encoded;consumed+=token.length;result.directWidth=token.length;if(!spec.broadcast)break;
            }
            part=changed;
        }
        if(!consumed)return fail("No hay un elemento completo que editar.");result.matches.append({range.offset,consumed});result.bytes+=part;if(!spec.broadcast)break;
    }
    auto check=source;check.offset=0;check.length=0;auto meta=readPage(check);if(!meta.error.isEmpty())return meta;return result;
}
