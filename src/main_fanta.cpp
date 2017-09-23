/**
 * Based on Sylvain Beucler's tutorial from the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Author: Jerry Chen
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

#define ACCEPT_AVG_ANGLE_DISTANCE    0.001
#define ACCEPT_END_EFFECTOR_DISTANCE 0.001
#define BODY_ELEVATION               (0.6 + 0.1)
#define IK_ARM_DISTANCE              3
#define IK_BASE_HEIGHT               0.5
#define IK_BASE_LENGTH               0.5
#define IK_BASE_WIDTH                0.5
#define IK_ITERS                     1
#define IK_SEGMENT_COUNT             4
#define IK_SEGMENT_HEIGHT            0.125
#define IK_SEGMENT_LENGTH            1.5
#define IK_SEGMENT_WIDTH             0.125
#define PATH_RADIUS                  0.5

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

bool angle_constraint = false;

float prev_zoom         = 0,
      zoom              = 1,
      ortho_dolly_speed = 0.1;

int angle_delta = 1;

vt::Mesh *ik_base  = NULL,
         *ik_base2 = NULL,
         *ik_base3 = NULL;

std::vector<vt::Mesh*> ik_meshes;
std::vector<vt::Mesh*> ik_meshes2;
std::vector<vt::Mesh*> ik_meshes3;

std::vector<vt::Mesh*> tray_meshes;
std::vector<vt::Mesh*> tray_meshes2;
std::vector<vt::Mesh*> tray_meshes3;
std::vector<vt::Mesh*> tray_handles;

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

static void create_tray_meshes(vt::Scene*              scene,
                               std::vector<vt::Mesh*>* tray_meshes,
                               std::vector<vt::Mesh*>* tray_handles,
                               std::string             name,
                               bool                    spike_only)
{
    if(!scene || !tray_meshes) {
        return;
    }

    vt::Mesh* spike = vt::PrimitiveFactory::create_cylinder(name + "_spike", 8, 0.05, 0.6);
    spike->set_origin(glm::vec3(0, 0, 0));
    tray_meshes->push_back(spike);
    scene->add_mesh(spike);

    if(tray_handles) {
        vt::Mesh* spike_handle = vt::PrimitiveFactory::create_box(name + "_spike_handle");
        spike_handle->link_parent(spike);
        spike_handle->center_axis();
        spike_handle->set_origin(glm::vec3(0, 0, 0));
        spike_handle->set_scale(glm::vec3(0.1, 0.1, 0.1));
        spike_handle->flatten();
        spike_handle->set_origin(glm::vec3(0, 0.6, 0));
        tray_handles->push_back(spike_handle);
        scene->add_mesh(spike_handle);
    }

    if(spike_only) {
        return;
    }

    vt::Mesh* tray = vt::PrimitiveFactory::create_box(name + "_tray");
    tray->link_parent(spike);
    //tray->center_axis();
    tray->set_origin(glm::vec3(-0.55, 0.6, -0.8));
    tray->set_scale(glm::vec3(1.1, 0.1, 1.6));
    tray->flatten();
    tray_meshes->push_back(tray);
    scene->add_mesh(tray);

    if(tray_handles) {
        vt::Mesh* tray_handle = vt::PrimitiveFactory::create_box(name + "_tray_handle");
        tray_handle->link_parent(spike);
        tray_handle->center_axis();
        tray_handle->set_origin(glm::vec3(0, 0, 0));
        tray_handle->set_scale(glm::vec3(0.1, 0.1, 0.1));
        tray_handle->flatten();
        tray_handle->set_origin(glm::vec3(0.55, 0.6, 0));
        tray_handles->push_back(tray_handle);
        scene->add_mesh(tray_handle);
    }

    glm::vec3 offset(-0.55, 0.6 + 0.1, -0.8);
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 3; j++) {
            std::stringstream ss;
            ss << name << "_fanta_can_" << i;
            vt::Mesh* fanta_can = vt::PrimitiveFactory::create_cylinder(ss.str(), 8, 0.2, 0.6);
            fanta_can->link_parent(tray);
            fanta_can->set_origin(glm::vec3((i * 0.5f) + 0.3, 0, (j * 0.5f) + 0.3) + offset);
            tray_meshes->push_back(fanta_can);
            scene->add_mesh(fanta_can);
        }
    }
}

int init_resources()
{
    vt::Scene* scene = vt::Scene::instance();

    mesh_skybox = vt::PrimitiveFactory::create_viewport_quad("grid");
    scene->set_skybox(mesh_skybox);

    vt::Material* ambient_material = new vt::Material(
            "ambient",
            "src/shaders/ambient.v.glsl",
            "src/shaders/ambient.f.glsl");
    scene->add_material(ambient_material);
    scene->set_wireframe_material(ambient_material);

    vt::Material* skybox_material = new vt::Material(
            "skybox",
            "src/shaders/skybox.v.glsl",
            "src/shaders/skybox.f.glsl",
            true); // use_overlay
    scene->add_material(skybox_material);

    vt::Material* bump_mapped_material = new vt::Material(
            "bump_mapped",
            "src/shaders/bump_mapped.v.glsl",
            "src/shaders/bump_mapped.f.glsl");
    scene->add_material(bump_mapped_material);

    vt::Material* phong_material = new vt::Material(
            "phong",
            "src/shaders/phong.v.glsl",
            "src/shaders/phong.f.glsl");
    scene->add_material(phong_material);

    texture_skybox = new vt::Texture(
            "skybox_texture",
            "data/SaintPetersSquare2/posx.png",
            "data/SaintPetersSquare2/negx.png",
            "data/SaintPetersSquare2/posy.png",
            "data/SaintPetersSquare2/negy.png",
            "data/SaintPetersSquare2/posz.png",
            "data/SaintPetersSquare2/negz.png");
    scene->add_texture(          texture_skybox);
    skybox_material->add_texture(texture_skybox);

    texture_box_color = new vt::Texture(
            "chesterfield_color",
            "data/chesterfield_color.png");
    scene->add_texture(               texture_box_color);
    bump_mapped_material->add_texture(texture_box_color);

    texture_box_normal = new vt::Texture(
            "chesterfield_normal",
            "data/chesterfield_normal.png");
    scene->add_texture(               texture_box_normal);
    bump_mapped_material->add_texture(texture_box_normal);

    glm::vec3 origin = glm::vec3(0);
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    ik_base = vt::PrimitiveFactory::create_box("base");
    scene->add_mesh(ik_base);
    ik_base->center_axis();
    ik_base->set_scale(glm::vec3(IK_BASE_WIDTH, IK_BASE_HEIGHT, IK_BASE_LENGTH));
    ik_base->flatten();
    ik_base->center_axis();
    ik_base->set_origin(glm::vec3(IK_ARM_DISTANCE, 0, 0));
    ik_base->set_euler(glm::vec3(0, -90, 0));
    ik_base->set_material(phong_material);
    ik_base->set_ambient_color(glm::vec3(0));

    ik_base2 = vt::PrimitiveFactory::create_box("base2");
    scene->add_mesh(ik_base2);
    ik_base2->center_axis();
    ik_base2->set_scale(glm::vec3(IK_BASE_WIDTH, IK_BASE_HEIGHT, IK_BASE_LENGTH));
    ik_base2->flatten();
    ik_base2->center_axis();
    ik_base2->set_origin(glm::vec3(-IK_ARM_DISTANCE, 0, 0));
    ik_base2->set_euler(glm::vec3(0, -90, 0));
    ik_base2->set_material(phong_material);
    ik_base2->set_ambient_color(glm::vec3(0));

    ik_base3 = vt::PrimitiveFactory::create_box("base3");
    scene->add_mesh(ik_base3);
    ik_base3->center_axis();
    ik_base3->set_scale(glm::vec3(IK_BASE_WIDTH, IK_BASE_HEIGHT, IK_BASE_LENGTH));
    ik_base3->flatten();
    ik_base3->center_axis();
    ik_base3->set_origin(glm::vec3(0, 0, -IK_ARM_DISTANCE));
    ik_base3->set_euler(glm::vec3(0, -90, 0));
    ik_base3->set_material(phong_material);
    ik_base3->set_ambient_color(glm::vec3(0));

    create_linked_segments(scene,
                           &ik_meshes,
                           IK_SEGMENT_COUNT,
                           "ik_box",
                           glm::vec3(IK_SEGMENT_WIDTH,
                                     IK_SEGMENT_HEIGHT,
                                     IK_SEGMENT_LENGTH));
    if(ik_meshes.size()) {
        ik_meshes[0]->link_parent(ik_base);
    }
    int leg_segment_index = 0;
    for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); p++) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
        if(!leg_segment_index) {
            (*p)->set_enable_joint_constraints(glm::ivec3(0, 1, 1));
            (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
            (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
        } else {
            (*p)->set_enable_joint_constraints(glm::ivec3(1, 0, 1));
            (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
            (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
        }
        leg_segment_index++;
    }

    create_linked_segments(scene,
                           &ik_meshes2,
                           IK_SEGMENT_COUNT,
                           "ik_box2",
                           glm::vec3(IK_SEGMENT_WIDTH,
                                     IK_SEGMENT_HEIGHT,
                                     IK_SEGMENT_LENGTH));
    if(ik_meshes2.size()) {
        ik_meshes2[0]->link_parent(ik_base2);
    }
    int leg_segment_index2 = 0;
    for(std::vector<vt::Mesh*>::iterator p = ik_meshes2.begin(); p != ik_meshes2.end(); p++) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
        if(!leg_segment_index2) {
            (*p)->set_enable_joint_constraints(glm::ivec3(0, 1, 1));
            (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
            (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
        } else {
            (*p)->set_enable_joint_constraints(glm::ivec3(1, 0, 1));
            (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
            (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
        }
        leg_segment_index2++;
    }

    create_linked_segments(scene,
                           &ik_meshes3,
                           IK_SEGMENT_COUNT,
                           "ik_box3",
                           glm::vec3(IK_SEGMENT_WIDTH,
                                     IK_SEGMENT_HEIGHT,
                                     IK_SEGMENT_LENGTH));
    if(ik_meshes3.size()) {
        ik_meshes3[0]->link_parent(ik_base3);
    }
    int leg_segment_index3 = 0;
    for(std::vector<vt::Mesh*>::iterator p = ik_meshes3.begin(); p != ik_meshes3.end(); p++) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
        if(!leg_segment_index3) {
            (*p)->set_enable_joint_constraints(glm::ivec3(0, 1, 1));
            (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
            (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
        } else {
            (*p)->set_enable_joint_constraints(glm::ivec3(1, 0, 1));
            (*p)->set_joint_constraints_center(glm::vec3(0, 0, 0));
            (*p)->set_joint_constraints_max_deviation(glm::vec3(0, 0, 0));
        }
        leg_segment_index3++;
    }

    create_tray_meshes(scene, &tray_meshes, &tray_handles, "group1", false);
    for(std::vector<vt::Mesh*>::iterator p = tray_meshes.begin(); p != tray_meshes.end(); p++) {
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
    }

    create_tray_meshes(scene, &tray_meshes2, &tray_handles, "group2", false);
    for(std::vector<vt::Mesh*>::iterator q = tray_meshes2.begin(); q != tray_meshes2.end(); q++) {
        (*q)->set_material(phong_material);
        (*q)->set_ambient_color(glm::vec3(0));
    }
    tray_meshes2[0]->link_parent(tray_meshes[0]);

    create_tray_meshes(scene, &tray_meshes3, &tray_handles, "group3", true);
    for(std::vector<vt::Mesh*>::iterator r = tray_meshes3.begin(); r != tray_meshes3.end(); r++) {
        (*r)->set_material(phong_material);
        (*r)->set_ambient_color(glm::vec3(0));
    }
    tray_meshes3[0]->link_parent(tray_meshes2[0]);

    long object_id = 0;
    float height = BODY_ELEVATION + 0.3;
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 0,   new vt::Keyframe(glm::vec3( 0,    height, -0.5),  true, 1, 0));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 50,  new vt::Keyframe(glm::vec3( 0,    height,  0.5),  true, 0, 1));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 75,  new vt::Keyframe(glm::vec3( 0.25, height,  0.75), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 100, new vt::Keyframe(glm::vec3( 0.5,  height,  0.5),  true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 125, new vt::Keyframe(glm::vec3( 0.25, height,  0.25), true, 1, 0));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 150, new vt::Keyframe(glm::vec3(-0.25, height,  0.25), true, 0, 1));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 175, new vt::Keyframe(glm::vec3(-0.5,  height,  0),    true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 200, new vt::Keyframe(glm::vec3(-0.25, height, -0.25), true, 1, 0));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 225, new vt::Keyframe(glm::vec3( 0.25, height, -0.25), true, 0, 1));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 250, new vt::Keyframe(glm::vec3( 0.5,  height, -0.5),  true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 275, new vt::Keyframe(glm::vec3( 0.25, height, -0.75), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id, vt::MotionTrack::MOTION_TYPE_ORIGIN, 300, new vt::Keyframe(glm::vec3( 0,    height, -0.5),  true));
    vt::KeyframeMgr::instance()->update_control_points(0.5);
    std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
    vt::KeyframeMgr::instance()->export_frame_values_for_object(object_id, &origin_frame_values, NULL, NULL, true);
    std::vector<glm::vec3> &origin_keyframe_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_keyframe_values;
    vt::KeyframeMgr::instance()->export_keyframe_values_for_object(object_id, &origin_keyframe_values, NULL, NULL, true);

    long object_id2 = 1;
    float height2 = BODY_ELEVATION + 0.3;
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 0,   new vt::Keyframe(glm::vec3( 0,    height2, -0.5),  true, 1, 0));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 50,  new vt::Keyframe(glm::vec3( 0,    height2,  0.5),  true, 0, 1));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 75,  new vt::Keyframe(glm::vec3( 0.25, height2,  0.75), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 100, new vt::Keyframe(glm::vec3( 0.5,  height2,  0.5),  true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 125, new vt::Keyframe(glm::vec3( 0.25, height2,  0.25), true, 1, 0));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 150, new vt::Keyframe(glm::vec3(-0.25, height2,  0.25), true, 0, 1));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 175, new vt::Keyframe(glm::vec3(-0.5,  height2,  0),    true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 200, new vt::Keyframe(glm::vec3(-0.25, height2, -0.25), true, 1, 0));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 225, new vt::Keyframe(glm::vec3( 0.25, height2, -0.25), true, 0, 1));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 250, new vt::Keyframe(glm::vec3( 0.5,  height2, -0.5),  true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 275, new vt::Keyframe(glm::vec3( 0.25, height2, -0.75), true));
    vt::KeyframeMgr::instance()->insert_keyframe(object_id2, vt::MotionTrack::MOTION_TYPE_ORIGIN, 300, new vt::Keyframe(glm::vec3( 0,    height2, -0.5),  true));
    vt::KeyframeMgr::instance()->update_control_points(0.5);
    std::vector<glm::vec3> &origin_frame_values2 = vt::Scene::instance()->m_debug_object_context[object_id2].m_debug_origin_frame_values;
    vt::KeyframeMgr::instance()->export_frame_values_for_object(object_id2, &origin_frame_values2, NULL, NULL, true);
    std::vector<glm::vec3> &origin_keyframe_values2 = vt::Scene::instance()->m_debug_object_context[object_id2].m_debug_origin_keyframe_values;
    vt::KeyframeMgr::instance()->export_keyframe_values_for_object(object_id2, &origin_keyframe_values2, NULL, NULL, true);

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
    if(left_key) {
        tray_meshes[0]->rotate(-angle_delta, tray_meshes[0]->get_abs_up_direction());
        user_input = true;
    }
    if(right_key) {
        tray_meshes[0]->rotate(angle_delta, tray_meshes[0]->get_abs_up_direction());
        user_input = true;
    }
    if(up_key) {
        tray_meshes[0]->rotate(-angle_delta, tray_meshes[0]->get_abs_left_direction());
        user_input = true;
    }
    if(down_key) {
        tray_meshes[0]->rotate(angle_delta, tray_meshes[0]->get_abs_left_direction());
        user_input = true;
    }
    if(page_up_key) {
        tray_meshes[0]->rotate(angle_delta, tray_meshes[0]->get_abs_heading());
        user_input = true;
    }
    if(page_down_key) {
        tray_meshes[0]->rotate(-angle_delta, tray_meshes[0]->get_abs_heading());
        user_input = true;
    }
    if(user_input) {
        glm::vec3 end_effector_euler;
        if(angle_constraint) {
            end_effector_euler = glm::vec3(0, -1, 0);
        }
        ik_meshes[IK_SEGMENT_COUNT - 1]->solve_ik_ccd(ik_meshes[0],
                                                      glm::vec3(0, 0, IK_SEGMENT_LENGTH),
                                                      tray_handles[4]->in_abs_system(),
                                                      angle_constraint ? &end_effector_euler : NULL,
                                                      IK_ITERS,
                                                      ACCEPT_END_EFFECTOR_DISTANCE,
                                                      ACCEPT_AVG_ANGLE_DISTANCE);
        ik_meshes2[IK_SEGMENT_COUNT - 1]->solve_ik_ccd(ik_meshes2[0],
                                                       glm::vec3(0, 0, IK_SEGMENT_LENGTH),
                                                       tray_handles[3]->in_abs_system(),
                                                       angle_constraint ? &end_effector_euler : NULL,
                                                       IK_ITERS,
                                                       ACCEPT_END_EFFECTOR_DISTANCE,
                                                       ACCEPT_AVG_ANGLE_DISTANCE);
        ik_meshes3[IK_SEGMENT_COUNT - 1]->solve_ik_ccd(ik_meshes3[0],
                                                       glm::vec3(0, 0, IK_SEGMENT_LENGTH),
                                                       tray_handles[1]->in_abs_system(),
                                                       angle_constraint ? &end_effector_euler : NULL,
                                                       IK_ITERS,
                                                       ACCEPT_END_EFFECTOR_DISTANCE,
                                                       ACCEPT_AVG_ANGLE_DISTANCE);
        user_input = false;
    }
    static int angle = 0;
    tray_meshes[0]->set_euler(glm::vec3(0, -15, angle));
    angle = (angle + angle_delta) % 360;

    static int target_index = 0;
    long object_id = 0;
    std::vector<glm::vec3> &origin_frame_values = vt::Scene::instance()->m_debug_object_context[object_id].m_debug_origin_frame_values;
    tray_meshes2[0]->set_origin(origin_frame_values[target_index]);
    vt::Scene::instance()->m_debug_object_context[object_id].m_transform = tray_meshes2[0]->get_parent()->get_transform();
    target_index = (target_index + 1) % origin_frame_values.size();

    static int target_index2 = 0;
    long object_id2 = 1;
    std::vector<glm::vec3> &origin_frame_values2 = vt::Scene::instance()->m_debug_object_context[object_id2].m_debug_origin_frame_values;
    tray_meshes3[0]->set_origin(origin_frame_values2[target_index]);
    vt::Scene::instance()->m_debug_object_context[object_id2].m_transform = tray_meshes3[0]->get_parent()->get_transform();
    target_index2 = (target_index2 + 1) % origin_frame_values2.size();

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
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
            user_input = true;
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
                ik_base->set_ambient_color(glm::vec3(0, 1, 0));
                ik_base2->set_ambient_color(glm::vec3(0, 1, 0));
                ik_base3->set_ambient_color(glm::vec3(0, 1, 0));
                for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); p++) {
                    (*p)->set_ambient_color(glm::vec3(0, 1, 0));
                }
                for(std::vector<vt::Mesh*>::iterator p2 = ik_meshes2.begin(); p2 != ik_meshes2.end(); p2++) {
                    (*p2)->set_ambient_color(glm::vec3(0, 1, 0));
                }
                for(std::vector<vt::Mesh*>::iterator p3 = ik_meshes3.begin(); p3 != ik_meshes3.end(); p3++) {
                    (*p3)->set_ambient_color(glm::vec3(0, 1, 0));
                }
                for(std::vector<vt::Mesh*>::iterator q = tray_meshes.begin(); q != tray_meshes.end(); q++) {
                    (*q)->set_ambient_color(glm::vec3(1));
                }
                for(std::vector<vt::Mesh*>::iterator r = tray_meshes2.begin(); r != tray_meshes2.end(); r++) {
                    (*r)->set_ambient_color(glm::vec3(1));
                }
                for(std::vector<vt::Mesh*>::iterator t = tray_meshes3.begin(); t != tray_meshes3.end(); t++) {
                    (*t)->set_ambient_color(glm::vec3(1));
                }
                for(std::vector<vt::Mesh*>::iterator u = tray_handles.begin(); u != tray_handles.end(); u++) {
                    (*u)->set_ambient_color(glm::vec3(1, 0, 0));
                }
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
                ik_base->set_ambient_color(glm::vec3(0));
                ik_base2->set_ambient_color(glm::vec3(0));
                ik_base3->set_ambient_color(glm::vec3(0));
                for(std::vector<vt::Mesh*>::iterator p = ik_meshes.begin(); p != ik_meshes.end(); p++) {
                    (*p)->set_ambient_color(glm::vec3(0));
                }
                for(std::vector<vt::Mesh*>::iterator p2 = ik_meshes2.begin(); p2 != ik_meshes2.end(); p2++) {
                    (*p2)->set_ambient_color(glm::vec3(0));
                }
                for(std::vector<vt::Mesh*>::iterator p3 = ik_meshes3.begin(); p3 != ik_meshes3.end(); p3++) {
                    (*p3)->set_ambient_color(glm::vec3(0));
                }
                for(std::vector<vt::Mesh*>::iterator q = tray_meshes.begin(); q != tray_meshes.end(); q++) {
                    (*q)->set_ambient_color(glm::vec3(0));
                }
                for(std::vector<vt::Mesh*>::iterator r = tray_meshes2.begin(); r != tray_meshes2.end(); r++) {
                    (*r)->set_ambient_color(glm::vec3(0));
                }
                for(std::vector<vt::Mesh*>::iterator t = tray_meshes3.begin(); t != tray_meshes3.end(); t++) {
                    (*t)->set_ambient_color(glm::vec3(0));
                }
                for(std::vector<vt::Mesh*>::iterator u = tray_handles.begin(); u != tray_handles.end(); u++) {
                    (*u)->set_ambient_color(glm::vec3(0));
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
                tray_meshes[0]->set_euler(glm::vec3(0));
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
