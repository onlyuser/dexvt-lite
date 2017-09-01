#include <TransformObject.h>
#include <NamedObject.h>
#include <Util.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/glm.hpp>
#include <set>

//#define DEBUG

namespace vt {

TransformObject::TransformObject(std::string name,
                                 glm::vec3   origin,
                                 glm::vec3   euler,
                                 glm::vec3   scale)
    : NamedObject(name),
      m_origin(origin),
      m_euler(euler),
      m_scale(scale),
      m_joint_type(                     JOINT_TYPE_REVOLUTE),
      m_enable_joint_constraints(       glm::ivec3(0)),
      m_joint_constraints_center(       glm::vec3(0)),
      m_joint_constraints_max_deviation(glm::vec3(0)),
      m_hinge_type(-1),
      m_enable_constraints_within_plane_of_free_rotation(false),
      m_parent(NULL),
      m_is_dirty_transform(true),
      m_is_dirty_normal_transform(true)
{
}

TransformObject::~TransformObject()
{
}

//===============
// basic features
//===============

void TransformObject::set_origin(glm::vec3 origin)
{
    m_origin = origin;
    apply_joint_constraints();
    mark_dirty_transform();
}

void TransformObject::set_euler(glm::vec3 euler)
{
    m_euler = euler;
    apply_joint_constraints();
    mark_dirty_transform();
}

void TransformObject::set_scale(glm::vec3 scale)
{
    m_scale = scale;
    mark_dirty_transform();
}

void TransformObject::reset_transform()
{
    m_origin = glm::vec3(0);
    set_euler(glm::vec3(0));
    m_scale = glm::vec3(1);
    mark_dirty_transform();
}

//==============================
// coordinate system conversions
//==============================

glm::vec3 TransformObject::in_abs_system(glm::vec3 local_point)
{
    return glm::vec3(get_transform() * glm::vec4(local_point, 1));
}

glm::vec3 TransformObject::in_parent_system(glm::vec3 abs_point) const
{
    if(!m_parent) {
        return abs_point;
    }
    return glm::vec3(glm::inverse(m_parent->get_transform()) * glm::vec4(abs_point, 1));
}

glm::vec3 TransformObject::from_origin_in_parent_system(glm::vec3 abs_point) const
{
    return in_parent_system(abs_point) - m_origin;
}

glm::vec3 TransformObject::get_abs_left_direction()
{
    return glm::vec3(get_normal_transform() * glm::vec4(VEC_LEFT, 1));
}

glm::vec3 TransformObject::get_abs_up_direction()
{
    return glm::vec3(get_normal_transform() * glm::vec4(VEC_UP, 1));
}

glm::vec3 TransformObject::get_abs_heading()
{
    return glm::vec3(get_normal_transform() * glm::vec4(VEC_FORWARD, 1));
}

//=============================
// coordinate system operations
//=============================

void TransformObject::point_at_local(glm::vec3 local_target, glm::vec3* local_up_direction)
{
    set_euler(offset_to_euler(local_target, local_up_direction));
}

void TransformObject::set_local_rotation_transform(glm::mat4 rotation_transform)
{
    glm::vec3 local_heading      = glm::vec3(rotation_transform * glm::vec4(VEC_FORWARD, 1));
    glm::vec3 local_up_direction = glm::vec3(rotation_transform * glm::vec4(VEC_UP, 1));
    point_at_local(local_heading, &local_up_direction);
}

void TransformObject::rotate(glm::mat4 rotation_transform)
{
    glm::vec3 abs_origin         = in_abs_system();
    glm::vec3 local_heading      = from_origin_in_parent_system(abs_origin + glm::vec3(rotation_transform * glm::vec4(get_abs_heading(), 1)));
    glm::vec3 local_up_direction = from_origin_in_parent_system(abs_origin + glm::vec3(rotation_transform * glm::vec4(get_abs_up_direction(), 1)));
    point_at_local(local_heading, &local_up_direction);
}

void TransformObject::rotate(float angle_delta, glm::vec3 pivot)
{
    rotate(GLM_ROTATION_TRANSFORM(glm::mat4(1), angle_delta, pivot));
}

//==================
// hierarchy related
//==================

void TransformObject::link_parent(TransformObject* new_parent, bool keep_transform)
{
    glm::vec3 abs_origin;
    if(keep_transform) {
        abs_origin = in_abs_system();
    }
    if(new_parent) {
        if(keep_transform) {
            // unproject to global space and then reproject to new parent space
            glm::mat4 new_parent_inverse_transform = glm::inverse(new_parent->get_transform());
            flatten(&new_parent_inverse_transform);

            // break all connections -- TODO: review this
            link_parent(NULL, false);
            unlink_children();
        }

        // make new new parent remember you
        new_parent->get_children().insert(this);
    } else {
        if(keep_transform) {
            // unproject to global space
            flatten();

            //link_parent(NULL); // infinite recursion
            unlink_children();
        }

        if(m_parent) {
            // make old parent forget you
            std::set<TransformObject*>::iterator p = m_parent->get_children().find(this);
            if(p != m_parent->get_children().end()) {
                m_parent->get_children().erase(p);
            }
        }
    }
    m_parent = new_parent;
    if(keep_transform) {
        set_axis(abs_origin);
    } else {
        reset_transform();
    }
}

void TransformObject::unlink_children()
{
    for(std::set<TransformObject*>::iterator p = m_children.begin(); p != m_children.end(); p++) {
        (*p)->link_parent(NULL);
    }
}

//==================
// joint constraints
//==================

void TransformObject::apply_hinge_constraints_perpendicular_to_plane_of_free_rotation()
{
    static bool disable_recursion = false;
    if(!is_hinge() || disable_recursion) {
        return;
    }
    glm::vec3 local_heading;
    glm::vec3 local_up_dir;
    glm::vec3 parent_plane_origin = m_parent ? m_parent->in_abs_system() : glm::vec3(0);
    if(!m_parent) {
        mark_dirty_transform(); // strangely necessary, otherwise absolute axis endpoints aren't calculated
    }
    glm::vec3 joint_origin                    = in_abs_system();
    glm::vec3 joint_abs_left_axis_endpoint    = joint_origin + get_abs_left_direction(); // X
    glm::vec3 joint_abs_up_axis_endpoint      = joint_origin + get_abs_up_direction();   // Y
    glm::vec3 joint_abs_heading_axis_endpoint = joint_origin + get_abs_heading();        // Z
    switch(m_hinge_type) {
        case EULER_INDEX_ROLL:
            {
                // allow ONLY roll -- project onto XY plane
                glm::vec3 parent_plane_normal                    = m_parent ? m_parent->get_abs_heading() : VEC_FORWARD;                                           // Z
                glm::vec3 joint_flattened_abs_left_axis_endpoint = nearest_point_on_plane(parent_plane_origin, parent_plane_normal, joint_abs_left_axis_endpoint); // X
                glm::vec3 joint_flattened_abs_up_axis_endpoint   = nearest_point_on_plane(parent_plane_origin, parent_plane_normal, joint_abs_up_axis_endpoint);   // Y
                glm::vec3 local_left_dir                         = from_origin_in_parent_system(joint_flattened_abs_left_axis_endpoint); // X
                          local_up_dir                           = from_origin_in_parent_system(joint_flattened_abs_up_axis_endpoint);   // Y
                          local_heading                          = glm::normalize(glm::cross(local_left_dir, local_up_dir));             // Z
            }
            break;
        case EULER_INDEX_PITCH:
            {
                // allow ONLY pitch -- project onto YZ plane
                glm::vec3 parent_plane_normal                  = m_parent ? m_parent->get_abs_left_direction() : VEC_LEFT;                                          // X
                glm::vec3 joint_flattened_abs_up_axis_endpoint = nearest_point_on_plane(parent_plane_origin, parent_plane_normal, joint_abs_up_axis_endpoint);      // Y
                glm::vec3 joint_flattened_abs_heading_endpoint = nearest_point_on_plane(parent_plane_origin, parent_plane_normal, joint_abs_heading_axis_endpoint); // Z
                          local_up_dir                         = from_origin_in_parent_system(joint_flattened_abs_up_axis_endpoint); // Y
                          local_heading                        = from_origin_in_parent_system(joint_flattened_abs_heading_endpoint); // Z
            }
            break;
        case EULER_INDEX_YAW:
            {
                // allow ONLY yaw -- project onto XZ plane
                glm::vec3 parent_plane_normal                    = m_parent ? m_parent->get_abs_up_direction() : VEC_UP;                                              // Y
                glm::vec3 joint_flattened_abs_heading_endpoint   = nearest_point_on_plane(parent_plane_origin, parent_plane_normal, joint_abs_heading_axis_endpoint); // Z
                glm::vec3 joint_flattened_abs_left_axis_endpoint = nearest_point_on_plane(parent_plane_origin, parent_plane_normal, joint_abs_left_axis_endpoint);    // X
                          local_heading                          = from_origin_in_parent_system(joint_flattened_abs_heading_endpoint);   // Z
                glm::vec3 local_left_dir                         = from_origin_in_parent_system(joint_flattened_abs_left_axis_endpoint); // X
                          local_up_dir                           = glm::normalize(glm::cross(local_heading, local_left_dir));            // Y
            }
            break;
    }
    disable_recursion = true;
    point_at_local(local_heading, &local_up_dir);
    disable_recursion = false;
}

void TransformObject::apply_hinge_constraints_within_plane_of_free_rotation()
{
    if(!is_hinge()) {
        return;
    }
    glm::vec3 parent_abs_origin;
    glm::mat4 parent_transform;
    if(m_parent) {
        parent_abs_origin = m_parent->in_abs_system();
        parent_transform  = m_parent->get_transform();
    } else {
        parent_abs_origin = glm::vec3(0);
        parent_transform  = glm::mat4(1);
    }
    glm::vec3 abs_heading = get_abs_heading();
    switch(m_hinge_type) {
        case EULER_INDEX_ROLL:
            {
                // allow ONLY roll -- apply constraint within XY plane
                glm::vec3 center_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_joint_constraints_center), EULER_PITCH(m_euler), EULER_YAW(m_euler)));
                glm::vec3 center_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(center_local_offset, 1)) - parent_abs_origin);
                if(glm::degrees(glm::angle(abs_heading, center_dir)) > EULER_ROLL(m_joint_constraints_max_deviation)) {
                    float min_value = EULER_ROLL(m_joint_constraints_center) - EULER_ROLL(m_joint_constraints_max_deviation);
                    float max_value = EULER_ROLL(m_joint_constraints_center) + EULER_ROLL(m_joint_constraints_max_deviation);
                    glm::vec3 min_local_offset = euler_to_offset(glm::vec3(min_value, EULER_PITCH(m_euler), EULER_YAW(m_euler)));
                    glm::vec3 max_local_offset = euler_to_offset(glm::vec3(max_value, EULER_PITCH(m_euler), EULER_YAW(m_euler)));
                    glm::vec3 min_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(min_local_offset, 1)) - parent_abs_origin);
                    glm::vec3 max_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(max_local_offset, 1)) - parent_abs_origin);
                    EULER_ROLL(m_euler) = (glm::distance(abs_heading, min_dir) < glm::distance(abs_heading, max_dir)) ? min_value : max_value;
                    mark_dirty_transform();
                }
            }
            break;
        case EULER_INDEX_PITCH:
            {
                // allow ONLY pitch -- apply constraint within YZ plane
                glm::vec3 center_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_euler), EULER_PITCH(m_joint_constraints_center), EULER_YAW(m_euler)));
                glm::vec3 center_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(center_local_offset, 1)) - parent_abs_origin);
                if(glm::degrees(glm::angle(abs_heading, center_dir)) > EULER_PITCH(m_joint_constraints_max_deviation)) {
                    float min_value = EULER_PITCH(m_joint_constraints_center) - EULER_PITCH(m_joint_constraints_max_deviation);
                    float max_value = EULER_PITCH(m_joint_constraints_center) + EULER_PITCH(m_joint_constraints_max_deviation);
                    glm::vec3 min_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_euler), min_value, EULER_YAW(m_euler)));
                    glm::vec3 max_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_euler), max_value, EULER_YAW(m_euler)));
                    glm::vec3 min_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(min_local_offset, 1)) - parent_abs_origin);
                    glm::vec3 max_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(max_local_offset, 1)) - parent_abs_origin);
                    EULER_PITCH(m_euler) = (glm::distance(abs_heading, min_dir) < glm::distance(abs_heading, max_dir)) ? min_value : max_value;
                    mark_dirty_transform();
                }
            }
            break;
        case EULER_INDEX_YAW:
            {
                // allow ONLY yaw -- apply constraint within XZ plane
                glm::vec3 center_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_euler), EULER_PITCH(m_euler), EULER_YAW(m_joint_constraints_center)));
                glm::vec3 center_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(center_local_offset, 1)) - parent_abs_origin);
                if(glm::degrees(glm::angle(abs_heading, center_dir)) > EULER_YAW(m_joint_constraints_max_deviation)) {
                    float min_value = EULER_YAW(m_joint_constraints_center) - EULER_YAW(m_joint_constraints_max_deviation);
                    float max_value = EULER_YAW(m_joint_constraints_center) + EULER_YAW(m_joint_constraints_max_deviation);
                    glm::vec3 min_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_euler), EULER_PITCH(m_euler), min_value));
                    glm::vec3 max_local_offset = euler_to_offset(glm::vec3(EULER_ROLL(m_euler), EULER_PITCH(m_euler), max_value));
                    glm::vec3 min_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(min_local_offset, 1)) - parent_abs_origin);
                    glm::vec3 max_dir = glm::normalize(glm::vec3(parent_transform * glm::vec4(max_local_offset, 1)) - parent_abs_origin);
                    EULER_YAW(m_euler) = (glm::distance(abs_heading, min_dir) < glm::distance(abs_heading, max_dir)) ? min_value : max_value;
                    mark_dirty_transform();
                }
            }
            break;
    }
}

void TransformObject::apply_joint_constraints()
{
    switch(m_joint_type) {
        case JOINT_TYPE_REVOLUTE:
            if(is_hinge()) {
                apply_hinge_constraints_perpendicular_to_plane_of_free_rotation();
                apply_hinge_constraints_within_plane_of_free_rotation();
            } else {
                for(int i = 0; i < 3; i++) {
                    if(!m_enable_joint_constraints[i]) {
                        continue;
                    }
                    if(angle_distance(m_euler[i], m_joint_constraints_center[i]) > m_joint_constraints_max_deviation[i]) {
                        float min_value = m_joint_constraints_center[i] - m_joint_constraints_max_deviation[i];
                        float max_value = m_joint_constraints_center[i] + m_joint_constraints_max_deviation[i];
                        m_euler[i] = (angle_distance(m_euler[i], min_value) < angle_distance(m_euler[i], max_value)) ? min_value : max_value;
                        mark_dirty_transform();
                    }
                }
            }
            break;
        case JOINT_TYPE_PRISMATIC:
            for(int i = 0; i < 3; i++) {
                if(!m_enable_joint_constraints[i]) {
                    continue;
                }
                if(fabs(m_origin[i] - m_joint_constraints_center[i]) > m_joint_constraints_max_deviation[i]) {
                    float min_value = m_joint_constraints_center[i] - m_joint_constraints_max_deviation[i];
                    float max_value = m_joint_constraints_center[i] + m_joint_constraints_max_deviation[i];
                    m_origin[i] = (fabs(m_origin[i] - min_value) < fabs(m_origin[i] - max_value)) ? min_value : max_value;
                    mark_dirty_transform();
                }
            }
            break;
    }
}

//==================
// advanced features
//==================

void TransformObject::arcball(glm::vec3* local_arc_pivot_dir,
                              float*     angle_delta,
                              glm::vec3  abs_target,
                              glm::vec3  abs_reference_point)
{
    glm::vec3 local_target_dir          = glm::normalize(from_origin_in_parent_system(abs_target));
    glm::vec3 local_reference_point_dir = glm::normalize(from_origin_in_parent_system(abs_reference_point));
    glm::vec3 local_arc_delta_dir       = glm::normalize(local_target_dir - local_reference_point_dir);
    glm::vec3 local_arc_midpoint_dir    = glm::normalize((local_target_dir + local_reference_point_dir) * 0.5f);
    if(local_arc_pivot_dir) {
        *local_arc_pivot_dir = glm::cross(local_arc_delta_dir, local_arc_midpoint_dir);
    }
    if(angle_delta) {
        *angle_delta = glm::degrees(glm::angle(local_target_dir, local_reference_point_dir));
    }
}

void TransformObject::project_to_plane_of_free_rotation(glm::vec3* target, glm::vec3* end_effector_tip)
{
    if(m_hinge_type != -1) {
        glm::vec3 plane_origin = in_abs_system();
        glm::vec3 plane_normal;
        switch(m_hinge_type) {
            case EULER_INDEX_ROLL:
                // allow ONLY roll -- project onto XY plane
                plane_normal = get_abs_heading();
                break;
            case EULER_INDEX_PITCH:
                // allow ONLY pitch -- project onto YZ plane
                plane_normal = get_abs_left_direction();
                break;
            case EULER_INDEX_YAW:
                // allow ONLY yaw -- project onto XZ plane
                plane_normal = get_abs_up_direction();
                break;
        }
        if(target) {
            *target = nearest_point_on_plane(plane_origin, plane_normal, *target);
        }
        if(end_effector_tip) {
            *end_effector_tip = nearest_point_on_plane(plane_origin, plane_normal, *end_effector_tip);
        }
    }
}

// http://what-when-how.com/advanced-methods-in-computer-graphics/kinematics-advanced-methods-in-computer-graphics-part-4/
bool TransformObject::solve_ik_ccd(TransformObject* root,
                                   glm::vec3        local_end_effector_tip,
                                   glm::vec3        target,
                                   glm::vec3*       end_effector_dir,
                                   int              iters,
                                   float            accept_end_effector_distance,
                                   float            accept_avg_angle_distance)
{
    bool converge = false;
    bool find_solution = false;
    for(int i = 0; i < iters && !converge; i++) {
        int segment_count = 0;
        float sum_angle = 0;
        for(TransformObject* current_segment = this; current_segment && current_segment != root->get_parent(); current_segment = current_segment->get_parent()) {
            glm::vec3 end_effector_tip = in_abs_system(local_end_effector_tip);
            find_solution = (glm::distance(end_effector_tip, target) < accept_end_effector_distance);
            glm::vec3 _target;
            if(end_effector_dir && current_segment == this) {
                _target = in_abs_system() + *end_effector_dir;
            } else {
                _target = target;
            }
            if(current_segment->get_joint_type() == JOINT_TYPE_PRISMATIC) {
                current_segment->set_origin(current_segment->get_origin() + (current_segment->from_origin_in_parent_system(_target) -
                                                                             current_segment->from_origin_in_parent_system(end_effector_tip)));
                continue;
            }
            current_segment->project_to_plane_of_free_rotation(&_target, &end_effector_tip);
#if 1
            glm::vec3 local_arc_pivot_dir;
            float angle_delta = 0;
            current_segment->arcball(&local_arc_pivot_dir, &angle_delta, _target, end_effector_tip);
            glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), -angle_delta, local_arc_pivot_dir);
    #if 1
            // attempt #3 -- same as attempt #2, but make use of roll component (suitable for ropes/snakes/boids)
            current_segment->set_local_rotation_transform(local_arc_rotation_transform * current_segment->get_local_rotation_transform());
            // update guide wires (for debug)
            glm::vec3 debug_local_target_dir           = glm::normalize(current_segment->from_origin_in_parent_system(_target));
            glm::vec3 debug_local_end_effector_tip_dir = glm::normalize(current_segment->from_origin_in_parent_system(end_effector_tip));
            glm::vec3 debug_local_arc_delta_dir        = glm::normalize(debug_local_target_dir - debug_local_end_effector_tip_dir);
            glm::vec3 debug_local_arc_midpoint_dir     = glm::normalize((debug_local_target_dir + debug_local_end_effector_tip_dir) * 0.5f);
            glm::vec3 debug_local_arc_pivot_dir        = glm::cross(debug_local_arc_delta_dir, debug_local_arc_midpoint_dir);
            current_segment->m_debug_target_dir           = debug_local_target_dir;
            current_segment->m_debug_end_effector_tip_dir = debug_local_end_effector_tip_dir;
            current_segment->m_debug_local_pivot          = debug_local_arc_pivot_dir;
            current_segment->m_debug_local_target         = current_segment->from_origin_in_parent_system(_target);
        #ifdef DEBUG
            //std::cout << "TARGET: " << glm::to_string(local_target_dir) << ", END_EFF: " << glm::to_string(local_end_effector_tip_dir) << ", ANGLE: " << angle_delta << std::endl;
            //std::cout << "BEFORE: " << glm::to_string(new_current_segment_transform * glm::vec4(VEC_FORWARD, 1))
            //          << ", AFTER: " << glm::to_string(new_current_segment_heading) << std::endl;
            //std::cout << "PIVOT: " << glm::to_string(local_arc_pivot_dir) << std::endl;
        #endif
    #else
            // attempt #2 -- do rotations in Cartesian coordinates (suitable for robots)
            current_segment->point_at_local(glm::vec3(local_arc_rotation_transform * glm::vec4(euler_to_offset(current_segment->get_euler()), 1)));
    #endif
            sum_angle += angle_delta;
#else
            // attempt #1 -- do rotations in Euler coordinates (poor man's ik)
            glm::vec3 local_target_euler           = offset_to_euler(current_segment->from_origin_in_parent_system(_target));
            glm::vec3 local_end_effector_tip_euler = offset_to_euler(current_segment->from_origin_in_parent_system(end_effector_tip));
            current_segment->set_euler(euler_modulo(current_segment->get_euler() + euler_modulo(local_target_euler - local_end_effector_tip_euler)));
            sum_angle += accept_avg_angle_distance; // to avoid convergence
#endif
#ifdef DEBUG
            std::cout << "NAME: " << current_segment->get_name() << ", EULER: " << glm::to_string(current_segment->get_euler()) << std::endl;
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

void TransformObject::update_boid(glm::vec3 target,
                                  float     forward_speed,
                                  float     angle_delta,
                                  float     avoid_radius)
{
    glm::vec3 local_arc_pivot_dir;
    arcball(&local_arc_pivot_dir, NULL, target, in_abs_system(VEC_FORWARD));
    int avoid_or_seek = (glm::distance(target, m_origin) < avoid_radius) ? -1 : 1;
    glm::mat4 local_arc_rotation_transform = GLM_ROTATION_TRANSFORM(glm::mat4(1), -angle_delta * avoid_or_seek, local_arc_pivot_dir);
#if 1
    // attempt #3 -- same as attempt #2, but make use of roll component (suitable for ropes/snakes/boids)
    set_local_rotation_transform(local_arc_rotation_transform * get_local_rotation_transform());
#else
    // attempt #2 -- do rotations in Cartesian coordinates (suitable for robots)
    point_at_local(glm::vec3(local_arc_rotation_transform * glm::vec4(euler_to_offset(get_euler()), 1)));
#endif
    set_origin(in_abs_system(VEC_FORWARD * forward_speed));
}

//===================
// core functionality
//===================

const glm::mat4 &TransformObject::get_transform(bool trace_down)
{
    if(trace_down) {
        update_transform_hier();
        return m_transform;
    }
    if(m_is_dirty_transform) {
        update_transform();
        if(m_parent) {
            m_transform = m_parent->get_transform(false) * m_transform;
        }
        m_is_dirty_transform = false;
    }
    return m_transform;
}

const glm::mat4 &TransformObject::get_normal_transform()
{
    if(m_is_dirty_normal_transform) {
        update_normal_transform();
        m_is_dirty_normal_transform = false;
    }
    return m_normal_transform;
}

glm::mat4 TransformObject::get_local_rotation_transform() const
{
    return GLM_EULER_TRANSFORM(EULER_YAW(m_euler), EULER_PITCH(m_euler), EULER_ROLL(m_euler));
}

void TransformObject::update_transform_hier()
{
    for(std::set<TransformObject*>::iterator p = m_children.begin(); p != m_children.end(); p++) {
        (*p)->mark_dirty_transform(); // mark entire subtree dirty
        (*p)->update_transform_hier();
    }
    if(m_children.empty()) {
        get_transform(false); // update entire leaf-to-root lineage for all leaf nodes
    }
}

void TransformObject::update_normal_transform()
{
    m_normal_transform = glm::transpose(glm::inverse(get_transform()));
}

}
