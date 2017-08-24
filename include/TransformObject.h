#ifndef VT_TRANSFORM_OBJECT_H_
#define VT_TRANSFORM_OBJECT_H_

#include <NamedObject.h>
#include <Util.h>
#include <glm/glm.hpp>
#include <set>

namespace vt {

class TransformObject : public NamedObject
{
public:
    enum joint_type_t {
        JOINT_TYPE_REVOLUTE,
        JOINT_TYPE_PRISMATIC
    };

    // guide wires (for debug)
    glm::vec3 m_debug_target_dir;
    glm::vec3 m_debug_end_effector_tip_dir;
    glm::vec3 m_debug_local_pivot;
    glm::vec3 m_debug_local_target;

    TransformObject(std::string name,
                    glm::vec3   origin = glm::vec3(0),
                    glm::vec3   euler  = glm::vec3(0),
                    glm::vec3   scale  = glm::vec3(1));
    virtual ~TransformObject();

    // basic features
    const glm::vec3 &get_origin() const { return m_origin; }
    void set_origin(glm::vec3 origin);
    const glm::vec3 &get_euler() const { return m_euler; }
    void set_euler(glm::vec3 euler);
    const glm::vec3 &get_scale() const { return m_scale; }
    void set_scale(glm::vec3 scale);
    void reset_transform();

    // coordinate system conversions
    glm::vec3 in_abs_system(glm::vec3 local_point = glm::vec3(0));
    glm::vec3 in_parent_system(glm::vec3 abs_point) const;
    glm::vec3 from_origin_in_parent_system(glm::vec3 abs_point) const;
    glm::vec3 get_abs_left_direction();
    glm::vec3 get_abs_up_direction();
    glm::vec3 get_abs_heading();

    // coordinate system operations
    void point_at_local(glm::vec3 local_target, glm::vec3* local_up_direction = NULL);
    void set_local_rotation_transform(glm::mat4 local_rotation_transform);
    void rotate(glm::mat4 rotation_transform);
    void rotate(float angle_delta, glm::vec3 pivot);

    // hierarchy related
    void link_parent(TransformObject* new_parent, bool keep_transform = false);
    TransformObject* get_parent() const        { return m_parent; }
    std::set<TransformObject*> &get_children() { return m_children; }
    void unlink_children();

    // joint constraints
    joint_type_t get_joint_type() const                                                 { return m_joint_type; }
    void set_joint_type(joint_type_t joint_type)                                        { m_joint_type = joint_type; }
    const glm::ivec3 &get_enable_joint_constraints() const                              { return m_enable_joint_constraints; }
    void set_enable_joint_constraints(glm::ivec3 enable_joint_constraints);
    const glm::vec3 &get_joint_constraints_center() const                               { return m_joint_constraints_center; }
    void set_joint_constraints_center(glm::vec3 joint_constraints_center)               { m_joint_constraints_center = joint_constraints_center; }
    const glm::vec3 &get_joint_constraints_max_deviation() const                        { return m_joint_constraints_max_deviation; }
    void set_joint_constraints_max_deviation(glm::vec3 joint_constraints_max_deviation) { m_joint_constraints_max_deviation = joint_constraints_max_deviation; }
    void set_eclusive_pivot(int exclusive_pivot)                                        { m_exclusive_pivot = exclusive_pivot; }
    int get_exclusive_pivot() const                                                     { return m_exclusive_pivot; }
    void apply_joint_constraints();
    bool apply_exclusive_pivot_constraints();

    // advanced features
    void arcball(glm::vec3* local_arc_pivot_dir,
                 float*     angle_delta,
                 glm::vec3  abs_target,
                 glm::vec3  abs_reference_point);
    bool solve_ik_ccd(TransformObject* root,
                      glm::vec3        local_end_effector_tip,
                      glm::vec3        target,
                      glm::vec3*       end_effector_dir,
                      int              iters,
                      float            accept_end_effector_distance,
                      float            accept_avg_angle_distance);
    void update_boid(glm::vec3 target,
                     float     forward_speed,
                     float     angle_delta,
                     float     avoid_radius);

    // core functionality
    const glm::mat4 &get_transform(bool trace_down = true);
    const glm::mat4 &get_normal_transform();
    glm::mat4 get_local_rotation_transform() const;

protected:
    // basic features
    glm::vec3 m_origin;
    glm::vec3 m_euler;
    glm::vec3 m_scale;
    glm::mat4 m_transform;
    glm::mat4 m_normal_transform;

    // joint constraints
    joint_type_t m_joint_type;
    glm::ivec3   m_enable_joint_constraints;
    glm::vec3    m_joint_constraints_center;
    glm::vec3    m_joint_constraints_max_deviation;
    int          m_exclusive_pivot;

    // hierarchy related
    TransformObject*           m_parent;
    std::set<TransformObject*> m_children;

    // caching
    void mark_dirty_transform() {
        m_is_dirty_transform        = true;
        m_is_dirty_normal_transform = true;
    }

    // caching (to be implemented by derived class)
    virtual void update_transform() = 0;

private:
    // caching
    bool m_is_dirty_transform;
    bool m_is_dirty_normal_transform;

    // optional advanced features (to be implemented by derived class, if needed)
    virtual void flatten(glm::mat4* basis = NULL) {}
    virtual void set_axis(glm::vec3 axis) {}

    // caching
    void update_transform_hier();
    void update_normal_transform();
};

}

#endif
