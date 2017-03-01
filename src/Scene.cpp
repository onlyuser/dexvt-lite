#include <Scene.h>
#include <ShaderContext.h>
#include <Camera.h>
#include <FrameBuffer.h>
#include <Light.h>
#include <Mesh.h>
#include <Material.h>
#include <Texture.h>
#include <Util.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/string_cast.hpp>
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
#define LIGHT_RADIUS            0.125
#define TARGET_RADIUS           0.125
#define TARGETS_RADIUS          0.0625

namespace vt {

Scene::Scene()
    : m_camera(NULL),
      m_skybox(NULL),
      m_overlay(NULL),
      m_normal_material(NULL),
      m_wireframe_material(NULL),
      m_ssao_material(NULL)
{
    //const int bloom_kernel_row[BLOOM_KERNEL_SIZE] = {1, 4, 6, 4, 1};
    const int bloom_kernel_row[BLOOM_KERNEL_SIZE] = {1, 6, 15, 20, 15, 6, 1};
    for(int i = 0; i < BLOOM_KERNEL_SIZE; i++) {
        m_bloom_kernel[i] = bloom_kernel_row[i];
    }
    float sum_weights = 0;
    for(int j = 0; j < BLOOM_KERNEL_SIZE; j++) {
        sum_weights += m_bloom_kernel[j];
    }
    float sum_weights_inv = 1 / sum_weights;
    for(int k = 0; k < BLOOM_KERNEL_SIZE; k++) {
        m_bloom_kernel[k] *= sum_weights_inv;
    }
    m_glow_cutoff_threshold = 0;
    m_light_pos     = new GLfloat[NUM_LIGHTS * 3];
    m_light_color   = new GLfloat[NUM_LIGHTS * 3];
    m_light_enabled = new GLint[NUM_LIGHTS];
    for(int q = 0; q < NUM_LIGHTS; q++) {
        m_light_pos[q * 3 + 0]   = 0;
        m_light_pos[q * 3 + 1]   = 0;
        m_light_pos[q * 3 + 2]   = 0;
        m_light_color[q * 3 + 0] = 0;
        m_light_color[q * 3 + 1] = 0;
        m_light_color[q * 3 + 2] = 0;
        m_light_enabled[q]       = 0;
    }

    // http://john-chapman-graphics.blogspot.tw/2013/01/ssao-tutorial.html
    m_ssao_sample_kernel_pos = new GLfloat[NUM_SSAO_SAMPLE_KERNELS * 3];
    for(int r = 0; r < NUM_SSAO_SAMPLE_KERNELS; r++) {
        glm::vec3 offset;
        do {
            offset = glm::vec3(
                    m_ssao_sample_kernel_pos[r * 3 + 0] = (static_cast<float>(rand()) / RAND_MAX) * 2 - 1,
                    m_ssao_sample_kernel_pos[r * 3 + 1] = (static_cast<float>(rand()) / RAND_MAX) * 2 - 1,
                    m_ssao_sample_kernel_pos[r * 3 + 2] =  static_cast<float>(rand()) / RAND_MAX);
        } while(glm::dot(glm::vec3(0, 0, 1), offset) < 0.15);
        float scale = static_cast<float>(r) / NUM_SSAO_SAMPLE_KERNELS;
        scale = glm::lerp(0.1f, 1.0f, scale * scale);
        offset *= scale;
        m_ssao_sample_kernel_pos[r * 3 + 0] = offset.x;
        m_ssao_sample_kernel_pos[r * 3 + 1] = offset.y;
        m_ssao_sample_kernel_pos[r * 3 + 2] = offset.z;
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
        delete[] m_light_pos;
    }
    if(m_light_color) {
        delete[] m_light_color;
    }
    if(m_light_enabled) {
        delete[] m_light_enabled;
    }
    if(m_ssao_sample_kernel_pos) {
        delete[] m_ssao_sample_kernel_pos;
    }
}

void Scene::reset()
{
    m_camera = NULL;
    m_lights.clear();
    m_meshes.clear();
    m_materials.clear();
    m_textures.clear();
}

Light* Scene::find_light(std::string name)
{
    lights_t::iterator p = std::find_if(m_lights.begin(), m_lights.end(), FindByName(name));
    if(p == m_lights.end()) {
        return NULL;
    }
    return *p;
}

void Scene::add_light(Light* light)
{
    m_lights.push_back(light);
}

void Scene::remove_light(Light* light)
{
    lights_t::iterator p = std::find(m_lights.begin(), m_lights.end(), light);
    if(p == m_lights.end()) {
        return;
    }
    m_lights.erase(p);
}

Mesh* Scene::find_mesh(std::string name)
{
    meshes_t::iterator p = std::find_if(m_meshes.begin(), m_meshes.end(), FindByName(name));
    if(p == m_meshes.end()) {
        return NULL;
    }
    return *p;
}

void Scene::add_mesh(Mesh* mesh)
{
    m_meshes.push_back(mesh);
}

void Scene::remove_mesh(Mesh* mesh)
{
    meshes_t::iterator p = std::find(m_meshes.begin(), m_meshes.end(), mesh);
    if(p == m_meshes.end()) {
        return;
    }
    (*p)->link_parent(NULL);
    (*p)->unlink_children();
    m_meshes.erase(p);
}

Material* Scene::find_material(std::string name)
{
    materials_t::iterator p = std::find_if(m_materials.begin(), m_materials.end(), FindByName(name));
    if(p == m_materials.end()) {
        return NULL;
    }
    return *p;
}

void Scene::add_material(Material* material)
{
    m_materials.push_back(material);
}

void Scene::remove_material(Material* material)
{
    // TODO: should check for existing references
    materials_t::iterator p = std::find(m_materials.begin(), m_materials.end(), material);
    if(p == m_materials.end()) {
        return;
    }
    m_materials.erase(p);
}

Texture* Scene::find_texture(std::string name)
{
    textures_t::iterator p = std::find_if(m_textures.begin(), m_textures.end(), FindByName(name));
    if(p == m_textures.end()) {
        return NULL;
    }
    return *p;
}

void Scene::add_texture(Texture* texture)
{
    m_textures.push_back(texture);
}

void Scene::remove_texture(Texture* texture)
{
    // TODO: should check for existing references
    textures_t::iterator p = std::find(m_textures.begin(), m_textures.end(), texture);
    if(p == m_textures.end()) {
        return;
    }
    m_textures.erase(p);
}

void Scene::use_program()
{
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); q++) {
        (*q)->get_shader_context()->get_material()->get_program()->use();
    }
}

void Scene::render(bool                clear_canvas,
                   bool                render_overlay,
                   bool                render_skybox,
                   use_material_type_t use_material_type)
{
    if(clear_canvas) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }
    if(render_overlay && m_overlay) {
        ShaderContext* shader_context = m_overlay->get_shader_context();
        if(!shader_context) {
            return;
        }
        Material* material = shader_context->get_material();
        if(!material) {
            return;
        }
        Program* program = material->get_program();
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
            shader_context->set_viewport_dim(glm::value_ptr(m_camera->get_dim()));
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
        Program* program = material->get_program();
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
        m_light_pos[i * 3 + 0] = light_pos.x;
        m_light_pos[i * 3 + 1] = light_pos.y;
        m_light_pos[i * 3 + 2] = light_pos.z;
        glm::vec3 light_color = (*p)->get_color();
        m_light_color[i * 3 + 0] = light_color.r;
        m_light_color[i * 3 + 1] = light_color.g;
        m_light_color[i * 3 + 2] = light_color.b;
        m_light_enabled[i] = (*p)->is_enabled();
        i++;
    }
    FrameBuffer* frame_buffer = m_camera->get_frame_buffer();
    Texture* texture = NULL;
    if(frame_buffer) {
        texture = frame_buffer->get_texture();
    }
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); q++) {
        Mesh* mesh = (*q);
        if(!mesh->is_visible()) {
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
        Program* program = material->get_program();
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
            if(texture) {
                float dim[2];
                dim[0] = static_cast<float>(texture->get_dim().x);
                dim[1] = static_cast<float>(texture->get_dim().y);
                shader_context->set_viewport_dim(dim);
            } else {
                shader_context->set_viewport_dim(glm::value_ptr(m_camera->get_dim()));
            }
        }
        shader_context->render();
    }
}

void Scene::render_lines_and_text(bool  draw_guide_wires,
                                  bool  draw_axis,
                                  bool  draw_axis_labels,
                                  bool  draw_bbox,
                                  bool  draw_normals,
                                  bool  draw_hud_text,
                                  char* hud_text) const
{
    const float normal_surface_distance     = 0.05;
    const float up_arm_length               = 0.5;                // white
    const float local_pivot_length          = 1  * up_arm_length; // magenta
    const float end_effector_tip_dir_length = 10 * up_arm_length; // yellow
    const float target_dir_length           = 10 * up_arm_length; // cyan
    const float guide_wire_width            = 1;
    const float axis_arm_length             = 0.25;
    const float axis_line_width             = 3;
    const float bbox_line_width             = 1;
    const float normal_arm_length           = 0.125;
    const float normal_line_width           = 1;

    glDisable(GL_DEPTH_TEST);

    glUseProgram(0);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_xform()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    if(draw_guide_wires) {
        glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * glm::translate(glm::mat4(1), m_debug_target)));
        glLineWidth(guide_wire_width);
        glBegin(GL_LINES);

        // magenta
        glColor3f(1, 0, 1);
        glutWireSphere(TARGET_RADIUS, 4, 2);

        for(std::vector<glm::vec3>::const_iterator p = m_debug_targets.begin(); p != m_debug_targets.end(); p++) {
            glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * glm::translate(glm::mat4(1), *p)));

            // red
            glColor3f(1, 0, 0);
            glutWireSphere(TARGETS_RADIUS, 4, 2);
        }

        glEnd();
        glLineWidth(1);
    }

    for(meshes_t::const_iterator p = m_meshes.begin(); p != m_meshes.end(); p++) {
        if(!(*p)->is_visible()) {
            continue;
        }

        if(draw_guide_wires && ((*p)->get_parent() || (*p)->get_children().size())) {
            glLoadMatrixf(glm::value_ptr(m_camera->get_xform()));
            glLineWidth(guide_wire_width);
            glBegin(GL_LINES);

            // white
            glColor3f(1, 1, 1);
            glm::vec3 abs_origin = (*p)->in_abs_system();
            glVertex3fv(&abs_origin.x);
            glm::vec3 endpoint = glm::vec3((*p)->get_xform() * glm::vec4(VEC_UP * up_arm_length, 1));
            glVertex3fv(&endpoint.x);

            glEnd();
            glLineWidth(1);
        }

        if(draw_guide_wires && (*p)->get_parent()) {
            glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * (*p)->get_parent()->get_xform()));
            glLineWidth(guide_wire_width);
            glBegin(GL_LINES);

            {
                // magenta
                glColor3f(1, 0, 1);
                glm::vec3 origin = glm::vec3(glm::vec4((*p)->get_origin(), 1));
                glVertex3fv(&origin.x);
                glm::vec3 endpoint = glm::vec3(glm::vec4((*p)->get_origin() + (*p)->m_debug_local_pivot * local_pivot_length, 1));
                glVertex3fv(&endpoint.x);
            }

            {
                // yellow
                glColor3f(1, 1, 0);
                glm::vec3 origin = glm::vec3(glm::vec4((*p)->get_origin(), 1));
                glVertex3fv(&origin.x);
                glm::vec3 endpoint = glm::vec3(glm::vec4((*p)->get_origin() + (*p)->m_debug_end_effector_tip_dir * end_effector_tip_dir_length, 1));
                glVertex3fv(&endpoint.x);
            }

            {
                // cyan
                glColor3f(0, 1, 1);
                glm::vec3 origin = glm::vec3(glm::vec4((*p)->get_origin(), 1));
                glVertex3fv(&origin.x);
                glm::vec3 endpoint = glm::vec3(glm::vec4((*p)->get_origin() + (*p)->m_debug_target_dir * target_dir_length, 1));
                glVertex3fv(&endpoint.x);
            }

            {
                // blue
                glColor3f(0, 0, 1);
                glm::vec3 origin = glm::vec3(glm::vec4((*p)->get_origin(), 1));
                glVertex3fv(&origin.x);
                glm::vec3 endpoint = glm::vec3(glm::vec4((*p)->get_origin() + (*p)->m_debug_local_target, 1));
                glVertex3fv(&endpoint.x);
            }

            glEnd();
            glLineWidth(1);
        }

        if(draw_axis) {
            glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * (*p)->get_xform()));
            glLineWidth(axis_line_width);
            glBegin(GL_LINES);

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

            glEnd();
            glLineWidth(1);
        }

        if(draw_bbox) {
            glEnable(GL_DEPTH_TEST);

            glm::vec3 min, max;
            (*p)->get_min_max(&min, &max);
            min -= glm::vec3(normal_surface_distance);
            max += glm::vec3(normal_surface_distance);

            glm::vec3 llb(min.x, min.y, min.z);
            glm::vec3 lrb(max.x, min.y, min.z);
            glm::vec3 urb(max.x, max.y, min.z);
            glm::vec3 ulb(min.x, max.y, min.z);
            glm::vec3 llf(min.x, min.y, max.z);
            glm::vec3 lrf(max.x, min.y, max.z);
            glm::vec3 urf(max.x, max.y, max.z);
            glm::vec3 ulf(min.x, max.y, max.z);

            glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * (*p)->get_xform()));
            glLineWidth(bbox_line_width);
            glBegin(GL_LINES);

            glColor3f(1, 0, 0);

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

            glEnd();
            glLineWidth(1);

            glDisable(GL_DEPTH_TEST);
        }

        if(draw_normals) {
            glEnable(GL_DEPTH_TEST);

            glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * (*p)->get_xform()));
            glLineWidth(normal_line_width);
            glBegin(GL_LINES);

            size_t num_vertex = (*p)->get_num_vertex();

            // normal
            glColor3f(0, 0, 1);
            for(int i = 0; i < static_cast<int>(num_vertex); i++) {
                glm::vec3 v = (*p)->get_vert_coord(i);
                v += (*p)->get_vert_normal(i) * normal_surface_distance;
                glVertex3fv(&v.x);
                v += (*p)->get_vert_normal(i) * normal_arm_length;
                glVertex3fv(&v.x);
            }

            // tangent
            glColor3f(1, 0, 0);
            for(int j = 0; j < static_cast<int>(num_vertex); j++) {
                glm::vec3 v = (*p)->get_vert_coord(j);
                v += (*p)->get_vert_normal(j) * normal_surface_distance;
                glVertex3fv(&v.x);
                v += (*p)->get_vert_tangent(j) * normal_arm_length;
                glVertex3fv(&v.x);
            }

            // bitangent
            glColor3f(0, 1, 0);
            for(int k = 0; k < static_cast<int>(num_vertex); k++) {
                glm::vec3 v = (*p)->get_vert_coord(k);
                v += (*p)->get_vert_normal(k) * normal_surface_distance;
                glVertex3fv(&v.x);
                glm::vec3 bitangent = glm::normalize(glm::cross((*p)->get_vert_normal(k), (*p)->get_vert_tangent(k)));
                v += bitangent * normal_arm_length;
                glVertex3fv(&v.x);
            }

            glEnd();
            glLineWidth(1);

            glDisable(GL_DEPTH_TEST);
        }

        if(draw_axis_labels) {
            glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * (*p)->get_xform()));
            std::string axis_label;
#if 1
            axis_label = (*p)->get_name();
#else
            glm::vec3 abs_origin;
            XformObject* parent = (*p)->get_parent();
            if(parent) {
                abs_origin = glm::vec3(parent->get_xform() * glm::vec4((*p)->get_origin(), 1));
            } else {
                abs_origin = (*p)->get_origin();
            }
            axis_label = (*p)->get_name() + ": " + glm::to_string(abs_origin);
#endif
            glColor3f(1, 1, 1);
            glRasterPos2f(0, 0);
            print_bitmap_string(GLUT_BITMAP_HELVETICA_18, axis_label.c_str());
        }
    }

    glPopMatrix();

    if(draw_hud_text) {
        glMatrixMode(GL_PROJECTION);
        Camera::projection_mode_t prev_projection_mode = m_camera->get_projection_mode();
        m_camera->set_projection_mode(Camera::PROJECTION_MODE_ORTHO);
        glLoadMatrixf(glm::value_ptr(m_camera->get_projection_xform()));
        m_camera->set_projection_mode(prev_projection_mode);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glm::vec2 dim(m_camera->get_width(), m_camera->get_height());
        float aspect_ratio = m_camera->get_aspect_ratio();
        float half_width  = 0.45;
        float half_height = 0.45;
        if(dim.y < dim.x) {
            half_width *= aspect_ratio;
        }
        if(dim.x < dim.y) {
            half_height /= aspect_ratio;
        }
        glLoadMatrixf(glm::value_ptr(glm::translate(glm::mat4(1), glm::vec3(-half_width, half_height, 0)) * m_camera->get_xform()));
        glColor3f(1, 1, 1);
        glRasterPos2f(0, 0);
        print_bitmap_string(GLUT_BITMAP_HELVETICA_18, hud_text);
        glPopMatrix();
    }

    glEnable(GL_DEPTH_TEST);
}

void Scene::render_lights() const
{
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_xform()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    for(lights_t::const_iterator p = m_lights.begin(); p != m_lights.end(); p++) {
        glLoadMatrixf(glm::value_ptr(m_camera->get_xform() * (*p)->get_xform()));
        glColor3f(1, 1, 0);
        glutWireSphere(TARGET_RADIUS, 4, 2);
    }
    glPopMatrix();
}

}
