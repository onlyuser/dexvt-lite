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

#ifndef VT_PRIMITIVE_FACTORY_H_
#define VT_PRIMITIVE_FACTORY_H_

#include <glm/glm.hpp>
#include <string>

#define DEFAULT_SLICES 16
#define DEFAULT_STACKS 16

namespace vt {

class Mesh;
class MeshBase;

class PrimitiveFactory
{
public:
    static void get_box_corners(glm::vec3        (&points)[8],
                                const glm::vec3* origin = NULL,
                                const glm::vec3* dim    = NULL);
    static Mesh* create_grid(const std::string& name             = "",
                                   int          cols             = 1,
                                   int          rows             = 1,
                                   float        width            = 1,
                                   float        length           = 1,
                                   float        tex_width_scale  = 1,
                                   float        tex_length_scale = 1);
    static Mesh* create_sphere(const std::string& name = "",
                                     int          slices = DEFAULT_SLICES,
                                     int          stacks = DEFAULT_STACKS,
                                     float        radius = 1);
    static Mesh* create_hemisphere(const std::string& name   = "",
                                         int          slices = DEFAULT_SLICES,
                                         int          stacks = DEFAULT_STACKS,
                                         float        radius = 1);
    static Mesh* create_cylinder(const std::string& name   = "",
                                       int          slices = DEFAULT_SLICES,
                                       float        radius = 1,
                                       float        height = 1);
    static Mesh* create_cone(const std::string& name   = "",
                                   int          slices = DEFAULT_SLICES,
                                   float        radius = 1,
                                   float        height = 1);
    static Mesh* create_torus(const std::string& name         = "",
                                    int          slices       = DEFAULT_SLICES,
                                    int          stacks       = DEFAULT_STACKS,
                                    float        radius_major = 1,
                                    float        radius_minor = 0.5);
    static Mesh* create_box(const std::string& name   = "",
                                  float        width  = 1,
                                  float        height = 1,
                                  float        length = 1);
    static Mesh* create_tetrahedron(const std::string& name   = "",
                                          float        width  = 1,
                                          float        height = 1,
                                          float        length = 1);
    static Mesh* create_geosphere(const std::string& name               = "",
                                        float        radius             = 1,
                                        int          tessellation_iters = 3);
    static Mesh* create_diamond_brilliant_cut(const std::string& name,
                                                    float        radius                                     = 0.5,
                                                    float        table_radius                               = 0.25,
                                                    float        height                                     = 1,
                                                    float        crown_height_to_total_height_ratio         = 0.25,
                                                    float        upper_girdle_height_to_crown_height_ratio  = 0.75,
                                                    float        lower_girdle_depth_to_pavilion_depth_ratio = 0.5,
                                                    float        girdle_thick_part_thickness                = 0.05,
                                                    float        girdle_thin_part_thickness                 = 0.0125);
    static Mesh* create_viewport_quad(const std::string& name)
    {
        return create_grid(name, 1, 1);
    }
};

}

#endif
