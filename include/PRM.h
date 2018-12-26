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

#ifndef VT_PRM_H_
#define VT_PRM_H_

#include <Octree.h>
#include <Mesh.h>
#include <tuple>
#include <glm/glm.hpp>

namespace vt {

class PRM_Waypoint
{
public:
    PRM_Waypoint(glm::vec3 origin);
    const glm::vec3 &get_origin() const { return m_origin; }
    void set_origin(glm::vec3 origin)   { m_origin = origin; }
    void connect(int neighbor_index, float dist);
    void disconnect(int neighbor_index);
    const std::map<int, float>& get_connected() const { return m_neighbor_indices; }

private:
    glm::vec3            m_origin;
    std::map<int, float> m_neighbor_indices;
};

class PRM
{
public:
    typedef enum { EXPORT_EDGE_P1,
                   EXPORT_EDGE_P2,
                   EXPORT_EDGE_COST } export_edge_attr_t;

    PRM(Octree* octree);
    ~PRM();
    void randomize_waypoints(size_t n);
    void connect_waypoints(int k, float radius);
    int find_nearest_waypoint(glm::vec3 pos) const;
    bool find_shortest_path(glm::vec3 start_pos, glm::vec3 finish_pos, std::vector<int>* path);
    void prune_edges();
    bool export_waypoints(std::vector<glm::vec3>* waypoint_values) const;
    bool export_edges(std::vector<std::tuple<int, int, float>>* edges) const;
    void add_obstacle(Mesh* obstacle);
    PRM_Waypoint* at(int index) const;
    void clear();

private:
    Octree*                                  m_octree;
    std::vector<PRM_Waypoint*>               m_waypoints;
    std::vector<std::tuple<int, int, float>> m_edges;
    std::vector<Mesh*>                       m_obstacles;
};

}

#endif
