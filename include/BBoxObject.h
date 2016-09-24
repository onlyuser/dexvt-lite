#ifndef VT_BBOX_OBJECT_H_
#define VT_BBOX_OBJECT_H_

#include <glm/glm.hpp>

namespace vt {

class BBoxObject
{
public:
    void set_min_max(glm::vec3 min, glm::vec3 max);
    void get_min_max(glm::vec3* min, glm::vec3* max) const;
    glm::vec3 get_center() const;

protected:
    glm::vec3 m_min;
    glm::vec3 m_max;

    BBoxObject();
};

}

#endif
