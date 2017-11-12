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

#include <VarAttribute.h>
#include <Buffer.h>
#include <Program.h>
#include <GL/glew.h>

namespace vt {

VarAttribute::VarAttribute(const Program* program, const GLchar* name)
    : m_is_enabled(false)
{
    m_id = glGetAttribLocation(program->id(), name);
}

VarAttribute::~VarAttribute()
{
}

void VarAttribute::enable_vertex_attrib_array() const
{
    glEnableVertexAttribArray(m_id);
}

void VarAttribute::disable_vertex_attrib_array() const
{
    glDisableVertexAttribArray(m_id);
}

void VarAttribute::vertex_attrib_pointer(Buffer*       buffer,
                                         GLint         size,
                                         GLenum        type,
                                         GLboolean     normalized,
                                         GLsizei       stride,
                                         const GLvoid* pointer) const
{
    buffer->bind();
    glVertexAttribPointer(m_id,
                          size,
                          type,
                          normalized,
                          stride,
                          pointer);
}

}
