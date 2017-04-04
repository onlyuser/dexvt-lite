#ifndef VT_BUFFER_H_
#define VT_BUFFER_H_

#include <IdentObject.h>
#include <BindableObjectBase.h>
#include <GL/glew.h>

namespace vt {

class Buffer : public IdentObject, public BindableObjectBase
{
public:
    Buffer(GLenum target, size_t size, void* data);
    virtual ~Buffer();
    void update();
    void bind();
    size_t size() const
    {
        return m_size;
    }

private:
    GLenum m_target;
    size_t m_size;
    void* m_data;
};

}

#endif
