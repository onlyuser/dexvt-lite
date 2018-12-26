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

#include <Util.h>
#include <Mesh.h>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <regex.h>
#include <math.h>
#include <stdarg.h>
#include <GL/glut.h>
#include <png.h>
#include <memory.h>
#include <random>

#define EPSILON2 (EPSILON * 0.9) // factor >= 1 causes artifacts

namespace vt {

void print_bitmap_string(void* font, const char* s)
{
    if(s && *s != '\0') {
        while(*s) {
            glutBitmapCharacter(font, *s);
            s++;
        }
    }
}

glm::vec3 euler_to_offset(glm::vec3  euler,
                          glm::vec3* up_direction) // out
{
    if(up_direction) {
        glm::mat4 euler_transform = GLM_EULER_TRANSFORM(EULER_YAW(euler), EULER_PITCH(euler), EULER_ROLL(euler));
        *up_direction = glm::vec3(euler_transform * glm::vec4(VEC_UP, 1));
        return glm::vec3(euler_transform * glm::vec4(VEC_FORWARD, 1));
    }
    glm::mat4 euler_transform_sans_roll = GLM_EULER_TRANSFORM_SANS_ROLL(EULER_YAW(euler), EULER_PITCH(euler));
    return glm::vec3(euler_transform_sans_roll * glm::vec4(VEC_FORWARD, 1));
}

glm::vec3 offset_to_euler(glm::vec3  offset,
                          glm::vec3* up_direction) // in
{
    glm::vec3 euler, flattened_offset;

    // pitch
    if(static_cast<float>(fabs(offset.x)) > EPSILON || static_cast<float>(fabs(offset.z)) > EPSILON) {
        // non-vertical
        flattened_offset   = safe_normalize(glm::vec3(offset.x, 0, offset.z));
        EULER_PITCH(euler) = glm::degrees(glm::angle(flattened_offset, safe_normalize(offset)));
    } else {
        // vertical
        if(up_direction) {
            flattened_offset = glm::vec3(up_direction->x, 0, up_direction->z);
            if(SIGN(offset.y) == SIGN(up_direction->y)) {
                flattened_offset = -flattened_offset;
            }
        }
        EULER_PITCH(euler) = 90;
    }
    if(offset.y > 0) {
        EULER_PITCH(euler) = -fabs(EULER_PITCH(euler));
    }

    // yaw
    EULER_YAW(euler) = glm::degrees(glm::angle(flattened_offset, VEC_FORWARD));
    if(flattened_offset.x < 0) {
        EULER_YAW(euler) = -fabs(EULER_YAW(euler));
    }

    // roll
    if(up_direction) {
        glm::mat4 euler_transform_sans_roll         = GLM_EULER_TRANSFORM_SANS_ROLL(EULER_YAW(euler), EULER_PITCH(euler));
        glm::vec3 local_up_direction_roll_component = glm::vec3(glm::inverse(euler_transform_sans_roll) * glm::vec4(*up_direction, 1));
        EULER_ROLL(euler)                           = glm::degrees(glm::angle(safe_normalize(local_up_direction_roll_component), VEC_UP));
        if(local_up_direction_roll_component.x > 0) {
            EULER_ROLL(euler) = -fabs(EULER_ROLL(euler));
        }
    }

    return euler;
}

glm::vec3 as_offset_in_other_system(glm::vec3 euler, glm::mat4 transform, bool as_up_direction)
{
    glm::vec3 offset;
    if(as_up_direction) {
        euler_to_offset(euler, &offset);
    } else {
        offset = euler_to_offset(euler);
    }
    return glm::vec3(transform * glm::vec4(offset, 1));
}

glm::vec3 dir_from_point_as_offset_in_other_system(glm::vec3 euler, glm::mat4 transform, glm::vec3 point, bool as_up_direction)
{
    return safe_normalize(as_offset_in_other_system(euler, transform, as_up_direction) - point);
}

glm::vec3 euler_modulo(glm::vec3 euler)
{
    if(fabs(EULER_YAW(euler)) > 180) {
        // yaw:  181 ==> -179
        // yaw: -181 ==>  179
        EULER_YAW(euler) = -SIGN(EULER_YAW(euler)) * (360 - fabs(EULER_YAW(euler)));
    }
    if(fabs(EULER_PITCH(euler)) > 90) {
        // pitch:  91 ==>  89
        // pitch: -91 ==> -89
        EULER_PITCH(euler) = SIGN(EULER_PITCH(euler)) * (180 - fabs(EULER_PITCH(euler)));
        // yaw:  179 ==> -1
        // yaw: -179 ==>  1
        EULER_YAW(euler) = -SIGN(EULER_YAW(euler)) * (180 - fabs(EULER_YAW(euler)));
    }
    if(fabs(EULER_ROLL(euler)) > 180) {
        // roll:  181 ==> -179
        // roll: -181 ==>  179
        EULER_ROLL(euler) = -SIGN(EULER_ROLL(euler)) * (360 - fabs(EULER_YAW(euler)));
    }
    return euler;
}

float angle_modulo(float angle)
{
    while(angle >= 360) {
        angle -= 360;
    }
    while(angle < 0) {
        angle += 360;
    }
    return angle;
}

float angle_distance(float angle1, float angle2)
{
    float angle_diff = fabs(angle_modulo(angle1) - angle_modulo(angle2));
    if(angle_diff > 180) {
        return 360 - angle_diff;
    }
    return angle_diff;
}

glm::vec3 nearest_point_on_plane(glm::vec3 plane_origin, glm::vec3 plane_normal, glm::vec3 point)
{
    return point - plane_normal * (glm::dot(point, plane_normal) - glm::dot(plane_origin, plane_normal));
}

// https://en.wikipedia.org/wiki/Vector_projection
glm::vec3 projection_onto(glm::vec3 a, glm::vec3 b)
{
    glm::vec3 norm_b = safe_normalize(b);
    return norm_b * glm::dot(a, norm_b);
}

// https://en.wikipedia.org/wiki/Vector_projection
glm::vec3 rejection_from(glm::vec3 a, glm::vec3 b)
{
    return a - projection_onto(a, b);
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
// https://www.cs.uaf.edu/2012/spring/cs481/section/0/lecture/02_14_refraction.html
float ray_sphere_intersection(glm::vec3  sphere_origin,
                              float      sphere_radius,
                              glm::vec3  ray_origin,
                              glm::vec3  ray_dir,
                              glm::vec3* surface_normal,
                              bool*      ray_starts_inside_sphere)
{
    glm::vec3 ray_nearest_point_to_sphere        = ray_origin + projection_onto(sphere_origin - ray_origin, ray_dir);
    glm::vec3 sphere_origin_to_ray_nearest_point = ray_nearest_point_to_sphere - sphere_origin;
    float     dist_to_ray_from_sphere_squared    = glm::dot(sphere_origin_to_ray_nearest_point, sphere_origin_to_ray_nearest_point);
    float     sphere_radius_squared              = sphere_radius * sphere_radius;
    if(dist_to_ray_from_sphere_squared > sphere_radius_squared) { // ray entirely misses sphere
        return BIG_NUMBER;
    }
    float half_length_inside_sphere = pow(sphere_radius_squared - dist_to_ray_from_sphere_squared, 0.5);
    glm::vec3 surface_point;
    bool _ray_starts_inside_sphere = false;
    if(fabs(glm::dot(ray_nearest_point_to_sphere, ray_dir) - glm::dot(ray_origin, ray_dir)) < half_length_inside_sphere) { // ray starts inside sphere
        surface_point = ray_nearest_point_to_sphere + ray_dir * half_length_inside_sphere;
        _ray_starts_inside_sphere = true;
    } else { // ray starts outside sphere
        if(glm::dot(sphere_origin, ray_dir) < glm::dot(ray_origin, ray_dir)) { // ray points away from sphere
            return BIG_NUMBER;
        }
        surface_point = ray_nearest_point_to_sphere - ray_dir * half_length_inside_sphere;
    }
    if(surface_normal) {
        *surface_normal = glm::normalize(surface_point - sphere_origin);
    }
    if(ray_starts_inside_sphere) {
        *ray_starts_inside_sphere = _ray_starts_inside_sphere;
    }
    return glm::distance(ray_origin, surface_point);
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
// https://www.cs.uaf.edu/2012/spring/cs481/section/0/lecture/02_14_refraction.html
float ray_plane_intersection(glm::vec3 plane_point,
                             glm::vec3 plane_normal,
                             glm::vec3 ray_origin,
                             glm::vec3 ray_dir)
{
    float denom = glm::dot(ray_dir, plane_normal);
    if(fabs(denom) < EPSILON /*|| denom > 0*/) { // zero denominator (undefined) or ray points at plane backside (implies ray exiting plane)
        return BIG_NUMBER;
    }
    float dist = glm::dot(plane_point - ray_origin, plane_normal) / denom;
    if(dist < 0) { // ray points away from plane
        return BIG_NUMBER;
    }
    return dist;
}

glm::vec3 get_absolute_direction(int euler_index)
{
    switch(euler_index) {
        case 0: return glm::vec3(glm::vec4(VEC_FORWARD, 1));
        case 1: return glm::vec3(glm::vec4(VEC_LEFT, 1));
        case 2: return glm::vec3(glm::vec4(VEC_UP, 1));
    }
    return glm::vec3(0);
}

bool is_within(glm::vec3 pos, glm::vec3 _min, glm::vec3 _max)
{
    glm::vec3 __min = _min - glm::vec3(EPSILON);
    glm::vec3 __max = _max + glm::vec3(EPSILON);
    return pos.x > __min.x && pos.y > __min.y && pos.z > __min.z &&
           pos.x < __max.x && pos.y < __max.y && pos.z < __max.z;
}

// http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
float ray_box_intersect(glm::mat4  box_transform,
                        glm::mat4  box_inverse_transform,
                        glm::vec3  box_min,
                        glm::vec3  box_max,
                        glm::vec3  ray_origin,
                        glm::vec3  ray_dir,
                        glm::vec3* surface_point,
                        glm::vec3* surface_normal)
{
    // for each potential separation axis
    float _dist = BIG_NUMBER;
    glm::vec3 _surface_point;
    glm::vec3 _surface_normal;
    glm::mat4 self_normal_transform = glm::transpose(box_inverse_transform);
    glm::vec3 plane_point_min       = glm::vec3(box_transform * glm::vec4(box_min, 1));
    glm::vec3 plane_point_max       = glm::vec3(box_transform * glm::vec4(box_max, 1));
    for(int i = 0; i < 3; i++) { // principle directions
        glm::vec3 absolute_direction = glm::vec3(self_normal_transform * glm::vec4(get_absolute_direction(i), 1));
        for(int j = 0; j < 2; j++) { // min side, max side
            glm::vec3 plane_point;
            glm::vec3 plane_normal;
            if(j == 0) {
                plane_point  =  plane_point_min;
                plane_normal = -absolute_direction;
            } else {
                plane_point  = plane_point_max;
                plane_normal = absolute_direction;
            }
            float dist = ray_plane_intersection(plane_point,
                                                plane_normal,
                                                ray_origin,
                                                ray_dir);
            if(dist == BIG_NUMBER) { // ray hits nothing
                continue;
            }
            glm::vec3 point = ray_origin + ray_dir * dist;
            glm::vec3 local_point = glm::vec3(box_inverse_transform * glm::vec4(point, 1));
            if(!is_within(local_point, box_min, box_max)) {
                continue;
            }
            if(dist < _dist) {
                _dist           = dist;
                _surface_point  = plane_point;
                _surface_normal = plane_normal;
            }
        }
    }
    if(_dist == BIG_NUMBER) { // ray hits nothing
        return BIG_NUMBER;
    }
    if(surface_point) {
        *surface_point = _surface_point;
    }
    if(surface_normal) {
        *surface_normal = _surface_normal;
    }
    return _dist;
}

glm::vec3 get_random_offset()
{
    return glm::vec3(rand() / RAND_MAX,
                     rand() / RAND_MAX,
                     rand() / RAND_MAX);
}

float ray_sphere_next_ray(glm::vec3  ray_origin,
                          glm::vec3  ray_dir,
                          float      dist,
                          glm::vec3  surface_normal,
                          bool       ray_starts_inside_sphere,
                          float      sphere_eta,
                          float      sphere_diffuse_fuzz,
                          glm::vec3* next_ray)
{
    bool total_internal_reflection = false;
    glm::vec3 _next_ray;
    if(fabs(sphere_eta) == BIG_NUMBER) { // opaque material
        _next_ray = glm::reflect(ray_dir, surface_normal);
    } else { // transparent material
        if(ray_starts_inside_sphere) { // ray starts inside sphere
            _next_ray = glm::refract(ray_dir, -surface_normal, 1.0f / sphere_eta);
            if(glm::length(_next_ray) == 0) { // refract fails -- total internal reflection
                _next_ray = glm::reflect(ray_dir, -surface_normal);
                total_internal_reflection = true;
            }
        } else { // ray starts outside sphere
            _next_ray = glm::refract(ray_dir, surface_normal, sphere_eta);
        }
    }
    if(next_ray) {
        if(sphere_eta == -BIG_NUMBER) { // diffuse material
            *next_ray = glm::normalize(_next_ray + get_random_offset() * sphere_diffuse_fuzz);
        } else {
            *next_ray = _next_ray;
        }
    }
    if(fabs(sphere_eta) == BIG_NUMBER) { // opaque material
        return dist;                     // no need for offset since we prevent self-intersection for opaque materials in ray_world_intersect
    }
    if(total_internal_reflection) { // transparent material -- offset to prevent self-intersection
        return dist - EPSILON;
    }
    return dist + EPSILON2; // transparent material -- offset to prevent self-intersection
}

float ray_plane_next_ray(glm::vec3  ray_origin,
                         glm::vec3  ray_dir,
                         float      dist,
                         glm::vec3  plane_point,
                         glm::vec3  plane_normal,
                         float      plane_eta,
                         float      plane_diffuse_fuzz,
                         glm::vec3* next_ray)
{
    if(dist == BIG_NUMBER) { // ray hits nothing
        return dist;
    }
    bool total_internal_reflection = false;
    glm::vec3 _next_ray;
    if(fabs(plane_eta) == BIG_NUMBER) { // opaque material
        _next_ray = glm::reflect(ray_dir, plane_normal);
    } else { // transparent material
        if(glm::dot(ray_origin, plane_normal) < glm::dot(plane_point, plane_normal)) { // ray starts inside plane
            _next_ray = glm::refract(ray_dir, -plane_normal, 1.0f / plane_eta);
            if(glm::length(_next_ray) == 0) { // refract fails -- total internal reflection
                _next_ray = glm::reflect(ray_dir, -plane_normal);
                total_internal_reflection = true;
            }
        } else { // ray starts outside plane
            _next_ray = glm::refract(ray_dir, plane_normal, plane_eta);
        }
    }
    if(next_ray) {
        if(plane_eta == -BIG_NUMBER) { // diffuse material
            *next_ray = glm::normalize(_next_ray + get_random_offset() * plane_diffuse_fuzz);
        } else {
            *next_ray = _next_ray;
        }
    }
    if(fabs(plane_eta) == BIG_NUMBER) { // opaque material
        return dist;                    // no need for offset since we prevent self-intersection for opaque materials in ray_world_intersect
    }
    if(total_internal_reflection) { // transparent material -- offset to prevent self-intersection
        return dist - EPSILON;
    }
    return dist + EPSILON2; // transparent material -- offset to prevent self-intersection
}

// https://en.wikipedia.org/wiki/Bernstein_polynomial
glm::vec3 bezier_interpolate(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, float alpha)
{
    float w1 = pow(1 - alpha, 3);
    float w2 = 3 * alpha * pow(1 - alpha, 2);
    float w3 = 3 * pow(alpha, 2) * (1 - alpha);
    float w4 = pow(alpha, 3);
    return (p1 * w1) + (p2 * w2) + (p3 * w3) + (p4 * w4);
}

bool read_file(std::string filename, std::string &s)
{
    FILE* file = fopen(filename.c_str(), "rb");
    if(!file) {
        std::cerr << "cannot open file" << std::endl;
        return false;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);
    if(!length) {
        std::cerr << "file empty" << std::endl;
        fclose(file);
        return false;
    }
    char* buffer = new char[length+1];
    if(!buffer) {
        std::cerr << "not enough memory" << std::endl;
        fclose(file);
        return false;
    }
    buffer[length] = '\0';
    fread(buffer, 1, length, file);
    fclose(file);
    s = buffer;
    delete[] buffer;
    return true;
}

bool regexp(const std::string& s, const std::string& pattern, std::vector<std::string*> &cap_groups, size_t* start_pos)
{
    int nmatch = cap_groups.size();
    if(!nmatch) {
        return false;
    }
    size_t _start_pos(start_pos ? (*start_pos) : 0);
    if(_start_pos >= s.length()) {
        return false;
    }
    std::string rest = s.substr(_start_pos, s.length() - _start_pos);
    regex_t preg;
    if(regcomp(&preg, pattern.c_str(), REG_ICASE | REG_EXTENDED)) {
        return false;
    }
    regmatch_t* pmatch = new regmatch_t[nmatch];
    if(!pmatch) {
        return false;
    }
    if(regexec(&preg, rest.c_str(), nmatch, pmatch, 0)) {
        delete[] pmatch;
        regfree(&preg);
        return false;
    }
    regfree(&preg);
    for(int i = 0; i < nmatch; i++) {
        if(!cap_groups[i]) {
            continue;
        }
        *(cap_groups[i]) = rest.substr(pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so);
    }
    if(start_pos) {
        *start_pos = _start_pos + pmatch[0].rm_so;
    }
    delete[] pmatch;
    return true;
}

bool regexp(const std::string& s, const std::string& pattern, std::vector<std::string*> &cap_groups)
{
    return regexp(s, pattern, cap_groups, NULL);
}

bool regexp(const std::string& s, const std::string& pattern, int nmatch, ...)
{
    if(!nmatch) {
        return false;
    }
    std::vector<std::string*> args(nmatch);
    va_list ap;
    va_start(ap, nmatch);
    for(int i = 0; i < nmatch; i++) {
        args[i] = va_arg(ap, std::string*);
    }
    va_end(ap);
    return regexp(s, pattern, args);
}

}
