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

glm::vec3 euler_to_offset(glm::vec3 euler)
{
    return euler_to_offset(euler, NULL);
}

glm::vec3 offset_to_euler(glm::vec3  offset,
                          glm::vec3* up_direction) // in
{
    glm::vec3 euler, flattened_offset;

    // pitch
    if(static_cast<float>(fabs(offset.x)) > EPSILON || static_cast<float>(fabs(offset.z)) > EPSILON) {
        // non-vertical
        flattened_offset   = glm::normalize(glm::vec3(offset.x, 0, offset.z));
        EULER_PITCH(euler) = glm::degrees(glm::angle(flattened_offset, glm::normalize(offset)));
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
        EULER_ROLL(euler)                           = glm::degrees(glm::angle(glm::normalize(local_up_direction_roll_component), VEC_UP));
        if(local_up_direction_roll_component.x > 0) {
            EULER_ROLL(euler) = -fabs(EULER_ROLL(euler));
        }
    }

    return euler;
}

glm::vec3 offset_to_euler(glm::vec3 offset)
{
    return offset_to_euler(offset, NULL);
}

glm::vec3 as_offset_in_other_system(glm::vec3 euler, glm::mat4 transform)
{
    return glm::vec3(transform * glm::vec4(euler_to_offset(euler), 1));
}

glm::vec3 dir_from_point_as_offset_in_other_system(glm::vec3 euler, glm::mat4 transform, glm::vec3 point)
{
    return glm::normalize(as_offset_in_other_system(euler, transform) - point);
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

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
float ray_plane_intersection(glm::vec3 plane_origin, glm::vec3 plane_normal, glm::vec3 ray_origin, glm::vec3 ray_dir)
{
    float denom = glm::dot(ray_dir, plane_normal);
    if(fabs(denom) < EPSILON) {
        return BIG_NUMBER;
    }
    float result = (glm::dot(plane_origin - ray_origin, plane_normal)) / denom;
    if(result < 0) {
        return BIG_NUMBER;
    }
    return result;
}

// https://en.wikipedia.org/wiki/Vector_projection
glm::vec3 projection_onto(glm::vec3 a, glm::vec3 b)
{
    glm::vec3 norm_b = glm::normalize(b);
    return norm_b * glm::dot(a, norm_b);
}

// https://en.wikipedia.org/wiki/Vector_projection
glm::vec3 rejection_from(glm::vec3 a, glm::vec3 b)
{
    return a - projection_onto(a, b);
}

bool is_ray_sphere_intersection(glm::vec3 sphere_origin, float sphere_radius, glm::vec3 ray_origin, glm::vec3 ray_dir)
{
    glm::vec3 ray_origin_to_sphere_origin = sphere_origin - ray_origin;
    glm::vec3 ray_nearest_point_to_sphere = ray_origin + projection_onto(ray_origin_to_sphere_origin, ray_dir);
    return glm::length(ray_nearest_point_to_sphere - sphere_origin) < sphere_radius && glm::angle(glm::normalize(ray_origin_to_sphere_origin), ray_dir) < HALF_PI;
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

bool regexp(std::string &s, std::string pattern, std::vector<std::string*> &cap_groups, size_t* start_pos)
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

bool regexp(std::string &s, std::string pattern, std::vector<std::string*> &cap_groups)
{
    return regexp(s, pattern, cap_groups, NULL);
}

bool regexp(std::string &s, std::string pattern, int nmatch, ...)
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
