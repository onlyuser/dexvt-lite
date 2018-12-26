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

#ifndef VT_PROGRAM_H_
#define VT_PROGRAM_H_

#include <IdentObject.h>
#include <NamedObject.h>
#include <GL/glew.h>
#include <set>
#include <string>

namespace vt {

class Shader;
class VarAttribute;
class VarUniform;

class Program : public IdentObject, public NamedObject
{
public:
    enum var_type_t {
        VAR_TYPE_ATTRIBUTE,
        VAR_TYPE_UNIFORM
    };

    enum var_attribute_type_t {
        var_attribute_type_texcoord,
        var_attribute_type_vertex_normal,
        var_attribute_type_vertex_position,
        var_attribute_type_vertex_tangent,
        var_attribute_type_count
    };

    enum var_uniform_type_t {
        var_uniform_type_ambient_color,
        var_uniform_type_backface_depth_overlay_texture,
        var_uniform_type_backface_normal_overlay_texture,
        var_uniform_type_bloom_kernel,
        var_uniform_type_bump_texture,
        var_uniform_type_camera_dir,
        var_uniform_type_camera_far,
        var_uniform_type_camera_near,
        var_uniform_type_camera_pos,
        var_uniform_type_color_texture,
        var_uniform_type_color_texture2,
        var_uniform_type_color_texture_source,
        var_uniform_type_env_map_texture,
        var_uniform_type_frontface_depth_overlay_texture,
        var_uniform_type_glow_cutoff_threshold,
        var_uniform_type_image_res,
        var_uniform_type_inv_normal_transform,
        var_uniform_type_inv_projection_transform,
        var_uniform_type_inv_view_proj_transform,
        var_uniform_type_light_color,
        var_uniform_type_light_count,
        var_uniform_type_light_enabled,
        var_uniform_type_light_pos,
        var_uniform_type_model_transform,
        var_uniform_type_mvp_transform,
        var_uniform_type_normal_transform,
        var_uniform_type_random_texture,
        var_uniform_type_ray_tracer_render_mode,
        var_uniform_type_ray_tracer_bounce_count,
        var_uniform_type_ray_tracer_box_color,
        var_uniform_type_ray_tracer_box_count,
        var_uniform_type_ray_tracer_box_diffuse_fuzz,
        var_uniform_type_ray_tracer_box_eta,
        var_uniform_type_ray_tracer_box_inverse_transform,
        var_uniform_type_ray_tracer_box_luminosity,
        var_uniform_type_ray_tracer_box_max,
        var_uniform_type_ray_tracer_box_min,
        var_uniform_type_ray_tracer_box_reflectance,
        var_uniform_type_ray_tracer_box_transform,
        var_uniform_type_ray_tracer_box_transparency,
        var_uniform_type_ray_tracer_plane_color,
        var_uniform_type_ray_tracer_plane_count,
        var_uniform_type_ray_tracer_plane_diffuse_fuzz,
        var_uniform_type_ray_tracer_plane_eta,
        var_uniform_type_ray_tracer_plane_luminosity,
        var_uniform_type_ray_tracer_plane_normal,
        var_uniform_type_ray_tracer_plane_point,
        var_uniform_type_ray_tracer_plane_reflectance,
        var_uniform_type_ray_tracer_plane_transparency,
        var_uniform_type_ray_tracer_random_point_count,
        var_uniform_type_ray_tracer_random_points,
        var_uniform_type_ray_tracer_random_seed,
        var_uniform_type_ray_tracer_sphere_color,
        var_uniform_type_ray_tracer_sphere_count,
        var_uniform_type_ray_tracer_sphere_diffuse_fuzz,
        var_uniform_type_ray_tracer_sphere_eta,
        var_uniform_type_ray_tracer_sphere_luminosity,
        var_uniform_type_ray_tracer_sphere_origin,
        var_uniform_type_ray_tracer_sphere_radius,
        var_uniform_type_ray_tracer_sphere_reflectance,
        var_uniform_type_ray_tracer_sphere_transparency,
        var_uniform_type_reflect_to_refract_ratio,
        var_uniform_type_ssao_sample_kernel_pos,
        var_uniform_type_view_proj_transform,
        var_uniform_type_viewport_dim,
        var_uniform_type_count
    };

    explicit Program(const std::string& name);
    virtual ~Program();
    void attach_shader(Shader* shader);
    Shader* get_vertex_shader() const
    {
        return m_vertex_shader;
    }
    Shader* get_fragment_shader() const
    {
        return m_fragment_shader;
    }
    bool auto_add_shader_vars();
    bool link();
    void use() const;
    VarAttribute* get_var_attribute(const GLchar* name) const;
    VarUniform* get_var_uniform(const GLchar* name) const;
    void get_program_iv(
            GLenum pname,
            GLint* params) const;
    static std::string get_var_attribute_name(int id);
    static std::string get_var_uniform_name(int id);
    bool check_var_exists_in_shader(var_type_t var_type, const std::string& name) const;
    bool add_var(var_type_t var_type, std::string name);
    bool has_var(var_type_t var_type, std::string name) const;
    bool has_var(var_type_t var_type, int id) const;
    void clear_vars();

private:
    Shader* m_vertex_shader;
    Shader* m_fragment_shader;

    // attributes
    typedef std::pair<var_attribute_type_t, const char*> var_attribute_type_to_name_table_t;
    static var_attribute_type_to_name_table_t m_var_attribute_type_to_name_table[];
    typedef std::set<std::string> var_attribute_names_t;
    var_attribute_names_t m_var_attribute_names;
    bool m_var_attribute_ids[var_attribute_type_count];

    // uniforms
    typedef std::pair<var_uniform_type_t, const char*> var_uniform_type_to_name_table_t;
    static var_uniform_type_to_name_table_t m_var_uniform_type_to_name_table[];
    typedef std::set<std::string> var_uniform_names_t;
    var_uniform_names_t m_var_uniform_names;
    bool m_var_uniform_ids[var_uniform_type_count];
};

}

#endif
