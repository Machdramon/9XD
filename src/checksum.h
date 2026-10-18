#pragma once
#include "binary_document.h"
#include <QCryptographicHash>
#include <QStringList>
#include <array>
#include <vector>

struct ChecksumSpec {qint64 begin=0,length=0;QStringList algorithms;PatchList patches;};
struct ChecksumState {ChecksumSpec spec;std::atomic<bool> cancel{false};std::atomic<qint64> progress{0};};
inline QStringList checksumAlgorithms(){return {"Checksum-8","Checksum-16","Checksum-32","CRC-32","MD5","SHA-1","SHA-256","SHA-512"};}
inline int checksumHexLength(const QString &name){
    if(name=="Checksum-8")return 2;if(name=="Checksum-16")return 4;if(name=="Checksum-32"||name=="CRC-32")return 8;
    if(name=="MD5")return 32;if(name=="SHA-1")return 40;if(name=="SHA-256")return 64;if(name=="SHA-512")return 128;return 0;
}
inline ReadResult calculateChecksums(const ReadRequest &source,const ChecksumSpec &spec,std::atomic<bool> &cancel,std::atomic<qint64> &progress){
    ReadResult result;result.request=source;
    if(source.expectedSize<0||spec.begin<0||spec.length<0||spec.begin>source.expectedSize||spec.length>source.expectedSize-spec.begin||spec.algorithms.isEmpty()){
        result.error="Invalid checksum region or algorithm.";return result;
    }
    struct Digest {QString name;std::unique_ptr<QCryptographicHash> hash;};std::vector<Digest> digests;
    bool sumNeeded=false,crcNeeded=false;
    for(const auto &name:spec.algorithms){
        if(!checksumHexLength(name)){result.error="Unsupported checksum algorithm.";return result;}
        if(name.startsWith("Checksum-")){sumNeeded=true;continue;}if(name=="CRC-32"){crcNeeded=true;continue;}
        const auto algorithm=name=="MD5"?QCryptographicHash::Md5:name=="SHA-1"?QCryptographicHash::Sha1:name=="SHA-256"?QCryptographicHash::Sha256:QCryptographicHash::Sha512;
        digests.push_back({name,std::make_unique<QCryptographicHash>(algorithm)});
    }
    static const auto crcTable=[] {
        std::array<quint32,256> table{};
        for(quint32 i=0;i<256;++i){quint32 value=i;for(int bit=0;bit<8;++bit)value=(value>>1)^((value&1)?0xEDB88320u:0u);table[i]=value;}
        return table;
    }();
    quint32 sum=0,crc=0xFFFFFFFFu;
    for(qint64 done=0;done<spec.length;){
        if(cancel.load()){result.cancelled=true;return result;}
        auto request=source;request.offset=spec.begin+done;request.length=qMin(qint64(1048576),spec.length-done);
        auto block=readPage(request);if(!block.error.isEmpty()){result.error=block.error;return result;}
        if(block.bytes.size()!=request.length){result.error="The checksum region could not be read completely.";return result;}
        applyPatches(block.bytes,request.offset,spec.patches);
        if(sumNeeded||crcNeeded)for(unsigned char byte:block.bytes){if(sumNeeded)sum+=byte;if(crcNeeded)crc=crcTable[(crc^byte)&255]^(crc>>8);}
        for(auto &digest:digests)digest.hash->addData(block.bytes);
        done+=block.bytes.size();progress.store(done);
    }
    auto metadata=source;metadata.offset=0;metadata.length=0;const auto checked=readPage(metadata);
    if(!checked.error.isEmpty()){result.error=checked.error;return result;}
    if(cancel.load()){result.cancelled=true;return result;}
    const auto hex=[](quint32 value,int digits){return QString::number(value,16).rightJustified(digits,'0').toUpper();};
    for(const auto &name:spec.algorithms){
        if(name=="Checksum-8")result.checksums[name]=hex(sum&255u,2);
        else if(name=="Checksum-16")result.checksums[name]=hex(sum&65535u,4);
        else if(name=="Checksum-32")result.checksums[name]=hex(sum,8);
        else if(name=="CRC-32")result.checksums[name]=hex(crc^0xFFFFFFFFu,8);
    }
    for(auto &digest:digests)result.checksums[digest.name]=QString::fromLatin1(digest.hash->result().toHex().toUpper());
    result.size=checked.size;result.modified=checked.modified;return result;
}
