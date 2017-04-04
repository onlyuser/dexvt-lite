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
