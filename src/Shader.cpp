#include <Shader.h>
#include <shader_utils.h>
#include <GL/glew.h>
#include <string>
#include <iostream>
#include <assert.h>

namespace vt {

Shader::Shader(std::string filename, GLenum type)
    : m_filename(filename),
      m_type(type)
{
    std::cout << "Creating shader \"" << filename << "\"" << std::endl;
    m_id = create_shader(filename.c_str(), type);
    assert(m_id);
}

Shader::~Shader()
{
    glDeleteShader(m_id);
}

}
