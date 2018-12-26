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

#ifndef VT_SHADER_CONTEXT_H_
#define VT_SHADER_CONTEXT_H_

#include <VarAttribute.h>
#include <VarUniform.h>
#include <glm/glm.hpp>
#include <vector>

namespace vt {

class Buffer;
class Material;
class Program;
class Texture;

class ShaderContext
{
public:
    typedef std::vector<Texture*> textures_t;

    ShaderContext(Material* material,
                  Buffer*   vbo_vert_coords,
                  Buffer*   vbo_vert_normal,
                  Buffer*   vbo_vert_tangent,
                  Buffer*   vbo_tex_coords,
                  Buffer*   ibo_tri_indices);
    ~ShaderContext();
    Material* get_material() const
    {
        return m_material;
    }
    void render();
    void set_ambient_color(const float* ambient_color);
    void set_backface_depth_overlay_texture_index(GLint texture_id);
    void set_backface_normal_overlay_texture_index(GLint texture_id);
    void set_bloom_kernel(const float* bloom_kernel_arr);
    void set_bump_texture_index(GLint texture_id);
    void set_camera_dir(const float* camera_dir_arr);
    void set_camera_far(GLfloat camera_far);
    void set_camera_near(GLfloat camera_near);
    void set_camera_pos(const float* camera_pos_arr);
    void set_color_texture_index(GLint color_texture_id);
    void set_color_texture2_index(GLint color_texture_id);
    void set_color_texture_source(GLint color_texture_source);
    void set_env_map_texture_index(GLint texture_id);
    void set_frontface_depth_overlay_texture_index(GLint texture_id);
    void set_glow_cutoff_threshold(GLfloat glow_cutoff_threshold);
    void set_image_res(const GLint* image_res_arr);
    void set_inv_normal_transform(glm::mat4 inv_normal_transform);
    void set_inv_projection_transform(glm::mat4 inv_projection_transform);
    void set_inv_view_proj_transform(glm::mat4 inv_view_proj_transform);
    void set_light_color(size_t num_lights, const float* light_color_arr);
    void set_light_count(GLint light_count);
    void set_light_enabled(size_t num_lights, GLint* light_enabled_arr);
    void set_light_pos(size_t num_lights, const float* light_pos_arr);
    void set_model_transform(glm::mat4 model_transform);
    void set_mvp_transform(glm::mat4 mvp_transform);
    void set_normal_transform(glm::mat4 normal_transform);
    void set_random_texture_index(GLint texture_id);
    void set_ray_tracer_render_mode(GLint render_mode);
    void set_ray_tracer_bounce_count(GLint bounce_count);
    void set_ray_tracer_box_color(size_t num_boxes, glm::vec3* ray_tracer_box_color_arr);
    void set_ray_tracer_box_count(GLint box_count);
    void set_ray_tracer_box_diffuse_fuzz(size_t num_boxes, GLfloat* ray_tracer_box_diffuse_fuzz_arr);
    void set_ray_tracer_box_eta(size_t num_boxes, GLfloat* ray_tracer_box_eta_arr);
    void set_ray_tracer_box_luminosity(size_t num_boxes, GLfloat* ray_tracer_box_luminosity_arr);
    void set_ray_tracer_box_inverse_transform(size_t num_boxes, glm::mat4* ray_tracer_box_inverse_transform_arr);
    void set_ray_tracer_box_max(size_t num_boxes, glm::vec3* ray_tracer_box_max_arr);
    void set_ray_tracer_box_min(size_t num_boxes, glm::vec3* ray_tracer_box_min_arr);
    void set_ray_tracer_box_reflectance(size_t num_boxes, GLfloat* ray_tracer_box_reflectance_arr);
    void set_ray_tracer_box_transform(size_t num_boxes, glm::mat4* ray_tracer_box_transform_arr);
    void set_ray_tracer_box_transparency(size_t num_boxes, GLfloat* ray_tracer_box_transparency_arr);
    void set_ray_tracer_plane_color(size_t num_planes, glm::vec3* ray_tracer_plane_color_arr);
    void set_ray_tracer_plane_count(GLint plane_count);
    void set_ray_tracer_plane_diffuse_fuzz(size_t num_planes, GLfloat* ray_tracer_plane_diffuse_fuzz_arr);
    void set_ray_tracer_plane_eta(size_t num_planes, GLfloat* ray_tracer_plane_eta_arr);
    void set_ray_tracer_plane_luminosity(size_t num_planes, GLfloat* ray_tracer_plane_luminosity_arr);
    void set_ray_tracer_plane_normal(size_t num_planes, glm::vec3* ray_tracer_plane_normal_arr);
    void set_ray_tracer_plane_point(size_t num_planes, glm::vec3* ray_tracer_plane_point_arr);
    void set_ray_tracer_plane_reflectance(size_t num_planes, GLfloat* ray_tracer_plane_reflectance_arr);
    void set_ray_tracer_plane_transparency(size_t num_planes, GLfloat* ray_tracer_plane_transparency_arr);
    void set_ray_tracer_random_point_count(size_t num_random_point_count);
    void set_ray_tracer_random_points(size_t num_random_points, glm::vec3* ray_tracer_random_points_arr);
    void set_ray_tracer_random_seed(GLfloat random_seed);
    void set_ray_tracer_sphere_color(size_t num_spheres, glm::vec3* ray_tracer_sphere_color_arr);
    void set_ray_tracer_sphere_count(GLint sphere_count);
    void set_ray_tracer_sphere_diffuse_fuzz(size_t num_spheres, GLfloat* ray_tracer_sphere_diffuse_fuzz_arr);
    void set_ray_tracer_sphere_eta(size_t num_spheres, GLfloat* ray_tracer_sphere_eta_arr);
    void set_ray_tracer_sphere_luminosity(size_t num_spheres, GLfloat* ray_tracer_sphere_luminosity_arr);
    void set_ray_tracer_sphere_origin(size_t num_spheres, glm::vec3* ray_tracer_sphere_origin_arr);
    void set_ray_tracer_sphere_radius(size_t num_spheres, GLfloat* ray_tracer_sphere_radius_arr);
    void set_ray_tracer_sphere_reflectance(size_t num_spheres, GLfloat* ray_tracer_sphere_reflectance_arr);
    void set_ray_tracer_sphere_transparency(size_t num_spheres, GLfloat* ray_tracer_sphere_transparency_arr);
    void set_reflect_to_refract_ratio(GLfloat reflect_to_refract_ratio);
    void set_ssao_sample_kernel_pos(size_t num_kernels, const float* kernel_pos_arr);
    void set_view_proj_transform(glm::mat4 view_proj_transform);
    void set_viewport_dim(const GLint* viewport_dim_arr);

private:
    Material *m_material;
    Buffer *m_vbo_vert_coords, *m_vbo_vert_normal, *m_vbo_vert_tangent, *m_vbo_tex_coords, *m_ibo_tri_indices;
    std::vector<VarAttribute*> m_var_attributes;
    std::vector<VarUniform*> m_var_uniforms;
    const textures_t &m_textures;
};

}

#endif
