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

#ifndef VT_MESH_H_
#define VT_MESH_H_

#include <ShaderContext.h>
#include <Buffer.h>
#include <TransformObject.h>
#include <BBoxObject.h>
#include <MeshBase.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <stddef.h>
#include <memory> // std::unique_ptr

namespace vt {

class Material;

class Mesh : public TransformObject,
             public BBoxObject,
             public MeshBase
{
public:
    Mesh(std::string name,
         size_t      num_vertex,
         size_t      num_tri);
    virtual ~Mesh();
    void resize(size_t num_vertex, size_t num_tri, bool preserve_mesh_geometry = false);
    void merge(const MeshBase* other, bool copy_tex_coords = false);

    size_t get_num_vertex() const
    {
        return m_num_vertex;
    }
    size_t get_num_tri() const
    {
        return m_num_tri;
    }

    bool is_visible() const
    {
        return m_visible;
    }
    void set_visible(bool visible)
    {
        m_visible = visible;
    }

    bool is_smooth() const
    {
        return m_smooth;
    }
    void set_smooth(bool smooth)
    {
        m_smooth = smooth;
    }

    glm::vec3  get_vert_coord(int index) const;
    void       set_vert_coord(int index, glm::vec3 coord);
    glm::vec3  get_vert_normal(int index) const;
    void       set_vert_normal(int index, glm::vec3 normal);
    glm::vec3  get_vert_tangent(int index) const;
    void       set_vert_tangent(int index, glm::vec3 tangent);
    glm::vec2  get_tex_coord(int index) const;
    void       set_tex_coord(int index, glm::vec2 coord);
    glm::ivec3 get_tri_indices(int index) const;
    void       set_tri_indices(int index, glm::ivec3 indices);

    void update_bbox();
    void update_normals_and_tangents();

    // NOTE: strangely required by pure virtual (already defined in base class!)
    void get_min_max(glm::vec3* min, glm::vec3* max) const;

    // NOTE: strangely required by pure virtual (already defined in base class!)
    glm::vec3 in_abs_system(glm::vec3 local_point = glm::vec3(0));

    void init_buffers();
    void update_buffers() const;
    Buffer* get_vbo_vert_coords();
    Buffer* get_vbo_vert_normal();
    Buffer* get_vbo_vert_tangent();
    Buffer* get_vbo_tex_coords();
    Buffer* get_ibo_tri_indices();

    void set_material(Material* material);
    Material* get_material() const
    {
        return m_material;
    }

    ShaderContext* get_shader_context();
    ShaderContext* get_normal_shader_context(Material* normal_material);
    ShaderContext* get_wireframe_shader_context(Material* wireframe_material);
    ShaderContext* get_ssao_shader_context(Material* ssao_material);

    int get_texture_index() const
    {
        return m_texture_index;
    }
    void set_texture_index(int texture_index)
    {
        m_texture_index = texture_index;
    }

    int get_texture2_index() const
    {
        return m_texture2_index;
    }
    void set_texture2_index(int texture2_index)
    {
        m_texture2_index = texture2_index;
    }

    int get_bump_texture_index() const
    {
        return m_bump_texture_index;
    }
    void set_bump_texture_index(int bump_texture_index)
    {
        m_bump_texture_index = bump_texture_index;
    }

    int get_env_map_texture_index() const
    {
        return m_env_map_texture_index;
    }
    void set_env_map_texture_index(int env_map_texture_index)
    {
        m_env_map_texture_index = env_map_texture_index;
    }

    int get_random_texture_index() const
    {
        return m_random_texture_index;
    }
    void set_random_texture_index(int random_texture_index)
    {
        m_random_texture_index = random_texture_index;
    }

    int get_frontface_depth_overlay_texture_index() const
    {
        return m_frontface_depth_overlay_texture_index;
    }
    void set_frontface_depth_overlay_texture_index(int frontface_depth_overlay_texture_index)
    {
        m_frontface_depth_overlay_texture_index = frontface_depth_overlay_texture_index;
    }

    int get_backface_depth_overlay_texture_index() const
    {
        return m_backface_depth_overlay_texture_index;
    }
    void set_backface_depth_overlay_texture_index(int backface_depth_overlay_texture_index)
    {
        m_backface_depth_overlay_texture_index = backface_depth_overlay_texture_index;
    }

    int get_backface_normal_overlay_texture_index() const
    {
        return m_backface_normal_overlay_texture_index;
    }
    void set_backface_normal_overlay_texture_index(int backface_normal_overlay_texture_index)
    {
        m_backface_normal_overlay_texture_index = backface_normal_overlay_texture_index;
    }

    float get_reflect_to_refract_ratio() const
    {
        return m_reflect_to_refract_ratio;
    }
    void set_reflect_to_refract_ratio(float reflect_to_refract_ratio)
    {
        m_reflect_to_refract_ratio = reflect_to_refract_ratio;
    }

    glm::vec3 get_ambient_color() const;
    void set_ambient_color(glm::vec3 ambient_color);

    void transform_vertices(glm::mat4 transform);
    void flatten(glm::mat4* basis = NULL);
    void set_axis(glm::vec3 axis);
    void center_axis(BBoxObject::align_t align = BBoxObject::ALIGN_CENTER);

private:
    std::string    m_name;
    size_t         m_num_vertex;
    size_t         m_num_tri;
    bool           m_visible;
    bool           m_smooth;
    GLfloat*       m_vert_coords;
    GLfloat*       m_vert_normal;
    GLfloat*       m_vert_tangent;
    GLfloat*       m_tex_coords;
    GLushort*      m_tri_indices;
    Buffer*        m_vbo_vert_coords;
    Buffer*        m_vbo_vert_normal;
    Buffer*        m_vbo_vert_tangent;
    Buffer*        m_vbo_tex_coords;
    Buffer*        m_ibo_tri_indices;
    bool           m_buffers_already_init;
    Material*      m_material;                 // TODO: Mesh has one Material
    ShaderContext* m_shader_context;           // TODO: Mesh has one ShaderContext
    ShaderContext* m_normal_shader_context;    // TODO: Mesh has one normal ShaderContext
    ShaderContext* m_wireframe_shader_context; // TODO: Mesh has one wireframe ShaderContext
    ShaderContext* m_ssao_shader_context;      // TODO: Mesh has one ssao ShaderContext
    int            m_texture_index;
    int            m_texture2_index;
    int            m_bump_texture_index;
    int            m_env_map_texture_index;
    int            m_random_texture_index;
    int            m_frontface_depth_overlay_texture_index;
    int            m_backface_depth_overlay_texture_index;
    int            m_backface_normal_overlay_texture_index;
    float          m_reflect_to_refract_ratio;
    GLfloat*       m_ambient_color;

    void update_transform();
};

MeshBase* alloc_mesh_base(std::string name, size_t num_vertex, size_t num_tri);
Mesh* cast_mesh(MeshBase* mesh);
MeshBase* cast_mesh_base(Mesh* mesh);

}

#endif
