uniform sampler2D color_texture;
uniform sampler2D color_texture2;
varying vec2 lerp_texcoord;

void main(void) {
    gl_FragColor = max(texture2D(color_texture, lerp_texcoord), texture2D(color_texture2, lerp_texcoord));
}