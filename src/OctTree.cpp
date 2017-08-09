#include <OctTree.h>
#include "memory.h"

namespace vt {

OctTree::OctTree(glm::vec3 origin,
                 glm::vec3 dim,
                 int max_depth,
                 int depth)
    : m_origin(origin),
      m_dim(dim),
      m_center(origin + dim * 0.5f),
      m_max_depth(max_depth),
      m_depth(depth)
{
    if(depth + 1 == max_depth) {
        memset(m_nodes, 0, sizeof(m_nodes));
        return;
    }
    glm::vec3 node_dim = dim * 0.5f;
    for(int x_index = 0; x_index < 2; x_index++) {
        for(int y_index = 0; y_index < 2; y_index++) {
            for(int z_index = 0; z_index < 2; z_index++) {
                glm::vec3 node_origin = origin + glm::vec3(x_index * node_dim.x,
                                                           y_index * node_dim.y,
                                                           z_index * node_dim.z);
                m_nodes[x_index * 4 + y_index * 2 + z_index] = new OctTree(node_origin,
                                                                           node_dim,
                                                                           max_depth,
                                                                           depth + 1);
            }
        }
    }
}

OctTree::~OctTree()
{
}

}
