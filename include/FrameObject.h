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

#ifndef VT_FRAME_OBJECT_H_
#define VT_FRAME_OBJECT_H_

#include <glm/glm.hpp>

namespace vt {

template<class T, class T2>
class FrameObject
{
public:
    T get_offset() const
    {
        return m_offset;
    }
    T get_dim() const
    {
        return m_dim;
    }
    T2 get_left() const
    {
        return m_offset.x;
    }
    T2 get_bottom() const
    {
        return m_offset.y;
    }
    T2 get_width() const
    {
        return m_dim.x;
    }
    T2 get_height() const
    {
        return m_dim.y;
    }
    float get_aspect_ratio() const
    {
        return static_cast<float>(m_dim.x) / m_dim.y;
    }
    void resize(T2 left, T2 bottom, T2 width, T2 height)
    {
        m_offset.x = left;
        m_offset.y = bottom;
        m_dim.x = width;
        m_dim.y = height;
    }

protected:
    T m_offset;
    T m_dim;

    FrameObject(
            T offset,
            T dim)
        : m_offset(offset),
          m_dim(dim)
    {
    }
    virtual ~FrameObject()
    {
    }
};

}

#endif
