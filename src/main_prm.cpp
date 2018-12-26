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
#include <KeyframeMgr.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <Modifiers.h>
#include <Octree.h>
#include <PRM.h>
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
#include <tuple>
#include <iostream> // std::cout
#include <sstream> // std::stringstream
#include <iomanip> // std::setprecision
#include <math.h>

#include <cfenv>

#define TARGET_SPEED 0.05f

#define OBSTACLE_COUNT            4
#define OBSTACLE_DIM_MAX          glm::vec3(10, 0.1, 10)
#define OBSTACLE_DIM_MIN          glm::vec3(5, 0.1, 5)
#define OBSTACLE_INIT_SCATTER_MAX glm::vec3(5)
#define OBSTACLE_INIT_SCATTER_MIN glm::vec3(-5)

#define OCTREE_ORIGIN glm::vec3(-5)
#define OCTREE_DIM    glm::vec3(10)

#define WAYPOINT_COUNT                   200
#define WAYPOINT_NEAREST_NEIGHBOR_COUNT  10
#define WAYPOINT_NEAREST_NEIGHBOR_RADIUS 5

#define FRAMES_PER_SEGMENT 40

//#define DEBUG 1

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 600;
vt::Camera  *camera         = NULL;
vt::Octree  *octree         = NULL;
vt::PRM     *prm            = NULL;
vt::Mesh    *mesh_skybox    = NULL,
            *sphere         = NULL;
// NOTE: not needed
#if 0
vt::Mesh    *box            = NULL;
#endif
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
     show_guide_wires = true,
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

std::vector<vt::Mesh*> obstacle_meshes;

static void randomize_meshes(std::vector<vt::Mesh*>* meshes,
                             glm::vec3               scatter_min,
                             glm::vec3               scatter_max)
{
    for(std::vector<vt::Mesh*>::iterator p = meshes->begin(); p != meshes->end(); ++p) {
        glm::vec3 rand_vec(static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX);
        (*p)->set_origin(MIX(scatter_min, scatter_max, rand_vec));
        (*p)->set_euler(glm::vec3(-180 + static_cast<float>(rand()) / RAND_MAX * 360,
                                  -90  + static_cast<float>(rand()) / RAND_MAX * 90,
                                  -180 + static_cast<float>(rand()) / RAND_MAX * 360));
    }
}

static void create_obstacles(vt::Scene*              scene,
                             std::vector<vt::Mesh*>* obstacle_meshes,
                             int                     obstacle_count,
                             glm::vec3               scatter_min,
                             glm::vec3               scatter_max,
                             glm::vec3               dim_min,
                             glm::vec3               dim_max,
                             std::string             name)
{
    if(!scene || !obstacle_meshes) {
        return;
    }
    srand(time(NULL));
    for(int i = 0; i < obstacle_count; i++) {
        std::stringstream ss;
        ss << name << "_" << i;
        vt::Mesh* mesh = vt::PrimitiveFactory::create_box(ss.str());
        mesh->center_axis();
        glm::vec3 rand_vec(static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX);
        mesh->set_scale(MIX(dim_min, dim_max, rand_vec));
        mesh->flatten();
        mesh->center_axis();
        scene->add_mesh(mesh);
        obstacle_meshes->push_back(mesh);
    }
    randomize_meshes(obstacle_meshes,
                     scatter_min,
                     scatter_max);
}

static void randomize_prm(vt::Scene*              scene,
                          vt::PRM*                prm,
                          int                     n,
                          int                     k,
                          float                   radius,
                          std::vector<vt::Mesh*>* obstacle_meshes)
{
    prm->clear();
    prm->randomize_waypoints(n);
    prm->connect_waypoints(k, radius);
    for(std::vector<vt::Mesh*>::iterator t = obstacle_meshes->begin(); t != obstacle_meshes->end(); ++t) {
        prm->add_obstacle(*t);
    }
    prm->prune_edges();

    scene->m_debug_targets.clear();
    scene->m_debug_targets.push_back(std::make_tuple(targets[target_index], glm::vec3(1, 0, 1), 4, 2));
    scene->m_debug_targets.push_back(std::make_tuple(glm::vec3(0),          glm::vec3(1, 1, 0), 4, 2));

    std::vector<glm::vec3> waypoints;
    prm->export_waypoints(&waypoints);
    for(std::vector<glm::vec3>::iterator q = waypoints.begin(); q != waypoints.end(); ++q) {
        scene->m_debug_targets.push_back(std::make_tuple(*q, glm::vec3(1, 0, 0), 1, 1));
    }

    std::vector<std::tuple<int, int, float>> edges;
    prm->export_edges(&edges);
    scene->m_debug_lines.clear();
    for(std::vector<std::tuple<int, int, float>>::iterator r = edges.begin(); r != edges.end(); ++r) {
        glm::vec3 color = glm::vec3(0, 1, 1);
        glm::vec3 p1    = waypoints[std::get<vt::PRM::EXPORT_EDGE_P1>(*r)];
        glm::vec3 p2    = waypoints[std::get<vt::PRM::EXPORT_EDGE_P2>(*r)];
        scene->m_debug_lines.push_back(std::make_tuple(p1, p2, color, 1));
    }
}

static void update_target(glm::vec3 target)
{
    int nearest_waypoint_index = prm->find_nearest_waypoint(target);
    if(nearest_waypoint_index == -1) {
        return;
    }
    vt::PRM_Waypoint* nearest_waypoint = prm->at(nearest_waypoint_index);
    if(!nearest_waypoint) {
        return;
    }
    vt::Scene* scene = vt::Scene::instance();
    std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[1]) = nearest_waypoint->get_origin();
    std::set<int> path_indices;
    std::vector<int> path;
    if(prm->find_shortest_path(glm::vec3(0), nearest_waypoint->get_origin(), &path) && path.size() > 1) {
        vt::KeyframeMgr::instance()->clear();
        long object_id = 0;
        int frame = 0;
        for(std::vector<int>::iterator p = path.begin(); p != path.end(); ++p) {
            path_indices.insert(*p);
            vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, frame, new vt::Keyframe(prm->at(*p)->get_origin(), true));
            frame += FRAMES_PER_SEGMENT;
        }
        vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, frame, new vt::Keyframe(targets[target_index], true));
        vt::KeyframeMgr::instance()->update_control_points(0.5);
        std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
        origin_frame_values.clear();
        vt::KeyframeMgr::instance()->export_frame_values_for_object(object_id, &origin_frame_values, NULL, NULL, true);
        std::vector<glm::vec3> &origin_keyframe_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_keyframe_values;
        origin_keyframe_values.clear();
        vt::KeyframeMgr::instance()->export_keyframe_values_for_object(object_id, &origin_keyframe_values, NULL, NULL, true);
    }
    std::vector<std::tuple<int, int, float>> edges;
    prm->export_edges(&edges);
    scene->m_debug_lines.clear();
    for(std::vector<std::tuple<int, int, float>>::iterator r = edges.begin(); r != edges.end(); ++r) {
        int p1_index = std::get<vt::PRM::EXPORT_EDGE_P1>(*r);
        int p2_index = std::get<vt::PRM::EXPORT_EDGE_P2>(*r);
        glm::vec3 color;
        float     linewidth = 0;
        if((path_indices.find(p1_index) != path_indices.end()) &&
           (path_indices.find(p2_index) != path_indices.end()))
        {
            color     = glm::vec3(1, 0, 0);
            linewidth = 4;
        } else {
            color     = glm::vec3(0, 1, 1);
            linewidth = 1;
        }
        glm::vec3 p1 = prm->at(p1_index)->get_origin();
        glm::vec3 p2 = prm->at(p2_index)->get_origin();
        scene->m_debug_lines.push_back(std::make_tuple(p1, p2, color, linewidth));
    }
}

int init_resources()
{
    glm::vec3 target_origin(-2.5, -2.5, -2.5);
    glm::vec3 target_dim(5, 5, 5);
    vt::PrimitiveFactory::get_box_corners(targets, &target_origin, &target_dim);

    vt::Scene* scene = vt::Scene::instance();

    mesh_skybox = vt::PrimitiveFactory::create_viewport_quad("grid");
    scene->set_skybox(mesh_skybox);

    sphere = vt::PrimitiveFactory::create_sphere("sphere", 16, 16,  0.5);
    scene->add_mesh(sphere);

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
// NOTE: not needed
#if 0
    box = vt::PrimitiveFactory::create_box("octree", OCTREE_DIM.x, OCTREE_DIM.y, OCTREE_DIM.z);
    box->center_axis();
    box->set_origin(glm::vec3(0));
    //box->set_material(phong_material);
    box->set_ambient_color(glm::vec3(1));
    box->set_visible(false);
    scene->add_mesh(box);
#endif

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_color_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    create_obstacles(scene,
                     &obstacle_meshes,
                     OBSTACLE_COUNT,
                     OBSTACLE_INIT_SCATTER_MIN,
                     OBSTACLE_INIT_SCATTER_MAX,
                     OBSTACLE_DIM_MIN,
                     OBSTACLE_DIM_MAX,
                     "box");
    for(std::vector<vt::Mesh*>::iterator p = obstacle_meshes.begin(); p != obstacle_meshes.end(); ++p) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
    }

    sphere->set_material(phong_material);
    sphere->set_ambient_color(glm::vec3(0));

// NOTE: don't include octree box in obstacle meshes
#if 0
    // NOTE: must add last!
    obstacle_meshes.push_back(box);
#endif

    prm = new vt::PRM(octree);
    randomize_prm(scene,
                  prm,
                  WAYPOINT_COUNT,
                  WAYPOINT_NEAREST_NEIGHBOR_COUNT,
                  WAYPOINT_NEAREST_NEIGHBOR_RADIUS,
                  &obstacle_meshes);

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
        targets[target_index] -= VEC_LEFT * TARGET_SPEED;
        user_input = true;
    }
    if(right_key) {
        targets[target_index] += VEC_LEFT * TARGET_SPEED;
        user_input = true;
    }
    if(up_key) {
        targets[target_index] -= VEC_FORWARD * TARGET_SPEED;
        user_input = true;
    }
    if(down_key) {
        targets[target_index] += VEC_FORWARD * TARGET_SPEED;
        user_input = true;
    }
    if(page_up_key) {
        targets[target_index] += VEC_UP * TARGET_SPEED;
        user_input = true;
    }
    if(page_down_key) {
        targets[target_index] -= VEC_UP * TARGET_SPEED;
        user_input = true;
    }
    if(user_input) {
        vt::Scene* scene = vt::Scene::instance();
        std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[0]) = targets[target_index];
        update_target(targets[target_index]);
        user_input = false;
    }

    // clear
    //octree->clear();

    // rebalance
    octree->rebalance();

    static int frame = 0;
    long object_id = 0;
    std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
    if(origin_frame_values.empty()) {
        std::cout << "Error: Empty frame values!" << std::endl;
        return;
    }
    sphere->set_origin(origin_frame_values[frame]);
    static bool forward = true;
    if(forward) {
        frame++;
        if(frame >= static_cast<int>(origin_frame_values.size()) - 1) {
            frame   = static_cast<int>(origin_frame_values.size()) - 1;
            forward = false;
        }
    } else {
        frame--;
        if(frame <= 0) {
            frame   = 0;
            forward = true;
        }
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
                vt::Scene* scene = vt::Scene::instance();
                std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[0]) = targets[target_index];
                std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[1]) = glm::vec3(0);
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
            {
// NOTE: don't include octree box in obstacle meshes
#if 0
                obstacle_meshes.pop_back();
#endif
                randomize_meshes(&obstacle_meshes,
                                 OBSTACLE_INIT_SCATTER_MIN,
                                 OBSTACLE_INIT_SCATTER_MAX);
// NOTE: don't include octree box in obstacle meshes
#if 0
                obstacle_meshes.push_back(box);
#endif
                octree->clear();

                vt::Scene* scene = vt::Scene::instance();
                randomize_prm(scene,
                              prm,
                              WAYPOINT_COUNT,
                              WAYPOINT_NEAREST_NEIGHBOR_COUNT,
                              WAYPOINT_NEAREST_NEIGHBOR_RADIUS,
                              &obstacle_meshes);

                std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[0]) = targets[target_index];
                int nearest_waypoint_index = prm->find_nearest_waypoint(targets[target_index]);
                if(nearest_waypoint_index != -1) {
                    vt::PRM_Waypoint* nearest_waypoint = prm->at(nearest_waypoint_index);
                    if(nearest_waypoint) {
                        std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[1]) = nearest_waypoint->get_origin();
                    }
                }
            }
            break;
        case 's': // paths
            show_paths = !show_paths;
            break;
        case 'w': // wireframe
            wireframe_mode = !wireframe_mode;
            if(wireframe_mode) {
                glPolygonMode(GL_FRONT, GL_LINE);
                for(std::vector<vt::Mesh*>::iterator p = obstacle_meshes.begin(); p != obstacle_meshes.end(); ++p) {
                    (*p)->set_ambient_color(glm::vec3(1));
                }
                sphere->set_ambient_color(glm::vec3(1));
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
                for(std::vector<vt::Mesh*>::iterator p = obstacle_meshes.begin(); p != obstacle_meshes.end(); ++p) {
                    (*p)->set_ambient_color(glm::vec3(0));
                }
                sphere->set_ambient_color(glm::vec3(0));
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
                vt::Scene* scene = vt::Scene::instance();
                std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[0]) = targets[target_index];
                std::get<vt::Scene::DEBUG_TARGET_ORIGIN>(scene->m_debug_targets[1]) = glm::vec3(0);
                update_target(targets[target_index]);
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
