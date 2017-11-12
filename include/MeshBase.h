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

#ifndef VT_MESH_BASE_H_
#define VT_MESH_BASE_H_

#include <BBoxObject.h>
#include <glm/glm.hpp>

namespace vt {

class Material;

class MeshBase
{
public:
    virtual ~MeshBase() {}
    virtual bool       is_visible() const = 0;
    virtual void       set_visible(bool visible) = 0;
    virtual bool       is_smooth() const = 0;
    virtual void       set_smooth(bool smooth) = 0;
    virtual void       resize(size_t num_vertex, size_t num_tri, bool preserve_mesh_geometry = false) = 0;
    virtual void       merge(const MeshBase* other, bool copy_tex_coords = false) = 0;
    virtual Material*  get_material() const = 0;
    virtual size_t     get_num_vertex() const = 0;
    virtual size_t     get_num_tri() const = 0;
    virtual glm::vec3  get_vert_coord(int index) const = 0;
    virtual void       set_vert_coord(int index, glm::vec3 coord) = 0;
    virtual glm::vec3  get_vert_normal(int index) const = 0;
    virtual void       set_vert_normal(int index, glm::vec3 normal) = 0;
    virtual glm::vec3  get_vert_tangent(int index) const = 0;
    virtual void       set_vert_tangent(int index, glm::vec3 tangent) = 0;
    virtual glm::vec2  get_tex_coord(int index) const = 0;
    virtual void       set_tex_coord(int index, glm::vec2 coord) = 0;
    virtual glm::ivec3 get_tri_indices(int index) const = 0;
    virtual void       set_tri_indices(int index, glm::ivec3 indices) = 0;
    virtual void       update_bbox() = 0;
    virtual void       update_normals_and_tangents() = 0;
    virtual void       get_min_max(glm::vec3* min, glm::vec3* max) const = 0;
    virtual glm::vec3  in_abs_system(glm::vec3 local_point = glm::vec3(0)) = 0;
    virtual void       set_axis(glm::vec3 axis) = 0;
    virtual void       center_axis(BBoxObject::align_t align = BBoxObject::ALIGN_CENTER) = 0;
};

}

#endif
