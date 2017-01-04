#include <XformObject.h>
#include <Util.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/glm.hpp>
#include <set>

namespace vt {

XformObject::XformObject(
        glm::vec3 origin,
        glm::vec3 orient,
        glm::vec3 scale)
    : m_origin(origin),
      m_orient(orient),
      m_up_direction(VEC_UP),
      m_scale(scale),
      m_parent(NULL),
      m_is_dirty_xform(true),
      m_is_dirty_normal_xform(true)
{
}

XformObject::~XformObject()
{
}

void XformObject::set_origin(glm::vec3 origin)
{
    m_origin = origin;
    mark_dirty_xform();
}

void XformObject::set_orient(glm::vec3 orient)
{
    m_orient = orient;
    mark_dirty_xform();
}

void XformObject::set_up_direction(glm::vec3 up_direction, glm::vec3* heading)
{
    glm::vec3 _heading;
    if(heading) {
        _heading = *heading;
    } else {
        _heading = orient_to_offset(m_orient);
    }
    if(fabs(glm::angle(glm::normalize(up_direction), glm::normalize(_heading))) - HALF_PI >= EPSILON) {
        up_direction = renormalize_up_vector(_heading, up_direction);
    }
    m_up_direction = up_direction;
}

void XformObject::set_scale(glm::vec3 scale)
{
    m_scale = scale;
    mark_dirty_xform();
}

void XformObject::reset_xform()
{
    m_origin = glm::vec3(0);
    m_orient = glm::vec3(0);
    m_scale = glm::vec3(1);
    mark_dirty_xform();
}

void XformObject::link_parent(XformObject* parent, bool keep_xform)
{
    glm::vec3 axis;
    if(keep_xform) {
        glm::vec3 local_axis = glm::vec3(0);
        axis = glm::vec3(get_xform() * glm::vec4(local_axis, 1));
    }
    if(parent) {
        if(keep_xform) {
            // unproject to global space and reproject to parent space
            glm::mat4 basis = glm::inverse(parent->get_xform());
            rebase(&basis);

            // break all connections -- TODO: review this
            link_parent(NULL);
            unlink_children();
        }

        // make new parent remember you
        parent->get_children().insert(this);
    } else {
        if(keep_xform) {
            // unproject to global space
            rebase();

            //link_parent(NULL); // infinite recursion
            unlink_children();
        }

        if(m_parent) {
            // make parent disown you
            std::set<XformObject*>::iterator p = m_parent->get_children().find(this);
            if(p != m_parent->get_children().end()) {
                m_parent->get_children().erase(p);
            }
        }
    }
    m_parent = parent;
    if(keep_xform) {
        set_axis(axis);
    } else {
        reset_xform();
    }
}

void XformObject::unlink_children()
{
    for(std::set<XformObject*>::iterator p = m_children.begin(); p != m_children.end(); p++) {
        (*p)->link_parent(NULL);
    }
}

void XformObject::point_at(glm::vec3 target)
{
    glm::vec3 local_target;
    if(m_parent) {
        local_target = glm::vec3(glm::inverse(m_parent->get_xform()) * glm::vec4(target, 1));
    } else {
        local_target = target;
    }
    m_orient = offset_to_orient(local_target - m_origin);
    mark_dirty_xform();
}

void XformObject::rotate(float angle_delta, glm::vec3 pivot)
{
    glm::vec3 local_pivot;
    if(m_parent) {
        glm::mat4 inv_parent_xform = glm::inverse(m_parent->get_xform());
        local_pivot = glm::vec3(inv_parent_xform * glm::vec4(pivot, 1)) -
                      glm::vec3(inv_parent_xform * glm::vec4(glm::vec3(0), 1));
    } else {
        local_pivot = pivot;
    }
    glm::vec3 heading          = orient_to_offset(m_orient);
    glm::mat4 rotate_xform     = GLM_ROTATE(glm::mat4(1), angle_delta, local_pivot);
    glm::vec3 new_heading      = glm::vec3(rotate_xform * glm::vec4(heading, 1));
    glm::vec3 new_up_direction = glm::vec3(rotate_xform * glm::vec4(m_up_direction, 1));
    set_up_direction(new_up_direction, &new_heading);
    set_orient(offset_to_orient(new_heading, &new_up_direction));
}

// http://what-when-how.com/advanced-methods-in-computer-graphics/kinematics-advanced-methods-in-computer-graphics-part-4/
bool XformObject::solve_ik_ccd(
    XformObject* root,
    glm::vec3    end_effector_tip_local_offset,
    glm::vec3    target,
    int          iters,
    float        accept_distance)
{
    for(int i = 0; i < iters; i++) {
        for(XformObject* current_segment = this;
            current_segment && current_segment != root->get_parent();
            current_segment = current_segment->get_parent())
        {
            glm::vec3 end_effector_tip = glm::vec3(get_xform() * glm::vec4(end_effector_tip_local_offset, 1));
            if(glm::distance(end_effector_tip, target) < accept_distance) {
                return true;
            }
            glm::mat4 current_segment_inverse_xform = glm::inverse(current_segment->get_xform());
#if 1
            // attempt #3 -- do rotations in Cartesian coordinates
            glm::vec3 local_target_normal           = glm::normalize(glm::vec3(current_segment_inverse_xform * glm::vec4(target, 1)));
            glm::vec3 local_end_effector_tip_normal = glm::normalize(glm::vec3(current_segment_inverse_xform * glm::vec4(end_effector_tip, 1)));
            glm::vec3 local_delta_normal            = glm::normalize(local_target_normal - local_end_effector_tip_normal);
            glm::vec3 local_midpoint_normal         = glm::normalize((local_target_normal + local_end_effector_tip_normal) * 0.5f);
            glm::vec3 local_pivot                   = glm::normalize(glm::cross(local_delta_normal, local_midpoint_normal));
            float     angle_delta                   = glm::degrees(glm::angle(local_target_normal, local_end_effector_tip_normal));
            glm::vec3 heading                       = orient_to_offset(current_segment->get_orient());
            glm::vec3 new_heading                   = glm::vec3(GLM_ROTATE(glm::mat4(1), -angle_delta, local_pivot) * glm::vec4(heading, 1));
            current_segment->set_orient(offset_to_orient(new_heading));
#elif 0
            // attempt #2 -- do rotations in Euler coordinates with special handling for angle loop-around
            glm::vec3 local_target_orient           = offset_to_orient(glm::vec3(current_segment_inverse_xform * glm::vec4(target, 1)));
            glm::vec3 local_end_effector_tip_orient = offset_to_orient(glm::vec3(current_segment_inverse_xform * glm::vec4(end_effector_tip, 1)));
            glm::vec3 orient_delta                  = orient_diff(local_target_orient, local_end_effector_tip_orient);
            current_segment->set_orient(orient_sum(current_segment->get_orient(), orient_delta));
#else
            // attempt #1 -- do rotations in Euler coordinates
            glm::vec3 local_target_orient           = offset_to_orient(glm::vec3(current_segment_inverse_xform * glm::vec4(target, 1)));
            glm::vec3 local_end_effector_tip_orient = offset_to_orient(glm::vec3(current_segment_inverse_xform * glm::vec4(end_effector_tip, 1)));
            glm::vec3 orient_delta                  = local_target_orient - local_end_effector_tip_orient;
            current_segment->set_orient(current_segment->get_orient() + orient_delta);
#endif
        }
    }
    return false;
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

const glm::mat4 &XformObject::get_normal_xform()
{
    if(m_is_dirty_normal_xform) {
        update_normal_xform();
        m_is_dirty_normal_xform = false;
    }
    return m_normal_xform;
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

void XformObject::update_normal_xform()
{
    m_normal_xform = glm::transpose(glm::inverse(get_xform()));
}

}
