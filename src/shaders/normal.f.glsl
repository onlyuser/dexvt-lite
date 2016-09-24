const float BUMP_FACTOR = 0.001;
uniform sampler2D bump_texture;
varying mat3 lerp_tbn_xform;
varying vec2 lerp_texcoord;

void main(void) {
    vec2 flipped_texcoord = vec2(lerp_texcoord.x, 1 - lerp_texcoord.y);

    vec3 normal_surface =
            mix(vec3(0, 0, 1), normalize(vec3(texture2D(bump_texture, flipped_texcoord))), BUMP_FACTOR);
    vec3 normal = normalize(lerp_tbn_xform*normal_surface);

    gl_FragColor = vec4((normal + vec3(1))*0.5, 0); // map from [-1,1] to [0,1]
}