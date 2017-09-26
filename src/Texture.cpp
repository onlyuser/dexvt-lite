#include <Texture.h>
#include <NamedObject.h>
#include <FrameObject.h>
#include <Util.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <iostream>
#include <memory.h>
#include <unistd.h>

namespace vt {

Texture::Texture(std::string          name,
                 format_t             internal_format,
                 glm::ivec2           dim,
                 bool                 smooth,
                 format_t             format,
                 const unsigned char* pixels)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), dim),
      m_skybox(false),
      m_internal_format(internal_format),
      m_pixels(NULL),
      m_pixels_pos_x(NULL),
      m_pixels_neg_x(NULL),
      m_pixels_pos_y(NULL),
      m_pixels_neg_y(NULL),
      m_pixels_pos_z(NULL),
      m_pixels_neg_z(NULL)
{
    unsigned char* dest_pixels = NULL;
    if(format == RGB && pixels) {
        size_t size_buf = dim.x * dim.y * sizeof(unsigned char) * 4;
        dest_pixels = new unsigned char[size_buf];
        if(!dest_pixels) {
            return;
        }
        for(int y = 0; y < dim.y; y++) {
            for(int x = 0; x < dim.x; x++) {
                int src_pixel_offset  = (y * dim.x + x) * 3;
                int dest_pixel_offset = (y * dim.x + x) * 4;
                dest_pixels[dest_pixel_offset + 0] = pixels[src_pixel_offset + 0];
                dest_pixels[dest_pixel_offset + 1] = pixels[src_pixel_offset + 1];
                dest_pixels[dest_pixel_offset + 2] = pixels[src_pixel_offset + 2];
                dest_pixels[dest_pixel_offset + 3] = 1;
            }
        }
    } else {
        dest_pixels = const_cast<unsigned char*>(pixels);
    }
    alloc(internal_format,
          dim,
          smooth,
          dest_pixels);
    if(pixels) {
        return;
    }
    draw_x();
    update();
}

Texture::Texture(std::string name,
                 std::string png_filename,
                 bool        smooth)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), glm::ivec2(0)),
      m_skybox(false),
      m_internal_format(Texture::RGBA),
      m_pixels(NULL),
      m_pixels_pos_x(NULL),
      m_pixels_neg_x(NULL),
      m_pixels_pos_y(NULL),
      m_pixels_neg_y(NULL),
      m_pixels_pos_z(NULL),
      m_pixels_neg_z(NULL)
{
    unsigned char* pixels = NULL;
    size_t width  = 0;
    size_t height = 0;
    if(!read_png(png_filename, (void**)&pixels, &width, &height, true) || !pixels) {
        return;
    }
    alloc(Texture::RGBA,
          glm::ivec2(width, height),
          smooth,
          pixels);
    if(!pixels) {
        return;
    }
    delete []pixels;
}

Texture::Texture(std::string name,
                 std::string png_filename_pos_x,
                 std::string png_filename_neg_x,
                 std::string png_filename_pos_y,
                 std::string png_filename_neg_y,
                 std::string png_filename_pos_z,
                 std::string png_filename_neg_z)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), glm::ivec2(0)),
      m_skybox(true),
      m_internal_format(Texture::RGBA),
      m_pixels(NULL),
      m_pixels_pos_x(NULL),
      m_pixels_neg_x(NULL),
      m_pixels_pos_y(NULL),
      m_pixels_neg_y(NULL),
      m_pixels_pos_z(NULL),
      m_pixels_neg_z(NULL)
{
    if(png_filename_pos_x.empty() ||
       png_filename_neg_x.empty() ||
       png_filename_pos_y.empty() ||
       png_filename_neg_y.empty() ||
       png_filename_pos_z.empty() ||
       png_filename_neg_z.empty())
    {
        return;
    }
    if(access(png_filename_pos_x.c_str(), F_OK) == -1 ||
       access(png_filename_neg_x.c_str(), F_OK) == -1 ||
       access(png_filename_pos_y.c_str(), F_OK) == -1 ||
       access(png_filename_neg_y.c_str(), F_OK) == -1 ||
       access(png_filename_pos_z.c_str(), F_OK) == -1 ||
       access(png_filename_neg_z.c_str(), F_OK) == -1)
    {
        return;
    }
    size_t width  = 0;
    size_t height = 0;
    unsigned char* pixels_pos_x = NULL;
    unsigned char* pixels_neg_x = NULL;
    unsigned char* pixels_pos_y = NULL;
    unsigned char* pixels_neg_y = NULL;
    unsigned char* pixels_pos_z = NULL;
    unsigned char* pixels_neg_z = NULL;
    if(!read_png(png_filename_pos_x, (void**)&pixels_pos_x, &width, &height, true) || !pixels_pos_x) {
        std::cout << "failed to load cube map positive x" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_x, (void**)&pixels_neg_x, &width, &height, true) || !pixels_neg_x) {
        std::cout << "failed to load cube map negative x" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_y, (void**)&pixels_pos_y, &width, &height, true) || !pixels_pos_y) {
        std::cout << "failed to load cube map positive y" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_y, (void**)&pixels_neg_y, &width, &height, true) || !pixels_neg_y) {
        std::cout << "failed to load cube map negative y" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_z, (void**)&pixels_pos_z, &width, &height, true) || !pixels_pos_z) {
        std::cout << "failed to load cube map positive z" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_z, (void**)&pixels_neg_z, &width, &height, true) || !pixels_neg_z) {
        std::cout << "failed to load cube map negative z" << std::endl;
        return;
    }
    alloc(glm::ivec2(width, height),
          pixels_pos_x,
          pixels_neg_x,
          pixels_pos_y,
          pixels_neg_y,
          pixels_pos_z,
          pixels_neg_z);
    delete[] pixels_pos_x;
    delete[] pixels_neg_x;
    delete[] pixels_pos_y;
    delete[] pixels_neg_y;
    delete[] pixels_pos_z;
    delete[] pixels_neg_z;
}

Texture::~Texture()
{
    if(!m_id) {
        return;
    }
    glDeleteTextures(1, &m_id);
    if(m_skybox) {
        if(!m_pixels_pos_x ||
           !m_pixels_neg_x ||
           !m_pixels_pos_y ||
           !m_pixels_neg_y ||
           !m_pixels_pos_z ||
           !m_pixels_neg_z)
        {
            return;
        }
        delete[] m_pixels_pos_x;
        delete[] m_pixels_neg_x;
        delete[] m_pixels_pos_y;
        delete[] m_pixels_neg_y;
        delete[] m_pixels_pos_z;
        delete[] m_pixels_neg_z;
        return;
    }
    if(!m_pixels) {
        return;
    }
    delete[] m_pixels;
}

void Texture::bind()
{
    if(!m_id) {
        return;
    }
    if(m_skybox) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
        return;
    }
    glBindTexture(GL_TEXTURE_2D, m_id);
}

//===================
// core functionality
//===================

void Texture::alloc(format_t    internal_format,
                    glm::ivec2  dim,
                    bool        smooth,
                    const void* pixels)
{
    glGenTextures(1, &m_id);
    if(!m_id) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    m_dim             = dim;
    m_skybox          = false;
    m_internal_format = internal_format;
    size_t size_buf   = size();
    m_pixels = new unsigned char[size_buf];
    if(!m_pixels) {
        return;
    }
    if(!pixels) {
        update();
        return;
    }
    memcpy(m_pixels, pixels, size_buf);
    update();
}

void Texture::alloc(glm::ivec2  dim,
                    const void* pixels_pos_x,
                    const void* pixels_neg_x,
                    const void* pixels_pos_y,
                    const void* pixels_neg_y,
                    const void* pixels_pos_z,
                    const void* pixels_neg_z)
{
    glGenTextures(1, &m_id);
    if(!m_id) {
        return;
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    m_dim             = dim;
    m_skybox          = true;
    m_internal_format = Texture::RGBA;
    size_t size_buf   = size();
    m_pixels_pos_x = new unsigned char[size_buf];
    m_pixels_neg_x = new unsigned char[size_buf];
    m_pixels_pos_y = new unsigned char[size_buf];
    m_pixels_neg_y = new unsigned char[size_buf];
    m_pixels_pos_z = new unsigned char[size_buf];
    m_pixels_neg_z = new unsigned char[size_buf];
    if(!m_pixels_pos_x ||
       !m_pixels_neg_x ||
       !m_pixels_pos_y ||
       !m_pixels_neg_y ||
       !m_pixels_pos_z ||
       !m_pixels_neg_z)
    {
        return;
    }
    if(!pixels_pos_x   ||
       !pixels_neg_x   ||
       !pixels_pos_y   ||
       !pixels_neg_y   ||
       !pixels_pos_z   ||
       !pixels_neg_z)
    {
        update();
        return;
    }
    memcpy(m_pixels_pos_x, pixels_pos_x, size_buf);
    memcpy(m_pixels_neg_x, pixels_neg_x, size_buf);
    memcpy(m_pixels_pos_y, pixels_pos_y, size_buf);
    memcpy(m_pixels_neg_y, pixels_neg_y, size_buf);
    memcpy(m_pixels_pos_z, pixels_pos_z, size_buf);
    memcpy(m_pixels_neg_z, pixels_neg_z, size_buf);
    update();
}

size_t Texture::size() const
{
    if(m_skybox) {
        return m_dim.x * m_dim.y * sizeof(unsigned char) * 4; // per cube face
    }
    switch(m_internal_format) {
        case Texture::RGBA:  return m_dim.x * m_dim.y * sizeof(unsigned char) * 4;
        case Texture::RGB:   assert(false); break;
        case Texture::RED:   return m_dim.x * m_dim.y * sizeof(unsigned char) * 4;
        case Texture::DEPTH: return m_dim.x * m_dim.y * sizeof(float);
        default:
            break;
    }
    return 0;
}

//================
// basic modifiers
//================

void Texture::randomize()
{
    if(m_skybox) {
        return;
    }
    if(!m_pixels) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            {
                size_t size_buf = size();
                srand(time(NULL));
                for(int i = 0; i < static_cast<int>(size_buf); i++) {
                    m_pixels[i] = rand() % 256;
                }
            }
            break;
        case Texture::RGB:
            assert(false);
            break;
        case Texture::RED:
        case Texture::DEPTH:
            {
                float* pixels = reinterpret_cast<float*>(m_pixels);
                size_t n = m_dim.x * m_dim.y;
                for(int i = 0; i < static_cast<int>(n); i++) {
                    pixels[i] = static_cast<float>(rand()) / RAND_MAX;
                }
            }
            break;
        default:
            break;
    }
}

void Texture::draw_x()
{
    if(m_skybox) {
        return;
    }
    if(!m_pixels) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            {
                size_t min_dim = std::min(m_dim.x, m_dim.y);
                for(int i = 0; i < static_cast<int>(min_dim); i++) {
                    int pixel_offset  = (i * m_dim.x + i) * 4;
                    int pixel_offset2 = (i * m_dim.x + (m_dim.x - i)) * 4;
                    m_pixels[pixel_offset  + 0] = 255;
                    m_pixels[pixel_offset  + 1] = 255;
                    m_pixels[pixel_offset  + 2] = 255;
                    m_pixels[pixel_offset  + 3] = 255;
                    m_pixels[pixel_offset2 + 0] = 255;
                    m_pixels[pixel_offset2 + 1] = 255;
                    m_pixels[pixel_offset2 + 2] = 255;
                    m_pixels[pixel_offset2 + 3] = 255;
                }
            }
            break;
        case Texture::RGB:
            assert(false);
            break;
        case Texture::RED:
        case Texture::DEPTH:
            {
                float* pixels = reinterpret_cast<float*>(m_pixels);
                size_t min_dim = std::min(m_dim.x, m_dim.y);
                for(int i = 0; i < static_cast<int>(min_dim); i++) {
                    pixels[i * m_dim.x + i]             = 1;
                    pixels[i * m_dim.x + (m_dim.x - i)] = 1;
                }
            }
            break;
        default:
            break;
    }
}

//=============================
// basic modifiers -- rgba only
//=============================

glm::ivec4 Texture::get_pixel(glm::ivec2 pos) const
{
    if(!m_pixels) {
        return glm::ivec4(0);
    }
    int pixel_offset = (pos.y * m_dim.x + pos.x) * 4;
    return glm::ivec4(m_pixels[pixel_offset + 0],
                      m_pixels[pixel_offset + 1],
                      m_pixels[pixel_offset + 2],
                      m_pixels[pixel_offset + 3]);
}

void Texture::set_pixel(glm::ivec2 pos, glm::ivec4 color)
{
    if(!m_pixels) {
        return;
    }
    int pixel_offset = (pos.y * m_dim.x + pos.x) * 4;
    m_pixels[pixel_offset + 0] = color.r;
    m_pixels[pixel_offset + 1] = color.g;
    m_pixels[pixel_offset + 2] = color.b;
    m_pixels[pixel_offset + 3] = color.a;
}

void Texture::set_color(glm::ivec4 color)
{
    if(m_skybox) {
        return;
    }
    if(!m_pixels) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            {
                size_t size_buf = size();
                for(int i = 0; i < static_cast<int>(size_buf); i += 4) {
                    m_pixels[i + 0] = color.r;
                    m_pixels[i + 1] = color.g;
                    m_pixels[i + 2] = color.b;
                    m_pixels[i + 3] = color.a;
                }
            }
            break;
        case Texture::RGB:
            assert(false);
            break;
        case Texture::RED:
            break;
        case Texture::DEPTH:
            break;
        default:
            break;
    }
}

//============================
// basic modifiers -- red only
//============================

float Texture::get_pixel_r32f(glm::ivec2 pos) const
{
    if(!m_pixels) {
        return 0;
    }
    int pixel_offset = (pos.y * m_dim.x + pos.x) * 4;
    return *reinterpret_cast<float*>(&(m_pixels[pixel_offset + 0]));
}

void Texture::set_pixel_r32f(glm::ivec2 pos, float color)
{
    if(!m_pixels) {
        return;
    }
    int pixel_offset = (pos.y * m_dim.x + pos.x) * 4;
    *reinterpret_cast<float*>(&(m_pixels[pixel_offset + 0])) = color;
}

void Texture::set_color_r32f(float color)
{
    if(m_skybox) {
        return;
    }
    if(!m_pixels) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            break;
        case Texture::RGB:
            assert(false);
            break;
        case Texture::RED:
            {
                float* pixels = reinterpret_cast<float*>(m_pixels);
                size_t n = m_dim.x * m_dim.y;
                for(int i = 0; i < static_cast<int>(n); i++) {
                    pixels[i] = color;
                }
            }
            break;
        case Texture::DEPTH:
            break;
        default:
            break;
    }
}

//===================
// core functionality
//===================

// NOTE: upload to gpu
void Texture::update()
{
    bind();
    if(m_skybox) {
        if(!m_pixels_pos_x ||
           !m_pixels_neg_x ||
           !m_pixels_pos_y ||
           !m_pixels_neg_y ||
           !m_pixels_pos_z ||
           !m_pixels_neg_z)
        {
            return;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixels_pos_x);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixels_neg_x);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixels_neg_y);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixels_pos_y);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixels_pos_z);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixels_neg_z);
        return;
    }
    if(!m_pixels) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            glTexImage2D(GL_TEXTURE_2D,    // target
                         0,                // level, 0 = base, no mipmap,
                         GL_RGBA,          // internal format
                         m_dim.x,          // width
                         m_dim.y,          // height
                         0,                // border, always 0 in OpenGL ES
                         GL_RGBA,          // format
                         GL_UNSIGNED_BYTE, // type
                         m_pixels);
            break;
        case Texture::RGB:
            assert(false);
            break;
        case Texture::RED:
            glTexImage2D(GL_TEXTURE_2D, // target
                         0,             // level, 0 = base, no mipmap,
                         GL_R32F,       // internal format
                         m_dim.x,       // width
                         m_dim.y,       // height
                         0,             // border, always 0 in OpenGL ES
                         GL_RED,        // format
                         GL_FLOAT,      // type
                         m_pixels);
            break;
        case Texture::DEPTH:
            glTexImage2D(GL_TEXTURE_2D,      // target
                         0,                  // level, 0 = base, no mipmap,
                         GL_DEPTH_COMPONENT, // internal format
                         m_dim.x,            // width
                         m_dim.y,            // height
                         0,                  // border, always 0 in OpenGL ES
                         GL_DEPTH_COMPONENT, // format
                         GL_FLOAT,           // type
                         m_pixels);
            break;
        default:
            break;
    }
}

// NOTE: download from gpu
void Texture::refresh()
{
    bind();
    if(m_skybox) {
        if(!m_pixels_pos_x ||
           !m_pixels_neg_x ||
           !m_pixels_pos_y ||
           !m_pixels_neg_y ||
           !m_pixels_pos_z ||
           !m_pixels_neg_z)
        {
            return;
        }
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixels_pos_x);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixels_neg_x);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixels_neg_y);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixels_pos_y);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixels_pos_z);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixels_neg_z);
        return;
    }
    if(!m_pixels) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            glGetTexImage(GL_TEXTURE_2D,    // target
                          0,                // level, 0 = base, no mipmap,
                          GL_RGBA,          // format
                          GL_UNSIGNED_BYTE, // type
                          m_pixels);
            break;
        case Texture::RGB:
            assert(false);
            break;
        case Texture::RED:
            glGetTexImage(GL_TEXTURE_2D, // target
                          0,             // level, 0 = base, no mipmap,
                          GL_RED,        // format
                          GL_FLOAT,      // type
                          m_pixels);
            break;
        case Texture::DEPTH:
            glGetTexImage(GL_TEXTURE_2D,      // target
                          0,                  // level, 0 = base, no mipmap,
                          GL_DEPTH_COMPONENT, // format
                          GL_FLOAT,           // type
                          m_pixels);
            break;
        default:
            break;
    }
}

}
