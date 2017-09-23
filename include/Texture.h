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
    typedef enum { RGBA, RGB, DEPTH } format_t;

    Texture(std::string          name            = "",
            format_t             internal_format = Texture::RGBA,
            glm::ivec2           dim             = glm::ivec2(DEFAULT_TEXTURE_WIDTH,
                                                              DEFAULT_TEXTURE_HEIGHT),
            format_t             format          = Texture::RGBA,
            const unsigned char* pixel_data      = NULL,
            bool                 smooth          = true);
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
    format_t get_internal_format() const
    {
        return m_internal_format;
    }
    unsigned char* get_pixel_data() const { return m_pixel_data; }
    size_t get_pixel_data_size() const;
    glm::ivec4 get_pixel(glm::ivec2 pos) const;
    void set_pixel(glm::ivec2 pos, glm::ivec4 color);
    void set_solid_color(glm::ivec4 color);
    void randomize();
    void draw_big_x();
    void upload_to_gpu();
    void download_from_gpu();

private:
    bool           m_skybox;
    format_t       m_internal_format;
    unsigned char* m_pixel_data;
    unsigned char* m_pixel_data_pos_x;
    unsigned char* m_pixel_data_neg_x;
    unsigned char* m_pixel_data_pos_y;
    unsigned char* m_pixel_data_neg_y;
    unsigned char* m_pixel_data_pos_z;
    unsigned char* m_pixel_data_neg_z;

    void alloc(glm::ivec2  dim,
               const void* pixel_data,
               format_t    internal_format,
               bool        smooth);
    void alloc(glm::ivec2  dim,
               const void* pixel_data_pos_x,
               const void* pixel_data_neg_x,
               const void* pixel_data_pos_y,
               const void* pixel_data_neg_y,
               const void* pixel_data_pos_z,
               const void* pixel_data_neg_z);
};

}

#endif
