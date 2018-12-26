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

/**
 * Based on Sylvain Beucler's tutorial from the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Author: onlyuser
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
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <shader_utils.h>

#include <Buffer.h>
#include <Camera.h>
#include <File3ds.h>
#include <FilePng.h>
#include <FrameBuffer.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <Modifiers.h>
#include <PrimitiveFactory.h>
#include <Program.h>
#include <Scene.h>
#include <Shader.h>
#include <ShaderContext.h>
#include <Texture.h>
#include <Util.h>
#include <VarAttribute.h>
#include <VarUniform.h>
#include <vector>
#include <iostream> // std::cout
#include <sstream> // std::stringstream
#include <iomanip> // std::setprecision

#include <cfenv>

#define MAX_RAY_TRACER_BOUNCE_COUNT     10
#define MIN_RAY_TRACER_BOUNCE_COUNT     1
#define DEFAULT_RAY_TRACER_BOUNCE_COUNT 5

#define AIR_REFRACTIVE_INDEX   1.0
#define GLASS_REFRACTIVE_INDEX 1.5
#define AIR_TO_GLASS_ETA       (AIR_REFRACTIVE_INDEX / GLASS_REFRACTIVE_INDEX)

#define MAX_RANDOM_POINTS 20

#define SUPER_HI_RES_TEX_DIM  512
#define HI_RES_TEX_DIM        256
#define MED_RES_TEX_DIM       128
#define LO_RES_TEX_DIM        64
#define BLUR_ITERS            5
#define GLOW_CUTOFF_THRESHOLD 0.9

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 600;
vt::Camera  *camera         = NULL;
vt::Mesh    *mesh_overlay   = NULL;
vt::Light   *light          = NULL,
            *light2         = NULL,
            *light3         = NULL;
vt::Texture *texture_skybox                = NULL,
            *color_texture                 = NULL,
            *color_texture2                = NULL,
            *color_texture3                = NULL,
            //*hi_res_color_overlay_texture  = NULL,
            *med_res_color_overlay_texture = NULL,
            *lo_res_color_overlay_texture  = NULL;

vt::FrameBuffer *color_texture_fb         = NULL,
                *color_texture2_fb        = NULL,
                *color_texture3_fb        = NULL,
                //*hi_res_color_overlay_fb  = NULL,
                *med_res_color_overlay_fb = NULL,
                *lo_res_color_overlay_fb  = NULL;

bool left_mouse_down  = false,
     right_mouse_down = false;
glm::vec2 prev_mouse_coord,
          mouse_drag;
glm::vec3 prev_euler,
          euler       = glm::vec3(0, -30, 30),
          orbit_speed = glm::vec3(0, -0.5, -0.5);
float prev_orbit_radius = 0,
      orbit_radius      = 8,
      dolly_speed       = 0.1,
      light_distance    = 4;
bool show_bbox         = false,
     show_fps          = false,
     show_help         = false,
     show_lights       = false,
     show_normals      = false,
     wireframe_mode    = false,
     show_guide_wires  = true,
     show_paths        = true,
     show_axis         = false,
     show_axis_labels  = false,
     post_process_blur = false,
     do_animation      = true,
     left_key          = false,
     right_key         = false,
     up_key            = false,
     down_key          = false,
     page_up_key       = false,
     page_down_key     = false,
     alt_down_key      = false,
     user_input        = true;

float prev_zoom         = 0,
      zoom              = 1,
      ortho_dolly_speed = 0.1;

int angle_delta = 1;

vt::Material *skybox_material                = NULL,
             *overlay_ray_traced_material    = NULL,
             *overlay_write_through_material = NULL,
             *overlay_bloom_filter_material  = NULL,
             *overlay_max_material           = NULL;

bool need_redraw_hi_res_frame = true;

int init_resources()
{
    vt::Scene* scene = vt::Scene::instance();

    mesh_overlay = vt::PrimitiveFactory::create_viewport_quad("overlay");
    scene->set_overlay(mesh_overlay);

    vt::Material* ambient_material = new vt::Material("ambient",
                                                      "src/shaders/ambient.v.glsl",
                                                      "src/shaders/ambient.f.glsl");
    scene->add_material(ambient_material);
    scene->set_wireframe_material(ambient_material);

    skybox_material = new vt::Material("skybox",
                                       "src/shaders/skybox.v.glsl",
                                       "src/shaders/skybox.f.glsl",
                                       true); // use_overlay
    scene->add_material(skybox_material);

    overlay_ray_traced_material = new vt::Material("overlay_ray_traced",
                                                   "src/shaders/overlay_ray_traced.v.glsl",
                                                   "src/shaders/overlay_ray_traced.f.glsl",
                                                   true); // use_overlay
    scene->add_material(overlay_ray_traced_material);

    overlay_write_through_material = new vt::Material("overlay_write_through",
                                                      "src/shaders/overlay_write_through.v.glsl",
                                                      "src/shaders/overlay_write_through.f.glsl",
                                                      true); // use_overlay
    scene->add_material(overlay_write_through_material);

    overlay_bloom_filter_material = new vt::Material("overlay_bloom_filter",
                                                     "src/shaders/overlay_bloom_filter.v.glsl",
                                                     "src/shaders/overlay_bloom_filter.f.glsl",
                                                     true); // use_overlay
    scene->add_material(overlay_bloom_filter_material);

    overlay_max_material = new vt::Material("overlay_max",
                                            "src/shaders/overlay_max.v.glsl",
                                            "src/shaders/overlay_max.f.glsl",
                                            true); // use_overlay
    scene->add_material(overlay_max_material);

    texture_skybox = new vt::Texture("skybox_texture",
                                     "data/SaintPetersSquare2/posx.png",
                                     "data/SaintPetersSquare2/negx.png",
                                     "data/SaintPetersSquare2/posy.png",
                                     "data/SaintPetersSquare2/negy.png",
                                     "data/SaintPetersSquare2/posz.png",
                                     "data/SaintPetersSquare2/negz.png");
    scene->add_texture(                      texture_skybox);
    skybox_material->add_texture(            texture_skybox);
    overlay_ray_traced_material->add_texture(texture_skybox);

    color_texture = new vt::Texture("color_texture",
                                    vt::Texture::RGBA,
                                    glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM));
    overlay_ray_traced_material->add_texture(   color_texture);
    overlay_write_through_material->add_texture(color_texture);
    overlay_bloom_filter_material->add_texture( color_texture);
    overlay_max_material->add_texture(          color_texture);

    color_texture2 = new vt::Texture("color_texture2",
                                     vt::Texture::RGBA,
                                     glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM));
    overlay_ray_traced_material->add_texture(   color_texture2);
    overlay_write_through_material->add_texture(color_texture2);
    overlay_bloom_filter_material->add_texture( color_texture2);
    overlay_max_material->add_texture(          color_texture2);

    color_texture3 = new vt::Texture("color_texture3",
                                     vt::Texture::RGBA,
                                     glm::ivec2(SUPER_HI_RES_TEX_DIM, SUPER_HI_RES_TEX_DIM));
    overlay_ray_traced_material->add_texture(   color_texture3);
    overlay_write_through_material->add_texture(color_texture3);
    overlay_bloom_filter_material->add_texture( color_texture3);
    overlay_max_material->add_texture(          color_texture3);

    //hi_res_color_overlay_texture = new vt::Texture("hi_res_color_overlay",
    //                                               vt::Texture::RGBA,
    //                                               glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM));
    //overlay_ray_traced_material->add_texture(   hi_res_color_overlay_texture);
    //overlay_write_through_material->add_texture(hi_res_color_overlay_texture);
    //overlay_bloom_filter_material->add_texture( hi_res_color_overlay_texture);
    //overlay_max_material->add_texture(          hi_res_color_overlay_texture);

    med_res_color_overlay_texture = new vt::Texture("med_res_color_overlay",
                                                    vt::Texture::RGBA,
                                                    glm::ivec2(MED_RES_TEX_DIM, MED_RES_TEX_DIM));
    overlay_ray_traced_material->add_texture(   med_res_color_overlay_texture);
    overlay_write_through_material->add_texture(med_res_color_overlay_texture);
    overlay_bloom_filter_material->add_texture( med_res_color_overlay_texture);
    overlay_max_material->add_texture(          med_res_color_overlay_texture);

    lo_res_color_overlay_texture = new vt::Texture("lo_res_color_overlay",
                                                   vt::Texture::RGBA,
                                                   glm::ivec2(LO_RES_TEX_DIM, LO_RES_TEX_DIM));
    overlay_ray_traced_material->add_texture(   lo_res_color_overlay_texture);
    overlay_write_through_material->add_texture(lo_res_color_overlay_texture);
    overlay_bloom_filter_material->add_texture( lo_res_color_overlay_texture);
    overlay_max_material->add_texture(          lo_res_color_overlay_texture);

    glm::vec3 origin = glm::vec3(0);
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    camera->orbit(euler, orbit_radius);
    scene->set_camera(camera);

    color_texture_fb         = new vt::FrameBuffer(color_texture, camera);
    color_texture2_fb        = new vt::FrameBuffer(color_texture2, camera);
    color_texture3_fb        = new vt::FrameBuffer(color_texture3, camera);
    //hi_res_color_overlay_fb  = new vt::FrameBuffer(hi_res_color_overlay_texture, camera);
    med_res_color_overlay_fb = new vt::FrameBuffer(med_res_color_overlay_texture, camera);
    lo_res_color_overlay_fb  = new vt::FrameBuffer(lo_res_color_overlay_texture, camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));
    light->set_enabled(false);
    light2->set_enabled(true);
    light3->set_enabled(false);

    mesh_overlay->set_material(overlay_ray_traced_material);
    //mesh_overlay->set_reflect_to_refract_ratio(1); // 100% reflective
    //mesh_overlay->set_ambient_color(glm::vec3(0, 0, 0));
    vt::Material* mesh_overlay_material = mesh_overlay->get_material();
    mesh_overlay->set_color_texture_index( mesh_overlay_material->get_texture_index_by_name("color_texture"));
    mesh_overlay->set_color_texture2_index(mesh_overlay_material->get_texture_index_by_name("color_texture2"));
    mesh_overlay->set_color_texture_source(0);

    scene->m_ray_tracer_bounce_count = DEFAULT_RAY_TRACER_BOUNCE_COUNT;

    // load spheres
    scene->add_ray_tracer_sphere(glm::vec3(-1, 0, 0), // origin
                                 1,                   // radius
                                 AIR_TO_GLASS_ETA,    // eta (transparent material)
                                 0,                   // diffuse fuzz
                                 glm::vec3(0, 0, 1),  // color
                                 0.5,                 // reflectance
                                 0.5,                 // transparency
                                 0);                  // luminosity
    scene->add_ray_tracer_sphere(glm::vec3(1, 0, 0), // origin
                                 1,                  // radius
                                 -BIG_NUMBER,        // eta (diffuse material)
                                 0.25,               // diffuse fuzz
                                 glm::vec3(1, 0, 0), // color
                                 0.5,                // reflectance
                                 0,                  // transparency
                                 0);                 // luminosity
    scene->add_ray_tracer_sphere(glm::vec3(-1, 2, 0), // origin
                                 1,                   // radius
                                 BIG_NUMBER,          // eta (reflective material)
                                 0,                   // diffuse fuzz
                                 glm::vec3(1, 1, 1),  // color
                                 0,                   // reflectance
                                 0,                   // transparency
                                 10);                 // luminosity
    scene->add_ray_tracer_sphere(glm::vec3(1, 2, 0), // origin
                                 1,                  // radius
                                 BIG_NUMBER,         // eta (opaque material -- don't care)
                                 0,                  // diffuse fuzz
                                 glm::vec3(0),       // color (don't care)
                                 1,                  // reflectance
                                 0,                  // transparency
                                 0);                 // luminosity (glowing material)

    // load planes
    scene->add_ray_tracer_plane(glm::vec3(0, -1, 0), // point
                                glm::vec3(0,  1, 0), // normal
                                -BIG_NUMBER,         // eta (diffuse material)
                                0,                   // diffuse fuzz
                                glm::vec3(0),        // color
                                0.5,                 // reflectance
                                0,                   // transparency
                                0);                  // luminosity

    // load boxes
    scene->add_ray_tracer_box(glm::vec3(-3,  1,  0), // origin
                              glm::vec3( 0,  0,  0), // euler
                              glm::vec3(-1, -1, -1), // min
                              glm::vec3( 1,  1,  1), // max
                              AIR_TO_GLASS_ETA,      // eta (transparent material)
                              0,                     // diffuse fuzz
                              glm::vec3(1),          // color
                              0.5,                   // reflectance
                              0.5,                   // transparency
                              0);                    // luminosity
    scene->add_ray_tracer_box(glm::vec3( 3,  1,  0), // origin
                              glm::vec3( 0,  0,  0), // euler
                              glm::vec3(-1, -1, -1), // min
                              glm::vec3( 1,  1,  1), // max
                              BIG_NUMBER,            // eta (dielectric material)
                              0,                     // diffuse fuzz
                              glm::vec3(1),          // color
                              1,                     // reflectance
                              0,                     // transparency
                              0);                    // luminosity

    scene->m_ray_tracer_render_mode = 2;

    srand(time(NULL));
    for(int i = 0; i < MAX_RANDOM_POINTS; i++) {
        scene->m_ray_tracer_random_points.push_back(glm::normalize(glm::vec3((static_cast<float>(rand()) / RAND_MAX) * 2 - 1,
                                                                             (static_cast<float>(rand()) / RAND_MAX) * 2 - 1,
                                                                             (static_cast<float>(rand()) / RAND_MAX) * 2 - 1)));
    }
    scene->m_ray_tracer_random_point_count = MAX_RANDOM_POINTS;
    scene->m_ray_tracer_random_seed = 0;

    return 1;
}

int deinit_resources()
{
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
    unsigned int delta_time = tick - prev_tick;
    static float fps = 0;
    if(delta_time > 1000) {
        fps = 1000.0 * frames / delta_time;
        frames = 0;
        prev_tick = tick;
    }
    if(show_fps && delta_time > 100) {
        std::stringstream ss;
        ss << std::setprecision(2) << std::fixed << fps << " FPS, "
            << "Mouse: {" << mouse_drag.x << ", " << mouse_drag.y << "}, "
            << "Yaw=" << EULER_YAW(euler) << ", Pitch=" << EULER_PITCH(euler) << ", Radius=" << orbit_radius << ", "
            << "Zoom=" << zoom;
        //ss << "Width=" << camera->get_width() << ", Width=" << camera->get_height();
        glutSetWindowTitle(ss.str().c_str());
    }
    frames++;
    //if(!do_animation) {
    //    return;
    //}
    if(left_key) {
        user_input = true;
    }
    if(right_key) {
        user_input = true;
    }
    if(up_key) {
        user_input = true;
    }
    if(down_key) {
        user_input = true;
    }
    if(user_input) {
        user_input = false;
    }
    static int angle = 0;
    angle = (angle + angle_delta) % 360;
    vt::Scene* scene = vt::Scene::instance();
    scene->m_ray_tracer_random_seed = tick;
}

char* get_help_string()
{
    return const_cast<char*>("HUD text");
}

void apply_bloom_filter(vt::Scene*       scene,
                        vt::Texture*     input_to_blur_texture,
                        int              blur_iters,
                        vt::Texture*     input_sharp_texture,
                        float            glow_cutoff_threshold,
                        vt::FrameBuffer* output_fb)
{
    // switch to write-through mode to perform downsampling
    mesh_overlay->set_material(overlay_write_through_material);

    // linear downsample texture from hi-res to med-res
    mesh_overlay->set_color_texture_index(overlay_write_through_material->get_texture_index(input_to_blur_texture));
    med_res_color_overlay_fb->bind();
    scene->render(false, true);
    med_res_color_overlay_fb->unbind();

    // linear downsample texture from med-res to lo-res
    mesh_overlay->set_color_texture_index(overlay_write_through_material->get_texture_index(med_res_color_overlay_fb->get_texture()));
    lo_res_color_overlay_fb->bind();
    scene->render(false, true);
    lo_res_color_overlay_fb->unbind();

    // switch to bloom filter mode
    mesh_overlay->set_material(overlay_bloom_filter_material);

    // blur texture in low-res
    mesh_overlay->set_color_texture_index(overlay_bloom_filter_material->get_texture_index(lo_res_color_overlay_fb->get_texture()));
    lo_res_color_overlay_fb->bind();
    for(int i = 0; i < blur_iters; i++) {
        scene->render(false, true);
    }
    lo_res_color_overlay_fb->unbind();

    // switch to max mode to merge blurred low-res texture with hi-res texture
    mesh_overlay->set_material(overlay_max_material);
    mesh_overlay->set_color_texture_index(overlay_max_material->get_texture_index(lo_res_color_overlay_fb->get_texture()));
    mesh_overlay->set_color_texture2_index(overlay_max_material->get_texture_index(input_sharp_texture));

    output_fb->bind();
    scene->set_glow_cutoff_threshold(glow_cutoff_threshold); // allow bloom only if summed rgb brighter than glow cutoff threshold
    scene->render(false, true);
    output_fb->unbind();
}

void draw_frame(vt::FrameBuffer *_color_texture_fb, int prev_frame_color_texture_source)
{
    vt::Scene* scene = vt::Scene::instance();

    // merge previous frame with current frame and render to texture
    _color_texture_fb->bind();
    mesh_overlay->set_material(overlay_ray_traced_material); // use ray traced material
    mesh_overlay->set_color_texture_source(prev_frame_color_texture_source);
    scene->render(false, true);
    _color_texture_fb->unbind();

    if(post_process_blur) {
        // backup texture pair (because apply_bloom_filter uses them)
        int color_texture_index  = mesh_overlay->get_color_texture_index();
        int color_texture2_index = mesh_overlay->get_color_texture2_index();

        apply_bloom_filter(scene,
                           _color_texture_fb->get_texture(), // input_to_blur_texture
                           BLUR_ITERS,                       // blur_iters
                           _color_texture_fb->get_texture(), // input_sharp_texture
                           GLOW_CUTOFF_THRESHOLD,            // glow_cutoff_threshold
                           _color_texture_fb);               // output_fb

        // restore texture pair
        mesh_overlay->set_color_texture_index(color_texture_index);
        mesh_overlay->set_color_texture2_index(color_texture2_index);
    }

    // render texture to screen
    mesh_overlay->set_material(overlay_write_through_material); // use write-through material
    mesh_overlay->set_color_texture_index(mesh_overlay->get_material()->get_texture_index(_color_texture_fb->get_texture()));
    scene->render(false, true);
}

void onDisplay()
{
    static int color_texture_source = 1; // color_texture2 (index == 1) ==> color_texture (index == 0)
    if(do_animation) {
        onTick();
    }

    if(!(left_mouse_down || right_mouse_down)) {
        if(need_redraw_hi_res_frame) {
            draw_frame(color_texture_fb, -1);  // pre-render previous frame to avoid blending blank frame on first mouse/keyboard event
            *color_texture2 = *color_texture;  // not sure which previous frame texture will be used upon first mouse/keyboard event, so sync to both
            draw_frame(color_texture3_fb, -1); // render hi-resolution frame without blending previous frame
            glutSwapBuffers();
            need_redraw_hi_res_frame = false; // only render high resolution frame once after each continuous mouse/keyboard event
        }
        return;
    }

    vt::Scene* scene = vt::Scene::instance();

    if(scene->m_ray_tracer_render_mode == 2) {
        draw_frame(color_texture_fb, color_texture_source);
    } else {
        mesh_overlay->set_material(overlay_ray_traced_material); // use ray traced material
        mesh_overlay->set_color_texture_index(mesh_overlay->get_material()->get_texture_index(color_texture_fb->get_texture()));
        scene->render(false, true);
    }

    glutSwapBuffers();

    if(scene->m_ray_tracer_render_mode == 2) {
        color_texture_source = !color_texture_source;   // swap render source
        std::swap(color_texture_fb, color_texture2_fb); // swap render target
    }
}

void onKeyboard(unsigned char key, int x, int y)
{
    switch(key) {
        case 'b': // bbox
            show_bbox = !show_bbox;
            break;
        case 'f': // frame rate
            show_fps = !show_fps;
            if(!show_fps) {
                glutSetWindowTitle(DEFAULT_CAPTION);
            }
            break;
        case 'g': // guide wires
            show_guide_wires = !show_guide_wires;
            break;
        case 'h': // help
            show_help = !show_help;
            break;
        case 'l': // lights
            show_lights = !show_lights;
            break;
        case 'n': // normals
            show_normals = !show_normals;
            break;
        case 'p': // projection
            if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
                camera->set_projection_mode(vt::Camera::PROJECTION_MODE_ORTHO);
            } else if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
                camera->set_projection_mode(vt::Camera::PROJECTION_MODE_PERSPECTIVE);
            }
            break;
        case 'r': // blur
            post_process_blur = !post_process_blur;
            break;
        case 's': // paths
            show_paths = !show_paths;
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
        case 'z': // axis labels
            show_axis_labels = !show_axis_labels;
            break;
        case '+': // increase bounce count
            {
                vt::Scene* scene = vt::Scene::instance();
                scene->m_ray_tracer_bounce_count = std::min(scene->m_ray_tracer_bounce_count + 1, MAX_RAY_TRACER_BOUNCE_COUNT);
                std::cout << "Info: Bounce count: " << scene->m_ray_tracer_bounce_count << std::endl;
            }
            break;
        case '-': // decrease bounce count
            {
                vt::Scene* scene = vt::Scene::instance();
                scene->m_ray_tracer_bounce_count = std::max(scene->m_ray_tracer_bounce_count - 1, MIN_RAY_TRACER_BOUNCE_COUNT);
                std::cout << "Info: Bounce count: " << scene->m_ray_tracer_bounce_count << std::endl;
            }
            break;
        case 'd':
            {
                vt::Scene* scene = vt::Scene::instance();
                scene->m_ray_tracer_render_mode = (scene->m_ray_tracer_render_mode + 1) % 3;
                std::cout << "Info: Render mode: " << scene->m_ray_tracer_render_mode << std::endl;
            }
            break;
        case 32: // space
            do_animation = !do_animation;
            break;
        case 27: // escape
            exit(0);
            break;
    }
    need_redraw_hi_res_frame = true;
}

void onSpecial(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_F1:
            light->set_enabled(!light->is_enabled());
            break;
        case GLUT_KEY_F2:
            light2->set_enabled(!light2->is_enabled());
            break;
        case GLUT_KEY_F3:
            light3->set_enabled(!light3->is_enabled());
            break;
        case GLUT_KEY_LEFT:
            left_key = true;
            break;
        case GLUT_KEY_RIGHT:
            right_key = true;
            break;
        case GLUT_KEY_UP:
            up_key = true;
            break;
        case GLUT_KEY_DOWN:
            down_key = true;
            break;
        case GLUT_KEY_PAGE_UP:
            page_up_key = true;
            break;
        case GLUT_KEY_PAGE_DOWN:
            page_down_key = true;
            break;
    }
    if(glutGetModifiers() == GLUT_ACTIVE_ALT) {
        alt_down_key = true;
    }
    need_redraw_hi_res_frame = true;
}

void onSpecialUp(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_LEFT:
            left_key = false;
            break;
        case GLUT_KEY_RIGHT:
            right_key = false;
            break;
        case GLUT_KEY_UP:
            up_key = false;
            break;
        case GLUT_KEY_DOWN:
            down_key = false;
            break;
        case GLUT_KEY_PAGE_UP:
            page_up_key = false;
            break;
        case GLUT_KEY_PAGE_DOWN:
            page_down_key = false;
            break;
    }
    if(glutGetModifiers() == GLUT_ACTIVE_ALT) {
        alt_down_key = false;
    }
}

void onMouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN) {
        prev_mouse_coord.x = x;
        prev_mouse_coord.y = y;
        if(button == GLUT_LEFT_BUTTON) {
            left_mouse_down = true;
            prev_euler = euler;
        }
        if(button == GLUT_RIGHT_BUTTON) {
            right_mouse_down = true;
            if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
                prev_orbit_radius = orbit_radius;
            } else if (camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
                prev_zoom = zoom;
            }
        }
    } else {
        left_mouse_down = right_mouse_down = false;
        need_redraw_hi_res_frame = true;
    }
}

void onMotion(int x, int y)
{
    if(left_mouse_down || right_mouse_down) {
        mouse_drag = glm::vec2(x, y) - prev_mouse_coord;
    }
    if(left_mouse_down) {
        euler = prev_euler + glm::vec3(0, mouse_drag.y * EULER_PITCH(orbit_speed), mouse_drag.x * EULER_YAW(orbit_speed));
        camera->orbit(euler, orbit_radius);
    }
    if(right_mouse_down) {
        if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
            orbit_radius = prev_orbit_radius + mouse_drag.y * dolly_speed;
            camera->orbit(euler, orbit_radius);
        } else if (camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
            zoom = prev_zoom + mouse_drag.y * ortho_dolly_speed;
            camera->set_zoom(&zoom);
        }
    }
}

void onReshape(int width, int height)
{
    camera->resize(0, 0, width, height);
    glViewport(0, 0, width, height);
}

int main(int argc, char* argv[])
{
#if 1
    // https://stackoverflow.com/questions/5393997/stopping-the-debugger-when-a-nan-floating-point-number-is-produced/5394095
    feenableexcept(FE_INVALID | FE_OVERFLOW);
#endif

    DEFAULT_CAPTION = argv[0];

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH /*| GLUT_STENCIL*/);
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

    const char* s = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
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
