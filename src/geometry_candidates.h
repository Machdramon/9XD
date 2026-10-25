#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QVector3D>
#include <QtEndian>
#include <atomic>
#include <cmath>
#include <cstring>

// A format-independent, deliberately conservative descriptor-pattern detector.
// It never guesses faces from spatial proximity and never auto-renders hypotheses.
namespace GeometryCandidates {
inline QJsonObject scan(const QByteArray &input,const std::atomic_bool &cancel){
    const qint64 size=qMin(qint64(input.size()),qint64(8*1024*1024));QJsonArray result;int descriptors=0;bool limited=false;
    struct Descriptor {qint64 at,offset,count,stride;bool big;};QList<Descriptor> vertices,indices;
    auto word=[&](qint64 p,bool big){return big?qFromBigEndian<quint32>(input.constData()+p):qFromLittleEndian<quint32>(input.constData()+p);};
    auto point=[&](qint64 p,bool big){QVector3D v;for(int a=0;a<3;++a){auto bits=word(p+a*4,big);float f;std::memcpy(&f,&bits,4);v[a]=f;}return v;};
    auto plausible=[](QVector3D v){for(int a=0;a<3;++a)if(!std::isfinite(v[a])||std::abs(v[a])>1e7||(v[a]!=0&&std::abs(v[a])<1e-20))return false;return true;};
    for(qint64 at=0;at+12<=size&&descriptors<10000;at+=4){if((at&4095)==0&&cancel.load())return {{"error","Geometry search canceled."}};for(bool big:{false,true}){quint32 fields[3]={word(at,big),word(at+4,big),word(at+8,big)};for(auto order:{0,1,2}){qint64 offset=fields[order==0?0:order==1?1:2],count=fields[order==0?1:0],stride=fields[order==2?1:2];if(count<4||count>600000||offset%4||offset<12||offset>=size)continue;bool vertex=stride>=12&&stride<=192&&stride%4==0&&count<=100000;bool index=(stride==2||stride==4)&&count%3==0;if((!vertex&&!index)||count>(size-offset)/stride)continue;++descriptors;
                if(index){if(indices.size()<256)indices.append({at,offset,count,stride,big});else limited=true;continue;}
                if(vertices.size()>=64){limited=true;continue;}bool sane=true;QVector3D lo,hi;for(int n=0;n<12;++n){auto v=point(offset+(count-1)*n/11*stride,big);if(!plausible(v)){sane=false;break;}if(n==0)lo=hi=v;else for(int a=0;a<3;++a){lo[a]=qMin(lo[a],v[a]);hi[a]=qMax(hi[a],v[a]);}}if(sane&&(hi-lo).length()>1e-5)vertices.append({at,offset,count,stride,big});
            }}}
    if(descriptors>=10000)limited=true;QSet<QString> seen;qint64 work=0;
    for(auto v:vertices){if(cancel.load())return {{"error","Geometry search canceled."}};QList<QVector3D> points;bool sane=true;for(qint64 n=0;n<v.count;++n){if(++work>4000000){limited=true;sane=false;break;}auto p=point(v.offset+n*v.stride,v.big);if(!plausible(p)){sane=false;break;}points.append(p);}if(!sane)continue;
        for(auto idx:indices){if(cancel.load())return {{"error","Geometry search canceled."}};if(idx.big!=v.big||std::abs(idx.at-v.at)>256||(idx.offset<v.offset+v.count*v.stride&&v.offset<idx.offset+idx.count*idx.stride))continue;auto index=[&](qint64 n){auto p=input.constData()+idx.offset+n*idx.stride;return idx.stride==2?quint32(idx.big?qFromBigEndian<quint16>(p):qFromLittleEndian<quint16>(p)):(idx.big?qFromBigEndian<quint32>(p):qFromLittleEndian<quint32>(p));};QSet<quint32> used;int nondegenerate=0;sane=true;
            for(qint64 n=0;n<idx.count;n+=3){if((n&4095)==0&&cancel.load())return {{"error","Geometry search canceled."}};if((work+=3)>4000000){limited=true;sane=false;break;}auto a=index(n),b=index(n+1),c=index(n+2);if(a>=v.count||b>=v.count||c>=v.count){sane=false;break;}used.insert(a);used.insert(b);used.insert(c);if(a!=b&&b!=c&&a!=c&&QVector3D::crossProduct(points[b]-points[a],points[c]-points[a]).lengthSquared()>1e-16)++nondegenerate;}
            if(!sane||used.size()<4||nondegenerate<2||nondegenerate*2<idx.count/3)continue;auto key=QString("%1:%2:%3:%4:%5").arg(v.offset).arg(idx.offset).arg(v.count).arg(idx.count).arg(v.big);if(seen.contains(key))continue;seen.insert(key);
            QJsonObject settings{{"vertexOffset",QString::number(v.offset)},{"vertexCount",v.count},{"vertexStride",v.stride},{"positionType","Float32"},{"positionScale",1},{"indexOffset",QString::number(idx.offset)},{"indexCount",idx.count},{"indexStride",idx.stride},{"indexType",idx.stride==2?"UInt16":"UInt32"},{"topology","Triangle list"},{"bigEndian",v.big},{"uv",false},{"normals",false}};
            result.append(QJsonObject{{"name",QString("Possible mesh at 0x%1").arg(v.offset,0,16)},{"heuristic",true},{"settings",settings},{"basis",QString("Candidate: nearby offset/count/stride descriptors at 0x%1 and 0x%2; finite Float32 XYZ records, bounded indices and %3 nondegenerate triangles. %4 endian. Normals, UVs, transforms and materials are unknown. Preview and verify before export.").arg(v.at,0,16).arg(idx.at,0,16).arg(nondegenerate).arg(v.big?"Big":"Little")}});if(result.size()>=16){limited=true;break;}
        }if(result.size()>=16||work>4000000)break;
    }
    return {{"candidates",result},{"scannedBytes",QString::number(size)},{"descriptorCount",descriptors},{"limited",limited},{"status","Heuristic candidates only; Float32 XYZ and UInt16/32 triangle lists with nearby absolute offset/count/stride descriptors. Other layouts may be missed."}};
}
}
