#ifndef VT_OCTREE_H_
#define VT_OCTREE_H_

#include <glm/glm.hpp>
#include <vector>

namespace vt {

class OctTree
{
public:
    OctTree(glm::vec3 origin,
            glm::vec3 dim,
            int max_depth,
            int depth = 0);
    ~OctTree();
    glm::vec3 get_origin() const        { return m_origin; }
    glm::vec3 get_dim() const           { return m_dim; }
    glm::vec3 get_center() const        { return m_center; }
    int       get_max_depth() const     { return m_max_depth; }
    int       get_depth() const         { return m_depth; }
    OctTree*  get_node(int index) const { return m_nodes[index]; }

private:
    glm::vec3 m_origin;
    glm::vec3 m_dim;
    glm::vec3 m_center;
    int       m_max_depth;
    int       m_depth;
    OctTree*  m_nodes[8];
};

}

#endif
