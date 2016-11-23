#include <BBoxObject.h>
#include <glm/glm.hpp>

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

glm::vec3 BBoxObject::get_center(align_t align) const
{
    glm::vec3 center = (m_min + m_max) * 0.5f;
    glm::vec3 half_dim = (m_max - m_min) * 0.5f;
    if(align == ALIGN_CENTER) {
        return center;
    }
    switch(align) {
        case ALIGN_X_MIN:
            center.x -= half_dim.x;
            break;
        case ALIGN_X_MAX:
            center.x += half_dim.x;
            break;
        case ALIGN_Y_MIN:
            center.y -= half_dim.y;
            break;
        case ALIGN_Y_MAX:
            center.y += half_dim.y;
            break;
        case ALIGN_Z_MIN:
            center.z -= half_dim.z;
            break;
        case ALIGN_Z_MAX:
            center.z += half_dim.z;
            break;
        default:
            break;
    }
    return center;
}

}
