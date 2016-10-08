attribute vec3 vertex_normal;
attribute vec3 vertex_position;
const float AIR_REFRACTIVE_INDEX = 1.0;
const float WATER_REFRACTIVE_INDEX = 1.333;
uniform mat4 model_xform;
uniform mat4 mvp_xform;
uniform mat4 normal_xform;
uniform vec3 camera_pos;
varying vec3 lerp_reflected_flipped_cubemap_texcoord;
varying vec3 lerp_refracted_flipped_cubemap_texcoord;

void main(void) {
    vec3 vertex_position_world = vec3(model_xform*vec4(vertex_position, 1));
    vec3 normal_world = normalize(vec3(normal_xform*vec4(vertex_normal, 0)));

    vec3 camera_direction = normalize(camera_pos - vertex_position_world);

    vec3 reflected_camera_dir = reflect(-camera_direction, normal_world);
    vec3 refracted_camera_dir = refract(-camera_direction, normal_world, AIR_REFRACTIVE_INDEX/WATER_REFRACTIVE_INDEX);

    lerp_reflected_flipped_cubemap_texcoord = vec3(reflected_camera_dir.x, -reflected_camera_dir.y, reflected_camera_dir.z);
    lerp_refracted_flipped_cubemap_texcoord = vec3(refracted_camera_dir.x, -refracted_camera_dir.y, refracted_camera_dir.z);

    gl_Position = mvp_xform*vec4(vertex_position, 1);
}