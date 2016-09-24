#ifndef VT_SHADER_H_
#define VT_SHADER_H_

#include <IdentObject.h>
#include <GL/glew.h>
#include <string>

namespace vt {

class Shader : public IdentObject
{
public:
    Shader(std::string filename, GLenum type);
    virtual ~Shader();
    std::string get_filename() const
    {
        return m_filename;
    }
    GLenum type() const
    {
        return m_type;
    }

private:
    std::string m_filename;
    GLenum m_type;
};

}

#endif
