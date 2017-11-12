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

#ifndef VT_TEXTURE_H_
#define VT_TEXTURE_H_

#include <NamedObject.h>
#include <FrameObject.h>
#include <IdentObject.h>
#include <BindableObjectBase.h>
#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>

#define DEFAULT_TEXTURE_WIDTH  256
#define DEFAULT_TEXTURE_HEIGHT 256

namespace vt {

class Texture : public NamedObject,
                public FrameObject<glm::ivec2, int>,
                public IdentObject,
                public BindableObjectBase
{
public:
    typedef enum { RGBA, RGB, RED, DEPTH } format_t;

    Texture(std::string          name            = "",
            format_t             internal_format = Texture::RGBA,
            glm::ivec2           dim             = glm::ivec2(DEFAULT_TEXTURE_WIDTH,
                                                              DEFAULT_TEXTURE_HEIGHT),
            bool                 smooth          = true,
            format_t             format          = Texture::RGBA,
            const unsigned char* pixels      = NULL);
    Texture(std::string name,
            std::string png_filename,
            bool        smooth = true);
    Texture(std::string name,
            std::string png_filename_pos_x,
            std::string png_filename_neg_x,
            std::string png_filename_pos_y,
            std::string png_filename_neg_y,
            std::string png_filename_pos_z,
            std::string png_filename_neg_z);
    virtual ~Texture();
    void bind();

    // accessors
    format_t get_internal_format() const { return m_internal_format; }
    unsigned char* get_pixels() const    { return m_pixels; }

private:
    // core functionality
    void alloc(format_t    internal_format,
               glm::ivec2  dim,
               bool        smooth,
               const void* pixels);
    void alloc(glm::ivec2  dim,
               const void* pixels_pos_x,
               const void* pixels_neg_x,
               const void* pixels_pos_y,
               const void* pixels_neg_y,
               const void* pixels_pos_z,
               const void* pixels_neg_z);

public:
    size_t size() const;
    Texture& operator=(Texture& other);

    // basic modifiers
    void randomize(bool binary = false);
    void draw_x();
    void draw_frame();

    // basic modifiers -- rgba only
    glm::ivec4 get_pixel(glm::ivec2 pos) const;
    void set_pixel(glm::ivec2 pos, glm::ivec4 color);
    void set_color(glm::ivec4 color);

    // basic modifiers -- red only
    float get_pixel_r32f(glm::ivec2 pos) const;
    void set_pixel_r32f(glm::ivec2 pos, float color);
    void set_color_r32f(float color);

    // core functionality
    void update();
    void refresh();

private:
    bool           m_skybox;
    format_t       m_internal_format;
    unsigned char* m_pixels;
    unsigned char* m_pixels_pos_x;
    unsigned char* m_pixels_neg_x;
    unsigned char* m_pixels_pos_y;
    unsigned char* m_pixels_neg_y;
    unsigned char* m_pixels_pos_z;
    unsigned char* m_pixels_neg_z;
};

}

#endif
