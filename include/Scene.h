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

#ifndef VT_SCENE_H_
#define VT_SCENE_H_

#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <vector>
#include <map>
#include <string>

namespace vt {

class Camera;
class Light;
class Material;
class Mesh;
class Texture;
class Octree;

struct DebugObjectContext
{
    glm::mat4              m_transform;
    std::vector<glm::vec3> m_debug_origin_frame_values;
    std::vector<glm::vec3> m_debug_origin_keyframe_values;

    DebugObjectContext();
};

class Scene
{
public:
    typedef enum { DEBUG_TARGET_ORIGIN,
                   DEBUG_TARGET_COLOR,
                   DEBUG_TARGET_RADIUS,
                   DEBUG_TARGET_LINEWIDTH } debug_target_attr_t;
    typedef enum { DEBUG_LINE_P1,
                   DEBUG_LINE_P2,
                   DEBUG_LINE_COLOR,
                   DEBUG_LINE_LINEWIDTH } debug_line_attr_t;

    // for guide wires
    std::vector<std::tuple<glm::vec3, glm::vec3, float, float>>     m_debug_targets;
    std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3, float>> m_debug_lines;

    std::map<long, DebugObjectContext> m_debug_object_context;

    typedef enum { USE_MESH_MATERIAL,
                   USE_NORMAL_MATERIAL,
                   USE_WIREFRAME_MATERIAL,
                   USE_SSAO_MATERIAL } use_material_type_t;

    typedef std::vector<Light*>    lights_t;
    typedef std::vector<Mesh*>     meshes_t;
    typedef std::vector<Material*> materials_t;
    typedef std::vector<Texture*>  textures_t;

    int m_ray_tracer_render_mode;
    int m_ray_tracer_bounce_count;

    int                    m_ray_tracer_sphere_count;
    std::vector<glm::vec3> m_ray_tracer_sphere_origin;
    std::vector<float>     m_ray_tracer_sphere_radius;
    std::vector<float>     m_ray_tracer_sphere_eta;
    std::vector<float>     m_ray_tracer_sphere_diffuse_fuzz;
    std::vector<glm::vec3> m_ray_tracer_sphere_color;
    std::vector<float>     m_ray_tracer_sphere_reflectance;
    std::vector<float>     m_ray_tracer_sphere_transparency;
    std::vector<float>     m_ray_tracer_sphere_luminosity;

    int                    m_ray_tracer_plane_count;
    std::vector<glm::vec3> m_ray_tracer_plane_point;
    std::vector<glm::vec3> m_ray_tracer_plane_normal;
    std::vector<float>     m_ray_tracer_plane_eta;
    std::vector<float>     m_ray_tracer_plane_diffuse_fuzz;
    std::vector<glm::vec3> m_ray_tracer_plane_color;
    std::vector<float>     m_ray_tracer_plane_reflectance;
    std::vector<float>     m_ray_tracer_plane_transparency;
    std::vector<float>     m_ray_tracer_plane_luminosity;

    int                    m_ray_tracer_box_count;
    std::vector<glm::mat4> m_ray_tracer_box_transform;
    std::vector<glm::mat4> m_ray_tracer_box_inverse_transform;
    std::vector<glm::vec3> m_ray_tracer_box_min;
    std::vector<glm::vec3> m_ray_tracer_box_max;
    std::vector<float>     m_ray_tracer_box_eta;
    std::vector<float>     m_ray_tracer_box_diffuse_fuzz;
    std::vector<glm::vec3> m_ray_tracer_box_color;
    std::vector<float>     m_ray_tracer_box_reflectance;
    std::vector<float>     m_ray_tracer_box_transparency;
    std::vector<float>     m_ray_tracer_box_luminosity;

    int                    m_ray_tracer_random_point_count;
    std::vector<glm::vec3> m_ray_tracer_random_points;
    float                  m_ray_tracer_random_seed;

    static Scene* instance()
    {
        static Scene scene;
        return &scene;
    }

    void set_camera(Camera* camera)
    {
        m_camera = camera;
    }
    Camera* get_camera() const
    {
        return m_camera;
    }

    void set_octree(Octree* octree)
    {
        m_octree = octree;
    }
    Octree* get_octree() const
    {
        return m_octree;
    }

    Light* find_light(std::string name);
    void add_light(Light* light);
    void remove_light(Light* light);

    Mesh* find_mesh(std::string name);
    void add_mesh(Mesh* mesh);
    void remove_mesh(Mesh* mesh);

    Material* find_material(std::string name);
    void add_material(Material* material);
    void remove_material(Material* material);

    Texture* find_texture(std::string name);
    void add_texture(Texture* texture);
    void remove_texture(Texture* texture);

    void set_skybox(Mesh* skybox)
    {
        m_skybox = skybox;
    }
    Mesh* get_skybox() const
    {
        return m_skybox;
    }

    void set_overlay(Mesh* overlay)
    {
        m_overlay = overlay;
    }
    Mesh* get_overlay() const
    {
        return m_overlay;
    }

    void set_normal_material(Material* material)
    {
        m_normal_material = material;
    }
    Material* get_normal_material() const
    {
        return m_normal_material;
    }

    void set_wireframe_material(Material* material)
    {
        m_wireframe_material = material;
    }
    Material* get_wireframe_material() const
    {
        return m_wireframe_material;
    }

    void set_ssao_material(Material* material)
    {
        m_ssao_material = material;
    }
    Material* get_ssao_material() const
    {
        return m_ssao_material;
    }

    void set_glow_cutoff_threshold(float glow_cutoff_threshold)
    {
        m_glow_cutoff_threshold = glow_cutoff_threshold;
    }

    void reset();
    void use_program();
    void render(bool                clear_canvas      = true,
                bool                render_overlay    = false,
                bool                render_skybox     = true,
                use_material_type_t use_material_type = use_material_type_t::USE_MESH_MATERIAL);
    void render_lines_and_text(bool  _draw_guide_wires,
                               bool  _draw_paths,
                               bool  _draw_axis,
                               bool  _draw_axis_labels,
                               bool  _draw_bbox,
                               bool  _draw_normals,
                               bool  _draw_hud_text = false,
                               char* hud_text       = const_cast<char*>("")) const;
    void render_lights() const;

    void clear_ray_tracer_objects();
    void add_ray_tracer_sphere(glm::vec3 origin,
                               float     radius,
                               float     eta,
                               float     diffuse_fuzz,
                               glm::vec3 color,
                               float     reflectance,
                               float     transparency,
                               float     luminosity);
    void add_ray_tracer_plane(glm::vec3 point,
                              glm::vec3 normal,
                              float     eta,
                              float     diffuse_fuzz,
                              glm::vec3 color,
                              float     reflectance,
                              float     transparency,
                              float     luminosity);
    void add_ray_tracer_box(glm::vec3 origin,
                            glm::vec3 euler,
                            glm::vec3 _min,
                            glm::vec3 _max,
                            float     eta,
                            float     diffuse_fuzz,
                            glm::vec3 color,
                            float     reflectance,
                            float     transparency,
                            float     luminosity);

private:
    Camera*     m_camera;
    Octree*     m_octree;
    Mesh*       m_skybox;
    Mesh*       m_overlay;
    lights_t    m_lights;
    meshes_t    m_meshes;
    materials_t m_materials;
    textures_t  m_textures;
    Material*   m_normal_material;
    Material*   m_wireframe_material;
    Material*   m_ssao_material;

    GLfloat* m_bloom_kernel;
    GLfloat  m_glow_cutoff_threshold;
    GLfloat* m_light_pos;
    GLfloat* m_light_color;
    GLint*   m_light_enabled;
    GLfloat* m_ssao_sample_kernel_pos;

    Scene();
    ~Scene();

    void draw_targets() const;
    void draw_octree(Octree* octree, glm::mat4 camera_transform) const;
    void draw_paths() const;
    void draw_debug_lines(Mesh* mesh) const;
    void draw_up_vector(Mesh* mesh) const;
    void draw_ik_guide_wires(Mesh* mesh) const;
    void draw_axis(Mesh* mesh) const;
    void draw_hinge_constraints(Mesh* mesh) const;
    void draw_bbox(Mesh* mesh) const;
    void draw_normals(Mesh* mesh) const;
    void draw_axis_labels(Mesh* mesh) const;
    void draw_hud(std::string hud_text) const;
};

}

#endif
