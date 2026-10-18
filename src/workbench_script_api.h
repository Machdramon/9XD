#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>
#include <array>
#include <cstring>
#include <cmath>
struct BWVertex { float position[3],normal[3],uv[2]; };
struct BWApi {
    uint32_t version;
    int64_t size,selectionStart,selectionEnd;
    const char *path;
    const BWVertex *vertices;uint64_t vertexCount;
    const uint32_t *indices;uint64_t indexCount;
    int (*read)(int64_t offset,int64_t count,uint8_t *output);
    int (*publish)(const BWVertex *,uint64_t,const uint32_t *,uint64_t);
    const char *(*error)();
};
namespace bw {
inline const BWApi *api=nullptr;
inline int64_t size(){return api->size;}
inline std::string path(){return api->path;}
inline std::array<int64_t,2> selection(){return {api->selectionStart,api->selectionEnd};}
inline std::vector<uint8_t> read(int64_t offset,int64_t count){if(count<0||count>1048576)throw std::runtime_error("read: maximum 1 MiB");std::vector<uint8_t> bytes(count);if(!api->read(offset,count,bytes.data()))throw std::runtime_error(api->error());return bytes;}
struct Mesh { std::vector<BWVertex> vertices;std::vector<uint32_t> indices; };
inline Mesh mesh(){Mesh m;if(api->vertexCount)m.vertices.assign(api->vertices,api->vertices+api->vertexCount);if(api->indexCount)m.indices.assign(api->indices,api->indices+api->indexCount);return m;}
inline void set_mesh(const Mesh &mesh){if(!api->publish(mesh.vertices.data(),mesh.vertices.size(),mesh.indices.data(),mesh.indices.size()))throw std::runtime_error(api->error());}
}
