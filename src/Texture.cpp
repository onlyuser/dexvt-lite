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
                 const unsigned char* pixel_data)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), dim),
      m_skybox(false),
      m_internal_format(internal_format),
      m_pixel_data(NULL),
      m_pixel_data_pos_x(NULL),
      m_pixel_data_neg_x(NULL),
      m_pixel_data_pos_y(NULL),
      m_pixel_data_neg_y(NULL),
      m_pixel_data_pos_z(NULL),
      m_pixel_data_neg_z(NULL)
{
    unsigned char* dest_pixel_data = NULL;
    if(format == RGB && pixel_data) {
        size_t size_buf = dim.x * dim.y * sizeof(unsigned char) * 4;
        dest_pixel_data = new unsigned char[size_buf];
        if(!dest_pixel_data) {
            return;
        }
        for(int y = 0; y < dim.y; y++) {
            for(int x = 0; x < dim.x; x++) {
                int src_pixel_offset_scanline_start  = (y * dim.x + x) * 3;
                int dest_pixel_offset_scanline_start = (y * dim.x + x) * 4;
                dest_pixel_data[dest_pixel_offset_scanline_start + 0] = pixel_data[src_pixel_offset_scanline_start + 0];
                dest_pixel_data[dest_pixel_offset_scanline_start + 1] = pixel_data[src_pixel_offset_scanline_start + 1];
                dest_pixel_data[dest_pixel_offset_scanline_start + 2] = pixel_data[src_pixel_offset_scanline_start + 2];
                dest_pixel_data[dest_pixel_offset_scanline_start + 3] = 1;
            }
        }
    } else {
        dest_pixel_data = const_cast<unsigned char*>(pixel_data);
    }
    alloc(internal_format,
          dim,
          smooth,
          dest_pixel_data);
    if(pixel_data) {
        return;
    }
    draw_big_x();
}

Texture::Texture(std::string name,
                 std::string png_filename,
                 bool        smooth)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), glm::ivec2(0)),
      m_skybox(false),
      m_internal_format(Texture::RGBA),
      m_pixel_data(NULL),
      m_pixel_data_pos_x(NULL),
      m_pixel_data_neg_x(NULL),
      m_pixel_data_pos_y(NULL),
      m_pixel_data_neg_y(NULL),
      m_pixel_data_pos_z(NULL),
      m_pixel_data_neg_z(NULL)
{
    unsigned char* pixel_data = NULL;
    size_t width  = 0;
    size_t height = 0;
    if(!read_png(png_filename, (void**)&pixel_data, &width, &height, true) || !pixel_data) {
        return;
    }
    alloc(Texture::RGBA,
          glm::ivec2(width, height),
          smooth,
          pixel_data);
    if(!pixel_data) {
        return;
    }
    delete []pixel_data;
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
      m_pixel_data(NULL),
      m_pixel_data_pos_x(NULL),
      m_pixel_data_neg_x(NULL),
      m_pixel_data_pos_y(NULL),
      m_pixel_data_neg_y(NULL),
      m_pixel_data_pos_z(NULL),
      m_pixel_data_neg_z(NULL)
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
    unsigned char* pixel_data_pos_x = NULL;
    unsigned char* pixel_data_neg_x = NULL;
    unsigned char* pixel_data_pos_y = NULL;
    unsigned char* pixel_data_neg_y = NULL;
    unsigned char* pixel_data_pos_z = NULL;
    unsigned char* pixel_data_neg_z = NULL;
    if(!read_png(png_filename_pos_x, (void**)&pixel_data_pos_x, &width, &height, true) || !pixel_data_pos_x) {
        std::cout << "failed to load cube map positive x" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_x, (void**)&pixel_data_neg_x, &width, &height, true) || !pixel_data_neg_x) {
        std::cout << "failed to load cube map negative x" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_y, (void**)&pixel_data_pos_y, &width, &height, true) || !pixel_data_pos_y) {
        std::cout << "failed to load cube map positive y" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_y, (void**)&pixel_data_neg_y, &width, &height, true) || !pixel_data_neg_y) {
        std::cout << "failed to load cube map negative y" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_z, (void**)&pixel_data_pos_z, &width, &height, true) || !pixel_data_pos_z) {
        std::cout << "failed to load cube map positive z" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_z, (void**)&pixel_data_neg_z, &width, &height, true) || !pixel_data_neg_z) {
        std::cout << "failed to load cube map negative z" << std::endl;
        return;
    }
    alloc(glm::ivec2(width, height),
          pixel_data_pos_x,
          pixel_data_neg_x,
          pixel_data_pos_y,
          pixel_data_neg_y,
          pixel_data_pos_z,
          pixel_data_neg_z);
    delete[] pixel_data_pos_x;
    delete[] pixel_data_neg_x;
    delete[] pixel_data_pos_y;
    delete[] pixel_data_neg_y;
    delete[] pixel_data_pos_z;
    delete[] pixel_data_neg_z;
}

Texture::~Texture()
{
    if(!m_id) {
        return;
    }
    glDeleteTextures(1, &m_id);
    if(m_skybox) {
        if(!m_pixel_data_pos_x ||
           !m_pixel_data_neg_x ||
           !m_pixel_data_pos_y ||
           !m_pixel_data_neg_y ||
           !m_pixel_data_pos_z ||
           !m_pixel_data_neg_z)
        {
            return;
        }
        delete[] m_pixel_data_pos_x;
        delete[] m_pixel_data_neg_x;
        delete[] m_pixel_data_pos_y;
        delete[] m_pixel_data_neg_y;
        delete[] m_pixel_data_pos_z;
        delete[] m_pixel_data_neg_z;
        return;
    }
    if(!m_pixel_data) {
        return;
    }
    delete[] m_pixel_data;
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

void Texture::alloc(format_t    internal_format,
                    glm::ivec2  dim,
                    bool        smooth,
                    const void* pixel_data)
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
    size_t size_buf = get_pixel_data_size();
    m_pixel_data = new unsigned char[size_buf];
    if(!m_pixel_data) {
        return;
    }
    if(!pixel_data) {
        upload_to_gpu();
        return;
    }
    memcpy(m_pixel_data, pixel_data, size_buf);
    upload_to_gpu();
}

void Texture::alloc(glm::ivec2  dim,
                    const void* pixel_data_pos_x,
                    const void* pixel_data_neg_x,
                    const void* pixel_data_pos_y,
                    const void* pixel_data_neg_y,
                    const void* pixel_data_pos_z,
                    const void* pixel_data_neg_z)
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
    size_t size_buf = get_pixel_data_size();
    m_pixel_data_pos_x = new unsigned char[size_buf];
    m_pixel_data_neg_x = new unsigned char[size_buf];
    m_pixel_data_pos_y = new unsigned char[size_buf];
    m_pixel_data_neg_y = new unsigned char[size_buf];
    m_pixel_data_pos_z = new unsigned char[size_buf];
    m_pixel_data_neg_z = new unsigned char[size_buf];
    if(!m_pixel_data_pos_x ||
       !m_pixel_data_neg_x ||
       !m_pixel_data_pos_y ||
       !m_pixel_data_neg_y ||
       !m_pixel_data_pos_z ||
       !m_pixel_data_neg_z)
    {
        return;
    }
    if(!pixel_data_pos_x   ||
       !pixel_data_neg_x   ||
       !pixel_data_pos_y   ||
       !pixel_data_neg_y   ||
       !pixel_data_pos_z   ||
       !pixel_data_neg_z)
    {
        upload_to_gpu();
        return;
    }
    memcpy(m_pixel_data_pos_x, pixel_data_pos_x, size_buf);
    memcpy(m_pixel_data_neg_x, pixel_data_neg_x, size_buf);
    memcpy(m_pixel_data_pos_y, pixel_data_pos_y, size_buf);
    memcpy(m_pixel_data_neg_y, pixel_data_neg_y, size_buf);
    memcpy(m_pixel_data_pos_z, pixel_data_pos_z, size_buf);
    memcpy(m_pixel_data_neg_z, pixel_data_neg_z, size_buf);
    upload_to_gpu();
}

size_t Texture::get_pixel_data_size() const
{
    if(m_skybox) {
        return m_dim.x * m_dim.y * sizeof(unsigned char) * 4; // per cube face
    }
    switch(m_internal_format) {
        case Texture::RGBA:  return m_dim.x * m_dim.y * sizeof(unsigned char) * 4;
        case Texture::DEPTH: return m_dim.x * m_dim.y * sizeof(float);
        default:
            break;
    }
    return 0;
}

glm::ivec4 Texture::get_pixel(glm::ivec2 pos) const
{
    if(!m_pixel_data) {
        return glm::ivec4(0);
    }
    int pixel_offset = (pos.y * m_dim.x + pos.x) * 4;
    return glm::ivec4(m_pixel_data[pixel_offset + 0],
                      m_pixel_data[pixel_offset + 1],
                      m_pixel_data[pixel_offset + 2],
                      m_pixel_data[pixel_offset + 3]);
}

void Texture::set_pixel(glm::ivec2 pos, glm::ivec4 color)
{
    if(!m_pixel_data) {
        return;
    }
    int pixel_offset = (pos.y * m_dim.x + pos.x) * 4;
    m_pixel_data[pixel_offset + 0] = color.r;
    m_pixel_data[pixel_offset + 1] = color.g;
    m_pixel_data[pixel_offset + 2] = color.b;
    m_pixel_data[pixel_offset + 3] = color.a;
}

void Texture::set_solid_color(glm::ivec4 color)
{
    if(m_skybox) {
        return;
    }
    if(!m_pixel_data) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            {
                size_t size_buf = get_pixel_data_size();
                for(int i = 0; i < static_cast<int>(size_buf); i += 4) {
                    m_pixel_data[i + 0] = color.r;
                    m_pixel_data[i + 1] = color.g;
                    m_pixel_data[i + 2] = color.b;
                    m_pixel_data[i + 3] = color.a;
                }
            }
            break;
        case Texture::DEPTH:
            break;
        default:
            break;
    }
    upload_to_gpu();
}

void Texture::randomize()
{
    if(m_skybox) {
        return;
    }
    if(!m_pixel_data) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            {
                size_t size_buf = get_pixel_data_size();
                srand(time(NULL));
                for(int i = 0; i < static_cast<int>(size_buf); i++) {
                    m_pixel_data[i] = rand() % 256;
                }
            }
            break;
        case Texture::DEPTH:
            break;
        default:
            break;
    }
    upload_to_gpu();
}

void Texture::draw_big_x()
{
    if(m_skybox) {
        return;
    }
    if(!m_pixel_data) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            {
                size_t min_dim = std::min(m_dim.x, m_dim.y);
                for(int i = 0; i < static_cast<int>(min_dim); i++) {
                    int pixel_offset_scanline_start = (i * m_dim.x + i) * 4;
                    int pixel_offset_scanline_end   = (i * m_dim.x + (m_dim.x - i)) * 4;
                    m_pixel_data[pixel_offset_scanline_start + 0] = 255;
                    m_pixel_data[pixel_offset_scanline_start + 1] = 255;
                    m_pixel_data[pixel_offset_scanline_start + 2] = 255;
                    m_pixel_data[pixel_offset_scanline_start + 3] = 255;
                    m_pixel_data[pixel_offset_scanline_end   + 0] = 255;
                    m_pixel_data[pixel_offset_scanline_end   + 1] = 255;
                    m_pixel_data[pixel_offset_scanline_end   + 2] = 255;
                    m_pixel_data[pixel_offset_scanline_end   + 3] = 255;
                }
            }
            break;
        case Texture::DEPTH:
            {
                float* pixel_data = reinterpret_cast<float*>(m_pixel_data);
                size_t min_dim = std::min(m_dim.x, m_dim.y);
                for(int i = 0; i < static_cast<int>(min_dim); i++) {
                    pixel_data[i * m_dim.x + i]             = 1;
                    pixel_data[i * m_dim.x + (m_dim.x - i)] = 1;
                }
            }
            break;
        default:
            break;
    }
    upload_to_gpu();
}

void Texture::upload_to_gpu()
{
    bind();
    if(m_skybox) {
        if(!m_pixel_data_pos_x ||
           !m_pixel_data_neg_x ||
           !m_pixel_data_pos_y ||
           !m_pixel_data_neg_y ||
           !m_pixel_data_pos_z ||
           !m_pixel_data_neg_z)
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
                     m_pixel_data_pos_x);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_neg_x);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_neg_y);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_pos_y);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_pos_z);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGBA,                        // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGBA,                        // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_neg_z);
        return;
    }
    if(!m_pixel_data) {
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
                         m_pixel_data);
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
                         m_pixel_data);
            break;
        default:
            break;
    }
}

void Texture::download_from_gpu()
{
    bind();
    if(m_skybox) {
        if(!m_pixel_data_pos_x ||
           !m_pixel_data_neg_x ||
           !m_pixel_data_pos_y ||
           !m_pixel_data_neg_y ||
           !m_pixel_data_pos_z ||
           !m_pixel_data_neg_z)
        {
            return;
        }
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_pos_x);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_neg_x);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_neg_y);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_pos_y);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_pos_z);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGBA,                        // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_neg_z);
        return;
    }
    if(!m_pixel_data) {
        return;
    }
    switch(m_internal_format) {
        case Texture::RGBA:
            glGetTexImage(GL_TEXTURE_2D,    // target
                          0,                // level, 0 = base, no mipmap,
                          GL_RGBA,          // format
                          GL_UNSIGNED_BYTE, // type
                          m_pixel_data);
            break;
        case Texture::DEPTH:
            glGetTexImage(GL_TEXTURE_2D,      // target
                          0,                  // level, 0 = base, no mipmap,
                          GL_DEPTH_COMPONENT, // format
                          GL_FLOAT,           // type
                          m_pixel_data);
            break;
        default:
            break;
    }
}

}
