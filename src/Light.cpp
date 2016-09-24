#include <Light.h>
#include <NamedObject.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace vt {

Light::Light(
        std::string name,
        glm::vec3   origin,
        glm::vec3   color)
    : NamedObject(name),
      XformObject(origin),
      m_color(color),
      m_enabled(true)
{
}

void Light::update_xform()
{
    m_xform = glm::translate(glm::mat4(1), m_origin);
}

}
