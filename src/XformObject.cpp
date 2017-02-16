#include <XformObject.h>
#include <NamedObject.h>
#include <Util.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/glm.hpp>
#include <set>

//#define DEBUG

namespace vt {

XformObject::XformObject(
        std::string name,
        glm::vec3   origin,
        glm::vec3   orient,
        glm::vec3   scale)
    : NamedObject(name),
      m_origin(origin),
      m_orient(orient),
      m_scale(scale),
      m_enable_orient_constraints(       glm::ivec3(0)),
      m_orient_constraints_center(       glm::vec3(0)),
      m_orient_constraints_max_deviation(glm::vec3(180, 90, 180)),
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
    apply_constraints();
    mark_dirty_xform();
}

void XformObject::set_scale(glm::vec3 scale)
{
    m_scale = scale;
    mark_dirty_xform();
}

void XformObject::reset_xform()
{
    m_origin = glm::vec3(0);
    set_orient(glm::vec3(0));
    m_scale  = glm::vec3(1);
    mark_dirty_xform();
}

void XformObject::apply_constraints()
{
    for(int i = 0; i < 3; i++) {
        if(!m_enable_orient_constraints[i]) {
            continue;
        }
        if(angle_distance(m_orient[i], m_orient_constraints_center[i]) > m_orient_constraints_max_deviation[i]) {
            float min_angle = m_orient_constraints_center[i] - m_orient_constraints_max_deviation[i];
            float max_angle = m_orient_constraints_center[i] + m_orient_constraints_max_deviation[i];
            float distance_to_lower_bound = angle_distance(m_orient[i], min_angle);
            float distance_to_upper_bound = angle_distance(m_orient[i], max_angle);
            if(distance_to_lower_bound < distance_to_upper_bound) {
                m_orient[i] = min_angle;
            } else {
                m_orient[i] = max_angle;
            }
        }
    }
}

glm::vec3 XformObject::in_abs_system(glm::vec3 local_point)
{
    return glm::vec3(get_xform() * glm::vec4(local_point, 1));
}

glm::vec3 XformObject::in_parent_system(glm::vec3 abs_point) const
{
    if(m_parent) {
        return glm::vec3(glm::inverse(m_parent->get_xform()) * glm::vec4(abs_point, 1));
    }
    return abs_point;
}

glm::vec3 XformObject::from_origin_in_parent_system(glm::vec3 abs_point) const
{
    return in_parent_system(abs_point) - m_origin;
}

glm::vec3 XformObject::get_abs_left_direction()
{
    return glm::vec3(get_normal_xform() * glm::vec4(VEC_LEFT, 1));
}

glm::vec3 XformObject::get_abs_up_direction()
{
    return glm::vec3(get_normal_xform() * glm::vec4(VEC_UP, 1));
}

glm::vec3 XformObject::get_abs_heading()
{
    return glm::vec3(get_normal_xform() * glm::vec4(VEC_FORWARD, 1));
}

void XformObject::link_parent(XformObject* parent, bool keep_xform)
{
    glm::vec3 abs_origin;
    if(keep_xform) {
        abs_origin = in_abs_system();
    }
    if(parent) {
        if(keep_xform) {
            // unproject to global space and reproject to parent space
            glm::mat4 parent_inverse_xform = glm::inverse(parent->get_xform());
            rebase(&parent_inverse_xform);

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
        set_axis(abs_origin);
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

void XformObject::point_at_local(glm::vec3 local_target, glm::vec3* up_direction)
{
    set_orient(offset_to_orient(local_target, up_direction));
}

void XformObject::point_at(glm::vec3 target, glm::vec3* up_direction)
{
    point_at_local(from_origin_in_parent_system(target), up_direction);
}

void XformObject::rotate(float angle_delta, glm::vec3 pivot)
{
    glm::mat4 rotate_xform           = GLM_ROTATE(glm::mat4(1), angle_delta, pivot);
    glm::vec3 abs_origin             = in_abs_system();
    glm::vec3 local_new_heading      = from_origin_in_parent_system(abs_origin + glm::vec3(rotate_xform * glm::vec4(get_abs_heading(), 1)));
    glm::vec3 local_new_up_direction = from_origin_in_parent_system(abs_origin + glm::vec3(rotate_xform * glm::vec4(get_abs_up_direction(), 1)));
    point_at_local(local_new_heading, &local_new_up_direction);
}

// http://what-when-how.com/advanced-methods-in-computer-graphics/kinematics-advanced-methods-in-computer-graphics-part-4/
bool XformObject::solve_ik_ccd(XformObject* root,
                               glm::vec3    local_end_effector_tip,
                               glm::vec3    target,
                               int          iters,
                               float        accept_end_effector_distance,
                               float        accept_avg_angle_distance)
{
    bool converge = false;
    bool find_solution = false;
    for(int i = 0; i < iters && !converge; i++) {
        int segment_count = 0;
        float sum_angle = 0;
        for(XformObject* current_segment = this; current_segment && current_segment != root->get_parent(); current_segment = current_segment->get_parent()) {
            glm::vec3 end_effector_tip = in_abs_system(local_end_effector_tip);
            find_solution = (glm::distance(end_effector_tip, target) < accept_end_effector_distance);
#if 1
            glm::vec3 local_target_dir           = glm::normalize(current_segment->from_origin_in_parent_system(target));
            glm::vec3 local_end_effector_tip_dir = glm::normalize(current_segment->from_origin_in_parent_system(end_effector_tip));
            glm::vec3 local_arc_dir              = glm::normalize(local_target_dir - local_end_effector_tip_dir);
            glm::vec3 local_arc_midpoint_dir     = glm::normalize((local_target_dir + local_end_effector_tip_dir) * 0.5f);
            glm::vec3 local_arc_pivot            = glm::cross(local_arc_dir, local_arc_midpoint_dir);
            float     angle_delta                = glm::degrees(glm::angle(local_target_dir, local_end_effector_tip_dir));
            glm::mat4 local_arc_rotate_xform     = GLM_ROTATE(glm::mat4(1), -angle_delta, local_arc_pivot);
    #if 1
            // attempt #3 -- same as attempt #2, but make use of roll component (suitable for ropes/snakes/boids)
            glm::mat4 new_current_segment_orient_xform = local_arc_rotate_xform * current_segment->get_local_orient_xform();
            glm::vec3 new_current_segment_heading      = glm::vec3(new_current_segment_orient_xform * glm::vec4(VEC_FORWARD, 1));
            glm::vec3 new_current_segment_up_direction = glm::vec3(new_current_segment_orient_xform * glm::vec4(VEC_UP, 1));
            current_segment->point_at_local(new_current_segment_heading, &new_current_segment_up_direction);

            // update guide wires
            local_target_dir           = glm::normalize(current_segment->from_origin_in_parent_system(target));
            local_end_effector_tip_dir = glm::normalize(current_segment->from_origin_in_parent_system(end_effector_tip));
            local_arc_dir              = glm::normalize(local_target_dir - local_end_effector_tip_dir);
            local_arc_midpoint_dir     = glm::normalize((local_target_dir + local_end_effector_tip_dir) * 0.5f);
            local_arc_pivot            = glm::cross(local_arc_dir, local_arc_midpoint_dir);
            current_segment->m_debug_target_dir           = local_target_dir;
            current_segment->m_debug_end_effector_tip_dir = local_end_effector_tip_dir;
            current_segment->m_debug_local_pivot          = local_arc_pivot;
            current_segment->m_debug_local_target         = current_segment->from_origin_in_parent_system(target);
        #ifdef DEBUG
            //std::cout << "TARGET: " << glm::to_string(local_target_dir) << ", END_EFF: " << glm::to_string(local_end_effector_tip_dir) << ", ANGLE: " << angle_delta << std::endl;
            //std::cout << "BEFORE: " << glm::to_string(new_current_segment_xform * glm::vec4(VEC_FORWARD, 1))
            //          << ", AFTER: " << glm::to_string(new_current_segment_heading) << std::endl;
            //std::cout << "PIVOT: " << glm::to_string(local_arc_pivot) << std::endl;
        #endif
    #else
            // attempt #2 -- do rotations in Cartesian coordinates (suitable for robots)
            current_segment->point_at_local(glm::vec3(local_arc_rotate_xform * glm::vec4(orient_to_offset(current_segment->get_orient()), 1)));
    #endif
            sum_angle += angle_delta;
#else
            // attempt #1 -- do rotations in Euler coordinates (poor man's ik)
            glm::vec3 local_target_orient           = offset_to_orient(current_segment->from_origin_in_parent_system(target));
            glm::vec3 local_end_effector_tip_orient = offset_to_orient(current_segment->from_origin_in_parent_system(end_effector_tip));
            current_segment->set_orient(orient_modulo(current_segment->get_orient() + orient_modulo(local_target_orient - local_end_effector_tip_orient)));
            sum_angle += accept_avg_angle_distance; // to avoid convergence
#endif
#ifdef DEBUG
            std::cout << "NAME: " << current_segment->get_name() << ", ORIENT: " << glm::to_string(current_segment->get_orient()) << std::endl;
#endif
            segment_count++;
        }
        if(!segment_count) {
            continue;
        }
        float average_angle = sum_angle / segment_count;
        if(average_angle < accept_avg_angle_distance) {
            converge = true;
        }
    }
    return converge && find_solution;
}

void XformObject::update_boid(glm::vec3 target,
                              float     forward_speed,
                              float     angle_delta,
                              float     avoid_radius)
{
    glm::vec3 local_target_dir       = glm::normalize(from_origin_in_parent_system(target));
    glm::vec3 local_heading          = glm::normalize(from_origin_in_parent_system(in_abs_system(VEC_FORWARD)));
    glm::vec3 local_arc_dir          = glm::normalize(local_target_dir - local_heading);
    glm::vec3 local_arc_midpoint_dir = glm::normalize((local_target_dir + local_heading) * 0.5f);
    glm::vec3 local_arc_pivot        = glm::cross(local_arc_dir, local_arc_midpoint_dir);
    glm::mat4 local_arc_rotate_xform = GLM_ROTATE(glm::mat4(1), -angle_delta * ((glm::distance(target, m_origin) < avoid_radius) ? -1 : 1), local_arc_pivot);
#if 1
    // attempt #3 -- same as attempt #2, but make use of roll component (suitable for ropes/snakes/boids)
    glm::mat4 new_current_segment_orient_xform = local_arc_rotate_xform * get_local_orient_xform();
    glm::vec3 new_current_segment_heading      = glm::vec3(new_current_segment_orient_xform * glm::vec4(VEC_FORWARD, 1));
    glm::vec3 new_current_segment_up_direction = glm::vec3(new_current_segment_orient_xform * glm::vec4(VEC_UP, 1));
    point_at_local(new_current_segment_heading, &new_current_segment_up_direction);
#else
    // attempt #2 -- do rotations in Cartesian coordinates (suitable for robots)
    point_at_local(glm::vec3(local_arc_rotate_xform * glm::vec4(orient_to_offset(get_orient()), 1)));
#endif
    set_origin(in_abs_system(VEC_FORWARD * forward_speed));
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

glm::mat4 XformObject::get_local_orient_xform() const
{
    return GLM_EULER_ANGLE(ORIENT_YAW(m_orient), ORIENT_PITCH(m_orient), ORIENT_ROLL(m_orient));
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
