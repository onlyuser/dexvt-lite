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

#include <PRM.h>
#include <Util.h>
#include <vector>
#include <set>
#include <tuple>
#include <glm/glm.hpp>
#include <math.h>

namespace vt {

PRM_Waypoint::PRM_Waypoint(glm::vec3 origin)
    : m_origin(origin)
{
}

void PRM_Waypoint::connect(int neighbor_index, float dist)
{
    m_neighbor_indices.insert(std::pair<int, float>(neighbor_index, dist));
}

void PRM_Waypoint::disconnect(int neighbor_index)
{
    m_neighbor_indices.erase(neighbor_index);
}

PRM::PRM(Octree* octree)
    : m_octree(octree)
{
}

PRM::~PRM()
{
}

void PRM::randomize_waypoints(size_t n)
{
    m_octree->clear();
    m_waypoints.clear();
    glm::vec3 scatter_min = m_octree->get_origin();
    glm::vec3 scatter_max = m_octree->get_origin() + m_octree->get_dim();
    for(int i = 0; i < static_cast<int>(n); i++) {
        glm::vec3 rand_vec(static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX,
                           static_cast<float>(rand()) / RAND_MAX);
        glm::vec3 origin = MIX(scatter_min, scatter_max, rand_vec);
        m_octree->insert(i, origin);
        PRM_Waypoint* waypoint = new PRM_Waypoint(origin);
        m_waypoints.push_back(waypoint);
    }
}

void PRM::connect_waypoints(int k, float radius)
{
    m_edges.clear();
    std::set<long> unique_edges;
    long index = 0;
    for(std::vector<PRM_Waypoint*>::iterator p = m_waypoints.begin(); p != m_waypoints.end(); ++p) {
        vt::PRM_Waypoint* self_object     = *p;
        glm::vec3         self_object_pos = self_object->get_origin();
        std::vector<long> nearest_k_indices;
        if(m_octree->find(self_object_pos,
                          k,
                          &nearest_k_indices,
                          radius))
        {
            glm::vec3 group_centroid(0);
            for(std::vector<long>::iterator q = nearest_k_indices.begin(); q != nearest_k_indices.end(); ++q) {
                if(*q == index) { // ignore self
                    continue;
                }
                int min_index = std::min(index, *q);
                int max_index = std::max(index, *q);
                unique_edges.insert(MAKELONG(min_index, max_index));
            }
        }
        index++;
    }
    for(std::set<long>::iterator r = unique_edges.begin(); r != unique_edges.end(); ++r) {
        int min_index = LOWORD(*r);
        int max_index = HIWORD(*r);
        glm::vec3 p1 = m_waypoints[min_index]->get_origin();
        glm::vec3 p2 = m_waypoints[max_index]->get_origin();
        float dist = glm::distance(p1, p2);
        m_edges.push_back(std::make_tuple(min_index, max_index, dist));
        m_waypoints[min_index]->connect(max_index, dist);
        m_waypoints[max_index]->connect(min_index, dist);
    }
}

int PRM::find_nearest_waypoint(glm::vec3 pos) const
{
    std::vector<long> nearest_k_indices;
    if(!m_octree->find(pos,
                       1,
                       &nearest_k_indices,
                       m_octree->get_dim().x))
    {
        return -1;
    }
    return nearest_k_indices[0];
}

bool PRM::find_shortest_path(glm::vec3 start_pos, glm::vec3 finish_pos, std::vector<int>* path)
{
    typedef enum { PRM_COST,
                   PRM_FROM } export_edge_attr_t;
    if(!path) {
        return false;
    }
    int start_index  = find_nearest_waypoint(start_pos);
    int finish_index = find_nearest_waypoint(finish_pos);
    std::vector<std::tuple<float, int>> best_cost;
    size_t n = m_waypoints.size();
    best_cost.reserve(n);
    for(int i = 0; i < static_cast<int>(n); i++) {
        best_cost[i] = std::make_tuple(BIG_NUMBER, -1);
    }
    std::set<int> frontier;
    std::set<int> visited;
    frontier.insert(start_index);
    visited.insert(start_index);
    best_cost[start_index] = std::make_tuple(0, -1);
    while(frontier.size()) {
        std::set<int> new_frontier;
        for(std::set<int>::iterator p = frontier.begin(); p != frontier.end(); ++p) {
            int self_index = *p;
            const std::map<int, float> &neighbor_indices = m_waypoints[*p]->get_connected();
            for(std::map<int, float>::const_iterator q = neighbor_indices.begin(); q != neighbor_indices.end(); ++q) {
                int other_index = (*q).first;
                if(visited.find(other_index) != visited.end()) {
                    continue;
                }
                float self_cost      = std::get<PRM_COST>(best_cost[self_index]);
                float other_cost     = std::get<PRM_COST>(best_cost[other_index]);
                float edge_cost      = (*q).second;
                float new_route_cost = self_cost + edge_cost;
                if(new_route_cost < other_cost) {
                    best_cost[other_index] = std::make_tuple(new_route_cost, self_index);
                }
                new_frontier.insert(other_index);
            }
        }
        visited.insert(new_frontier.begin(), new_frontier.end());
        frontier = new_frontier;
    }
    int current_index = -1;
    for(current_index = finish_index; current_index != start_index; current_index = std::get<PRM_FROM>(best_cost[current_index])) {
        if(current_index == -1) {
            return false;
        }
        path->insert(path->begin(), current_index);
    }
    path->insert(path->begin(), current_index);
    return true;
}

void PRM::prune_edges()
{
    for(int i = m_edges.size() - 1; i >= 0; i--) {
        int p1_index = std::get<EXPORT_EDGE_P1>(m_edges[i]);
        int p2_index = std::get<EXPORT_EDGE_P2>(m_edges[i]);
        glm::vec3 p1 = m_waypoints[p1_index]->get_origin();
        glm::vec3 p2 = m_waypoints[p2_index]->get_origin();
        float dist = glm::distance(p1, p2);
        if(dist < EPSILON) {
            continue;
        }
        glm::vec3 dir = glm::normalize(p2 - p1);
        bool is_collide = false;
        for(std::vector<Mesh*>::iterator q = m_obstacles.begin(); q != m_obstacles.end(); ++q) {
            if((*q)->is_ray_intersect(*q, p1, dir)) {
                is_collide = true;
                break;
            }
        }
        if(is_collide) {
            m_edges.erase(m_edges.begin() + i);
            m_waypoints[p1_index]->disconnect(p2_index);
            m_waypoints[p2_index]->disconnect(p1_index);
        }
    }
}

bool PRM::export_waypoints(std::vector<glm::vec3>* waypoint_values) const
{
    if(!waypoint_values) {
        return false;
    }
    for(std::vector<PRM_Waypoint*>::const_iterator p = m_waypoints.begin(); p != m_waypoints.end(); ++p) {
        if(!*p) {
            continue;
        }
        waypoint_values->push_back((*p)->get_origin());
    }
    return true;
}

bool PRM::export_edges(std::vector<std::tuple<int, int, float>>* edges) const
{
    if(!edges) {
        return false;
    }
    *edges = m_edges;
    return true;
}

void PRM::add_obstacle(Mesh* obstacle)
{
    m_obstacles.push_back(obstacle);
}

PRM_Waypoint* PRM::at(int index) const
{
    if(index < 0) {
        return NULL;
    }
    return m_waypoints[index];
}

void PRM::clear()
{
    m_octree->clear();
    for(std::vector<PRM_Waypoint*>::iterator p = m_waypoints.begin(); p != m_waypoints.end(); ++p) {
        delete *p;
    }
    m_waypoints.clear();
    m_edges.clear();
    m_obstacles.clear();
}

}
