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

#ifndef VT_OCTREE_H_
#define VT_OCTREE_H_

#include <glm/glm.hpp>
#include <Util.h>
#include <queue>
#include <map>
#include <set>

namespace vt {

typedef std::pair<long, float> id_dist_t;

struct id_dist_less_than_t
{
    bool operator()(const id_dist_t& a, const id_dist_t& b) const
    {
        return a.second < b.second;
    }
};

class Octree
{
public:
    Octree(glm::vec3 origin,
           glm::vec3 dim,
           int       index  = -1,
           int       depth  = 0,
           Octree*   parent = NULL,
           Octree*   root   = NULL);
    virtual ~Octree();
    void clear();
    void prune_empty_nodes();

    glm::vec3 get_origin() const            { return m_origin; }
    glm::vec3 get_dim() const               { return m_dim; }
    int       get_index() const             { return m_index; }
    int       get_depth() const             { return m_depth; }
    Octree*   get_node(int index) const     { return m_nodes[index]; }
    Octree*   get_parent() const            { return m_parent; }
    Octree*   get_root() const              { return m_root; }
    int       get_child_count() const       { return m_child_count; }
    bool      is_leaf() const               { return !m_child_count; }
    bool      is_root() const               { return !m_parent; }
    size_t    get_leaf_object_count() const { return m_leaf_objects.size(); }

    bool insert(long id, glm::vec3 pos);
    bool remove(long id);
    int find(glm::vec3          target,
             int                k,
             std::vector<long>* nearest_k_vec,
             float              radius = -1) const;
    bool exists(long id);
    bool move(long id, glm::vec3 pos);
    bool rebalance();

    std::string get_name() const;
    void dump() const;

private:
    void find_hier(glm::vec3                                                                    target,
                   int                                                                          k,
                   std::priority_queue<id_dist_t, std::vector<id_dist_t>, id_dist_less_than_t>* nearest_k_pq,
                   bool                                                                         is_direct_lineage,
                   float                                                                        radius) const;
    Octree* alloc_octant(glm::vec3 pos);
    Octree* first_including_parent_node(glm::vec3 pos);
    int get_octant_index(glm::vec3 pos) const;
    bool within_bbox(glm::vec3 pos) const;

    glm::vec3                 m_origin;
    glm::vec3                 m_dim;
    glm::vec3                 m_center;
    int                       m_index;
    int                       m_depth;
    Octree*                   m_nodes[8];
    Octree*                   m_parent;
    Octree*                   m_root;
    int                       m_child_count;
    std::map<long, glm::vec3> m_leaf_objects;
};

}

#endif
