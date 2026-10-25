#pragma once
#include "native_resources.h"
namespace Pgr1Data {
inline QByteArray read(ReadRequest source,const PatchList &patches,const std::atomic_bool &cancel,QString &error){
    auto bytes=NativeResources::read(source,patches,0,source.expectedSize,cancel,error);if(!error.isEmpty())return {};
    for(int depth=0;depth<3;++depth){bool goth=bytes.startsWith(QByteArray::fromHex("474f5448504f4b1a"));bool zlib=bytes.size()>2&&quint8(bytes[0])==0x78&&((quint8(bytes[0])*256+quint8(bytes[1]))%31)==0;if(!goth&&!zlib)return bytes;auto memory=std::make_shared<MemoryData>();memory->size=bytes.size();for(qsizetype p=0;p<bytes.size();p+=MemoryData::BlockSize)memory->blocks.append(bytes.mid(p,MemoryData::BlockSize));ReadRequest input;input.expectedSize=bytes.size();input.memory=memory;auto resources=NativeResources::inspect(input,{},cancel);auto entries=resources["entries"].toArray();if(resources.contains("error")||entries.isEmpty()){error=resources["error"].toString("Invalid PGR1 compression envelope.");return {};}auto row=entries.first().toObject();if(entries.size()>1&&row["codec"]!="zlib-blocks"){error="Ambiguous PGR1 compression members.";return {};}auto decoded=NativeResources::extract(input,{},row,cancel);if(!decoded.error.isEmpty()){error=decoded.error;return {};}bytes=std::move(decoded.bytes);}
    error="PGR1 compression nesting limit reached.";return {};
}
}
