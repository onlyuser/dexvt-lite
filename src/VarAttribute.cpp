#include <VarAttribute.h>
#include <Buffer.h>
#include <Program.h>
#include <GL/glew.h>

namespace vt {

VarAttribute::VarAttribute(const Program* program, const GLchar* name)
    : m_is_enabled(false)
{
    m_id = glGetAttribLocation(program->id(), name);
}

VarAttribute::~VarAttribute()
{
}

void VarAttribute::enable_vertex_attrib_array() const
{
    glEnableVertexAttribArray(m_id);
}

void VarAttribute::disable_vertex_attrib_array() const
{
    glDisableVertexAttribArray(m_id);
}

void VarAttribute::vertex_attrib_pointer(Buffer*       buffer,
                                         GLint         size,
                                         GLenum        type,
                                         GLboolean     normalized,
                                         GLsizei       stride,
                                         const GLvoid* pointer) const
{
    buffer->bind();
    glVertexAttribPointer(m_id,
                          size,
                          type,
                          normalized,
                          stride,
                          pointer);
}

}
