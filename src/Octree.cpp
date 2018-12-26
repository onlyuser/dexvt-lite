// This file is part of dexvt-lite.
// -- 3D Inverse Kinematics (Cyclic Coordinate Descent) with Constraints
// Copyright (C) 2018 onlyuser <mailto:onlyuser@gmail.com>
//
// dexvt-lite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dexvt-lite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dexvt-lite.  If not, see <http://www.gnu.org/licenses/>.

#include <Octree.h>
#include <TransformObject.h>
#include <BBoxObject.h>
#include <PrimitiveFactory.h>
#include <queue>
#include <map>
#include <set>
#include <sstream>
#include <memory.h>

#define NODE_CAPACITY      5
#define DEPTH_LIMIT        4
#define EARLY_PRUNE_LEVELS 0 // of questionable benefit

namespace vt {

Octree::Octree(glm::vec3 origin,
               glm::vec3 dim,
               int       index,
               int       depth,
               Octree*   parent,
               Octree*   root)
    : m_origin(origin),
      m_dim(dim),
      m_center(origin + dim * 0.5f),
      m_index(index),
      m_depth(depth),
      m_parent(parent),
      m_root(parent ? root : this),
      m_child_count(0)
{
    memset(m_nodes, 0, sizeof(Octree*) * 8);
}

Octree::~Octree()
{
    clear();
}

void Octree::clear()
{
    m_leaf_objects.clear(); // purge leaf contents
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        delete m_nodes[i];
        m_nodes[i] = NULL;
        m_child_count--;
    }
}

void Octree::prune_empty_nodes()
{
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        m_nodes[i]->prune_empty_nodes();
        if(!m_nodes[i]->is_leaf() || m_nodes[i]->get_leaf_object_count()) {
            continue;
        }
        delete m_nodes[i];
        m_nodes[i] = NULL;
        m_child_count--;
    }
}

bool Octree::insert(long id, glm::vec3 pos)
{
    if(is_leaf()) { // if leaf
        if((m_leaf_objects.size() < NODE_CAPACITY || m_depth > DEPTH_LIMIT)) { // if leaf and there's still room or we've reached depth limit
            if(m_leaf_objects.find(id) != m_leaf_objects.end()) { // object already added?
                return false;
            }
            m_leaf_objects.insert(std::pair<long, glm::vec3>(id, pos)); // add object to leaf
            return true;
        }
        // create sub-nodes and copy leaf contents to sub-nodes
        for(std::map<long, glm::vec3>::iterator p = m_leaf_objects.begin(); p != m_leaf_objects.end(); ++p) {
            long      _id  = (*p).first;
            glm::vec3 _pos = (*p).second;
            Octree* node = alloc_octant(_pos);
            if(!node) {
                continue;
            }
            node->insert(_id, _pos);
        }
        m_leaf_objects.clear(); // purge leaf contents
    }
    Octree* node = alloc_octant(pos);
    if(!node || !node->insert(id, pos)) { // add object to including node
        return false;
    }
    return true;
}

bool Octree::remove(long id)
{
    if(is_leaf()) {
        std::map<long, glm::vec3>::iterator p = m_leaf_objects.find(id);
        if(p == m_leaf_objects.end()) {
            return false;
        }
        m_leaf_objects.erase(p); // remove core action
        return true;
    }
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        if(m_nodes[i]->remove(id)) {
            return true;
        }
    }
    return false;
}

int Octree::find(glm::vec3          target,
                 int                k,
                 std::vector<long>* nearest_k_vec,
                 float              radius) const
{
    std::priority_queue<id_dist_t, std::vector<id_dist_t>, id_dist_less_than_t> nearest_k_pq;
    find_hier(target, k, &nearest_k_pq, true, radius);

    // reverse order and copy k elements into more friendly container
    int i = nearest_k_pq.size() - 1;
    while(nearest_k_pq.size()) {
        if(i < k) {
            nearest_k_vec->insert(nearest_k_vec->begin(), nearest_k_pq.top().first);
        }
        nearest_k_pq.pop();
        i--;
    }

    // return actual result size
    return nearest_k_vec->size();
}

void Octree::find_hier(glm::vec3                                                                    target,
                       int                                                                          k,
                       std::priority_queue<id_dist_t, std::vector<id_dist_t>, id_dist_less_than_t>* nearest_k_pq,
                       bool                                                                         is_direct_lineage,
                       float                                                                        radius) const
{
    // apply early prune near root; theoretically efficient, in practice very expensive
    if(m_depth <= EARLY_PRUNE_LEVELS) {
        TransformObject transform_object("", m_origin);
        if(!BBoxObject(glm::vec3(0), m_dim).is_sphere_collide(&transform_object,
                                                              target,
                                                              radius))
        {
            return;
        }
    }

    //==========
    // leaf node
    //==========

    if(is_leaf()) {
        for(std::map<long, glm::vec3>::const_iterator p = m_leaf_objects.begin(); p != m_leaf_objects.end(); ++p) {
            long      id  = (*p).first;
            glm::vec3 pos = (*p).second;
            if(radius > 0 && glm::distance(pos, target) > radius) { // apply radius filter
                continue;
            }
            nearest_k_pq->push(id_dist_t(id, glm::distance(pos, target))); // record ALL visited objects (filtered)
        }
        return;
    }

    //==============
    // internal node
    //==============

    int octant_index = get_octant_index(target);
    if(octant_index == -1) {
        return;
    }

    // search best-candidate octant
    if(m_nodes[octant_index]) {
        m_nodes[octant_index]->find_hier(target, k, nearest_k_pq, is_direct_lineage, radius);
    }

    // stop here if best-candidate octant results sufficient

    // conditions for searching sibling octants:
    // * collecting fewer than k objects from within the best-candidate octant sub-tree
    // * farthest object distance within best-candidate octant exceeds nearest wall distance (automatically true if not direct lineage)

    // consider a quadtree search for k nearest neighbors with k == 2 and query points X, Y, and Z
    // * query point X is closer to the nearest wall than to A in the including quadrant, warranting a search within sibling quadrants
    // * query point Y is closer to C in the including quadrant than to the nearest wall
    //   however, C alone is not enough to satisfy the k == 2 requirement, warranting a search within sibling quadrants
    // * query point Z is closer to D and E in the including quadrant than to the nearest wall, satisfying the k == 2 requirement
    //   therefore, this no longer warrants a search within sibling quadrants
    //
    // NOTE: when searching within sibling quadrants, any found objects are inherently farther from the original query point than the nearest wall
    //       stop here if sibling quadrant results insufficient
    //
    // +---------------+---------------+
    // |               |               |
    // |               |               |
    // |               |               |
    // |               |               |
    // |               |               |
    // |               |               |
    // |               |               |
    // +---------------+-------+-------+
    // |               |       |       |
    // |               |   Z   |   Y   |
    // |               | D   E | C     |
    // |               +-------+-------+
    // |               |       | X     |
    // |               |       |       |
    // |               |       |     A |
    // +---------------+-------+-------+

    // get nearest wall distance
    float nearest_wall_distance = BIG_NUMBER;
    nearest_wall_distance = std::min(nearest_wall_distance, static_cast<float>(fabs(target.x - m_origin.x)));
    nearest_wall_distance = std::min(nearest_wall_distance, static_cast<float>(fabs(target.y - m_origin.y)));
    nearest_wall_distance = std::min(nearest_wall_distance, static_cast<float>(fabs(target.z - m_origin.z)));
    glm::vec3 opposite = m_origin + m_dim;
    nearest_wall_distance = std::min(nearest_wall_distance, static_cast<float>(fabs(target.x - opposite.x)));
    nearest_wall_distance = std::min(nearest_wall_distance, static_cast<float>(fabs(target.y - opposite.y)));
    nearest_wall_distance = std::min(nearest_wall_distance, static_cast<float>(fabs(target.z - opposite.z)));

    float farthest_object_distance = nearest_k_pq->size() ? nearest_k_pq->top().second : 0;
    bool should_search_siblings = !is_direct_lineage || (farthest_object_distance > nearest_wall_distance);
    if(static_cast<int>(nearest_k_pq->size()) >= k && !should_search_siblings) {
        return;
    }

    // search sibling octants
    for(int i = 0; i < 8; i++) {
        if(i == octant_index) { // skip best-candidate octant (already searched)
            continue;
        }
        if(m_nodes[i]) {
            m_nodes[i]->find_hier(target, k, nearest_k_pq, false, radius);
        }
    }
}

bool Octree::exists(long id)
{
    if(is_leaf()) {
        std::map<long, glm::vec3>::iterator p = m_leaf_objects.find(id);
        if(p != m_leaf_objects.end()) {
            return true; // find core action
        }
        return false;
    }
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        if(m_nodes[i]->exists(id)) {
            return true;
        }
    }
    return false;
}

bool Octree::move(long id, glm::vec3 pos)
{
    if(is_leaf()) {
        std::map<long, glm::vec3>::iterator p = m_leaf_objects.find(id);
        if(p != m_leaf_objects.end()) {
            (*p).second = pos; // move core action
            return true;
        }
        return false;
    }
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        if(m_nodes[i]->move(id, pos)) {
            return true;
        }
    }
    return false;
}

bool Octree::rebalance()
{
    bool changed = false;
    if(is_leaf()) {
        std::vector<long> remove_vec;
        for(std::map<long, glm::vec3>::iterator p = m_leaf_objects.begin(); p != m_leaf_objects.end(); ++p) {
            long      id  = (*p).first;
            glm::vec3 pos = (*p).second;
            if(!within_bbox(pos)) {
                remove_vec.push_back(id);
            }
        }
        for(std::vector<long>::iterator q = remove_vec.begin(); q != remove_vec.end(); ++q) {
            long id = *q;
            std::map<long, glm::vec3>::iterator r = m_leaf_objects.find(id);
            if(r == m_leaf_objects.end()) {
                continue;
            }

            // remove from subtree
            if(!remove(id)) {
                continue;
            }

            // add back to first including parent node
            glm::vec3 pos = (*r).second;
            Octree* node = first_including_parent_node(pos);
            if(node) {
                if(!node->insert(id, pos)) {
                    continue;
                }
            }

            changed = true;
        }
        return changed;
    }
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        changed |= m_nodes[i]->rebalance();
    }
    if(is_root()) {
        prune_empty_nodes();
    }
    return changed;
}

std::string Octree::get_name() const
{
    std::stringstream ss;
    ss << (m_parent ? m_parent->get_name() + "." : "");
    if(m_index == -1) {
        ss << "<root>";
    } else {
        ss << m_index;
    }
    return ss.str();
}

void Octree::dump() const
{
    static size_t indent = 0;
    std::string indent_str = std::string(indent, '\t');
    std::cout << indent_str << this << std::endl;
    std::cout << indent_str << "name: "    << get_name()  << std::endl;
    std::cout << indent_str << "depth: "   << get_depth() << std::endl;
    std::cout << indent_str << "is_root: " << is_root()   << std::endl;
    std::cout << indent_str << "is_leaf: " << is_leaf()   << std::endl;
    std::cout << indent_str << "parent: "  << m_parent    << std::endl;
    std::cout << indent_str << "root: "    << m_root      << std::endl;
    std::cout << std::endl;
    indent++;
    for(int i = 0; i < 8; i++) {
        if(!m_nodes[i]) {
            continue;
        }
        m_nodes[i]->dump();
    }
    indent--;
}

Octree* Octree::alloc_octant(glm::vec3 pos)
{
    int octant_index = get_octant_index(pos);
    if(octant_index == -1) {
        return NULL;
    }
    if(!m_nodes[octant_index]) {
        glm::vec3 points[8];
        glm::vec3 half_dim = m_dim * 0.5f;
        vt::PrimitiveFactory::get_box_corners(points, &m_origin, &half_dim);
        m_nodes[octant_index] = new Octree(points[octant_index], half_dim, octant_index, m_depth + 1, this, m_root);
        m_child_count++;
    }
    return m_nodes[octant_index];
}

Octree* Octree::first_including_parent_node(glm::vec3 pos)
{
    if(within_bbox(pos)) {
        return this;
    }
    if(m_parent) {
        return m_parent->first_including_parent_node(pos);
    }
    return NULL;
}

int Octree::get_octant_index(glm::vec3 pos) const
{
    // points
    //
    //     y
    //     4-------7
    //    /|      /|
    //   / |     / |
    //  5-------6  |
    //  |  0----|--3 x
    //  | /     | /
    //  |/      |/
    //  1-------2
    // z

    //if(!within_bbox(pos)) {
    //    return -1;
    //}
    if(pos.y < m_center.y) {
        if(pos.x < m_center.x) {
            if(pos.z < m_center.z) {
                return 0;
            } else {
                return 1;
            }
        } else {
            if(pos.z < m_center.z) {
                return 3;
            } else {
                return 2;
            }
        }
    } else {
        if(pos.x < m_center.x) {
            if(pos.z < m_center.z) {
                return 4;
            } else {
                return 5;
            }
        } else {
            if(pos.z < m_center.z) {
                return 7;
            } else {
                return 6;
            }
        }
    }
    return -1;
}

bool Octree::within_bbox(glm::vec3 pos) const
{
    glm::vec3 min = m_origin;
    glm::vec3 max = m_origin + m_dim;
    return (min.x <= pos.x && pos.x <= max.x) &&
           (min.y <= pos.y && pos.y <= max.y) &&
           (min.z <= pos.z && pos.z <= max.z);
}

}
