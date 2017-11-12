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

#ifndef VT_BBOX_OBJECT_H_
#define VT_BBOX_OBJECT_H_

#include <glm/glm.hpp>

namespace vt {

class TransformObject;

class BBoxObject
{
public:
    enum align_t {
        ALIGN_CENTER,
        ALIGN_X_MIN,
        ALIGN_X_MAX,
        ALIGN_Y_MIN,
        ALIGN_Y_MAX,
        ALIGN_Z_MIN,
        ALIGN_Z_MAX
    };

    BBoxObject();
    BBoxObject(glm::vec3 min, glm::vec3 max);
    void set_min_max(glm::vec3 min, glm::vec3 max);
    void get_min_max(glm::vec3* min, glm::vec3* max) const;
    glm::vec3 get_center(align_t align = ALIGN_CENTER) const;
    bool is_within(glm::vec3 pos) const;
    glm::vec3 limit(glm::vec3 pos) const;
    glm::vec3 wrap(glm::vec3 pos) const;
    bool is_bbox_collide(TransformObject* self_transform_object,
                         TransformObject* other_transform_object,
                         BBoxObject*      other_bbox_object);
    bool is_sphere_collide(TransformObject* self_transform_object,
                           glm::vec3        other_abs_point,
                           float            other_sphere_radius);
    bool is_ray_intersect(TransformObject* self_transform_object,
                          glm::vec3        ray_origin,
                          glm::vec3        ray_dir,
                          float*           alpha);
    bool as_sphere_is_ray_intersect(TransformObject* self_transform_object,
                                    glm::vec3        ray_origin,
                                    glm::vec3        ray_dir);

protected:
    glm::vec3 m_min;
    glm::vec3 m_max;
};

}

#endif
