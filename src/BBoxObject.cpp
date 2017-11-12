// This file is part of dexvt-lite.
// -- 3D Inverse Kinematics (Cyclic Coordinate Descent) with Constraints
// Copyright (C) 2018 onlyuser <mailto:onlyuser@gmail.com>
//
// dexvt-lite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dexvt-lite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dexvt-lite.  If not, see <http://www.gnu.org/licenses/>.

#include <BBoxObject.h>
#include <PrimitiveFactory.h>
#include <TransformObject.h>
#include <glm/glm.hpp>

namespace vt {

BBoxObject::BBoxObject()
{
}

BBoxObject::BBoxObject(glm::vec3 min, glm::vec3 max)
    : m_min(min),
      m_max(max)
{
}

void BBoxObject::set_min_max(glm::vec3 min, glm::vec3 max)
{
    m_min = min;
    m_max = max;
}

void BBoxObject::get_min_max(glm::vec3* min, glm::vec3* max) const
{
    if(!min || !max) {
        return;
    }
    *min = m_min;
    *max = m_max;
}

glm::vec3 BBoxObject::get_center(align_t align) const
{
    glm::vec3 center = (m_min + m_max) * 0.5f;
    if(align == ALIGN_CENTER) {
        return center;
    }
    glm::vec3 half_dim = (m_max - m_min) * 0.5f;
    switch(align) {
        case ALIGN_X_MIN:
            center.x -= half_dim.x;
            break;
        case ALIGN_X_MAX:
            center.x += half_dim.x;
            break;
        case ALIGN_Y_MIN:
            center.y -= half_dim.y;
            break;
        case ALIGN_Y_MAX:
            center.y += half_dim.y;
            break;
        case ALIGN_Z_MIN:
            center.z -= half_dim.z;
            break;
        case ALIGN_Z_MAX:
            center.z += half_dim.z;
            break;
        default:
            break;
    }
    return center;
}

bool BBoxObject::is_within(glm::vec3 pos) const
{
    glm::vec3 _min = m_min - glm::vec3(EPSILON);
    glm::vec3 _max = m_max + glm::vec3(EPSILON);
    return pos.x > _min.x && pos.y > _min.y && pos.z > _min.z &&
           pos.x < _max.x && pos.y < _max.y && pos.z < _max.z;
}

glm::vec3 BBoxObject::limit(glm::vec3 pos) const
{
    glm::vec3 _pos = pos;
    _pos.x = std::min(_pos.x, m_max.x);
    _pos.y = std::min(_pos.y, m_max.y);
    _pos.z = std::min(_pos.z, m_max.z);
    _pos.x = std::max(_pos.x, m_min.x);
    _pos.y = std::max(_pos.y, m_min.y);
    _pos.z = std::max(_pos.z, m_min.z);
    return _pos;
}

glm::vec3 BBoxObject::wrap(glm::vec3 pos) const
{
    glm::vec3 _pos = pos;
    if(_pos.x < m_min.x) { _pos.x = m_max.x; }
    if(_pos.y < m_min.y) { _pos.y = m_max.y; }
    if(_pos.z < m_min.z) { _pos.z = m_max.z; }
    if(_pos.x > m_max.x) { _pos.x = m_min.x; }
    if(_pos.y > m_max.y) { _pos.y = m_min.y; }
    if(_pos.z > m_max.z) { _pos.z = m_min.z; }
    return _pos;
}

// "separating axis theory"
// https://gamedev.stackexchange.com/questions/25397/obb-vs-obb-collision-detection
bool BBoxObject::is_bbox_collide(TransformObject* self_transform_object,
                                 TransformObject* other_transform_object,
                                 BBoxObject*      other_bbox_object)
{
    glm::vec3 other_local_bbox_extents[2];
    other_bbox_object->get_min_max(&other_local_bbox_extents[0], &other_local_bbox_extents[1]);

    // test if bbox radii touch
    glm::vec3 self_bbox_center  = self_transform_object->in_abs_system((m_max + m_min) * 0.5f);
    float     self_bbox_radius  = glm::distance(m_min, m_max) * 0.5f;
    glm::vec3 other_bbox_center = other_transform_object->in_abs_system((other_local_bbox_extents[1] + other_local_bbox_extents[0]) * 0.5f);
    float     other_bbox_radius = glm::distance(other_local_bbox_extents[0], other_local_bbox_extents[1]) * 0.5f;
    if(glm::distance(self_bbox_center, other_bbox_center) > self_bbox_radius + other_bbox_radius) {
        return false; // if not, no point in testing OOB collision
    }

    // =========================================
    // convert bbox corners to world coordinates
    // =========================================

    glm::vec3 self_abs_points[8];
    glm::vec3 other_abs_points[8];

    {
        glm::vec3 self_abs_bbox[2][2][2];
        glm::vec3 other_abs_bbox[2][2][2];

        {
            glm::mat4 self_transform  = self_transform_object->get_transform();
            glm::mat4 other_transform = other_transform_object->get_transform();
            glm::vec3 self_local_bbox_extents[2];
            self_local_bbox_extents[0] = m_min;
            self_local_bbox_extents[1] = m_max;
            for(int i = 0; i < 2; i++) {
                for(int j = 0; j < 2; j++) {
                    for(int k = 0; k < 2; k++) {
                        self_abs_bbox[i][j][k] = glm::vec3(self_transform * glm::vec4(self_local_bbox_extents[i].x,
                                                                                      self_local_bbox_extents[j].y,
                                                                                      self_local_bbox_extents[k].z, 1));
                        other_abs_bbox[i][j][k] = glm::vec3(other_transform * glm::vec4(other_local_bbox_extents[i].x,
                                                                                        other_local_bbox_extents[j].y,
                                                                                        other_local_bbox_extents[k].z, 1));
                    }
                }
            }
        }

        glm::vec3 points[8];

        // points
        //
        //     y
        //     4-------7
        //    /|      /|
        //   / |     / |
        //  5-------6  |
        //  |  0----|--3 x
        //  | /     | /
        //  |/      |/
        //  1-------2
        // z

        vt::PrimitiveFactory::get_box_corners(points);

        for(int i = 0; i < 8; i++) {
            glm::ivec3 bbox_extents_indices(static_cast<int>(points[i].x),
                                            static_cast<int>(points[i].y),
                                            static_cast<int>(points[i].z));
            self_abs_points[i]  = self_abs_bbox[bbox_extents_indices.x][bbox_extents_indices.y][bbox_extents_indices.z];
            other_abs_points[i] = other_abs_bbox[bbox_extents_indices.x][bbox_extents_indices.y][bbox_extents_indices.z];
        }
    }

    // ==================================
    // generate potential separation axes
    // ==================================

    glm::vec3 principle_axes[3 + 3 + 9];
    int k = 0;

    // generate potential face-to-vertex collision axes
    for(int i = 0; i < 3; i++) {
        principle_axes[k] = self_transform_object->get_abs_direction(static_cast<euler_index_t>(i));
        k++;
    }
    for(int i = 0; i < 3; i++) {
        principle_axes[k] = other_transform_object->get_abs_direction(static_cast<euler_index_t>(i));
        k++;
    }

    // generate potential edge-to-edge collision axes
    // https://gamedev.stackexchange.com/questions/44500/how-many-and-which-axes-to-use-for-3d-obb-collision-with-sat
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            glm::vec3 self_axis  = self_transform_object->get_abs_direction(static_cast<euler_index_t>(i));
            glm::vec3 other_axis = other_transform_object->get_abs_direction(static_cast<euler_index_t>(j));
            principle_axes[k] = glm::normalize(glm::cross(self_axis, other_axis));
            k++;
        }
    }

    // =======================================
    // test bbox overlap along separation axes
    // =======================================

    // for each potential separation axis
    for(int i = 0; i < k; i++) {
        glm::vec3 separation_axis = principle_axes[i];

        // calculate self bbox maximum extents along axis
        float self_projected_vert_min =  FLT_MAX;
        float self_projected_vert_max = -FLT_MAX;
        for(int j = 0; j < 8; j++) {
            float self_projected_vert_value = glm::dot(separation_axis, self_abs_points[j]);
            self_projected_vert_min = std::min(self_projected_vert_value, self_projected_vert_min);
            self_projected_vert_max = std::max(self_projected_vert_value, self_projected_vert_max);
        }

        // calculate other bbox maximum extents along axis
        float other_projected_vert_min = FLT_MAX;
        float other_projected_vert_max = -FLT_MAX;
        for(int j = 0; j < 8; j++) {
            float other_projected_vert_value = glm::dot(separation_axis, other_abs_points[j]);
            other_projected_vert_min = std::min(other_projected_vert_value, other_projected_vert_min);
            other_projected_vert_max = std::max(other_projected_vert_value, other_projected_vert_max);
        }

        // test if ranges don't overlap
        if(self_projected_vert_max < other_projected_vert_min || other_projected_vert_max < self_projected_vert_min) {
            return false; // all it takes is one gap
        }
    }

    return true;
}

// "separating axis theory"
// https://gamedev.stackexchange.com/questions/25397/obb-vs-obb-collision-detection
bool BBoxObject::is_sphere_collide(TransformObject* self_transform_object,
                                   glm::vec3        other_abs_point,
                                   float            other_sphere_radius)
{
    // test if bbox radii touch
    glm::vec3 self_bbox_center = self_transform_object->in_abs_system((m_max + m_min) * 0.5f);
    float     self_bbox_radius = glm::distance(m_min, m_max) * 0.5f;
    if(glm::distance(self_bbox_center, other_abs_point) > self_bbox_radius + other_sphere_radius) {
        return false; // if not, no point in testing OOB collision
    }

    // =========================================
    // convert bbox corners to world coordinates
    // =========================================

    glm::vec3 self_abs_points[8];

    {
        glm::vec3 self_abs_bbox[2][2][2];

        {
            glm::mat4 self_transform = self_transform_object->get_transform();
            glm::vec3 self_local_bbox_extents[2];
            self_local_bbox_extents[0] = m_min;
            self_local_bbox_extents[1] = m_max;
            for(int i = 0; i < 2; i++) {
                for(int j = 0; j < 2; j++) {
                    for(int k = 0; k < 2; k++) {
                        self_abs_bbox[i][j][k] = glm::vec3(self_transform * glm::vec4(self_local_bbox_extents[i].x,
                                                                                      self_local_bbox_extents[j].y,
                                                                                      self_local_bbox_extents[k].z, 1));
                    }
                }
            }
        }

        glm::vec3 points[8];

        // points
        //
        //     y
        //     4-------7
        //    /|      /|
        //   / |     / |
        //  5-------6  |
        //  |  0----|--3 x
        //  | /     | /
        //  |/      |/
        //  1-------2
        // z

        vt::PrimitiveFactory::get_box_corners(points);

        for(int i = 0; i < 8; i++) {
            glm::ivec3 bbox_extents_indices(static_cast<int>(points[i].x),
                                            static_cast<int>(points[i].y),
                                            static_cast<int>(points[i].z));
            self_abs_points[i] = self_abs_bbox[bbox_extents_indices.x][bbox_extents_indices.y][bbox_extents_indices.z];
        }
    }

    // ==================================
    // generate potential separation axes
    // ==================================

    glm::vec3 principle_axes[3];

    // generate potential face-to-vertex collision axes
    for(int i = 0; i < 3; i++) {
        principle_axes[i] = self_transform_object->get_abs_direction(static_cast<euler_index_t>(i));
    }

    // =======================================
    // test bbox overlap along separation axes
    // =======================================

    // for each potential separation axis
    for(int i = 0; i < 3; i++) {
        glm::vec3 separation_axis = principle_axes[i];

        // calculate self bbox maximum extents along axis
        float self_projected_vert_min =  FLT_MAX;
        float self_projected_vert_max = -FLT_MAX;
        for(int j = 0; j < 8; j++) {
            float self_projected_vert_value = glm::dot(separation_axis, self_abs_points[j]);
            self_projected_vert_min = std::min(self_projected_vert_value, self_projected_vert_min);
            self_projected_vert_max = std::max(self_projected_vert_value, self_projected_vert_max);
        }

        // calculate other sphere maximum extents along axis
        float other_projected_vert_value = glm::dot(separation_axis, other_abs_point);
        float other_projected_vert_min   = other_projected_vert_value - other_sphere_radius;
        float other_projected_vert_max   = other_projected_vert_value + other_sphere_radius;

        // test if ranges don't overlap
        if(self_projected_vert_max < other_projected_vert_min || other_projected_vert_max < self_projected_vert_min) {
            return false; // all it takes is one gap
        }
    }

    return true;
}

static bool in_range(float x, float a, float b)
{
    return (a < x && x < b) ||
           (a > x && x > b);
}

static bool ranges_overlap(float a1, float a2, float b1, float b2)
{
    // cap open-ended range (ray origin must be between two hyperplanes of the box)
    if(a1 == BIG_NUMBER && a2 != BIG_NUMBER) { a1 = 0; }
    if(a1 != BIG_NUMBER && a2 == BIG_NUMBER) { a2 = 0; }
    if(b1 == BIG_NUMBER && b2 != BIG_NUMBER) { b1 = 0; }
    if(b1 != BIG_NUMBER && b2 == BIG_NUMBER) { b2 = 0; }

    // check if ranges overlap
    return in_range(a1, b1, b2) || in_range(a2, b1, b2);
}

// http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
bool BBoxObject::is_ray_intersect(TransformObject* self_transform_object,
                                  glm::vec3        ray_origin,
                                  glm::vec3        ray_dir,
                                  float*           alpha)
{
    // ==================================
    // generate potential separation axes
    // ==================================

    glm::vec3 range_axes[3];

    // generate potential face-to-vertex collision axes
    for(int i = 0; i < 3; i++) {
        range_axes[i] = self_transform_object->get_abs_direction(static_cast<euler_index_t>(i));
    }

    // ==========================================
    // test ray intersection with bbox's 6 planes
    // ==========================================

    float intersections[6];
    glm::vec3 plane_origin_min = self_transform_object->in_abs_system(m_min);
    glm::vec3 plane_origin_max = self_transform_object->in_abs_system(m_max);

    // for each potential separation axis
    float nearest_distance = BIG_NUMBER;
    int k = 0;
    for(int i = 0; i < 3; i++) {
        glm::vec3 plane_normal = range_axes[i];
        for(int j = 0; j < 2; j++) {
            glm::vec3 plane_origin = (j == 0 ? plane_origin_min : plane_origin_max);
            intersections[k] = ray_plane_intersection(plane_origin, plane_normal, ray_origin, ray_dir);
            glm::vec3 point = ray_origin + ray_dir * intersections[k];
            glm::vec3 local_point = glm::vec3(glm::inverse(self_transform_object->get_transform()) * glm::vec4(point, 1)); // expensive inverse call!
            if(is_within(local_point)) {
                if(intersections[k] < nearest_distance) {
                    nearest_distance = intersections[k];
                }
            }
            k++;
        }
    }

#if 0
    for(int i = 0; i < 6; i++) {
        std::cout << intersections[i] << std::endl;
    }
    std::cout << std::endl;
#endif

    // ray intersects box (need this in case ray is parallel to one of box's axes)
    if(nearest_distance != BIG_NUMBER) {
        if(alpha) {
            *alpha = nearest_distance;
        }
        return true;
    }

    // all three ranges must overlap (NOTE: fails if ray is parallel to one of box's axes)
    return ranges_overlap(intersections[0], intersections[1], intersections[2], intersections[3]) &&
           ranges_overlap(intersections[2], intersections[3], intersections[4], intersections[5]) &&
           ranges_overlap(intersections[4], intersections[5], intersections[0], intersections[1]);
}

bool BBoxObject::as_sphere_is_ray_intersect(TransformObject* self_transform_object,
                                            glm::vec3        ray_origin,
                                            glm::vec3        ray_dir)
{
    float radius = fabs(m_max.x - m_min.x) * 0.5f;
    return is_ray_sphere_intersection(self_transform_object->in_abs_system(),
                                      radius,
                                      ray_origin,
                                      ray_dir);
}

}
