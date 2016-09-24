const int SAMPLE_DISTANCE = 4;
uniform vec2 viewport_dim;
varying vec2 lerp_sample_offset_unit_size;
varying vec2 lerp_texcoord;

void main(void) {
    gl_Position = gl_Vertex;
    lerp_texcoord = (gl_Vertex.xy + vec2(1))*0.5; // map from [-1,1] to [0,1];
    lerp_sample_offset_unit_size = vec2(1/viewport_dim.x, 1/viewport_dim.y)*SAMPLE_DISTANCE;
}