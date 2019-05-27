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
/* Using the GLUT library for the body windowing setup */
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
#include <stdio.h>
#include <stdarg.h>

#include <cfenv>

#define ACCEPT_AVG_ANGLE_DISTANCE    0.001
#define ACCEPT_END_EFFECTOR_DISTANCE 0.001
#define BODY_ANGLE_SPEED             2.0
#define BODY_ELEVATION               2
#define BODY_HEIGHT                  0.125
#define BODY_SPEED                   0.05f
#define IK_FOOTING_RADIUS            0.25
#define IK_ITERS                     2
#define IK_LEG_COUNT                 3
#define IK_LEG_RADIUS                1
#define IK_SEGMENT_0_LENGTH          0.25
#define IK_SEGMENT_1_LENGTH          0.1
#define IK_SEGMENT_2_LENGTH          1.0
#define IK_SEGMENT_COUNT             3
#define IK_SEGMENT_HEIGHT            0.125
#define IK_SEGMENT_WIDTH             0.25
#define PATH_RADIUS                  0.25
#define PATH_LOW_HEIGHT              -(BODY_ELEVATION - BODY_HEIGHT)
#define PATH_HIGH_HEIGHT             (-(BODY_ELEVATION - BODY_HEIGHT) * 0.75)
#define PUMP_SHRINK_FACTOR           0.5
#define PUMP_SIDES                   6

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 600;
vt::Camera  *camera         = NULL;
vt::Mesh    *mesh_skybox    = NULL;
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
      orbit_radius      = 4,
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

vt::Mesh *base_upper = NULL,
         *base_lower = NULL,
         *body       = NULL;

struct IK_Leg
{
    vt::Mesh*              m_joint;
    std::vector<vt::Mesh*> m_ik_meshes;
    vt::Mesh*              m_rail;
};

std::vector<IK_Leg*> ik_legs;

static void create_linked_segments(vt::Scene*              scene,
                                   std::vector<vt::Mesh*>* ik_meshes,
                                   std::string             name,
                                   glm::vec2               box_dim,
                                   int                     ik_segment_count,
                                   ...)
{
    if(!scene || !ik_meshes) {
        return;
    }
    std::vector<float> ik_segment_lengths;
    va_list vl;
    va_start(vl, ik_segment_count);
    for(int i = 0; i < ik_segment_count; i++) {
        ik_segment_lengths.push_back(va_arg(vl, double));
    }
    va_end(vl);
    vt::Mesh* prev_mesh = NULL;
    float box_dim_inner_min = std::min(box_dim.x, box_dim.y) * PUMP_SHRINK_FACTOR;
    glm::vec2 box_dim_inner = glm::vec2(box_dim_inner_min, box_dim_inner_min);
    for(int i = 0; i < ik_segment_count; i++) {
        std::stringstream ss;
        ss << name << "_" << i;
        vt::Mesh* mesh = NULL;
        if(i <= 1) {
            mesh = vt::PrimitiveFactory::create_box(ss.str());
            mesh->center_axis();
        } else {
            mesh = vt::PrimitiveFactory::create_cylinder(ss.str(), PUMP_SIDES);
            mesh->center_axis();
            mesh->set_origin(glm::vec3(0, 0, 0));
            mesh->set_euler(glm::vec3(0, 90, 0));
            mesh->flatten();
        }
        mesh->set_origin(glm::vec3(0, 0, 0));
        if(i <= 1) {
            mesh->set_scale(glm::vec3(box_dim, ik_segment_lengths[i]));
        } else {
            mesh->set_scale(glm::vec3(box_dim_inner, ik_segment_lengths[i]));
        }
        mesh->flatten();
        mesh->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
        if(!prev_mesh) {
            mesh->set_origin(glm::vec3(0, 0, 0));
        } else {
            mesh->link_parent(prev_mesh, true);
            mesh->set_origin(glm::vec3(0, 0, ik_segment_lengths[i - 1])); // must go after link_parent
        }
        scene->add_mesh(mesh);
        ik_meshes->push_back(mesh);
        prev_mesh = mesh;
    }
}

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

    glm::vec3 origin = glm::vec3(0, -BODY_ELEVATION * 0.5, 0);
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_color_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    base_upper = vt::PrimitiveFactory::create_cylinder("base_upper", IK_LEG_COUNT, IK_LEG_RADIUS, BODY_HEIGHT);
    //base_upper->center_axis(vt::BBoxObject::ALIGN_CENTER);
    base_upper->set_axis(glm::vec3(0, BODY_HEIGHT * 0.5, 0));
    base_upper->set_origin(glm::vec3(0, 0, 0));
    base_upper->set_material(phong_material);
    base_upper->set_ambient_color(glm::vec3(0));
    scene->add_mesh(base_upper);

    base_lower = vt::PrimitiveFactory::create_cylinder("base_lower", IK_LEG_COUNT, IK_LEG_RADIUS, BODY_HEIGHT);
    //base_lower->center_axis(vt::BBoxObject::ALIGN_CENTER);
    base_lower->set_axis(glm::vec3(0, BODY_HEIGHT * 0.5, 0));
    base_lower->set_origin(glm::vec3(0, 0, 0) - glm::vec3(0, BODY_ELEVATION, 0));
    base_lower->set_material(phong_material);
    base_lower->set_ambient_color(glm::vec3(0));
    scene->add_mesh(base_lower);

    body = vt::PrimitiveFactory::create_cylinder("body", IK_LEG_COUNT, IK_FOOTING_RADIUS, BODY_HEIGHT);
    //body->center_axis(vt::BBoxObject::ALIGN_CENTER);
    body->set_axis(glm::vec3(0, BODY_HEIGHT * 0.5, 0));
    body->set_origin(glm::vec3(0, -BODY_ELEVATION + IK_SEGMENT_HEIGHT, 0));
    body->set_material(phong_material);
    body->set_ambient_color(glm::vec3(0));
    scene->add_mesh(body);

    for(int i = 0; i < IK_LEG_COUNT; i++) {
        float angle = i * 360 / IK_LEG_COUNT;
        IK_Leg* ik_leg = new IK_Leg();

        std::stringstream joint_name_ss;
        joint_name_ss << "joint_type_body_" << i;
        ik_leg->m_joint = vt::PrimitiveFactory::create_box(joint_name_ss.str(), IK_SEGMENT_WIDTH,
                                                                                IK_SEGMENT_WIDTH,
                                                                                IK_SEGMENT_WIDTH);
        ik_leg->m_joint->center_axis();
        ik_leg->m_joint->link_parent(body);
        ik_leg->m_joint->set_origin(vt::euler_to_offset(glm::vec3(0, 0, angle)) * static_cast<float>(IK_FOOTING_RADIUS));
        scene->add_mesh(ik_leg->m_joint);

        std::stringstream rail_name_ss;
        rail_name_ss << "ik_rail_" << i;
        ik_leg->m_rail = vt::PrimitiveFactory::create_box(rail_name_ss.str(), IK_SEGMENT_WIDTH,
                                                                              IK_SEGMENT_HEIGHT,
                                                                              BODY_ELEVATION);
        ik_leg->m_rail->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
        ik_leg->m_rail->set_origin(vt::euler_to_offset(glm::vec3(0, BODY_HEIGHT * 0.5, angle)) * static_cast<float>(IK_LEG_RADIUS));
        ik_leg->m_rail->set_euler(glm::vec3(0, 90, angle));
        ik_leg->m_rail->set_material(phong_material);
        ik_leg->m_rail->set_ambient_color(glm::vec3(0));
        scene->add_mesh(ik_leg->m_rail);

        std::vector<vt::Mesh*> &ik_meshes = ik_leg->m_ik_meshes;
        std::stringstream ik_segment_name_ss;
        ik_segment_name_ss << "ik_box_" << i;
        create_linked_segments(scene,
                               &ik_meshes,
                               ik_segment_name_ss.str(),
                               glm::vec2(IK_SEGMENT_WIDTH,
                                         IK_SEGMENT_HEIGHT),
                               IK_SEGMENT_COUNT,
                               IK_SEGMENT_0_LENGTH,
                               IK_SEGMENT_1_LENGTH,
                               IK_SEGMENT_2_LENGTH);
        int leg_segment_index = 0;
        for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); ++p) {
            (*p)->set_material(phong_material);
            (*p)->set_ambient_color(glm::vec3(0));
            (*p)->set_euler(glm::vec3(0, 90, angle));
            if(!leg_segment_index) {
                (*p)->set_joint_type(vt::TransformObject::JOINT_TYPE_PRISMATIC);
                (*p)->set_enable_joint_constraints(glm::ivec3(1, 1, 1));
                (*p)->set_joint_constraints_center(vt::euler_to_offset(glm::vec3(0, 0, angle)) * static_cast<float>(IK_LEG_RADIUS - IK_SEGMENT_HEIGHT) -
                                                   glm::vec3(0, BODY_ELEVATION * 0.5, 0));
                (*p)->set_joint_constraints_max_deviation(glm::vec3(0, BODY_ELEVATION * 0.5, 0));
            } else if(leg_segment_index == 1) {
                (*p)->set_hinge_type(vt::EULER_INDEX_PITCH);
                //(*p)->set_enable_joint_constraints(glm::ivec3(0, 0, 0));
                (*p)->set_joint_constraints_center(glm::vec3(0, 30, 0));
                (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 30, 0));
            } else {
                (*p)->set_hinge_type(vt::EULER_INDEX_YAW);
                //(*p)->set_enable_joint_constraints(glm::ivec3(0, 0, 0));
                (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
                (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 30));
            }
            leg_segment_index++;
        }
        ik_legs.push_back(ik_leg);
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
        body->set_origin(body->get_origin() - body->get_abs_left_direction() * BODY_SPEED);
        user_input = true;
    }
    if(right_key) {
        body->set_origin(body->get_origin() + body->get_abs_left_direction() * BODY_SPEED);
        user_input = true;
    }
    if(up_key) {
        body->set_origin(body->get_origin() - body->get_abs_heading() * BODY_SPEED);
        user_input = true;
    }
    if(down_key) {
        body->set_origin(body->get_origin() + body->get_abs_heading() * BODY_SPEED);
        user_input = true;
    }
    if(page_up_key) {
        body->set_origin(body->get_origin() + body->get_abs_up_direction() * BODY_SPEED);
        user_input = true;
    }
    if(page_down_key) {
        body->set_origin(body->get_origin() - body->get_abs_up_direction() * BODY_SPEED);
        user_input = true;
    }
    static int target_index = 0;
    long object_id = 0;
    std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
    body->set_origin(origin_frame_values[target_index]);
    body->get_transform(); // ensure transform is updated
    target_index = (target_index + 1) % origin_frame_values.size();
    if(user_input) {
        for(std::vector<IK_Leg*>::iterator r = ik_legs.begin(); r != ik_legs.end(); ++r) {
            std::vector<vt::Mesh*> &ik_meshes = (*r)->m_ik_meshes;
            ik_meshes[IK_SEGMENT_COUNT - 1]->solve_ik_ccd(ik_meshes[0],
                                                          glm::vec3(0, 0, IK_SEGMENT_2_LENGTH),
                                                          (*r)->m_joint->in_abs_system(),
                                                          NULL,
                                                          IK_ITERS,
                                                          ACCEPT_END_EFFECTOR_DISTANCE,
                                                          ACCEPT_AVG_ANGLE_DISTANCE);
        }
        user_input = false;
    }
    static int angle = 0;
    angle = (angle + angle_delta) % 360;
    user_input = true;
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
                base_upper->set_ambient_color(glm::vec3(1));
                base_lower->set_ambient_color(glm::vec3(1));
                body->set_ambient_color(glm::vec3(1));
                for(std::vector<IK_Leg*>::iterator q = ik_legs.begin(); q != ik_legs.end(); ++q) {
                    (*q)->m_joint->set_ambient_color(glm::vec3(1, 0, 0));
                    (*q)->m_rail->set_ambient_color(glm::vec3(1));
                    std::vector<vt::Mesh*> &ik_meshes = (*q)->m_ik_meshes;
                    for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); ++p) {
                        (*p)->set_ambient_color(glm::vec3(0, 1, 0));
                    }
                }
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
                base_upper->set_ambient_color(glm::vec3(0));
                base_lower->set_ambient_color(glm::vec3(0));
                body->set_ambient_color(glm::vec3(0));
                for(std::vector<IK_Leg*>::iterator q = ik_legs.begin(); q != ik_legs.end(); ++q) {
                    (*q)->m_joint->set_ambient_color(glm::vec3(0));
                    (*q)->m_rail->set_ambient_color(glm::vec3(0));
                    std::vector<vt::Mesh*> &ik_meshes = (*q)->m_ik_meshes;
                    for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); ++p) {
                        (*p)->set_ambient_color(glm::vec3(0));
                    }
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
                body->set_origin(glm::vec3(0, -BODY_ELEVATION + IK_SEGMENT_HEIGHT, 0));
                user_input = true;
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
