const float MAX_DIST = 20;
const float MAX_DIST_SQUARED = MAX_DIST*MAX_DIST;
const int NUM_LIGHTS = 8;
const int SPECULAR_SHARPNESS = 16;
uniform int light_count;
uniform int light_enabled[NUM_LIGHTS];
uniform vec3 ambient_color;
uniform vec3 light_color[NUM_LIGHTS];
uniform vec3 light_pos[NUM_LIGHTS];
varying vec3 lerp_camera_vector;
varying vec3 lerp_normal;
varying vec3 lerp_position_world;

void main(void) {
    vec3 diffuse_sum = vec3(0.0, 0.0, 0.0);
    vec3 specular_sum = vec3(0.0, 0.0, 0.0);

    vec3 camera_direction = normalize(lerp_camera_vector);

    vec3 normal = normalize(lerp_normal);

    for(int i = 0; i < NUM_LIGHTS && i < light_count; i++) {
        if(light_enabled[i] == 0) {
            continue;
        }
        vec3 light_vector = light_pos[i] - lerp_position_world;

        float dist = min(dot(light_vector, light_vector), MAX_DIST_SQUARED)/MAX_DIST_SQUARED;
        float distance_factor = 1.0 - dist;

        vec3 light_direction = normalize(light_vector);
        float diffuse_per_light = dot(normal, light_direction);
        diffuse_sum += light_color[i]*clamp(diffuse_per_light, 0.0, 1.0)*distance_factor;

        vec3 half_angle = normalize(camera_direction + light_direction);
        vec3 specular_color = min(light_color[i] + 0.5, 1.0);
        float specular_per_light = dot(normal, half_angle);
        specular_sum += specular_color*pow(clamp(specular_per_light, 0.0, 1.0), SPECULAR_SHARPNESS)*distance_factor;
    }

    vec4 sample = vec4(1);
    gl_FragColor = vec4(clamp(sample.rgb*(diffuse_sum + ambient_color) + specular_sum, 0.0, 1.0), sample.a);
}