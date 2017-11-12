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

#ifndef VT_FRAME_BUFFER_H_
#define VT_FRAME_BUFFER_H_

#include <IdentObject.h>
#include <BindableObjectBase.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace vt {

class Texture;
class Camera;

class FrameBuffer : public IdentObject, public BindableObjectBase
{
public:
    FrameBuffer(Texture* texture, Camera* camera);
    virtual ~FrameBuffer();
    void bind();
    void unbind();
    Texture* get_texture() const {
        return m_texture;
    }
    Camera* get_camera() const {
        return m_camera;
    }

private:
    Texture* m_texture;
    Camera* m_camera;
    GLuint m_depthrenderbuffer_id;
};

}

#endif
