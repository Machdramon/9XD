#pragma once
#include <QByteArray>
#include <QString>
#include <QtEndian>
#include <array>
#include <atomic>
#include "memory_budget.h"
// EA HUFF: bounded MSB-first canonical Huffman codes with cyclic alphabet
// deltas and an escape symbol for literal/repeat/end commands. No machine code.
namespace NfsHuff {
inline QByteArray decode(const QByteArray &input,qint64 expected,QString &error,const std::atomic_bool *cancel=nullptr){
    error.clear();auto fail=[&](QString message){error=message;return QByteArray();};
    if(input.size()<16||input.left(8)!=QByteArray::fromHex("4855464601100000"))return fail("Unsupported HUFF wrapper.");
    auto u=[&](int p){return qFromLittleEndian<quint32>(input.constData()+p);};qint64 goal=u(8),packed=u(12);
    if(goal!=expected||goal<1||goal>qMin(AssetMemory::bytes()/4,qint64(128)*1024*1024)||packed<6||packed>input.size()-16)return fail("Invalid HUFF input/output lengths.");
    qint64 bit=128,end=(16+packed)*8;bool good=true;auto take=[&](int count)->quint32{if(count<0||count>32||count>end-bit){good=false;return 0;}quint32 value=0;for(int i=0;i<count;++i,++bit)value=(value<<1)|((quint8(input[bit/8])>>(7-bit%8))&1);return value;};
    auto number=[&]()->quint32{if(take(1))return take(2);int zeroes=1;while(good&&!take(1))if(++zeroes>28){good=false;break;}if(!good)return 0;int width=zeroes+2;return (quint32(1)<<width)-4+take(width);};
    auto mode=take(16);if(mode!=0x30fb&&mode!=0x32fb&&mode!=0x34fb&&mode!=0xb0fb&&mode!=0xb2fb&&mode!=0xb4fb)return fail("Unsupported HUFF stream mode.");
    auto inner=take(mode&0x8000?32:24);if(!good||inner!=goal)return fail("HUFF stream/output lengths disagree.");int escape=take(8);
    std::array<quint32,16> counts{},first{},offsets{};quint32 code=0,total=0;int levels=0;
    for(int length=1;length<=16;++length){auto count=number(),capacity=quint32(1)<<length;if(!good||code>capacity||count>capacity-code||count>256-total)return fail("Invalid HUFF code lengths.");first[length-1]=code;offsets[length-1]=total;counts[length-1]=count;total+=count;levels=length;if(code+count==capacity)break;code=(code+count)*2;if(length==16)return fail("Incomplete HUFF code tree.");}
    if(total<1)return fail("Empty HUFF alphabet.");std::array<quint8,256> alphabet{};std::array<bool,256> used{};int previous=255;bool hasEscape=false;
    for(quint32 i=0;i<total;++i){auto delta=number();if(!good||delta>=256-i)return fail("Invalid HUFF alphabet delta.");int steps=delta+1;while(steps){previous=(previous+1)&255;if(!used[previous])--steps;}used[previous]=true;alphabet[i]=previous;hasEscape|=previous==escape;}
    if(!hasEscape)return fail("HUFF alphabet has no end marker.");
    QByteArray output;output.reserve(goal);bool finished=false;
    while(!finished){if(cancel&&cancel->load())return fail("HUFF decompression canceled.");quint32 value=0;int symbol=-1;for(int length=0;length<levels;++length){value=(value<<1)|take(1);if(!good)break;if(value>=first[length]&&value-first[length]<counts[length]){symbol=alphabet[offsets[length]+value-first[length]];break;}}
        if(!good||symbol<0)return fail("Truncated or invalid HUFF symbol.");
        if(symbol!=escape){if(output.size()>=goal)return fail("HUFF literal exceeds its declared output.");output.append(char(symbol));continue;}
        auto repeat=number();if(!good)return fail("Truncated HUFF repeat command.");if(repeat){if(output.isEmpty()||repeat>goal-output.size())return fail("HUFF repeat exceeds its output history/size.");auto last=output.back();for(quint32 i=0;i<repeat;++i){if((i&4095)==0&&cancel&&cancel->load())return fail("HUFF decompression canceled.");output.append(last);}}
        else{bool stop=take(1);if(!good)return fail("Truncated HUFF escape command.");if(stop)finished=true;else{auto literal=take(8);if(!good||output.size()>=goal)return fail("Truncated or oversized HUFF escaped literal.");output.append(char(literal));}}
    }
    if(output.size()!=goal)return fail("HUFF end marker/output lengths disagree.");
    if((mode&0x7fff)!=0x30fb){quint8 sum=0,second=0;for(qint64 i=0;i<output.size();++i){if((i&4095)==0&&cancel&&cancel->load())return fail("HUFF reconstruction canceled.");sum=quint8(sum+quint8(output[i]));if((mode&0x7fff)==0x34fb){second=quint8(second+sum);output[i]=char(second);}else output[i]=char(sum);}}
    return output;
}
}
