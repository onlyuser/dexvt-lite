/*
 * Copyright (C) 2010 Josh A. Beam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

const float MAX_DIST = 20;
const float MAX_DIST_SQUARED = MAX_DIST*MAX_DIST;
const int NUM_LIGHTS = 8;
const int SPECULAR_SHARPNESS = 16;
uniform int light_count;
uniform int light_enabled[NUM_LIGHTS];
uniform vec3 ambient_color;
uniform vec3 light_color[NUM_LIGHTS];
uniform vec3 light_pos[NUM_LIGHTS];
varying vec3 lerp_camera_vector;
varying vec3 lerp_normal;
varying vec3 lerp_position_world;

void main(void) {
    vec3 diffuse_sum = vec3(0.0, 0.0, 0.0);
    vec3 specular_sum = vec3(0.0, 0.0, 0.0);

    vec3 camera_direction = normalize(lerp_camera_vector);

    vec3 normal = normalize(lerp_normal);

    for(int i = 0; i < NUM_LIGHTS && i < light_count; i++) {
        if(light_enabled[i] == 0) {
            continue;
        }
        vec3 light_vector = light_pos[i] - lerp_position_world;

        float dist = min(dot(light_vector, light_vector), MAX_DIST_SQUARED)/MAX_DIST_SQUARED;
        float distance_factor = 1.0 - dist;

        vec3 light_direction = normalize(light_vector);
        float diffuse_per_light = dot(normal, light_direction);
        diffuse_sum += light_color[i]*clamp(diffuse_per_light, 0.0, 1.0)*distance_factor;

        vec3 half_angle = normalize(camera_direction + light_direction);
        vec3 specular_color = min(light_color[i] + 0.5, 1.0);
        float specular_per_light = dot(normal, half_angle);
        specular_sum += specular_color*pow(clamp(specular_per_light, 0.0, 1.0), SPECULAR_SHARPNESS)*distance_factor;
    }

    vec4 sample = vec4(1);
    gl_FragColor = vec4(clamp(sample.rgb*(diffuse_sum + ambient_color) + specular_sum, 0.0, 1.0), sample.a);
}