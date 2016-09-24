#ifndef VT_XFORM_OBJECT_H_
#define VT_XFORM_OBJECT_H_

#include <glm/glm.hpp>

namespace vt {

class XformObject
{
public:
    XformObject(glm::vec3 origin = glm::vec3(0), glm::vec3 orient = glm::vec3(0), glm::vec3 scale = glm::vec3(1));
    virtual ~XformObject();

    const glm::vec3 &get_origin() const
    {
        return m_origin;
    }
    void set_origin(glm::vec3 origin)
    {
        m_origin = origin;
    }

    const glm::vec3 &get_orient() const
    {
        return m_orient;
    }
    void set_orient(glm::vec3 orient)
    {
        m_orient = orient;
    }

    const glm::vec3 &get_scale() const
    {
        return m_scale;
    }
    void set_scale(glm::vec3 scale)
    {
        m_scale = scale;
    }

    const glm::mat4 &get_xform();
    const glm::mat4 &get_normal_xform();

protected:
    glm::vec3 m_origin;
    glm::vec3 m_orient;
    glm::vec3 m_scale;
    glm::mat4 m_xform;
    glm::mat4 m_normal_xform;

    void set_need_update_xform()
    {
        m_need_update_xform        = true;
        m_need_update_normal_xform = true;
    }
    virtual void update_xform() = 0;
    virtual void update_normal_xform();

private:
    bool m_need_update_xform;
    bool m_need_update_normal_xform;
};

}

#endif
