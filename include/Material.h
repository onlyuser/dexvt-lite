#ifndef VT_MATERIAL_H_
#define VT_MATERIAL_H_

#include <NamedObject.h>
#include <Program.h>
#include <Shader.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>
#include <memory> // std::unique_ptr

namespace vt {

class Texture;

class Material : public NamedObject
{
public:
    typedef std::vector<Texture*> textures_t;

    Material(
            std::string name                 = "",
            std::string vertex_shader_file   = "",
            std::string fragment_shader_file = "",
            bool use_overlay                 = false);
    virtual ~Material();
    Program* get_program() const
    {
        return m_program;
    }
    Shader* get_vertex_shader() const
    {
        return m_vertex_shader;
    }
    Shader* get_fragment_shader() const
    {
        return m_fragment_shader;
    }

    void add_texture(Texture* texture);
    void clear_textures();
    const textures_t &get_textures() const
    {
        return m_textures;
    }

    Texture* get_texture_by_index(int index) const;
    int get_texture_index(vt::Texture* texture) const;
    Texture* get_texture_by_name(std::string name) const;
    int get_texture_index_by_name(std::string name) const;

    bool use_overlay() const
    {
        return m_use_overlay;
    }
    bool use_ssao() const
    {
        return m_use_ssao;
    }

private:
    Program*   m_program;
    Shader*    m_vertex_shader;
    Shader*    m_fragment_shader;
    textures_t m_textures; // TODO: Material has multiple Textures
    bool       m_use_overlay;
    bool       m_use_ssao;

    typedef std::map<std::string, Texture*> texture_lookup_table_t;
    texture_lookup_table_t m_texture_lookup_table;
};

}

#endif
