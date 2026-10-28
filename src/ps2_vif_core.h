#pragma once
// Generic PS2 VIF geometry recovery. Format-independent: finds VIF code streams, groups UNPACK
// writes into microprogram batches, infers attribute roles and selects a strip/restart rule by
// measured mesh quality. Pure C++ so it can be validated outside Qt.
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace Ps2VifCore {
using Vec3 = std::array<double,3>;
using Tri = std::array<uint32_t,3>;

struct Unpack { int cmd=0,vn=0,vl=0,n=0,addr=0,usn=0,cl=1,wl=1,inputs=0; int64_t data=0; };
struct Batch { std::vector<Unpack> unpacks; };

struct KeptBatch {
    std::vector<Vec3> positions,normals;
    std::vector<std::array<double,2>> uvs;
    std::vector<Tri> triangles;
    int64_t positionOffset=0; int positionStride=12; int part=0; int group=0; double quality=0;
};
struct GroupReport {
    std::string signature,rule,position; int batches=0,kept=0; size_t triangles=0; bool accepted=false; std::string reason;
};
struct Result {
    std::vector<KeptBatch> batches; std::vector<GroupReport> groups; std::vector<int64_t> rets;
    int classified=0,streams=0; bool canceled=false;
    size_t triangleCount()const{size_t n=0;for(auto &b:batches)n+=b.triangles.size();return n;}
    size_t vertexCount()const{size_t n=0;for(auto &b:batches)n+=b.positions.size();return n;}
};

inline uint32_t rd32(const uint8_t *d,int64_t p){uint32_t v;std::memcpy(&v,d+p,4);return v;}
inline int bitsOf(int vl){return vl==0?32:vl==1?16:vl==2?8:5;}
inline int inputsFor(int n,int cl,int wl){return wl<=cl?n:(n/wl)*cl+std::min(n%wl,cl);}

enum Kind { Invalid, Command, UnpackCode, Mpg, Direct, DmaTag };
struct Code { Kind kind=Invalid; int64_t extra=0; int cmd=0,num=0,imm=0; };

inline Code codeAt(const uint8_t *d,int64_t p,int cl,int wl){
    Code c; uint32_t w=rd32(d,p); c.cmd=(w>>24)&0x7f; c.num=(w>>16)&0xff; c.imm=w&0xffff;
    const int cmd=c.cmd,num=c.num,imm=c.imm;
    if(cmd>=0x60){
        const int vn=(cmd>>2)&3,vl=cmd&3;
        if(vl==3&&vn!=3)return {};
        if(cl==0||wl==0)return {};
        const int n=num?num:256,inputs=inputsFor(n,cl,wl);
        const int64_t size=vl==3?int64_t((inputs*2+3)/4)*4:int64_t((int64_t(inputs)*(vn+1)*bitsOf(vl)+31)/32)*4;
        if((imm&0x3ff)+(wl<=cl?n:0)>1024+256)return {};
        c.kind=UnpackCode;c.extra=size;return c;
    }
    switch(cmd){
    case 0x00: if(num)return {}; break;
    case 0x10: case 0x11: case 0x13: if(num||imm)return {}; break;
    case 0x01: if(!((imm&0xff)>=1&&(imm&0xff)<=16&&(imm>>8)>=1&&(imm>>8)<=16))return {}; break;
    case 0x05: if(imm>3)return {}; break;
    case 0x02: case 0x03: case 0x04: if(imm>0x3ff)return {}; break;
    case 0x06: case 0x07: case 0x14: case 0x15: case 0x17: break;
    case 0x20: c.kind=Command;c.extra=4;return c;
    case 0x30: case 0x31: c.kind=Command;c.extra=16;return c;
    case 0x4a: c.kind=Mpg;c.extra=int64_t(num?num:256)*8;return c;
    case 0x50: case 0x51: c.kind=Direct;c.extra=int64_t(imm?imm:65536)*16;return c;
    default: return {};
    }
    c.kind=Command;c.extra=0;return c;
}
// DMA source-chain tag (cnt/next/ret/end) interleaved with VIF data.
inline bool dmaTagAt(const uint8_t *d,int64_t p,int64_t end){
    if(p+16>end)return false;
    const uint32_t w0=rd32(d,p),w1=rd32(d,p+4);const uint32_t id=(w0>>28)&7,qwc=w0&0xffff;
    if((w0&0xffff0000u)!=(id<<28))return false;
    if(id!=1&&id!=2&&id!=6&&id!=7)return false;
    if(id!=2&&w1!=0)return false;
    return p+16+int64_t(qwc)*16<=end;
}
struct Event { Kind kind; int64_t at; int cmd,num,imm,cl,wl; int64_t extra; };
inline int64_t walk(const uint8_t *d,int64_t start,int64_t end,int64_t maxLength,bool dma,std::vector<Event> &events){
    events.clear();int64_t p=start;int cl=1,wl=1;
    while(p+4<=end&&p-start<maxLength){
        if(dma&&(p-start)%16==0&&dmaTagAt(d,p,end)&&((rd32(d,p)>>24)&0x7f)!=0){events.push_back({DmaTag,p,0,0,0,cl,wl,0});p+=16;continue;}
        Code c=codeAt(d,p,cl,wl);if(c.kind==Invalid)break;
        if(p+4+c.extra>end)break;
        if(c.kind==Command&&c.cmd==0x01){cl=c.imm&0xff;wl=c.imm>>8;}
        events.push_back({c.kind,p,c.cmd,c.num,c.imm,cl,wl,c.extra});
        p+=4+c.extra;
    }
    return p;
}
struct Stream { int64_t start,stop; std::vector<Event> events; };
inline std::vector<Stream> findStreams(const uint8_t *d,int64_t size,const std::atomic_bool *cancel){
    std::vector<Stream> out;int64_t covered=0;std::vector<Event> ev;
    for(int64_t s=0;s+4<=size;s+=4){
        if(((rd32(d,s)>>24)&0x7f)<0x60||s<covered)continue;
        if(cancel&&(s&0xfffff)==0&&cancel->load())break;
        int64_t best=-1;
        for(int64_t back=64;back>=0;back-=4){
            const int64_t st=s-back;if(st<covered||st<0)continue;
            walk(d,st,size,back+8,false,ev);
            bool hit=false;for(auto &e:ev)if(e.at==s){hit=true;break;}
            if(hit){best=st;break;}
        }
        if(best<0)continue;
        Stream stream;stream.start=best;stream.stop=walk(d,best,size,int64_t(1)<<40,true,stream.events);
        int unpacks=0;for(auto &e:stream.events)unpacks+=e.kind==UnpackCode;
        if(unpacks>=3&&stream.stop-best>=64){covered=stream.stop;out.push_back(std::move(stream));}
        else covered=std::max(covered,s+4);
    }
    return out;
}
// Decoded unpack: per-vector doubles (interpreted) and raw integer components (for flag bits).
struct Decoded { Unpack unpack; std::vector<double> value; std::vector<uint32_t> raw; int rows=0; };
inline Decoded decode(const uint8_t *d,const Unpack &u){
    Decoded r;r.unpack=u;r.rows=u.inputs;const int vn=u.vn;r.value.resize(size_t(r.rows)*vn);r.raw.resize(r.value.size());
    for(int i=0;i<r.rows;++i)for(int k=0;k<vn;++k){
        const size_t at=size_t(i)*vn+k;double v=0;uint32_t raw=0;
        if(u.vl==0){raw=rd32(d,u.data+4*int64_t(at));float f;std::memcpy(&f,&raw,4);v=f;}
        else if(u.vl==1){uint16_t h;std::memcpy(&h,d+u.data+2*int64_t(at),2);raw=h;v=u.usn?double(h):double(int16_t(h));}
        else if(u.vl==2){uint8_t b=d[u.data+int64_t(at)];raw=b;v=u.usn?double(b):double(int8_t(b));}
        else{uint16_t h;std::memcpy(&h,d+u.data+2*int64_t(i),2);const uint32_t c[4]={uint32_t(h&31)<<3,uint32_t((h>>5)&31)<<3,uint32_t((h>>10)&31)<<3,uint32_t(h>>15)<<7};raw=c[k];v=raw;}
        r.value[at]=v;r.raw[at]=raw;
    }
    return r;
}
inline std::vector<Batch> batches(const std::vector<Stream> &streams){
    std::vector<Batch> out;
    for(auto &s:streams){
        Batch cur;
        for(auto &e:s.events){
            if(e.kind==UnpackCode){
                Unpack u;u.cmd=e.cmd;u.vn=((e.cmd>>2)&3)+1;u.vl=e.cmd&3;u.n=e.num?e.num:256;u.addr=e.imm&0x3ff;u.usn=(e.imm>>14)&1;u.cl=e.cl;u.wl=e.wl;u.data=e.at+4;u.inputs=inputsFor(u.n,u.cl,u.wl);
                cur.unpacks.push_back(u);
            }else if(e.kind==Command&&(e.cmd==0x14||e.cmd==0x15||e.cmd==0x17)){if(!cur.unpacks.empty())out.push_back(std::move(cur));cur=Batch();}
        }
        if(!cur.unpacks.empty())out.push_back(std::move(cur));
    }
    return out;
}
inline double median(std::vector<double> v){if(v.empty())return 0;auto m=v.begin()+v.size()/2;std::nth_element(v.begin(),m,v.end());if(v.size()%2)return *m;double hi=*m;double lo=*std::max_element(v.begin(),m);return (lo+hi)/2;}
inline double norm3(const Vec3 &a){return std::sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);}
inline Vec3 sub(const Vec3 &a,const Vec3 &b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline Vec3 cross(const Vec3 &a,const Vec3 &b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline double diagonal(const std::vector<Vec3> &p){if(p.empty())return 0;Vec3 lo=p[0],hi=p[0];for(auto &v:p)for(int k=0;k<3;++k){lo[k]=std::min(lo[k],v[k]);hi[k]=std::max(hi[k],v[k]);}return norm3(sub(hi,lo));}
inline double spread(const std::vector<Vec3> &p){if(p.empty())return 0;Vec3 lo=p[0],hi=p[0];for(auto &v:p)for(int k=0;k<3;++k){lo[k]=std::min(lo[k],v[k]);hi[k]=std::max(hi[k],v[k]);}return (hi[0]-lo[0])+(hi[1]-lo[1])+(hi[2]-lo[2]);}

struct Classified {
    int N=0;std::vector<Decoded> streams;int position=-1,normal=-1,uv=-1;
    std::vector<Vec3> P,Nrm;std::vector<std::array<double,2>> UV;
};
inline bool classify(const uint8_t *d,const Batch &b,Classified &c){
    std::map<int,int> counts;std::vector<int> order;
    for(auto &u:b.unpacks)if(u.n>=3&&u.vn>=2){if(!counts[u.n]++)order.push_back(u.n);}
    if(order.empty())return false;
    int N=order[0];for(int n:order)if(counts[n]>counts[N])N=n;
    if(counts[N]<2)return false;
    c.N=N;
    struct Cand{int index;std::vector<Vec3> v;};std::vector<Cand> pos,nrm;std::vector<int> uvs;
    for(auto &u:b.unpacks){
        if(u.n!=N||u.inputs!=N)continue;
        c.streams.push_back(decode(d,u));const int idx=int(c.streams.size())-1;auto &s=c.streams.back();
        auto vec3=[&](int i){return Vec3{s.value[size_t(i)*u.vn],s.value[size_t(i)*u.vn+1],s.value[size_t(i)*u.vn+2]};};
        if(u.vl==0){
            bool ok=true;double mn=1e300,mx=0;for(double v:s.value){if(!std::isfinite(v)){ok=false;break;}if(v!=0){mn=std::min(mn,std::abs(v));mx=std::max(mx,std::abs(v));}}
            if(!ok||(mx>0&&(mn<1e-7||mx>1e6)))continue;
            if(u.vn>=3){std::vector<Vec3> v(N);bool unit=true;for(int i=0;i<N;++i){v[i]=vec3(i);if(std::abs(norm3(v[i])-1)>=0.05)unit=false;}(unit?nrm:pos).push_back({idx,std::move(v)});}
            else if(u.vn==2)uvs.push_back(idx);
        }else if(u.vl==1&&u.vn>=3&&!u.usn){
            std::vector<Vec3> v(N);double sum=0,sq=0;for(int i=0;i<N;++i){v[i]=vec3(i);double l=norm3(v[i]);sum+=l;sq+=l*l;}
            double mean=sum/N,sd=std::sqrt(std::max(0.,sq/N-mean*mean));
            if(mean>0&&sd/mean<0.05){for(auto &x:v)for(auto &k:x)k/=mean;nrm.push_back({idx,std::move(v)});}else pos.push_back({idx,std::move(v)});
        }else if(u.vl==1&&u.vn==2)uvs.push_back(idx);
        else if(u.vl==2&&u.vn>=3&&!u.usn){
            std::vector<Vec3> v(N);double sum=0,sq=0;std::vector<double> len(N);for(int i=0;i<N;++i){v[i]=vec3(i);len[i]=norm3(v[i]);sum+=len[i];sq+=len[i]*len[i];}
            double mean=sum/N,sd=std::sqrt(std::max(0.,sq/N-mean*mean));
            if(mean>40&&sd/mean<0.15){for(int i=0;i<N;++i)for(auto &k:v[i])k/=std::max(1.0,len[i]);nrm.push_back({idx,std::move(v)});}
        }
    }
    auto coherent=[](const std::vector<Vec3> &p){if(p.size()<24)return true;std::vector<double> st;for(size_t i=1;i<p.size();++i)st.push_back(norm3(sub(p[i],p[i-1])));double dg=diagonal(p);return dg>0&&median(st)/dg<0.2;};
    std::vector<Cand> keep;for(auto &p:pos)if(coherent(p.v))keep.push_back(std::move(p));
    if(keep.empty())return false;
    size_t best=0;for(size_t i=1;i<keep.size();++i)if(spread(keep[i].v)>spread(keep[best].v))best=i;
    c.position=keep[best].index;c.P=std::move(keep[best].v);
    if(!nrm.empty()){c.normal=nrm[0].index;c.Nrm=nrm[0].v;}
    if(!uvs.empty()){c.uv=uvs[0];auto &s=c.streams[c.uv];c.UV.resize(N);for(int i=0;i<N;++i)c.UV[i]={s.value[size_t(i)*2],s.value[size_t(i)*2+1]};}
    return true;
}
struct Rule { std::string name; int stream=-1,comp=0,bit=0; bool negate=false,list=false; };
inline std::vector<Rule> rules(const Classified &c){
    std::vector<Rule> out{{"strip"}};
    for(int si=0;si<int(c.streams.size());++si){
        auto &s=c.streams[si];const int vl=s.unpack.vl;
        std::vector<int> bits=vl==1?std::vector<int>{15}:vl==0?std::vector<int>{15,0}:std::vector<int>{7};
        for(int comp=0;comp<s.unpack.vn;++comp)for(int bit:bits){
            int set=0;for(int i=0;i<s.rows;++i)set+=(s.raw[size_t(i)*s.unpack.vn+comp]>>bit)&1;
            if(set>0&&set<s.rows){
                const std::string key="a"+std::to_string(s.unpack.addr)+"c"+std::to_string(comp)+"b"+std::to_string(bit);
                out.push_back({"strip/"+key,si,comp,bit,false});out.push_back({"strip/!"+key,si,comp,bit,true});
            }
        }
    }
    if(c.N%3==0){Rule r;r.name="list";r.list=true;out.push_back(r);}
    return out;
}
inline std::vector<Tri> faces(const Classified &c,const Rule &r){
    std::vector<Tri> t;
    if(r.list){for(int i=0;i+2<c.N;i+=3)t.push_back({uint32_t(i),uint32_t(i+1),uint32_t(i+2)});}
    else for(int i=2;i<c.N;++i){
        if(r.stream>=0){auto &s=c.streams[r.stream];bool f=(s.raw[size_t(i)*s.unpack.vn+r.comp]>>r.bit)&1;if(f!=r.negate)continue;}
        if(i%2)t.push_back({uint32_t(i-1),uint32_t(i-2),uint32_t(i)});else t.push_back({uint32_t(i-2),uint32_t(i-1),uint32_t(i)});
    }
    // Drop stitching degenerates (repeated positions); they are intentional and not errors.
    std::vector<Tri> out;for(auto &x:t){auto &a=c.P[x[0]],&b=c.P[x[1]],&e=c.P[x[2]];if(a==b||b==e||a==e)continue;out.push_back(x);}
    return out;
}
inline std::pair<double,size_t> quality(const Classified &c,const std::vector<Tri> &T){
    if(T.empty())return {0,0};
    std::vector<double> edges;edges.reserve(T.size()*3);
    for(auto &t:T){edges.push_back(norm3(sub(c.P[t[0]],c.P[t[1]])));edges.push_back(norm3(sub(c.P[t[1]],c.P[t[2]])));edges.push_back(norm3(sub(c.P[t[2]],c.P[t[0]])));}
    const double m=median(edges);if(m==0)return {0,0};
    size_t good=0,agreeGood=0;
    for(size_t i=0;i<T.size();++i){
        auto &t=T[i];const double mx=std::max({edges[3*i],edges[3*i+1],edges[3*i+2]});
        Vec3 n=cross(sub(c.P[t[1]],c.P[t[0]]),sub(c.P[t[2]],c.P[t[0]]));const double area=norm3(n);
        const bool g=mx<5*m&&area>1e-12*m*m;good+=g;
        if(!c.Nrm.empty()&&g){Vec3 v{0,0,0};for(int k=0;k<3;++k)for(int a=0;a<3;++a)v[a]+=c.Nrm[t[k]][a];const double l=std::max(norm3(v),1e-9);const double dot=(n[0]*v[0]+n[1]*v[1]+n[2]*v[2])/(std::max(area,1e-30)*l);agreeGood+=std::abs(dot)>0.5;}
    }
    double s=double(good)/T.size();
    if(!c.Nrm.empty()){s=0.5*s+0.5*double(agreeGood)/T.size();return {s,agreeGood};}
    return {s,good};
}
inline std::string signature(const Classified &c){
    std::vector<std::array<int,4>> k;for(auto &s:c.streams)k.push_back({s.unpack.addr,s.unpack.vn,s.unpack.vl,s.unpack.usn});std::sort(k.begin(),k.end());
    std::string out;for(auto &x:k)out+=std::to_string(x[0])+":"+std::to_string(x[1])+":"+std::to_string(x[2])+":"+std::to_string(x[3])+";";return out;
}
inline std::vector<int64_t> retTags(const uint8_t *d,int64_t size){
    std::vector<int64_t> out;for(int64_t p=0;p+16<=size;p+=16)if(rd32(d,p)==0x60000000u&&rd32(d,p+4)==0&&rd32(d,p+8)==0&&rd32(d,p+12)==0)out.push_back(p);return out;
}
inline Result analyse(const uint8_t *d,int64_t size,const std::atomic_bool *cancel=nullptr,double minQuality=0.75){
    Result result;auto streams=findStreams(d,size,cancel);result.streams=int(streams.size());
    if(cancel&&cancel->load()){result.canceled=true;return result;}
    std::vector<Classified> cs;for(auto &b:batches(streams)){Classified c;if(classify(d,b,c))cs.push_back(std::move(c));}
    result.classified=int(cs.size());
    std::vector<std::string> order;std::map<std::string,std::vector<int>> groups;
    for(int i=0;i<int(cs.size());++i){auto s=signature(cs[i]);if(groups[s].empty())order.push_back(s);groups[s].push_back(i);}
    struct Pending{GroupReport report;std::vector<std::pair<int,std::vector<Tri>>> kept;std::vector<double> q;double scale=0;Rule rule;};
    std::vector<Pending> pending;
    for(auto &sig:order){
        if(cancel&&cancel->load()){result.canceled=true;return result;}
        auto &g=groups[sig];Pending p;p.report.signature=sig;p.report.batches=int(g.size());
        auto &u=cs[g[0]].streams[cs[g[0]].position].unpack;p.report.position="a"+std::to_string(u.addr)+" V"+std::to_string(u.vn)+"-"+std::to_string(bitsOf(u.vl));
        std::vector<Vec3> all;std::vector<double> steps;for(int i:g){auto &P=cs[i].P;all.insert(all.end(),P.begin(),P.end());for(size_t k=1;k<P.size();++k)steps.push_back(norm3(sub(P[k],P[k-1])));}
        const double dg=diagonal(all);
        if(dg==0||median(steps)/dg>0.08){p.report.rule="incoherent";p.report.reason="Consecutive vertices are not spatially coherent.";pending.push_back(std::move(p));continue;}
        std::vector<double> mags;mags.reserve(all.size()*3);for(auto &v:all)for(double x:v)mags.push_back(std::abs(x));p.scale=median(mags)+1e-12;
        std::map<std::string,double> score;std::vector<std::string> names;
        std::vector<std::map<std::string,std::pair<double,std::vector<Tri>>>> per;
        for(int i:g){
            std::map<std::string,std::pair<double,std::vector<Tri>>> r;
            for(auto &rule:rules(cs[i])){auto T=faces(cs[i],rule);auto q=quality(cs[i],T);if(!score.count(rule.name))names.push_back(rule.name);score[rule.name]+=double(q.second)-3.0*double(T.size()-q.second);r[rule.name]={q.first,std::move(T)};}
            per.push_back(std::move(r));
        }
        std::string best="strip";double bestScore=-1e300;
        for(auto &n:names){const double v=score[n]+(n=="strip"?0.01*std::abs(score[n]):0);if(v>bestScore){bestScore=v;best=n;}}
        p.report.rule=best;
        for(size_t k=0;k<g.size();++k){auto it=per[k].find(best);if(it==per[k].end()||it->second.first<minQuality)continue;p.kept.push_back({g[k],std::move(it->second.second)});p.q.push_back(it->second.first);}
        for(auto &r:rules(cs[g[0]]))if(r.name==best)p.rule=r;
        p.report.kept=int(p.kept.size());for(auto &k:p.kept)p.report.triangles+=k.second.size();
        if(p.report.batches<2||p.report.kept<2){p.report.reason="Fewer than two consistent batches.";p.kept.clear();}
        pending.push_back(std::move(p));
    }
    double ref=0;size_t refTris=0;for(auto &p:pending)if(!p.kept.empty()&&p.report.triangles>refTris){refTris=p.report.triangles;ref=p.scale;}
    result.rets=retTags(d,size);
    for(int gi=0;gi<int(pending.size());++gi){
        auto &p=pending[gi];
        if(!p.kept.empty()&&!(p.scale/ref>0.05&&p.scale/ref<20)){p.report.reason="Coordinate scale differs from the main geometry.";p.kept.clear();}
        p.report.accepted=!p.kept.empty();
        for(size_t k=0;k<p.kept.size();++k){
            auto &c=cs[p.kept[k].first];KeptBatch b;b.positions=c.P;b.normals=c.Nrm;b.uvs=c.UV;b.triangles=std::move(p.kept[k].second);b.quality=p.q[k];b.group=gi;
            auto &u=c.streams[c.position].unpack;b.positionOffset=u.data;b.positionStride=u.vl==3?2:u.vn*bitsOf(u.vl)/8;
            // A draw flag stored in the low mantissa bit of a float position is not geometry.
            if(p.rule.stream==c.position&&u.vl==0&&p.rule.bit==0)for(auto &v:b.positions){float f=float(v[p.rule.comp]);uint32_t bits;std::memcpy(&bits,&f,4);bits&=~1u;std::memcpy(&f,&bits,4);v[p.rule.comp]=f;}
            if(u.vl!=0)for(auto &v:b.positions)for(auto &x:v)x/=4096.0;
            if(c.uv>=0&&c.streams[c.uv].unpack.vl==1)for(auto &t:b.uvs){t[0]/=4096.0;t[1]/=4096.0;}
            b.part=int(std::lower_bound(result.rets.begin(),result.rets.end(),b.positionOffset)-result.rets.begin());
            result.batches.push_back(std::move(b));
        }
        result.groups.push_back(p.report);
    }
    return result;
}
}
