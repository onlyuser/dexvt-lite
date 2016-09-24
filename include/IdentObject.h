#ifndef VT_IDENT_OBJECT_H_
#define VT_IDENT_OBJECT_H_

#include <GL/glew.h>

namespace vt {

class IdentObject
{
public:
    GLuint id() const
    {
        return m_id;
    }

protected:
    GLuint m_id;

    IdentObject();
};

}

#endif
