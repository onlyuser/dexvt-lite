#include <Buffer.h>
#include <GL/glew.h>

namespace vt {

Buffer::Buffer(GLenum target, size_t size, void* data)
    : m_target(target),
      m_size(size),
      m_data(data)
{
    glGenBuffers(1, &m_id);
    bind();
    glBufferData(target, size, data, GL_STATIC_DRAW);
}

Buffer::~Buffer()
{
    glDeleteBuffers(1, &m_id);
}

void Buffer::update()
{
    bind();
    glBufferData(m_target, m_size, m_data, GL_DYNAMIC_DRAW);
}

void Buffer::bind()
{
    glBindBuffer(m_target, m_id);
}

}
