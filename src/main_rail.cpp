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
#include <shader_utils.h>

#include <Buffer.h>
#include <Camera.h>
#include <File3ds.h>
#include <FrameBuffer.h>
#include <KeyframeMgr.h>
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

#define ACCEPT_AVG_ANGLE_DISTANCE    0.001
#define ACCEPT_END_EFFECTOR_DISTANCE 0.001
#define GROUND_THICKNESS             0.125
#define IK_BASE_HEIGHT               0.5
#define IK_BASE_LENGTH               0.5
#define IK_BASE_WIDTH                0.5
#define IK_ITERS                     1
#define IK_RAIL_HEIGHT               0.25
#define IK_RAIL_LENGTH               5
#define IK_RAIL_WIDTH                0.125
#define IK_SEGMENT_COUNT             3
#define IK_SEGMENT_HEIGHT            0.25
#define IK_SEGMENT_LENGTH            1
#define IK_SEGMENT_WIDTH             0.25
#define LOCAL_TARGET_OFFSET_RADIUS   1
#define PATH_RADIUS                  0.5
#define PATH_LOW_HEIGHT              -0.25
#define PATH_HIGH_HEIGHT             0.25

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 600;
vt::Camera  *camera             = NULL;
vt::Mesh    *mesh_skybox        = NULL;
vt::Light   *light              = NULL,
            *light2             = NULL,
            *light3             = NULL;
vt::Texture *texture_box_color  = NULL,
            *texture_box_normal = NULL,
            *texture_skybox     = NULL;

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

bool angle_constraint = false;

float prev_zoom         = 0,
      zoom              = 1,
      ortho_dolly_speed = 0.1;

int angle_delta = 1;

int target_index = 0;
glm::vec3 targets[] = {glm::vec3( 1, 2,  2),
                       glm::vec3( 1, 2, -2),
                       glm::vec3(-1, 2, -2),
                       glm::vec3(-1, 2,  2),
                       glm::vec3( 2, 2,  1),
                       glm::vec3( 2, 2, -1),
                       glm::vec3(-2, 2, -1),
                       glm::vec3(-2, 2,  1)};

vt::Mesh *ground         = NULL,
         *ik_hrail       = NULL,
         *ik_vrail_dummy = NULL,
         *ik_vrail       = NULL,
         *ik_base        = NULL;

std::vector<vt::Mesh*> ik_meshes;

static void create_linked_segments(vt::Scene*              scene,
                                   std::vector<vt::Mesh*>* ik_meshes,
                                   int                     ik_segment_count,
                                   std::string             name,
                                   glm::vec3               box_dim)
{
    if(!scene || !ik_meshes) {
        return;
    }
    vt::Mesh* prev_mesh = NULL;
    for(int i = 0; i < ik_segment_count; i++) {
        std::stringstream ss;
        ss << name << "_" << i;
        vt::Mesh* mesh = vt::PrimitiveFactory::create_box(ss.str());
        mesh->center_axis();
        mesh->set_origin(glm::vec3(0, 0, 0));
        mesh->set_scale(box_dim);
        mesh->flatten();
        mesh->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
        if(!prev_mesh) {
            mesh->set_origin(glm::vec3(0, 0, 0));
        } else {
            mesh->link_parent(prev_mesh, true);
            mesh->set_origin(glm::vec3(0, 0, box_dim.z)); // must go after link_parent
        }
        scene->add_mesh(mesh);
        ik_meshes->push_back(mesh);
        prev_mesh = mesh;
    }
}

void onSpecial(int key, int x, int y);

int init_resources()
{
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

    vt::Material* bump_mapped_material = new vt::Material("bump_mapped",
                                                          "src/shaders/bump_mapped.v.glsl",
                                                          "src/shaders/bump_mapped.f.glsl");
    scene->add_material(bump_mapped_material);

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

    texture_box_color = new vt::Texture("chesterfield_color", "data/chesterfield_color.png");
    scene->add_texture(               texture_box_color);
    bump_mapped_material->add_texture(texture_box_color);

    texture_box_normal = new vt::Texture("chesterfield_normal", "data/chesterfield_normal.png");
    scene->add_texture(               texture_box_normal);
    bump_mapped_material->add_texture(texture_box_normal);

    glm::vec3 origin = glm::vec3(0);
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_color_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    ground = vt::PrimitiveFactory::create_box("ground");
    scene->add_mesh(ground);
    ground->center_axis(vt::BBoxObject::ALIGN_Y_MAX);
    ground->set_origin(glm::vec3(0));
    ground->set_scale(glm::vec3(IK_RAIL_LENGTH, GROUND_THICKNESS, IK_RAIL_LENGTH));
    ground->flatten();
    ground->set_origin(glm::vec3(0, -IK_RAIL_HEIGHT, 0));
    ground->set_material(phong_material);
    ground->set_ambient_color(glm::vec3(0));

    ik_hrail = vt::PrimitiveFactory::create_box("hrail");
    scene->add_mesh(ik_hrail);
    ik_hrail->center_axis();
    ik_hrail->set_origin(glm::vec3(0));
    ik_hrail->set_scale(glm::vec3(IK_RAIL_LENGTH, IK_RAIL_HEIGHT, IK_RAIL_WIDTH));
    ik_hrail->flatten();
    ik_hrail->set_material(phong_material);
    ik_hrail->set_ambient_color(glm::vec3(0));
    ik_hrail->set_joint_type(vt::TransformObject::JOINT_TYPE_PRISMATIC);
    ik_hrail->set_enable_joint_constraints(glm::ivec3(1, 1, 1));
    ik_hrail->set_joint_constraints_center(glm::vec3(0, 0, 0));
    ik_hrail->set_joint_constraints_max_deviation(glm::vec3(0, 0, IK_RAIL_LENGTH * 0.5));

    ik_vrail = vt::PrimitiveFactory::create_box("vrail");
    scene->add_mesh(ik_vrail);
    ik_vrail->center_axis();
    ik_vrail->set_origin(glm::vec3(0));
    ik_vrail->set_scale(glm::vec3(IK_RAIL_WIDTH, IK_RAIL_HEIGHT, IK_RAIL_LENGTH));
    ik_vrail->flatten();
    ik_vrail->set_material(phong_material);
    ik_vrail->set_ambient_color(glm::vec3(0));

    ik_vrail_dummy = vt::PrimitiveFactory::create_box("vrail_ghost");
    scene->add_mesh(ik_vrail_dummy);
    ik_vrail_dummy->center_axis();
    ik_vrail_dummy->set_origin(glm::vec3(0));
    ik_vrail_dummy->flatten();
    ik_vrail_dummy->set_joint_type(vt::TransformObject::JOINT_TYPE_PRISMATIC);
    ik_vrail_dummy->set_enable_joint_constraints(glm::ivec3(1, 1, 1));
    ik_vrail_dummy->set_joint_constraints_center(glm::vec3(0, 0, 0));
    ik_vrail_dummy->set_joint_constraints_max_deviation(glm::vec3(IK_RAIL_LENGTH * 0.5, 0, 0));
    ik_vrail_dummy->link_parent(ik_hrail);

    ik_base = vt::PrimitiveFactory::create_cylinder("base", 8, IK_BASE_WIDTH * 0.5, IK_BASE_HEIGHT);
    scene->add_mesh(ik_base);
    ik_base->center_axis();
    ik_base->set_origin(glm::vec3(0));
    ik_base->flatten();
    ik_base->set_material(phong_material);
    ik_base->set_ambient_color(glm::vec3(0));
    ik_base->set_joint_type(vt::TransformObject::JOINT_TYPE_REVOLUTE);
    ik_base->set_enable_joint_constraints(glm::ivec3(1, 1, 0));
    ik_base->set_joint_constraints_center(glm::vec3(0, 0, 0));
    ik_base->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
    ik_base->link_parent(ik_vrail_dummy);

    create_linked_segments(scene,
                           &ik_meshes,
                           IK_SEGMENT_COUNT,
                           "ik_box",
                           glm::vec3(IK_SEGMENT_WIDTH,
                                     IK_SEGMENT_HEIGHT,
                                     IK_SEGMENT_LENGTH));
    if(ik_meshes.size()) {
        ik_meshes[0]->set_origin(glm::vec3(0));
    }
    ik_meshes[0]->link_parent(ik_base);
    for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); ++p) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
        (*p)->set_hinge_type(vt::EULER_INDEX_PITCH);
        //(*p)->set_enable_joint_constraints(glm::ivec3(0, 0, 0));
        (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
        (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 120, 0));
    }

    long object_id = 0;
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 0,   new vt::Keyframe(glm::vec3( PATH_RADIUS, PATH_LOW_HEIGHT,   PATH_RADIUS), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 25,  new vt::Keyframe(glm::vec3(-PATH_RADIUS, PATH_HIGH_HEIGHT,  PATH_RADIUS), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 50,  new vt::Keyframe(glm::vec3(-PATH_RADIUS, PATH_LOW_HEIGHT,  -PATH_RADIUS), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 75,  new vt::Keyframe(glm::vec3( PATH_RADIUS, PATH_HIGH_HEIGHT, -PATH_RADIUS), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 100, new vt::Keyframe(glm::vec3( PATH_RADIUS, PATH_LOW_HEIGHT,   PATH_RADIUS), true));
    vt::KeyframeMgr::instance()->update_control_points(0.5);
    std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
    vt::KeyframeMgr::instance()->export_frame_values_for_object(object_id, &origin_frame_values, NULL, NULL, true);
    std::vector<glm::vec3> &origin_keyframe_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_keyframe_values;
    vt::KeyframeMgr::instance()->export_keyframe_values_for_object(object_id, &origin_keyframe_values, NULL, NULL, true);

    onSpecial(GLUT_KEY_HOME, 0, 0);

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
    static int angle = 0;
    static glm::vec3 end_effector_dir = glm::vec3(0, 1, 0);
    long object_id = 0;
    std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
    if(origin_frame_values.size()) {
        static int frame_target_index = 0;
        ik_meshes[IK_SEGMENT_COUNT - 1]->solve_ik_ccd(ik_hrail,
                                                      glm::vec3(0, 0, IK_SEGMENT_LENGTH),
                                                      glm::vec3(vt::Scene::instance()->m_debug_object_context[object_id].m_transform *
                                                                glm::vec4(origin_frame_values[frame_target_index + 1], 1)),
                                                      angle_constraint ? &end_effector_dir : NULL,
                                                      IK_ITERS,
                                                      ACCEPT_END_EFFECTOR_DISTANCE,
                                                      ACCEPT_AVG_ANGLE_DISTANCE);
        frame_target_index = (frame_target_index + 1) % (origin_frame_values.size() - 1);
    }
    ik_vrail->set_origin(ik_vrail_dummy->get_origin());
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
        case 'c': // angle constraint
            angle_constraint = !angle_constraint;
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
        case 's': // paths
            show_paths = !show_paths;
            break;
        case 'w': // wireframe
            wireframe_mode = !wireframe_mode;
            if(wireframe_mode) {
                glPolygonMode(GL_FRONT, GL_LINE);
                ground->set_ambient_color(glm::vec3(1));
                ik_hrail->set_ambient_color(glm::vec3(0, 1, 0));
                ik_vrail->set_ambient_color(glm::vec3(0, 1, 0));
                ik_vrail_dummy->set_ambient_color(glm::vec3(1, 0, 0));
                ik_base->set_ambient_color(glm::vec3(0, 1, 0));
                for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); ++p) {
                    (*p)->set_ambient_color(glm::vec3(0, 1, 0));
                }
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
                ground->set_ambient_color(glm::vec3(0));
                ik_hrail->set_ambient_color(glm::vec3(0));
                ik_vrail->set_ambient_color(glm::vec3(0));
                ik_vrail_dummy->set_ambient_color(glm::vec3(0));
                ik_base->set_ambient_color(glm::vec3(0));
                for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); ++p) {
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
                vt::Scene* scene = vt::Scene::instance();
                scene->m_debug_targets.clear();
                scene->m_debug_targets.push_back(std::make_tuple(targets[target_index], glm::vec3(1, 0, 1), 1, 1));
                long object_id = 0;
                vt::Scene::instance()->m_debug_object_context[object_id].m_transform = glm::translate(glm::mat4(1), targets[target_index]);
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
