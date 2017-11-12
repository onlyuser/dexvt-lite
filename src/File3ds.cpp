// This file is part of dexvt-lite.
// -- 3D Inverse Kinematics (Cyclic Coordinate Descent) with Constraints
// Copyright (C) 2018 onlyuser <mailto:onlyuser@gmail.com>
//
// dexvt-lite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dexvt-lite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dexvt-lite.  If not, see <http://www.gnu.org/licenses/>.

#include <File3ds.h>
#include <MeshBase.h>
#include <Util.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdio.h>

namespace vt {

MeshBase* alloc_mesh_base(std::string name, size_t num_vertex, size_t num_tri);

class Mesh;

Mesh* cast_mesh(MeshBase* mesh);

static void read_string(FILE* stream, char* buf)
{
    int i = 0;
    do {
        buf[i] = fgetc(stream);
    } while(buf[i++]);
}

bool File3ds::load3ds(std::string filename, int index, std::vector<Mesh*>* meshes)
{
    std::vector<MeshBase*> meshes_iface;
    if(!load3ds_impl(filename, index, &meshes_iface)) {
        return false;
    }
    for(std::vector<MeshBase*>::iterator p = meshes_iface.begin(); p != meshes_iface.end(); p++) {
        meshes->push_back(cast_mesh(*p));
    }
    return true;
}

bool File3ds::load3ds_impl(std::string filename, int index, std::vector<MeshBase*>* meshes)
{
    if(!meshes) {
        return false;
    }
    FILE* stream = fopen(filename.c_str(), "rb");
    if(stream) {
        glm::vec3 global_min, global_max;
        bool init_global_bbox = false;
        fseek(stream, 0, SEEK_END);
        uint32_t size = ftell(stream);
        rewind(stream);
        uint32_t main_end = enter_chunk(stream, MAIN3DS, size);
        uint32_t edit_end = enter_chunk(stream, EDIT3DS, main_end);
        int count = 0;
        while(ftell(stream) < edit_end) {
            uint32_t object_end = enter_chunk(stream, EDIT_OBJECT, edit_end);
            char buf[80];
            read_string(stream, buf); // read object name
            int object_type = read_short(stream);
            fseek(stream, -sizeof(uint16_t), SEEK_CUR); // rewind
            if(object_type == OBJ_TRIMESH) {
                if(count != index && index != -1) {
                    break;
                }
                uint32_t mesh_end = enter_chunk(stream, OBJ_TRIMESH, object_end);
                if(mesh_end) {
                    uint32_t mesh_base = ftell(stream);

                    enter_chunk(stream, TRI_VERTEXL, mesh_end);
                    int num_vertex = read_short(stream);
                    fseek(stream, mesh_base, SEEK_SET);

                    enter_chunk(stream, TRI_FACEL, mesh_end);
                    int num_tri = read_short(stream);
                    fseek(stream, mesh_base, SEEK_SET);

                    MeshBase* mesh = alloc_mesh_base(buf, num_vertex, num_tri);

                    enter_chunk(stream, TRI_VERTEXL, mesh_end);
                    read_vertices(stream, mesh);
                    fseek(stream, mesh_base, SEEK_SET);

                    enter_chunk(stream, TRI_FACEL, mesh_end);
                    read_faces(stream, mesh);
                    fseek(stream, mesh_base, SEEK_SET);

                    mesh->update_bbox();
                    glm::vec3 local_min, local_max;
                    mesh->get_min_max(&local_min, &local_max);
                    if(init_global_bbox) {
                        global_min = glm::min(global_min, local_min);
                        global_max = glm::max(global_max, local_max);
                    } else {
                        global_min = local_min;
                        global_max = local_max;
                        init_global_bbox = true;
                    }
                    meshes->push_back(mesh);
                }
                fseek(stream, mesh_end, SEEK_SET);
                count++;
            }
            fseek(stream, object_end, SEEK_SET);
        }
        fclose(stream);
        glm::vec3 global_center = (global_min + global_max) * 0.5f;
        for(std::vector<MeshBase*>::iterator p = meshes->begin(); p != meshes->end(); p++) {
            (*p)->set_axis(global_center);
            (*p)->update_normals_and_tangents();
            (*p)->update_bbox();
        }
    }
    return true;
}

uint32_t File3ds::enter_chunk(FILE* stream, uint32_t chunk_id, uint32_t chunk_end)
{
    uint32_t offset = 0;
    while(ftell(stream) < chunk_end) {
        uint32_t _chunk_id = read_short(stream);
        uint32_t chunk_size = read_long(stream);
        if(_chunk_id == chunk_id) {
            offset = -(sizeof(uint16_t)+sizeof(uint32_t))+chunk_size;
            break;
        } else {
            fseek(stream, -(sizeof(uint16_t)+sizeof(uint32_t)), SEEK_CUR); // rewind
            fseek(stream, chunk_size, SEEK_CUR); // skip this chunk
        }
    }
    return ftell(stream)+offset;
}

void File3ds::read_vertices(FILE* stream, MeshBase* mesh)
{
    float* vert_coord = new float[3];
    fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip list size
    size_t num_vertex = mesh->get_num_vertex();
    for(int i = 0; i < static_cast<int>(num_vertex); i++) {
        fread(vert_coord, sizeof(float), 3, stream);
        mesh->set_vert_coord(i, glm::vec3(vert_coord[0], vert_coord[2], vert_coord[1]));
    }
    delete[] vert_coord;
}

void File3ds::read_faces(FILE* stream, MeshBase* mesh)
{
    uint16_t* tri_indices = new uint16_t[3];
    fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip list size
    size_t num_tri = mesh->get_num_tri();
    for(int i = 0; i < static_cast<int>(num_tri); i++) {
        fread(tri_indices, sizeof(uint16_t), 3, stream);
        mesh->set_tri_indices(i, glm::ivec3(tri_indices[0], tri_indices[2], tri_indices[1]));
        fseek(stream, sizeof(uint16_t), SEEK_CUR); // skip tri_indices info
    }
    delete[] tri_indices;
}

uint16_t File3ds::read_short(FILE* stream)
{
    uint8_t lo_byte;
    uint8_t hi_byte;
    fread(&lo_byte, sizeof(uint8_t), 1, stream);
    fread(&hi_byte, sizeof(uint8_t), 1, stream);
    return MAKEWORD(lo_byte, hi_byte);
}

uint32_t File3ds::read_long(FILE* stream)
{
    uint16_t lo_word = read_short(stream);
    uint16_t hi_word = read_short(stream);
    return MAKELONG(lo_word, hi_word);
}

}
