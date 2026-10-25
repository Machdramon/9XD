#pragma once
#include <QByteArray>
#include <QString>
#include <atomic>
#ifdef _WIN32
extern "C" __declspec(dllimport) int nines_lzx_decode(const unsigned char *,int,unsigned char *,int,int);
#else
extern "C" int nines_lzx_decode(const unsigned char *,int,unsigned char *,int,int);
#endif
inline QByteArray decodeXmemLzx(const QByteArray &input,int expected,const std::atomic_bool &cancel,QString &error){
    if(expected<1||expected>64*1024*1024||input.size()>64*1024*1024){error="LZX buffer exceeds 64 MiB.";return {};}
    QByteArray raw;int p=0; qint64 total=0;
    auto word=[&](int at){return (quint8(input[at])<<8)|quint8(input[at+1]);};
    while(p<input.size()){
        if(cancel.load()){error="LZX canceled.";return {};}
        if(input.size()-p<2){error="Truncated XMem frame header.";return {};}
        int n=0,decoded=32768;
        if(quint8(input[p])==255){if(input.size()-p<5){error="Truncated XMem final frame.";return {};}decoded=word(p+1);n=word(p+3);p+=5;}else{n=word(p);p+=2;}
        if(n<1||decoded<1||decoded>32768||n>input.size()-p||total+decoded>expected){error="Invalid XMem frame span.";return {};}
        raw.append(input.constData()+p,n);p+=n;total+=decoded;
    }
    if(total!=expected){error="XMem frames do not cover the declared output.";return {};}
    QByteArray out(expected,Qt::Uninitialized);int code=nines_lzx_decode(reinterpret_cast<const unsigned char *>(raw.constData()),raw.size(),reinterpret_cast<unsigned char *>(out.data()),expected,17);
    if(cancel.load()||code){error=cancel.load()?"LZX canceled.":QString("LZX decompression failed (%1).").arg(code);return {};}
    return out;
}
