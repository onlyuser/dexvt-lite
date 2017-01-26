#include <OctTree.h>
#include <XformObject.h>
#include <vector>

namespace vt {

OctTreeNode::OctTreeNode(glm::vec3     origin,
                         glm::vec3     dim,
                         int           depth,
                         OctTreeNode** nodes)
    : m_depth(0),
      m_nodes(NULL)
{
}

OctTreeNode::~OctTreeNode()
{
}

void OctTreeNode::clear()
{
}

bool OctTreeNode::add(XformObject* object)
{
    return false;
}

bool OctTreeNode::remove(XformObject* object)
{
    return false;
}

int OctTreeNode::find_k_nearest(int k, std::vector<XformObject*> &k_nearest_objects)
{
    return 0;
}

OctTree::OctTree(glm::vec3 origin,
                 glm::vec3 dim,
                 int depth)
    : m_depth(0),
      m_root(NULL)
{
}

OctTree::~OctTree()
{
}

void OctTree::clear()
{
}

bool OctTree::add(XformObject* object)
{
    return false;
}

bool OctTree::remove(XformObject* object)
{
    return false;
}

int OctTree::find_k_nearest(int k, std::vector<XformObject*> &k_nearest_objects)
{
    return 0;
}

void OctTree::update()
{
}

}
