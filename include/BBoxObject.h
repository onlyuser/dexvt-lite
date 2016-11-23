#ifndef VT_BBOX_OBJECT_H_
#define VT_BBOX_OBJECT_H_

#include <glm/glm.hpp>

namespace vt {

class BBoxObject
{
public:
    enum align_t {
        ALIGN_CENTER,
        ALIGN_X_MIN,
        ALIGN_X_MAX,
        ALIGN_Y_MIN,
        ALIGN_Y_MAX,
        ALIGN_Z_MIN,
        ALIGN_Z_MAX
    };

    void set_min_max(glm::vec3 min, glm::vec3 max);
    void get_min_max(glm::vec3* min, glm::vec3* max) const;
    glm::vec3 get_center(align_t align = ALIGN_CENTER) const;

protected:
    glm::vec3 m_min;
    glm::vec3 m_max;

    BBoxObject();
};

}

#endif
