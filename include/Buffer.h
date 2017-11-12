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

#ifndef VT_BUFFER_H_
#define VT_BUFFER_H_

#include <IdentObject.h>
#include <BindableObjectBase.h>
#include <GL/glew.h>

namespace vt {

class Buffer : public IdentObject, public BindableObjectBase
{
public:
    Buffer(GLenum target, size_t size, void* data);
    virtual ~Buffer();
    void update();
    void bind();
    size_t size() const
    {
        return m_size;
    }

private:
    GLenum m_target;
    size_t m_size;
    void* m_data;
};

}

#endif
