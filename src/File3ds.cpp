#include <File3ds.h>
#include <Mesh.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdio.h>

#define MAKEWORD(a, b) ((uint16_t)(((uint8_t)(a))  | (((uint16_t)((uint8_t)(b))) << 8)))
#define MAKELONG(a, b) ((uint32_t)(((uint16_t)(a)) | (((uint32_t)((uint16_t)(b))) << 16)))

namespace vt {

static void readString(FILE *stream, char *buf)
{
    int i = 0;
    do {
        buf[i] = fgetc(stream);
    } while(buf[i++]);
}

bool File3ds::load3ds(std::string filename, int index, std::vector<Mesh*>* meshes)
{
    if(!meshes) {
        return false;
    }
    FILE *stream = fopen(filename.c_str(), "rb");
    if(stream) {
        glm::vec3 g_min, g_max;
        bool init_global_bbox = false;
        fseek(stream, 0, SEEK_END);
        uint32_t size = ftell(stream);
        rewind(stream);
        uint32_t mainEnd = enterChunk(stream, MAIN3DS, size);
        uint32_t editEnd = enterChunk(stream, EDIT3DS, mainEnd);
        int count = 0;
        while(ftell(stream) < editEnd) {
            uint32_t objEnd = enterChunk(stream, EDIT_OBJECT, editEnd);
            char buf[80];
            readString(stream, buf); // read object name
            int objType = readShort(stream);
            fseek(stream, -1 * sizeof(uint16_t), SEEK_CUR); // rewind
            if(objType == OBJ_TRIMESH) {
                if(count == index || index == -1) {
                    uint32_t meshEnd = enterChunk(stream, OBJ_TRIMESH, objEnd);
                    if(meshEnd) {
                        uint32_t meshBase = ftell(stream);
                        //=========================================================
                        enterChunk(stream, TRI_VERTEXL, meshEnd);
                        int vertCnt = readShort(stream);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        enterChunk(stream, TRI_FACEL, meshEnd);
                        int faceCnt = readShort(stream);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        Mesh* mesh = new Mesh(buf, vertCnt, faceCnt);
                        //=========================================================
                        enterChunk(stream, TRI_VERTEXL, meshEnd);
                        readVertList(stream, mesh);
                        fseek(stream, meshBase, SEEK_SET);
                        //=========================================================
                        enterChunk(stream, TRI_FACEL, meshEnd);
                        readFaceList(stream, mesh);
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
        for(std::vector<vt::Mesh*>::iterator p = meshes->begin(); p != meshes->end(); p++) {
            (*p)->xform_vertices(glm::translate(glm::mat4(1), -g_center));
            (*p)->update_normals_and_tangents();
        }
    }
    return true;
}

uint32_t File3ds::enterChunk(FILE *stream, uint32_t chunkID, uint32_t chunkEnd)
{
    uint32_t offset = 0;
    while(ftell(stream) < chunkEnd) {
        uint32_t pChunkID = readShort(stream);
        uint32_t chunkSize = readLong(stream);
        if(pChunkID == chunkID) {
            offset = -1 * (sizeof(uint16_t)+sizeof(uint32_t))+chunkSize;
            break;
        } else {
            fseek(stream, -1 * (sizeof(uint16_t)+sizeof(uint32_t)), SEEK_CUR); // rewind
            fseek(stream, chunkSize, SEEK_CUR); // skip this chunk
        }
    }
    return ftell(stream)+offset;
}

void File3ds::readVertList(FILE *stream, Mesh *mesh)
{
    float *vertex = new float[3];
    fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip list size
    size_t vertCnt = mesh->get_num_vertex();
    for(int i = 0; i < static_cast<int>(vertCnt); i++) {
        fread(vertex, sizeof(float), 3, stream);
        mesh->set_vert_coord(i, glm::vec3(vertex[0], vertex[2], vertex[1]));
    }
    delete []vertex;
}

void File3ds::readFaceList(FILE *stream, Mesh *mesh)
{
    uint16_t *pFace = new uint16_t[3];
    fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip list size
    size_t faceCnt = mesh->get_num_tri();
    for(int i = 0; i < static_cast<int>(faceCnt); i++) {
        fread(pFace, sizeof(uint16_t), 3, stream);
        mesh->set_tri_indices(i, glm::uvec3(pFace[0], pFace[2], pFace[1]));
        fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip face info
    }
    delete []pFace;
}

uint16_t File3ds::readShort(FILE *stream)
{
    uint8_t loByte;
    uint8_t hiByte;
    fread(&loByte, sizeof(uint8_t), 1, stream);
    fread(&hiByte, sizeof(uint8_t), 1, stream);
    return MAKEWORD(loByte, hiByte);
}

uint32_t File3ds::readLong(FILE *stream)
{
    uint16_t loWord = readShort(stream);
    uint16_t hiWord = readShort(stream);
    return MAKELONG(loWord, hiWord);
}

}
