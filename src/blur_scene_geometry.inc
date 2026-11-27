// Included inside BlurScene after Reader/directory. Original PC serializer subset.
namespace Geometry {
inline constexpr auto ProfileId="native-blur-scene";
struct R:Reader {
    const std::atomic_bool &cancel;
    R(const QByteArray &b,qint64 p,qint64 end,const std::atomic_bool &c):Reader{b,p,end},cancel(c){}
    quint32 count(){auto n=u();if(n>100000||cancel.load())ok=false;return ok?n:0;}
    quint16 version(){auto n=u();if((n&65535)!=0x4152||(n>>16)>4)ok=false;return n>>16;}
    bool f(){if(!frame())ok=false;return ok;}
    quint32 array(){f();return count();}
    QString text(){auto n=count();auto at=p;if(!take(n))return {};return QString::fromLatin1(b.constData()+at,n);}
    Pair ref(){f();return {u(),u()};}
    void payload(){f();u();f();auto n=count();take(n);}
    void cull(){auto v=version();if(v>2)ok=false;f();text();take(qint64(array())*8);take(16);f();take(32);payload();if(v)u();if(v>1)payload();}
    qint64 blob(quint64 size){f();if(u()!=size||u()!=16)ok=false;qint64 found=-1;int matches=0;
        for(int pad=0;ok&&pad<32;++pad){auto at=p+pad;if(at>end)break;if(pad&&b[at-1])break;if(at%16||size>quint64(end-at))continue;auto next=at+qint64(size);
            if((next<=end-4&&b.mid(next,4)==QByteArray("RA\0\0",4))||(next<=end-5&&b.mid(next,5)==QByteArray("\2RA\1\0",5))){found=at;++matches;}}
        if(matches!=1){ok=false;return -1;}p=found+qint64(size);return found;
    }
};
struct Buffer {qint64 payload=0;quint32 count=0,stride=0;int position=-1,uv=-1;quint32 positionKind=6;bool colorOnly=false;};
struct Group {QVector<Buffer> vertices,indices;};
inline Group resourceBuffers(R &r){const auto &b=r.b;Group g;
            auto nv=r.array();if(nv>1024){r.ok=false;return g;}
            for(quint32 i=0;r.ok&&i<nv;++i){auto at=r.p;if(!r.take(1)||uchar(b[at])!=2||!r.frame(1)){r.ok=false;break;}Buffer v;r.f();v.stride=r.u();auto n=r.array();if(!v.stride||v.stride>256||!n||n>16){r.ok=false;break;}
                QVector<std::array<quint32,4>> attrs;for(quint32 j=0;j<n;++j){r.f();attrs.append({r.u(),r.u(),r.u(),r.u()});}if(r.array()!=n)r.ok=false;
                for(quint32 j=0;j<n&&r.ok;++j){auto off=r.p;if(!r.take(2))break;auto o=qFromLittleEndian<quint16>(b.constData()+off);auto a=attrs[j];
                    if((a[0]==6||a[0]==7)&&a[2]==0&&a[3]==0){if(v.position>=0||o+(a[0]==7?16:12)>v.stride)r.ok=false;v.position=o;v.positionKind=a[0];}
                    if((a[0]==8||a[0]==9)&&a[2]==5&&a[3]==0){if(v.uv>=0||o+(a[0]==9?8:4)>v.stride)r.ok=false;v.uv=o;}}
                v.colorOnly=r.ok&&n==1&&v.stride==4&&attrs[0]==std::array<quint32,4>{11,0,9,0}&&qFromLittleEndian<quint16>(b.constData()+r.p-2)==0;
                v.count=r.count();v.payload=r.blob(quint64(v.count)*v.stride);g.vertices.append(v);}
            auto ni=r.array();if(ni>1024){r.ok=false;return g;}
            for(quint32 i=0;r.ok&&i<ni;++i){auto at=r.p;if(!r.take(1)||uchar(b[at])!=2||!r.frame(1)){r.ok=false;break;}Buffer v;v.count=r.count();auto type=r.u();if(type>1){r.ok=false;break;}v.stride=type?4:2;v.payload=r.blob(quint64(v.count)*v.stride);g.indices.append(v);}
    if(r.array()!=0)r.ok=false;return g;
}
inline QMap<quint32,Group> buffers(const QByteArray &b,const QVector<Block> &blocks,const std::atomic_bool &cancel){
    QMap<quint32,Group> out;QSet<quint32> duplicates;
    for(const auto &block:blocks){R r(b,block.offset+24,block.offset+block.size,cancel);
        for(auto key:block.pairs){if(cancel.load())return {};r.f();r.f();if(r.u()!=2||r.u()!=key.variant||key.variant)break;Group g;
            g=resourceBuffers(r);
            if(r.u()!=0||!r.ok)break;
            if(out.contains(key.group))duplicates.insert(key.group);else if(!g.vertices.isEmpty()&&!g.indices.isEmpty())out[key.group]=g;
        }}for(auto group:duplicates)out.remove(group);return out;
}
// Exact group/index identity; choose largest decoded mip variant for a base-colour preview.
inline QString imageKey(Pair pair){return QString::number(pair.group)+":"+QString::number(pair.variant);}
inline QMap<QString,Image> catalog(const QByteArray &b,const QByteArray &render,const std::atomic_bool &cancel){
    QMap<QString,Image> best;QMap<QString,QString> names;QSet<QString> ambiguous;
    for(const auto &block:directory(render,b,cancel)){R r(b,block.offset+24,block.offset+block.size,cancel);
        for(auto key:block.pairs){if(cancel.load())return {};r.f();r.f();if(r.u()!=2||r.u()!=key.variant||!r.ok)break;if(!key.variant)resourceBuffers(r);if(!r.ok)break;auto n=r.count();if(n>1024)break;QVector<Image> staged;
            for(quint32 i=0;i<n&&r.ok;++i){Image imageRecord;bool inherited=false;if(!image(r,key,i,imageRecord,inherited)){r.ok=false;break;}if(!inherited)staged.append(imageRecord);}if(!r.ok)break;
            for(auto imageRecord:staged){auto id=imageKey({imageRecord.group,imageRecord.index});if(!imageRecord.name.isEmpty()){if(names.contains(id)&&names[id]!=imageRecord.name)ambiguous.insert(id);else names[id]=imageRecord.name;}
                if(!best.contains(id)||quint64(imageRecord.w)*imageRecord.h>quint64(best[id].w)*best[id].h||(quint64(imageRecord.w)*imageRecord.h==quint64(best[id].w)*best[id].h&&imageRecord.variant<best[id].variant))best[id]=imageRecord;}
        }}for(auto i=best.begin();i!=best.end();++i)i->name=names.value(i.key());for(auto id:ambiguous)best.remove(id);return best;
}
struct Field {QString name;quint32 type=0,count=0;};
struct Schema {QString name;QVector<Field> fields;};
inline QVector<Schema> descriptors(R &r){r.f();auto n=r.array();QVector<Schema> out;if(n>1024){r.ok=false;return {};}
    for(quint32 i=0;r.ok&&i<n;++i){r.f();Schema s;s.name=r.text();auto nf=r.array();if(nf>1024){r.ok=false;break;}for(quint32 j=0;j<nf&&r.ok;++j){r.f();Field f;f.name=r.text();f.type=r.u();f.count=r.count();s.fields.append(f);}r.take(8);r.take(qint64(r.array())*2);r.take(8);out.append(s);}return out;}
struct Param {QString name;bool identity=false,hasWorld=false;std::array<float,16> world{};QMap<QString,quint32> textures;};
inline QVector<Param> parameters(R &r,const QVector<Schema> &schemas){auto n=r.count();auto size=r.u();auto start=r.p;qint64 referenceOverhead=0;QVector<Param> out;
    for(quint32 i=0;r.ok&&i<n;++i){auto index=r.u();if(index>=quint32(schemas.size())){r.ok=false;break;}const auto &s=schemas[index];Param p;p.name=s.name;
        for(const auto &f:s.fields){if(f.type>10){r.ok=false;break;}auto at=r.p;
            if(f.type==8||f.type==9){for(quint32 k=0;k<f.count&&r.ok;++k){auto value=r.ref();if(value.variant>15)r.ok=false;}referenceOverhead+=qint64(f.count)*8;if(!r.ok)break;}
            else{auto bytes=quint64(f.count)*(f.type<2?1:f.type<4?2:4);if(!r.take(bytes))break;}
            if(f.type==7&&f.count==1)p.textures[f.name]=Blur::word(r.b,at);
            if(f.name=="g_mWorld"&&f.type==6&&f.count==16){p.identity=true;p.hasWorld=true;for(int k=0;k<16;++k){p.world[k]=Blur::real(r.b,at+4*k);if(!std::isfinite(p.world[k]))p.hasWorld=false;if(p.world[k]!=float(k%5==0))p.identity=false;}}}
        out.append(p);}if(r.p-start!=qint64(size)+4*qint64(n)+referenceOverhead)r.ok=false;return out;}
struct Binding {quint32 vertex=0,stream=0,offset=0,extra=0;Pair parameter;};
struct Draw {quint32 primitive=0,index=0;Pair instance;QVector<std::array<quint32,4>> ranges;QVector<Binding> bindings;};
inline QVector<Binding> bindings(R &r){QVector<Binding> out;auto n=r.array();if(n>16){r.ok=false;return {};}
    for(quint32 i=0;r.ok&&i<n;++i){r.f();Binding v;v.vertex=r.u();v.stream=r.u();v.offset=r.u();v.extra=r.u();v.parameter=r.ref();out.append(v);}return out;}
inline Draw draw(R &r){Draw d;auto v=r.version();if(v>1)r.ok=false;r.u();r.u();d.primitive=r.u();d.index=r.u();r.u();r.ref();if(v)d.instance=r.ref();else d.instance={0xffffffff,0xffffffff};r.f();r.take(24);auto n=r.array();if(n>10000){r.ok=false;return d;}
    for(quint32 i=0;i<n&&r.ok;++i){r.f();d.ranges.append({r.u(),r.u(),r.u(),r.u()});}d.bindings=bindings(r);return d;}
inline void bits(R &r){auto v=r.version();if(v>1)r.ok=false;r.u();if(v)r.take(qint64(r.array())*2);r.u();r.take(r.array());}
struct Instance {quint16 version=0;quint32 prototype=0,flags=0;Pair parameter;QVector<Binding> bindings;};
struct List {qint64 source=0;QVector<Draw> draws,prototypes;QVector<Instance> instances;QVector<std::array<float,8>> instanceBounds;QVector<Param> params;QVector<Pair> indices,vertices,textures;};
inline List list(R &r){List l;l.source=r.p;r.f();auto v=r.version();if(v<2)r.ok=false;r.cull();auto schemas=descriptors(r);parameters(r,schemas);auto n=r.array();
    for(quint32 i=0;i<n&&r.ok;++i)l.draws.append(draw(r));n=r.array();for(quint32 i=0;i<n&&r.ok;++i){Instance v;v.version=r.version();if(v.version>1)r.ok=false;v.prototype=r.u();v.parameter=r.ref();v.bindings=bindings(r);v.flags=r.u();l.instances.append(v);}
    if(r.array()!=0)r.ok=false;n=r.array();for(quint32 i=0;i<n&&r.ok;++i)l.prototypes.append(draw(r));l.params=parameters(r,schemas);
    n=r.array();for(quint32 i=0;i<n&&r.ok;++i){r.f();r.take(24);}for(int k=0;k<3;++k){n=r.array();for(quint32 i=0;i<n&&r.ok;++i){r.f();auto at=r.p;if(!r.take(32))break;if(k==1){std::array<float,8> box;for(int j=0;j<8;++j)box[j]=Blur::real(r.b,at+4*j);l.instanceBounds.append(box);}}}
    for(int k=0;k<3;++k)bits(r);r.take(qint64(r.array())*4);n=r.array();for(quint32 i=0;i<n&&r.ok;++i){r.f();for(int k=0;k<2;++k){r.f();r.u();r.take(r.array());}}
    if(v==3){for(int k=0;k<2&&r.ok;++k){auto schema=descriptors(r);parameters(r,schema);}}else if(v>3){n=r.array();for(quint32 i=0;i<n&&r.ok;++i){auto schema=descriptors(r);parameters(r,schema);}}
    n=r.array();for(quint32 i=0;i<n&&r.ok;++i)l.indices.append({r.u(),r.u()});n=r.array();for(quint32 i=0;i<n&&r.ok;++i)l.vertices.append({r.u(),r.u()});n=r.array();for(quint32 i=0;i<n&&r.ok;++i)l.textures.append({r.u(),r.u()});return l;}
struct Section {qint64 list=0;int draw=0,instance=-1,usedVertices=0;Buffer vb,ib;quint32 byteOffset=0;QVector<std::array<quint32,4>> ranges;QString layout,albedoField;bool billboard=false,flipWinding=false;std::array<float,16> world{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::array<float,8> sourceBounds{};Pair albedo{0xffffffff,0xffffffff};};
inline QVector<quint32> triangles(const QByteArray &b,const Section &s,const std::atomic_bool &cancel){QVector<quint32> out;
    for(const auto &range:s.ranges){auto first=range[0],n=range[1];if(n<3||first>s.ib.count||n>s.ib.count-first)return {};quint32 lo=0xffffffff,hi=0,a=0,c=0;
        for(quint32 j=0;j<n;++j){if((j&4095)==0&&cancel.load())return {};auto at=s.ib.payload+qint64(first+j)*s.ib.stride;auto v=s.ib.stride==2?qFromLittleEndian<quint16>(b.constData()+at):Blur::word(b,at);lo=qMin(lo,v);hi=qMax(hi,v);
            if(v>=s.vb.count-s.byteOffset/s.vb.stride)return {};if(j>=2&&a!=c&&a!=v&&c!=v){if(out.size()>30000000)return {};out.append(j&1?c:a);out.append(j&1?a:c);out.append(v);}a=c;c=v;}
        if(lo!=range[2]||quint64(hi)-lo+1!=range[3])return {};}
    return out;}
inline bool rigidShader(const QString &name){return (name.startsWith("default_")&&!name.contains("_skin_"))||name=="SpeedTree_LN.fx"||name=="SpeedTreeLeaf_LN.fx"||name=="SyntheticBillboard_LN.fx"||name.startsWith("HighVis");}
inline std::array<double,3> position(const QByteArray &b,const Section &s,quint32 index){auto at=s.vb.payload+s.byteOffset+qint64(index)*s.vb.stride+s.vb.position;std::array<double,3> local{Blur::real(b,at),Blur::real(b,at+4),Blur::real(b,at+8)},world{};for(int k=0;k<3;++k){world[k]=s.world[12+k];for(int j=0;j<3;++j)world[k]+=local[j]*s.world[4*j+k];}return world;}
inline bool affine(const std::array<float,16> &matrix,double &det){for(auto f:matrix)if(!std::isfinite(f))return false;if(std::abs(matrix[3])>1e-6||std::abs(matrix[7])>1e-6||std::abs(matrix[11])>1e-6||std::abs(matrix[15]-1)>1e-6)return false;
    det=double(matrix[0])*(double(matrix[5])*matrix[10]-double(matrix[6])*matrix[9])-double(matrix[1])*(double(matrix[4])*matrix[10]-double(matrix[6])*matrix[8])+double(matrix[2])*(double(matrix[4])*matrix[9]-double(matrix[5])*matrix[8]);return std::abs(det)>1e-8;}
inline bool resolve(const QMap<quint32,Group> &groups,const List &l,const Draw &d,Section &s){
    if(d.primitive!=1||d.index>=quint32(l.indices.size()))return false;auto ir=l.indices[d.index];auto ig=groups.constFind(ir.group);if(ig==groups.cend()||ir.variant>=quint32(ig->indices.size()))return false;int mains=0;Binding main;for(auto v:d.bindings)if(!v.stream){main=v;++mains;}
    if(mains!=1||main.vertex>=quint32(l.vertices.size())||main.parameter.variant||main.parameter.group>=quint32(l.params.size()))return false;auto vr=l.vertices[main.vertex];auto vg=groups.constFind(vr.group);if(vg==groups.cend()||vr.variant>=quint32(vg->vertices.size()))return false;
    s.list=l.source;s.vb=vg->vertices[vr.variant];s.ib=ig->indices[ir.variant];s.byteOffset=main.offset;s.ranges=d.ranges;const auto &material=l.params[main.parameter.group];s.layout=material.name;
    for(const auto &field:{QString("g_tAlbedo"),QString("g_tTexture")})if(material.textures.contains(field)){auto slot=material.textures[field];if(slot<quint32(l.textures.size())){s.albedo=l.textures[slot];s.albedoField=field;}break;}
    s.billboard=s.vb.positionKind==7;if(s.billboard&&s.layout!="BillBoard_LN.fx"&&s.layout!="FoliageBillBoard_LN.fx"&&(s.instance<0||(s.layout!="SpeedTreeLeaf_LN.fx"&&s.layout!="SyntheticBillboard_LN.fx")))return false;
    return s.vb.position>=0&&s.vb.stride&&s.byteOffset%s.vb.stride==0&&quint64(s.byteOffset)<quint64(s.vb.count)*s.vb.stride;
}
inline bool validate(const QByteArray &b,Section &s,const std::atomic_bool &cancel){auto faces=triangles(b,s,cancel);if(faces.isEmpty())return false;QVector<quint8> used(s.vb.count-s.byteOffset/s.vb.stride);std::array<double,3> lo{1e100,1e100,1e100},hi{-1e100,-1e100,-1e100};s.usedVertices=0;
    for(auto index:faces){if(used[index])continue;used[index]=1;++s.usedVertices;auto at=s.vb.payload+s.byteOffset+qint64(index)*s.vb.stride+s.vb.position;if(s.billboard&&Blur::real(b,at+12)!=0.f)return false;auto p=position(b,s,index);for(int k=0;k<3;++k){if(!std::isfinite(p[k])||std::abs(p[k])>1e7)return false;lo[k]=qMin(lo[k],p[k]);hi[k]=qMax(hi[k],p[k]);}if(cancel.load())return false;}
    if(s.instance>=0){double extent=0;for(int k=0;k<3;++k){if(!std::isfinite(s.sourceBounds[k])||!std::isfinite(s.sourceBounds[k+3])||s.sourceBounds[k]>s.sourceBounds[k+3])return false;extent=qMax(extent,double(s.sourceBounds[k+3])-s.sourceBounds[k]);}double tolerance=s.layout=="SpeedTreeLeaf_LN.fx"?qMax(.05,.05*extent):.05;for(int k=0;k<3;++k)if(lo[k]<double(s.sourceBounds[k])-tolerance||hi[k]>double(s.sourceBounds[k+3])+tolerance)return false;}
    return true;
}
inline bool colorOverrides(const QMap<quint32,Group> &groups,const List &l,const Instance &instance,const Section &s){
    for(const auto &binding:instance.bindings){if(binding.stream!=1||binding.parameter.variant||binding.parameter.group>=quint32(l.params.size())||!l.params[binding.parameter.group].name.isEmpty()||binding.vertex>=quint32(l.vertices.size()))return false;auto ref=l.vertices[binding.vertex];auto group=groups.constFind(ref.group);if(group==groups.cend()||ref.variant>=quint32(group->vertices.size()))return false;const auto &v=group->vertices[ref.variant];if(!v.colorOnly||binding.offset%4)return false;
        for(auto range:s.ranges)if(quint64(binding.offset)/4+quint64(range[2])+range[3]>v.count)return false;}
    return true;
}
inline QVector<Section> inspect(const QByteArray &b,const QByteArray &render,const std::atomic_bool &cancel,QJsonObject *stats=nullptr){auto blocks=directory(render,b,cancel);auto groups=buffers(b,blocks,cancel);QVector<Section> out;const QByteArray name="RenderingData::RenderListNode_Common";int instanceRows=0,placed=0,skinned=0,unsupported=0,duplicates=0,lists=0;
    for(const auto &block:blocks){qint64 pos=block.offset+24;while((pos=b.indexOf(name,pos))>=0&&pos+name.size()<block.offset+block.size){auto at=pos;pos+=name.size();if(cancel.load())return {};if(at<block.offset+29||uchar(b[at-5])!=3||Blur::word(b,at-4)!=quint32(name.size()))continue;R r(b,pos,block.offset+block.size,cancel);auto l=list(r);if(!r.ok)continue;++lists;
            for(int di=0;di<l.draws.size();++di){const auto &d=l.draws[di];if(d.instance.variant||d.instance.group>=quint32(l.params.size())||!l.params[d.instance.group].identity)continue;Section s;s.draw=di;if(resolve(groups,l,d,s)&&validate(b,s,cancel))out.append(s);}
            instanceRows+=l.instances.size();QSet<QByteArray> seen;
            for(int ii=0;ii<l.instances.size();++ii){if(cancel.load())return {};const auto &i=l.instances[ii];if(i.version||i.flags||i.prototype>=quint32(l.prototypes.size())||i.parameter.variant||i.parameter.group>=quint32(l.params.size())||l.instanceBounds.size()!=l.instances.size()){++unsupported;continue;}const auto &p=l.params[i.parameter.group];if(!p.hasWorld){++unsupported;continue;}Section s;s.instance=ii;s.draw=i.prototype;s.world=p.world;s.sourceBounds=l.instanceBounds[ii];double det=0;
                if(!affine(s.world,det)||!resolve(groups,l,l.prototypes[i.prototype],s)){++unsupported;continue;}if(s.layout.contains("_skin_")){++skinned;continue;}if(!rigidShader(s.layout)||!colorOverrides(groups,l,i,s)||!validate(b,s,cancel)){++unsupported;continue;}s.flipWinding=det<0;
                QByteArray key=QByteArray::number(i.prototype)+':';for(float value:s.world){if(value==0)value=0;key.append(reinterpret_cast<const char *>(&value),4);}if(seen.contains(key)){++duplicates;continue;}seen.insert(key);out.append(s);++placed;
            }pos=qMax(pos,r.p);}}
    if(stats)*stats={{"renderLists",lists},{"instanceRecords",instanceRows},{"placedInstances",placed},{"skinnedInstancesPending",skinned},{"unsupportedInstances",unsupported},{"duplicateInstances",duplicates}};return cancel.load()?QVector<Section>{}:out;
}
inline QString identity(const Section &s){return s.instance<0?QString("%1:%2").arg(s.list).arg(s.draw):QString("%1:instance:%2").arg(s.list).arg(s.instance);}
inline QJsonArray candidates(const QByteArray &b,const QByteArray &render,const std::atomic_bool &cancel,QJsonObject *stats=nullptr){QJsonArray out;auto textures=catalog(b,render,cancel);for(const auto &s:inspect(b,render,cancel,stats)){auto faces=triangles(b,s,cancel);out.append(QJsonObject{{"name",QString(s.instance>=0?"Blur scene / list 0x%1 / instance %2 / %3":s.billboard?"Blur scene / list 0x%1 / billboard %2 / %3":"Blur scene / list 0x%1 / section %2 / %3").arg(s.list,0,16).arg(s.instance>=0?s.instance+1:s.draw+1).arg(s.layout)},{"profileId",ProfileId},{"profileName","Blur PC scene"},{"nativeDecoder",true},{"placedInstance",s.instance>=0},{"instanceIndex",s.instance},{"prototypeIndex",s.instance>=0?s.draw:-1},{"billboardGeometry",s.billboard},{"billboardFacingApplied",false},{"objectOffset",identity(s)},{"vertexCount",qint64(s.usedVertices)},{"triangleCount",faces.size()/3},{"textureAssigned",textures.contains(imageKey(s.albedo))&&s.vb.uv>=0},{"blurTextureGroup",qint64(s.albedo.group)},{"blurTextureIndex",qint64(s.albedo.variant)},{"blurTextureField",s.albedoField},{"settings",QJsonObject{{"vertexOffset",QString::number(s.vb.payload+s.byteOffset)},{"vertexCount",qint64(s.usedVertices)},{"indexCount",faces.size()}}},{"basis","Serialized draw, declared position layout, verified world placement and independently bounded triangle-strip ranges. Base colour follows original material slot and resource pair; largest decoded variant preview, billboards use stored fixed-orientation quads, full shader and additional packet kinds pending."}});}return out;}
struct Session {QByteArray key,stream,render;QVector<Section> sections;QMap<QString,qsizetype> byIdentity;QMap<QString,Image> textures;mutable std::mutex imageMutex;mutable QMap<QString,QImage> decodedImages;mutable qint64 decodedBytes=0;};
inline std::mutex sessionMutex;
inline std::shared_ptr<const Session> lastSession;
inline QByteArray sessionKey(const ReadRequest &source,const ReadRequest &sibling,const PatchList &patches){
    QCryptographicHash hash(QCryptographicHash::Sha256);for(const auto *request:{&source,&sibling}){auto path=QFileInfo(request->path).absoluteFilePath().toUtf8();hash.addData(path);hash.addData(QByteArray(1,0));hash.addData(QByteArray::number(request->expectedSize));hash.addData(QByteArray(1,0));hash.addData(QByteArray::number(request->expectedModified.toMSecsSinceEpoch()));hash.addData(QByteArray(1,0));}
    for(const auto &patch:patches){hash.addData(QByteArray::number(patch.offset));hash.addData(QByteArray(1,0));hash.addData(QByteArray::number(patch.bytes.size()));hash.addData(QByteArray(1,0));hash.addData(patch.bytes);}return hash.result();
}
inline std::shared_ptr<const Session> session(const ReadRequest &source,const ReadRequest &sibling,const PatchList &patches,const std::atomic_bool &cancel,QString &error){
    if(cancel.load()){error="Blur scene decoding canceled.";return {};}
    for(auto request:{source,sibling}){request.offset=0;request.length=0;auto check=readPage(request);if(!check.error.isEmpty()){error=check.error;return {};}}
    auto key=sessionKey(source,sibling,patches);{std::lock_guard<std::mutex> guard(sessionMutex);if(lastSession&&lastSession->stream.size()>AssetMemory::bytes()/4)lastSession.reset();if(lastSession&&lastSession->key==key)return lastSession;}
    auto loaded=std::make_shared<Session>();loaded->key=key;loaded->render=readComplete(sibling,{},cancel,error);if(!error.isEmpty())return {};loaded->stream=readComplete(source,patches,cancel,error);if(!error.isEmpty())return {};loaded->sections=inspect(loaded->stream,loaded->render,cancel);loaded->textures=catalog(loaded->stream,loaded->render,cancel);for(qsizetype i=0;i<loaded->sections.size();++i)loaded->byIdentity[identity(loaded->sections[i])]=i;
    if(cancel.load()){error="Blur scene decoding canceled.";return {};}
    for(auto request:{source,sibling}){request.offset=0;request.length=0;auto check=readPage(request);if(!check.error.isEmpty()){error=check.error;return {};}}
    {std::lock_guard<std::mutex> guard(sessionMutex);lastSession=loaded;}return loaded;
}
inline MeshResult decode(ReadRequest source,const PatchList &patches,const QJsonObject &candidate,const std::atomic_bool &cancel){
    MeshResult result;auto fail=[&](QString e){result.error=e;result.mesh.reset();return result;};
    if(source.memory||QFileInfo(source.path).fileName().compare("level.levelstream",Qt::CaseInsensitive)||source.expectedSize<24||source.expectedSize>AssetMemory::bytes()/4)return fail("Blur scene requires a complete filesystem level.levelstream within the input budget.");
    auto info=QFileInfo(QFileInfo(source.path).dir().filePath("level.levelrender"));if(!info.isFile()||info.isSymLink()||info.size()<=0||info.size()>qMin<qint64>(AssetMemory::bytes()/16,64*1024*1024))return fail("Blur scene needs its same-folder level.levelrender.");
    ReadRequest sibling;sibling.path=info.filePath();sibling.expectedSize=info.size();sibling.expectedModified=info.lastModified();QString error;auto loaded=session(source,sibling,patches,cancel,error);if(!loaded)return fail(error);
    const auto &b=loaded->stream;auto found=loaded->byIdentity.constFind(candidate["objectOffset"].toString());if(found==loaded->byIdentity.cend())return fail("Blur scene section is unsupported or changed. Scan again.");const auto &selected=loaded->sections[found.value()];
    auto faces=triangles(b,selected,cancel);if(selected.flipWinding)for(qsizetype i=0;i<faces.size();i+=3)std::swap(faces[i+1],faces[i+2]);auto mesh=std::make_shared<MeshData>();QMap<quint32,quint32> remap;mesh->hasUvs=selected.vb.uv>=0;
    for(auto idx:faces){if(cancel.load())return fail("Blur scene decoding canceled.");if(!remap.contains(idx)){MeshVertex v{};auto at=selected.vb.payload+selected.byteOffset+qint64(idx)*selected.vb.stride;auto world=position(b,selected,idx);for(int k=0;k<3;++k)v.position[k]=float(world[k]);if(mesh->hasUvs)for(int k=0;k<2;++k){v.uv[k]=Blur::meshUv(Blur::halfValue(qFromLittleEndian<quint16>(b.constData()+at+selected.vb.uv+2*k)),k);if(!std::isfinite(v.uv[k]))return fail("Invalid Blur UV value.");}remap[idx]=mesh->vertices.size();mesh->vertices.append(v);mesh->vertexOffsets.append(at+selected.vb.position);}mesh->triangles.append(remap[idx]);}
    if(mesh->triangles.isEmpty())return fail("Blur scene has no supported triangles.");QVector<std::array<double,3>> sums(mesh->vertices.size());
    for(qsizetype i=0;i<mesh->triangles.size();i+=3){auto a=mesh->triangles[i],d=mesh->triangles[i+1],c=mesh->triangles[i+2];double u[3],v[3];for(int k=0;k<3;++k){u[k]=double(mesh->vertices[d].position[k])-mesh->vertices[a].position[k];v[k]=double(mesh->vertices[c].position[k])-mesh->vertices[a].position[k];}std::array<double,3> n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};for(auto idx:{a,d,c})for(int k=0;k<3;++k)sums[idx][k]+=n[k];MeshFaceSource f;f.strip=0;f.offsets={mesh->vertexOffsets[a],mesh->vertexOffsets[d],mesh->vertexOffsets[c]};f.indexSlots={-1,-1,-1};mesh->faceSources.append(f);}
    for(qsizetype i=0;i<mesh->vertices.size();++i){auto n=sums[i];auto length=std::hypot(n[0],n[1],n[2]);for(int k=0;k<3;++k)mesh->vertices[i].normal[k]=length>1e-12?float(n[k]/length):float(k==1);}
    auto check=source;check.offset=0;check.length=0;auto final=readPage(check);sibling.length=0;auto directoryCheck=readPage(sibling);if(!final.error.isEmpty()||!directoryCheck.error.isEmpty()||cancel.load())return fail("Blur scene source changed or decoding canceled.");
    result.mesh=mesh;result.surface=std::make_shared<MeshSurface>();result.surface->material.name=selected.layout;result.surface->material.doubleSided=selected.billboard;result.surface->material.alphaMask=selected.billboard;result.surface->material.sourceBindings={{"format","Blur PC scene"},{"textureAssigned",false},{"textureGroup",qint64(selected.albedo.group)},{"textureIndex",qint64(selected.albedo.variant)},{"textureStatus",selected.albedo.group==0xffffffff?"Global scene texture unresolved":"Primary texture or UV0 unresolved"},{"gameShaderApplied",false},{"normalsGenerated",true},{"identityWorldVerified",selected.instance<0},{"instanceWorldApplied",selected.instance>=0},{"instanceIndex",selected.instance},{"prototypeIndex",selected.instance>=0?selected.draw:-1},{"instanceBoundsVerified",selected.instance>=0},{"negativeDeterminantWindingCorrected",selected.flipWinding},{"sourceVertexColoursApplied",false},{"billboardGeometry",selected.billboard},{"billboardFacingApplied",false},{"additionalPacketKindsPending",true},{"sourceBasisPreserved",true},{"uvDecoded",mesh->hasUvs},{"sourceUvTopLeft",true},{"uvVFlipped",mesh->hasUvs},{"drawIndex",selected.draw},{"renderListOffset",QString::number(selected.list)}};
    const auto &textures=loaded->textures;auto id=imageKey(selected.albedo);
    if(mesh->hasUvs&&textures.contains(id)){const auto &image=textures[id];ImageRecipe recipe;recipe.offset=0;recipe.width=image.w;recipe.height=image.h;recipe.format=image.format;QString error;auto size=imageSpan(recipe,error);
        if(!error.isEmpty()||size<=0||image.offset<0||size>image.size||image.offset>b.size()-size)return fail("Blur bound texture span is invalid.");
        {std::lock_guard<std::mutex> guard(loaded->imageMutex);result.surface->texture=loaded->decodedImages.value(id);}
        if(result.surface->texture.isNull()){auto decoded=decodeTextureData(b.mid(image.offset,size),{},recipe,error,cancel).convertToFormat(QImage::Format_RGBA8888);if(decoded.isNull())return fail(error.isEmpty()?"Blur bound texture failed to decode.":error);std::lock_guard<std::mutex> guard(loaded->imageMutex);if(loaded->decodedImages.contains(id))result.surface->texture=loaded->decodedImages[id];else{result.surface->texture=decoded;if(decoded.sizeInBytes()<=AssetMemory::textureBytes()-loaded->decodedBytes){loaded->decodedBytes+=decoded.sizeInBytes();loaded->decodedImages[id]=decoded;}}}
        result.surface->textureName=image.name.isEmpty()?QString("Blur resource %1").arg(id):image.name;result.surface->textureLibrary[result.surface->textureName]=result.surface->texture;result.surface->material.unlit=true;
        auto &bindings=result.surface->material.sourceBindings;bindings["textureAssigned"]=true;bindings["textureStatus"]="Decoded original primary resource";bindings["textureAssignmentVerified"]=true;bindings["textureAssignmentMethod"]="Original material parameter -> RenderList texture slot -> resource group/index";bindings["textureField"]=selected.albedoField;bindings["textureGroup"]=qint64(selected.albedo.group);bindings["textureIndex"]=qint64(selected.albedo.variant);bindings["sourceImageOffset"]=QString::number(image.offset);bindings["sourceImageName"]=image.name;bindings["textureVariant"]=qint64(image.variant);bindings["variantSelectionVerified"]=false;bindings["variantSelection"]="Largest decoded variant, base-colour preview";bindings["textureWrapU"]="Repeat";bindings["textureWrapV"]="Repeat";
    }
    if(cancel.load())return fail("Blur scene decoding canceled.");final=readPage(check);directoryCheck=readPage(sibling);if(!final.error.isEmpty()||!directoryCheck.error.isEmpty())return fail("Blur scene changed during texture decoding.");return result;
}
} // Geometry
