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

attribute vec2 texcoord;
attribute vec3 vertex_normal;
attribute vec3 vertex_position;
attribute vec3 vertex_tangent;
uniform mat4 model_transform;
uniform mat4 mvp_transform;
uniform mat4 normal_transform;
uniform vec3 camera_pos;
varying mat3 lerp_tbn_transform;
varying vec2 lerp_texcoord;
varying vec3 lerp_camera_vector;
varying vec3 lerp_position_world;

void main(void) {
    vec3 normal = normalize(vec3(normal_transform*vec4(vertex_normal, 0)));
    vec3 tangent = normalize(vec3(normal_transform*vec4(vertex_tangent, 0)));
    vec3 bitangent = normalize(cross(normal, tangent));
    lerp_tbn_transform = mat3(tangent, bitangent, normal);

    vec3 vertex_position_world = vec3(model_transform*vec4(vertex_position, 1));
    lerp_position_world = vertex_position_world;
    lerp_camera_vector = camera_pos - vertex_position_world;

    gl_Position = mvp_transform*vec4(vertex_position, 1);
    lerp_texcoord = texcoord;
}