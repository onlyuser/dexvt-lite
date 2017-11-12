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

#include <Camera.h>
#include <TransformObject.h>
#include <FrameObject.h>
#include <Util.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <algorithm>

#define MAX_PITCH  89.999
#define MIN_PITCH -89.999

#define MIN_ORTHO_SCALE 1

namespace vt {

Camera::Camera(std::string       name,
               glm::vec3         origin,
               glm::vec3         target,
               float             fov,
               glm::ivec2        offset,
               glm::ivec2        dim,
               float             near_plane,
               float             far_plane,
               glm::vec2         ortho_dim,
               float             zoom,
               projection_mode_t projection_mode)
    : TransformObject(name, origin),
      FrameObject(offset, dim),
      m_target(target),
      m_fov(fov),
      m_near_plane(near_plane),
      m_far_plane(far_plane),
      m_is_dirty_projection_transform(true),
      m_ortho_dim(ortho_dim),
      m_zoom(zoom),
      m_projection_mode(projection_mode),
      m_frame_buffer(NULL)
{
    mark_dirty_transform();
}

Camera::~Camera()
{
}

void Camera::set_origin(glm::vec3 origin)
{
    m_origin = origin;
    m_euler  = offset_to_euler(m_target - m_origin);
    mark_dirty_transform();
}

void Camera::set_euler(glm::vec3 euler)
{
    m_euler  = euler;
    m_target = m_origin + euler_to_offset(euler);
    mark_dirty_transform();
}

void Camera::set_target(glm::vec3 target)
{
    m_target = target;
    m_euler  = offset_to_euler(m_target - m_origin);
    mark_dirty_transform();
}

const glm::vec3 Camera::get_dir() const
{
    return glm::normalize(m_target - m_origin);
}

void Camera::move(glm::vec3 origin, glm::vec3 target)
{
    m_origin = origin;
    m_target = target;
    m_euler  = offset_to_euler(m_target - m_origin);
    mark_dirty_transform();
}

void Camera::orbit(glm::vec3 &euler, float &radius)
{
    if(EULER_PITCH(euler) > MAX_PITCH) {
        EULER_PITCH(euler) = MAX_PITCH;
    }
    if(EULER_PITCH(euler) < MIN_PITCH) {
        EULER_PITCH(euler) = MIN_PITCH;
    }
    if(EULER_YAW(euler) > 180) {
        EULER_YAW(euler) -= 360;
    }
    if(EULER_YAW(euler) < -180) {
        EULER_YAW(euler) += 360;
    }
    if(radius < 0) {
        radius = 0;
    }
    m_euler  = euler;
    m_origin = m_target + euler_to_offset(euler) * radius;
    mark_dirty_transform();
}

void Camera::set_fov(float fov)
{
    m_fov = fov;
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::resize(int left, int bottom, int width, int height)
{
    FrameObject<glm::ivec2, int>::resize(left, bottom, width, height);
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::set_near_plane(float near_plane)
{
    m_near_plane = near_plane;
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::set_far_plane(float far_plane)
{
    m_far_plane = far_plane;
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::set_projection_mode(projection_mode_t projection_mode)
{
    m_projection_mode = projection_mode;
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::resize_ortho_viewport(float width, float height)
{
    m_ortho_dim.x = width;
    m_ortho_dim.y = height;
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::set_zoom(float *zoom)
{
    if(!zoom) {
        return;
    }
    if(*zoom < MIN_ORTHO_SCALE) {
        *zoom = MIN_ORTHO_SCALE;
    }
    m_zoom = *zoom;
    m_is_dirty_projection_transform = true;
    mark_dirty_transform();
}

void Camera::set_frame_buffer(FrameBuffer* frame_buffer)
{
    m_frame_buffer = frame_buffer;
}

void Camera::set_image_res(glm::ivec2 image_res)
{
    m_image_res = image_res;
}

const glm::mat4 &Camera::get_projection_transform()
{
    if(m_is_dirty_projection_transform) {
        update_projection_transform();
        m_is_dirty_projection_transform = false;
    }
    return m_projection_transform;
}

void Camera::update_projection_transform()
{
    if(m_projection_mode == PROJECTION_MODE_PERSPECTIVE) {
        m_projection_transform = glm::perspective(m_fov, static_cast<float>(m_dim.x) / m_dim.y, m_near_plane, m_far_plane);
    } else if(m_projection_mode == PROJECTION_MODE_ORTHO) {
        float aspect_ratio = get_aspect_ratio();
        float half_width  = m_ortho_dim.x * 0.5 * m_zoom;
        float half_height = m_ortho_dim.x * 0.5 * m_zoom;
        if(m_dim.y < m_dim.x) {
            half_width *= aspect_ratio;
        }
        if(m_dim.x < m_dim.y) {
            half_height /= aspect_ratio;
        }
        float left   = -half_width;
        float right  =  half_width;
        float bottom = -half_height;
        float top    =  half_height;
        m_projection_transform = glm::ortho(left, right, bottom, top, m_near_plane, m_far_plane);
    }
}

void Camera::update_transform()
{
    glm::vec3 up_direction;
    euler_to_offset(m_euler, &up_direction);
    m_transform = glm::lookAt(m_origin, m_target, up_direction);
}

}
