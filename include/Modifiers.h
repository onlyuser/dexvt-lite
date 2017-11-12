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

#ifndef VT_MODIFIERS_H_
#define VT_MODIFIERS_H_

#include <glm/glm.hpp>

namespace vt {

class MeshBase;
class Scene;

enum tessellation_type_t {
    TESSELLATION_TYPE_EDGE_CENTER,
    TESSELLATION_TYPE_TRI_CENTER
};

void mesh_attach(Scene* scene, MeshBase* mesh1, MeshBase* mesh2);
void mesh_apply_ripple(MeshBase* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase, bool smooth);
void mesh_tessellate(MeshBase* mesh, tessellation_type_t tessellation_type, bool smooth);

}

#endif
