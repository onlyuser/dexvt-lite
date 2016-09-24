#include <Program.h>
#include <Shader.h>
#include <VarAttribute.h>
#include <VarUniform.h>
#include <GL/glew.h>
#include <stdlib.h>
#include <iostream>

namespace vt {

Program::Program(std::string name)
    : NamedObject(name),
      m_vertex_shader(NULL),
      m_fragment_shader(NULL)
{
    m_id = glCreateProgram();
}

Program::~Program()
{
    glDeleteProgram(m_id);
}

void Program::attach_shader(Shader* shader)
{
    glAttachShader(m_id, shader->id());
    switch(shader->type())
    {
        case GL_VERTEX_SHADER:
            m_vertex_shader = shader;
            break;
        case GL_FRAGMENT_SHADER:
            m_fragment_shader = shader;
            break;
    }
}

bool Program::link() const
{
    glLinkProgram(m_id);
    GLint link_ok = GL_FALSE;
    get_program_iv(GL_LINK_STATUS, &link_ok);
    return (link_ok == GL_TRUE);
}

void Program::use() const
{
    glUseProgram(m_id);
}

VarAttribute* Program::get_var_attribute(const GLchar* name) const
{
    VarAttribute* var_attrribute = new VarAttribute(this, name);
    if(var_attrribute && var_attrribute->id() == static_cast<GLuint>(-1))
    {
        fprintf(stderr, "Could not bind attribute %s\n", name);
        delete var_attrribute;
        return NULL;
    }
    return var_attrribute;
}

VarUniform* Program::get_var_uniform(const GLchar* name) const
{
    VarUniform* var_uniform = new VarUniform(this, name);
    if(var_uniform && var_uniform->id() == static_cast<GLuint>(-1))
    {
        fprintf(stderr, "Could not bind uniform %s\n", name);
        delete var_uniform;
        return NULL;
    }
    return var_uniform;
}

void Program::get_program_iv(
        GLenum pname,
        GLint* params) const
{
    glGetProgramiv(m_id, pname, params);
}

bool Program::add_var(var_type_t var_type, std::string name)
{
    switch(var_type)
    {
        case VAR_TYPE_ATTRIBUTE:
            {
                std::string cmd1 = std::string("/bin/grep \"attribute .* ") + name + "[;\[]\" " + m_vertex_shader->get_filename()   + " > /dev/null";
                std::string cmd2 = std::string("/bin/grep \"attribute .* ") + name + "[;\[]\" " + m_fragment_shader->get_filename() + " > /dev/null";
                if(!system(cmd1.c_str()) || !system(cmd2.c_str())) {
                    std::cout << "\tFound attribute var \"" << name << "\"" << std::endl;
                } else {
                    std::cout << "\tError: Cannot find attribute var \"" << name << "\"" << std::endl;
                    exit(1);
                    return false;
                }
            }
            break;
        case VAR_TYPE_UNIFORM:
            {
                std::string cmd1 = std::string("/bin/grep \"uniform .* ") + name + "[;\[]\" " + m_vertex_shader->get_filename()   + " > /dev/null";
                std::string cmd2 = std::string("/bin/grep \"uniform .* ") + name + "[;\[]\" " + m_fragment_shader->get_filename() + " > /dev/null";
                if(!system(cmd1.c_str()) || !system(cmd2.c_str())) {
                    std::cout << "\tFound uniform var \"" << name << "\"" << std::endl;
                } else {
                    std::cout << "\tError: Cannot find uniform var \"" << name << "\"" << std::endl;
                    exit(1);
                    return false;
                }
            }
            break;
    }
    var_names_t::iterator p = m_var_names.find(name);
    if(p != m_var_names.end()) {
        std::cout << "Program variable \"" << name << "\" already added." << std::endl;
        return false;
    }
    m_var_names[name] = var_type;
    return true;
}

bool Program::has_var(std::string name, var_type_t* var_type)
{
    var_names_t::iterator p = m_var_names.find(name);
    if(p == m_var_names.end()) {
        return false;
    }
    if(var_type) {
        *var_type = (*p).second;
    }
    return true;
}

void Program::clear_vars()
{
    m_var_names.clear();
}

}
