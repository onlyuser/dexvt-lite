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

#ifndef VT_LIGHT_H_
#define VT_LIGHT_H_

#include <TransformObject.h>
#include <glm/glm.hpp>
#include <string>

namespace vt {

class Light : public TransformObject
{
public:
    Light(std::string name   = "",
          glm::vec3   origin = glm::vec3(0),
          glm::vec3   color  = glm::vec3(1));

    const glm::vec3 get_color() const
    {
        return m_color;
    }

    bool is_enabled() const
    {
        return m_enabled;
    }
    void set_enabled(bool enabled)
    {
        m_enabled = enabled;
    }

private:
    glm::vec3 m_color;
    bool      m_enabled;

    void update_transform();
};

}

#endif
