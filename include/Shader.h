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

#ifndef VT_SHADER_H_
#define VT_SHADER_H_

#include <IdentObject.h>
#include <GL/glew.h>
#include <string>

namespace vt {

class Shader : public IdentObject
{
public:
    Shader(std::string filename, GLenum type);
    virtual ~Shader();
    std::string get_filename() const
    {
        return m_filename;
    }
    GLenum type() const
    {
        return m_type;
    }

private:
    std::string m_filename;
    GLenum m_type;
};

}

#endif
