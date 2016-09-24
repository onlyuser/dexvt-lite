attribute vec3 vertex_position;
uniform mat4 mvp_xform;

void main(void) {
    gl_Position = mvp_xform*vec4(vertex_position, 1);
}