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
    glm::vec3 euler;

    // yaw
    if(static_cast<float>(fabs(offset.x)) < EPSILON && static_cast<float>(fabs(offset.z)) < EPSILON) {
        // vertical
        EULER_PITCH(euler) = 90;
        if(up_direction) {
            glm::vec3 flattened_offset = glm::vec3(up_direction->x, 0, up_direction->z);
            if(SIGN(offset.y) == SIGN(up_direction->y)) {
                flattened_offset = -flattened_offset;
            }
            EULER_YAW(euler) = glm::degrees(glm::angle(glm::normalize(flattened_offset), VEC_FORWARD));
            if(flattened_offset.x < 0) {
                EULER_YAW(euler) = -fabs(EULER_YAW(euler));
            }
        }
    } else {
        // non-vertical
        glm::vec3 flattened_offset = glm::normalize(glm::vec3(offset.x, 0, offset.z));
        EULER_PITCH(euler) = glm::degrees(glm::angle(flattened_offset, glm::normalize(offset))),
        EULER_YAW(euler)   = glm::degrees(glm::angle(flattened_offset, VEC_FORWARD));
        if(flattened_offset.x < 0) {
            EULER_YAW(euler) = -fabs(EULER_YAW(euler));
        }
    }

    // pitch
    if(offset.y > 0) {
        EULER_PITCH(euler) = -fabs(EULER_PITCH(euler));
    }

    // roll
    if(up_direction) {
        glm::mat4 euler_transform_sans_roll = GLM_EULER_TRANSFORM_SANS_ROLL(EULER_YAW(euler), EULER_PITCH(euler));
        glm::vec3 local_up_direction_roll_component = glm::vec3(glm::inverse(euler_transform_sans_roll) *
                                                      glm::vec4(*up_direction, 1));
        EULER_ROLL(euler) = glm::degrees(glm::angle(glm::normalize(local_up_direction_roll_component), VEC_UP));
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

bool read_png(std::string png_filename,
              void**      pixel_data,
              size_t*     width,
              size_t*     height)
{
    if(!pixel_data || !width || !height) {
        return false;
    }

    // header for testing if it is a png
    png_byte header[8];

    // open file as binary
    FILE* fp = fopen(png_filename.c_str(), "rb");
    if(!fp) {
        return false;
    }

    // read the header
    fread(header, 1, 8, fp);

    // test if png
    int is_png = !png_sig_cmp(header, 0, 8);
    if(!is_png) {
        fclose(fp);
        return false;
    }

    // create png struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        fclose(fp);
        return false;
    }

    // create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fclose(fp);
        return false;
    }

    // create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if(!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fclose(fp);
        return false;
    }

    // png error stuff, not sure libpng man suggests this
    if(setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return false;
    }

    // init png reading
    png_init_io(png_ptr, fp);

    // let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);

    // variables to pass to get info
    int bit_depth, color_type;
    png_uint_32 twidth, theight;

    // get info about png
    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    // update the png info struct.
    png_read_update_info(png_ptr, info_ptr);

    // row size in bytes.
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    // allocate the image_data as a big block, to be given to opengl
    png_byte* image_data = new png_byte[rowbytes * theight];
    if(!image_data) {
        // clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return false;
    }

    // row_pointers is for pointing to image_data for reading the png with libpng
    png_bytep* row_pointers = new png_bytep[theight];
    if(!row_pointers) {
        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        delete[] image_data;
        fclose(fp);
        return false;
    }
    // set the individual row_pointers to point at the correct offsets of image_data
    for(int i = 0; i < static_cast<int>(theight); i++) {
        row_pointers[theight - 1 - i] = image_data + i * rowbytes;
    }

    // read the png into image_data through row_pointers
    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;
    fclose(fp);

    // clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    // update width and height based on png info
    *pixel_data = image_data;
    *width      = twidth;
    *height     = theight;

    return true;
}

}
