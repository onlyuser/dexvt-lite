uniform samplerCube env_map_texture;
varying vec3 lerp_texcoord;

void main() {
    vec3 flipped_cubemap_texcoord = vec3(lerp_texcoord.x, -lerp_texcoord.y, lerp_texcoord.z);
    gl_FragColor = textureCube(env_map_texture, flipped_cubemap_texcoord);
}