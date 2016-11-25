#include <File3ds.h>
#include <MeshIFace.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdio.h>

#define MAKEWORD(a, b) ((uint16_t)(((uint8_t)(a))  | (((uint16_t)((uint8_t)(b))) << 8)))
#define MAKELONG(a, b) ((uint32_t)(((uint16_t)(a)) | (((uint32_t)((uint16_t)(b))) << 16)))

namespace vt {

MeshIFace* alloc_meshiface(std::string name, size_t num_vertex, size_t num_tri);

class Mesh;

Mesh* downcast_meshiface_to_mesh(MeshIFace* mesh);

static void read_string(FILE* stream, char* buf)
{
    int i = 0;
    do {
        buf[i] = fgetc(stream);
    } while(buf[i++]);
}

bool File3ds::load3ds(std::string filename, int index, std::vector<Mesh*>* meshes)
{
    std::vector<MeshIFace*> meshes_iface;
    if(!load3ds_impl(filename, index, &meshes_iface)) {
        return false;
    }
    for(std::vector<MeshIFace*>::iterator p = meshes_iface.begin(); p != meshes_iface.end(); p++) {
        meshes->push_back(downcast_meshiface_to_mesh(*p));
    }
    return true;
}

bool File3ds::load3ds_impl(std::string filename, int index, std::vector<MeshIFace*>* meshes)
{
    if(!meshes) {
        return false;
    }
    FILE* stream = fopen(filename.c_str(), "rb");
    if(stream) {
        glm::vec3 g_min, g_max;
        bool init_global_bbox = false;
        fseek(stream, 0, SEEK_END);
        uint32_t size = ftell(stream);
        rewind(stream);
        uint32_t mainEnd = enter_chunk(stream, MAIN3DS, size);
        uint32_t editEnd = enter_chunk(stream, EDIT3DS, mainEnd);
        int count = 0;
        while(ftell(stream) < editEnd) {
            uint32_t objEnd = enter_chunk(stream, EDIT_OBJECT, editEnd);
            char buf[80];
            read_string(stream, buf); // read object name
            int objType = read_short(stream);
            fseek(stream, -1*  sizeof(uint16_t), SEEK_CUR); // rewind
            if(objType == OBJ_TRIMESH) {
                if(count == index || index == -1) {
                    uint32_t meshEnd = enter_chunk(stream, OBJ_TRIMESH, objEnd);
                    if(meshEnd) {
                        uint32_t meshBase = ftell(stream);
                        //=========================================================
                        enter_chunk(stream, TRI_VERTEXL, meshEnd);
                        int vertCnt = read_short(stream);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        enter_chunk(stream, TRI_FACEL, meshEnd);
                        int faceCnt = read_short(stream);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        MeshIFace* mesh = alloc_meshiface(buf, vertCnt, faceCnt);
                        //=========================================================
                        enter_chunk(stream, TRI_VERTEXL, meshEnd);
                        read_vertices(stream, mesh);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        enter_chunk(stream, TRI_FACEL, meshEnd);
                        read_faces(stream, mesh);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        mesh->update_bbox();
                        glm::vec3 min, max;
                        mesh->get_min_max(&min, &max);
                        if(init_global_bbox) {
                            g_min = glm::min(g_min, min);
                            g_max = glm::max(g_max, max);
                        } else {
                            g_min = min;
                            g_max = max;
                            init_global_bbox = true;
                        }
                        meshes->push_back(mesh);
                    }
                    fseek(stream, meshEnd, SEEK_SET);
                } else {
                    break;
                }
                count++;
            }
            fseek(stream, objEnd, SEEK_SET);
        }
        fclose(stream);
        glm::vec3 g_center = g_min;
        g_center += g_max;
        g_center *= 0.5;
        for(std::vector<MeshIFace*>::iterator p = meshes->begin(); p != meshes->end(); p++) {
            (*p)->xform_vertices(glm::translate(glm::mat4(1), -g_center));
            (*p)->update_normals_and_tangents();
        }
    }
    return true;
}

uint32_t File3ds::enter_chunk(FILE* stream, uint32_t chunkID, uint32_t chunkEnd)
{
    uint32_t offset = 0;
    while(ftell(stream) < chunkEnd) {
        uint32_t pChunkID = read_short(stream);
        uint32_t chunkSize = read_long(stream);
        if(pChunkID == chunkID) {
            offset = -1*  (sizeof(uint16_t)+sizeof(uint32_t))+chunkSize;
            break;
        } else {
            fseek(stream, -1*  (sizeof(uint16_t)+sizeof(uint32_t)), SEEK_CUR); // rewind
            fseek(stream, chunkSize, SEEK_CUR); // skip this chunk
        }
    }
    return ftell(stream)+offset;
}

void File3ds::read_vertices(FILE* stream, MeshIFace* mesh)
{
    float* vertex = new float[3];
    fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip list size
    size_t vertCnt = mesh->get_num_vertex();
    for(int i = 0; i < static_cast<int>(vertCnt); i++) {
        fread(vertex, sizeof(float), 3, stream);
        mesh->set_vert_coord(i, glm::vec3(vertex[0], vertex[2], vertex[1]));
    }
    delete []vertex;
}

void File3ds::read_faces(FILE* stream, MeshIFace* mesh)
{
    uint16_t* pFace = new uint16_t[3];
    fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip list size
    size_t faceCnt = mesh->get_num_tri();
    for(int i = 0; i < static_cast<int>(faceCnt); i++) {
        fread(pFace, sizeof(uint16_t), 3, stream);
        mesh->set_tri_indices(i, glm::uvec3(pFace[0], pFace[2], pFace[1]));
        fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip face info
    }
    delete []pFace;
}

uint16_t File3ds::read_short(FILE* stream)
{
    uint8_t loByte;
    uint8_t hiByte;
    fread(&loByte, sizeof(uint8_t), 1, stream);
    fread(&hiByte, sizeof(uint8_t), 1, stream);
    return MAKEWORD(loByte, hiByte);
}

uint32_t File3ds::read_long(FILE* stream)
{
    uint16_t loWord = read_short(stream);
    uint16_t hiWord = read_short(stream);
    return MAKELONG(loWord, hiWord);
}

}
