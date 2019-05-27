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

#include <Mesh.h>
#include <Buffer.h>
#include <Material.h>
#include <Texture.h>
#include <PrimitiveFactory.h>
#include <Util.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <string>
#include <cstring>
#include <iostream>

namespace vt {

Mesh::Mesh(const std::string& name,
                 size_t       num_vertex,
                 size_t       num_tri)
    : TransformObject(name),
      m_num_vertex(num_vertex),
      m_num_tri(num_tri),
      m_visible(true),
      m_smooth(false),
      m_vbo_vert_coords(NULL),
      m_vbo_vert_normal(NULL),
      m_vbo_vert_tangent(NULL),
      m_vbo_tex_coords(NULL),
      m_ibo_tri_indices(NULL),
      m_buffers_already_init(false),
      m_material(NULL),
      m_shader_context(NULL),
      m_normal_shader_context(NULL),
      m_wireframe_shader_context(NULL),
      m_ssao_shader_context(NULL),
      m_color_texture_index(-1),
      m_color_texture2_index(-1),
      m_color_texture_source(-1),
      m_bump_texture_index(-1),
      m_env_map_texture_index(-1),
      m_random_texture_index(-1),
      m_frontface_depth_overlay_texture_index(-1),
      m_backface_depth_overlay_texture_index(-1),
      m_backface_normal_overlay_texture_index(-1),
      m_reflect_to_refract_ratio(1)
{
    m_vert_coords   = new GLfloat[ num_vertex * 3];
    m_vert_normal   = new GLfloat[ num_vertex * 3];
    m_vert_tangent  = new GLfloat[ num_vertex * 3];
    m_tex_coords    = new GLfloat[ num_vertex * 2];
    m_tri_indices   = new GLushort[num_tri    * 3];
    memset(m_vert_coords,  0, sizeof(GLfloat)  * num_vertex * 3);
    memset(m_vert_normal,  0, sizeof(GLfloat)  * num_vertex * 3);
    memset(m_vert_tangent, 0, sizeof(GLfloat)  * num_vertex * 3);
    memset(m_tex_coords,   0, sizeof(GLfloat)  * num_vertex * 2);
    memset(m_tri_indices,  0, sizeof(GLushort) * num_tri    * 3);
    m_ambient_color = new GLfloat[3];
    m_ambient_color[0] = 1;
    m_ambient_color[1] = 1;
    m_ambient_color[2] = 1;
}

Mesh::~Mesh()
{
    if(m_vert_coords)              { delete[] m_vert_coords; }
    if(m_vert_normal)              { delete[] m_vert_normal; }
    if(m_vert_tangent)             { delete[] m_vert_tangent; }
    if(m_tex_coords)               { delete[] m_tex_coords; }
    if(m_tri_indices)              { delete[] m_tri_indices; }
    if(m_ambient_color)            { delete[] m_ambient_color; }
    if(m_vbo_vert_coords)          { delete m_vbo_vert_coords; }
    if(m_vbo_vert_normal)          { delete m_vbo_vert_normal; }
    if(m_vbo_vert_tangent)         { delete m_vbo_vert_tangent; }
    if(m_vbo_tex_coords)           { delete m_vbo_tex_coords; }
    if(m_ibo_tri_indices)          { delete m_ibo_tri_indices; }
    if(m_shader_context)           { delete m_shader_context; }
    if(m_normal_shader_context)    { delete m_normal_shader_context; }
    if(m_wireframe_shader_context) { delete m_wireframe_shader_context; }
    if(m_ssao_shader_context)      { delete m_ssao_shader_context; }
}

void Mesh::resize(size_t num_vertex, size_t num_tri, bool preserve_mesh_geometry)
{
    glm::vec3*  backup_vert_coord   = NULL;
    glm::vec3*  backup_vert_normal  = NULL;
    glm::vec3*  backup_vert_tangent = NULL;
    glm::vec2*  backup_tex_coord    = NULL;
    glm::ivec3* backup_tri_indices  = NULL;
    if(preserve_mesh_geometry) {
        backup_vert_coord   = new glm::vec3[num_vertex];
        backup_vert_normal  = new glm::vec3[num_vertex];
        backup_vert_tangent = new glm::vec3[num_vertex];
        backup_tex_coord    = new glm::vec2[num_vertex];
        backup_tri_indices  = new glm::ivec3[num_tri];
        if(backup_vert_coord && backup_vert_normal && backup_vert_tangent && backup_tex_coord) {
            for(int i = 0; i < static_cast<int>(num_vertex); i++) {
                backup_vert_coord[i]   = get_vert_coord(i);
                backup_vert_normal[i]  = get_vert_normal(i);
                backup_vert_tangent[i] = get_vert_tangent(i);
                backup_tex_coord[i]    = get_tex_coord(i);
            }
        }
        if(backup_tri_indices) {
            for(int i = 0; i < static_cast<int>(num_tri); i++) {
                backup_tri_indices[i] = get_tri_indices(i);
            }
        }
    }
    if(m_vert_coords)              { delete[] m_vert_coords; }
    if(m_vert_normal)              { delete[] m_vert_normal; }
    if(m_vert_tangent)             { delete[] m_vert_tangent; }
    if(m_tex_coords)               { delete[] m_tex_coords; }
    if(m_tri_indices)              { delete[] m_tri_indices; }
    if(m_vbo_vert_coords)          { delete m_vbo_vert_coords;          m_vbo_vert_coords = NULL; }
    if(m_vbo_vert_normal)          { delete m_vbo_vert_normal;          m_vbo_vert_normal = NULL; }
    if(m_vbo_vert_tangent)         { delete m_vbo_vert_tangent;         m_vbo_vert_tangent = NULL; }
    if(m_vbo_tex_coords)           { delete m_vbo_tex_coords;           m_vbo_tex_coords = NULL; }
    if(m_ibo_tri_indices)          { delete m_ibo_tri_indices;          m_ibo_tri_indices = NULL; }
    if(m_shader_context)           { delete m_shader_context;           m_shader_context = NULL; }
    if(m_normal_shader_context)    { delete m_normal_shader_context;    m_normal_shader_context = NULL; }
    if(m_wireframe_shader_context) { delete m_wireframe_shader_context; m_wireframe_shader_context = NULL; }
    if(m_ssao_shader_context)      { delete m_ssao_shader_context;      m_ssao_shader_context = NULL; }
    m_num_vertex   = num_vertex;
    m_num_tri      = num_tri;
    m_vert_coords  = new GLfloat[ num_vertex * 3];
    m_vert_normal  = new GLfloat[ num_vertex * 3];
    m_vert_tangent = new GLfloat[ num_vertex * 3];
    m_tex_coords   = new GLfloat[ num_vertex * 2];
    m_tri_indices  = new GLushort[num_tri    * 3];
    memset(m_vert_coords,  0, sizeof(GLfloat)  * num_vertex * 3);
    memset(m_vert_normal,  0, sizeof(GLfloat)  * num_vertex * 3);
    memset(m_vert_tangent, 0, sizeof(GLfloat)  * num_vertex * 3);
    memset(m_tex_coords,   0, sizeof(GLfloat)  * num_vertex * 2);
    memset(m_tri_indices,  0, sizeof(GLushort) * num_tri    * 3);
    m_buffers_already_init = false;
    if(preserve_mesh_geometry) {
        if(backup_vert_coord && backup_vert_normal && backup_vert_tangent && backup_tex_coord) {
            for(int i = 0; i < static_cast<int>(num_vertex); i++) {
                set_vert_coord(i,   backup_vert_coord[i]);
                set_vert_normal(i,  backup_vert_normal[i]);
                set_vert_tangent(i, backup_vert_tangent[i]);
                set_tex_coord(i,    backup_tex_coord[i]);
            }
            delete[] backup_vert_coord;
            delete[] backup_vert_normal;
            delete[] backup_vert_tangent;
            delete[] backup_tex_coord;
        }
        if(backup_tri_indices) {
            for(int i = 0; i < static_cast<int>(num_tri); i++) {
                set_tri_indices(i, backup_tri_indices[i]);
            }
            delete[] backup_tri_indices;
        }
    }
}

void Mesh::merge(const MeshBase* other, bool copy_tex_coords)
{
    size_t prev_num_vertex  = get_num_vertex();
    size_t prev_num_tri     = get_num_tri();
    size_t other_num_vertex = other->get_num_vertex();
    size_t other_num_tri    = other->get_num_tri();
    resize(prev_num_vertex + other_num_vertex,
           prev_num_tri    + other_num_tri,
           true);
    for(int i = 0; i < static_cast<int>(other_num_vertex); i++) {
        set_vert_coord(prev_num_vertex + i,   other->get_vert_coord(i));
        set_vert_normal(prev_num_vertex + i,  other->get_vert_normal(i));
        set_vert_tangent(prev_num_vertex + i, other->get_vert_tangent(i));
    }
    if(copy_tex_coords) {
        for(int j = 0; j < static_cast<int>(other_num_vertex); j++) {
            set_tex_coord(prev_num_vertex + j, other->get_tex_coord(j));
        }
    }
    for(int k = 0; k < static_cast<int>(other_num_tri); k++) {
        glm::ivec3 prev_tri_indices = glm::ivec3(prev_num_vertex, prev_num_vertex, prev_num_vertex);
        set_tri_indices(prev_num_tri + k, prev_tri_indices + other->get_tri_indices(k));
    }
    update_bbox();
}

glm::vec3 Mesh::get_vert_coord(int index) const
{
    int offset = index * 3;
    return glm::vec3(m_vert_coords[offset + 0],
                     m_vert_coords[offset + 1],
                     m_vert_coords[offset + 2]);
}

void Mesh::set_vert_coord(int index, glm::vec3 coord)
{
    int offset = index * 3;
    m_vert_coords[offset + 0] = coord.x;
    m_vert_coords[offset + 1] = coord.y;
    m_vert_coords[offset + 2] = coord.z;
}

glm::vec3 Mesh::get_vert_normal(int index) const
{
    int offset = index * 3;
    return glm::vec3(m_vert_normal[offset + 0],
                     m_vert_normal[offset + 1],
                     m_vert_normal[offset + 2]);
}

void Mesh::set_vert_normal(int index, glm::vec3 normal)
{
    int offset = index * 3;
    m_vert_normal[offset + 0] = normal.x;
    m_vert_normal[offset + 1] = normal.y;
    m_vert_normal[offset + 2] = normal.z;
}

glm::vec3 Mesh::get_vert_tangent(int index) const
{
    int offset = index * 3;
    return glm::vec3(m_vert_tangent[offset + 0],
                     m_vert_tangent[offset + 1],
                     m_vert_tangent[offset + 2]);
}

void Mesh::set_vert_tangent(int index, glm::vec3 tangent)
{
    int offset = index * 3;
    m_vert_tangent[offset + 0] = tangent.x;
    m_vert_tangent[offset + 1] = tangent.y;
    m_vert_tangent[offset + 2] = tangent.z;
}

glm::vec2 Mesh::get_tex_coord(int index) const
{
    int offset = index * 2;
    return glm::vec2(m_tex_coords[offset + 0],
                     m_tex_coords[offset + 1]);
}

void Mesh::set_tex_coord(int index, glm::vec2 coord)
{
    int offset = index*2;
    m_tex_coords[offset+0] = coord.x;
    m_tex_coords[offset+1] = coord.y;
}

glm::ivec3 Mesh::get_tri_indices(int index) const
{
    int offset = index * 3;
    return glm::ivec3(m_tri_indices[offset + 0],
                      m_tri_indices[offset + 1],
                      m_tri_indices[offset + 2]);
}

void Mesh::set_tri_indices(int index, glm::ivec3 indices)
{
    assert(indices[0] >= 0 && indices[0] < static_cast<int>(m_num_vertex));
    assert(indices[1] >= 0 && indices[1] < static_cast<int>(m_num_vertex));
    assert(indices[2] >= 0 && indices[2] < static_cast<int>(m_num_vertex));
    int offset = index * 3;
    m_tri_indices[offset + 0] = indices[0];
    m_tri_indices[offset + 1] = indices[1];
    m_tri_indices[offset + 2] = indices[2];
}

glm::vec3 Mesh::get_vert_bitangent(int index) const
{
    return safe_normalize(glm::cross(get_vert_normal(index), get_vert_tangent(index)));
}

void Mesh::update_bbox()
{
#if 1
    glm::ivec3 tri_indices = get_tri_indices(0);
    m_min = m_max = get_vert_coord(tri_indices[0]);
    m_min = m_max = get_vert_coord(tri_indices[1]);
    m_min = m_max = get_vert_coord(tri_indices[2]);
    for(int i = 1; i < static_cast<int>(m_num_tri); i++) {
        glm::ivec3 tri_indices = get_tri_indices(i);
        glm::vec3 p0 = get_vert_coord(tri_indices[0]);
        glm::vec3 p1 = get_vert_coord(tri_indices[1]);
        glm::vec3 p2 = get_vert_coord(tri_indices[2]);
        m_max = glm::max(m_max, p0);
        m_min = glm::min(m_min, p0);
        m_max = glm::max(m_max, p1);
        m_min = glm::min(m_min, p1);
        m_max = glm::max(m_max, p2);
        m_min = glm::min(m_min, p2);
    }
#else
    m_min = m_max = get_vert_coord(0);
    for(int i = 1; i < static_cast<int>(m_num_vertex); i++) {
        glm::vec3 cur = get_vert_coord(i);
        m_max = glm::max(m_max, cur);
        m_min = glm::min(m_min, cur);
    }
#endif
}

void Mesh::update_normals_and_tangents()
{
    for(int i = 0; i < static_cast<int>(m_num_vertex); i++) {
        set_vert_normal( i, glm::vec3(0));
        set_vert_tangent(i, glm::vec3(0));
    }
    if(m_smooth) {
        for(int i = 0; i < static_cast<int>(m_num_tri); i++) {
            glm::ivec3 tri_indices = get_tri_indices(i);
            glm::vec3 p0 = get_vert_coord(tri_indices[0]);
            glm::vec3 p1 = get_vert_coord(tri_indices[1]);
            glm::vec3 p2 = get_vert_coord(tri_indices[2]);
            glm::vec3 e1 = safe_normalize(p1 - p0);
            glm::vec3 e2 = safe_normalize(p2 - p0);
            glm::vec3 n  = safe_normalize(glm::cross(e1, e2));
            for(int j = 0; j < 3; j++) {
                int vert_index = tri_indices[j];
                set_vert_normal( vert_index, get_vert_normal(vert_index) + n);
                set_vert_tangent(vert_index, glm::vec3(0));
            }
        }
        for(int k = 0; k < static_cast<int>(m_num_vertex); k++) {
            set_vert_normal(k, safe_normalize(get_vert_normal(k)));
        }
        resize(m_num_vertex, m_num_tri, true);
        return;
    }
    for(int i = 0; i < static_cast<int>(m_num_tri); i++) {
        glm::ivec3 tri_indices = get_tri_indices(i);
        glm::vec3 p0 = get_vert_coord(tri_indices[0]);
        glm::vec3 p1 = get_vert_coord(tri_indices[1]);
        glm::vec3 p2 = get_vert_coord(tri_indices[2]);
        glm::vec3 e1 = safe_normalize(p1 - p0);
        glm::vec3 e2 = safe_normalize(p2 - p0);
        glm::vec3 n  = safe_normalize(glm::cross(e1, e2));
        for(int j = 0; j < 3; j++) {
            int vert_index = tri_indices[j];
            set_vert_normal( vert_index, n);
            set_vert_tangent(vert_index, e1);
        }
    }
}

// NOTE: required by base class pure virtual despite being defined in another base class
void Mesh::get_min_max(glm::vec3* min, glm::vec3* max) const
{
    BBoxObject::get_min_max(min, max);
}

// NOTE: required by base class pure virtual despite being defined in another base class
glm::vec3 Mesh::in_abs_system(glm::vec3 local_point)
{
    return TransformObject::in_abs_system(local_point);
}

void Mesh::init_buffers()
{
    if(m_buffers_already_init) {
        return;
    }
    m_vbo_vert_coords  = new Buffer(GL_ARRAY_BUFFER,         sizeof(GLfloat)  * m_num_vertex * 3, m_vert_coords);
    m_vbo_vert_normal  = new Buffer(GL_ARRAY_BUFFER,         sizeof(GLfloat)  * m_num_vertex * 3, m_vert_normal);
    m_vbo_vert_tangent = new Buffer(GL_ARRAY_BUFFER,         sizeof(GLfloat)  * m_num_vertex * 3, m_vert_tangent);
    m_vbo_tex_coords   = new Buffer(GL_ARRAY_BUFFER,         sizeof(GLfloat)  * m_num_vertex * 2, m_tex_coords);
    m_ibo_tri_indices  = new Buffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * m_num_tri    * 3, m_tri_indices);
    m_buffers_already_init = true;
}

void Mesh::update_buffers() const
{
    if(!m_buffers_already_init) {
        return;
    }
    m_vbo_vert_coords->update();
    m_vbo_vert_normal->update();
    m_vbo_vert_tangent->update();
    m_vbo_tex_coords->update();
    m_ibo_tri_indices->update();
}

Buffer* Mesh::get_vbo_vert_coords()
{
    init_buffers();
    return m_vbo_vert_coords;
}

Buffer* Mesh::get_vbo_vert_normal()
{
    init_buffers();
    return m_vbo_vert_normal;
}

Buffer* Mesh::get_vbo_vert_tangent()
{
    init_buffers();
    return m_vbo_vert_tangent;
}

Buffer* Mesh::get_vbo_tex_coords()
{
    init_buffers();
    return m_vbo_tex_coords;
}

Buffer* Mesh::get_ibo_tri_indices()
{
    init_buffers();
    return m_ibo_tri_indices;
}

void Mesh::set_material(Material* material)
{
    // NOTE: texture index for same texture varies from material to material
    if(material == m_material) {
        return;
    }
    std::string texture_name;
    if(m_material) {
        Texture* texture = m_material->get_texture_by_index(m_color_texture_index);
        if(texture) {
            texture_name = texture->get_name();
        }
    }
    if(m_shader_context) {
        delete m_shader_context;
        m_shader_context = NULL;
    }
    m_material = material;
    m_color_texture_index = material ? material->get_texture_index_by_name(texture_name) : -1;
}

ShaderContext* Mesh::get_shader_context()
{
    if(m_shader_context || !m_material) { // FIX-ME! -- potential bug if Material not set
        return m_shader_context;
    }
    m_shader_context = new ShaderContext(m_material,
                                         get_vbo_vert_coords(),
                                         get_vbo_vert_normal(),
                                         get_vbo_vert_tangent(),
                                         get_vbo_tex_coords(),
                                         get_ibo_tri_indices());
    return m_shader_context;
}

ShaderContext* Mesh::get_normal_shader_context(Material* normal_material)
{
    if(m_normal_shader_context || !normal_material) { // FIX-ME! -- potential bug if Material not set
        return m_normal_shader_context;
    }
    m_normal_shader_context = new ShaderContext(normal_material,
                                                get_vbo_vert_coords(),
                                                get_vbo_vert_normal(),
                                                get_vbo_vert_tangent(),
                                                get_vbo_tex_coords(),
                                                get_ibo_tri_indices());
    return m_normal_shader_context;
}

ShaderContext* Mesh::get_wireframe_shader_context(Material* wireframe_material)
{
    if(m_wireframe_shader_context || !wireframe_material) { // FIX-ME! -- potential bug if Material not set
        return m_wireframe_shader_context;
    }
    m_wireframe_shader_context = new ShaderContext(wireframe_material,
                                                   get_vbo_vert_coords(),
                                                   get_vbo_vert_normal(),
                                                   get_vbo_vert_tangent(),
                                                   get_vbo_tex_coords(),
                                                   get_ibo_tri_indices());
    return m_wireframe_shader_context;
}

ShaderContext* Mesh::get_ssao_shader_context(Material* ssao_material)
{
    if(m_ssao_shader_context || !ssao_material) { // FIX-ME! -- potential bug if Material not set
        return m_ssao_shader_context;
    }
    m_ssao_shader_context = new ShaderContext(ssao_material,
                                              get_vbo_vert_coords(),
                                              get_vbo_vert_normal(),
                                              get_vbo_vert_tangent(),
                                              get_vbo_tex_coords(),
                                              get_ibo_tri_indices());
    return m_ssao_shader_context;
}

glm::vec3 Mesh::get_ambient_color() const
{
    return glm::vec3(m_ambient_color[0],
                     m_ambient_color[1],
                     m_ambient_color[2]);
}

void Mesh::set_ambient_color(glm::vec3 ambient_color)
{
    m_ambient_color[0] = ambient_color.r;
    m_ambient_color[1] = ambient_color.g;
    m_ambient_color[2] = ambient_color.b;
}

void Mesh::transform_vertices(glm::mat4 transform)
{
    glm::mat4 normal_transform = glm::transpose(glm::inverse(transform));
    for(int i = 0; i < static_cast<int>(m_num_vertex); i++) {
        set_vert_coord(i,   glm::vec3(transform        * glm::vec4(get_vert_coord(i),   1)));
        set_vert_normal(i,  glm::vec3(normal_transform * glm::vec4(get_vert_normal(i),  1)));
        set_vert_tangent(i, glm::vec3(normal_transform * glm::vec4(get_vert_tangent(i), 1)));
    }
    update_bbox();
}

void Mesh::flatten(glm::mat4* basis)
{
    if(basis) {
        transform_vertices((*basis) * get_transform());
    } else {
        transform_vertices(get_transform());
    }
    reset_transform();
}

void Mesh::set_axis(glm::vec3 axis)
{
    glm::vec3 local_axis = glm::vec3(glm::inverse(get_transform()) * glm::vec4(axis, 1));
    transform_vertices(glm::translate(glm::mat4(1), -local_axis));
    m_origin = in_parent_system(axis);
    mark_dirty_transform();
}

void Mesh::center_axis(BBoxObject::align_t align)
{
    update_bbox();
    set_axis(glm::vec3(get_transform() * glm::vec4(get_center(align), 1)));
}

void Mesh::update_transform()
{
    m_transform = glm::translate(glm::mat4(1), m_origin) * get_local_rotation_transform() * glm::scale(glm::mat4(1), m_scale);
}

MeshBase* alloc_mesh_base(const std::string& name, size_t num_vertex, size_t num_tri)
{
    return new Mesh(name, num_vertex, num_tri);
}

Mesh* cast_mesh(MeshBase* mesh)
{
    return dynamic_cast<Mesh*>(mesh);
}

MeshBase* cast_mesh_base(Mesh* mesh)
{
    return dynamic_cast<MeshBase*>(mesh);
}

}
