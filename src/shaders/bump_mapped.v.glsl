attribute vec2 texcoord;
attribute vec3 vertex_normal;
attribute vec3 vertex_position;
attribute vec3 vertex_tangent;
uniform mat4 model_xform;
uniform mat4 mvp_xform;
uniform mat4 normal_xform;
uniform vec3 camera_pos;
varying mat3 lerp_tbn_transform;
varying vec2 lerp_texcoord;
varying vec3 lerp_camera_vector;
varying vec3 lerp_position_world;

void main(void) {
    vec3 normal = normalize(vec3(normal_xform*vec4(vertex_normal, 0)));
    vec3 tangent = normalize(vec3(normal_xform*vec4(vertex_tangent, 0)));
    vec3 bitangent = normalize(cross(normal, tangent));
    lerp_tbn_transform = mat3(tangent, bitangent, normal);

    vec3 vertex_position_world = vec3(model_xform*vec4(vertex_position, 1));
    lerp_camera_vector = camera_pos - vertex_position_world;

    gl_Position = mvp_xform*vec4(vertex_position, 1);
    lerp_position_world = gl_Position.xyz;
    lerp_texcoord = texcoord;
}