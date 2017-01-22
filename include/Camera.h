#ifndef VT_CAMERA_H_
#define VT_CAMERA_H_

#include <XformObject.h>
#include <ViewObject.h>
#include <string>
#include <glm/glm.hpp>

#define DEFAULT_VIEWPORT_WIDTH        800
#define DEFAULT_VIEWPORT_HEIGHT       600
#define DEFAULT_ORTHO_VIEWPORT_WIDTH  1
#define DEFAULT_ORTHO_VIEWPORT_HEIGHT 1

namespace vt {

class FrameBuffer;

class Camera : public XformObject,
               public ViewObject<glm::vec2, float>
{
public:
    enum projection_mode_t {
        PROJECTION_MODE_PERSPECTIVE,
        PROJECTION_MODE_ORTHO
    };

    Camera(std::string       name            = "",
           glm::vec3         origin          = glm::vec3(0),
           glm::vec3         target          = glm::vec3(-1),
           float             fov             = 45,
           glm::vec2         offset          = glm::vec2(0, 0),
           glm::vec2         dim             = glm::vec2(DEFAULT_VIEWPORT_WIDTH,
                                                         DEFAULT_VIEWPORT_HEIGHT),
           float             near_plane      = 4,
           float             far_plane       = 16,
           glm::vec2         ortho_dim       = glm::vec2(DEFAULT_ORTHO_VIEWPORT_WIDTH,
                                                         DEFAULT_ORTHO_VIEWPORT_HEIGHT),
           float             zoom            = 1,
           projection_mode_t projection_mode = PROJECTION_MODE_PERSPECTIVE);
    virtual ~Camera();

    void set_origin(glm::vec3 origin);
    void set_orient(glm::vec3 orient);

    const glm::vec3 get_target() const
    {
        return m_target;
    }
    void set_target(glm::vec3 target);

    const glm::vec3 get_dir() const;

    void move(glm::vec3 origin, glm::vec3 target = glm::vec3(0));
    void orbit(glm::vec3 &orient, float &radius);

    float get_fov() const
    {
        return m_fov;
    }
    void set_fov(float fov);

    void resize(float left, float bottom, float width, float height);

    float get_near_plane() const
    {
        return m_near_plane;
    }
    void set_near_plane(float near_plane);

    float get_far_plane() const
    {
        return m_far_plane;
    }
    void set_far_plane(float far_plane);

    projection_mode_t get_projection_mode() const
    {
        return m_projection_mode;
    }
    void set_projection_mode(projection_mode_t projection_mode);

    float get_ortho_width() const
    {
        return m_ortho_dim.x;
    }
    float get_ortho_height() const
    {
        return m_ortho_dim.y;
    }
    void resize_ortho_viewport(float width, float height);

    float get_zoom() const
    {
        return m_zoom;
    }
    void set_zoom(float *zoom);

    FrameBuffer* get_frame_buffer() const
    {
        return m_frame_buffer;
    }
    void set_frame_buffer(FrameBuffer* frame_buffer);

    const glm::mat4 &get_projection_xform();

private:
    std::string       m_name;
    glm::vec3         m_target;
    float             m_fov;
    float             m_near_plane;
    float             m_far_plane;
    glm::mat4         m_projection_xform;
    bool              m_is_dirty_projection_xform;
    glm::vec2         m_ortho_dim;
    float             m_zoom;
    projection_mode_t m_projection_mode;
    FrameBuffer*      m_frame_buffer;

    void update_projection_xform();
    void update_xform();
};

}

#endif
