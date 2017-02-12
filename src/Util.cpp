#include <Util.h>
#include <Mesh.h>
#include <MeshIFace.h>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <regex.h>
#include <math.h>
#include <stdarg.h>
#include <GL/glut.h>

#define SIGN(x) (!(x) ? 0 : (((x) > 0) ? 1 : -1))
#define MAKELONG(a, b) ((uint32_t)(((uint16_t)(a)) | (((uint32_t)((uint16_t)(b))) << 16)))

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

glm::vec3 orient_to_offset(glm::vec3  orient,
                           glm::vec3* up_direction) // out
{
    if(up_direction) {
        glm::mat4 orient_xform = GLM_EULER_ANGLE(ORIENT_YAW(orient), ORIENT_PITCH(orient), ORIENT_ROLL(orient));
        *up_direction = glm::vec3(orient_xform * glm::vec4(VEC_UP, 1));
        return glm::vec3(orient_xform * glm::vec4(VEC_FORWARD, 1));
    }
    glm::mat4 orient_xform_sans_roll = GLM_EULER_ANGLE_SANS_ROLL(ORIENT_YAW(orient), ORIENT_PITCH(orient));
    return glm::vec3(orient_xform_sans_roll * glm::vec4(VEC_FORWARD, 1));
}

glm::vec3 orient_to_offset(glm::vec3 orient)
{
    return orient_to_offset(orient, NULL);
}

glm::vec3 offset_to_orient(glm::vec3  offset,
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
        glm::mat4 orient_xform_sans_roll = GLM_EULER_ANGLE_SANS_ROLL(ORIENT_YAW(orient), ORIENT_PITCH(orient));
        glm::vec3 local_up_direction_roll_component =
                glm::vec3(glm::inverse(orient_xform_sans_roll) * glm::vec4(*up_direction, 1));
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
    if(fabs(ORIENT_YAW(orient)) > 180) {
        // yaw:  181 ==> -179
        // yaw: -181 ==>  179
        ORIENT_YAW(orient) = -SIGN(ORIENT_YAW(orient)) * (360 - fabs(ORIENT_YAW(orient)));
    }
    if(fabs(ORIENT_PITCH(orient)) > 90) {
        // pitch:  91 ==>  89
        // pitch: -91 ==> -89
        ORIENT_PITCH(orient) = SIGN(ORIENT_PITCH(orient)) * (180 - fabs(ORIENT_PITCH(orient)));
        // yaw:  179 ==> -1
        // yaw: -179 ==>  1
        ORIENT_YAW(orient) = -SIGN(ORIENT_YAW(orient)) * (180 - fabs(ORIENT_YAW(orient)));
    }
    if(fabs(ORIENT_ROLL(orient)) > 180) {
        // roll:  181 ==> -179
        // roll: -181 ==>  179
        ORIENT_ROLL(orient) = -SIGN(ORIENT_ROLL(orient)) * (360 - fabs(ORIENT_YAW(orient)));
    }
    return orient;
}

float angle_distance(float angle1, float angle2)
{
    float angle_diff = fabs(angle1 - angle2);
    if(angle_diff > 180) {
        return 360 - angle_diff;
    }
    return angle_diff;
}

void mesh_apply_ripple(MeshIFace* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase)
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

void mesh_tessellate(MeshIFace* mesh, tessellation_t tessellation)
{
    int prev_num_vert = mesh->get_num_vertex();
    int prev_num_tri  = mesh->get_num_tri();
    switch(tessellation) {
        case TESSELLATION_EDGE_CENTER:
            {
                int new_num_vertex = prev_num_vert + prev_num_tri * 3;
                int new_num_tri    = prev_num_tri * 4;
                glm::vec3 new_vert_coord[new_num_vertex];
                glm::vec2 new_tex_coord[new_num_vertex];
                glm::vec3 new_tri_indices[new_num_tri];
                for(int i = 0; i < prev_num_vert; i++) {
                    new_vert_coord[i] = mesh->get_vert_coord(i);
                    new_tex_coord[i]  = mesh->get_tex_coord(i);
                }

                int current_vert_index = prev_num_vert;
                int current_face_index = 0;
                std::map<uint32_t, int> shared_vert_map;
                for(int i = 0; i < prev_num_tri; i++) {
                    glm::ivec3 tri_indices = mesh->get_tri_indices(i);
                    glm::vec3 vert_a_coord = mesh->get_vert_coord(tri_indices[0]);
                    glm::vec3 vert_b_coord = mesh->get_vert_coord(tri_indices[1]);
                    glm::vec3 vert_c_coord = mesh->get_vert_coord(tri_indices[2]);
                    glm::vec2 tex_a_coord = mesh->get_tex_coord(tri_indices[0]);
                    glm::vec2 tex_b_coord = mesh->get_tex_coord(tri_indices[1]);
                    glm::vec2 tex_c_coord = mesh->get_tex_coord(tri_indices[2]);

                    uint32_t new_vert_shared_ab_key = MAKELONG(std::min(tri_indices[0], tri_indices[1]), std::max(tri_indices[0], tri_indices[1]));
                    int new_vert_shared_ab_index = 0;
                    std::map<uint32_t, int>::iterator p = shared_vert_map.find(new_vert_shared_ab_key);
                    if(p == shared_vert_map.end()) {
                        new_vert_shared_ab_index = current_vert_index++;
                        shared_vert_map.insert(std::pair<uint32_t, int>(new_vert_shared_ab_key, new_vert_shared_ab_index));
                    } else {
                        new_vert_shared_ab_index = (*p).second;
                    }
                    new_vert_coord[new_vert_shared_ab_index] = (vert_a_coord + vert_b_coord) * 0.5f;
                    new_tex_coord[new_vert_shared_ab_index]  = (tex_a_coord + tex_b_coord) * 0.5f;

                    uint32_t new_vert_shared_bc_key = MAKELONG(std::min(tri_indices[1], tri_indices[2]), std::max(tri_indices[1], tri_indices[2]));
                    int new_vert_shared_bc_index = 0;
                    std::map<uint32_t, int>::iterator q = shared_vert_map.find(new_vert_shared_bc_key);
                    if(q == shared_vert_map.end()) {
                        new_vert_shared_bc_index = current_vert_index++;
                        shared_vert_map.insert(std::pair<uint32_t, int>(new_vert_shared_bc_key, new_vert_shared_bc_index));
                    } else {
                        new_vert_shared_bc_index = (*q).second;
                    }
                    new_vert_coord[new_vert_shared_bc_index] = (vert_b_coord + vert_c_coord) * 0.5f;
                    new_tex_coord[new_vert_shared_bc_index]  = (tex_b_coord + tex_c_coord) * 0.5f;

                    uint32_t new_vert_shared_ca_key = MAKELONG(std::min(tri_indices[2], tri_indices[0]), std::max(tri_indices[2], tri_indices[0]));
                    int new_vert_shared_ca_index = 0;
                    std::map<uint32_t, int>::iterator r = shared_vert_map.find(new_vert_shared_ca_key);
                    if(r == shared_vert_map.end()) {
                        new_vert_shared_ca_index = current_vert_index++;
                        shared_vert_map.insert(std::pair<uint32_t, int>(new_vert_shared_ca_key, new_vert_shared_ca_index));
                    } else {
                        new_vert_shared_ca_index = (*r).second;
                    }
                    new_vert_coord[new_vert_shared_ca_index] = (vert_c_coord + vert_a_coord) * 0.5f;
                    new_tex_coord[new_vert_shared_ca_index]  = (tex_c_coord + tex_a_coord) * 0.5f;

                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[0], new_vert_shared_ab_index, new_vert_shared_ca_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[1], new_vert_shared_bc_index, new_vert_shared_ab_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[2], new_vert_shared_ca_index, new_vert_shared_bc_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(new_vert_shared_ab_index, new_vert_shared_bc_index, new_vert_shared_ca_index);
                }

                mesh->realloc(new_num_vertex, new_num_tri);
                for(int i = 0; i < new_num_vertex; i++) {
                    mesh->set_vert_coord(i, new_vert_coord[i]);
                    mesh->set_tex_coord(i, new_tex_coord[i]);
                }
                for(int i = 0; i < new_num_tri; i++) {
                    mesh->set_tri_indices(i, new_tri_indices[i]);
                }
            }
            break;
        case TESSELLATION_TRI_CENTER:
            {
                int new_num_vertex = prev_num_vert + prev_num_tri;
                int new_num_tri    = prev_num_tri * 3;
                glm::vec3 new_vert_coord[new_num_vertex];
                glm::vec2 new_tex_coord[new_num_vertex];
                glm::vec3 new_tri_indices[new_num_tri];
                for(int i = 0; i < prev_num_vert; i++) {
                    new_vert_coord[i] = mesh->get_vert_coord(i);
                    new_tex_coord[i]  = mesh->get_tex_coord(i);
                }

                int current_vert_index = prev_num_vert;
                int current_face_index = 0;
                std::map<uint32_t, int> shared_vert_map;
                for(int i = 0; i < prev_num_tri; i++) {
                    glm::ivec3 tri_indices = mesh->get_tri_indices(i);
                    glm::vec3 vert_a_coord = mesh->get_vert_coord(tri_indices[0]);
                    glm::vec3 vert_b_coord = mesh->get_vert_coord(tri_indices[1]);
                    glm::vec3 vert_c_coord = mesh->get_vert_coord(tri_indices[2]);
                    glm::vec2 tex_a_coord = mesh->get_tex_coord(tri_indices[0]);
                    glm::vec2 tex_b_coord = mesh->get_tex_coord(tri_indices[1]);
                    glm::vec2 tex_c_coord = mesh->get_tex_coord(tri_indices[2]);

                    int new_vert_index = current_vert_index;
                    new_vert_coord[new_vert_index] = (vert_a_coord + vert_b_coord + vert_c_coord) * (1.0f / 3);
                    new_tex_coord[new_vert_index]  = (tex_a_coord + tex_b_coord + tex_c_coord) * (1.0f / 3);
                    current_vert_index++;

                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[0], tri_indices[1], new_vert_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[1], tri_indices[2], new_vert_index);
                    new_tri_indices[current_face_index++] = glm::ivec3(tri_indices[2], tri_indices[0], new_vert_index);
                }

                mesh->realloc(new_num_vertex, new_num_tri);
                for(int i = 0; i < new_num_vertex; i++) {
                    mesh->set_vert_coord(i, new_vert_coord[i]);
                    mesh->set_tex_coord(i, new_tex_coord[i]);
                }
                for(int i = 0; i < new_num_tri; i++) {
                    mesh->set_tri_indices(i, new_tri_indices[i]);
                }
            }
            break;
    }
    mesh->update_normals_and_tangents();
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
