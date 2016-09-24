const int BLOOM_KERNEL_SIZE = 7;
uniform float bloom_kernel[BLOOM_KERNEL_SIZE];
uniform float glow_cutoff_threshold;
uniform sampler2D color_texture;
//varying vec2 lerp_sample_offset_unit_size; // strange artifact when declared before lerp_texcoord
varying vec2 lerp_texcoord;
varying vec2 lerp_sample_offset_unit_size;

void sum_components(in vec3 color, out float sum) {
    sum = color.r + color.g + color.b;
}

void main(void) {
    vec3 sum_color = vec3(0);
    for(int i = 0; i < BLOOM_KERNEL_SIZE; i++) {
        ivec2 hblur_kernel_offset_index = ivec2(-3 + i, 0);
        ivec2 vblur_kernel_offset_index = ivec2(0, -3 + i);
        vec2 hblur_sample_coord = vec2(lerp_texcoord.x + lerp_sample_offset_unit_size.x*hblur_kernel_offset_index.x,
                                       lerp_texcoord.y + lerp_sample_offset_unit_size.y*hblur_kernel_offset_index.y);
        vec2 vblur_sample_coord = vec2(lerp_texcoord.x + lerp_sample_offset_unit_size.x*vblur_kernel_offset_index.x,
                                       lerp_texcoord.y + lerp_sample_offset_unit_size.y*vblur_kernel_offset_index.y);
        vec3 hblur_sample_color = texture2D(color_texture, hblur_sample_coord).xyz;
        vec3 vblur_sample_color = texture2D(color_texture, vblur_sample_coord).xyz;
        float hblur_sum;
        float vblur_sum;
        sum_components(hblur_sample_color, hblur_sum);
        sum_components(vblur_sample_color, vblur_sum);
        if(hblur_sum < glow_cutoff_threshold) {
            hblur_sample_color = vec3(0);
        }
        if(vblur_sum < glow_cutoff_threshold) {
            vblur_sample_color = vec3(0);
        }
        sum_color += (hblur_sample_color + vblur_sample_color)*0.5*bloom_kernel[i];
    }
    gl_FragColor = vec4(clamp(sum_color, 0, 1), 0);
}