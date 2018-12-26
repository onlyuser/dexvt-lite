uniform samplerCube env_map_texture;
varying mat3 lerp_tbn_transform;
varying vec3 lerp_camera_vector;

void sample_env_map(in    vec3        ray_direction,
                    in    samplerCube env_map_texture,
                    inout vec4        env_color)
{
    vec3 flipped_cubemap_texcoord = vec3(ray_direction.x, -ray_direction.y, ray_direction.z);
    env_color = textureCube(env_map_texture, flipped_cubemap_texcoord);
}

void reflect_into_env_map(in    vec3        ray_direction,
                          in    vec3        surface_normal,
                          in    samplerCube env_map_texture,
                          inout vec4        reflected_color)
{
    vec3 reflected_camera_dir = reflect(ray_direction, surface_normal);
    sample_env_map(reflected_camera_dir, env_map_texture, reflected_color);
}

void main()
{
    vec3 camera_direction = normalize(lerp_camera_vector);

    vec3 normal_surface = vec3(0, 0, 1);
    vec3 normal = normalize(lerp_tbn_transform * normal_surface);

    // reflection component
    vec4 reflected_color;
    reflect_into_env_map(-camera_direction, normal, env_map_texture, reflected_color);

    gl_FragColor = reflected_color;
}
