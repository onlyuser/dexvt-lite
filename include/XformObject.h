#ifndef VT_XFORM_OBJECT_H_
#define VT_XFORM_OBJECT_H_

#include <glm/glm.hpp>
#include <set>

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

    const glm::mat4 &get_xform(bool trace_down = true);
    const glm::mat4 &get_normal_xform(bool trace_down = true);

    XformObject* get_parent() const
    {
        return m_parent;
    }
    std::set<XformObject*> &get_children()
    {
        return m_children;
    }
    void link_parent(XformObject* parent);
    void unlink_children();
    bool is_root() const
    {
        return !m_parent;
    }
    bool is_leaf() const
    {
        return m_children.empty();
    }
    void update_xform_hier();
    void update_normal_xform_hier();

protected:
    glm::vec3 m_origin;
    glm::vec3 m_orient;
    glm::vec3 m_scale;
    glm::mat4 m_xform;
    glm::mat4 m_normal_xform;
    XformObject* m_parent;
    std::set<XformObject*> m_children;

    void mark_dirty_xform()
    {
        m_is_dirty_xform        = true;
        m_is_dirty_normal_xform = true;
    }
    virtual void update_xform() = 0;
    virtual void update_normal_xform();
    virtual void xform_vertices(glm::mat4 xform) {}
    virtual void rebase(glm::mat4* basis = NULL) {}
    virtual void set_axis(glm::vec3 axis) {}

private:
    bool m_is_dirty_xform;
    bool m_is_dirty_normal_xform;
};

}

#endif
