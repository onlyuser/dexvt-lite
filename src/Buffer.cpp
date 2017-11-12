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

#include <Buffer.h>
#include <GL/glew.h>

namespace vt {

Buffer::Buffer(GLenum target, size_t size, void* data)
    : m_target(target),
      m_size(size),
      m_data(data)
{
    glGenBuffers(1, &m_id);
    bind();
    glBufferData(target, size, data, GL_STATIC_DRAW);
}

Buffer::~Buffer()
{
    glDeleteBuffers(1, &m_id);
}

void Buffer::update()
{
    bind();
    glBufferData(m_target, m_size, m_data, GL_DYNAMIC_DRAW);
}

void Buffer::bind()
{
    glBindBuffer(m_target, m_id);
}

}
