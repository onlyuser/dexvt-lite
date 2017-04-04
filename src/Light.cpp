#include <Light.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace vt {

Light::Light(std::string name,
             glm::vec3   origin,
             glm::vec3   color)
    : TransformObject(name, origin),
      m_color(color),
      m_enabled(true)
{
}

void Light::update_transform()
{
    m_transform = glm::translate(glm::mat4(1), m_origin);
}

}
