attribute vec3 vertex_position;
uniform mat4 mvp_transform;

void main(void) {
    gl_Position = mvp_transform*vec4(vertex_position, 1);
}