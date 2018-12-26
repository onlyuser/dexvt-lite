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
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <shader_utils.h>

#include <Buffer.h>
#include <Camera.h>
#include <Octree.h>
#include <File3ds.h>
#include <FrameBuffer.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <Modifiers.h>
#include <Octree.h>
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
#include <math.h>

#include <cfenv>

#define BOID_NEAREST_NEIGHBOR_RADIUS 10

#define BOID_COUNT            100
#define BOID_DIM              glm::vec3(0.0625, 0.0625, 0.0625)
#define BOID_INIT_SCATTER_MAX glm::vec3(5)
#define BOID_INIT_SCATTER_MIN glm::vec3(-5)

#define BOID_ANGLE_DELTA            2.5f
#define BOID_FORWARD_SPEED_MIN      0.0125f
#define BOID_FORWARD_SPEED_MAX      0.025f
#define BOID_NEAREST_NEIGHBOR_COUNT 20
#define OCTREE_ORIGIN               glm::vec3(-5)
#define OCTREE_DIM                  glm::vec3(10)

#define GRAVITATIONAL_CONSTANT 0.00001f

#define HEATMAP_NEAR_DIST 0
#define HEATMAP_FAR_DIST  5

//#define DEBUG 1

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 600;
vt::Camera  *camera         = NULL;
vt::Octree  *octree         = NULL;
vt::Mesh    *mesh_skybox    = NULL,
            *box            = NULL;
vt::Light   *light          = NULL,
            *light2         = NULL,
            *light3         = NULL;
vt::Texture *texture_skybox = NULL;

bool left_mouse_down  = false,
     right_mouse_down = false;
glm::vec2 prev_mouse_coord,
          mouse_drag;
glm::vec3 prev_euler,
          euler,
          orbit_speed = glm::vec3(0, -0.5, -0.5);
float prev_orbit_radius = 0,
      orbit_radius      = 8,
      dolly_speed       = 0.1,
      light_distance    = 4;
bool show_bbox        = false,
     show_fps         = false,
     show_help        = false,
     show_lights      = false,
     show_normals     = false,
     wireframe_mode   = false,
     show_guide_wires = false,
     show_paths       = true,
     show_axis        = false,
     show_axis_labels = false,
     do_animation     = true,
     left_key         = false,
     right_key        = false,
     up_key           = false,
     down_key         = false,
     page_up_key      = false,
     page_down_key    = false,
     user_input       = true;

float prev_zoom         = 0,
      zoom              = 1,
      ortho_dolly_speed = 0.1;

int angle_delta = 1;

int target_index = 7;
glm::vec3 targets[8];

std::vector<vt::Mesh*> boid_meshes;
glm::vec3 boid_origin[BOID_COUNT];
glm::vec3 boid_velocity[BOID_COUNT];

static void randomize_boids(std::vector<vt::Mesh*>* meshes,
                            glm::vec3               scatter_min,
                            glm::vec3               scatter_max)
{
    size_t boid_count = meshes->size();
    for(int i = 0; i < static_cast<int>(boid_count); i++) {
        glm::vec3 rand_vec(static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX);
        boid_origin[i] = MIX(scatter_min, scatter_max, rand_vec);
        glm::vec3 rand_vec2(static_cast<float>(rand()) / RAND_MAX,
                            static_cast<float>(rand()) / RAND_MAX,
                            static_cast<float>(rand()) / RAND_MAX);
        boid_velocity[i] = MIX(glm::vec3(-BOID_FORWARD_SPEED_MAX), glm::vec3(BOID_FORWARD_SPEED_MAX), rand_vec2);
    }
}

static void create_boids(vt::Scene*              scene,
                         std::vector<vt::Mesh*>* boid_meshes,
                         int                     boid_count,
                         glm::vec3               scatter_min,
                         glm::vec3               scatter_max,
                         glm::vec3               box_dim,
                         std::string             name)
{
    if(!scene || !boid_meshes) {
        return;
    }
    srand(time(NULL));
    for(int i = 0; i < boid_count; i++) {
        std::stringstream ss;
        ss << i;
        //ss << name << "_" << i;
        vt::Mesh* mesh = vt::PrimitiveFactory::create_tetrahedron(ss.str());
        mesh->center_axis();
        mesh->set_scale(box_dim);
        mesh->flatten();
        mesh->center_axis();
        scene->add_mesh(mesh);
        boid_meshes->push_back(mesh);
    }
    randomize_boids(boid_meshes,
                    scatter_min,
                    scatter_max);
}

const float     heatmap_threshold[5] = {1, 0.75, 0.5, 0.25, 0};
const glm::vec3 heatmap_colors[5]    = {glm::vec3(1, 0, 0),  // red
                                        glm::vec3(1, 1, 0),  // yellow
                                        glm::vec3(0, 1, 0),  // green
                                        glm::vec3(0, 1, 1),  // cyan
                                        glm::vec3(0, 0, 1)}; // blue

static glm::vec3 lerp_heatmap(float value,
                              float min_value,
                              float max_value,
                              bool  min_is_red)
{
    value = CLAMP(value, min_value, max_value);
    float alpha = (value - min_value) / (max_value - min_value);
    if(min_is_red) {
        alpha = 1 - alpha;
    }
    for(int i = 0; i < 4; i++) {
        if(alpha > heatmap_threshold[i + 1]) {
            float segment_alpha = (alpha - heatmap_threshold[i + 1]) / (heatmap_threshold[i] - heatmap_threshold[i + 1]);
            return MIX(heatmap_colors[i + 1], heatmap_colors[i], segment_alpha);
        }
    }
    int heatmap_color_last_index = sizeof(heatmap_threshold) / sizeof(heatmap_threshold[0]) - 1;
    return heatmap_colors[heatmap_color_last_index];
}

int init_resources()
{
    glm::vec3 target_origin(-2.5, -2.5, -2.5);
    glm::vec3 target_dim(5, 5, 5);
    vt::PrimitiveFactory::get_box_corners(targets, &target_origin, &target_dim);

    vt::Scene* scene = vt::Scene::instance();

    mesh_skybox = vt::PrimitiveFactory::create_viewport_quad("grid");
    scene->set_skybox(mesh_skybox);

    vt::Material* ambient_material = new vt::Material("ambient",
                                                      "src/shaders/ambient.v.glsl",
                                                      "src/shaders/ambient.f.glsl");
    scene->add_material(ambient_material);
    scene->set_wireframe_material(ambient_material);

    vt::Material* skybox_material = new vt::Material("skybox",
                                                     "src/shaders/skybox.v.glsl",
                                                     "src/shaders/skybox.f.glsl",
                                                     true); // use_overlay
    scene->add_material(skybox_material);

    vt::Material* phong_material = new vt::Material("phong",
                                                    "src/shaders/phong.v.glsl",
                                                    "src/shaders/phong.f.glsl");
    scene->add_material(phong_material);

    texture_skybox = new vt::Texture("skybox_texture",
                                     "data/SaintPetersSquare2/posx.png",
                                     "data/SaintPetersSquare2/negx.png",
                                     "data/SaintPetersSquare2/posy.png",
                                     "data/SaintPetersSquare2/negy.png",
                                     "data/SaintPetersSquare2/posz.png",
                                     "data/SaintPetersSquare2/negz.png");
    scene->add_texture(          texture_skybox);
    skybox_material->add_texture(texture_skybox);

    glm::vec3 origin = glm::vec3(0);
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);
    octree = new vt::Octree(OCTREE_ORIGIN, OCTREE_DIM);
    scene->set_octree(octree);
    box = vt::PrimitiveFactory::create_box("octree", OCTREE_DIM.x, OCTREE_DIM.y, OCTREE_DIM.z);
    box->center_axis();
    box->set_origin(glm::vec3(0));
    //box->set_material(phong_material);
    box->set_ambient_color(glm::vec3(1));
    box->set_visible(false);
    scene->add_mesh(box);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_color_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    create_boids(scene,
                 &boid_meshes,
                 BOID_COUNT,
                 BOID_INIT_SCATTER_MIN,
                 BOID_INIT_SCATTER_MAX,
                 BOID_DIM,
                 "boid");
    long index = 0;
    for(std::vector<vt::Mesh*>::iterator p = boid_meshes.begin(); p != boid_meshes.end(); ++p) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
        octree->insert(index, (*p)->get_origin());
        index++;
    }

    scene->m_debug_targets.push_back(std::make_tuple(targets[target_index], glm::vec3(1, 0, 1), 1, 1));

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
    if(!do_animation) {
        return;
    }

    // clear
    //octree->clear();

    long index = 0;
    for(std::vector<vt::Mesh*>::iterator p = boid_meshes.begin(); p != boid_meshes.end(); ++p) {
        vt::Mesh* self_object = *p;

        // keep boids in octree
        boid_origin[index] = box->wrap(boid_origin[index]);
        glm::vec3 self_object_pos = boid_origin[index];
        self_object->set_origin(self_object_pos);

        // add/update
        if(octree->exists(index)) {
            octree->move(index, self_object_pos);
        } else {
            octree->insert(index, self_object_pos);
        }
        index++;
    }

    // rebalance
    octree->rebalance();

    long index2 = 0;
    for(std::vector<vt::Mesh*>::iterator p = boid_meshes.begin(); p != boid_meshes.end(); ++p) {
        vt::Mesh* self_object     = *p;
        glm::vec3 self_object_pos = self_object->get_origin();

        self_object->m_debug_lines.clear();

        // flocking behavior
        std::vector<long> nearest_k_indices;
        if(octree->find(self_object_pos,
                        BOID_NEAREST_NEIGHBOR_COUNT,
                        &nearest_k_indices,
                        BOID_NEAREST_NEIGHBOR_RADIUS))
        {
            glm::vec3 group_centroid(0);
            for(std::vector<long>::iterator q = nearest_k_indices.begin(); q != nearest_k_indices.end(); ++q) {
                if(*q == index2) { // ignore self
                    continue;
                }
                vt::Mesh* other_object     = boid_meshes[*q];
                glm::vec3 other_object_pos = other_object->get_origin();

                // collect stats
                group_centroid += other_object_pos;
                float dist = glm::distance(self_object_pos, other_object_pos);
                glm::vec3 color = lerp_heatmap(dist, HEATMAP_NEAR_DIST, HEATMAP_FAR_DIST, true);
                self_object->m_debug_lines.push_back(std::make_tuple(self_object_pos, other_object_pos, color, 1));
            }
            if(nearest_k_indices.size()) {
                group_centroid *= (1.0f / nearest_k_indices.size());
                float mass = nearest_k_indices.size();
                float dist = glm::distance(group_centroid, self_object_pos);
                if(dist < EPSILON) {
                    index2++;
                    continue;
                }
                float force = GRAVITATIONAL_CONSTANT * mass / pow(dist, 2);
                boid_velocity[index2] += vt::safe_normalize(group_centroid - self_object_pos) * force;

                // apply speed limit
                boid_velocity[index2] = vt::safe_normalize(boid_velocity[index2]) * std::min(glm::length(boid_velocity[index2]), BOID_FORWARD_SPEED_MAX);
            }
        }
        boid_origin[index2] += boid_velocity[index2];
        index2++;
    }

    static int angle = 0;
    angle = (angle + angle_delta) % 360;
}

char* get_help_string()
{
    return const_cast<char*>("HUD text");
}

void onDisplay()
{
    if(do_animation) {
        onTick();
    }
    vt::Scene* scene = vt::Scene::instance();
    if(wireframe_mode) {
        scene->render(true, false, false, vt::Scene::use_material_type_t::USE_WIREFRAME_MATERIAL);
    } else {
        scene->render();
    }
    if(show_guide_wires || show_paths || show_axis || show_axis_labels || show_bbox || show_normals || show_help) {
        scene->render_lines_and_text(show_guide_wires, show_paths, show_axis, show_axis_labels, show_bbox, show_normals, show_help, get_help_string());
    }
    if(show_lights) {
        scene->render_lights();
    }
    glutSwapBuffers();
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
            if(show_guide_wires) {
                vt::Scene::instance()->m_debug_targets[0] = std::make_tuple(targets[target_index], glm::vec3(1, 0, 1), 1, 1);
            }
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
        case 'r': // reset
            randomize_boids(&boid_meshes,
                            BOID_INIT_SCATTER_MIN,
                            BOID_INIT_SCATTER_MAX);
            octree->clear();
            break;
        case 's': // paths
            show_paths = !show_paths;
            break;
        case 'w': // wireframe
            wireframe_mode = !wireframe_mode;
            if(wireframe_mode) {
                glPolygonMode(GL_FRONT, GL_LINE);
                for(std::vector<vt::Mesh*>::iterator p = boid_meshes.begin(); p != boid_meshes.end(); ++p) {
                    (*p)->set_ambient_color(glm::vec3(1));
                }
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
                for(std::vector<vt::Mesh*>::iterator p = boid_meshes.begin(); p != boid_meshes.end(); ++p) {
                    (*p)->set_ambient_color(glm::vec3(0));
                }
            }
            break;
        case 'x': // axis
            show_axis = !show_axis;
            break;
        case 'z': // axis labels
            show_axis_labels = !show_axis_labels;
            break;
        case 32: // space
            do_animation = !do_animation;
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
            light->set_enabled(!light->is_enabled());
            break;
        case GLUT_KEY_F2:
            light2->set_enabled(!light2->is_enabled());
            break;
        case GLUT_KEY_F3:
            light3->set_enabled(!light3->is_enabled());
            break;
        case GLUT_KEY_HOME: // target
            {
                size_t target_count = sizeof(targets) / sizeof(targets[0]);
                target_index = (target_index + 1) % target_count;
                std::cout << "Target #" << target_index << ": " << glm::to_string(targets[target_index]) << std::endl;
                vt::Scene::instance()->m_debug_targets[0] = std::make_tuple(targets[target_index], glm::vec3(1, 0, 1), 1, 1);
            }
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
// NOTE: still something wrong; crashes on startup; only doesn't crash in gdb
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
