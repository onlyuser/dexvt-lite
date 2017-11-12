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

#ifndef VT_CAMERA_H_
#define VT_CAMERA_H_

#include <TransformObject.h>
#include <FrameObject.h>
#include <string>
#include <glm/glm.hpp>

#define DEFAULT_VIEWPORT_WIDTH        800
#define DEFAULT_VIEWPORT_HEIGHT       600
#define DEFAULT_ORTHO_VIEWPORT_WIDTH  1
#define DEFAULT_ORTHO_VIEWPORT_HEIGHT 1
#define DEFAULT_FOV                   45
#define DEFAULT_NEAR_PLANE            1
#define DEFAULT_FAR_PLANE             100
#define DEFAULT_ZOOM                  1

namespace vt {

class FrameBuffer;

class Camera : public TransformObject,
               public FrameObject<glm::ivec2, int>
{
public:
    enum projection_mode_t {
        PROJECTION_MODE_PERSPECTIVE,
        PROJECTION_MODE_ORTHO
    };

    Camera(std::string       name            = "",
           glm::vec3         origin          = glm::vec3(0),
           glm::vec3         target          = glm::vec3(-1),
           float             fov             = DEFAULT_FOV,
           glm::ivec2        offset          = glm::ivec2(0),
           glm::ivec2        dim             = glm::ivec2(DEFAULT_VIEWPORT_WIDTH,
                                                          DEFAULT_VIEWPORT_HEIGHT),
           float             near_plane      = DEFAULT_NEAR_PLANE,
           float             far_plane       = DEFAULT_FAR_PLANE,
           glm::vec2         ortho_dim       = glm::vec2(DEFAULT_ORTHO_VIEWPORT_WIDTH,
                                                         DEFAULT_ORTHO_VIEWPORT_HEIGHT),
           float             zoom            = DEFAULT_ZOOM,
           projection_mode_t projection_mode = PROJECTION_MODE_PERSPECTIVE);
    virtual ~Camera();

    void set_origin(glm::vec3 origin);
    void set_euler(glm::vec3 euler);

    const glm::vec3 get_target() const
    {
        return m_target;
    }
    void set_target(glm::vec3 target);

    const glm::vec3 get_dir() const;

    void move(glm::vec3 origin, glm::vec3 target = glm::vec3(0));
    void orbit(glm::vec3 &euler, float &radius);

    float get_fov() const
    {
        return m_fov;
    }
    void set_fov(float fov);

    void resize(int left, int bottom, int width, int height);

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

    const glm::ivec2 get_image_res() const
    {
        return m_image_res;
    }
    void set_image_res(glm::ivec2 image_res);

    const glm::mat4 &get_projection_transform();

private:
    std::string       m_name;
    glm::vec3         m_target;
    float             m_fov;
    float             m_near_plane;
    float             m_far_plane;
    glm::mat4         m_projection_transform;
    bool              m_is_dirty_projection_transform;
    glm::vec2         m_ortho_dim;
    float             m_zoom;
    projection_mode_t m_projection_mode;
    FrameBuffer*      m_frame_buffer;
    glm::ivec2        m_image_res;

    void update_projection_transform();
    void update_transform();
};

}

#endif
