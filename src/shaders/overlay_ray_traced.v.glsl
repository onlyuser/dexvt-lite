uniform mat4 inv_normal_transform;
uniform mat4 inv_projection_transform;
varying vec3 lerp_vertex_dir_world;
varying vec2 lerp_texcoord;

void main()
{
    lerp_vertex_dir_world = vec3(inv_normal_transform * inv_projection_transform * gl_Vertex);
    lerp_texcoord = (gl_Vertex.xy + vec2(1)) * 0.5; // map from [-1, 1] to [0, 1]

    gl_Position = gl_Vertex;
}
