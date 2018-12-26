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

#ifndef VT_UTIL_H_
#define VT_UTIL_H_

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>

#define EPSILON    0.0001f
#define BIG_NUMBER 10000

#define SIGN(x)              (!(x) ? 0 : (((x) > 0) ? 1 : -1))
#define MIX(a, b, alpha)     ((a) + ((b) - (a)) * (alpha))
#define CLAMP(x, _min, _max) std::min(std::max((x), (_min)), (_max))

#define MAKEWORD(lo, hi) ((uint16_t)(((uint8_t)(lo)) | (((uint16_t)((uint8_t)(hi))) << 8)))
#define HIBYTE(x)        (((uint16_t)(x)) >> 8)
#define LOBYTE(x)        ((((uint16_t)(x)) << 8) >> 8)

#define MAKELONG(lo, hi) ((uint32_t)(((uint16_t)(lo)) | (((uint32_t)((uint16_t)(hi))) << 16)))
#define HIWORD(x)        (((uint32_t)(x)) >> 16)
#define LOWORD(x)        ((((uint32_t)(x)) << 16) >> 16)

#ifdef NO_GLM_CONSTANTS
    #warning "Disabling glm header <glm/gtx/constants.hpp>"
    #define PI      3.1415926
    #define HALF_PI (PI*0.5)
#else
    #include <glm/gtc/constants.hpp>
    #define PI      glm::pi<float>()
    #define HALF_PI glm::half_pi<float>()
#endif

#if GLM_VERSION >= 96
    // glm::rotate changed from degrees to radians in GLM 0.9.6
    // https://glm.g-truc.net/0.9.6/updates.html
    #define GLM_ROTATION_TRANSFORM(m, a, v)     glm::rotate((m), glm::radians(a), (v))
    #define GLM_EULER_TRANSFORM(y, p, r)        glm::eulerAngleYXZ(glm::radians(y), glm::radians(p), glm::radians(r))
    #define GLM_EULER_TRANSFORM_SANS_ROLL(y, p) glm::eulerAngleYX(glm::radians(y), glm::radians(p))
#else
    #define GLM_ROTATION_TRANSFORM(m, a, v)     glm::rotate((m), (a), (v))
    #define GLM_EULER_TRANSFORM(y, p, r)        glm::eulerAngleYXZ((y), (p), (r))
    #define GLM_EULER_TRANSFORM_SANS_ROLL(y, p) glm::eulerAngleYX((y), (p))
#endif

#define EULER_ROLL(v)  v[vt::EULER_INDEX_ROLL]
#define EULER_PITCH(v) v[vt::EULER_INDEX_PITCH]
#define EULER_YAW(v)   v[vt::EULER_INDEX_YAW]

#define VEC_LEFT    glm::vec3(1, 0, 0)
#define VEC_UP      glm::vec3(0, 1, 0)
#define VEC_FORWARD glm::vec3(0, 0, 1)

namespace vt {

enum euler_index_t {
    EULER_INDEX_UNDEF = -1,
    EULER_INDEX_ROLL,
    EULER_INDEX_PITCH,
    EULER_INDEX_YAW
};

template<class T>
T safe_normalize(T v)
{
#if 1
    float l = glm::length(v);
    if(l < EPSILON) {
        return T(1);
    }
    return v * (1 / l);
#else
    return glm::normalize(v);
#endif
}

void print_bitmap_string(void* font, const char* s);
glm::vec3 euler_to_offset(glm::vec3  euler,
                          glm::vec3* up_direction = NULL); // out
glm::vec3 offset_to_euler(glm::vec3  offset,
                          glm::vec3* up_direction = NULL); // in
glm::vec3 as_offset_in_other_system(glm::vec3 euler, glm::mat4 transform, bool as_up_direction = false);
glm::vec3 dir_from_point_as_offset_in_other_system(glm::vec3 euler, glm::mat4 transform, glm::vec3 point, bool as_up_direction = false);
glm::vec3 euler_modulo(glm::vec3 euler);
float angle_modulo(float angle);
float angle_distance(float angle1, float angle2);
glm::vec3 nearest_point_on_plane(glm::vec3 plane_origin, glm::vec3 plane_normal, glm::vec3 point);
glm::vec3 projection_onto(glm::vec3 a, glm::vec3 b);
glm::vec3 rejection_from(glm::vec3 a, glm::vec3 b);
float ray_sphere_intersection(glm::vec3  sphere_origin,
                              float      sphere_radius,
                              glm::vec3  ray_origin,
                              glm::vec3  ray_dir,
                              glm::vec3* surface_normal,
                              bool*      ray_starts_inside_sphere);
float ray_plane_intersection(glm::vec3 plane_point,
                             glm::vec3 plane_normal,
                             glm::vec3 ray_origin,
                             glm::vec3 ray_dir);
glm::vec3 get_absolute_direction(int euler_index);
bool is_within(glm::vec3 pos, glm::vec3 _min, glm::vec3 _max);
float ray_box_intersect(glm::mat4  box_transform,
                        glm::mat4  box_inverse_transform,
                        glm::vec3  box_min,
                        glm::vec3  box_max,
                        glm::vec3  ray_origin,
                        glm::vec3  ray_dir,
                        glm::vec3* surface_point,
                        glm::vec3* surface_normal);
glm::vec3 get_random_offset();
float ray_sphere_next_ray(glm::vec3  ray_origin,
                          glm::vec3  ray_dir,
                          float      dist,
                          glm::vec3  surface_normal,
                          bool       ray_starts_inside_sphere,
                          float      sphere_eta,
                          float      sphere_diffuse_fuzz,
                          glm::vec3* next_ray);
float ray_plane_next_ray(glm::vec3  ray_origin,
                         glm::vec3  ray_dir,
                         float      dist,
                         glm::vec3  plane_point,
                         glm::vec3  plane_normal,
                         float      plane_eta,
                         float      plane_diffuse_fuzz,
                         glm::vec3* next_ray);
glm::vec3 bezier_interpolate(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, float alpha);
bool read_file(std::string filename, std::string &s);
bool regexp(const std::string &s, const std::string& pattern, std::vector<std::string*> &cap_groups, size_t* start_pos);
bool regexp(const std::string &s, const std::string& pattern, std::vector<std::string*> &cap_groups);
bool regexp(const std::string &s, const std::string& pattern, int nmatch, ...);

}

#endif
