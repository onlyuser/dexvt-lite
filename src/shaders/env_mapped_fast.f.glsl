uniform float reflect_to_refract_ratio;
uniform samplerCube env_map_texture;
varying vec3 lerp_reflected_flipped_cubemap_texcoord;
varying vec3 lerp_refracted_flipped_cubemap_texcoord;

void main(void) {
    vec4 reflected_color = textureCube(env_map_texture, lerp_reflected_flipped_cubemap_texcoord);
    vec4 refracted_color = textureCube(env_map_texture, lerp_refracted_flipped_cubemap_texcoord);

    gl_FragColor = mix(refracted_color, reflected_color, reflect_to_refract_ratio);
}