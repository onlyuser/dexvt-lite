#include <Scene.h>
#include <ShaderContext.h>
#include <Camera.h>
#include <FrameBuffer.h>
#include <Light.h>
#include <Mesh.h>
#include <Material.h>
#include <Texture.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <stdlib.h>

#define NUM_LIGHTS              8
#define NUM_SSAO_SAMPLE_KERNELS 3
//#define BLOOM_KERNEL_SIZE       5
#define BLOOM_KERNEL_SIZE       7

namespace vt {

Scene::Scene()
    : m_camera(NULL),
      m_skybox(NULL),
      m_overlay(NULL),
      m_normal_material(NULL),
      m_wireframe_material(NULL),
      m_ssao_material(NULL)
{
    m_viewport_dim[0] = 0;
    m_viewport_dim[1] = 0;
    //const int bloom_kernel_row[BLOOM_KERNEL_SIZE] = {1, 4, 6, 4, 1};
    const int bloom_kernel_row[BLOOM_KERNEL_SIZE] = {1, 6, 15, 20, 15, 6, 1};
    for(int i = 0; i < BLOOM_KERNEL_SIZE; i++) {
        m_bloom_kernel[i] = bloom_kernel_row[i];
    }
    float sum_weights = 0;
    for(int j = 0; j < BLOOM_KERNEL_SIZE; j++) {
        sum_weights += m_bloom_kernel[j];
    }
    float sum_weights_inv = 1/sum_weights;
    for(int k = 0; k < BLOOM_KERNEL_SIZE; k++) {
        m_bloom_kernel[k] *= sum_weights_inv;
    }
    m_glow_cutoff_threshold = 0;
    m_light_pos     = new GLfloat[NUM_LIGHTS*3];
    m_light_color   = new GLfloat[NUM_LIGHTS*3];
    m_light_enabled = new GLint[NUM_LIGHTS];
    for(int q = 0; q < NUM_LIGHTS; q++) {
        m_light_pos[q*3 + 0]   = 0;
        m_light_pos[q*3 + 1]   = 0;
        m_light_pos[q*3 + 2]   = 0;
        m_light_color[q*3 + 0] = 0;
        m_light_color[q*3 + 1] = 0;
        m_light_color[q*3 + 2] = 0;
        m_light_enabled[q]     = 0;
    }

    // http://john-chapman-graphics.blogspot.tw/2013/01/ssao-tutorial.html
    m_ssao_sample_kernel_pos = new GLfloat[NUM_SSAO_SAMPLE_KERNELS*3];
    for(int r = 0; r < NUM_SSAO_SAMPLE_KERNELS; r++) {
        glm::vec3 offset;
        do {
            offset = glm::vec3(
                    m_ssao_sample_kernel_pos[r*3 + 0] = static_cast<float>(rand())/RAND_MAX*2 - 1,
                    m_ssao_sample_kernel_pos[r*3 + 1] = static_cast<float>(rand())/RAND_MAX*2 - 1,
                    m_ssao_sample_kernel_pos[r*3 + 2] = static_cast<float>(rand())/RAND_MAX);
        } while(glm::dot(glm::vec3(0, 0, 1), offset) < 0.15);
        float scale = static_cast<float>(r)/NUM_SSAO_SAMPLE_KERNELS;
        scale = glm::lerp(0.1f, 1.0f, scale*scale);
        offset *= scale;
        m_ssao_sample_kernel_pos[r*3 + 0] = offset.x;
        m_ssao_sample_kernel_pos[r*3 + 1] = offset.y;
        m_ssao_sample_kernel_pos[r*3 + 2] = offset.z;
    }
}

Scene::~Scene()
{
    if(m_camera) {
        delete m_camera;
    }
    lights_t::const_iterator p;
    for(p = m_lights.begin(); p != m_lights.end(); p++) {
        delete *p;
    }
    meshes_t::const_iterator q;
    for(q = m_meshes.begin(); q != m_meshes.end(); q++) {
        delete *q;
    }
    materials_t::const_iterator r;
    for(r = m_materials.begin(); r != m_materials.end(); r++) {
        delete *r;
    }
    textures_t::const_iterator s;
    for(s = m_textures.begin(); s != m_textures.end(); s++) {
        delete *s;
    }
    if(m_light_pos) {
        delete []m_light_pos;
    }
    if(m_light_color) {
        delete []m_light_color;
    }
    if(m_light_enabled) {
        delete []m_light_enabled;
    }
    if(m_ssao_sample_kernel_pos) {
        delete []m_ssao_sample_kernel_pos;
    }
}

void Scene::add_texture(Texture* texture)
{
    m_textures.push_back(texture);
}

void Scene::reset()
{
    m_camera = NULL;
    m_lights.clear();
    m_meshes.clear();
    m_materials.clear();
    m_textures.clear();
}

void Scene::use_program()
{
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); q++) {
        (*q)->get_shader_context()->get_material()->get_program()->use();
    }
}

void Scene::render(
        bool                render_overlay,
        bool                render_skybox,
        use_material_type_t use_material_type)
{
    m_viewport_dim[0] = m_camera->get_width();
    m_viewport_dim[1] = m_camera->get_height();
    if(render_overlay && m_overlay) {
        ShaderContext* shader_context = m_overlay->get_shader_context();
        if(!shader_context) {
            return;
        }
        Material* material = shader_context->get_material();
        if(!material) {
            return;
        }
        Program * program = material->get_program();
        if(!program) {
            return;
        }
        program->use();
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_bloom_kernel)) {
            shader_context->set_bloom_kernel(m_bloom_kernel);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_glow_cutoff_threshold)) {
            shader_context->set_glow_cutoff_threshold(m_glow_cutoff_threshold);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture)) {
            shader_context->set_texture_index(m_overlay->get_texture_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture2)) {
            shader_context->set_texture2_index(m_overlay->get_texture2_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_viewport_dim)) {
            shader_context->set_viewport_dim(m_viewport_dim);
        }
        shader_context->render();
        return;
    }
    if(render_skybox && m_skybox) {
        ShaderContext* shader_context = m_skybox->get_shader_context();
        if(!shader_context) {
            return;
        }
        Material* material = shader_context->get_material();
        if(!material) {
            return;
        }
        Program * program = material->get_program();
        if(!program) {
            return;
        }
        program->use();
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_env_map_texture)) {
            shader_context->set_env_map_texture_index(0); // skymap texture index
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_normal_xform)) {
            shader_context->set_inv_normal_xform(glm::inverse(m_camera->get_normal_xform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_projection_xform)) {
            shader_context->set_inv_projection_xform(glm::inverse(m_camera->get_projection_xform()));
        }
        shader_context->render();
    }
    int i = 0;
    for(lights_t::const_iterator p = m_lights.begin(); p != m_lights.end(); p++) {
        glm::vec3 light_pos = (*p)->get_origin();
        m_light_pos[i*3 + 0] = light_pos.x;
        m_light_pos[i*3 + 1] = light_pos.y;
        m_light_pos[i*3 + 2] = light_pos.z;
        glm::vec3 light_color = (*p)->get_color();
        m_light_color[i*3 + 0] = light_color.r;
        m_light_color[i*3 + 1] = light_color.g;
        m_light_color[i*3 + 2] = light_color.b;
        m_light_enabled[i] = (*p)->get_enabled();
        i++;
    }
    FrameBuffer* frame_buffer = m_camera->get_frame_buffer();
    if(frame_buffer) {
        Texture* texture = frame_buffer->get_texture();
        if(texture) {
            m_viewport_dim[0] = texture->get_width();
            m_viewport_dim[1] = texture->get_height();
        }
    }
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); q++) {
        Mesh* mesh = (*q);
        if(!mesh->get_visible()) {
            continue;
        }
        ShaderContext* shader_context = NULL;
        switch(use_material_type) {
            case use_material_type_t::USE_MESH_MATERIAL:
                shader_context = mesh->get_shader_context();
                break;
            case use_material_type_t::USE_NORMAL_MATERIAL:
                shader_context = mesh->get_normal_shader_context(m_normal_material);
                break;
            case use_material_type_t::USE_WIREFRAME_MATERIAL:
                shader_context = mesh->get_wireframe_shader_context(m_wireframe_material);
                break;
            case use_material_type_t::USE_SSAO_MATERIAL:
                shader_context = mesh->get_ssao_shader_context(m_ssao_material);
                break;
        }
        if(!shader_context) {
            continue;
        }
        Material* material = shader_context->get_material();
        if(!material) {
            continue;
        }
        Program * program = material->get_program();
        if(!program) {
            continue;
        }
        program->use();
        glm::mat4 vp_xform = m_camera->get_projection_xform()*m_camera->get_xform();
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ambient_color)) {
            shader_context->set_ambient_color(glm::value_ptr(mesh->get_ambient_color()));
        }
        if(use_material_type != use_material_type_t::USE_SSAO_MATERIAL) {
            if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_backface_depth_overlay_texture)) {
                shader_context->set_backface_depth_overlay_texture_index(mesh->get_backface_depth_overlay_texture_index());
            }
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_backface_normal_overlay_texture)) {
            shader_context->set_backface_normal_overlay_texture_index(mesh->get_backface_normal_overlay_texture_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_bloom_kernel)) {
            shader_context->set_bloom_kernel(m_bloom_kernel);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_bump_texture)) {
            shader_context->set_bump_texture_index(mesh->get_bump_texture_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_camera_dir)) {
            shader_context->set_camera_dir(glm::value_ptr(m_camera->get_dir()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_camera_far)) {
            shader_context->set_camera_far(m_camera->get_far_plane());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_camera_near)) {
            shader_context->set_camera_near(m_camera->get_near_plane());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_camera_pos)) {
            shader_context->set_camera_pos(glm::value_ptr(m_camera->get_origin()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_env_map_texture)) {
            shader_context->set_env_map_texture_index(0); // skymap texture index
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_frontface_depth_overlay_texture)) {
            if(use_material_type == use_material_type_t::USE_SSAO_MATERIAL) {
                shader_context->set_frontface_depth_overlay_texture_index(material->get_texture_index_by_name("frontface_depth_overlay"));
            } else {
                shader_context->set_frontface_depth_overlay_texture_index(mesh->get_frontface_depth_overlay_texture_index());
            }
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_glow_cutoff_threshold)) {
            shader_context->set_glow_cutoff_threshold(m_glow_cutoff_threshold);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_normal_xform)) {
            shader_context->set_inv_normal_xform(glm::inverse(m_camera->get_normal_xform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_projection_xform)) {
            shader_context->set_inv_projection_xform(glm::inverse(m_camera->get_projection_xform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_view_proj_xform)) {
            shader_context->set_inv_view_proj_xform(glm::inverse(m_camera->get_xform())*glm::inverse(m_camera->get_projection_xform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_light_color)) {
            shader_context->set_light_color(NUM_LIGHTS, m_light_color);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_light_count)) {
            shader_context->set_light_count(m_lights.size());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_light_enabled)) {
            shader_context->set_light_enabled(NUM_LIGHTS, m_light_enabled);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_light_pos)) {
            shader_context->set_light_pos(NUM_LIGHTS, m_light_pos);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_model_xform)) {
            shader_context->set_model_xform(mesh->get_xform());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_mvp_xform)) {
            shader_context->set_mvp_xform(vp_xform*mesh->get_xform());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_normal_xform)) {
            shader_context->set_normal_xform(mesh->get_normal_xform());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_random_texture)) {
            shader_context->set_random_texture_index(material->get_texture_index_by_name("random_texture"));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_reflect_to_refract_ratio)) {
            shader_context->set_reflect_to_refract_ratio(mesh->get_reflect_to_refract_ratio());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ssao_sample_kernel_pos)) {
            shader_context->set_ssao_sample_kernel_pos(NUM_SSAO_SAMPLE_KERNELS, m_ssao_sample_kernel_pos);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture)) {
            shader_context->set_texture_index(mesh->get_texture_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture2)) {
            shader_context->set_texture2_index(m_overlay->get_texture2_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_view_proj_xform)) {
            shader_context->set_view_proj_xform(vp_xform);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_viewport_dim)) {
            shader_context->set_viewport_dim(m_viewport_dim);
        }
        shader_context->render();
    }
}

void Scene::render_lines(bool draw_axis, bool draw_bbox, bool draw_normals) const
{
    const float axis_surface_distance = 0.05;
    const float axis_arm_length       = 0.1;

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_xform()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); q++) {
        if(!(*q)->get_visible()) {
            continue;
        }
        glm::mat4 model_xform = m_camera->get_xform()*(*q)->get_xform();
        glLoadMatrixf(glm::value_ptr(model_xform));
        glBegin(GL_LINES);

        if(draw_axis) {
            // x-axis
            {
                glColor3f(1, 0, 0);
                glm::vec3 v;
                glVertex3fv(&v.x);
                v += glm::vec3(axis_arm_length, 0, 0);
                glVertex3fv(&v.x);
            }

            // y-axis
            {
                glColor3f(0, 1, 0);
                glm::vec3 v;
                glVertex3fv(&v.x);
                v += glm::vec3(0, axis_arm_length, 0);
                glVertex3fv(&v.x);
            }

            // z-axis
            {
                glColor3f(0, 0, 1);
                glm::vec3 v;
                glVertex3fv(&v.x);
                v += glm::vec3(0, 0, axis_arm_length);
                glVertex3fv(&v.x);
            }
        }

        if(draw_bbox) {
            glm::vec3 min, max;
            (*q)->get_min_max(&min, &max);
            min -= glm::vec3(axis_surface_distance);
            max += glm::vec3(axis_surface_distance);
            glColor3f(1, 0, 0);

            glm::vec3 llb(min.x, min.y, min.z);
            glm::vec3 lrb(max.x, min.y, min.z);
            glm::vec3 urb(max.x, max.y, min.z);
            glm::vec3 ulb(min.x, max.y, min.z);
            glm::vec3 llf(min.x, min.y, max.z);
            glm::vec3 lrf(max.x, min.y, max.z);
            glm::vec3 urf(max.x, max.y, max.z);
            glm::vec3 ulf(min.x, max.y, max.z);

            // back quad

            glVertex3fv(&llb.x);
            glVertex3fv(&lrb.x);

            glVertex3fv(&lrb.x);
            glVertex3fv(&urb.x);

            glVertex3fv(&urb.x);
            glVertex3fv(&ulb.x);

            glVertex3fv(&ulb.x);
            glVertex3fv(&llb.x);

            // front quad

            glVertex3fv(&llf.x);
            glVertex3fv(&lrf.x);

            glVertex3fv(&lrf.x);
            glVertex3fv(&urf.x);

            glVertex3fv(&urf.x);
            glVertex3fv(&ulf.x);

            glVertex3fv(&ulf.x);
            glVertex3fv(&llf.x);

            // back to front segments

            glVertex3fv(&llb.x);
            glVertex3fv(&llf.x);

            glVertex3fv(&lrb.x);
            glVertex3fv(&lrf.x);

            glVertex3fv(&urb.x);
            glVertex3fv(&urf.x);

            glVertex3fv(&ulb.x);
            glVertex3fv(&ulf.x);
        }

        if(draw_normals) {
            // normal
            glColor3f(0, 0, 1);
            for (int i = 0; i < static_cast<int>((*q)->get_num_vertex()); i++){
                glm::vec3 v = (*q)->get_vert_coord(i);
                v += (*q)->get_vert_normal(i)*axis_surface_distance;
                glVertex3fv(&v.x);
                v += (*q)->get_vert_normal(i)*axis_arm_length;
                glVertex3fv(&v.x);
            }

            // tangent
            glColor3f(1, 0, 0);
            for (int i = 0; i < static_cast<int>((*q)->get_num_vertex()); i++){
                glm::vec3 v = (*q)->get_vert_coord(i);
                v += (*q)->get_vert_normal(i)*axis_surface_distance;
                glVertex3fv(&v.x);
                v += (*q)->get_vert_tangent(i)*axis_arm_length;
                glVertex3fv(&v.x);
            }

            // bitangent
            glColor3f(0, 1, 0);
            for (int i = 0; i < static_cast<int>((*q)->get_num_vertex()); i++){
                glm::vec3 v = (*q)->get_vert_coord(i);
                v += (*q)->get_vert_normal(i)*axis_surface_distance;
                glVertex3fv(&v.x);
                glm::vec3 bitangent = glm::normalize(glm::cross((*q)->get_vert_normal(i), (*q)->get_vert_tangent(i)));
                v += bitangent*axis_arm_length;
                glVertex3fv(&v.x);
            }
        }

        glEnd();
    }
    glPopMatrix();
}

void Scene::render_lights() const
{
    const float light_radius = 0.125;

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_xform()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glColor3f(1, 1, 0);
    for(lights_t::const_iterator p = m_lights.begin(); p != m_lights.end(); p++) {
        glm::mat4 model_xform = m_camera->get_xform()*(*p)->get_xform();
        glLoadMatrixf(glm::value_ptr(model_xform));
        glutWireSphere(light_radius, 4, 2);
    }
}

}
