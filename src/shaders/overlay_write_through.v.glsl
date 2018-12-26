varying vec2 lerp_texcoord;

void main()
{
    gl_Position = gl_Vertex;
    lerp_texcoord = (gl_Vertex.xy + vec2(1)) * 0.5; // map from [-1, 1] to [0, 1]
}
