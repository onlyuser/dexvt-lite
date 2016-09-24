const float AIR_REFRACTIVE_INDEX = 1.0;
const float BUMP_FACTOR = 0.001;
const float FRESNEL_REFLECTANCE_SHARPNESS = 2.0;
const float GLASS_REFRACTIVE_INDEX = 1.5;
const float GLASS_REFRACTIVE_INDEX_RGB_OFFSET = 0.01;
uniform float reflect_to_refract_ratio;
uniform sampler2D bump_texture;
uniform samplerCube env_map_texture;
uniform vec3 camera_pos;
varying mat3 lerp_tbn_xform;
varying vec2 lerp_texcoord;
varying vec3 lerp_camera_vector;
varying vec3 lerp_vertex_position_world;

void sample_env_map(
        in    vec3        ray_direction,
        in    samplerCube env_map_texture,
        inout vec4        env_color)
{
    vec3 flipped_cubemap_texcoord = vec3(ray_direction.x, -ray_direction.y, ray_direction.z);
    env_color = textureCube(env_map_texture, flipped_cubemap_texcoord);
}

void reflect_into_env_map(
        in    vec3        ray_direction,
        in    vec3        surface_normal,
        in    samplerCube env_map_texture,
        inout vec4        reflected_color)
{
    vec3 reflected_camera_dir = reflect(ray_direction, surface_normal);
    sample_env_map(reflected_camera_dir, env_map_texture, reflected_color);
}

void refract_into_env_map(
        in    vec3        ray_direction,
        in    vec3        surface_normal,
        in    float       eta,
        in    samplerCube env_map_texture,
        inout vec4        refracted_color,
        inout vec3        refracted_camera_dir)
{
    refracted_camera_dir = refract(ray_direction, surface_normal, eta);
    sample_env_map(refracted_camera_dir, env_map_texture, refracted_color);
}

// http://en.wikipedia.org/wiki/Chromatic_aberration
// http://en.wikipedia.org/wiki/Dispersion_(optics)
// http://www.opticampus.com/cecourse.php?url=chromatic_aberration
void refract_into_env_map_ex(
        in    vec3        ray_direction,
        in    vec3        surface_normal,
        in    float       eta,
        in    float       eta_rgb_offset,
        in    samplerCube env_map_texture,
        inout vec4        refracted_color,
        inout vec3        refracted_camera_dir)
{
    // wiki: refraction indices of most transparent materials (e.g., air, glasses) decrease with increasing wavelength
    vec3 refracted_camera_dirR = refract(ray_direction, surface_normal, eta - eta_rgb_offset);
    vec3 refracted_camera_dirG = refract(ray_direction, surface_normal, eta);
    vec3 refracted_camera_dirB = refract(ray_direction, surface_normal, eta + eta_rgb_offset);
    vec3 refracted_flipped_cubemap_texcoordR = vec3(refracted_camera_dirR.x, -refracted_camera_dirR.y, refracted_camera_dirR.z);
    vec3 refracted_flipped_cubemap_texcoordG = vec3(refracted_camera_dirG.x, -refracted_camera_dirG.y, refracted_camera_dirG.z);
    vec3 refracted_flipped_cubemap_texcoordB = vec3(refracted_camera_dirB.x, -refracted_camera_dirB.y, refracted_camera_dirB.z);
    refracted_color.r = textureCube(env_map_texture, refracted_flipped_cubemap_texcoordR).r;
    refracted_color.g = textureCube(env_map_texture, refracted_flipped_cubemap_texcoordG).g;
    refracted_color.b = textureCube(env_map_texture, refracted_flipped_cubemap_texcoordB).b;
    refracted_color.a = 1;
    refracted_camera_dir = refracted_camera_dirG;
}

void main(void) {
    vec3 camera_direction = normalize(lerp_camera_vector);

    // another way to get camera direction
    //vec3 camera_direction = normalize(camera_pos - lerp_vertex_position_world);

    vec2 flipped_texcoord = vec2(lerp_texcoord.x, 1 - lerp_texcoord.y);

    vec3 normal_surface =
            mix(vec3(0, 0, 1), normalize(vec3(texture2D(bump_texture, flipped_texcoord))), BUMP_FACTOR);
    vec3 normal = normalize(lerp_tbn_xform*normal_surface);

    // reflection component
    vec4 reflected_color;
    reflect_into_env_map(-camera_direction, normal, env_map_texture, reflected_color);

    // refraction component with chromatic dispersion
    float frontface_eta = AIR_REFRACTIVE_INDEX/GLASS_REFRACTIVE_INDEX;
    float frontface_eta_red = AIR_REFRACTIVE_INDEX/(GLASS_REFRACTIVE_INDEX - GLASS_REFRACTIVE_INDEX_RGB_OFFSET);
    float frontface_eta_rgb_offset = abs(frontface_eta - frontface_eta_red);
    vec4 frontface_refracted_color;
    vec3 frontface_refracted_camera_dir;
    refract_into_env_map_ex(
            -camera_direction,
            normal,
            frontface_eta,
            frontface_eta_rgb_offset,
            env_map_texture,
            frontface_refracted_color,
            frontface_refracted_camera_dir);

    // fresnel component
    float frontface_fresnel_reflectance =
            pow(1 - clamp(dot(camera_direction, normal), 0, 1), FRESNEL_REFLECTANCE_SHARPNESS);
    gl_FragColor =
            mix(frontface_refracted_color, reflected_color,
                    max(reflect_to_refract_ratio, frontface_fresnel_reflectance));
}