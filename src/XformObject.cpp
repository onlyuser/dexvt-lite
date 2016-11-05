#include <XformObject.h>
#include <glm/glm.hpp>

namespace vt {

XformObject::XformObject(glm::vec3 origin, glm::vec3 orient, glm::vec3 scale)
    : m_origin(origin),
      m_orient(orient),
      m_scale(scale),
      m_need_update_xform(true),
      m_need_update_normal_xform(true),
      m_parent(NULL)
{
}

XformObject::~XformObject()
{
}

const glm::mat4 &XformObject::get_xform()
{
    if(m_need_update_xform) {
        update_xform();
        if(m_parent) {
            m_xform = m_parent->get_xform() * m_xform;
        }
        m_need_update_xform = false;
    }
    return m_xform;
}

const glm::mat4 &XformObject::get_normal_xform()
{
    if(m_need_update_normal_xform) {
        update_normal_xform();
        m_need_update_normal_xform = false;
    }
    return m_normal_xform;
}

void XformObject::update_normal_xform()
{
    m_normal_xform = glm::transpose(glm::inverse(get_xform()));
}

}
