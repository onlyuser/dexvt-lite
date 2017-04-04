uniform mat4 inv_normal_transform;
uniform mat4 inv_projection_transform;
varying vec3 lerp_texcoord;

void main() {
    lerp_texcoord = vec3(inv_normal_transform*inv_projection_transform*gl_Vertex);
    gl_Position = gl_Vertex;
}