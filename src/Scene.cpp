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

#include <Scene.h>
#include <ShaderContext.h>
#include <Camera.h>
#include <FrameBuffer.h>
#include <Light.h>
#include <Mesh.h>
#include <Material.h>
#include <Octree.h>
#include <Texture.h>
#include <PrimitiveFactory.h>
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
#define MAX_SPHERES             4
#define MAX_PLANES              4
#define MAX_BOXES               4
#define MAX_RANDOM_POINTS       20

#define BROKEN_EDGE_ALPHA 0.125f

#define draw_edge(p1, p2) \
        glVertex3fv(&p1.x); \
        glVertex3fv(&p2.x);
#define draw_broken_edge(p1, p2, m1, m2) \
        m1 = MIX(p1, p2, BROKEN_EDGE_ALPHA); \
        m2 = MIX(p1, p2, 1 - BROKEN_EDGE_ALPHA); \
        glVertex3fv(&p1.x); \
        glVertex3fv(&m1.x); \
        glVertex3fv(&m2.x); \
        glVertex3fv(&p2.x);

#define CONSTRAINT_SWIPE_STEP_ANGLE 5
#define CONSTRAINT_SWIPE_RADIUS     1.0f

#define OCTREE_MARGIN               0.01f
#define OCTREE_RENDER_LABEL_LEVELS -1

#define DEFAULT_RAY_TRACER_RENDER_MODE  0
#define DEFAULT_RAY_TRACER_BOUNCE_COUNT 2

namespace vt {

DebugObjectContext::DebugObjectContext()
    : m_transform(glm::translate(glm::mat4(1), glm::vec3(0)))
{
}

Scene::Scene()
    : m_ray_tracer_render_mode(DEFAULT_RAY_TRACER_RENDER_MODE),
      m_ray_tracer_bounce_count(DEFAULT_RAY_TRACER_BOUNCE_COUNT),
      m_ray_tracer_sphere_count(0),
      m_ray_tracer_plane_count(0),
      m_ray_tracer_box_count(0),
      m_ray_tracer_random_point_count(0),
      m_ray_tracer_random_seed(0),
      m_camera(NULL),
      m_octree(NULL),
      m_skybox(NULL),
      m_overlay(NULL),
      m_normal_material(NULL),
      m_wireframe_material(NULL),
      m_ssao_material(NULL),
      m_bloom_kernel(NULL),
      m_glow_cutoff_threshold(0),
      m_light_pos(NULL),
      m_light_color(NULL),
      m_light_enabled(NULL),
      m_ssao_sample_kernel_pos(NULL)
{
    m_glow_cutoff_threshold = 0;
    m_light_pos     = new GLfloat[NUM_LIGHTS * 3];
    m_light_color   = new GLfloat[NUM_LIGHTS * 3];
    m_light_enabled = new GLint[NUM_LIGHTS];
    memset(m_light_pos,     0, sizeof(GLfloat) * 3 * NUM_LIGHTS);
    memset(m_light_color,   0, sizeof(GLfloat) * 3 * NUM_LIGHTS);
    memset(m_light_enabled, 0, sizeof(GLint)       * NUM_LIGHTS);

    m_bloom_kernel = new GLfloat[BLOOM_KERNEL_SIZE];
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

    // http://john-chapman-graphics.blogspot.tw/2013/01/ssao-tutorial.html
    m_ssao_sample_kernel_pos = new GLfloat[NUM_SSAO_SAMPLE_KERNELS * 3];
    for(int r = 0; r < NUM_SSAO_SAMPLE_KERNELS; r++) {
        glm::vec3 offset;
        do {
            offset = glm::vec3(m_ssao_sample_kernel_pos[r * 3 + 0] = (static_cast<float>(rand()) / RAND_MAX) * 2 - 1,
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
    for(p = m_lights.begin(); p != m_lights.end(); ++p) {
        delete *p;
    }
    meshes_t::const_iterator q;
    for(q = m_meshes.begin(); q != m_meshes.end(); ++q) {
        delete *q;
    }
    materials_t::const_iterator r;
    for(r = m_materials.begin(); r != m_materials.end(); ++r) {
        delete *r;
    }
    textures_t::const_iterator s;
    for(s = m_textures.begin(); s != m_textures.end(); ++s) {
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
    if(m_bloom_kernel) {
        delete[] m_bloom_kernel;
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
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); ++q) {
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    int i = 0;
    for(lights_t::const_iterator p = m_lights.begin(); p != m_lights.end(); ++p) {
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
        glm::mat4 vp_transform = m_camera->get_projection_transform() * m_camera->get_transform();
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_bloom_kernel)) {
            shader_context->set_bloom_kernel(m_bloom_kernel);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_glow_cutoff_threshold)) {
            shader_context->set_glow_cutoff_threshold(m_glow_cutoff_threshold);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture)) {
            shader_context->set_color_texture_index(m_overlay->get_color_texture_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture2)) {
            shader_context->set_color_texture2_index(m_overlay->get_color_texture2_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture_source)) {
            shader_context->set_color_texture_source(m_overlay->get_color_texture_source());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_viewport_dim)) {
            shader_context->set_viewport_dim(glm::value_ptr(m_camera->get_dim()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_image_res)) {
            shader_context->set_image_res(glm::value_ptr(m_camera->get_image_res()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_normal_transform)) {
            shader_context->set_inv_normal_transform(glm::inverse(m_camera->get_normal_transform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_projection_transform)) {
            shader_context->set_inv_projection_transform(glm::inverse(m_camera->get_projection_transform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_view_proj_transform)) {
            shader_context->set_view_proj_transform(vp_transform);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_view_proj_transform)) {
            shader_context->set_inv_view_proj_transform(glm::inverse(m_camera->get_projection_transform() * m_camera->get_transform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_camera_pos)) {
            shader_context->set_camera_pos(glm::value_ptr(m_camera->get_origin()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_render_mode)) {
            shader_context->set_ray_tracer_render_mode(m_ray_tracer_render_mode);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_bounce_count)) {
            shader_context->set_ray_tracer_bounce_count(m_ray_tracer_bounce_count);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_color)) {
            shader_context->set_ray_tracer_box_color(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_color.size())),
                                                                                         &m_ray_tracer_box_color[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_count)) {
            shader_context->set_ray_tracer_box_count(m_ray_tracer_box_count);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_diffuse_fuzz)) {
            shader_context->set_ray_tracer_box_diffuse_fuzz(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_diffuse_fuzz.size())),
                                                                                                &m_ray_tracer_box_diffuse_fuzz[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_eta)) {
            shader_context->set_ray_tracer_box_eta(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_eta.size())),
                                                                                       &m_ray_tracer_box_eta[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_inverse_transform)) {
            shader_context->set_ray_tracer_box_inverse_transform(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_inverse_transform.size())),
                                                                                                     &m_ray_tracer_box_inverse_transform[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_luminosity)) {
            shader_context->set_ray_tracer_box_luminosity(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_luminosity.size())),
                                                                                              &m_ray_tracer_box_luminosity[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_max)) {
            shader_context->set_ray_tracer_box_max(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_max.size())),
                                                                                       &m_ray_tracer_box_max[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_min)) {
            shader_context->set_ray_tracer_box_min(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_min.size())),
                                                                                       &m_ray_tracer_box_min[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_transform)) {
            shader_context->set_ray_tracer_box_transform(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_transform.size())),
                                                                                             &m_ray_tracer_box_transform[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_reflectance)) {
            shader_context->set_ray_tracer_box_reflectance(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_reflectance.size())),
                                                                                               &m_ray_tracer_box_reflectance[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_box_transparency)) {
            shader_context->set_ray_tracer_box_transparency(std::min(MAX_BOXES, static_cast<int>(m_ray_tracer_box_transparency.size())),
                                                                                                &m_ray_tracer_box_transparency[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_color)) {
            shader_context->set_ray_tracer_plane_color(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_color.size())),
                                                                                            &m_ray_tracer_plane_color[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_count)) {
            shader_context->set_ray_tracer_plane_count(m_ray_tracer_plane_count);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_diffuse_fuzz)) {
            shader_context->set_ray_tracer_plane_diffuse_fuzz(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_diffuse_fuzz.size())),
                                                                                                   &m_ray_tracer_plane_diffuse_fuzz[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_eta)) {
            shader_context->set_ray_tracer_plane_eta(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_eta.size())),
                                                                                          &m_ray_tracer_plane_eta[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_luminosity)) {
            shader_context->set_ray_tracer_plane_luminosity(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_luminosity.size())),
                                                                                                 &m_ray_tracer_plane_luminosity[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_normal)) {
            shader_context->set_ray_tracer_plane_normal(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_normal.size())),
                                                                                             &m_ray_tracer_plane_normal[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_point)) {
            shader_context->set_ray_tracer_plane_point(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_point.size())),
                                                                                             &m_ray_tracer_plane_point[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_reflectance)) {
            shader_context->set_ray_tracer_plane_reflectance(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_reflectance.size())),
                                                                                                  &m_ray_tracer_plane_reflectance[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_plane_transparency)) {
            shader_context->set_ray_tracer_plane_transparency(std::min(MAX_PLANES, static_cast<int>(m_ray_tracer_plane_transparency.size())),
                                                                                                   &m_ray_tracer_plane_transparency[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_random_point_count)) {
            shader_context->set_ray_tracer_random_point_count(m_ray_tracer_random_point_count);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_random_points)) {
            shader_context->set_ray_tracer_random_points(std::min(MAX_RANDOM_POINTS, static_cast<int>(m_ray_tracer_random_points.size())),
                                                                                                     &m_ray_tracer_random_points[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_random_seed)) {
            shader_context->set_ray_tracer_random_seed(m_ray_tracer_random_seed);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_color)) {
            shader_context->set_ray_tracer_sphere_color(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_color.size())),
                                                                                              &m_ray_tracer_sphere_color[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_count)) {
            shader_context->set_ray_tracer_sphere_count(m_ray_tracer_sphere_count);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_diffuse_fuzz)) {
            shader_context->set_ray_tracer_sphere_diffuse_fuzz(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_diffuse_fuzz.size())),
                                                                                                     &m_ray_tracer_sphere_diffuse_fuzz[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_eta)) {
            shader_context->set_ray_tracer_sphere_eta(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_eta.size())),
                                                                                            &m_ray_tracer_sphere_eta[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_luminosity)) {
            shader_context->set_ray_tracer_sphere_luminosity(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_luminosity.size())),
                                                                                                   &m_ray_tracer_sphere_luminosity[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_origin)) {
            shader_context->set_ray_tracer_sphere_origin(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_origin.size())),
                                                                                               &m_ray_tracer_sphere_origin[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_radius)) {
            shader_context->set_ray_tracer_sphere_radius(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_radius.size())),
                                                                                               &m_ray_tracer_sphere_radius[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_reflectance)) {
            shader_context->set_ray_tracer_sphere_reflectance(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_reflectance.size())),
                                                                                                    &m_ray_tracer_sphere_reflectance[0]);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_ray_tracer_sphere_transparency)) {
            shader_context->set_ray_tracer_sphere_transparency(std::min(MAX_SPHERES, static_cast<int>(m_ray_tracer_sphere_transparency.size())),
                                                                                                     &m_ray_tracer_sphere_transparency[0]);
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
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_normal_transform)) {
            shader_context->set_inv_normal_transform(glm::inverse(m_camera->get_normal_transform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_projection_transform)) {
            shader_context->set_inv_projection_transform(glm::inverse(m_camera->get_projection_transform()));
        }
        shader_context->render();
    }
    FrameBuffer* frame_buffer = m_camera->get_frame_buffer();
    Texture* texture = NULL;
    if(frame_buffer) {
        texture = frame_buffer->get_texture();
    }
    for(meshes_t::const_iterator q = m_meshes.begin(); q != m_meshes.end(); ++q) {
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
        glm::mat4 vp_transform = m_camera->get_projection_transform() * m_camera->get_transform();
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
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_normal_transform)) {
            shader_context->set_inv_normal_transform(glm::inverse(m_camera->get_normal_transform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_projection_transform)) {
            shader_context->set_inv_projection_transform(glm::inverse(m_camera->get_projection_transform()));
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_inv_view_proj_transform)) {
            shader_context->set_inv_view_proj_transform(glm::inverse(m_camera->get_projection_transform() * m_camera->get_transform()));
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
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_model_transform)) {
            shader_context->set_model_transform(mesh->get_transform());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_mvp_transform)) {
            shader_context->set_mvp_transform(vp_transform * mesh->get_transform());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_normal_transform)) {
            shader_context->set_normal_transform(mesh->get_normal_transform());
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
            shader_context->set_color_texture_index(mesh->get_color_texture_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_color_texture2)) {
            shader_context->set_color_texture2_index(m_overlay->get_color_texture2_index());
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_view_proj_transform)) {
            shader_context->set_view_proj_transform(vp_transform);
        }
        if(program->has_var(Program::VAR_TYPE_UNIFORM, Program::var_uniform_type_viewport_dim)) {
            if(texture) {
                int dim[2];
                dim[0] = texture->get_dim().x;
                dim[1] = texture->get_dim().y;
                shader_context->set_viewport_dim(reinterpret_cast<GLint*>(dim));
            } else {
                shader_context->set_viewport_dim(glm::value_ptr(m_camera->get_dim()));
            }
        }
        shader_context->render();
    }
}

void Scene::render_lines_and_text(bool  _draw_guide_wires,
                                  bool  _draw_paths,
                                  bool  _draw_axis,
                                  bool  _draw_axis_labels,
                                  bool  _draw_bbox,
                                  bool  _draw_normals,
                                  bool  _draw_hud_text,
                                  char* hud_text) const
{
    glDisable(GL_DEPTH_TEST);

    glUseProgram(0);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_transform()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    if(_draw_guide_wires) {
        draw_targets();
    }

    if(_draw_bbox && m_octree) {
        draw_octree(m_octree, m_camera->get_transform());
    }

    if(_draw_paths) {
        draw_paths();
    }

    for(meshes_t::const_iterator p = m_meshes.begin(); p != m_meshes.end(); ++p) {
        if(!(*p)->is_visible()) {
            continue;
        }

        if(_draw_guide_wires) {
            draw_debug_lines(*p);
        }

        if(_draw_guide_wires && ((*p)->get_parent() || (*p)->get_children().size())) {
            draw_up_vector(*p);
        }

        if(_draw_guide_wires && (*p)->get_parent()) {
            draw_ik_guide_wires(*p);
        }

        if(_draw_axis) {
            draw_axis(*p);
        }

        if(_draw_guide_wires && (*p)->is_hinge()) {
            draw_hinge_constraints(*p);
        }

        if(_draw_bbox) {
            draw_bbox(*p);
        }

        if(_draw_normals) {
            draw_normals(*p);
        }

        if(_draw_axis_labels) {
            draw_axis_labels(*p);
        }
    }

    glPopMatrix();

    if(_draw_hud_text) {
        draw_hud(hud_text);
    }

    glEnable(GL_DEPTH_TEST);
}

void Scene::draw_targets() const
{
    if(m_debug_targets.empty()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);

    for(std::vector<std::tuple<glm::vec3, glm::vec3, float, float>>::const_iterator q = m_debug_targets.begin(); q != m_debug_targets.end(); ++q) {
        glm::vec3 origin    = std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(*q);
        glm::vec3 color     = std::get<vt::Scene::DEBUG_TARGET_COLOR>(*q);
        float     radius    = std::get<vt::Scene::DEBUG_TARGET_RADIUS>(*q);
        float     linewidth = std::get<vt::Scene::DEBUG_TARGET_LINEWIDTH>(*q);

        glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * glm::translate(glm::mat4(1), origin)));
        glLineWidth(linewidth);
        glBegin(GL_LINES);

        // any color
        glColor3f(color.r, color.g, color.b);
        glutWireSphere(TARGETS_RADIUS * radius, 4, 2);

        glEnd();
        glLineWidth(1);
    }

    glDisable(GL_DEPTH_TEST);
}

void Scene::draw_octree(Octree* node, glm::mat4 camera_transform) const
{
    const float bbox_line_width = 1;

    glEnable(GL_DEPTH_TEST);

    // points
    //
    //     y
    //     4-------7
    //    /|      /|
    //   / |     / |
    //  5-------6  |
    //  |  0----|--3 x
    //  | /     | /
    //  |/      |/
    //  1-------2
    // z

    glm::vec3 points[8];
#ifdef OCTREE_MARGIN
    glm::vec3 origin = node->get_origin() + node->get_dim() * OCTREE_MARGIN;
    glm::vec3 dim    = node->get_dim()    - node->get_dim() * OCTREE_MARGIN * 2.0f;
#else
    glm::vec3 origin = node->get_origin();
    glm::vec3 dim    = node->get_dim();
#endif
    vt::PrimitiveFactory::get_box_corners(points, &origin, &dim);

    glLoadMatrixf(glm::value_ptr(camera_transform));
    glLineWidth(bbox_line_width);
    glBegin(GL_LINES);

    if(node->is_leaf()) {
        glColor3f(0, 1, 0);
    } else {
        glColor3f(1, 0, 0);
    }

    // bottom quad
    draw_edge(points[0], points[1]);
    draw_edge(points[1], points[2]);
    draw_edge(points[2], points[3]);
    draw_edge(points[3], points[0]);

    // top quad
    draw_edge(points[4], points[5]);
    draw_edge(points[5], points[6]);
    draw_edge(points[6], points[7]);
    draw_edge(points[7], points[4]);

    // bottom to top segments
    draw_edge(points[0], points[4]);
    draw_edge(points[1], points[5]);
    draw_edge(points[2], points[6]);
    draw_edge(points[3], points[7]);

    glEnd();
    glLineWidth(1);

    if(node->get_depth() <= OCTREE_RENDER_LABEL_LEVELS) {
        glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * glm::translate(glm::mat4(1), node->get_origin())));
        std::string octree_node_label = node->get_name();
        glColor3f(1, 1, 1);
        glRasterPos2f(0, 0);
        print_bitmap_string(GLUT_BITMAP_HELVETICA_18, octree_node_label.c_str());
    }

    glDisable(GL_DEPTH_TEST);

    for(int i = 0; i < 8; i++) {
        Octree* child_node = node->get_node(i);
        if(!child_node) {
            continue;
        }
        draw_octree(child_node, camera_transform);
    }
}

void Scene::draw_paths() const
{
    const float path_width = 1;

    glEnable(GL_DEPTH_TEST);

    glLineWidth(path_width);
    glBegin(GL_LINES);

    for(std::map<long, DebugObjectContext>::const_iterator p = m_debug_object_context.begin(); p != m_debug_object_context.end(); ++p) {
        glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p).second.m_transform));
        glLineWidth(path_width);
        glBegin(GL_LINES);

        for(std::vector<glm::vec3>::const_iterator q = (*p).second.m_debug_origin_keyframe_values.begin(); q != (*p).second.m_debug_origin_keyframe_values.end(); ++q) {
            glm::vec3 p1 = *q++;
            glm::vec3 p2 = *q++;
            glm::vec3 p3 = *q;

            // yellow
            glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p).second.m_transform * glm::translate(glm::mat4(1), p1)));
            glColor3f(1, 1, 0);
            glutWireSphere(TARGETS_RADIUS, 4, 2);

            // cyan
            glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p).second.m_transform * glm::translate(glm::mat4(1), p2)));
            glColor3f(0, 1, 1);
            glutWireSphere(TARGETS_RADIUS, 4, 2);

            // yellow
            glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p).second.m_transform * glm::translate(glm::mat4(1), p3)));
            glColor3f(1, 1, 0);
            glutWireSphere(TARGETS_RADIUS, 4, 2);
        }

        glEnd();
        glLineWidth(1);

        for(std::vector<glm::vec3>::const_iterator r = (*p).second.m_debug_origin_keyframe_values.begin(); r != (*p).second.m_debug_origin_keyframe_values.end(); ++r) {
            glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p).second.m_transform));
            glLineWidth(path_width);
            glBegin(GL_LINES);

            glm::vec3 p1 = *r++;
            glm::vec3 p2 = *r++;
            glm::vec3 p3 = *r;

            // orange
            glColor3f(1, 0.66, 0);
            glVertex3fv(&p2.x);
            glVertex3fv(&p1.x);

            // orange
            glColor3f(1, 0.66, 0);
            glVertex3fv(&p2.x);
            glVertex3fv(&p3.x);

            glEnd();
            glLineWidth(1);
        }

        for(std::vector<glm::vec3>::const_iterator t = (*p).second.m_debug_origin_frame_values.begin(); t != (*p).second.m_debug_origin_frame_values.end(); ++t) {
            if(t == --(*p).second.m_debug_origin_frame_values.end()) {
                break;
            }

            glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p).second.m_transform));
            glLineWidth(path_width);
            glBegin(GL_LINES);

            glm::vec3 p1 = *t;
            glm::vec3 p2 = *(t + 1);

            // yellow
            glColor3f(1, 1, 0);
            glVertex3fv(&p1.x);
            glVertex3fv(&p2.x);

            glEnd();
            glLineWidth(1);
        }
    }

    glEnd();
    glLineWidth(1);

    glDisable(GL_DEPTH_TEST);
}

void Scene::draw_debug_lines(Mesh* mesh) const
{
    glEnable(GL_DEPTH_TEST);

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform()));

    for(std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3, float>>::const_iterator p = mesh->m_debug_lines.begin(); p != mesh->m_debug_lines.end(); ++p) {
        glm::vec3 p1        = std::get<vt::Scene::DEBUG_LINE_P1>(*p);
        glm::vec3 p2        = std::get<vt::Scene::DEBUG_LINE_P2>(*p);
        glm::vec3 color     = std::get<vt::Scene::DEBUG_LINE_COLOR>(*p);
        float     linewidth = std::get<vt::Scene::DEBUG_LINE_LINEWIDTH>(*p);

        glLineWidth(linewidth);
        glBegin(GL_LINES);

        // any color
        glColor3f(color.r, color.g, color.b);
        glVertex3fv(&p1.x);
        glVertex3fv(&p2.x);

        glEnd();
        glLineWidth(1);
    }

    for(std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3, float>>::const_iterator q = m_debug_lines.begin(); q != m_debug_lines.end(); ++q) {
        glm::vec3 p1        = std::get<vt::Scene::DEBUG_LINE_P1>(*q);
        glm::vec3 p2        = std::get<vt::Scene::DEBUG_LINE_P2>(*q);
        glm::vec3 color     = std::get<vt::Scene::DEBUG_LINE_COLOR>(*q);
        float     linewidth = std::get<vt::Scene::DEBUG_LINE_LINEWIDTH>(*q);

        glLineWidth(linewidth);
        glBegin(GL_LINES);

        // any color
        glColor3f(color.r, color.g, color.b);
        glVertex3fv(&p1.x);
        glVertex3fv(&p2.x);

        glEnd();
        glLineWidth(1);
    }

    glDisable(GL_DEPTH_TEST);
}

void Scene::draw_up_vector(Mesh* mesh) const
{
    const float up_arm_length    = 0.5; // white
    const float guide_wire_width = 1;

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform()));
    glLineWidth(guide_wire_width);
    glBegin(GL_LINES);

    // white
    glColor3f(1, 1, 1);
    glm::vec3 abs_origin = mesh->in_abs_system();
    glVertex3fv(&abs_origin.x);
    glm::vec3 endpoint(mesh->get_transform() * glm::vec4(VEC_UP * up_arm_length, 1));
    glVertex3fv(&endpoint.x);

    glEnd();
    glLineWidth(1);
}

void Scene::draw_ik_guide_wires(Mesh* mesh) const
{
    const float up_arm_length               = 0.5;                // white
    const float local_pivot_length          = 1  * up_arm_length; // magenta
    const float end_effector_tip_dir_length = 10 * up_arm_length; // yellow
    const float target_dir_length           = 10 * up_arm_length; // cyan
    const float guide_wire_width            = 1;

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * mesh->get_parent()->get_transform()));
    glLineWidth(guide_wire_width);
    glBegin(GL_LINES);

    {
        // magenta
        glColor3f(1, 0, 1);
        glm::vec3 origin(glm::vec4(mesh->get_origin(), 1));
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(glm::vec4(mesh->get_origin() + mesh->m_debug_local_pivot * local_pivot_length, 1));
        glVertex3fv(&endpoint.x);
    }

    {
        // yellow
        glColor3f(1, 1, 0);
        glm::vec3 origin(glm::vec4(mesh->get_origin(), 1));
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(glm::vec4(mesh->get_origin() + mesh->m_debug_end_effector_tip_dir * end_effector_tip_dir_length, 1));
        glVertex3fv(&endpoint.x);
    }

    {
        // cyan
        glColor3f(0, 1, 1);
        glm::vec3 origin(glm::vec4(mesh->get_origin(), 1));
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(glm::vec4(mesh->get_origin() + mesh->m_debug_target_dir * target_dir_length, 1));
        glVertex3fv(&endpoint.x);
    }

    {
        // blue
        glColor3f(0, 0, 1);
        glm::vec3 origin(glm::vec4(mesh->get_origin(), 1));
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(glm::vec4(mesh->get_origin() + mesh->m_debug_local_target, 1));
        glVertex3fv(&endpoint.x);
    }

    glEnd();
    glLineWidth(1);
}

void Scene::draw_axis(Mesh* mesh) const
{
    const float axis_arm_length = 0.25;
    const float axis_line_width = 3;

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * mesh->get_transform()));
    glLineWidth(axis_line_width);
    glBegin(GL_LINES);

    // x-axis
    {
        // red
        glColor3f(1, 0, 0);
        glm::vec3 origin(0, 0, 0);
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(axis_arm_length, 0, 0);
        glVertex3fv(&endpoint.x);
    }

    // y-axis
    {
        // green
        glColor3f(0, 1, 0);
        glm::vec3 origin(0, 0, 0);
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(0, axis_arm_length, 0);
        glVertex3fv(&endpoint.x);
    }

    // z-axis
    {
        // blue
        glColor3f(0, 0, 1);
        glm::vec3 origin(0, 0, 0);
        glVertex3fv(&origin.x);
        glm::vec3 endpoint(0, 0, axis_arm_length);
        glVertex3fv(&endpoint.x);
    }

    glEnd();
    glLineWidth(1);
}

void Scene::draw_hinge_constraints(Mesh* mesh) const
{
    const float guide_wire_width = 1;

    glm::mat4 parent_transform;
    TransformObject* parent = mesh->get_parent();
    if(parent) {
        parent_transform = parent->get_transform();
    } else {
        parent_transform = glm::mat4(1);
    }

    euler_index_t hinge_type = mesh->get_hinge_type();

    glm::mat4 euler_transform_sans_hinge_rotate;
    glm::vec3 euler = mesh->get_euler();
    switch(hinge_type) {
        case vt::EULER_INDEX_ROLL:
            euler_transform_sans_hinge_rotate = GLM_EULER_TRANSFORM(EULER_YAW(euler), EULER_PITCH(euler), 0.0f);
            break;
        case vt::EULER_INDEX_PITCH:
            euler_transform_sans_hinge_rotate = GLM_EULER_TRANSFORM(EULER_YAW(euler), 0.0f, EULER_ROLL(euler));
            break;
        case vt::EULER_INDEX_YAW:
            euler_transform_sans_hinge_rotate = GLM_EULER_TRANSFORM(0.0f, EULER_PITCH(euler), EULER_ROLL(euler));
            break;
        default:
            break;
    }

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * euler_transform_sans_hinge_rotate * parent_transform));
    glLineWidth(guide_wire_width);
    glBegin(GL_LINES);

    {
        // yellow
        glColor3f(1, 1, 0);

        glm::vec3 joint_constraints_center        = mesh->get_joint_constraints_center();
        glm::vec3 joint_constraints_max_deviation = mesh->get_joint_constraints_max_deviation();
        float min_value    = joint_constraints_center[hinge_type] - joint_constraints_max_deviation[hinge_type];
        float center_value = joint_constraints_center[hinge_type];
        float max_value    = joint_constraints_center[hinge_type] + joint_constraints_max_deviation[hinge_type];

        glm::vec3 swipe_vec;
        glm::vec3 rotate_axis;
        glm::vec3 dim        = mesh->get_dim();
        float     arc_radius = 0;
        switch(hinge_type) {
            case vt::EULER_INDEX_ROLL:
                swipe_vec   = VEC_UP;
                rotate_axis = VEC_FORWARD;
                arc_radius  = std::min(dim.x * 0.5f, 1.0f);
                break;
            case vt::EULER_INDEX_PITCH:
                swipe_vec   = VEC_FORWARD;
                rotate_axis = VEC_LEFT;
                arc_radius  = dim.z;
                break;
            case vt::EULER_INDEX_YAW:
                swipe_vec   = VEC_FORWARD;
                rotate_axis = VEC_UP;
                arc_radius  = dim.z;
                break;
            default:
                break;
        }
        swipe_vec *= CONSTRAINT_SWIPE_RADIUS;

        // min edge
        {
            glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), min_value, rotate_axis);
            glm::vec3 local_heading(local_arc_rotation_transform * glm::vec4(swipe_vec * arc_radius, 1));
            glm::vec3 origin(mesh->get_origin());
            glVertex3fv(&origin.x);
            glm::vec3 endpoint = origin + local_heading;
            glVertex3fv(&endpoint.x);
        }

        // center edge
        {
            glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), center_value, rotate_axis);
            glm::vec3 local_heading(local_arc_rotation_transform * glm::vec4(swipe_vec * arc_radius, 1));
            glm::vec3 origin(mesh->get_origin());
            glVertex3fv(&origin.x);
            glm::vec3 endpoint = origin + local_heading;
            glVertex3fv(&endpoint.x);
        }

        // max edge
        {
            glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), max_value, rotate_axis);
            glm::vec3 local_heading(local_arc_rotation_transform * glm::vec4(swipe_vec * arc_radius, 1));
            glm::vec3 origin(mesh->get_origin());
            glVertex3fv(&origin.x);
            glm::vec3 endpoint = origin + local_heading;
            glVertex3fv(&endpoint.x);
        }

        // arc rim
        {
            glm::vec3 prev_endpoint;
            for(float angle = min_value; angle < max_value; angle += CONSTRAINT_SWIPE_STEP_ANGLE) {
                glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), angle, rotate_axis);
                glm::vec3 local_heading(local_arc_rotation_transform * glm::vec4(swipe_vec * arc_radius, 1));
                glm::vec3 v(mesh->get_origin() + local_heading);
                if(angle != min_value) {
                    glVertex3fv(&prev_endpoint.x);
                    glVertex3fv(&v.x);
                }
                prev_endpoint = v;
            }
            glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), max_value, rotate_axis);
            glm::vec3 local_heading(local_arc_rotation_transform * glm::vec4(swipe_vec * arc_radius, 1));
            glm::vec3 v(mesh->get_origin() + local_heading);
            glVertex3fv(&prev_endpoint.x);
            glVertex3fv(&v.x);
        }
    }

    glEnd();
    glLineWidth(1);
}

void Scene::draw_bbox(Mesh* mesh) const
{
    const float bbox_line_width         = 1;
    const float normal_surface_distance = 0.05;

    glEnable(GL_DEPTH_TEST);

    glm::vec3 min, max;
    mesh->get_min_max(&min, &max);
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

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * mesh->get_transform()));
    glLineWidth(bbox_line_width);
    glBegin(GL_LINES);

    glColor3f(1, 0, 0);

    glm::vec3 m1, m2;

    // back quad
    draw_broken_edge(llb, lrb, m1, m2);
    draw_broken_edge(lrb, urb, m1, m2);
    draw_broken_edge(urb, ulb, m1, m2);
    draw_broken_edge(ulb, llb, m1, m2);

    // front quad
    draw_broken_edge(llf, lrf, m1, m2);
    draw_broken_edge(lrf, urf, m1, m2);
    draw_broken_edge(urf, ulf, m1, m2);
    draw_broken_edge(ulf, llf, m1, m2);

    // back to front segments
    draw_broken_edge(llb, llf, m1, m2);
    draw_broken_edge(lrb, lrf, m1, m2);
    draw_broken_edge(urb, urf, m1, m2);
    draw_broken_edge(ulb, ulf, m1, m2);

    glEnd();
    glLineWidth(1);

    glDisable(GL_DEPTH_TEST);
}

void Scene::draw_normals(Mesh* mesh) const
{
    const float normal_arm_length       = 0.125;
    const float normal_line_width       = 1;
    const float normal_surface_distance = 0.05;

    glEnable(GL_DEPTH_TEST);

    glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * mesh->get_transform()));
    glLineWidth(normal_line_width);
    glBegin(GL_LINES);

    size_t num_vertex = mesh->get_num_vertex();

    // normal
    glColor3f(0, 0, 1);
    for(int i = 0; i < static_cast<int>(num_vertex); i++) {
        glm::vec3 origin = mesh->get_vert_coord(i) + mesh->get_vert_normal(i) * normal_surface_distance;
        glVertex3fv(&origin.x);
        glm::vec3 endpoint = origin + mesh->get_vert_normal(i) * normal_arm_length;
        glVertex3fv(&endpoint.x);
    }

    // tangent
    glColor3f(1, 0, 0);
    for(int j = 0; j < static_cast<int>(num_vertex); j++) {
        glm::vec3 origin = mesh->get_vert_coord(j) + mesh->get_vert_normal(j) * normal_surface_distance;
        glVertex3fv(&origin.x);
        glm::vec3 endpoint = origin + mesh->get_vert_tangent(j) * normal_arm_length;
        glVertex3fv(&endpoint.x);
    }

    // bitangent
    glColor3f(0, 1, 0);
    for(int k = 0; k < static_cast<int>(num_vertex); k++) {
        glm::vec3 origin = mesh->get_vert_coord(k) + mesh->get_vert_normal(k) * normal_surface_distance;
        glVertex3fv(&origin.x);
        glm::vec3 endpoint = origin + mesh->get_vert_bitangent(k) * normal_arm_length;
        glVertex3fv(&endpoint.x);
    }

    glEnd();
    glLineWidth(1);

    glDisable(GL_DEPTH_TEST);
}

void Scene::draw_axis_labels(Mesh* mesh) const
{
    glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * mesh->get_transform()));
    std::string axis_label;
#if 1
    axis_label = mesh->get_name();
#else
    glm::vec3 abs_origin;
    TransformObject* parent = mesh->get_parent();
    if(parent) {
        abs_origin = glm::vec3(parent->get_transform() * glm::vec4(mesh->get_origin(), 1));
    } else {
        abs_origin = mesh->get_origin();
    }
    axis_label = mesh->get_name() + ": " + glm::to_string(abs_origin);
#endif
    glColor3f(1, 1, 1);
    glRasterPos2f(0, 0);
    print_bitmap_string(GLUT_BITMAP_HELVETICA_18, axis_label.c_str());
}

void Scene::draw_hud(std::string hud_text) const
{
    glMatrixMode(GL_PROJECTION);
    Camera::projection_mode_t prev_projection_mode = m_camera->get_projection_mode();
    m_camera->set_projection_mode(Camera::PROJECTION_MODE_ORTHO);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_transform()));
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
    glLoadMatrixf(glm::value_ptr(glm::translate(glm::mat4(1), glm::vec3(-half_width, half_height, 0)) * m_camera->get_transform()));
    glColor3f(1, 1, 1);
    glRasterPos2f(0, 0);
    print_bitmap_string(GLUT_BITMAP_HELVETICA_18, hud_text.c_str());
    glPopMatrix();
}

void Scene::render_lights() const
{
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_camera->get_projection_transform()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    for(lights_t::const_iterator p = m_lights.begin(); p != m_lights.end(); ++p) {
        glLoadMatrixf(glm::value_ptr(m_camera->get_transform() * (*p)->get_transform()));
        glColor3f(1, 1, 0);
        glutWireSphere(TARGET_RADIUS, 4, 2);
    }
    glPopMatrix();
}

void Scene::clear_ray_tracer_objects()
{
    // ray tracer spheres
    m_ray_tracer_sphere_origin.clear();
    m_ray_tracer_sphere_radius.clear();
    m_ray_tracer_sphere_eta.clear();
    m_ray_tracer_sphere_count = 0;

    // ray tracer planes
    m_ray_tracer_plane_point.clear();
    m_ray_tracer_plane_normal.clear();
    m_ray_tracer_plane_eta.clear();
    m_ray_tracer_plane_count = 0;

    // ray tracer boxes
    m_ray_tracer_box_transform.clear();
    m_ray_tracer_box_inverse_transform.clear();
    m_ray_tracer_box_min.clear();
    m_ray_tracer_box_max.clear();
    m_ray_tracer_box_eta.clear();
    m_ray_tracer_box_count = 0;
}

void Scene::add_ray_tracer_sphere(glm::vec3 origin,
                                  float     radius,
                                  float     eta,
                                  float     diffuse_fuzz,
                                  glm::vec3 color,
                                  float     reflectance,
                                  float     transparency,
                                  float     luminosity)
{
    m_ray_tracer_sphere_origin.push_back(origin);
    m_ray_tracer_sphere_radius.push_back(radius);
    m_ray_tracer_sphere_eta.push_back(eta);
    m_ray_tracer_sphere_diffuse_fuzz.push_back(diffuse_fuzz);
    m_ray_tracer_sphere_color.push_back(color);
    m_ray_tracer_sphere_reflectance.push_back(reflectance);
    m_ray_tracer_sphere_transparency.push_back(transparency);
    m_ray_tracer_sphere_luminosity.push_back(luminosity);
    m_ray_tracer_sphere_count++;
}

void Scene::add_ray_tracer_plane(glm::vec3 point,
                                 glm::vec3 normal,
                                 float     eta,
                                 float     diffuse_fuzz,
                                 glm::vec3 color,
                                 float     reflectance,
                                 float     transparency,
                                 float     luminosity)
{
    m_ray_tracer_plane_point.push_back(point);
    m_ray_tracer_plane_normal.push_back(normal);
    m_ray_tracer_plane_eta.push_back(eta);
    m_ray_tracer_plane_diffuse_fuzz.push_back(diffuse_fuzz);
    m_ray_tracer_plane_color.push_back(color);
    m_ray_tracer_plane_reflectance.push_back(reflectance);
    m_ray_tracer_plane_transparency.push_back(transparency);
    m_ray_tracer_plane_luminosity.push_back(luminosity);
    m_ray_tracer_plane_count++;
}

void Scene::add_ray_tracer_box(glm::vec3 origin,
                               glm::vec3 euler,
                               glm::vec3 _min,
                               glm::vec3 _max,
                               float     eta,
                               float     diffuse_fuzz,
                               glm::vec3 color,
                               float     reflectance,
                               float     transparency,
                               float     luminosity)
{
    glm::mat4 transform = glm::translate(glm::mat4(1), origin) * GLM_EULER_TRANSFORM(EULER_YAW(euler), EULER_PITCH(euler), EULER_ROLL(euler));
    m_ray_tracer_box_transform.push_back(transform);
    m_ray_tracer_box_inverse_transform.push_back(glm::inverse(transform));
    m_ray_tracer_box_min.push_back(_min);
    m_ray_tracer_box_max.push_back(_max);
    m_ray_tracer_box_eta.push_back(eta);
    m_ray_tracer_box_diffuse_fuzz.push_back(diffuse_fuzz);
    m_ray_tracer_box_color.push_back(color);
    m_ray_tracer_box_reflectance.push_back(reflectance);
    m_ray_tracer_box_transparency.push_back(transparency);
    m_ray_tracer_box_luminosity.push_back(luminosity);
    m_ray_tracer_box_count++;
}

}
