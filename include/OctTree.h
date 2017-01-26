#ifndef VT_OCTREE_H_
#define VT_OCTREE_H_

#include <glm/glm.hpp>
#include <vector>

namespace vt {

class XformObject;
class OctTreeNode
{
public:
    OctTreeNode(glm::vec3     origin,
                glm::vec3     dim,
                int           depth,
                OctTreeNode** nodes);
    ~OctTreeNode();

    void clear();
    bool add(XformObject* object);
    bool remove(XformObject* object);
    int find_k_nearest(int k, std::vector<XformObject*> &k_nearest_objects);

private:
    glm::vec3     m_origin;
    glm::vec3     m_dim;
    int           m_depth;
    OctTreeNode** m_nodes;
};

class OctTree
{
public:
    OctTree(glm::vec3 origin,
            glm::vec3 dim,
            int depth);
    ~OctTree();

    void clear();
    bool add(XformObject* object);
    bool remove(XformObject* object);
    int find_k_nearest(int k, std::vector<XformObject*> &k_nearest_objects);
    void update();

private:
    glm::vec3    m_origin;
    glm::vec3    m_dim;
    int          m_depth;
    OctTreeNode* m_root;
};

}

#endif
