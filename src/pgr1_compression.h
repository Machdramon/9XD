#pragma once
#include <QByteArray>
#include <QString>
#include <QtEndian>
#include <atomic>

namespace Pgr1Compression {
struct Result { QByteArray bytes; QString error; };
// Derived from static inspection of the user's PGR1 USA decoder. No game code is executed.
// A control word has two window-size bits and thirty MSB-first literal/match flags.
inline Result decode(const QByteArray &input,qint64 expected,const std::atomic_bool &cancel,qint64 limit,bool bigEndian=false){
    Result out;
    auto fail=[&](const char *message){out.bytes.clear();out.error=QString::fromLatin1(message);return out;};
    if(expected<0||expected>limit)return fail("GOTHPOK decoded size exceeds the resource budget.");
    qint64 at=0;
    out.bytes.reserve(expected);
    while(out.bytes.size()<expected){
        if(cancel.load())return fail("GOTHPOK decoding canceled.");
        if(input.size()-at<4)return fail("Incomplete GOTHPOK control word.");
        quint32 control=bigEndian?qFromBigEndian<quint32>(input.constData()+at):qFromLittleEndian<quint32>(input.constData()+at);at+=4;
        const int shift=14-(control&3), mask=(1<<shift)-1;
        for(int bit=0;bit<30&&out.bytes.size()<expected;++bit,control<<=1){
            if(control&0x80000000u){
                if(input.size()-at<2)return fail("Incomplete GOTHPOK match.");
                auto token=bigEndian?qFromBigEndian<quint16>(input.constData()+at):qFromLittleEndian<quint16>(input.constData()+at);at+=2;
                const int distance=(token&mask)+1, count=(token>>shift)+3;
                if(distance>out.bytes.size()||count>expected-out.bytes.size())return fail("GOTHPOK match exceeds its history or decoded size.");
                for(int i=0;i<count;++i)out.bytes.append(out.bytes[out.bytes.size()-distance]);
            }else{
                if(at>=input.size())return fail("Incomplete GOTHPOK literal.");
                out.bytes.append(input[at++]);
            }
        }
    }
    if(at!=input.size())return fail("Unexpected trailing GOTHPOK payload bytes.");
    return out;
}
}
