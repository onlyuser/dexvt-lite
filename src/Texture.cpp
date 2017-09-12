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
        size_t n = dim.x * dim.y * sizeof(unsigned char) * 3;
        m_pixel_data = new unsigned char[n];
        memcpy(m_pixel_data, pixel_data, n);
        m_id = gen_texture_internal(dim.x, dim.y, m_pixel_data, type, smooth);
    } else {
        switch(m_type) {
            case Texture::DEPTH:
                {
                    size_t n = dim.x * dim.y * sizeof(float) * 3;
                    m_pixel_data = new unsigned char[n];
                    memset(m_pixel_data, 0, n);
                    for(int i = 0; i < static_cast<int>(std::min(dim.x, dim.y)); i++) {
                        m_pixel_data[i * dim.x + i]         = 1;
                        m_pixel_data[i * dim.x + dim.x - i] = 1;
                    }
                    m_id = gen_texture_internal(dim.x, dim.y, m_pixel_data, type, smooth);
                }
                break;
            case Texture::RGB:
                if(random) {
                    srand(time(NULL));
                    size_t n = dim.x * dim.y * sizeof(unsigned char) * 3;
                    m_pixel_data = new unsigned char[n];
                    memset(m_pixel_data, 0, n);
                    for(int i = 0; i < static_cast<int>(dim.y); i++) {
                        for(int j = 0; j < static_cast<int>(dim.x); j++) {
                            int pixel_offset = (i * dim.x + j) * 3;
                            m_pixel_data[pixel_offset + 0] = rand() % 256;
                            m_pixel_data[pixel_offset + 1] = rand() % 256;
                            m_pixel_data[pixel_offset + 2] = rand() % 256;
                        }
                    }
                    m_id = gen_texture_internal(dim.x, dim.y, m_pixel_data, type, smooth);
                } else {
                    // draw big red 'x'
                    size_t n = dim.x * dim.y * sizeof(unsigned char) * 3;
                    m_pixel_data = new unsigned char[n];
                    memset(m_pixel_data, 0, n);
                    for(int i = 0; i < static_cast<int>(std::min(dim.x, dim.y)); i++) {
                        int pixel_offset_scanline_start = (i * dim.x + i) * 3;
                        int pixel_offset_scanline_end   = (i * dim.x + (dim.x - i)) * 3;
                        m_pixel_data[pixel_offset_scanline_start + 0] = 255;
                        m_pixel_data[pixel_offset_scanline_start + 1] = 0;
                        m_pixel_data[pixel_offset_scanline_start + 2] = 0;
                        m_pixel_data[pixel_offset_scanline_end   + 0] = 255;
                        m_pixel_data[pixel_offset_scanline_end   + 1] = 0;
                        m_pixel_data[pixel_offset_scanline_end   + 2] = 0;
                    }
                    m_id = gen_texture_internal(dim.x, dim.y, m_pixel_data, type, smooth);
                }
                break;
        }
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
    if(!read_png(png_filename, (void**)&pixel_data, &width, &height)) {
        return;
    }
    if(pixel_data) {
        size_t n = width * height * sizeof(unsigned char) * 3;
        m_pixel_data = new unsigned char[n];
        memcpy(m_pixel_data, pixel_data, n);
        m_id = gen_texture_internal(width, height, pixel_data, Texture::RGB, smooth);
        m_dim.x = width;
        m_dim.y = height;
    }
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
    if(!read_png(png_filename_pos_x, (void**)&pixel_data_pos_x, &width, &height)) {
        std::cout << "failed to load cube map positive x" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_x, (void**)&pixel_data_neg_x, &width, &height)) {
        std::cout << "failed to load cube map negative x" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_y, (void**)&pixel_data_pos_y, &width, &height)) {
        std::cout << "failed to load cube map positive y" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_y, (void**)&pixel_data_neg_y, &width, &height)) {
        std::cout << "failed to load cube map negative y" << std::endl;
        return;
    }
    if(!read_png(png_filename_pos_z, (void**)&pixel_data_pos_z, &width, &height)) {
        std::cout << "failed to load cube map positive z" << std::endl;
        return;
    }
    if(!read_png(png_filename_neg_z, (void**)&pixel_data_neg_z, &width, &height)) {
        std::cout << "failed to load cube map negative z" << std::endl;
        return;
    }
    if(pixel_data_pos_x &&
       pixel_data_neg_x &&
       pixel_data_pos_y &&
       pixel_data_neg_y &&
       pixel_data_pos_z &&
       pixel_data_neg_z)
    {
        size_t n = width * height * sizeof(unsigned char) * 3;
        m_pixel_data_pos_x = new unsigned char[n];
        m_pixel_data_neg_x = new unsigned char[n];
        m_pixel_data_pos_y = new unsigned char[n];
        m_pixel_data_neg_y = new unsigned char[n];
        m_pixel_data_pos_z = new unsigned char[n];
        m_pixel_data_neg_z = new unsigned char[n];
        memcpy(m_pixel_data_pos_x, pixel_data_pos_x, n);
        memcpy(m_pixel_data_neg_x, pixel_data_neg_x, n);
        memcpy(m_pixel_data_pos_y, pixel_data_pos_y, n);
        memcpy(m_pixel_data_neg_y, pixel_data_neg_y, n);
        memcpy(m_pixel_data_pos_z, pixel_data_pos_z, n);
        memcpy(m_pixel_data_neg_z, pixel_data_neg_z, n);
        m_id = gen_texture_skybox_internal(width,
                                           height,
                                           m_pixel_data_pos_x,
                                           m_pixel_data_neg_x,
                                           m_pixel_data_pos_y,
                                           m_pixel_data_neg_y,
                                           m_pixel_data_pos_z,
                                           m_pixel_data_neg_z);
        m_dim.x = width;
        m_dim.y = height;
    }
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_id);
    if(m_skybox) {
        if(m_pixel_data_pos_x) { delete[] m_pixel_data_pos_x; }
        if(m_pixel_data_neg_x) { delete[] m_pixel_data_neg_x; }
        if(m_pixel_data_pos_y) { delete[] m_pixel_data_pos_y; }
        if(m_pixel_data_neg_y) { delete[] m_pixel_data_neg_y; }
        if(m_pixel_data_pos_z) { delete[] m_pixel_data_pos_z; }
        if(m_pixel_data_neg_z) { delete[] m_pixel_data_neg_z; }
    } else {
        switch(m_type) {
            case Texture::DEPTH:
                if(m_pixel_data) { delete[] reinterpret_cast<float*>(m_pixel_data); }
                break;
            case Texture::RGB:
                if(m_pixel_data) { delete[] m_pixel_data; }
                break;
        }
    }
}

void Texture::bind()
{
    if(m_skybox) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
}

GLuint Texture::gen_texture_internal(size_t      width,
                                     size_t      height,
                                     const void* pixel_data,
                                     type_t      type,
                                     bool        smooth)
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    switch(type) {
        case Texture::DEPTH:
            glTexImage2D(GL_TEXTURE_2D,      // target
                         0,                  // level, 0 = base, no mipmap,
                         GL_DEPTH_COMPONENT, // internal format
                         width,              // width
                         height,             // height
                         0,                  // border, always 0 in OpenGL ES
                         GL_DEPTH_COMPONENT, // format
                         GL_FLOAT,           // type
                         pixel_data);
            break;
        case Texture::RGB:
            glTexImage2D(GL_TEXTURE_2D,    // target
                         0,                // level, 0 = base, no mipmap,
                         GL_RGB,           // internal format
                         width,            // width
                         height,           // height
                         0,                // border, always 0 in OpenGL ES
                         GL_RGB,           // format
                         GL_UNSIGNED_BYTE, // type
                         pixel_data);
            break;
    }
    return id;
}

GLuint Texture::gen_texture_skybox_internal(size_t      width,
                                            size_t      height,
                                            const void* pixel_data_pos_x,
                                            const void* pixel_data_neg_x,
                                            const void* pixel_data_pos_y,
                                            const void* pixel_data_neg_y,
                                            const void* pixel_data_pos_z,
                                            const void* pixel_data_neg_z)
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, // target
                 0,                              // level, 0 = base, no mipmap,
                 GL_RGB,                         // internal format
                 width,                          // width
                 height,                         // height
                 0,                              // border, always 0 in OpenGL ES
                 GL_RGB,                         // format
                 GL_UNSIGNED_BYTE,               // type
                 pixel_data_pos_x);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // target
                 0,                              // level, 0 = base, no mipmap,
                 GL_RGB,                         // internal format
                 width,                          // width
                 height,                         // height
                 0,                              // border, always 0 in OpenGL ES
                 GL_RGB,                         // format
                 GL_UNSIGNED_BYTE,               // type
                 pixel_data_neg_x);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // target
                 0,                              // level, 0 = base, no mipmap,
                 GL_RGB,                         // internal format
                 width,                          // width
                 height,                         // height
                 0,                              // border, always 0 in OpenGL ES
                 GL_RGB,                         // format
                 GL_UNSIGNED_BYTE,               // type
                 pixel_data_neg_y);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // target
                 0,                              // level, 0 = base, no mipmap,
                 GL_RGB,                         // internal format
                 width,                          // width
                 height,                         // height
                 0,                              // border, always 0 in OpenGL ES
                 GL_RGB,                         // format
                 GL_UNSIGNED_BYTE,               // type
                 pixel_data_pos_y);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // target
                 0,                              // level, 0 = base, no mipmap,
                 GL_RGB,                         // internal format
                 width,                          // width
                 height,                         // height
                 0,                              // border, always 0 in OpenGL ES
                 GL_RGB,                         // format
                 GL_UNSIGNED_BYTE,               // type
                 pixel_data_pos_z);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // target
                 0,                              // level, 0 = base, no mipmap,
                 GL_RGB,                         // internal format
                 width,                          // width
                 height,                         // height
                 0,                              // border, always 0 in OpenGL ES
                 GL_RGB,                         // format
                 GL_UNSIGNED_BYTE,               // type
                 pixel_data_neg_z);
    return id;
}

void Texture::update(unsigned char* pixel_data)
{
    bind();
    switch(m_type) {
        case Texture::DEPTH:
            glTexImage2D(GL_TEXTURE_2D,      // target
                         0,                  // level, 0 = base, no mipmap,
                         GL_DEPTH_COMPONENT, // internal format
                         m_dim.x,            // width
                         m_dim.y,            // height
                         0,                  // border, always 0 in OpenGL ES
                         GL_DEPTH_COMPONENT, // format
                         GL_FLOAT,           // type
                         pixel_data);
            break;
        case Texture::RGB:
            glTexImage2D(GL_TEXTURE_2D,    // target
                         0,                // level, 0 = base, no mipmap,
                         GL_RGB,           // internal format
                         m_dim.x,          // width
                         m_dim.y,          // height
                         0,                // border, always 0 in OpenGL ES
                         GL_RGB,           // format
                         GL_UNSIGNED_BYTE, // type
                         pixel_data);
            break;
    }
}

void Texture::refresh(unsigned char* pixel_data)
{
    bind();
    switch(m_type) {
        case Texture::DEPTH:
            glGetTexImage(GL_TEXTURE_2D,      // target
                          0,                  // level, 0 = base, no mipmap,
                          GL_DEPTH_COMPONENT, // format
                          GL_FLOAT,           // type
                          pixel_data);
            break;
        case Texture::RGB:
            glGetTexImage(GL_TEXTURE_2D,    // target
                          0,                // level, 0 = base, no mipmap,
                          GL_RGB,           // format
                          GL_UNSIGNED_BYTE, // type
                          pixel_data);
            break;
    }
}

}
