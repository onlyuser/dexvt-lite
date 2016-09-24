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

ShaderContext::var_attribute_type_to_name_table_t ShaderContext::m_var_attribute_type_to_name_table[] = {
        {ShaderContext::var_attribute_type_texcoord,        "texcoord"},
        {ShaderContext::var_attribute_type_vertex_normal,   "vertex_normal"},
        {ShaderContext::var_attribute_type_vertex_position, "vertex_position"},
        {ShaderContext::var_attribute_type_vertex_tangent,  "vertex_tangent"},
        {ShaderContext::var_attribute_type_count,           ""},
        };

ShaderContext::var_uniform_type_to_name_table_t ShaderContext::m_var_uniform_type_to_name_table[] = {
        {ShaderContext::var_uniform_type_ambient_color,                   "ambient_color"},
        {ShaderContext::var_uniform_type_backface_depth_overlay_texture,  "backface_depth_overlay_texture"},
        {ShaderContext::var_uniform_type_backface_normal_overlay_texture, "backface_normal_overlay_texture"},
        {ShaderContext::var_uniform_type_bloom_kernel,                    "bloom_kernel"},
        {ShaderContext::var_uniform_type_bump_texture,                    "bump_texture"},
        {ShaderContext::var_uniform_type_camera_dir,                      "camera_dir"},
        {ShaderContext::var_uniform_type_camera_far,                      "camera_far"},
        {ShaderContext::var_uniform_type_camera_near,                     "camera_near"},
        {ShaderContext::var_uniform_type_camera_pos,                      "camera_pos"},
        {ShaderContext::var_uniform_type_color_texture,                   "color_texture"},
        {ShaderContext::var_uniform_type_color_texture2,                  "color_texture2"},
        {ShaderContext::var_uniform_type_env_map_texture,                 "env_map_texture"},
        {ShaderContext::var_uniform_type_frontface_depth_overlay_texture, "frontface_depth_overlay_texture"},
        {ShaderContext::var_uniform_type_glow_cutoff_threshold,           "glow_cutoff_threshold"},
        {ShaderContext::var_uniform_type_inv_normal_xform,                "inv_normal_xform"},
        {ShaderContext::var_uniform_type_inv_projection_xform,            "inv_projection_xform"},
        {ShaderContext::var_uniform_type_inv_view_proj_xform,             "inv_view_proj_xform"},
        {ShaderContext::var_uniform_type_light_color,                     "light_color"},
        {ShaderContext::var_uniform_type_light_count,                     "light_count"},
        {ShaderContext::var_uniform_type_light_enabled,                   "light_enabled"},
        {ShaderContext::var_uniform_type_light_pos,                       "light_pos"},
        {ShaderContext::var_uniform_type_model_xform,                     "model_xform"},
        {ShaderContext::var_uniform_type_mvp_xform,                       "mvp_xform"},
        {ShaderContext::var_uniform_type_normal_xform,                    "normal_xform"},
        {ShaderContext::var_uniform_type_random_texture,                  "random_texture"},
        {ShaderContext::var_uniform_type_reflect_to_refract_ratio,        "reflect_to_refract_ratio"},
        {ShaderContext::var_uniform_type_ssao_sample_kernel_pos,          "ssao_sample_kernel_pos"},
        {ShaderContext::var_uniform_type_viewport_dim,                    "viewport_dim"},
        {ShaderContext::var_uniform_type_view_proj_xform,                 "view_proj_xform"},
        {ShaderContext::var_uniform_type_count,                           ""}
    };

ShaderContext::ShaderContext(
        Material* material,
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
      m_textures(material->get_textures()),
      m_use_ambient_color(material->use_ambient_color()),
      m_gen_normal_map(material->gen_normal_map()),
      m_use_phong_shading(material->use_phong_shading()),
      m_use_texture_mapping(material->use_texture_mapping()),
      m_use_bump_mapping(material->use_bump_mapping()),
      m_use_env_mapping(material->use_env_mapping()),
      m_use_env_mapping_dbl_refract(material->use_env_mapping_dbl_refract()),
      m_use_ssao(material->use_ssao()),
      m_use_bloom_kernel(material->use_bloom_kernel()),
      m_use_texture2(material->use_texture2()),
      m_use_fragment_world_pos(material->use_fragment_world_pos()),
      m_skybox(material->skybox()),
      m_overlay(material->overlay())
{
    Program* program = material->get_program();
    for(int i = 0; i < var_attribute_type_count; i++) {
        if(!program->has_var(m_var_attribute_type_to_name_table[i].second)) {
            m_var_attributes[i] = NULL;
            continue;
        }
        m_var_attributes[i] = program->get_var_attribute(m_var_attribute_type_to_name_table[i].second);
    }
    for(int j = 0; j < var_uniform_type_count; j++) {
        if(!program->has_var(m_var_uniform_type_to_name_table[j].second)) {
            m_var_uniforms[j] = NULL;
            continue;
        }
        m_var_uniforms[j] = program->get_var_uniform(m_var_uniform_type_to_name_table[j].second);
    }
}

ShaderContext::~ShaderContext()
{
    for(int i = 0; i < var_attribute_type_count; i++) {
        if(!m_var_attributes[i]) {
            continue;
        }
        delete m_var_attributes[i];
    }
    for(int j = 0; j < var_uniform_type_count; j++) {
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
    for(ShaderContext::textures_t::const_iterator p = m_textures.begin(); p != m_textures.end(); p++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        (*p)->bind();
        i++;
    }
    if(m_skybox || m_overlay) {
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
    m_var_attributes[var_attribute_type_vertex_position]->enable_vertex_attrib_array();
    m_var_attributes[var_attribute_type_vertex_position]->vertex_attrib_pointer(
            m_vbo_vert_coords,
            3,        // number of elements per vertex, here (x,y,z)
            GL_FLOAT, // the type of each element
            GL_FALSE, // take our values as-is
            0,        // no extra data between each position
            0);       // offset of first element
    if(m_gen_normal_map || m_use_phong_shading || m_use_bump_mapping || m_use_env_mapping || m_use_ssao) {
        m_var_attributes[var_attribute_type_vertex_normal]->enable_vertex_attrib_array();
        m_var_attributes[var_attribute_type_vertex_normal]->vertex_attrib_pointer(
                m_vbo_vert_normal,
                3,        // number of elements per vertex, here (x,y,z)
                GL_FLOAT, // the type of each element
                GL_FALSE, // take our values as-is
                0,        // no extra data between each position
                0);       // offset of first element
        if(m_use_bump_mapping) {
            m_var_attributes[var_attribute_type_vertex_tangent]->enable_vertex_attrib_array();
            m_var_attributes[var_attribute_type_vertex_tangent]->vertex_attrib_pointer(
                    m_vbo_vert_tangent,
                    3,        // number of elements per vertex, here (x,y,z)
                    GL_FLOAT, // the type of each element
                    GL_FALSE, // take our values as-is
                    0,        // no extra data between each position
                    0);       // offset of first element
        }
    }
    if(m_use_texture_mapping || m_use_bump_mapping || m_use_ssao) {
        m_var_attributes[var_attribute_type_texcoord]->enable_vertex_attrib_array();
        m_var_attributes[var_attribute_type_texcoord]->vertex_attrib_pointer(
                m_vbo_tex_coords,
                2,        // number of elements per vertex, here (x,y)
                GL_FLOAT, // the type of each element
                GL_FALSE, // take our values as-is
                0,        // no extra data between each position
                0);       // offset of first element
    }
    if(m_ibo_tri_indices) {
        m_ibo_tri_indices->bind();
        glDrawElements(GL_TRIANGLES, m_ibo_tri_indices->size()/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
    }
    m_var_attributes[var_attribute_type_vertex_position]->disable_vertex_attrib_array();
    if(m_gen_normal_map || m_use_phong_shading || m_use_bump_mapping || m_use_env_mapping || m_use_ssao) {
        m_var_attributes[var_attribute_type_vertex_normal]->disable_vertex_attrib_array();
        if(m_use_bump_mapping) {
            m_var_attributes[var_attribute_type_vertex_tangent]->disable_vertex_attrib_array();
        }
    }
    if(m_use_texture_mapping || m_use_bump_mapping || m_use_ssao) {
        m_var_attributes[var_attribute_type_texcoord]->disable_vertex_attrib_array();
    }
}

void ShaderContext::set_ambient_color(GLfloat* ambient_color)
{
    m_var_uniforms[var_uniform_type_ambient_color]->uniform_3fv(1, ambient_color);
}

void ShaderContext::set_mvp_xform(glm::mat4 mvp_xform)
{
    m_var_uniforms[var_uniform_type_mvp_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(mvp_xform));
}

void ShaderContext::set_model_xform(glm::mat4 model_xform)
{
    m_var_uniforms[var_uniform_type_model_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(model_xform));
}

void ShaderContext::set_view_proj_xform(glm::mat4 view_proj_xform)
{
    m_var_uniforms[var_uniform_type_view_proj_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(view_proj_xform));
}

void ShaderContext::set_normal_xform(glm::mat4 normal_xform)
{
    m_var_uniforms[var_uniform_type_normal_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(normal_xform));
}

void ShaderContext::set_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_color_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_texture2_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_color_texture2]->uniform_1i(texture_id);
}

void ShaderContext::set_bump_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_bump_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_env_map_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_env_map_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_random_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_random_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_camera_pos(GLfloat* camera_pos_arr)
{
    m_var_uniforms[var_uniform_type_camera_pos]->uniform_3fv(1, camera_pos_arr);
}

void ShaderContext::set_camera_dir(GLfloat* camera_dir_arr)
{
    m_var_uniforms[var_uniform_type_camera_dir]->uniform_3fv(1, camera_dir_arr);
}

void ShaderContext::set_light_pos(size_t num_lights, GLfloat* light_pos_arr)
{
    m_var_uniforms[var_uniform_type_light_pos]->uniform_3fv(num_lights, light_pos_arr);
}

void ShaderContext::set_light_color(size_t num_lights, GLfloat* light_color_arr)
{
    m_var_uniforms[var_uniform_type_light_color]->uniform_3fv(num_lights, light_color_arr);
}

void ShaderContext::set_light_enabled(size_t num_lights, GLint* light_enabled_arr)
{
    m_var_uniforms[var_uniform_type_light_enabled]->uniform_1iv(num_lights, light_enabled_arr);
}

void ShaderContext::set_light_count(GLint light_count)
{
    m_var_uniforms[var_uniform_type_light_count]->uniform_1i(light_count);
}

void ShaderContext::set_inv_projection_xform(glm::mat4 inv_projection_xform)
{
    m_var_uniforms[var_uniform_type_inv_projection_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(inv_projection_xform));
}

void ShaderContext::set_inv_normal_xform(glm::mat4 inv_normal_xform)
{
    m_var_uniforms[var_uniform_type_inv_normal_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(inv_normal_xform));
}

void ShaderContext::set_frontface_depth_overlay_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_frontface_depth_overlay_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_backface_depth_overlay_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_backface_depth_overlay_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_backface_normal_overlay_texture_index(GLint texture_id)
{
    assert(texture_id >= 0 && texture_id < static_cast<int>(m_textures.size()));
    m_var_uniforms[var_uniform_type_backface_normal_overlay_texture]->uniform_1i(texture_id);
}

void ShaderContext::set_viewport_dim(GLfloat* viewport_dim_arr)
{
    m_var_uniforms[var_uniform_type_viewport_dim]->uniform_2fv(1, viewport_dim_arr);
}

void ShaderContext::set_bloom_kernel(GLfloat* bloom_kernel_arr)
{
    m_var_uniforms[var_uniform_type_bloom_kernel]->uniform_1fv(BLOOM_KERNEL_SIZE, bloom_kernel_arr);
}

void ShaderContext::set_glow_cutoff_threshold(GLfloat glow_cutoff_threshold)
{
    m_var_uniforms[var_uniform_type_glow_cutoff_threshold]->uniform_1f(glow_cutoff_threshold);
}

void ShaderContext::set_camera_near(GLfloat camera_near)
{
    m_var_uniforms[var_uniform_type_camera_near]->uniform_1f(camera_near);
}

void ShaderContext::set_camera_far(GLfloat camera_far)
{
    m_var_uniforms[var_uniform_type_camera_far]->uniform_1f(camera_far);
}

void ShaderContext::set_reflect_to_refract_ratio(GLfloat reflect_to_refract_ratio)
{
    m_var_uniforms[var_uniform_type_reflect_to_refract_ratio]->uniform_1f(reflect_to_refract_ratio);
}

void ShaderContext::set_ssao_sample_kernel_pos(size_t num_kernels, GLfloat* kernel_pos_arr)
{
    m_var_uniforms[var_uniform_type_ssao_sample_kernel_pos]->uniform_3fv(num_kernels, kernel_pos_arr);
}

void ShaderContext::set_inv_view_proj_xform(glm::mat4 inv_view_proj_xform)
{
    m_var_uniforms[var_uniform_type_inv_view_proj_xform]->uniform_matrix_4fv(1, GL_FALSE, glm::value_ptr(inv_view_proj_xform));
}

}
