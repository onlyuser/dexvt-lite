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

#include <ShaderContext.h>
#include <Buffer.h>
#include <Material.h>
#include <Program.h>
#include <Texture.h>
#include <VarAttribute.h>
#include <VarUniform.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assert.h>

#define NUM_LIGHTS        8
//#define BLOOM_KERNEL_SIZE 5
#define BLOOM_KERNEL_SIZE 7

namespace vt {

ShaderContext::ShaderContext(Material* material,
                             Buffer*   vbo_vert_coords,
                             Buffer*   vbo_vert_normal,
                             Buffer*   vbo_vert_tangent,
                             Buffer*   vbo_tex_coords,
                             Buffer*   ibo_tri_indices)
    : m_material(material),
      m_vbo_vert_coords(vbo_vert_coords),
      m_vbo_vert_normal(vbo_vert_normal),
      m_vbo_vert_tangent(vbo_vert_tangent),
      m_vbo_tex_coords(vbo_tex_coords),
      m_ibo_tri_indices(ibo_tri_indices),
      m_textures(material->get_textures())
{
    Program* program = material->get_program();
    m_var_attributes.resize(Program::var_attribute_type_count);
    for(int i = 0; i < Program::var_attribute_type_count; i++) {
        if(!program->has_var(Program::VAR_TYPE_ATTRIBUTE, Program::get_var_attribute_name(i))) {
            m_var_attributes[i] = NULL;
            continue;
        }
        m_var_attributes[i] = program->get_var_attribute(Program::get_var_attribute_name(i).c_str());
    }
    m_var_uniforms.resize(Program::Program::var_uniform_type_count);
    for(int j = 0; j < Program::Program::var_uniform_type_count; j++) {
        if(!program->has_var(Program::VAR_TYPE_UNIFORM, Program::get_var_uniform_name(j))) {
            m_var_uniforms[j] = NULL;
            continue;
        }
        m_var_uniforms[j] = program->get_var_uniform(Program::get_var_uniform_name(j).c_str());
    }
}

ShaderContext::~ShaderContext()
{
    for(int i = 0; i < Program::var_attribute_type_count; i++) {
        if(!m_var_attributes[i]) {
            continue;
        }
        delete m_var_attributes[i];
    }
    for(int j = 0; j < Program::Program::var_uniform_type_count; j++) {
        if(!m_var_uniforms[j]) {
            continue;
        }
        delete m_var_uniforms[j];
    }
}

void ShaderContext::render()
{
    m_material->get_program()->use();
    int i = 0;
    for(ShaderContext::textures_t::const_iterator p = m_textures.begin(); p != m_textures.end(); ++p) {
        glActiveTexture(GL_TEXTURE0 + i);
        (*p)->bind();
        i++;
    }
    if(m_material->use_overlay()) {
        glDisable(GL_DEPTH_TEST);
        glBegin(GL_QUADS);
            glVertex3f(-1.0, -1.0, 0.0);
            glVertex3f( 1.0, -1.0, 0.0);
            glVertex3f( 1.0,  1.0, 0.0);
            glVertex3f(-1.0,  1.0, 0.0);
        glEnd();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    m_var_attributes[Program::var_attribute_type_vertex_position]->enable_vertex_attrib_array();
    m_var_attributes[Program::var_attribute_type_vertex_position]->vertex_attrib_pointer(m_vbo_vert_coords,
                                                                                         3,        // number of elements per vertex, here (x, y, z)
                                                                                         GL_FLOAT, // the type of each element
                                                                                         GL_FALSE, // take our values as-is
                                                                                         0,        // no extra data between each position
                                                                                         0);       // offset of first element
    if(m_material->get_program()->has_var(Program::VAR_TYPE_ATTRIBUTE, Program::var_attribute_type_vertex_normal)) {
        m_var_attributes[Program::var_attribute_type_vertex_normal]->enable_vertex_attrib_array();
        m_var_attributes[Program::var_attribute_type_vertex_normal]->vertex_attrib_pointer(m_vbo_vert_normal,
                                                                                           3,        // number of elements per vertex, here (x, y, z)
                                                                                           GL_FLOAT, // the type of each element
                                                                                           GL_FALSE, // take our values as-is
                                                                                           0,        // no extra data between each position
                                                                                           0);       // offset of first element
    }
    if(m_material->get_program()->has_var(Program::VAR_TYPE_ATTRIBUTE, Program::var_attribute_type_vertex_tangent)) {
        m_var_attributes[Program::var_attribute_type_vertex_tangent]->enable_vertex_attrib_array();
        m_var_attributes[Program::var_attribute_type_vertex_tangent]->vertex_attrib_pointer(m_vbo_vert_tangent,
                                                                                            3,        // number of elements per vertex, here (x, y, z)
                                                                                            GL_FLOAT, // the type of each element
                                                                                            GL_FALSE, // take our values as-is
                                                                                            0,        // no extra data between each position
                                                                                            0);       // offset of first element
    }
    if(m_material->get_program()->has_var(Program::VAR_TYPE_ATTRIBUTE, Program::var_attribute_type_texcoord)) {
        m_var_attributes[Program::var_attribute_type_texcoord]->enable_vertex_attrib_array();
        m_var_attributes[Program::var_attribute_type_texcoord]->vertex_attrib_pointer(m_vbo_tex_coords,
                                                                                      2,        // number of elements per vertex, here (x, y)
                                                                                      GL_FLOAT, // the type of each element
                                                                                      GL_FALSE, // take our values as-is
                                                                                      0,        // no extra data between each position
                                                                                      0);       // offset of first element
    }
    if(m_ibo_tri_indices) {
        m_ibo_tri_indices->bind();
        glDrawElements(GL_TRIANGLES, m_ibo_tri_indices->size()/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
    }
    for(int i = 0; i < Program::var_attribute_type_count; i++) {
        if(m_var_attributes[i] && m_var_attributes[i]->is_enabled()) {
            m_var_attributes[i]->disable_vertex_attrib_array();
        }
    }
}

void ShaderContext::set_ambient_color(const float* ambient_color)
{
    m_var_uniforms[Program::var_uniform_type_ambient_color]->uniform_3fv(1, ambient_color);
}

void ShaderContext::set_backface_depth_overlay_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_backface_depth_overlay_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_backface_normal_overlay_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_backface_normal_overlay_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_bloom_kernel(const float* bloom_kernel_arr)
{
    m_var_uniforms[Program::var_uniform_type_bloom_kernel]->uniform_1fv(BLOOM_KERNEL_SIZE, bloom_kernel_arr);
}

void ShaderContext::set_bump_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_bump_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_camera_dir(const float* camera_dir_arr)
{
    m_var_uniforms[Program::var_uniform_type_camera_dir]->uniform_3fv(1, camera_dir_arr);
}

void ShaderContext::set_camera_far(GLfloat camera_far)
{
    m_var_uniforms[Program::var_uniform_type_camera_far]->uniform_1f(camera_far);
}

void ShaderContext::set_camera_near(GLfloat camera_near)
{
    m_var_uniforms[Program::var_uniform_type_camera_near]->uniform_1f(camera_near);
}

void ShaderContext::set_camera_pos(const float* camera_pos_arr)
{
    m_var_uniforms[Program::var_uniform_type_camera_pos]->uniform_3fv(1, camera_pos_arr);
}

void ShaderContext::set_color_texture_index(GLint color_texture_id)
{
    assert(color_texture_id >= 0 && color_texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_color_texture]->uniform_1i(color_texture_id);
}

void ShaderContext::set_color_texture2_index(GLint color_texture_id)
{
    assert(color_texture_id >= 0 && color_texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_color_texture2]->uniform_1i(color_texture_id);
}

void ShaderContext::set_color_texture_source(GLint color_texture_source)
{
    m_var_uniforms[Program::var_uniform_type_color_texture_source]->uniform_1i(color_texture_source);
}

void ShaderContext::set_env_map_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_env_map_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_frontface_depth_overlay_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_frontface_depth_overlay_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_glow_cutoff_threshold(GLfloat glow_cutoff_threshold)
{
    m_var_uniforms[Program::var_uniform_type_glow_cutoff_threshold]->uniform_1f(glow_cutoff_threshold);
}

void ShaderContext::set_image_res(const GLint* image_res_arr)
{
    m_var_uniforms[Program::var_uniform_type_image_res]->uniform_2iv(1, image_res_arr);
}

void ShaderContext::set_inv_normal_transform(glm::mat4 inv_normal_transform)
{
    m_var_uniforms[Program::var_uniform_type_inv_normal_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(inv_normal_transform));
}

void ShaderContext::set_inv_projection_transform(glm::mat4 inv_projection_transform)
{
    m_var_uniforms[Program::var_uniform_type_inv_projection_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(inv_projection_transform));
}

void ShaderContext::set_inv_view_proj_transform(glm::mat4 inv_view_proj_transform)
{
    m_var_uniforms[Program::var_uniform_type_inv_view_proj_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(inv_view_proj_transform));
}

void ShaderContext::set_light_color(size_t num_lights, const float* light_color_arr)
{
    m_var_uniforms[Program::var_uniform_type_light_color]->uniform_3fv(num_lights, light_color_arr);
}

void ShaderContext::set_light_count(GLint light_count)
{
    m_var_uniforms[Program::var_uniform_type_light_count]->uniform_1i(light_count);
}

void ShaderContext::set_light_enabled(size_t num_lights, GLint* light_enabled_arr)
{
    m_var_uniforms[Program::var_uniform_type_light_enabled]->uniform_1iv(num_lights, light_enabled_arr);
}

void ShaderContext::set_light_pos(size_t num_lights, const float* light_pos_arr)
{
    m_var_uniforms[Program::var_uniform_type_light_pos]->uniform_3fv(num_lights, light_pos_arr);
}

void ShaderContext::set_model_transform(glm::mat4 model_transform)
{
    m_var_uniforms[Program::var_uniform_type_model_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(model_transform));
}

void ShaderContext::set_mvp_transform(glm::mat4 mvp_transform)
{
    m_var_uniforms[Program::var_uniform_type_mvp_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(mvp_transform));
}

void ShaderContext::set_normal_transform(glm::mat4 normal_transform)
{
    m_var_uniforms[Program::var_uniform_type_normal_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(normal_transform));
}

void ShaderContext::set_random_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[Program::var_uniform_type_random_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_ray_tracer_render_mode(GLint render_mode)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_render_mode]->uniform_1i(render_mode);
}

void ShaderContext::set_ray_tracer_bounce_count(GLint bounce_count)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_bounce_count]->uniform_1i(bounce_count);
}

void ShaderContext::set_ray_tracer_box_color(size_t num_boxes, glm::vec3* ray_tracer_box_color_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_color]->uniform_3fv(num_boxes, glm::value_ptr(*ray_tracer_box_color_arr));
}

void ShaderContext::set_ray_tracer_box_count(GLint box_count)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_count]->uniform_1i(box_count);
}

void ShaderContext::set_ray_tracer_box_diffuse_fuzz(size_t num_boxes, GLfloat* ray_tracer_box_diffuse_fuzz_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_diffuse_fuzz]->uniform_1fv(num_boxes, ray_tracer_box_diffuse_fuzz_arr);
}

void ShaderContext::set_ray_tracer_box_eta(size_t num_boxes, GLfloat* ray_tracer_box_eta_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_eta]->uniform_1fv(num_boxes, ray_tracer_box_eta_arr);
}

void ShaderContext::set_ray_tracer_box_inverse_transform(size_t num_boxes, glm::mat4* ray_tracer_box_inverse_transform_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_inverse_transform]->uniform_matrix_4fv(num_boxes, GL_FALSE, glm::value_ptr(*ray_tracer_box_inverse_transform_arr));
}

void ShaderContext::set_ray_tracer_box_luminosity(size_t num_boxes, GLfloat* ray_tracer_box_luminosity_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_luminosity]->uniform_1fv(num_boxes, ray_tracer_box_luminosity_arr);
}

void ShaderContext::set_ray_tracer_box_max(size_t num_boxes, glm::vec3* ray_tracer_box_max_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_max]->uniform_3fv(num_boxes, glm::value_ptr(*ray_tracer_box_max_arr));
}

void ShaderContext::set_ray_tracer_box_min(size_t num_boxes, glm::vec3* ray_tracer_box_min_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_min]->uniform_3fv(num_boxes, glm::value_ptr(*ray_tracer_box_min_arr));
}

void ShaderContext::set_ray_tracer_box_reflectance(size_t num_boxes, GLfloat* ray_tracer_box_reflectance)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_reflectance]->uniform_1fv(num_boxes, ray_tracer_box_reflectance);
}

void ShaderContext::set_ray_tracer_box_transform(size_t num_boxes, glm::mat4* ray_tracer_box_transform_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_transform]->uniform_matrix_4fv(num_boxes, GL_FALSE, glm::value_ptr(*ray_tracer_box_transform_arr));
}

void ShaderContext::set_ray_tracer_box_transparency(size_t num_boxes, GLfloat* ray_tracer_box_transparency)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_box_transparency]->uniform_1fv(num_boxes, ray_tracer_box_transparency);
}

void ShaderContext::set_ray_tracer_plane_color(size_t num_planes, glm::vec3* ray_tracer_plane_color_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_color]->uniform_3fv(num_planes, glm::value_ptr(*ray_tracer_plane_color_arr));
}

void ShaderContext::set_ray_tracer_plane_count(GLint plane_count)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_count]->uniform_1i(plane_count);
}

void ShaderContext::set_ray_tracer_plane_diffuse_fuzz(size_t num_planes, GLfloat* ray_tracer_plane_diffuse_fuzz_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_diffuse_fuzz]->uniform_1fv(num_planes, ray_tracer_plane_diffuse_fuzz_arr);
}

void ShaderContext::set_ray_tracer_plane_eta(size_t num_planes, GLfloat* ray_tracer_plane_eta_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_eta]->uniform_1fv(num_planes, ray_tracer_plane_eta_arr);
}

void ShaderContext::set_ray_tracer_plane_luminosity(size_t num_planes, GLfloat* ray_tracer_plane_luminosity_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_luminosity]->uniform_1fv(num_planes, ray_tracer_plane_luminosity_arr);
}

void ShaderContext::set_ray_tracer_plane_normal(size_t num_planes, glm::vec3* ray_tracer_plane_normal_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_normal]->uniform_3fv(num_planes, glm::value_ptr(*ray_tracer_plane_normal_arr));
}

void ShaderContext::set_ray_tracer_plane_point(size_t num_planes, glm::vec3* ray_tracer_plane_point_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_point]->uniform_3fv(num_planes, glm::value_ptr(*ray_tracer_plane_point_arr));
}

void ShaderContext::set_ray_tracer_plane_reflectance(size_t num_planes, GLfloat* ray_tracer_plane_reflectance_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_reflectance]->uniform_1fv(num_planes, ray_tracer_plane_reflectance_arr);
}

void ShaderContext::set_ray_tracer_plane_transparency(size_t num_planes, GLfloat* ray_tracer_plane_transparency_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_plane_transparency]->uniform_1fv(num_planes, ray_tracer_plane_transparency_arr);
}

void ShaderContext::set_ray_tracer_random_point_count(size_t num_random_point_count)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_random_point_count]->uniform_1i(num_random_point_count);
}

void ShaderContext::set_ray_tracer_random_points(size_t num_random_points, glm::vec3* ray_tracer_random_points_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_random_points]->uniform_3fv(num_random_points, glm::value_ptr(*ray_tracer_random_points_arr));
}

void ShaderContext::set_ray_tracer_random_seed(GLfloat random_seed)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_random_seed]->uniform_1f(random_seed);
}

void ShaderContext::set_ray_tracer_sphere_color(size_t num_spheres, glm::vec3* ray_tracer_sphere_color_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_color]->uniform_3fv(num_spheres, glm::value_ptr(*ray_tracer_sphere_color_arr));
}

void ShaderContext::set_ray_tracer_sphere_count(GLint sphere_count)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_count]->uniform_1i(sphere_count);
}

void ShaderContext::set_ray_tracer_sphere_diffuse_fuzz(size_t num_spheres, GLfloat* ray_tracer_sphere_diffuse_fuzz_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_diffuse_fuzz]->uniform_1fv(num_spheres, ray_tracer_sphere_diffuse_fuzz_arr);
}

void ShaderContext::set_ray_tracer_sphere_eta(size_t num_spheres, GLfloat* ray_tracer_sphere_eta_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_eta]->uniform_1fv(num_spheres, ray_tracer_sphere_eta_arr);
}

void ShaderContext::set_ray_tracer_sphere_luminosity(size_t num_spheres, GLfloat* ray_tracer_sphere_luminosity_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_luminosity]->uniform_1fv(num_spheres, ray_tracer_sphere_luminosity_arr);
}

void ShaderContext::set_ray_tracer_sphere_origin(size_t num_spheres, glm::vec3* ray_tracer_sphere_origin_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_origin]->uniform_3fv(num_spheres, glm::value_ptr(*ray_tracer_sphere_origin_arr));
}

void ShaderContext::set_ray_tracer_sphere_radius(size_t num_spheres, GLfloat* ray_tracer_sphere_radius_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_radius]->uniform_1fv(num_spheres, ray_tracer_sphere_radius_arr);
}

void ShaderContext::set_ray_tracer_sphere_reflectance(size_t num_spheres, GLfloat* ray_tracer_sphere_reflectance_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_reflectance]->uniform_1fv(num_spheres, ray_tracer_sphere_reflectance_arr);
}

void ShaderContext::set_ray_tracer_sphere_transparency(size_t num_spheres, GLfloat* ray_tracer_sphere_transparency_arr)
{
    m_var_uniforms[Program::var_uniform_type_ray_tracer_sphere_transparency]->uniform_1fv(num_spheres, ray_tracer_sphere_transparency_arr);
}

void ShaderContext::set_reflect_to_refract_ratio(GLfloat reflect_to_refract_ratio)
{
    m_var_uniforms[Program::var_uniform_type_reflect_to_refract_ratio]->uniform_1f(reflect_to_refract_ratio);
}

void ShaderContext::set_ssao_sample_kernel_pos(size_t num_kernels, const float* kernel_pos_arr)
{
    m_var_uniforms[Program::var_uniform_type_ssao_sample_kernel_pos]->uniform_3fv(num_kernels, kernel_pos_arr);
}

void ShaderContext::set_view_proj_transform(glm::mat4 view_proj_transform)
{
    m_var_uniforms[Program::var_uniform_type_view_proj_transform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(view_proj_transform));
}

void ShaderContext::set_viewport_dim(const GLint* viewport_dim_arr)
{
    m_var_uniforms[Program::var_uniform_type_viewport_dim]->uniform_2iv(1, viewport_dim_arr);
}

}
