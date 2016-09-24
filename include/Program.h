#ifndef VT_PROGRAM_H_
#define VT_PROGRAM_H_

#include <IdentObject.h>
#include <NamedObject.h>
#include <GL/glew.h>
#include <map>
#include <string>

namespace vt {

class Shader;
class VarAttribute;
class VarUniform;

class Program : public IdentObject, public NamedObject
{
public:
    enum var_type_t {
        VAR_TYPE_ATTRIBUTE,
        VAR_TYPE_UNIFORM
    };

    Program(std::string name);
    virtual ~Program();
    void attach_shader(Shader* shader);
    Shader* get_vertex_shader() const
    {
        return m_vertex_shader;
    }
    Shader* get_fragment_shader() const
    {
        return m_fragment_shader;
    }
    bool link() const;
    void use() const;
    VarAttribute* get_var_attribute(const GLchar* name) const;
    VarUniform* get_var_uniform(const GLchar* name) const;
    void get_program_iv(
            GLenum pname,
            GLint* params) const;
    bool add_var(var_type_t var_type, std::string name);
    bool has_var(std::string name, var_type_t* var_type = NULL);
    void clear_vars();

private:
    Shader* m_vertex_shader;
    Shader* m_fragment_shader;
    typedef std::map<std::string, var_type_t> var_names_t;
    var_names_t m_var_names;
};

}

#endif
