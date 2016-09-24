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

    enum var_attribute_type_t {
        var_attribute_type_texcoord,
        var_attribute_type_vertex_normal,
        var_attribute_type_vertex_position,
        var_attribute_type_vertex_tangent,
        var_attribute_type_count
    };

    typedef std::pair<var_attribute_type_t, const char*> var_attribute_type_to_name_table_t;
    static var_attribute_type_to_name_table_t m_var_attribute_type_to_name_table[];

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
        var_uniform_type_env_map_texture,
        var_uniform_type_frontface_depth_overlay_texture,
        var_uniform_type_glow_cutoff_threshold,
        var_uniform_type_inv_normal_xform,
        var_uniform_type_inv_projection_xform,
        var_uniform_type_inv_view_proj_xform,
        var_uniform_type_light_color,
        var_uniform_type_light_count,
        var_uniform_type_light_enabled,
        var_uniform_type_light_pos,
        var_uniform_type_model_xform,
        var_uniform_type_mvp_xform,
        var_uniform_type_normal_xform,
        var_uniform_type_random_texture,
        var_uniform_type_reflect_to_refract_ratio,
        var_uniform_type_ssao_sample_kernel_pos,
        var_uniform_type_viewport_dim,
        var_uniform_type_view_proj_xform,
        var_uniform_type_count
    };

    typedef std::pair<var_uniform_type_t, const char*> var_uniform_type_to_name_table_t;
    static var_uniform_type_to_name_table_t m_var_uniform_type_to_name_table[];

    ShaderContext(
            Material* material,
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
    void set_ambient_color(GLfloat* ambient_color);
    void set_mvp_xform(glm::mat4 mvp_xform);
    void set_model_xform(glm::mat4 model_xform);
    void set_view_proj_xform(glm::mat4 view_proj_xform);
    void set_normal_xform(glm::mat4 normal_xform);
    void set_texture_index(GLint texture_id);
    void set_texture2_index(GLint texture_id);
    void set_bump_texture_index(GLint texture_id);
    void set_env_map_texture_index(GLint texture_id);
    void set_random_texture_index(GLint texture_id);
    void set_camera_pos(GLfloat* camera_pos_arr);
    void set_camera_dir(GLfloat* camera_dir_arr);
    void set_light_pos(size_t num_lights, GLfloat* light_pos_arr);
    void set_light_color(size_t num_lights, GLfloat* light_color_arr);
    void set_light_enabled(size_t num_lights, GLint* light_enabled_arr);
    void set_light_count(GLint light_count);
    void set_inv_projection_xform(glm::mat4 inv_projection_xform);
    void set_inv_normal_xform(glm::mat4 inv_normal_xform);
    void set_frontface_depth_overlay_texture_index(GLint texture_id);
    void set_backface_depth_overlay_texture_index(GLint texture_id);
    void set_backface_normal_overlay_texture_index(GLint texture_id);
    void set_viewport_dim(GLfloat* viewport_dim_arr);
    void set_bloom_kernel(GLfloat* bloom_kernel_arr);
    void set_glow_cutoff_threshold(GLfloat glow_cutoff_threshold);
    void set_camera_near(GLfloat camera_near);
    void set_camera_far(GLfloat camera_far);
    void set_reflect_to_refract_ratio(GLfloat reflect_to_refract_ratio);
    void set_ssao_sample_kernel_pos(size_t num_kernels, GLfloat* kernel_pos_arr);
    void set_inv_view_proj_xform(glm::mat4 inv_view_proj_xform);

private:
    Material *m_material;
    Buffer *m_vbo_vert_coords, *m_vbo_vert_normal, *m_vbo_vert_tangent, *m_vbo_tex_coords, *m_ibo_tri_indices;
    VarAttribute* m_var_attributes[var_attribute_type_count];
    VarUniform* m_var_uniforms[var_uniform_type_count];
    const textures_t &m_textures;
    bool m_use_ambient_color;
    bool m_gen_normal_map;
    bool m_use_phong_shading;
    bool m_use_texture_mapping;
    bool m_use_bump_mapping;
    bool m_use_env_mapping;
    bool m_use_env_mapping_dbl_refract;
    bool m_use_ssao;
    bool m_use_bloom_kernel;
    bool m_use_texture2;
    bool m_use_fragment_world_pos;
    bool m_skybox;
    bool m_overlay;
};

}

#endif
