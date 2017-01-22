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

#define SIGN(x) (!(x) ? 0 : (((x) > 0) ? 1 : -1))

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

glm::vec3 orient_to_offset(glm::vec3 orient,
                           glm::vec3* up_direction) // out
{
    if(up_direction) {
        glm::mat4 rotate_xform = GLM_ROTATE(glm::mat4(1), ORIENT_YAW(orient),   VEC_UP) *
                                 GLM_ROTATE(glm::mat4(1), ORIENT_PITCH(orient), VEC_LEFT) *
                                 GLM_ROTATE(glm::mat4(1), ORIENT_ROLL(orient),  VEC_FORWARD);
        *up_direction = glm::vec3(rotate_xform * glm::vec4(VEC_UP, 1));
        return glm::vec3(rotate_xform * glm::vec4(VEC_FORWARD, 1));
    }
    glm::mat4 rotate_xform_sans_roll = GLM_ROTATE(glm::mat4(1), ORIENT_YAW(orient),   VEC_UP) *
                                       GLM_ROTATE(glm::mat4(1), ORIENT_PITCH(orient), VEC_LEFT);
    return glm::vec3(rotate_xform_sans_roll * glm::vec4(VEC_FORWARD, 1));
}

glm::vec3 orient_to_offset(glm::vec3 orient)
{
    return orient_to_offset(orient, NULL);
}

glm::vec3 offset_to_orient(glm::vec3 offset,
                           glm::vec3* up_direction) // in
{
    glm::vec3 orient;

    // yaw
    if(static_cast<float>(fabs(offset.x)) < EPSILON && static_cast<float>(fabs(offset.z)) < EPSILON) {
        ORIENT_PITCH(orient) = 90;
        if(up_direction) {
            glm::vec3 flattened_offset = -glm::vec3(up_direction->x, 0, up_direction->z);
            ORIENT_YAW(orient) = glm::degrees(glm::angle(glm::normalize(flattened_offset), VEC_FORWARD));
            if(flattened_offset.x < 0) {
                ORIENT_YAW(orient) = -fabs(ORIENT_YAW(orient));
            }
        }
    } else {
        glm::vec3 flattened_offset = glm::normalize(glm::vec3(offset.x, 0, offset.z));
        ORIENT_PITCH(orient) = glm::degrees(glm::angle(flattened_offset, glm::normalize(offset))),
        ORIENT_YAW(orient)   = glm::degrees(glm::angle(flattened_offset, VEC_FORWARD));
        if(flattened_offset.x < 0) {
            ORIENT_YAW(orient) = -fabs(ORIENT_YAW(orient));
        }
    }

    // pitch
    if(offset.y > 0) {
        ORIENT_PITCH(orient) = -fabs(ORIENT_PITCH(orient));
    }

    // roll
    if(up_direction) {
        glm::mat4 rotate_xform_sans_roll = GLM_ROTATE(glm::mat4(1), ORIENT_YAW(orient),   VEC_UP) *
                                           GLM_ROTATE(glm::mat4(1), ORIENT_PITCH(orient), VEC_LEFT);
        glm::vec3 local_up_direction_roll_component = glm::vec3(glm::inverse(rotate_xform_sans_roll) *
                                                           glm::vec4(*up_direction, 1));
        ORIENT_ROLL(orient) = glm::degrees(glm::angle(glm::normalize(local_up_direction_roll_component), VEC_UP));
        if(local_up_direction_roll_component.x > 0) {
            ORIENT_ROLL(orient) = -fabs(ORIENT_ROLL(orient));
        }
    }

    return orient;
}

glm::vec3 offset_to_orient(glm::vec3 offset)
{
    return offset_to_orient(offset, NULL);
}

glm::vec3 orient_modulo(glm::vec3 orient)
{
    if(fabs(ORIENT_YAW(orient)) >= 180) {
        // yaw:  181 ==> -179
        // yaw: -181 ==>  179
        ORIENT_YAW(orient) = -SIGN(ORIENT_YAW(orient)) * (360 - fabs(ORIENT_YAW(orient)));
    }
    if(fabs(ORIENT_PITCH(orient)) >= 90) {
        // pitch:  91 ==>  89
        // pitch: -91 ==> -89
        ORIENT_PITCH(orient) = SIGN(ORIENT_PITCH(orient)) * (180 - fabs(ORIENT_PITCH(orient)));
        // yaw:  179 ==> -1
        // yaw: -179 ==>  1
        ORIENT_YAW(orient) = -SIGN(ORIENT_YAW(orient)) * (180 - fabs(ORIENT_YAW(orient)));
    }
    if(fabs(ORIENT_ROLL(orient)) >= 180) {
        // roll:  181 ==> -179
        // roll: -181 ==>  179
        ORIENT_ROLL(orient) = -SIGN(ORIENT_ROLL(orient)) * (360 - fabs(ORIENT_YAW(orient)));
    }
    return orient;
}

float angle_distance(float angle1, float angle2, float min_angle, float max_angle)
{
    float angle_range = max_angle - min_angle;
    float angle_diff = fabs(angle1 - angle2);
    if(angle_diff > angle_range * 0.5) {
        return angle_range - angle_diff;
    }
    return angle_diff;
}

glm::vec3 orient_limit(glm::vec3 orient, glm::ivec3 enable_orient_constraints, glm::vec3 orient_constraints_center, glm::vec3 orient_constraints_max_deviation)
{
    static glm::vec3 min_orient(-180, -90, -180);
    static glm::vec3 max_orient( 180,  90,  180);
    for(int i = 0; i < 3; i++) {
        if(!enable_orient_constraints[i]) {
            continue;
        }
        if(angle_distance(orient[i], orient_constraints_center[i], min_orient[i], max_orient[i]) > orient_constraints_max_deviation[i]) {
            float min_angle = orient_constraints_center[i] - orient_constraints_max_deviation[i];
            float max_angle = orient_constraints_center[i] + orient_constraints_max_deviation[i];
            float distance_to_lower_bound = angle_distance(orient[i], min_angle, min_orient[i], max_orient[i]);
            float distance_to_upper_bound = angle_distance(orient[i], max_angle, min_orient[i], max_orient[i]);
            if(distance_to_lower_bound < distance_to_upper_bound) {
                orient[i] = min_angle;
            } else {
                orient[i] = max_angle;
            }
        }
    }
    return orient;
}

glm::vec3 orient_diff(glm::vec3 orient, glm::vec3 orient_delta)
{
    return orient_modulo(glm::vec3(0,
                                   ORIENT_PITCH(orient) - ORIENT_PITCH(orient_delta),
                                   ORIENT_YAW(orient)   - ORIENT_YAW(orient_delta)));
}

glm::vec3 orient_sum(glm::vec3 orient, glm::vec3 orient_delta)
{
    return orient_modulo(glm::vec3(0,
                                   ORIENT_PITCH(orient) + ORIENT_PITCH(orient_delta),
                                   ORIENT_YAW(orient)   + ORIENT_YAW(orient_delta)));
}

void mesh_apply_ripple(Mesh* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase)
{
    for(int i = 0; i < static_cast<int>(mesh->get_num_vertex()); i++) {
        glm::vec3 pos = mesh->get_vert_coord(i);
        glm::vec3 new_pos = pos;
        new_pos.y = origin.y + static_cast<float>(sin(glm::distance(glm::vec2(origin.x, origin.z),
                               glm::vec2(pos.x, pos.z)) / (wavelength / (PI * 2)) + phase)) * amplitude;
        mesh->set_vert_coord(i, new_pos);
    }
    mesh->update_normals_and_tangents();
    mesh->update_bbox();
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
    size_t _start_pos(start_pos ? *start_pos : 0);
    if(_start_pos >= s.length()) {
        return false;
    }
    std::string rest = s.substr(_start_pos, s.length()-_start_pos);
    regex_t preg;
    if(regcomp(&preg, pattern.c_str(), REG_ICASE|REG_EXTENDED)) {
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
        *(cap_groups[i]) = rest.substr(pmatch[i].rm_so, pmatch[i].rm_eo-pmatch[i].rm_so);
    }
    if(start_pos) {
        *start_pos = _start_pos+pmatch[0].rm_so;
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
