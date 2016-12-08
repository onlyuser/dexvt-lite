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
#include <glm/gtx/string_cast.hpp>
#include <shader_utils.h>

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

const char* DEFAULT_CAPTION = "My Textured Cube";

int init_screen_width = 800, init_screen_height = 600;
vt::Camera* camera;
vt::Mesh *mesh_skybox, *mesh_box, *mesh_box2, *mesh_box3, *mesh_box4, *mesh_box5, *mesh_box6;
vt::Light *light, *light2, *light3;
vt::Texture *texture_box_color, *texture_box_normal, *texture_skybox;

bool left_mouse_down = false, right_mouse_down = false;
glm::vec2 prev_mouse_coord, mouse_drag;
glm::vec3 prev_orient, orient, orbit_speed = glm::vec3(0, -0.5, -0.5);
float prev_orbit_radius = 0, orbit_radius = 8, dolly_speed = 0.1, light_distance = 4;
bool wireframe_mode = false;
bool show_fps = false;
bool show_axis = false;
bool show_axis_labels = false;
bool show_bbox = false;
bool show_normals = false;
bool show_lights = false;
bool do_animation = true;

int texture_id = 0;
float prev_zoom = 0, zoom = 1, ortho_dolly_speed = 0.1;

int target_index = 0;
glm::vec3 targets[] = {glm::vec3( 1, 1,  1),
                       glm::vec3( 1, 1, -1),
                       glm::vec3(-1, 1, -1),
                       glm::vec3(-1, 1,  1)};

int init_resources()
{
    vt::Scene* scene = vt::Scene::instance();

    mesh_skybox = vt::PrimitiveFactory::create_viewport_quad("grid");
    scene->set_skybox(mesh_skybox);

    scene->add_mesh(mesh_box  = vt::PrimitiveFactory::create_box("box"));
    mesh_box->center_axis();
    mesh_box->set_origin(glm::vec3(0, 0, 0));
    mesh_box->set_scale(glm::vec3(0.25, 0.25, 1));
    mesh_box->rebase();
    mesh_box->center_axis(vt::BBoxObject::ALIGN_Z_MIN);

    scene->add_mesh(mesh_box2 = vt::PrimitiveFactory::create_box("box2"));
    mesh_box2->center_axis();
    mesh_box2->set_origin(glm::vec3(0, 0, 1));
    mesh_box2->set_scale(glm::vec3(0.25, 0.25, 1));
    mesh_box2->rebase();
    mesh_box2->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
    mesh_box2->link_parent(mesh_box, true);

    scene->add_mesh(mesh_box3 = vt::PrimitiveFactory::create_box("box3"));
    mesh_box3->center_axis();
    mesh_box3->set_origin(glm::vec3(0, 0, 2));
    mesh_box3->set_scale(glm::vec3(0.25, 0.25, 1));
    mesh_box3->rebase();
    mesh_box3->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
    mesh_box3->link_parent(mesh_box2, true);

    scene->add_mesh(mesh_box4 = vt::PrimitiveFactory::create_box("box4"));
    mesh_box4->center_axis();
    mesh_box4->set_origin(glm::vec3(0, 0, 3));
    mesh_box4->set_scale(glm::vec3(0.25, 0.25, 1));
    mesh_box4->rebase();
    mesh_box4->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
    mesh_box4->link_parent(mesh_box3, true);

    scene->add_mesh(mesh_box5 = vt::PrimitiveFactory::create_box("box5"));
    mesh_box5->center_axis();
    mesh_box5->set_origin(glm::vec3(0, 0, 4));
    mesh_box5->set_scale(glm::vec3(0.25, 0.25, 1));
    mesh_box5->rebase();
    mesh_box5->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
    mesh_box5->link_parent(mesh_box4, true);

    scene->add_mesh(mesh_box6 = vt::PrimitiveFactory::create_box("box6"));
    mesh_box6->center_axis();
    mesh_box6->set_origin(glm::vec3(0, 0, 5));
    mesh_box6->set_scale(glm::vec3(0.25, 0.25, 1));
    mesh_box6->rebase();
    mesh_box6->center_axis(vt::BBoxObject::ALIGN_Z_MIN);
    mesh_box6->link_parent(mesh_box5, true);

    mesh_box->set_origin(glm::vec3(0, 0, 0));

    vt::Material* bump_mapped_material = new vt::Material(
            "bump_mapped",
            "src/shaders/bump_mapped.v.glsl",
            "src/shaders/bump_mapped.f.glsl");
    scene->add_material(bump_mapped_material);

    vt::Material* skybox_material = new vt::Material(
            "skybox",
            "src/shaders/skybox.v.glsl",
            "src/shaders/skybox.f.glsl",
            true); // use_overlay
    scene->add_material(skybox_material);

    vt::Material* ambient_material = new vt::Material(
            "ambient",
            "src/shaders/ambient.v.glsl",
            "src/shaders/ambient.f.glsl");
    scene->add_material(ambient_material);
    scene->set_wireframe_material(ambient_material);

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

    glm::vec3 origin = glm::vec3();
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    // box
    mesh_box->set_material(bump_mapped_material);
    mesh_box->set_texture_index(     mesh_box->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh_box->set_bump_texture_index(mesh_box->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh_box->set_ambient_color(glm::vec3(0, 0, 0));

    // box2
    mesh_box2->set_material(bump_mapped_material);
    mesh_box2->set_texture_index(     mesh_box2->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh_box2->set_bump_texture_index(mesh_box2->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh_box2->set_ambient_color(glm::vec3(0, 0, 0));

    // box3
    mesh_box3->set_material(bump_mapped_material);
    mesh_box3->set_texture_index(     mesh_box3->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh_box3->set_bump_texture_index(mesh_box3->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh_box3->set_ambient_color(glm::vec3(0, 0, 0));

    // box4
    mesh_box4->set_material(bump_mapped_material);
    mesh_box4->set_texture_index(     mesh_box4->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh_box4->set_bump_texture_index(mesh_box4->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh_box4->set_ambient_color(glm::vec3(0, 0, 0));

    // box5
    mesh_box5->set_material(bump_mapped_material);
    mesh_box5->set_texture_index(     mesh_box5->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh_box5->set_bump_texture_index(mesh_box5->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh_box5->set_ambient_color(glm::vec3(0, 0, 0));

    // box6
    mesh_box6->set_material(bump_mapped_material);
    mesh_box6->set_texture_index(     mesh_box6->get_material()->get_texture_index_by_name("chesterfield_color"));
    mesh_box6->set_bump_texture_index(mesh_box6->get_material()->get_texture_index_by_name("chesterfield_normal"));
    mesh_box6->set_ambient_color(glm::vec3(0, 0, 0));

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
    static int angle = 0;
    mesh_box->set_orient(glm::vec3(0, 0, angle));
    //mesh_box2->set_orient(glm::vec3(angle, 0, 0));
    //mesh_box3->set_orient(glm::vec3(0, angle, 0));
    //mesh_box->point_at(targets[target_index]);
    mesh_box6->solve_ik_ccd(mesh_box2, glm::vec3(0, 0, 1), targets[target_index], 10);
    angle = (angle + 1) % 360;
}

void onDisplay()
{
    if(do_animation) {
        onTick();
    }

    vt::Scene* scene = vt::Scene::instance();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(wireframe_mode) {
        scene->render(true, false, false, vt::Scene::use_material_type_t::USE_WIREFRAME_MATERIAL);
    } else {
        scene->render();
    }

    if(show_axis || show_axis_labels || show_bbox || show_normals) {
        scene->render_lines(show_axis, show_axis_labels, show_bbox, show_normals);
    }
    if(show_lights) {
        scene->render_lights();
    }
    glutSwapBuffers();
}

void set_mesh_visibility(bool visible)
{
    mesh_box->set_visible(visible); // box
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
        case 't': // target
            {
                size_t target_count = sizeof(targets)/sizeof(targets[0]);
                target_index = (target_index + 1) % target_count;
                std::cout << "target #" << target_index << ": " << glm::to_string(targets[target_index]) << std::endl;
            }
            break;
        case 'w': // wireframe
            wireframe_mode = !wireframe_mode;
            if(wireframe_mode) {
                glPolygonMode(GL_FRONT, GL_LINE);
                mesh_box->set_ambient_color(glm::vec3(1, 1, 1));
                mesh_box2->set_ambient_color(glm::vec3(1, 1, 1));
                mesh_box3->set_ambient_color(glm::vec3(1, 1, 1));
                mesh_box4->set_ambient_color(glm::vec3(1, 1, 1));
                mesh_box5->set_ambient_color(glm::vec3(1, 1, 1));
                mesh_box6->set_ambient_color(glm::vec3(1, 1, 1));
            } else {
                glPolygonMode(GL_FRONT, GL_FILL);
                mesh_box->set_ambient_color(glm::vec3(0, 0, 0));
                mesh_box2->set_ambient_color(glm::vec3(0, 0, 0));
                mesh_box3->set_ambient_color(glm::vec3(0, 0, 0));
                mesh_box4->set_ambient_color(glm::vec3(0, 0, 0));
                mesh_box5->set_ambient_color(glm::vec3(0, 0, 0));
                mesh_box6->set_ambient_color(glm::vec3(0, 0, 0));
            }
            break;
        case 'x': // axis
            show_axis = !show_axis;
            break;
        case 'z': // axis_labels
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
    camera->resize(0, 0, width, height);
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
