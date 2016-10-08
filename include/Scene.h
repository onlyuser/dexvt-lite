#ifndef VT_SCENE_H_
#define VT_SCENE_H_

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

class Scene
{
public:
    typedef enum { USE_MESH_MATERIAL,
                   USE_NORMAL_MATERIAL,
                   USE_WIREFRAME_MATERIAL,
                   USE_SSAO_MATERIAL } use_material_type_t;

    typedef std::vector<Light*>    lights_t;
    typedef std::vector<Mesh*>     meshes_t;
    typedef std::vector<Material*> materials_t;
    typedef std::vector<Texture*>  textures_t;

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

    void add_light(Light* light)
    {
        m_lights.push_back(light);
    }

    void add_mesh(Mesh* mesh)
    {
        m_meshes.push_back(mesh);
    }

    void add_material(Material* material)
    {
        m_materials.push_back(material);
    }

    void add_texture(Texture* texture);

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
    void render(
            bool                clear_canvas      = true,
            bool                render_skybox     = true,
            bool                render_overlay    = false,
            use_material_type_t use_material_type = use_material_type_t::USE_MESH_MATERIAL);
    void render_lines(bool draw_axis, bool draw_bbox, bool draw_normals) const;
    void render_lights() const;

private:
    Camera*     m_camera;
    Mesh*       m_skybox;
    Mesh*       m_overlay;
    lights_t    m_lights;
    meshes_t    m_meshes;
    materials_t m_materials;
    textures_t  m_textures;
    Material*   m_normal_material;
    Material*   m_wireframe_material;
    Material*   m_ssao_material;

    GLfloat  m_bloom_kernel[7];
    GLfloat  m_glow_cutoff_threshold;
    GLfloat* m_light_pos;
    GLfloat* m_light_color;
    GLint*   m_light_enabled;
    GLfloat* m_ssao_sample_kernel_pos;

    Scene();
    ~Scene();
};

}

#endif
