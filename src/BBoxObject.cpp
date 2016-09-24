#include <BBoxObject.h>

namespace vt {

BBoxObject::BBoxObject()
{
}

void BBoxObject::set_min_max(glm::vec3 min, glm::vec3 max)
{
    m_min = min;
    m_max = max;
}

void BBoxObject::get_min_max(glm::vec3* min, glm::vec3* max) const
{
    if(!min || !max) {
        return;
    }
    *min = m_min;
    *max = m_max;
}

glm::vec3 BBoxObject::get_center() const
{
    glm::vec3 center = m_min;
    center += m_max;
    center *= 0.5;
    return center;
}

}
