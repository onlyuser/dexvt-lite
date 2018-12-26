const float BIG_NUMBER = 10000.0f;
const float EPSILON    = 0.0001f;
const float EPSILON2   = EPSILON * 0.9;

uniform samplerCube env_map_texture;
uniform vec3        camera_pos;
varying vec3        lerp_vertex_dir_world;

uniform int ray_tracer_render_mode;
uniform int ray_tracer_bounce_count;

const int MAX_SPHERES = 4;
uniform int   ray_tracer_sphere_count;
uniform vec3  ray_tracer_sphere_origin[MAX_SPHERES];
uniform float ray_tracer_sphere_radius[MAX_SPHERES];
uniform float ray_tracer_sphere_eta[MAX_SPHERES];
uniform float ray_tracer_sphere_diffuse_fuzz[MAX_SPHERES];
uniform vec3  ray_tracer_sphere_color[MAX_SPHERES];
uniform float ray_tracer_sphere_reflectance[MAX_SPHERES];
uniform float ray_tracer_sphere_transparency[MAX_SPHERES];
uniform float ray_tracer_sphere_luminosity[MAX_SPHERES];

const int MAX_PLANES = 4;
uniform int   ray_tracer_plane_count;
uniform vec3  ray_tracer_plane_point[MAX_PLANES];
uniform vec3  ray_tracer_plane_normal[MAX_PLANES];
uniform float ray_tracer_plane_eta[MAX_PLANES];
uniform float ray_tracer_plane_diffuse_fuzz[MAX_PLANES];
uniform vec3  ray_tracer_plane_color[MAX_PLANES];
uniform float ray_tracer_plane_reflectance[MAX_PLANES];
uniform float ray_tracer_plane_transparency[MAX_PLANES];
uniform float ray_tracer_plane_luminosity[MAX_PLANES];

const int MAX_BOXES = 4;
uniform int   ray_tracer_box_count;
uniform mat4  ray_tracer_box_transform[MAX_BOXES];
uniform mat4  ray_tracer_box_inverse_transform[MAX_BOXES];
uniform vec3  ray_tracer_box_min[MAX_BOXES];
uniform vec3  ray_tracer_box_max[MAX_BOXES];
uniform float ray_tracer_box_eta[MAX_BOXES];
uniform float ray_tracer_box_diffuse_fuzz[MAX_BOXES];
uniform vec3  ray_tracer_box_color[MAX_BOXES];
uniform float ray_tracer_box_reflectance[MAX_BOXES];
uniform float ray_tracer_box_transparency[MAX_BOXES];
uniform float ray_tracer_box_luminosity[MAX_BOXES];

const int MAX_RANDOM_POINTS = 20;
uniform int   ray_tracer_random_point_count;
uniform vec3  ray_tracer_random_points[MAX_RANDOM_POINTS];
uniform float ray_tracer_random_seed;

int sphere_start_index = 0;
int sphere_stop_index  = sphere_start_index + min(ray_tracer_sphere_count, MAX_SPHERES);
int plane_start_index  = sphere_stop_index;
int plane_stop_index   = plane_start_index + min(ray_tracer_plane_count, MAX_PLANES);
int box_start_index    = plane_stop_index;
int box_stop_index     = box_start_index + min(ray_tracer_box_count, MAX_BOXES);

const float MAX_DIST = 20;
const float MAX_DIST_SQUARED = MAX_DIST * MAX_DIST;
const int MAX_LIGHTS = 8;
const int SPECULAR_SHARPNESS = 16;
uniform int light_count;
uniform int light_enabled[MAX_LIGHTS];
uniform vec3 ambient_color = vec3(0);
uniform vec3 light_color[MAX_LIGHTS];
uniform vec3 light_pos[MAX_LIGHTS];

const int MAX_ITERS = 10;
vec3  phong_stack[MAX_ITERS];
float normal_projection_stack[MAX_ITERS];
vec3  ray_dir_stack[MAX_ITERS];
float surface_eta_stack[MAX_ITERS];
vec3  surface_color_stack[MAX_ITERS];
float reflectance_stack[MAX_ITERS];
float transparency_stack[MAX_ITERS];
float luminosity_stack[MAX_ITERS];
float dist_stack[MAX_ITERS];

uniform sampler2D color_texture;
uniform sampler2D color_texture2;
uniform int       color_texture_source;
varying vec2      lerp_texcoord;

const float COLOR_DRIFT_RATIO = 0.1;
const float FOG_DISTANCE      = 5.0;

int last_ray_tracer_bounce_index = min(ray_tracer_bounce_count - 1, MAX_ITERS);

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
// https://thebookofshaders.com/10/
float rand(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 get_random_offset()
{
    //return vec3(rand(gl_FragCoord.xy),
    //            rand(gl_FragCoord.xy),
    //            rand(gl_FragCoord.xy));
    float _rand = rand(gl_FragCoord.xy);
    int random_index = int(_rand * ray_tracer_random_point_count + ray_tracer_random_seed);
    random_index = int(mod(random_index, ray_tracer_random_point_count));
    return ray_tracer_random_points[random_index];
}

// https://en.wikipedia.org/wiki/Vector_projection
vec3 projection_onto(in vec3 a, in vec3 b)
{
    vec3 norm_b = normalize(b);
    return norm_b * dot(a, norm_b);
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
// https://www.cs.uaf.edu/2012/spring/cs481/section/0/lecture/02_14_refraction.html
float ray_sphere_intersection(in    vec3  sphere_origin,
                              in    float sphere_radius,
                              in    vec3  ray_origin,
                              in    vec3  ray_dir,
                              inout vec3  surface_normal,
                              inout bool  ray_starts_inside_sphere)
{
    vec3  ray_nearest_point_to_sphere        = ray_origin + projection_onto(sphere_origin - ray_origin, ray_dir);
    vec3  sphere_origin_to_ray_nearest_point = ray_nearest_point_to_sphere - sphere_origin;
    float dist_to_ray_from_sphere_squared    = dot(sphere_origin_to_ray_nearest_point, sphere_origin_to_ray_nearest_point);
    float sphere_radius_squared              = sphere_radius * sphere_radius;
    if(dist_to_ray_from_sphere_squared > sphere_radius_squared) { // ray entirely misses sphere
        return BIG_NUMBER;
    }
    float half_length_inside_sphere = pow(sphere_radius_squared - dist_to_ray_from_sphere_squared, 0.5);
    vec3 surface_point;
    ray_starts_inside_sphere = false;
    if(abs(dot(ray_nearest_point_to_sphere, ray_dir) - dot(ray_origin, ray_dir)) < half_length_inside_sphere) { // ray starts inside sphere
        surface_point = ray_nearest_point_to_sphere + ray_dir * half_length_inside_sphere;
        ray_starts_inside_sphere = true;
    } else { // ray starts outside sphere
        if(dot(sphere_origin, ray_dir) < dot(ray_origin, ray_dir)) { // ray points away from sphere
            return BIG_NUMBER;
        }
        surface_point = ray_nearest_point_to_sphere - ray_dir * half_length_inside_sphere;
    }
    surface_normal = normalize(surface_point - sphere_origin);
    return distance(ray_origin, surface_point);
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
// https://www.cs.uaf.edu/2012/spring/cs481/section/0/lecture/02_14_refraction.html
float ray_plane_intersection(in vec3 plane_point,
                             in vec3 plane_normal,
                             in vec3 ray_origin,
                             in vec3 ray_dir)
{
    float denom = dot(ray_dir, plane_normal);
    if(abs(denom) < EPSILON /*|| denom > 0*/) { // zero denominator (undefined) or ray points at plane backside (implies ray exiting plane)
        return BIG_NUMBER;
    }
    float dist = dot(plane_point - ray_origin, plane_normal) / denom;
    if(dist < 0) { // ray points away from plane
        return BIG_NUMBER;
    }
    return dist;
}

bool is_within(vec3 pos,
               vec3 _min,
               vec3 _max)
{
    vec3 __min = _min - vec3(EPSILON);
    vec3 __max = _max + vec3(EPSILON);
    return pos.x > __min.x && pos.y > __min.y && pos.z > __min.z &&
           pos.x < __max.x && pos.y < __max.y && pos.z < __max.z;
}

vec3 get_absolute_direction(int euler_index)
{
    if(euler_index == 0) { return vec3(0, 0, 1); }
    if(euler_index == 1) { return vec3(1, 0, 0); }
    if(euler_index == 2) { return vec3(0, 1, 0); }
    return vec3(0);
}

// http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
float ray_box_intersect(in    mat4 box_transform,
                        in    mat4 box_inverse_transform,
                        in    vec3 box_min,
                        in    vec3 box_max,
                        in    vec3 ray_origin,
                        in    vec3 ray_dir,
                        inout vec3 surface_point,
                        inout vec3 surface_normal)
{
    // for each potential separation axis
    float _dist = BIG_NUMBER;
    vec3  _surface_point;
    vec3  _surface_normal;
    mat4  self_normal_transform = transpose(box_inverse_transform);
    vec3  plane_point_min       = vec3(box_transform * vec4(box_min, 1));
    vec3  plane_point_max       = vec3(box_transform * vec4(box_max, 1));
    for(int i = 0; i < 3; i++) { // principle directions
        vec3 absolute_direction = vec3(self_normal_transform * vec4(get_absolute_direction(i), 1));
        for(int j = 0; j < 2; j++) { // min side, max side
            vec3 plane_point;
            vec3 plane_normal;
            if(j == 0) {
                plane_point  =  plane_point_min;
                plane_normal = -absolute_direction;
            } else {
                plane_point  = plane_point_max;
                plane_normal = absolute_direction;
            }
            float dist = ray_plane_intersection(plane_point,
                                                plane_normal,
                                                ray_origin,
                                                ray_dir);
            if(dist == BIG_NUMBER) { // ray hits nothing
                continue;
            }
            vec3 point = ray_origin + ray_dir * dist;
            vec3 local_point = vec3(box_inverse_transform * vec4(point, 1));
            if(!is_within(local_point, box_min, box_max)) {
                continue;
            }
            if(dist < _dist) {
                _dist           = dist;
                _surface_point  = plane_point;
                _surface_normal = plane_normal;
            }
        }
    }
    if(_dist == BIG_NUMBER) { // ray hits nothing
        return BIG_NUMBER;
    }
    surface_point  = _surface_point;
    surface_normal = _surface_normal;
    return _dist;
}

float ray_world_intersect(in    vec3 ray_origin,
                          in    vec3 ray_dir,
                          in    bool early_hit_test,
                          inout int  obj_index,
                          inout vec3 surface_point,
                          inout vec3 surface_normal,
                          inout bool ray_starts_inside_sphere)
{
    float dist = BIG_NUMBER;
    obj_index  = -1;
    for(int i = sphere_start_index; i < sphere_stop_index; i++) {
        int local_index = i - sphere_start_index;
        vec3 _surface_normal;
        float _dist = ray_sphere_intersection(ray_tracer_sphere_origin[local_index],
                                              ray_tracer_sphere_radius[local_index],
                                              ray_origin,
                                              ray_dir,
                                              _surface_normal,
                                              ray_starts_inside_sphere);
        if(_dist == BIG_NUMBER) { // ray hits nothing
            continue;
        }
        if(early_hit_test) {
            return _dist; // not BIG_NUMBER indicates hit
        }
        if(_dist < dist) {
            dist           = _dist;
            obj_index      = i;
            surface_normal = _surface_normal;
        }
    }
    for(int i = plane_start_index; i < plane_stop_index; i++) {
        int local_index = i - plane_start_index;
        float _dist = ray_plane_intersection(ray_tracer_plane_point[local_index],
                                             ray_tracer_plane_normal[local_index],
                                             ray_origin,
                                             ray_dir);
        if(_dist == BIG_NUMBER) { // ray hits nothing
            continue;
        }
        if(early_hit_test) {
            return _dist; // not BIG_NUMBER indicates hit
        }
        if(_dist < dist) {
            dist           = _dist;
            obj_index      = i;
            surface_point  = ray_tracer_plane_point[local_index];
            surface_normal = ray_tracer_plane_normal[local_index];
        }
    }
    for(int i = box_start_index; i < box_stop_index; i++) {
        int local_index = i - box_start_index;
        vec3 _surface_point;
        vec3 _surface_normal;
        float _dist = ray_box_intersect(ray_tracer_box_transform[local_index],
                                        ray_tracer_box_inverse_transform[local_index],
                                        ray_tracer_box_min[local_index],
                                        ray_tracer_box_max[local_index],
                                        ray_origin,
                                        ray_dir,
                                        _surface_point,
                                        _surface_normal);
        if(_dist == BIG_NUMBER) { // ray hits nothing
            continue;
        }
        if(early_hit_test) {
            return _dist; // not BIG_NUMBER indicates hit
        }
        if(_dist < dist) {
            dist           = _dist;
            obj_index      = i;
            surface_point  = _surface_point;
            surface_normal = _surface_normal;
        }
    }
    return dist;
}

void adjust_ray(in    bool  do_reflection,
                in    float eta,
                in    float diffuse_fuzz,
                inout float dist,
                inout vec3  next_ray)
{
    if(eta == -BIG_NUMBER) { // diffuse material
        next_ray = normalize(next_ray + get_random_offset() * diffuse_fuzz);
    }
    if(do_reflection) { // transparent material -- offset to prevent self-intersection
        dist -= EPSILON;
    }
    dist += EPSILON2; // transparent material -- offset to prevent self-intersection
}

void ray_sphere_next_ray(in    vec3  ray_origin,
                         in    vec3  ray_dir,
                         in    vec3  surface_normal,
                         in    bool  ray_starts_inside_sphere,
                         in    float sphere_eta,
                         in    float sphere_diffuse_fuzz,
                         inout float dist,
                         inout vec3  next_ray)
{
    if(dist == BIG_NUMBER) { // ray hits nothing
        return;
    }
    bool do_reflection = false;
    if(abs(sphere_eta) == BIG_NUMBER) { // opaque material
        next_ray = reflect(ray_dir, surface_normal);
        do_reflection = true;
    } else { // transparent material
        if(ray_starts_inside_sphere) { // ray starts inside sphere
            next_ray = refract(ray_dir, -surface_normal, 1.0f / sphere_eta);
            if(length(next_ray) == 0) { // refract fails -- total internal reflection
                next_ray = reflect(ray_dir, -surface_normal);
                do_reflection = true;
            }
        } else { // ray starts outside sphere
            next_ray = refract(ray_dir, surface_normal, sphere_eta);
        }
    }
    adjust_ray(do_reflection,
               sphere_eta,
               sphere_diffuse_fuzz,
               dist,
               next_ray);
}

void ray_plane_next_ray(in    vec3  ray_origin,
                        in    vec3  ray_dir,
                        in    vec3  plane_point,
                        in    vec3  plane_normal,
                        in    float plane_eta,
                        in    float plane_diffuse_fuzz,
                        inout float dist,
                        inout vec3  next_ray)
{
    if(dist == BIG_NUMBER) { // ray hits nothing
        return;
    }
    bool do_reflection = false;
    if(abs(plane_eta) == BIG_NUMBER) { // opaque material
        next_ray = reflect(ray_dir, plane_normal);
        do_reflection = true;
    } else { // transparent material
        if(dot(ray_origin, plane_normal) < dot(plane_point, plane_normal)) { // ray starts inside plane
            next_ray = refract(ray_dir, -plane_normal, 1.0f / plane_eta);
            if(length(next_ray) == 0) { // refract fails -- total internal reflection
                next_ray = reflect(ray_dir, -plane_normal);
                do_reflection = true;
            }
        } else { // ray starts outside plane
            next_ray = refract(ray_dir, plane_normal, plane_eta);
        }
    }
    adjust_ray(do_reflection,
               plane_eta,
               plane_diffuse_fuzz,
               dist,
               next_ray);
}

vec3 ray_phong(in vec3 camera_pos,
               in vec3 surface_point,
               in vec3 surface_normal)
{
    vec3 diffuse_sum      = vec3(0);
    vec3 specular_sum     = vec3(0);
    vec3 camera_direction = normalize(camera_pos - surface_point);
    if(dot(camera_direction, surface_normal) < 0) {
        return vec3(0);
    }
    float dist = BIG_NUMBER;
    for(int i = 0; i < MAX_LIGHTS && i < light_count; i++) {
        if(light_enabled[i] == 0) {
            continue;
        }

        vec3 light_vector    = light_pos[i] - surface_point;
        vec3 light_direction = normalize(light_vector);

        vec3 ray_origin = surface_point + surface_normal * EPSILON; // offset to prevent self-intersection
        vec3 ray_dir    = light_direction;
        int  obj_index  = -1;
        vec3 _surface_point;
        vec3 _surface_normal;
        bool ray_starts_inside_sphere = false;
        dist = ray_world_intersect(ray_origin,
                                   ray_dir,
                                   true, // early_hit_test
                                   obj_index,
                                   _surface_point,
                                   _surface_normal,
                                   ray_starts_inside_sphere);
        if(dist != BIG_NUMBER &&        // not BIG_NUMBER indicates hit (implies surface is in shadow)
           dist < length(light_vector)) // obstacle is nearer to surface than light
                                        // NOTE: questionable usage of early hit test returned distance (not guaranteed to be nearest hit)
        {
            continue;
        }

        float dist = min(dot(light_vector, light_vector), MAX_DIST_SQUARED) / MAX_DIST_SQUARED;
        float dist_factor = 1 - dist;

        float diffuse_per_light = dot(surface_normal, light_direction);
        diffuse_sum += light_color[i] * clamp(diffuse_per_light, 0, 1) * dist_factor;

        vec3 half_angle = normalize(camera_direction + light_direction);
        vec3 specular_color = min(light_color[i] + 0.5, 1);
        float specular_per_light = dot(surface_normal, half_angle);
        specular_sum += specular_color * pow(clamp(specular_per_light, 0, 1), SPECULAR_SHARPNESS) * dist_factor;
    }
    return clamp((diffuse_sum + ambient_color) + specular_sum, 0, 1);
}

void sample_env_map(in    vec3        ray_direction,
                    in    samplerCube env_map_texture,
                    inout vec3        env_color)
{
    vec3 flipped_cubemap_texcoord = vec3(ray_direction.x, -ray_direction.y, ray_direction.z);
    env_color = textureCube(env_map_texture, flipped_cubemap_texcoord).rgb;
}

int stack_size = 0;

int push_ray(in vec3  ray_dir,
             in float surface_eta,
             in vec3  surface_color,
             in float reflectance,
             in float transparency,
             in float luminosity,
             in float normal_projection,
             in vec3  phong,
             in float dist)
{
    ray_dir_stack[stack_size]           = ray_dir;
    surface_eta_stack[stack_size]       = surface_eta;
    surface_color_stack[stack_size]     = surface_color;
    reflectance_stack[stack_size]       = reflectance;
    transparency_stack[stack_size]      = transparency;
    luminosity_stack[stack_size]        = luminosity;
    normal_projection_stack[stack_size] = normal_projection;
    phong_stack[stack_size]             = phong;
    dist_stack[stack_size]              = dist;
    stack_size++;
    return stack_size;
}

int pop_ray(inout vec3  ray_dir,
            inout float surface_eta,
            inout vec3  surface_color,
            inout float reflectance,
            inout float transparency,
            inout float luminosity,
            inout float normal_projection,
            inout vec3  phong,
            inout float dist)
{
    stack_size--;
    ray_dir           = ray_dir_stack[stack_size];
    surface_eta       = surface_eta_stack[stack_size];
    surface_color     = surface_color_stack[stack_size];
    reflectance       = reflectance_stack[stack_size];
    transparency      = transparency_stack[stack_size];
    luminosity        = luminosity_stack[stack_size];
    normal_projection = normal_projection_stack[stack_size];
    phong             = phong_stack[stack_size];
    dist              = dist_stack[stack_size];
    return stack_size;
}

void main()
{
    // calculate ray bounces backwards from camera to skybox
    vec3 ray_origin = camera_pos;
    vec3 ray_dir    = normalize(lerp_vertex_dir_world /*+ get_random_offset() * 0.01*/);

    float dist;
    float surface_eta;
    vec3  surface_color;
    float reflectance;
    float transparency;
    float luminosity;
    float normal_projection;
    vec3  phong;

    int  obj_index    = -1;
    vec3 next_ray_dir = ray_dir;
    stack_size = 0;
    for(int iter = 0; iter <= last_ray_tracer_bounce_index; iter++) {
        vec3 surface_point;
        vec3 surface_normal;
        bool ray_starts_inside_sphere = false;
        dist = ray_world_intersect(ray_origin,
                                   ray_dir,
                                   false, // early_hit_test
                                   obj_index,
                                   surface_point,
                                   surface_normal,
                                   ray_starts_inside_sphere);
        if(obj_index >= sphere_start_index && obj_index < sphere_stop_index) { // ray hits sphere
            int local_index = obj_index - sphere_start_index;
            surface_eta   = ray_tracer_sphere_eta[local_index];
            surface_color = ray_tracer_sphere_color[local_index];
            reflectance   = ray_tracer_sphere_reflectance[local_index];
            transparency  = ray_tracer_sphere_transparency[local_index];
            luminosity    = ray_tracer_sphere_luminosity[local_index];
            ray_sphere_next_ray(ray_origin,
                                ray_dir,
                                surface_normal,
                                ray_starts_inside_sphere,
                                ray_tracer_sphere_eta[local_index],
                                ray_tracer_sphere_diffuse_fuzz[local_index],
                                dist,
                                next_ray_dir);
        } else if(obj_index >= plane_start_index && obj_index < plane_stop_index) { // ray hits plane
            int local_index = obj_index - plane_start_index;
            surface_eta   = ray_tracer_plane_eta[local_index];
            surface_color = ray_tracer_plane_color[local_index];
            reflectance   = ray_tracer_plane_reflectance[local_index];
            transparency  = ray_tracer_plane_transparency[local_index];
            luminosity    = ray_tracer_plane_luminosity[local_index];
            ray_plane_next_ray(ray_origin,
                               ray_dir,
                               surface_point,
                               surface_normal,
                               ray_tracer_plane_eta[local_index],
                               ray_tracer_plane_diffuse_fuzz[local_index],
                               dist,
                               next_ray_dir);
        } else if(obj_index >= box_start_index && obj_index < box_stop_index) { // ray hits box
            int local_index = obj_index - box_start_index;
            surface_eta   = ray_tracer_box_eta[local_index];
            surface_color = ray_tracer_box_color[local_index];
            reflectance   = ray_tracer_box_reflectance[local_index];
            transparency  = ray_tracer_box_transparency[local_index];
            luminosity    = ray_tracer_box_luminosity[local_index];
            ray_plane_next_ray(ray_origin,
                               ray_dir,
                               surface_point,
                               surface_normal,
                               ray_tracer_box_eta[local_index],
                               ray_tracer_box_diffuse_fuzz[local_index],
                               dist,
                               next_ray_dir);
        } else if(dist == BIG_NUMBER) { // ray hits nothing
            push_ray(ray_dir,
                     surface_eta,
                     surface_color,
                     reflectance,
                     transparency,
                     luminosity,
                     normal_projection,
                     phong,
                     dist);
            continue;
        } else { // unhandled situation
            gl_FragColor = vec4(1, 0, 0, 0);
            return;
        }
        vec3 next_ray_origin = ray_origin + ray_dir * dist;
        if(ray_tracer_render_mode == 2) {
            normal_projection = dot(surface_normal, normalize(ray_origin - next_ray_origin));
            if(dist != BIG_NUMBER && // ray hits something
               luminosity == 0)      // non-glowing material
            {
                if(abs(surface_eta) == BIG_NUMBER || // opaque material
                   normal_projection >= 0)           // frontface
                {
                    phong = ray_phong(ray_origin,
                                      next_ray_origin,
                                      surface_normal);
                } else { // backface
                    phong = ray_phong(ray_origin,
                                      next_ray_origin,
                                      -surface_normal);
                }
            } else {
                phong = vec3(0);
            }
        }
        push_ray(ray_dir,
                 surface_eta,
                 surface_color,
                 reflectance,
                 transparency,
                 luminosity,
                 normal_projection,
                 phong,
                 dist);
        ray_origin = next_ray_origin;
        ray_dir    = next_ray_dir;
        //if(ray_tracer_render_mode != 2) {
        //    break; // use information gathered after first bounce
        //}
    }

    // color determined by ray direction after first bounce
    if(ray_tracer_render_mode == 1) {
        gl_FragColor = vec4(next_ray_dir, 1);
        return;
    }

    // color determined by accumulated samples after multiple bounces
    if(ray_tracer_render_mode == 2) {
        vec3 accum_color = vec3(0);
        while(pop_ray(ray_dir,
                      surface_eta,
                      surface_color,
                      reflectance,
                      transparency,
                      luminosity,
                      normal_projection,
                      phong,
                      dist) >= 0) // calculate color forward from skybox to camera
        {
            if(dist == BIG_NUMBER) { // ray hits nothing
                sample_env_map(ray_dir, env_map_texture, accum_color); // sample color from direction in skybox
            } else { // ray hits something
                if(luminosity > 0) { // glowing material
                    accum_color = surface_color * luminosity;
                } else { // non-glowing material
                    if(abs(surface_eta) == BIG_NUMBER) { // opaque material
                        accum_color = mix(surface_color, accum_color + phong, reflectance);
                    } else { // transparent material
                        if(normal_projection < 0) { // backface
                            accum_color = mix(accum_color, surface_color, clamp(dist / FOG_DISTANCE, 0, 1) * transparency);
                        } else { // frontface
                            accum_color += phong * reflectance;
                        }
                    }
                }
            }
        }
        if(color_texture_source == -1) {
            gl_FragColor = vec4(accum_color, 1);
            return;
        }
        vec3 prev_frame_color;
        if(color_texture_source == 0) {
            prev_frame_color = texture2D(color_texture, lerp_texcoord).rgb;
        } else {
            prev_frame_color = texture2D(color_texture2, lerp_texcoord).rgb;
        }
        gl_FragColor = vec4(mix(prev_frame_color, accum_color, COLOR_DRIFT_RATIO), 1);
        return;
    }

    // color determined by material property on contact or ray direction after first bounce
    vec3 color;
    if(dist_stack[0] == BIG_NUMBER) { // ray hits nothing
        sample_env_map(next_ray_dir, env_map_texture, color); // sample color from direction in skybox
    } else if(surface_eta_stack[0] == -BIG_NUMBER || luminosity_stack[0] > 0) { // diffuse or glowing material
        color = surface_color_stack[0];
    } else { // reflective or transparent material
        sample_env_map(next_ray_dir, env_map_texture, color); // sample color from direction in skybox
    }
    gl_FragColor = vec4(color, 1);
}
