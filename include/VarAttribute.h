#ifndef VT_VAR_ATTRIBUTE_H_
#define VT_VAR_ATTRIBUTE_H_

#include <IdentObject.h>
#include <GL/glew.h>

namespace vt {

class Buffer;
class Program;

class VarAttribute : public IdentObject
{
public:
    VarAttribute(const Program* program, const GLchar* name);
    virtual ~VarAttribute();
    void enable_vertex_attrib_array() const;
    void disable_vertex_attrib_array() const;
    bool is_enabled() const { return m_is_enabled; }
    void vertex_attrib_pointer(
            Buffer* buffer,
            GLint size,
            GLenum type,
            GLboolean normalized,
            GLsizei stride,
            const GLvoid* pointer) const;

private:
    bool m_is_enabled;
};

}

#endif
