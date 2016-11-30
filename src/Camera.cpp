#include <Camera.h>
#include <NamedObject.h>
#include <XformObject.h>
#include <ViewObject.h>
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

Camera::Camera(
        std::string       name,
        glm::vec3         origin,
        glm::vec3         target,
        float             fov,
        glm::vec2         offset,
        glm::vec2         dim,
        float             near_plane,
        float             far_plane,
        glm::vec2         ortho_dim,
        float             zoom,
        projection_mode_t projection_mode)
    : NamedObject(name),
      XformObject(origin),
      ViewObject(offset, dim),
      m_target(target),
      m_fov(fov),
      m_near_plane(near_plane),
      m_far_plane(far_plane),
      m_is_dirty_projection_xform(true),
      m_ortho_dim(ortho_dim),
      m_zoom(zoom),
      m_projection_mode(projection_mode),
      m_frame_buffer(NULL)
{
    mark_dirty_xform();
}

Camera::~Camera()
{
}

void Camera::set_origin(glm::vec3 origin)
{
    m_origin = origin;
    mark_dirty_xform();
}

void Camera::set_orient(glm::vec3 orient)
{
    m_orient = orient;
    m_target = m_origin+orient_to_offset(orient);
    mark_dirty_xform();
}

void Camera::set_target(glm::vec3 target)
{
    m_target = target;
    mark_dirty_xform();
}

const glm::vec3 Camera::get_dir() const
{
    return glm::normalize(m_target - m_origin);
}

void Camera::move(glm::vec3 origin, glm::vec3 target)
{
    m_origin = origin;
    m_target = target;
    mark_dirty_xform();
}

void Camera::orbit(glm::vec3 &orient, float &radius)
{
    if(ORIENT_PITCH(orient) > MAX_PITCH) ORIENT_PITCH(orient) = MAX_PITCH;
    if(ORIENT_PITCH(orient) < MIN_PITCH) ORIENT_PITCH(orient) = MIN_PITCH;
    if(ORIENT_YAW(orient) > 360)         ORIENT_YAW(orient) -= 360;
    if(ORIENT_YAW(orient) < 0)           ORIENT_YAW(orient) += 360;
    if(radius < 0) {
        radius = 0;
    }
    m_origin = m_target+orient_to_offset(orient)*radius;
    mark_dirty_xform();
}

void Camera::set_fov(float fov)
{
    m_fov = fov;
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
}

void Camera::resize(float left, float bottom, float width, float height)
{
    ViewObject<glm::vec2, float>::resize(left, bottom, width, height);
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
}

void Camera::set_near_plane(float near_plane)
{
    m_near_plane = near_plane;
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
}

void Camera::set_far_plane(float far_plane)
{
    m_far_plane = far_plane;
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
}

void Camera::set_projection_mode(projection_mode_t projection_mode)
{
    m_projection_mode = projection_mode;
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
}

void Camera::resize_ortho_viewport(float width, float height)
{
    m_ortho_dim.x = width;
    m_ortho_dim.y = height;
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
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
    m_is_dirty_projection_xform = true;
    mark_dirty_xform();
}

void Camera::set_frame_buffer(FrameBuffer* frame_buffer)
{
    m_frame_buffer = frame_buffer;
}

const glm::mat4 &Camera::get_projection_xform()
{
    if(m_is_dirty_projection_xform) {
        update_projection_xform();
        m_is_dirty_projection_xform = false;
    }
    return m_projection_xform;
}

void Camera::update_projection_xform()
{
    if(m_projection_mode == PROJECTION_MODE_PERSPECTIVE) {
        m_projection_xform = glm::perspective(m_fov, static_cast<float>(m_dim.x)/m_dim.y, m_near_plane, m_far_plane);
    } else if(m_projection_mode == PROJECTION_MODE_ORTHO) {
        float aspect_ratio = static_cast<float>(m_dim.x)/m_dim.y;
        float half_width  = m_ortho_dim.x*0.5*m_zoom;
        float half_height = m_ortho_dim.x*0.5*m_zoom;
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
        m_projection_xform = glm::ortho(left, right, bottom, top, m_near_plane, m_far_plane);
    }
}

void Camera::update_xform()
{
    static glm::vec3 up = glm::vec3(0, 1, 0);
    m_xform = glm::lookAt(m_origin, m_target, up);
    m_orient = offset_to_orient(m_target-m_origin);
}

}
