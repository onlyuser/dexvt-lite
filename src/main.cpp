/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 * Enhanced by: Jerry Chen
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/glut.h>
/* GLM */
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <shader_utils.h>

#include "res_texture.c"
#include "res_texture2.c"

#include <Buffer.h>
#include <Camera.h>
#include <FrameBuffer.h>
#include <ShaderContext.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <PrimitiveFactory.h>
#include <File3ds.h>
#include <Program.h>
#include <Shader.h>
#include <Scene.h>
#include <Texture.h>
#include <Util.h>
#include <VarAttribute.h>
#include <VarUniform.h>
#include <vector>
#include <iostream> // std::cout
#include <sstream> // std::stringstream
#include <iomanip> // std::setprecision
#include <unistd.h> // access

#define HI_RES_TEX_DIM  512
#define MED_RES_TEX_DIM 256
#define LO_RES_TEX_DIM  128
#define BLUR_ITERS      5
#define RAND_TEX_DIM    8

enum demo_mode_t {
    DEMO_MODE_DEFAULT,
    DEMO_MODE_DIAMOND,
    DEMO_MODE_SPHERE,
    DEMO_MODE_BOX,
    DEMO_MODE_GRID,
    DEMO_MODE_MESHES_IMPORTED,
    DEMO_MODE_COUNT
};

enum overlay_mode_t {
    OVERLAY_MODE_DEFAULT,
    OVERLAY_MODE_FF_DEPTH,
    OVERLAY_MODE_BF_DEPTH,
    OVERLAY_MODE_FF_NORMAL,
    OVERLAY_MODE_BF_NORMAL,
    OVERLAY_MODE_SSAO,
    OVERLAY_MODE_COUNT
};

const char* DEFAULT_CAPTION = "My Textured Cube";

int init_screen_width = 800, init_screen_height = 600;
vt::Camera* camera;
vt::Mesh *mesh_skybox, *mesh_overlay, *mesh, *mesh2, *mesh3, *mesh4, *mesh5, *mesh6, *mesh7, *mesh8, *mesh9, *mesh10, *hidden_mesh, *hidden_mesh2, *hidden_mesh3, *hidden_mesh4;
std::vector<vt::Mesh*> meshes_imported;
vt::Light *light, *light2, *light3;
vt::Texture *texture, *texture2, *texture3, *texture4, *texture5, *frontface_depth_overlay_texture, *backface_depth_overlay_texture, *frontface_normal_overlay_texture, *backface_normal_overlay_texture, *ssao_overlay_texture, *hi_res_color_overlay_texture, *med_res_color_overlay_texture, *lo_res_color_overlay_texture, *random_texture;

vt::FrameBuffer *frontface_depth_overlay_fb, *backface_depth_overlay_fb, *frontface_normal_overlay_fb, *backface_normal_overlay_fb, *ssao_overlay_fb, *hi_res_color_overlay_fb, *med_res_color_overlay_fb, *lo_res_color_overlay_fb;

bool left_mouse_down = false, right_mouse_down = false;
glm::vec2 prev_mouse_coord, mouse_drag;
glm::vec3 prev_orient, orient, orbit_speed = glm::vec3(0, -0.5, -0.5);
float prev_orbit_radius = 0, orbit_radius = 8, dolly_speed = 0.1, light_distance = 4;
bool wireframe_mode = false;
bool show_fps = false;
bool show_axis = false;
bool show_bbox = false;
bool show_normals = false;
bool show_lights = false;
bool show_diamond = false;
bool post_process_blur = false;

int texture_id = 0;
float prev_zoom = 0, zoom = 1, ortho_dolly_speed = 0.1;

int overlay_mode = OVERLAY_MODE_DEFAULT;
int demo_mode = DEMO_MODE_DEFAULT;

float phase = 0;

vt::Material* overlay_write_through_material;
vt::Material* overlay_bloom_filter_material;
vt::Material* overlay_max_material;

int init_resources()
{
    vt::Scene* scene = vt::Scene::instance();

    mesh_skybox = vt::PrimitiveFactory::create_viewport_quad("grid");
    scene->set_skybox(mesh_skybox);

    mesh_overlay = vt::PrimitiveFactory::create_viewport_quad("grid3");
    scene->set_overlay(mesh_overlay);

    scene->add_mesh(mesh         = vt::PrimitiveFactory::create_box(                  "box"));
    scene->add_mesh(mesh2        = vt::PrimitiveFactory::create_grid(                 "grid",       1,  1,   10, 10, 0.05, 0.05));
    scene->add_mesh(mesh3        = vt::PrimitiveFactory::create_sphere(               "sphere",     16, 16,  0.5));
    scene->add_mesh(mesh4        = vt::PrimitiveFactory::create_torus(                "torus",      16, 16,  0.5, 0.25));
    scene->add_mesh(mesh5        = vt::PrimitiveFactory::create_cylinder(             "cylinder",   16, 0.5, 1));
    scene->add_mesh(mesh6        = vt::PrimitiveFactory::create_cone(                 "cone",       16, 0.5, 1));
    scene->add_mesh(mesh7        = vt::PrimitiveFactory::create_hemisphere(           "hemisphere", 16, 16,  0.5));
    scene->add_mesh(mesh8        = vt::PrimitiveFactory::create_tetrahedron(          "tetrahedron"));
    scene->add_mesh(mesh9        = vt::PrimitiveFactory::create_diamond_brilliant_cut("diamond"));
    scene->add_mesh(mesh10       = vt::PrimitiveFactory::create_box(                  "box2"));
    scene->add_mesh(hidden_mesh  = vt::PrimitiveFactory::create_diamond_brilliant_cut("diamond2"));
    scene->add_mesh(hidden_mesh2 = vt::PrimitiveFactory::create_sphere(               "sphere2", 16, 16, 0.5));
    scene->add_mesh(hidden_mesh3 = vt::PrimitiveFactory::create_box(                  "box3"));
    scene->add_mesh(hidden_mesh4 = vt::PrimitiveFactory::create_grid(                 "grid2",   32, 32, 1, 1));
    const char* model_filename = "data/star_wars/TI_Low0.3ds";
    if(access(model_filename, F_OK) != -1) {
        vt::File3ds::load3ds(model_filename, -1, &meshes_imported);
    }
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        scene->add_mesh(*p);
    }

    mesh->set_origin(        glm::vec3(-0.5, -0.5, -0.5)); // box
    mesh2->set_origin(       glm::vec3(-5, 5, -1));        // grid
    mesh3->set_origin(       glm::vec3(2, 0, 0));          // sphere
    mesh4->set_origin(       glm::vec3(-2, 0, 0));         // torus
    mesh5->set_origin(       glm::vec3(0, -2.5, 0));       // cylinder
    mesh6->set_origin(       glm::vec3(0, 1.5, 0));        // cone
    mesh7->set_origin(       glm::vec3(-2, 1.75, 0));      // hemisphere
    mesh8->set_origin(       glm::vec3(1.5, 1.5, -0.5));   // tetrahedron
    mesh9->set_origin(       glm::vec3(-2, -2.5, 0));      // diamond
    mesh10->set_origin(      glm::vec3(1.5, -2.5, -0.5));  // box2
    hidden_mesh->set_origin( glm::vec3(0, -1, 0));         // diamond2
    hidden_mesh2->set_origin(glm::vec3(0, 0, 0));          // sphere2
    hidden_mesh3->set_origin(glm::vec3(-1, -1, -1));       // box3
    hidden_mesh4->set_origin(glm::vec3(-2, 0, -2));        // grid2
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        (*p)->set_origin(glm::vec3(0, 0, 0));
    }

    mesh2->set_orient(glm::vec3(0, -90, 0));

    mesh2->set_visible(false);
    //mesh5->set_visible(false);
    //mesh6->set_visible(false);
    hidden_mesh->set_visible(false);
    hidden_mesh2->set_visible(false);
    hidden_mesh3->set_visible(false);
    hidden_mesh4->set_visible(false);
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        (*p)->set_visible(false);
    }

    hidden_mesh->set_scale(glm::vec3( 2, 2, 2)); // diamond2
    hidden_mesh2->set_scale(glm::vec3(2, 2, 2)); // sphere2
    hidden_mesh3->set_scale(glm::vec3(2, 2, 2)); // box3
    hidden_mesh4->set_scale(glm::vec3(4, 4, 4)); // grid2
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        (*p)->set_scale(glm::vec3(0.1, 0.1, 0.1));
    }

    vt::Material* bump_mapped_material = new vt::Material(
            "bump_mapped",
            "src/shaders/bump_mapped.v.glsl",
            "src/shaders/bump_mapped.f.glsl",
            true,   // use_ambient_color
            false,  // gen_normal_map
            true,   // use_phong_shading
            true,   // use_texture_mapping
            true,   // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* bump_mapped_program = bump_mapped_material->get_program();
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "texcoord");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_tangent");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "ambient_color");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "bump_texture");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_pos");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "color_texture");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_color");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_count");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_enabled");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_pos");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "model_xform");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    bump_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    scene->add_material(bump_mapped_material);

    vt::Material* phong_material = new vt::Material(
            "phong",
            "src/shaders/phong.v.glsl",
            "src/shaders/phong.f.glsl",
            true,   // use_ambient_color
            false,  // gen_normal_map
            true,   // use_phong_shading
            false,  // use_texture_mapping
            false,  // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* phong_program = phong_material->get_program();
    phong_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    phong_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "ambient_color");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_pos");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_color");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_count");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_enabled");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_pos");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "model_xform");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    phong_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    scene->add_material(phong_material);

    vt::Material* ssao_material = new vt::Material(
            "ssao",
            "src/shaders/ssao.v.glsl",
            "src/shaders/ssao.f.glsl",
            false,  // use_ambient_color
            false,  // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            false,  // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            true,   // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            true,   // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* ssao_program = ssao_material->get_program();
    ssao_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "texcoord");
    ssao_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    ssao_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_dir");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_far");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_near");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_pos");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "frontface_depth_overlay_texture");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "inv_view_proj_xform");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "random_texture");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "ssao_sample_kernel_pos");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "viewport_dim");
    ssao_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "view_proj_xform");
    scene->add_material(ssao_material);
    scene->set_ssao_material(ssao_material);

    vt::Material* skybox_material = new vt::Material(
            "skybox",
            "src/shaders/skybox.v.glsl",
            "src/shaders/skybox.f.glsl",
            false,  // use_ambient_color
            false,  // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            false,  // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            true,   // skybox
            false); // overlay
    vt::Program* skybox_material_program = skybox_material->get_program();
    skybox_material_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "env_map_texture");
    skybox_material_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "inv_normal_xform");
    skybox_material_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "inv_projection_xform");
    scene->add_material(skybox_material);

    overlay_write_through_material = new vt::Material(
            "overlay_write_through",
            "src/shaders/overlay_write_through.v.glsl",
            "src/shaders/overlay_write_through.f.glsl",
            false, // use_ambient_color
            false, // gen_normal_map
            false, // use_phong_shading
            false, // use_texture_mapping
            false, // use_bump_mapping
            false, // use_env_mapping
            false, // use_env_mapping_dbl_refract
            false, // use_ssao
            false, // use_bloom_kernel
            false, // use_texture2
            false, // use_fragment_world_pos
            false, // skybox
            true); // overlay
    vt::Program* overlay_write_through_program = overlay_write_through_material->get_program();
    overlay_write_through_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "color_texture");
    scene->add_material(overlay_write_through_material);

    overlay_bloom_filter_material = new vt::Material(
            "overlay_bloom_filter",
            "src/shaders/overlay_bloom_filter.v.glsl",
            "src/shaders/overlay_bloom_filter.f.glsl",
            false, // use_ambient_color
            false, // gen_normal_map
            false, // use_phong_shading
            false, // use_texture_mapping
            false, // use_bump_mapping
            false, // use_env_mapping
            false, // use_env_mapping_dbl_refract
            false, // use_ssao
            true,  // use_bloom_kernel
            false, // use_texture2
            false, // use_fragment_world_pos
            false, // skybox
            true); // overlay
    vt::Program* overlay_bloom_filter_program = overlay_bloom_filter_material->get_program();
    overlay_bloom_filter_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "bloom_kernel");
    overlay_bloom_filter_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "color_texture");
    overlay_bloom_filter_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "glow_cutoff_threshold");
    overlay_bloom_filter_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "viewport_dim");
    scene->add_material(overlay_bloom_filter_material);

    overlay_max_material = new vt::Material(
            "overlay_max",
            "src/shaders/overlay_max.v.glsl",
            "src/shaders/overlay_max.f.glsl",
            false, // use_ambient_color
            false, // gen_normal_map
            false, // use_phong_shading
            false, // use_texture_mapping
            false, // use_bump_mapping
            false, // use_env_mapping
            false, // use_env_mapping_dbl_refract
            false, // use_ssao
            false, // use_bloom_kernel
            true,  // use_texture2
            false, // use_fragment_world_pos
            false, // skybox
            true); // overlay
    vt::Program* overlay_max_program = overlay_max_material->get_program();
    overlay_max_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "color_texture");
    overlay_max_program->add_var(vt::Program::VAR_TYPE_UNIFORM, "color_texture2");
    scene->add_material(overlay_max_material);

    vt::Material* texture_mapped_material = new vt::Material(
            "texture_mapped",
            "src/shaders/texture_mapped.v.glsl",
            "src/shaders/texture_mapped.f.glsl",
            false,  // use_ambient_color
            false,  // gen_normal_map
            false,  // use_phong_shading
            true,   // use_texture_mapping
            false,  // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* texture_mapped_program = texture_mapped_material->get_program();
    texture_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "color_texture");
    texture_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    texture_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "texcoord");
    texture_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    scene->add_material(texture_mapped_material);

    vt::Material* env_mapped_material = new vt::Material(
            "env_mapped",
            "src/shaders/env_mapped.v.glsl",
            "src/shaders/env_mapped.f.glsl",
            false,  // use_ambient_color
            false,  // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            true,   // use_bump_mapping
            true,   // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* env_mapped_program = env_mapped_material->get_program();
    env_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "texcoord");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_tangent");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "bump_texture");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_pos");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "env_map_texture");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "model_xform");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    env_mapped_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "reflect_to_refract_ratio");
    scene->add_material(env_mapped_material);

    vt::Material* env_mapped_dbl_refract_material = new vt::Material(
            "env_mapped_dbl_refract",
            "src/shaders/env_mapped_dbl_refract.v.glsl",
            "src/shaders/env_mapped_dbl_refract.f.glsl",
            false,  // use_ambient_color
            false,  // gen_normal_map
            true,   // use_phong_shading
            false,  // use_texture_mapping
            true,   // use_bump_mapping
            true,   // use_env_mapping
            true,   // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            true,   // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* env_mapped_dbl_refract_program = env_mapped_dbl_refract_material->get_program();
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "texcoord");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_tangent");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "backface_depth_overlay_texture");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "backface_normal_overlay_texture");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "bump_texture");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_far");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_near");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_pos");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "env_map_texture");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "frontface_depth_overlay_texture");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "inv_view_proj_xform");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_color");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_count");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_enabled");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "light_pos");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "model_xform");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "reflect_to_refract_ratio");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "viewport_dim");
    env_mapped_dbl_refract_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "view_proj_xform");
    scene->add_material(env_mapped_dbl_refract_material);

    vt::Material* env_mapped_fast_material = new vt::Material(
            "env_mapped_fast",
            "src/shaders/env_mapped_fast.v.glsl",
            "src/shaders/env_mapped_fast.f.glsl",
            false,  // use_ambient_color
            false,  // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            false,  // use_bump_mapping
            true,   // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* env_mapped_fast_program = env_mapped_fast_material->get_program();
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "camera_pos");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "env_map_texture");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "model_xform");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    env_mapped_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "reflect_to_refract_ratio");
    scene->add_material(env_mapped_fast_material);

    vt::Material* normal_material = new vt::Material(
            "normal",
            "src/shaders/normal.v.glsl",
            "src/shaders/normal.f.glsl",
            false,  // use_ambient_color
            true,   // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            true,   // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* normal_program = normal_material->get_program();
    normal_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "texcoord");
    normal_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "bump_texture");
    normal_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    normal_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    normal_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_tangent");
    normal_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    normal_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    scene->add_material(normal_material);

    vt::Material* normal_fast_material = new vt::Material(
            "normal_fast",
            "src/shaders/normal_fast.v.glsl",
            "src/shaders/normal_fast.f.glsl",
            false,  // use_ambient_color
            true,   // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            false,  // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* normal_fast_program = normal_fast_material->get_program();
    normal_fast_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_normal");
    normal_fast_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    normal_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    normal_fast_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "normal_xform");
    scene->add_material(normal_fast_material);
    scene->set_normal_material(normal_fast_material);

    vt::Material* ambient_material = new vt::Material(
            "ambient",
            "src/shaders/ambient.v.glsl",
            "src/shaders/ambient.f.glsl",
            true,   // use_ambient_color
            false,  // gen_normal_map
            false,  // use_phong_shading
            false,  // use_texture_mapping
            false,  // use_bump_mapping
            false,  // use_env_mapping
            false,  // use_env_mapping_dbl_refract
            false,  // use_ssao
            false,  // use_bloom_kernel
            false,  // use_texture2
            false,  // use_fragment_world_pos
            false,  // skybox
            false); // overlay
    vt::Program* ambient_program = ambient_material->get_program();
    ambient_program->add_var(vt::Program::VAR_TYPE_ATTRIBUTE, "vertex_position");
    ambient_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "ambient_color");
    ambient_program->add_var(vt::Program::VAR_TYPE_UNIFORM,   "mvp_xform");
    scene->add_material(ambient_material);
    scene->set_wireframe_material(ambient_material);

    texture = new vt::Texture(
            "dex3d",
            res_texture.width,
            res_texture.height,
            res_texture.pixel_data,
            vt::Texture::RGB,
            false);
    scene->add_texture(                  texture);
    bump_mapped_material->add_texture(   texture);
    texture_mapped_material->add_texture(texture);

    texture2 = new vt::Texture(
            "lode_runner",
            res_texture2.width,
            res_texture2.height,
            res_texture2.pixel_data);
    scene->add_texture(               texture2);
    bump_mapped_material->add_texture(texture2);

    texture3 = new vt::Texture(
            "chesterfield_color",
            "data/chesterfield_color.png");
    scene->add_texture(                          texture3);
    bump_mapped_material->add_texture(           texture3);
    env_mapped_material->add_texture(            texture3);
    env_mapped_dbl_refract_material->add_texture(texture3);

    texture4 = new vt::Texture(
            "chesterfield_normal",
            "data/chesterfield_normal.png");
    scene->add_texture(                          texture4);
    bump_mapped_material->add_texture(           texture4);
    env_mapped_material->add_texture(            texture4);
    env_mapped_dbl_refract_material->add_texture(texture4);
    normal_material->add_texture(                texture4);

    texture5 = new vt::Texture(
            "skybox_texture",
            "data/SaintPetersSquare2/posx.png",
            "data/SaintPetersSquare2/negx.png",
            "data/SaintPetersSquare2/posy.png",
            "data/SaintPetersSquare2/negy.png",
            "data/SaintPetersSquare2/posz.png",
            "data/SaintPetersSquare2/negz.png");
    scene->add_texture(                          texture5);
    skybox_material->add_texture(                texture5);
    env_mapped_material->add_texture(            texture5);
    env_mapped_dbl_refract_material->add_texture(texture5);
    env_mapped_fast_material->add_texture(       texture5);

    frontface_depth_overlay_texture = new vt::Texture(
            "frontface_depth_overlay",
            HI_RES_TEX_DIM,
            HI_RES_TEX_DIM,
            NULL,
            vt::Texture::DEPTH);
    texture_mapped_material->add_texture(        frontface_depth_overlay_texture);
    env_mapped_dbl_refract_material->add_texture(frontface_depth_overlay_texture);
    overlay_write_through_material->add_texture( frontface_depth_overlay_texture);
    overlay_bloom_filter_material->add_texture(  frontface_depth_overlay_texture);
    ssao_material->add_texture(                  frontface_depth_overlay_texture);

    backface_depth_overlay_texture = new vt::Texture(
            "backface_depth_overlay",
            HI_RES_TEX_DIM,
            HI_RES_TEX_DIM,
            NULL,
            vt::Texture::DEPTH);
    texture_mapped_material->add_texture(        backface_depth_overlay_texture);
    env_mapped_dbl_refract_material->add_texture(backface_depth_overlay_texture);
    overlay_write_through_material->add_texture( backface_depth_overlay_texture);
    overlay_bloom_filter_material->add_texture(  backface_depth_overlay_texture);

    frontface_normal_overlay_texture = new vt::Texture(
            "frontface_normal_overlay",
            HI_RES_TEX_DIM,
            HI_RES_TEX_DIM,
            NULL,
            vt::Texture::RGB);
    texture_mapped_material->add_texture(        frontface_normal_overlay_texture);
    env_mapped_dbl_refract_material->add_texture(frontface_normal_overlay_texture);
    overlay_write_through_material->add_texture( frontface_normal_overlay_texture);
    overlay_bloom_filter_material->add_texture(  frontface_normal_overlay_texture);

    backface_normal_overlay_texture = new vt::Texture(
            "backface_normal_overlay",
            HI_RES_TEX_DIM,
            HI_RES_TEX_DIM,
            NULL,
            vt::Texture::RGB);
    texture_mapped_material->add_texture(        backface_normal_overlay_texture);
    env_mapped_dbl_refract_material->add_texture(backface_normal_overlay_texture);
    overlay_write_through_material->add_texture( backface_normal_overlay_texture);
    overlay_bloom_filter_material->add_texture(  backface_normal_overlay_texture);

    ssao_overlay_texture = new vt::Texture(
            "ssao_overlay",
            HI_RES_TEX_DIM,
            HI_RES_TEX_DIM,
            NULL,
            vt::Texture::RGB);
    texture_mapped_material->add_texture(        ssao_overlay_texture);
    env_mapped_dbl_refract_material->add_texture(ssao_overlay_texture);
    overlay_write_through_material->add_texture( ssao_overlay_texture);
    overlay_bloom_filter_material->add_texture(  ssao_overlay_texture);

    hi_res_color_overlay_texture = new vt::Texture(
            "hi_res_color_overlay",
            HI_RES_TEX_DIM,
            HI_RES_TEX_DIM,
            NULL,
            vt::Texture::RGB);
    texture_mapped_material->add_texture(       hi_res_color_overlay_texture);
    overlay_write_through_material->add_texture(hi_res_color_overlay_texture);
    overlay_bloom_filter_material->add_texture( hi_res_color_overlay_texture);
    overlay_max_material->add_texture(          hi_res_color_overlay_texture);

    med_res_color_overlay_texture = new vt::Texture(
            "med_res_color_overlay",
            MED_RES_TEX_DIM,
            MED_RES_TEX_DIM,
            NULL,
            vt::Texture::RGB);
    texture_mapped_material->add_texture(       med_res_color_overlay_texture);
    overlay_write_through_material->add_texture(med_res_color_overlay_texture);
    overlay_bloom_filter_material->add_texture( med_res_color_overlay_texture);

    lo_res_color_overlay_texture = new vt::Texture(
            "lo_res_color_overlay",
            LO_RES_TEX_DIM,
            LO_RES_TEX_DIM,
            NULL,
            vt::Texture::RGB);
    texture_mapped_material->add_texture(       lo_res_color_overlay_texture);
    overlay_write_through_material->add_texture(lo_res_color_overlay_texture);
    overlay_bloom_filter_material->add_texture( lo_res_color_overlay_texture);
    overlay_max_material->add_texture(          lo_res_color_overlay_texture);

    random_texture = new vt::Texture(
            "random_texture",
            RAND_TEX_DIM,
            RAND_TEX_DIM,
            NULL,
            vt::Texture::RGB,
            false,
            true);
    texture_mapped_material->add_texture(random_texture);
    ssao_material->add_texture(          random_texture);

    glm::vec3 origin = glm::vec3();
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);

    frontface_depth_overlay_fb  = new vt::FrameBuffer(frontface_depth_overlay_texture, camera);
    backface_depth_overlay_fb   = new vt::FrameBuffer(backface_depth_overlay_texture, camera);
    frontface_normal_overlay_fb = new vt::FrameBuffer(frontface_normal_overlay_texture, camera);
    backface_normal_overlay_fb  = new vt::FrameBuffer(backface_normal_overlay_texture, camera);
    ssao_overlay_fb             = new vt::FrameBuffer(ssao_overlay_texture, camera);
    hi_res_color_overlay_fb     = new vt::FrameBuffer(hi_res_color_overlay_texture, camera);
    med_res_color_overlay_fb    = new vt::FrameBuffer(med_res_color_overlay_texture, camera);
    lo_res_color_overlay_fb     = new vt::FrameBuffer(lo_res_color_overlay_texture, camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    mesh_overlay->set_material(overlay_write_through_material);
    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("hi_res_color_overlay"));

    // box
    mesh->set_material(bump_mapped_material);
    mesh->set_texture_index(     mesh->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh->set_bump_texture_index(mesh->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh->set_ambient_color(glm::vec3(0,0,0));

    // grid
    mesh2->set_material(ambient_material);
    mesh2->set_ambient_color(glm::vec3(0,0,0));

    // sphere
    mesh3->set_material(env_mapped_dbl_refract_material);
    mesh3->set_reflect_to_refract_ratio(0.33); // 33% reflective
    mesh3->set_texture_index(                        mesh3->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh3->set_bump_texture_index(                   mesh3->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh3->set_frontface_depth_overlay_texture_index(mesh3->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
    mesh3->set_backface_depth_overlay_texture_index( mesh3->get_material()->get_texture_index_by_name("backface_depth_overlay"));
    mesh3->set_backface_normal_overlay_texture_index(mesh3->get_material()->get_texture_index_by_name("backface_normal_overlay"));

    // torus
    mesh4->set_material(env_mapped_material);
    mesh4->set_reflect_to_refract_ratio(1); // 100% reflective
    mesh4->set_texture_index(     mesh4->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh4->set_bump_texture_index(mesh4->get_material()->get_texture_index_by_name("chesterfield_normal"));

    // cylinder
    mesh5->set_material(texture_mapped_material);
    mesh5->set_texture_index(mesh5->get_material()->get_texture_index_by_name("dex3d"));

    // cone
    mesh6->set_material(normal_material);
    mesh6->set_bump_texture_index(mesh6->get_material()->get_texture_index_by_name("chesterfield_normal"));

    // hemisphere
    mesh7->set_material(env_mapped_fast_material);

    // tetrahedron
    mesh8->set_material(env_mapped_dbl_refract_material);
    mesh8->set_reflect_to_refract_ratio(0.33); // 33% reflective
    mesh8->set_texture_index(                        mesh8->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh8->set_bump_texture_index(                   mesh8->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh8->set_frontface_depth_overlay_texture_index(mesh8->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
    mesh8->set_backface_depth_overlay_texture_index( mesh8->get_material()->get_texture_index_by_name("backface_depth_overlay"));
    mesh8->set_backface_normal_overlay_texture_index(mesh8->get_material()->get_texture_index_by_name("backface_normal_overlay"));

    // diamond
    mesh9->set_material(env_mapped_fast_material);
    mesh9->set_reflect_to_refract_ratio(0.33); // 33% reflective

    // box2
    mesh10->set_material(env_mapped_dbl_refract_material);
    mesh10->set_reflect_to_refract_ratio(0.33); // 33% reflective
    mesh10->set_texture_index(                        mesh10->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh10->set_bump_texture_index(                   mesh10->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh10->set_frontface_depth_overlay_texture_index(mesh10->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
    mesh10->set_backface_depth_overlay_texture_index( mesh10->get_material()->get_texture_index_by_name("backface_depth_overlay"));
    mesh10->set_backface_normal_overlay_texture_index(mesh10->get_material()->get_texture_index_by_name("backface_normal_overlay"));

    // diamond2
    hidden_mesh->set_material(env_mapped_dbl_refract_material);
    hidden_mesh->set_reflect_to_refract_ratio(0.33); // 33% reflective
    hidden_mesh->set_texture_index(                        hidden_mesh->get_material()->get_texture_index_by_name("chesterfield_color"));
    hidden_mesh->set_bump_texture_index(                   hidden_mesh->get_material()->get_texture_index_by_name("chesterfield_normal"));
    hidden_mesh->set_frontface_depth_overlay_texture_index(hidden_mesh->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
    hidden_mesh->set_backface_depth_overlay_texture_index( hidden_mesh->get_material()->get_texture_index_by_name("backface_depth_overlay"));
    hidden_mesh->set_backface_normal_overlay_texture_index(hidden_mesh->get_material()->get_texture_index_by_name("backface_normal_overlay"));

    // sphere2
    hidden_mesh2->set_material(env_mapped_dbl_refract_material);
    hidden_mesh2->set_reflect_to_refract_ratio(0.33); // 33% reflective
    hidden_mesh2->set_texture_index(                        hidden_mesh2->get_material()->get_texture_index_by_name("chesterfield_color"));
    hidden_mesh2->set_bump_texture_index(                   hidden_mesh2->get_material()->get_texture_index_by_name("chesterfield_normal"));
    hidden_mesh2->set_frontface_depth_overlay_texture_index(hidden_mesh2->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
    hidden_mesh2->set_backface_depth_overlay_texture_index( hidden_mesh2->get_material()->get_texture_index_by_name("backface_depth_overlay"));
    hidden_mesh2->set_backface_normal_overlay_texture_index(hidden_mesh2->get_material()->get_texture_index_by_name("backface_normal_overlay"));

    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        (*p)->set_material(env_mapped_fast_material);
        //(*p)->set_reflect_to_refract_ratio(0.33); // 33% reflective
    }

    // box3
    hidden_mesh3->set_material(env_mapped_dbl_refract_material);
    hidden_mesh3->set_reflect_to_refract_ratio(0.33); // 33% reflective
    hidden_mesh3->set_texture_index(                        hidden_mesh3->get_material()->get_texture_index_by_name("chesterfield_color"));
    hidden_mesh3->set_bump_texture_index(                   hidden_mesh3->get_material()->get_texture_index_by_name("chesterfield_normal"));
    hidden_mesh3->set_frontface_depth_overlay_texture_index(hidden_mesh3->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
    hidden_mesh3->set_backface_depth_overlay_texture_index( hidden_mesh3->get_material()->get_texture_index_by_name("backface_depth_overlay"));
    hidden_mesh3->set_backface_normal_overlay_texture_index(hidden_mesh3->get_material()->get_texture_index_by_name("backface_normal_overlay"));

    // grid2
    //hidden_mesh4->set_material(env_mapped_fast_material);
    hidden_mesh4->set_material(phong_material);
    //hidden_mesh4->set_reflect_to_refract_ratio(0.33); // 33% reflective
    hidden_mesh4->set_ambient_color(glm::vec3(0,0,0));

    return 1;
}

int deinit_resources()
{
    if(frontface_depth_overlay_fb)  { delete frontface_depth_overlay_fb; }
    if(backface_depth_overlay_fb)   { delete backface_depth_overlay_fb; }
    if(frontface_normal_overlay_fb) { delete frontface_normal_overlay_fb; }
    if(backface_normal_overlay_fb)  { delete backface_normal_overlay_fb; }
    if(ssao_overlay_fb)             { delete ssao_overlay_fb; }
    if(hi_res_color_overlay_fb)     { delete hi_res_color_overlay_fb; }
    if(med_res_color_overlay_fb)    { delete med_res_color_overlay_fb; }
    if(lo_res_color_overlay_fb)     { delete lo_res_color_overlay_fb; }

    return 1;
}

void onIdle()
{
    glutPostRedisplay();
}

void onTick()
{
    static unsigned int prev_tick = 0;
    static unsigned int frames = 0;
    unsigned int tick = glutGet(GLUT_ELAPSED_TIME);
    unsigned int delta_time = tick-prev_tick;
    static float fps = 0;
    if(delta_time > 1000) {
        fps = 1000.0*frames/delta_time;
        frames = 0;
        prev_tick = tick;
    }
    if(show_fps && delta_time > 100) {
        std::stringstream ss;
        ss << std::setprecision(2) << std::fixed << fps << " FPS, "
            << "Mouse: {" << mouse_drag.x << ", " << mouse_drag.y << "}, "
            << "Yaw=" << ORIENT_YAW(orient) << ", Pitch=" << ORIENT_PITCH(orient) << ", Radius=" << orbit_radius << ", "
            << "Zoom=" << zoom;
        //ss << "Width=" << camera->get_width() << ", Width=" << camera->get_height();
        glutSetWindowTitle(ss.str().c_str());
    }
    frames++;

    phase = static_cast<float>(glutGet(GLUT_ELAPSED_TIME))/1000*15; // base 15 degrees per second

    mesh_apply_ripple(hidden_mesh4, glm::vec3(0.5, 0, 0.5), 0.1, 0.5, -phase*0.1);
    hidden_mesh4->update_buffers();
}

void do_blur(
        vt::Scene*       scene,
        vt::Texture*     input_texture1,
        vt::Texture*     input_texture2,
        vt::FrameBuffer* output_fb,
        int              blur_iters,
        float            glow_cutoff_threshold)
{
    scene->set_glow_cutoff_threshold(glow_cutoff_threshold);

    // switch to write-through mode to perform downsampling
    mesh_overlay->set_material(overlay_write_through_material);

    // linear downsample texture from hi-res to med-res
    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index(input_texture1));
    med_res_color_overlay_fb->bind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    scene->render(true);
    med_res_color_overlay_fb->unbind();

    // linear downsample texture from med-res to lo-res
    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("med_res_color_overlay"));
    lo_res_color_overlay_fb->bind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    scene->render(true);
    lo_res_color_overlay_fb->unbind();

    // switch to bloom filter mode
    mesh_overlay->set_material(overlay_bloom_filter_material);

    // blur texture in low-res
    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("lo_res_color_overlay"));
    lo_res_color_overlay_fb->bind();
    for(int i = 0; i < blur_iters; i++) {
        // don't clear since we're using same texture for input/output
        //glClearColor(0, 0, 0, 1);
        //glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        scene->render(true);
    }
    lo_res_color_overlay_fb->unbind();

    // switch to max mode to merge blurred low-res texture with hi-res texture
    mesh_overlay->set_material(overlay_max_material);
    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index(input_texture2));
    mesh_overlay->set_texture2_index(mesh_overlay->get_material()->get_texture_index_by_name("lo_res_color_overlay"));

    output_fb->bind();
    if(output_fb->get_texture() != input_texture1) {
        // clear if we're using different texture for input/output
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }
    scene->render(true);
    output_fb->unbind();

    // switch to write-through mode to display final output texture
    mesh_overlay->set_material(overlay_write_through_material);
    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index(input_texture1));
}

void onDisplay()
{
    onTick();

    vt::Scene* scene = vt::Scene::instance();

    frontface_depth_overlay_fb->bind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    scene->render(false, false);
    frontface_depth_overlay_fb->unbind();

    if(overlay_mode == OVERLAY_MODE_FF_NORMAL) {
        frontface_normal_overlay_fb->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        scene->render(false, false, vt::Scene::use_material_type_t::USE_NORMAL_MATERIAL);
        frontface_normal_overlay_fb->unbind();
    }

    if(overlay_mode == OVERLAY_MODE_SSAO) {
        ssao_overlay_fb->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        scene->render(false, false, vt::Scene::use_material_type_t::USE_SSAO_MATERIAL);
        ssao_overlay_fb->unbind();

        // render-to-texture for initial input texture
        hi_res_color_overlay_fb->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        scene->render(false, true, vt::Scene::use_material_type_t::USE_MESH_MATERIAL, true);
        hi_res_color_overlay_fb->unbind();

        do_blur(scene, ssao_overlay_texture, hi_res_color_overlay_texture, ssao_overlay_fb, BLUR_ITERS, 0);
    }

    glCullFace(GL_FRONT);

    backface_depth_overlay_fb->bind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    scene->render(false, false);
    backface_depth_overlay_fb->unbind();

    backface_normal_overlay_fb->bind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    scene->render(false, false, vt::Scene::use_material_type_t::USE_NORMAL_MATERIAL);
    backface_normal_overlay_fb->unbind();

    glCullFace(GL_BACK);

    if(post_process_blur) {
        // render-to-texture for initial input texture
        hi_res_color_overlay_fb->bind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        scene->render(false, true);
        hi_res_color_overlay_fb->unbind();

        do_blur(scene, hi_res_color_overlay_texture, hi_res_color_overlay_texture, hi_res_color_overlay_fb, BLUR_ITERS, 0.75);
    }

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(wireframe_mode) {
        scene->render(false, false, vt::Scene::use_material_type_t::USE_WIREFRAME_MATERIAL);
    } else {
        scene->render(post_process_blur || overlay_mode);
    }

    //stencil_fb->bind();
    //glEnable(GL_STENCIL_TEST);
    //
    //glClearColor(0, 0, 0, 1);
    //glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    //
    //glStencilFunc(GL_ALWAYS, 0x1, 0x1);
    //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    //scene->render();
    //
    //glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
    //glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    //glDisable(GL_LIGHTING);
    //glColor3f(0,1,0); // outline color
    //glLineWidth(5);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe
    //scene->render();
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // return to fill mode
    //glLineWidth(1);
    //glEnable(GL_LIGHTING);
    //
    //glDisable(GL_STENCIL_TEST);
    //stencil_fb->unbind();

    if(show_axis || show_bbox || show_normals) {
        scene->render_lines(show_axis, show_bbox, show_normals);
    }
    if(show_lights) {
        scene->render_lights();
    }
    glutSwapBuffers();
}

void set_mesh_visibility(bool visible)
{
    mesh->set_visible(visible);         // box
    //mesh2->set_visible(visible);      // grid
    mesh3->set_visible(visible);        // sphere
    mesh4->set_visible(visible);        // torus
    mesh5->set_visible(visible);        // cylinder
    mesh6->set_visible(visible);        // cone
    mesh7->set_visible(visible);        // hemisphe
    mesh8->set_visible(visible);        // tetrahed
    mesh9->set_visible(visible);        // diamond
    mesh10->set_visible(visible);       // box2
    hidden_mesh->set_visible(visible);  // diamond2
    hidden_mesh2->set_visible(visible); // sphere2
    hidden_mesh3->set_visible(visible); // box3
    hidden_mesh4->set_visible(visible); // grid2
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        (*p)->set_visible(visible);
    }
}

void onKeyboard(unsigned char key, int x, int y)
{
    switch(key) {
        case 'b': // bbox
            show_bbox = !show_bbox;
            break;
        case 'd': // demo
            demo_mode = (demo_mode + 1) % DEMO_MODE_COUNT;
            switch(demo_mode) {
                case DEMO_MODE_DEFAULT:
                    set_mesh_visibility(true);
                    hidden_mesh->set_visible(false);  // diamond2
                    hidden_mesh2->set_visible(false); // sphere2
                    hidden_mesh3->set_visible(false); // box3
                    hidden_mesh4->set_visible(false); // grid2
                    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
                        (*p)->set_visible(false);
                    }
                    break;
                case DEMO_MODE_DIAMOND:
                    set_mesh_visibility(false);
                    hidden_mesh->set_visible(true); // diamond2
                    break;
                case DEMO_MODE_SPHERE:
                    set_mesh_visibility(false);
                    hidden_mesh2->set_visible(true); // sphere2
                    break;
                case DEMO_MODE_BOX:
                    set_mesh_visibility(false);
                    hidden_mesh3->set_visible(true); // box3
                    break;
                case DEMO_MODE_GRID:
                    set_mesh_visibility(false);
                    hidden_mesh4->set_visible(true); // grid2
                    break;
                case DEMO_MODE_MESHES_IMPORTED:
                    set_mesh_visibility(false);
                    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
                        (*p)->set_visible(true);
                    }
                    break;
                default:
                    break;
            }
            break;
        case 'f': // frame rate
            show_fps = !show_fps;
            if(!show_fps) {
                glutSetWindowTitle(DEFAULT_CAPTION);
            }
            break;
        case 'l': // lights
            show_lights = !show_lights;
            break;
        case 'n': // normals
            show_normals = !show_normals;
            break;
        case 'o': // overlay
            overlay_mode = (overlay_mode + 1) % OVERLAY_MODE_COUNT;
            if(post_process_blur) {
                post_process_blur = false;
                mesh_overlay->set_material(overlay_write_through_material);
            }
            switch(overlay_mode) {
                case OVERLAY_MODE_DEFAULT:
                    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("hi_res_color_overlay"));
                    break;
                case OVERLAY_MODE_FF_DEPTH:
                    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("frontface_depth_overlay"));
                    break;
                case OVERLAY_MODE_BF_DEPTH:
                    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("backface_depth_overlay"));
                    break;
                case OVERLAY_MODE_FF_NORMAL:
                    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("frontface_normal_overlay"));
                    break;
                case OVERLAY_MODE_BF_NORMAL:
                    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("backface_normal_overlay"));
                    break;
                case OVERLAY_MODE_SSAO:
                    mesh_overlay->set_texture_index(mesh_overlay->get_material()->get_texture_index_by_name("ssao_overlay"));
                    break;
                default:
                    break;
            }
            mesh2->set_visible(overlay_mode == OVERLAY_MODE_SSAO);
            break;
        case 'p': // projection
            if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
                camera->set_projection_mode(vt::Camera::PROJECTION_MODE_ORTHO);
            } else if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
                camera->set_projection_mode(vt::Camera::PROJECTION_MODE_PERSPECTIVE);
            }
            break;
        case 'r': // blur
            if(overlay_mode != OVERLAY_MODE_DEFAULT) {
                overlay_mode = OVERLAY_MODE_DEFAULT; // switch back to default overlay
                post_process_blur = true;
                mesh_overlay->set_material(overlay_bloom_filter_material);
                mesh2->set_visible(false);
                break;
            }
            post_process_blur = !post_process_blur;
            if(post_process_blur) {
                mesh_overlay->set_material(overlay_bloom_filter_material);
            } else {
                mesh_overlay->set_material(overlay_write_through_material);
            }
            break;
        case 't': // texture
            if(texture_id == 0) {
                texture_id = 1; // GL_TEXTURE1
            } else if(texture_id == 1) {
                texture_id = 2; // GL_TEXTURE2
            } else if(texture_id == 2) {
                texture_id = 3; // GL_TEXTURE3
            } else if(texture_id == 3) {
                texture_id = 0; // GL_TEXTURE0
            }
            mesh->set_texture_index( texture_id);
            //mesh2->set_texture_index(texture_id);
            //mesh3->set_texture_index(texture_id);
            //mesh4->set_texture_index(texture_id);
            //mesh5->set_texture_index(texture_id);
            //mesh6->set_texture_index(texture_id);
            break;
        case 'w': // wireframe
            wireframe_mode = !wireframe_mode;
            if(wireframe_mode) {
                glPolygonMode(GL_FRONT, GL_LINE);
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
            }
            break;
        case 'x': // axis
            show_axis = !show_axis;
            break;
        case 27: // escape
            exit(0);
            break;
    }
}

void onSpecial(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_F1:
            light->set_enabled(!light->get_enabled());
            break;
        case GLUT_KEY_F2:
            light2->set_enabled(!light2->get_enabled());
            break;
        case GLUT_KEY_F3:
            light3->set_enabled(!light3->get_enabled());
            break;
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN:
            break;
    }
}

void onSpecialUp(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_F1:
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN:
            break;
    }
}

void onMouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN) {
        prev_mouse_coord.x = x;
        prev_mouse_coord.y = y;
        if(button == GLUT_LEFT_BUTTON) {
            left_mouse_down = true;
            prev_orient = orient;
        }
        if(button == GLUT_RIGHT_BUTTON) {
            right_mouse_down = true;
            if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
                prev_orbit_radius = orbit_radius;
            } else if (camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
                prev_zoom = zoom;
            }
        }
    }
    else {
        left_mouse_down = right_mouse_down = false;
    }
}

void onMotion(int x, int y)
{
    if(left_mouse_down || right_mouse_down) {
        mouse_drag = glm::vec2(x, y) - prev_mouse_coord;
    }
    if(left_mouse_down) {
        orient = prev_orient+glm::vec3(0, mouse_drag.y*ORIENT_PITCH(orbit_speed), mouse_drag.x*ORIENT_YAW(orbit_speed));
        camera->orbit(orient, orbit_radius);
    }
    if(right_mouse_down) {
        if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
            orbit_radius = prev_orbit_radius + mouse_drag.y*dolly_speed;
            camera->orbit(orient, orbit_radius);
        } else if (camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
            zoom = prev_zoom + mouse_drag.y*ortho_dolly_speed;
            camera->set_zoom(&zoom);
        }
    }
}

void onReshape(int width, int height)
{
    camera->resize_viewport(width, height);
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE|GLUT_DEPTH/*|GLUT_STENCIL*/);
    glutInitWindowSize(init_screen_width, init_screen_height);
    glutCreateWindow(DEFAULT_CAPTION);

    GLenum glew_status = glewInit();
    if(glew_status != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return 1;
    }

    if(!GLEW_VERSION_2_0) {
        fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
        return 1;
    }

    char* s = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GLSL version %s\n", s);

    if(init_resources()) {
        glutDisplayFunc(onDisplay);
        glutKeyboardFunc(onKeyboard);
        glutSpecialFunc(onSpecial);
        glutSpecialUpFunc(onSpecialUp);
        glutMouseFunc(onMouse);
        glutMotionFunc(onMotion);
        glutReshapeFunc(onReshape);
        glutIdleFunc(onIdle);
        //glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glutMainLoop();
        deinit_resources();
    }

    return 0;
}
