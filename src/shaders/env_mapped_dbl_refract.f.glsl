const float AIR_REFRACTIVE_INDEX = 1.0;
const float BEERS_LAW_FALLOFF_SHARPNESS = 0.5;
const float BUMP_FACTOR = 0.001;
const float EPSILON = 0.001;
const float FRESNEL_REFLECTANCE_SHARPNESS = 2.0;
const float GLASS_REFRACTIVE_INDEX = 1.5;
const float GLASS_REFRACTIVE_INDEX_RGB_OFFSET = 0.01;
const float MAX_DIST = 20;
const float MAX_DIST_SQUARED = MAX_DIST*MAX_DIST;
const int NUM_LIGHTS = 8;
const int NUM_NEWTONS_METHOD_ITERS = 3;
const int SPECULAR_SHARPNESS = 64;
const vec3 AMBIENT = vec3(0.1, 0.1, 0.1);
const vec4 MATERIAL_AMBIENT_COLOR = vec4(0);
uniform float camera_far;
uniform float camera_near;
uniform float reflect_to_refract_ratio;
uniform int light_count;
uniform int light_enabled[NUM_LIGHTS];
uniform mat4 inv_view_proj_xform;
uniform mat4 view_proj_xform;
uniform sampler2D backface_depth_overlay_texture;
uniform sampler2D backface_normal_overlay_texture;
uniform sampler2D bump_texture;
uniform sampler2D frontface_depth_overlay_texture;
uniform samplerCube env_map_texture;
uniform vec2 viewport_dim;
uniform vec3 camera_pos;
uniform vec3 light_color[NUM_LIGHTS];
uniform vec3 light_pos[NUM_LIGHTS];
varying mat3 lerp_tbn_xform;
varying vec2 lerp_texcoord;
varying vec3 lerp_camera_vector;
varying vec3 lerp_vertex_position_world;

// http://stackoverflow.com/questions/6652253/getting-the-true-z-value-from-the-depth-buffer?answertab=votes#tab-top
void map_depth_to_actual_depth(
        in    float z_near, // camera near-clipping plane distance from camera
        in    float z_far,  // camera far-clipping plane distance from camera
        in    float z_b,    // projected depth
        inout float z_e)    // actual depth
{
    float z_n = 2.0*z_b - 1.0; // projected depth scaled from [0,1] to [-1,1]
    z_e = 2.0*z_near*z_far/(z_far + z_near - z_n*(z_far - z_near));
}

// http://glm.g-truc.net/0.9.5/api/a00203.html#ga6203e3a0822575ced2b2cd500b396b0c
// http://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection#Algebraic_form
void intersect_ray_plane(
        in    vec3  orig,                       // point on ray   (I0)
        in    vec3  dir,                        // ray direction  (I)
        in    vec3  plane_orig,                 // point on plane (p0)
        in    vec3  plane_normal,               // plane normal   (n)
        inout float orig_intersection_distance) // distance between ray-plane intersection and plane
{
    // d = dot(p0 - I0, n)/dot(I, n)
    float num   = dot(plane_orig - orig, plane_normal);
    float denom = dot(dir, plane_normal);
    if((sign(num) != sign(denom)) || (abs(denom) < EPSILON)) {
        orig_intersection_distance = 0;
        return;
    }
    orig_intersection_distance = num/denom;
}

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
    vec3 refracted_camera_dirR;
    vec3 refracted_camera_dirG;
    vec3 refracted_camera_dirB;
    vec4 _refracted_color;
    refract_into_env_map(ray_direction, surface_normal, eta - eta_rgb_offset, env_map_texture, _refracted_color, refracted_camera_dirR);
    refracted_color.r = _refracted_color.r;
    refract_into_env_map(ray_direction, surface_normal, eta,                  env_map_texture, _refracted_color, refracted_camera_dirG);
    refracted_color.g = _refracted_color.g;
    refract_into_env_map(ray_direction, surface_normal, eta + eta_rgb_offset, env_map_texture, _refracted_color, refracted_camera_dirB);
    refracted_color.b = _refracted_color.b;
    refracted_color.a = 1;
    refracted_camera_dir = refracted_camera_dirG;
}

// http://www.songho.ca/opengl/gl_projectionmatrix.html
void unproject_fragment(
        in    vec3  frag_pos,
        in    mat4  _inv_view_proj_xform,
        inout vec3  world_pos)
{
    vec4 normalized_device_coord = vec4(frag_pos.x*2 - 1, frag_pos.y*2 - 1 , frag_pos.z*2 - 1, 1);
    vec4 unprojected_coord = _inv_view_proj_xform*normalized_device_coord;

    // http://www.iquilezles.org/blog/?p=1911
    unprojected_coord.xyz /= unprojected_coord.w; // perspective divide

    world_pos = unprojected_coord.xyz;
}

void newtons_method_update(
        in    sampler2D _backface_depth_overlay_texture,
        in    sampler2D _backface_normal_overlay_texture,
        in    mat4      _view_proj_xform,
        in    vec3      _camera_pos,
        in    vec3      orig,         // point on ray
        in    vec3      dir,          // ray direction
        inout vec3      plane_orig,   // point on plane
        inout vec3      plane_normal, // plane normal
        inout bool      abort)
{
    float orig_intersection_distance = 0;
    intersect_ray_plane(
            orig,                        // point on ray
            dir,                         // ray direction
            plane_orig,                  // point on plane
            plane_normal,                // plane normal
            orig_intersection_distance); // distance between ray-plane intersection and plane

    // 1st chance abort on glancing edge
    //if(abs(orig_intersection_distance) < EPSILON) {
    //    abort = true;
    //    return;
    //}

    vec3 ray_plane_isect = orig + normalize(dir)*orig_intersection_distance;

    vec4 projected_coord = _view_proj_xform*vec4(ray_plane_isect, 1);
    projected_coord.xyz /= projected_coord.w; // perspective divide
    vec3 ray_plane_isect_texcoord_raw = projected_coord.xyz;

    vec2 ray_plane_isect_texcoord = (ray_plane_isect_texcoord_raw.xy + vec2(1))*0.5; // map from [-1,1] to [0,1]

    float new_backface_depth = texture2D(_backface_depth_overlay_texture, ray_plane_isect_texcoord).x;

    float new_backface_depth_actual = 0;
    map_depth_to_actual_depth(camera_near, camera_far, new_backface_depth, new_backface_depth_actual);

    vec3 new_backface_frag_position_world = _camera_pos + normalize(ray_plane_isect - _camera_pos)*new_backface_depth_actual;
    vec4 new_backface_normal_color        = texture2D(_backface_normal_overlay_texture, ray_plane_isect_texcoord);
    vec3 new_backface_normal              = -normalize(new_backface_normal_color.xyz*2 - vec3(1)); // map from [0,1] to [-1,1]

    plane_orig   = new_backface_frag_position_world;
    plane_normal = new_backface_normal;
}

void calculate_light_contrib(
        in    vec3 camera_direction,
        in    vec3 plane_orig,
        in    vec3 plane_normal,
        inout vec4 light_contrib)
{
    vec3 diffuse_sum = vec3(0.0, 0.0, 0.0);
    vec3 specular_sum = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < NUM_LIGHTS && i < light_count; i++) {
        if(light_enabled[i] == 0) {
            continue;
        }

        vec3 light_vector = light_pos[i] - plane_orig;
        float dist = min(dot(light_vector, light_vector), MAX_DIST_SQUARED)/MAX_DIST_SQUARED;
        float distance_factor = 1.0 - dist;

        vec3 light_direction = normalize(light_vector);
        float diffuse_per_light = dot(plane_normal, light_direction);
        vec3 _light_color = light_color[i];
        diffuse_sum += _light_color*clamp(diffuse_per_light, 0.0, 1.0)*distance_factor;

        vec3 half_angle = normalize(camera_direction + light_direction);
        vec3 specular_color = min(_light_color + 0.5, 1.0);
        float specular_per_light = dot(plane_normal, half_angle);
        specular_sum += specular_color*pow(clamp(specular_per_light, 0.0, 1.0), SPECULAR_SHARPNESS)*distance_factor;
    }

    vec4 sample = vec4(1);
    light_contrib = vec4(clamp(sample.rgb*(diffuse_sum + AMBIENT)*0 + specular_sum, 0.0, 1.0), sample.a);
}

void main(void) {
    vec3 camera_direction = normalize(lerp_camera_vector);

    // another way to get camera direction
    //vec3 camera_direction = normalize(camera_pos - lerp_vertex_position_world);

    vec2 flipped_texcoord = vec2(lerp_texcoord.x, 1 - lerp_texcoord.y);

    vec3 normal_surface =
            mix(vec3(0, 0, 1), normalize(vec3(texture2D(bump_texture, flipped_texcoord))), BUMP_FACTOR);
    vec3 normal = normalize(lerp_tbn_xform*normal_surface);

    vec4 frontface_light_contrib;
    calculate_light_contrib(
            camera_direction,
            lerp_vertex_position_world,
            normal,
            frontface_light_contrib);

    // frontface reflection component
    vec4 reflected_color;
    reflect_into_env_map(-camera_direction, normal, env_map_texture, reflected_color);

    // frontface refraction component with chromatic dispersion
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

    vec2 overlay_texcoord = vec2(gl_FragCoord.x/viewport_dim.x, gl_FragCoord.y/viewport_dim.y);

    //if(overlay_texcoord.x < 0.5) {
    //    // frontface fresnel component
    //    float frontface_fresnel_reflectance =
    //            pow(1 - clamp(dot(camera_direction, normal), 0, 1), FRESNEL_REFLECTANCE_SHARPNESS);
    //    gl_FragColor =
    //            mix(frontface_refracted_color, reflected_color,
    //                    max(reflect_to_refract_ratio, frontface_fresnel_reflectance));
    //    return;
    //}

    float frontface_depth       = texture2D(frontface_depth_overlay_texture, overlay_texcoord).x;
    float backface_depth        = texture2D(backface_depth_overlay_texture, overlay_texcoord).x;
    vec4  backface_normal_color = texture2D(backface_normal_overlay_texture, overlay_texcoord);
    vec3  backface_normal       = -normalize(backface_normal_color.xyz*2 - vec3(1)); // map from [0,1] to [-1,1]

    float frontface_depth_actual = 0;
    float backface_depth_actual  = 0;

    map_depth_to_actual_depth(camera_near, camera_far, frontface_depth, frontface_depth_actual);
    map_depth_to_actual_depth(camera_near, camera_far, backface_depth, backface_depth_actual);

    float frag_thickness = backface_depth_actual - frontface_depth_actual;

    // replace incorrect way of getting fragment world position
    // z-depth is measured in rays parallel to camera, not rays emanating from camera
    //vec3 backface_frag_position_world = camera_pos - camera_direction*backface_depth_actual;
    vec3 backface_frag_position_world;
    unproject_fragment(vec3(overlay_texcoord, backface_depth), inv_view_proj_xform, backface_frag_position_world);

    //vec3 ray_plane_isect = lerp_vertex_position_world + frontface_refracted_camera_dir*???;

    // apply newton's method to find backface intersection with refracted ray from camera

    vec3 orig         = lerp_vertex_position_world;
    vec3 dir          = normalize(frontface_refracted_camera_dir);
    vec3 plane_orig   = backface_frag_position_world;
    vec3 plane_normal = normalize(backface_normal);

    bool abort = false;
    for(int i = 0; i < NUM_NEWTONS_METHOD_ITERS; i++) {
        newtons_method_update(
                backface_depth_overlay_texture,
                backface_normal_overlay_texture,
                view_proj_xform,
                camera_pos,
                orig,
                dir,
                plane_orig,
                plane_normal,
                abort);
        if(abort) {
            //gl_FragColor = vec4(0,1,0,0);//frontface_refracted_color;
            return;
        }
    }

    // backface refraction component with chromatic dispersion
    float backface_eta = GLASS_REFRACTIVE_INDEX/AIR_REFRACTIVE_INDEX;
    float backface_eta_red = (GLASS_REFRACTIVE_INDEX - GLASS_REFRACTIVE_INDEX_RGB_OFFSET)/AIR_REFRACTIVE_INDEX;
    float backface_eta_rgb_offset = abs(backface_eta - backface_eta_red);
    vec4 backface_refracted_color;
    vec3 backface_refracted_camera_dir;
    refract_into_env_map_ex(
            dir,
            plane_normal,
            backface_eta,
            -backface_eta_rgb_offset,
            env_map_texture,
            backface_refracted_color,
            backface_refracted_camera_dir);

    // backface reflection component
    // 2nd chance abort on total internal reflection
    if(distance(backface_refracted_camera_dir, vec3(0)) < EPSILON) {
        vec3 backface_reflected_camera_dir = reflect(frontface_refracted_camera_dir, plane_normal);
        vec4 total_internal_reflected_color;
        sample_env_map(backface_reflected_camera_dir, env_map_texture, total_internal_reflected_color);
        backface_refracted_color = total_internal_reflected_color;
    }

    float beers_law_transmittance = exp(-frag_thickness*BEERS_LAW_FALLOFF_SHARPNESS);

    // backface fresnel component
    float backface_fresnel_reflectance =
            pow(1 - clamp(dot(camera_direction, plane_normal), 0, 1), FRESNEL_REFLECTANCE_SHARPNESS);

    vec4 backface_light_contrib;
    calculate_light_contrib(
            -normalize(frontface_refracted_camera_dir),
            plane_orig,
            plane_normal,
            backface_light_contrib);

    vec4 inside_color_prelit =
            mix(MATERIAL_AMBIENT_COLOR, backface_refracted_color,
                    max(beers_law_transmittance, backface_fresnel_reflectance));
    vec4 inside_color = min(inside_color_prelit + backface_light_contrib*beers_law_transmittance, 1);

    //if(frontface_depth_actual >= (camera_far - 0.1)) {
    //    gl_FragColor = vec4(1,1,0,0);
    //    return;
    //} else if(frontface_depth_actual <= (camera_near + 0.1)) {
    //    gl_FragColor = vec4(0,1,1,0);
    //    return;
    //}

    // frontface fresnel component
    float frontface_fresnel_reflectance =
            pow(1 - clamp(dot(camera_direction, normal), 0, 1), FRESNEL_REFLECTANCE_SHARPNESS);
    vec4 outside_color_prelit =
            //mix(vec4(1,0,0,0), vec4(0,0,1,0), frontface_depth)*0.001 +
            //mix(vec4(1,0,0,0), vec4(0,0,1,0), backface_depth)*0.001 +
            //mix(vec4(1,0,0,0), vec4(0,0,1,0), frag_thickness)*0.001 +
            //backface_normal_color*0.001 +
            mix(inside_color, reflected_color,
                    max(reflect_to_refract_ratio, frontface_fresnel_reflectance));
    gl_FragColor = min(outside_color_prelit + frontface_light_contrib, 1);
}