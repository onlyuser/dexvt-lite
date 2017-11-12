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

#include <FrameBuffer.h>
#include <Texture.h>
#include <Camera.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>
#include <assert.h>

namespace vt {

FrameBuffer::FrameBuffer(Texture* texture, Camera* camera)
    : m_texture(texture),
      m_camera(camera),
      m_depthrenderbuffer_id(0)
{
    glGenFramebuffers(1, &m_id);
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);

    glGenRenderbuffers(1, &m_depthrenderbuffer_id);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthrenderbuffer_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_texture->get_width(), m_texture->get_height());
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if(texture->get_internal_format() == Texture::DEPTH) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture->id(), 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture->id(), 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthrenderbuffer_id);
    }

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "failed to generate frame buffer" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
    glDeleteFramebuffers(1, &m_id);
}

void FrameBuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);
    assert(!m_camera->get_frame_buffer());
    m_camera->set_frame_buffer(this);
    glViewport(m_texture->get_left(), m_texture->get_bottom(), m_texture->get_width(), m_texture->get_height());
}

void FrameBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    assert(m_camera->get_frame_buffer());
    m_camera->set_frame_buffer(NULL);
    glViewport(m_camera->get_left(), m_camera->get_bottom(), m_camera->get_width(), m_camera->get_height());
}

}
