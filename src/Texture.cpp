#include <Texture.h>
#include <NamedObject.h>
#include <FrameObject.h>
#include <Util.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <iostream>
#include <memory.h>

namespace vt {

Texture::Texture(std::string          name,
                 glm::ivec2           dim,
                 const unsigned char* pixel_data,
                 type_t               type,
                 bool                 smooth,
                 bool                 random)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), dim),
      m_skybox(false),
      m_type(type),
      m_pixel_data(NULL),
      m_pixel_data_pos_x(NULL),
      m_pixel_data_neg_x(NULL),
      m_pixel_data_pos_y(NULL),
      m_pixel_data_neg_y(NULL),
      m_pixel_data_pos_z(NULL),
      m_pixel_data_neg_z(NULL)
{
    if(pixel_data) {
        gen_texture_internal(dim, pixel_data, type, smooth);
        return;
    }
    switch(m_type) {
        case Texture::RGB:
            {
                size_t size_buf = dim.x * dim.y * sizeof(unsigned char) * 3;
                unsigned char* init_pixel_data = new unsigned char[size_buf];
                if(!init_pixel_data) {
                    return;
                }
                memset(init_pixel_data, 0, size_buf);
                if(random) {
                    srand(time(NULL));
                    for(int i = 0; i < static_cast<int>(dim.y); i++) {
                        for(int j = 0; j < static_cast<int>(dim.x); j++) {
                            int pixel_offset = (i * dim.x + j) * 3;
                            init_pixel_data[pixel_offset + 0] = rand() % 256;
                            init_pixel_data[pixel_offset + 1] = rand() % 256;
                            init_pixel_data[pixel_offset + 2] = rand() % 256;
                        }
                    }
                    gen_texture_internal(dim, init_pixel_data, type, smooth);
                } else {
                    // draw big red 'x'
                    for(int i = 0; i < static_cast<int>(std::min(dim.x, dim.y)); i++) {
                        int pixel_offset_scanline_start = (i * dim.x + i) * 3;
                        int pixel_offset_scanline_end   = (i * dim.x + (dim.x - i)) * 3;
                        init_pixel_data[pixel_offset_scanline_start + 0] = 255;
                        init_pixel_data[pixel_offset_scanline_start + 1] = 0;
                        init_pixel_data[pixel_offset_scanline_start + 2] = 0;
                        init_pixel_data[pixel_offset_scanline_end   + 0] = 255;
                        init_pixel_data[pixel_offset_scanline_end   + 1] = 0;
                        init_pixel_data[pixel_offset_scanline_end   + 2] = 0;
                    }
                    gen_texture_internal(dim, init_pixel_data, type, smooth);
                }
                delete []init_pixel_data;
            }
            break;
        case Texture::DEPTH:
            {
                size_t size_buf = dim.x * dim.y * sizeof(float) * 3;
                unsigned char* init_pixel_data = new unsigned char[size_buf];
                if(!init_pixel_data) {
                    return;
                }
                memset(init_pixel_data, 0, size_buf);
                for(int i = 0; i < static_cast<int>(std::min(dim.x, dim.y)); i++) {
                    init_pixel_data[i * dim.x + i]         = 1;
                    init_pixel_data[i * dim.x + dim.x - i] = 1;
                }
                gen_texture_internal(dim, init_pixel_data, type, smooth);
                delete []init_pixel_data;
            }
            break;
    }
}

Texture::Texture(std::string name,
                 std::string png_filename,
                 bool        smooth)
    : NamedObject(name),
      FrameObject(glm::ivec2(0), glm::ivec2(0)),
      m_skybox(false),
      m_type(Texture::RGB)
{
    unsigned char* pixel_data = NULL;
    size_t width  = 0;
    size_t height = 0;
    if(!read_png(png_filename, (void**)&pixel_data, &width, &height) || !pixel_data) {
        return;
    }
    gen_texture_internal(glm::ivec2(width, height), pixel_data, Texture::RGB, smooth);
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
      m_type(Texture::RGB)
{
    size_t width  = 0;
    size_t height = 0;
    unsigned char* pixel_data_pos_x = NULL;
    unsigned char* pixel_data_neg_x = NULL;
    unsigned char* pixel_data_pos_y = NULL;
    unsigned char* pixel_data_neg_y = NULL;
    unsigned char* pixel_data_pos_z = NULL;
    unsigned char* pixel_data_neg_z = NULL;
    if(!read_png(png_filename_pos_x, (void**)&pixel_data_pos_x, &width, &height) || !pixel_data_pos_x) {
        std::cout << "failed to load cube map positive x" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_x, (void**)&pixel_data_neg_x, &width, &height) || !pixel_data_neg_x) {
        std::cout << "failed to load cube map negative x" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_y, (void**)&pixel_data_pos_y, &width, &height) || !pixel_data_pos_y) {
        std::cout << "failed to load cube map positive y" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_y, (void**)&pixel_data_neg_y, &width, &height) || !pixel_data_neg_y) {
        std::cout << "failed to load cube map negative y" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_z, (void**)&pixel_data_pos_z, &width, &height) || !pixel_data_pos_z) {
        std::cout << "failed to load cube map positive z" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_z, (void**)&pixel_data_neg_z, &width, &height) || !pixel_data_neg_z) {
        std::cout << "failed to load cube map negative z" << std::endl;
        return;
    }
    gen_texture_skybox_internal(glm::ivec2(width, height),
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
        if(m_pixel_data_pos_x) { delete[] m_pixel_data_pos_x; }
        if(m_pixel_data_neg_x) { delete[] m_pixel_data_neg_x; }
        if(m_pixel_data_pos_y) { delete[] m_pixel_data_pos_y; }
        if(m_pixel_data_neg_y) { delete[] m_pixel_data_neg_y; }
        if(m_pixel_data_pos_z) { delete[] m_pixel_data_pos_z; }
        if(m_pixel_data_neg_z) { delete[] m_pixel_data_neg_z; }
        return;
    }
    switch(m_type) {
        case Texture::RGB:
            if(m_pixel_data) { delete[] m_pixel_data; }
            break;
        case Texture::DEPTH:
            if(m_pixel_data) { delete[] reinterpret_cast<float*>(m_pixel_data); }
            break;
    }
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

void Texture::gen_texture_internal(glm::ivec2  dim,
                                   const void* pixel_data,
                                   type_t      type,
                                   bool        smooth)
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
    m_dim    = dim;
    m_skybox = false;
    m_type   = type;
    size_t size_buf = get_pixel_data_size();
    m_pixel_data = new unsigned char[size_buf];
    if(!m_pixel_data) {
        return;
    }
    memcpy(m_pixel_data, pixel_data, size_buf);
    update();
}

void Texture::gen_texture_skybox_internal(glm::ivec2  dim,
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
    m_dim    = dim;
    m_skybox = true;
    m_type   = Texture::RGB;
    size_t size_buf = get_pixel_data_size();
    m_pixel_data_pos_x = new unsigned char[size_buf];
    m_pixel_data_neg_x = new unsigned char[size_buf];
    m_pixel_data_pos_y = new unsigned char[size_buf];
    m_pixel_data_neg_y = new unsigned char[size_buf];
    m_pixel_data_pos_z = new unsigned char[size_buf];
    m_pixel_data_neg_z = new unsigned char[size_buf];
    if(!m_pixel_data_pos_x) { return; }
    if(!m_pixel_data_neg_x) { return; }
    if(!m_pixel_data_pos_y) { return; }
    if(!m_pixel_data_neg_y) { return; }
    if(!m_pixel_data_pos_z) { return; }
    if(!m_pixel_data_neg_z) { return; }
    memcpy(m_pixel_data_pos_x, pixel_data_pos_x, size_buf);
    memcpy(m_pixel_data_neg_x, pixel_data_neg_x, size_buf);
    memcpy(m_pixel_data_pos_y, pixel_data_pos_y, size_buf);
    memcpy(m_pixel_data_neg_y, pixel_data_neg_y, size_buf);
    memcpy(m_pixel_data_pos_z, pixel_data_pos_z, size_buf);
    memcpy(m_pixel_data_neg_z, pixel_data_neg_z, size_buf);
    update();
}

size_t Texture::get_pixel_data_size() const
{
    if(m_skybox) {
        return m_dim.x * m_dim.y * sizeof(unsigned char) * 3; // per cube face
    }
    switch(m_type) {
        case Texture::RGB:   return m_dim.x * m_dim.y * sizeof(unsigned char) * 3;
        case Texture::DEPTH: return m_dim.x * m_dim.y * sizeof(float) * 3;
    }
    return 0;
}

void Texture::update()
{
    bind();
    if(m_skybox) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGB,                         // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGB,                         // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_pos_x);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGB,                         // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGB,                         // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_neg_x);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGB,                         // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGB,                         // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_neg_y);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGB,                         // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGB,                         // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_pos_y);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGB,                         // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGB,                         // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_pos_z);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                     0,                              // level, 0 = base, no mipmap,
                     GL_RGB,                         // internal format
                     m_dim.x,                        // width
                     m_dim.y,                        // height
                     0,                              // border, always 0 in OpenGL ES
                     GL_RGB,                         // format
                     GL_UNSIGNED_BYTE,               // type
                     m_pixel_data_neg_z);
        return;
    }
    switch(m_type) {
        case Texture::RGB:
            glTexImage2D(GL_TEXTURE_2D,    // target
                         0,                // level, 0 = base, no mipmap,
                         GL_RGB,           // internal format
                         m_dim.x,          // width
                         m_dim.y,          // height
                         0,                // border, always 0 in OpenGL ES
                         GL_RGB,           // format
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
    }
}

void Texture::refresh()
{
    bind();
    if(m_skybox) {
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGB,                         // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_pos_x);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGB,                         // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_neg_x);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGB,                         // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_neg_y);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGB,                         // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_pos_y);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGB,                         // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_pos_z);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                      0,                              // level, 0 = base, no mipmap,
                      GL_RGB,                         // format
                      GL_UNSIGNED_BYTE,               // type
                      m_pixel_data_neg_z);
        return;
    }
    switch(m_type) {
        case Texture::RGB:
            glGetTexImage(GL_TEXTURE_2D,    // target
                          0,                // level, 0 = base, no mipmap,
                          GL_RGB,           // format
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
    }
}

}
