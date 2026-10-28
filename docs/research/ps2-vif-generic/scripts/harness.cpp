#include "ps2_vif_core.h"
#include <cstdio>
#include <fstream>
#include <iterator>
int main(int argc,char**argv){
    for(int i=1;i<argc;++i){
        std::ifstream f(argv[i],std::ios::binary);std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)),{});
        auto r=Ps2VifCore::analyse(d.data(),d.size());
        int parts=0;std::vector<int> seen;for(auto &b:r.batches)if(std::find(seen.begin(),seen.end(),b.part)==seen.end())seen.push_back(b.part);
        printf("%s: classified %d kept %zu verts %zu tris %zu parts %zu\n",argv[i],r.classified,r.batches.size(),r.vertexCount(),r.triangleCount(),seen.size());
        for(auto &g:r.groups)if(g.batches>2)printf("    %d kept %d %s %s tris %zu %s\n",g.batches,g.kept,g.rule.c_str(),g.position.c_str(),g.triangles,g.reason.c_str());
        if(argc==3&&i==2){ // dump obj
            FILE*o=fopen("/home/claude/ps2/cpp/out.obj","w");size_t base=1;for(auto &b:r.batches){for(auto &v:b.positions)fprintf(o,"v %f %f %f\n",v[0],v[1],v[2]);for(auto &t:b.triangles)fprintf(o,"f %zu %zu %zu\n",t[0]+base,t[1]+base,t[2]+base);base+=b.positions.size();}fclose(o);}
    }
}
