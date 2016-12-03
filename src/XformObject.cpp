#include <XformObject.h>
#include <glm/glm.hpp>
#include <set>

namespace vt {

XformObject::XformObject(glm::vec3 origin, glm::vec3 orient, glm::vec3 scale)
    : m_origin(origin),
      m_orient(orient),
      m_scale(scale),
      m_parent(NULL),
      m_is_dirty_xform(true),
      m_is_dirty_normal_xform(true)
{
}

XformObject::~XformObject()
{
}

const glm::mat4 &XformObject::get_xform(bool trace_down)
{
    if(trace_down) {
        update_xform_hier();
        return m_xform;
    }
    if(m_is_dirty_xform) {
        update_xform();
        if(m_parent) {
            m_xform = m_parent->get_xform(false) * m_xform;
        }
        m_is_dirty_xform = false;
    }
    return m_xform;
}

const glm::mat4 &XformObject::get_normal_xform(bool trace_down)
{
    if(trace_down) {
        update_normal_xform_hier();
        return m_normal_xform;
    }
    if(m_is_dirty_normal_xform) {
        update_normal_xform();
        if(m_parent) {
            m_normal_xform = m_parent->get_normal_xform(false) * m_normal_xform;
        }
        m_is_dirty_normal_xform = false;
    }
    return m_normal_xform;
}

void XformObject::link_parent(XformObject* parent)
{
    glm::vec3 local_axis = glm::vec3(0);
    glm::vec3 axis = glm::vec3(get_xform() * glm::vec4(local_axis, 1));
    if(parent) {
        // unproject to global space and reproject to parent space
        glm::mat4 basis = glm::inverse(parent->get_xform());
        rebase(&basis);

        // break all connections -- TODO: review this
        link_parent(NULL);
        unlink_children();

        // make new parent remember you
        parent->get_children().insert(this);
    } else if(m_parent) {
        // unproject to global space
        rebase();

        //link_parent(NULL); // infinite recursion
        unlink_children();

        // make parent disown you
        std::set<XformObject*>::iterator p = m_parent->get_children().find(this);
        if(p != m_parent->get_children().end()) {
            m_parent->get_children().erase(p);
        }
    }
    m_parent = parent;
    set_axis(axis);
}

void XformObject::unlink_children()
{
    for(std::set<XformObject*>::iterator p = m_children.begin(); p != m_children.end(); p++) {
        (*p)->link_parent(NULL);
    }
}

void XformObject::update_xform_hier()
{
    for(std::set<XformObject*>::iterator p = m_children.begin(); p != m_children.end(); p++) {
        (*p)->mark_dirty_xform(); // mark entire subtree dirty
        (*p)->update_xform_hier();
    }
    if(m_children.empty()) {
        get_xform(false); // update entire leaf-to-root lineage for all leaf nodes
    }
}

void XformObject::update_normal_xform_hier()
{
    for(std::set<XformObject*>::iterator p = m_children.begin(); p != m_children.end(); p++) {
        (*p)->mark_dirty_xform(); // mark entire subtree dirty
        (*p)->update_normal_xform_hier();
    }
    if(m_children.empty()) {
        get_normal_xform(false); // update entire leaf-to-root lineage for all leaf nodes
    }
}

void XformObject::update_normal_xform()
{
    m_normal_xform = glm::transpose(glm::inverse(get_xform()));
}

}
