#ifndef VT_OCTREE_H_
#define VT_OCTREE_H_

#include <glm/glm.hpp>
#include <vector>
#include <map>

namespace vt {

class OctTree
{
public:
    OctTree(glm::vec3 origin,
            glm::vec3 dim,
            int       max_depth,
            int       depth = 0,
            OctTree*  parent = NULL);
    virtual ~OctTree();
    glm::vec3      get_origin() const        { return m_origin; }
    glm::vec3      get_dim() const           { return m_dim; }
    glm::vec3      get_center() const        { return m_center; }
    int            get_max_depth() const     { return m_max_depth; }
    int            get_depth() const         { return m_depth; }
    OctTree*       get_parent() const        { return m_parent; }
    OctTree*       get_node(int index) const { return m_nodes[index]; }
    bool           encloses(glm::vec3 pos) const;
    const OctTree* downtrace_leaf_enclosing(glm::vec3 pos) const;
    OctTree*       uptrace_parent_enclosing(glm::vec3 pos) const;
    bool           add_object(long id, glm::vec3* pos);
    int            find_k_nearest(int k, glm::vec3 pos, std::vector<long>* k_nearest) const;
    void           update();

private:
    glm::vec3                  m_origin;
    glm::vec3                  m_dim;
    glm::vec3                  m_center;
    int                        m_max_depth;
    int                        m_depth;
    OctTree*                   m_parent;
    OctTree*                   m_nodes[8];
    std::map<long, glm::vec3*> m_objects;
};

}

#endif
