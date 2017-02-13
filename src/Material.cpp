#include <Material.h>
#include <NamedObject.h>
#include <Program.h>
#include <Shader.h>
#include <Texture.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>

namespace vt {

Material::Material(std::string name,
                   std::string vertex_shader_file,
                   std::string fragment_shader_file,
                   bool        use_overlay)
    : NamedObject(name),
      m_program(NULL),
      m_vertex_shader(NULL),
      m_fragment_shader(NULL),
      m_use_overlay(use_overlay)
{
    m_program         = new Program(name);
    m_vertex_shader   = new Shader(vertex_shader_file,   GL_VERTEX_SHADER);
    m_fragment_shader = new Shader(fragment_shader_file, GL_FRAGMENT_SHADER);
    m_program->attach_shader(m_vertex_shader);
    m_program->attach_shader(m_fragment_shader);
    if(!m_program->link()) {
        fprintf(stderr, "glLinkProgram:");
//        print_log(m_program->id());
        return;
    }
}

Material::~Material()
{
    if(m_program)         { delete m_program; }
    if(m_vertex_shader)   { delete m_vertex_shader; }
    if(m_fragment_shader) { delete m_fragment_shader; }
}

void Material::add_texture(Texture* texture)
{
    m_textures.push_back(texture);
    m_texture_lookup_table[texture->get_name()] = texture;
}

void Material::clear_textures()
{
    m_textures.clear();
    m_texture_lookup_table.clear();
}

Texture* Material::get_texture_by_index(int index) const
{
    assert(index >= 0 && index < static_cast<int>(m_textures.size()));
    return m_textures[index];
}

int Material::get_texture_index(Texture* texture) const
{
    textures_t::const_iterator p = std::find(m_textures.begin(), m_textures.end(), texture);
    if(p == m_textures.end()) {
        return -1;
    }
    return std::distance(m_textures.begin(), p);
}

Texture* Material::get_texture_by_name(std::string name) const
{
    texture_lookup_table_t::const_iterator p = m_texture_lookup_table.find(name);
    if(p == m_texture_lookup_table.end()) {
        return NULL;
    }
    return (*p).second;
}

int Material::get_texture_index_by_name(std::string name) const
{
    return get_texture_index(get_texture_by_name(name));
}

}
