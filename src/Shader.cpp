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

#include <Shader.h>
#include <shader_utils.h>
#include <GL/glew.h>
#include <string>
#include <iostream>
#include <assert.h>

namespace vt {

Shader::Shader(std::string filename, GLenum type)
    : m_filename(filename),
      m_type(type)
{
    std::cout << "Creating shader \"" << filename << "\"" << std::endl;
    m_id = create_shader(filename.c_str(), type);
    assert(m_id);
}

Shader::~Shader()
{
    glDeleteShader(m_id);
}

}
