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

#include <Modifiers.h>
#include <MeshBase.h>
#include <Scene.h>
#include <Util.h>
#include <glm/glm.hpp>
#include <map>

namespace vt {

class Mesh;

Mesh* cast_mesh(MeshBase* mesh);

void mesh_attach(Scene* scene, MeshBase* mesh1, MeshBase* mesh2)
{
    mesh2->set_axis(mesh1->in_abs_system());
    mesh2->merge(mesh1, mesh1->get_material() == mesh2->get_material());
    scene->remove_mesh(cast_mesh(mesh1));
}

void mesh_apply_ripple(MeshBase* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase, bool smooth)
{
    size_t num_vertex = mesh->get_num_vertex();
    for(int i = 0; i < static_cast<int>(num_vertex); i++) {
        glm::vec3 pos = mesh->get_vert_coord(i);
        glm::vec2 p1(origin.x, origin.z);
        glm::vec2 p2(pos.x, pos.z);
        float radius = glm::distance(p1, p2);
        pos.y = origin.y + static_cast<float>(sin(radius / (wavelength / (PI * 2)) + phase)) * amplitude;
        mesh->set_vert_coord(i, pos);
    }
    if(smooth) {
        mesh->set_smooth(true);
    }
    mesh->update_normals_and_tangents();
    mesh->update_bbox();
}

void mesh_tessellate(MeshBase* mesh, tessellation_type_t tessellation_type, bool smooth)
{
    size_t prev_num_vert = mesh->get_num_vertex();
    size_t prev_num_tri  = mesh->get_num_tri();
    switch(tessellation_type) {
        case TESSELLATION_TYPE_EDGE_CENTER:
            {
                size_t new_num_vertex = prev_num_vert + prev_num_tri * 3;
                size_t new_num_tri    = prev_num_tri * 4;
                glm::vec3  new_vert_coord[new_num_vertex];
                glm::vec2  new_tex_coord[new_num_vertex];
                glm::ivec3 new_tri_indices[new_num_tri];
                for(int i = 0; i < static_cast<int>(prev_num_vert); i++) {
                    new_vert_coord[i] = mesh->get_vert_coord(i);
                    new_tex_coord[i]  = mesh->get_tex_coord(i);
                }

                int current_vert_index = prev_num_vert;
                int current_face_index = 0;
                std::map<uint32_t, int> shared_vert_map;
                for(int j = 0; j < static_cast<int>(prev_num_tri); j++) {
                    glm::ivec3 tri_indices = mesh->get_tri_indices(j);
                    glm::vec3 vert_a_coord = mesh->get_vert_coord(tri_indices[0]);
                    glm::vec3 vert_b_coord = mesh->get_vert_coord(tri_indices[1]);
                    glm::vec3 vert_c_coord = mesh->get_vert_coord(tri_indices[2]);
                    glm::vec2 tex_a_coord  = mesh->get_tex_coord(tri_indices[0]);
                    glm::vec2 tex_b_coord  = mesh->get_tex_coord(tri_indices[1]);
                    glm::vec2 tex_c_coord  = mesh->get_tex_coord(tri_indices[2]);

                    uint32_t new_vert_shared_ab_key = MAKELONG(std::min(tri_indices[0], tri_indices[1]), std::max(tri_indices[0], tri_indices[1]));
                    int new_vert_shared_ab_index = 0;
                    std::map<uint32_t, int>::iterator p = shared_vert_map.find(new_vert_shared_ab_key);
                    if(p == shared_vert_map.end()) {
                        new_vert_shared_ab_index = current_vert_index++;
                        shared_vert_map.insert(std::pair<uint32_t, int>(new_vert_shared_ab_key, new_vert_shared_ab_index));
                    } else {
                        new_vert_shared_ab_index = (*p).second;
                    }
                    new_vert_coord[new_vert_shared_ab_index] = (vert_a_coord + vert_b_coord) * 0.5f;
                    new_tex_coord[new_vert_shared_ab_index]  = (tex_a_coord + tex_b_coord) * 0.5f;

                    uint32_t new_vert_shared_bc_key = MAKELONG(std::min(tri_indices[1], tri_indices[2]), std::max(tri_indices[1], tri_indices[2]));
                    int new_vert_shared_bc_index = 0;
                    std::map<uint32_t, int>::iterator q = shared_vert_map.find(new_vert_shared_bc_key);
                    if(q == shared_vert_map.end()) {
                        new_vert_shared_bc_index = current_vert_index++;
                        shared_vert_map.insert(std::pair<uint32_t, int>(new_vert_shared_bc_key, new_vert_shared_bc_index));
                    } else {
                        new_vert_shared_bc_index = (*q).second;
                    }
                    new_vert_coord[new_vert_shared_bc_index] = (vert_b_coord + vert_c_coord) * 0.5f;
                    new_tex_coord[new_vert_shared_bc_index]  = (tex_b_coord + tex_c_coord) * 0.5f;

                    uint32_t new_vert_shared_ca_key = MAKELONG(std::min(tri_indices[2], tri_indices[0]), std::max(tri_indices[2], tri_indices[0]));
                    int new_vert_shared_ca_index = 0;
                    std::map<uint32_t, int>::iterator r = shared_vert_map.find(new_vert_shared_ca_key);
                    if(r == shared_vert_map.end()) {
                        new_vert_shared_ca_index = current_vert_index++;
                        shared_vert_map.insert(std::pair<uint32_t, int>(new_vert_shared_ca_key, new_vert_shared_ca_index));
                    } else {
                        new_vert_shared_ca_index = (*r).second;
                    }
                    new_vert_coord[new_vert_shared_ca_index] = (vert_c_coord + vert_a_coord) * 0.5f;
                    new_tex_coord[new_vert_shared_ca_index]  = (tex_c_coord + tex_a_coord) * 0.5f;

                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[0], new_vert_shared_ab_index, new_vert_shared_ca_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[1], new_vert_shared_bc_index, new_vert_shared_ab_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[2], new_vert_shared_ca_index, new_vert_shared_bc_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(new_vert_shared_ab_index, new_vert_shared_bc_index, new_vert_shared_ca_index);
                }

                new_num_vertex = current_vert_index;
                new_num_tri    = current_face_index;
                mesh->resize(new_num_vertex, new_num_tri);
                for(int k = 0; k < static_cast<int>(new_num_vertex); k++) {
                    mesh->set_vert_coord(k, new_vert_coord[k]);
                    mesh->set_tex_coord(k,  new_tex_coord[k]);
                }
                for(int t = 0; t < static_cast<int>(new_num_tri); t++) {
                    mesh->set_tri_indices(t, new_tri_indices[t]);
                }
            }
            break;
        case TESSELLATION_TYPE_TRI_CENTER:
            {
                size_t new_num_vertex = prev_num_vert + prev_num_tri;
                size_t new_num_tri    = prev_num_tri * 3;
                glm::vec3  new_vert_coord[new_num_vertex];
                glm::vec2  new_tex_coord[new_num_vertex];
                glm::ivec3 new_tri_indices[new_num_tri];
                for(int i = 0; i < static_cast<int>(prev_num_vert); i++) {
                    new_vert_coord[i] = mesh->get_vert_coord(i);
                    new_tex_coord[i]  = mesh->get_tex_coord(i);
                }

                int current_vert_index = prev_num_vert;
                int current_face_index = 0;
                std::map<uint32_t, int> shared_vert_map;
                for(int j = 0; j < static_cast<int>(prev_num_tri); j++) {
                    glm::ivec3 tri_indices = mesh->get_tri_indices(j);
                    glm::vec3 vert_a_coord = mesh->get_vert_coord(tri_indices[0]);
                    glm::vec3 vert_b_coord = mesh->get_vert_coord(tri_indices[1]);
                    glm::vec3 vert_c_coord = mesh->get_vert_coord(tri_indices[2]);
                    glm::vec2 tex_a_coord  = mesh->get_tex_coord(tri_indices[0]);
                    glm::vec2 tex_b_coord  = mesh->get_tex_coord(tri_indices[1]);
                    glm::vec2 tex_c_coord  = mesh->get_tex_coord(tri_indices[2]);

                    int new_vert_index = current_vert_index;
                    new_vert_coord[new_vert_index] = (vert_a_coord + vert_b_coord + vert_c_coord) * (1.0f / 3);
                    new_tex_coord[new_vert_index]  = (tex_a_coord + tex_b_coord + tex_c_coord) * (1.0f / 3);
                    current_vert_index++;

                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[0], tri_indices[1], new_vert_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[1], tri_indices[2], new_vert_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[2], tri_indices[0], new_vert_index);
                }

                new_num_vertex = current_vert_index;
                new_num_tri    = current_face_index;
                mesh->resize(new_num_vertex, new_num_tri);
                for(int k = 0; k < static_cast<int>(new_num_vertex); k++) {
                    mesh->set_vert_coord(k, new_vert_coord[k]);
                    mesh->set_tex_coord(k,  new_tex_coord[k]);
                }
                for(int t = 0; t < static_cast<int>(new_num_tri); t++) {
                    mesh->set_tri_indices(t, new_tri_indices[t]);
                }
            }
            break;
    }
    if(smooth) {
        mesh->set_smooth(true);
    }
    mesh->update_normals_and_tangents();
}

}
