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
                                 glm::vec3   orient,
                                 glm::vec3   scale)
    : NamedObject(name),
      m_origin(origin),
      m_orient(orient),
      m_scale(scale),
      m_joint_type(                     JOINT_TYPE_REVOLUTE),
      m_enable_joint_constraints(       glm::ivec3(0)),
      m_joint_constraints_center(       glm::vec3(0)),
      m_joint_constraints_max_deviation(glm::vec3(0)),
      m_parent(NULL),
      m_is_dirty_transform(true),
      m_is_dirty_normal_transform(true)
{
}

TransformObject::~TransformObject()
{
}

void TransformObject::set_origin(glm::vec3 origin)
{
    m_origin = origin;
    apply_joint_constraints();
    mark_dirty_transform();
}

void TransformObject::set_orient(glm::vec3 orient)
{
    m_orient = orient;
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
    set_orient(glm::vec3(0));
    m_scale  = glm::vec3(1);
    mark_dirty_transform();
}

void TransformObject::apply_joint_constraints()
{
    switch(m_joint_type) {
        case JOINT_TYPE_REVOLUTE:
            for(int i = 0; i < 3; i++) {
                if(!m_enable_joint_constraints[i]) {
                    continue;
                }
                if(angle_distance(m_orient[i], m_joint_constraints_center[i]) > m_joint_constraints_max_deviation[i]) {
                    float min_value = m_joint_constraints_center[i] - m_joint_constraints_max_deviation[i];
                    float max_value = m_joint_constraints_center[i] + m_joint_constraints_max_deviation[i];
                    float distance_to_lower_bound = angle_distance(m_orient[i], min_value);
                    float distance_to_upper_bound = angle_distance(m_orient[i], max_value);
                    if(distance_to_lower_bound < distance_to_upper_bound) {
                        m_orient[i] = min_value;
                    } else {
                        m_orient[i] = max_value;
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
                    float distance_to_lower_bound = fabs(m_origin[i] - min_value);
                    float distance_to_upper_bound = fabs(m_origin[i] - max_value);
                    if(distance_to_lower_bound < distance_to_upper_bound) {
                        m_origin[i] = min_value;
                    } else {
                        m_origin[i] = max_value;
                    }
                }
            }
            break;
    }
}

glm::vec3 TransformObject::in_abs_system(glm::vec3 local_point)
{
    return glm::vec3(get_transform() * glm::vec4(local_point, 1));
}

glm::vec3 TransformObject::in_parent_system(glm::vec3 abs_point) const
{
    if(m_parent) {
        return glm::vec3(glm::inverse(m_parent->get_transform()) * glm::vec4(abs_point, 1));
    }
    return abs_point;
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

void TransformObject::link_parent(TransformObject* parent, bool keep_transform)
{
    glm::vec3 abs_origin;
    if(keep_transform) {
        abs_origin = in_abs_system();
    }
    if(parent) {
        if(keep_transform) {
            // unproject to global space and reproject to parent space
            glm::mat4 parent_inverse_transform = glm::inverse(parent->get_transform());
            flatten(&parent_inverse_transform);

            // break all connections -- TODO: review this
            link_parent(NULL, false);
            unlink_children();
        }

        // make new parent remember you
        parent->get_children().insert(this);
    } else {
        if(keep_transform) {
            // unproject to global space
            flatten();

            //link_parent(NULL); // infinite recursion
            unlink_children();
        }

        if(m_parent) {
            // make parent disown you
            std::set<TransformObject*>::iterator p = m_parent->get_children().find(this);
            if(p != m_parent->get_children().end()) {
                m_parent->get_children().erase(p);
            }
        }
    }
    m_parent = parent;
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

void TransformObject::point_at_local(glm::vec3 local_target, glm::vec3* up_direction)
{
    set_orient(offset_to_orient(local_target, up_direction));
}

void TransformObject::point_at(glm::vec3 target, glm::vec3* up_direction)
{
    point_at_local(from_origin_in_parent_system(target), up_direction);
}

void TransformObject::rotate(float angle_delta, glm::vec3 pivot)
{
    glm::mat4 rotate_transform       = GLM_ROTATE(glm::mat4(1), angle_delta, pivot);
    glm::vec3 abs_origin             = in_abs_system();
    glm::vec3 local_new_heading      = from_origin_in_parent_system(abs_origin + glm::vec3(rotate_transform * glm::vec4(get_abs_heading(), 1)));
    glm::vec3 local_new_up_direction = from_origin_in_parent_system(abs_origin + glm::vec3(rotate_transform * glm::vec4(get_abs_up_direction(), 1)));
    point_at_local(local_new_heading, &local_new_up_direction);
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
            glm::vec3 tmp_target;
            if(end_effector_dir && current_segment == this) {
                tmp_target = in_abs_system() + *end_effector_dir;
            } else {
                tmp_target = target;
            }
            if(current_segment->get_joint_type() == JOINT_TYPE_PRISMATIC) {
                current_segment->set_origin(current_segment->get_origin() + (current_segment->from_origin_in_parent_system(tmp_target) -
                                                                             current_segment->from_origin_in_parent_system(end_effector_tip)));
                continue;
            }
#if 1
            glm::vec3 local_target_dir           = glm::normalize(current_segment->from_origin_in_parent_system(tmp_target));
            glm::vec3 local_end_effector_tip_dir = glm::normalize(current_segment->from_origin_in_parent_system(end_effector_tip));
            glm::vec3 local_arc_dir              = glm::normalize(local_target_dir - local_end_effector_tip_dir);
            glm::vec3 local_arc_midpoint_dir     = glm::normalize((local_target_dir + local_end_effector_tip_dir) * 0.5f);
            glm::vec3 local_arc_pivot            = glm::cross(local_arc_dir, local_arc_midpoint_dir);
            float     angle_delta                = glm::degrees(glm::angle(local_target_dir, local_end_effector_tip_dir));
            glm::mat4 local_arc_rotate_transform = GLM_ROTATE(glm::mat4(1), -angle_delta, local_arc_pivot);
    #if 1
            // attempt #3 -- same as attempt #2, but make use of roll component (suitable for ropes/snakes/boids)
            glm::mat4 new_current_segment_orient_transform = local_arc_rotate_transform * current_segment->get_local_orient_transform();
            glm::vec3 new_current_segment_heading          = glm::vec3(new_current_segment_orient_transform * glm::vec4(VEC_FORWARD, 1));
            glm::vec3 new_current_segment_up_direction     = glm::vec3(new_current_segment_orient_transform * glm::vec4(VEC_UP, 1));
            current_segment->point_at_local(new_current_segment_heading, &new_current_segment_up_direction);

            // update guide wires
            local_target_dir           = glm::normalize(current_segment->from_origin_in_parent_system(tmp_target));
            local_end_effector_tip_dir = glm::normalize(current_segment->from_origin_in_parent_system(end_effector_tip));
            local_arc_dir              = glm::normalize(local_target_dir - local_end_effector_tip_dir);
            local_arc_midpoint_dir     = glm::normalize((local_target_dir + local_end_effector_tip_dir) * 0.5f);
            local_arc_pivot            = glm::cross(local_arc_dir, local_arc_midpoint_dir);
            current_segment->m_debug_target_dir           = local_target_dir;
            current_segment->m_debug_end_effector_tip_dir = local_end_effector_tip_dir;
            current_segment->m_debug_local_pivot          = local_arc_pivot;
            current_segment->m_debug_local_target         = current_segment->from_origin_in_parent_system(tmp_target);
        #ifdef DEBUG
            //std::cout << "TARGET: " << glm::to_string(local_target_dir) << ", END_EFF: " << glm::to_string(local_end_effector_tip_dir) << ", ANGLE: " << angle_delta << std::endl;
            //std::cout << "BEFORE: " << glm::to_string(new_current_segment_transform * glm::vec4(VEC_FORWARD, 1))
            //          << ", AFTER: " << glm::to_string(new_current_segment_heading) << std::endl;
            //std::cout << "PIVOT: " << glm::to_string(local_arc_pivot) << std::endl;
        #endif
    #else
            // attempt #2 -- do rotations in Cartesian coordinates (suitable for robots)
            current_segment->point_at_local(glm::vec3(local_arc_rotate_transform * glm::vec4(orient_to_offset(current_segment->get_orient()), 1)));
    #endif
            sum_angle += angle_delta;
#else
            // attempt #1 -- do rotations in Euler coordinates (poor man's ik)
            glm::vec3 local_target_orient           = offset_to_orient(current_segment->from_origin_in_parent_system(tmp_target));
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

void TransformObject::update_boid(glm::vec3 target,
                                  float     forward_speed,
                                  float     angle_delta,
                                  float     avoid_radius)
{
    glm::vec3 local_target_dir           = glm::normalize(from_origin_in_parent_system(target));
    glm::vec3 local_heading              = glm::normalize(from_origin_in_parent_system(in_abs_system(VEC_FORWARD)));
    glm::vec3 local_arc_dir              = glm::normalize(local_target_dir - local_heading);
    glm::vec3 local_arc_midpoint_dir     = glm::normalize((local_target_dir + local_heading) * 0.5f);
    glm::vec3 local_arc_pivot            = glm::cross(local_arc_dir, local_arc_midpoint_dir);
    glm::mat4 local_arc_rotate_transform = GLM_ROTATE(glm::mat4(1), -angle_delta * ((glm::distance(target, m_origin) < avoid_radius) ? -1 : 1), local_arc_pivot);
#if 1
    // attempt #3 -- same as attempt #2, but make use of roll component (suitable for ropes/snakes/boids)
    glm::mat4 new_current_segment_orient_transform = local_arc_rotate_transform * get_local_orient_transform();
    glm::vec3 new_current_segment_heading          = glm::vec3(new_current_segment_orient_transform * glm::vec4(VEC_FORWARD, 1));
    glm::vec3 new_current_segment_up_direction     = glm::vec3(new_current_segment_orient_transform * glm::vec4(VEC_UP, 1));
    point_at_local(new_current_segment_heading, &new_current_segment_up_direction);
#else
    // attempt #2 -- do rotations in Cartesian coordinates (suitable for robots)
    point_at_local(glm::vec3(local_arc_rotate_transform * glm::vec4(orient_to_offset(get_orient()), 1)));
#endif
    set_origin(in_abs_system(VEC_FORWARD * forward_speed));
}

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

glm::mat4 TransformObject::get_local_orient_transform() const
{
    return GLM_EULER_ANGLE(ORIENT_YAW(m_orient), ORIENT_PITCH(m_orient), ORIENT_ROLL(m_orient));
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
