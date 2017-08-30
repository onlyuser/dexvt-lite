#include <PrimitiveFactory.h>
#include <Modifiers.h>
#include <MeshBase.h>
#include <Util.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <assert.h>

namespace vt {

MeshBase* alloc_mesh_base(std::string name, size_t num_vertex, size_t num_tri);

class Mesh;

Mesh* cast_mesh(MeshBase* mesh);
MeshBase* cast_mesh_base(Mesh* mesh);

Mesh* PrimitiveFactory::create_grid(std::string name,
                                    int         cols,
                                    int         rows,
                                    float       width,
                                    float       length,
                                    float       tex_width_scale,
                                    float       tex_length_scale)
{
    int       num_vertex = (rows + 1) * (cols + 1);
    int       num_tri    = rows * cols * 2;
    MeshBase* mesh       = alloc_mesh_base(name, num_vertex, num_tri);

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    int vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            mesh->set_vert_coord( vert_index, glm::vec3(
                    width * (static_cast<float>(col) / cols),
                    0,
                    length * (1 - static_cast<float>(row) / rows)));
            mesh->set_vert_normal( vert_index, glm::vec3(0, 1, 0));
            mesh->set_vert_tangent(vert_index, glm::vec3(1, 0, 0));
            vert_index++;
        }
    }

    // ========================
    // init mesh texture coords
    // ========================

    int tex_vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            mesh->set_tex_coord(tex_vert_index++, glm::vec2(
                    static_cast<float>(col) / cols / tex_width_scale,
                    1 - static_cast<float>(row) / rows / tex_length_scale));
        }
    }

    // ==========================
    // init mesh triangle indices
    // ==========================

    int tri_index = 0;
    for(int row = 0; row < rows; row++) {
        for(int col = 0; col < cols; col++) {
            int lower_left  = row * (cols + 1) + col;
            int lower_right = lower_left + 1;
            int upper_left  = (row + 1) * (cols + 1) + col;
            int upper_right = upper_left + 1;
            mesh->set_tri_indices(tri_index++, glm::ivec3(lower_left, lower_right, upper_right));
            mesh->set_tri_indices(tri_index++, glm::ivec3(upper_right, upper_left, lower_left));
        }
    }

    mesh->update_bbox();

    assert(vert_index == num_vertex);
    assert(tri_index  == num_tri);
    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_sphere(std::string name,
                                      int         slices,
                                      int         stacks,
                                      float       radius)
{
    int       cols = slices;
    int       rows = stacks;
    MeshBase* mesh = cast_mesh_base(create_grid(name, cols, rows));

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    int vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            glm::vec3 normal = euler_to_offset(glm::vec3(
                    0,
                    -(static_cast<float>(row) / rows * 180 - 90), // pitch
                    static_cast<float>(col) / cols * 360));       // yaw
            glm::vec3 offset = normal * radius;
            mesh->set_vert_coord(  vert_index, offset);
            mesh->set_vert_normal( vert_index, glm::normalize(offset));
            mesh->set_vert_tangent(vert_index, euler_to_offset(glm::vec3(
                    0,
                    0,                                            // pitch
                    static_cast<float>(col) / cols * 360 + 90))); // yaw
            vert_index++;
        }
    }

    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_hemisphere(std::string name,
                                          int         slices,
                                          int         stacks,
                                          float       radius)
{
    int       cols = slices;
    int       rows = stacks * 0.5 + 2;
    MeshBase* mesh = cast_mesh_base(create_grid(name, cols, rows));

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    int vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            switch(row) {
                case 0: // bottom
                    mesh->set_vert_coord( vert_index, glm::vec3(0,  0, 0));
                    mesh->set_vert_normal(vert_index, glm::vec3(0, -1, 0));
                    break;
                default:
                    {
                        glm::vec3 offset = euler_to_offset(glm::vec3(
                                0,
                                (row == 1) ? 0 : -(static_cast<float>(row - 2) / (rows - 2) * 90), // pitch
                                static_cast<float>(col) / cols * 360))                             // yaw
                                *radius;
                        mesh->set_vert_coord( vert_index, offset);
                        mesh->set_vert_normal(vert_index, (row == 1) ?
                                glm::vec3(0, -1, 0) : glm::normalize(offset));
                    }
                    break;
            }
            mesh->set_vert_tangent(vert_index, euler_to_offset(glm::vec3(
                    0,
                    0,                                            // pitch
                    static_cast<float>(col) / cols * 360 + 90))); // yaw
            vert_index++;
        }
    }

    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_cylinder(std::string name,
                                        int         slices,
                                        float       radius,
                                        float       height)
{
    int       cols = slices;
    int       rows = 5;
    MeshBase* mesh = cast_mesh_base(create_grid(name, cols, rows));

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    int vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            switch(row) {
                case 0: // bottom
                    mesh->set_vert_coord( vert_index, glm::vec3(0,  0, 0));
                    mesh->set_vert_normal(vert_index, glm::vec3(0, -1, 0));
                    break;
                case 1: // bottom rim
                case 2: // bottom side rim
                    {
                        glm::vec3 offset = euler_to_offset(glm::vec3(
                                0,
                                0,                                     // pitch
                                static_cast<float>(col) / cols * 360)) // yaw
                                *radius;
                        mesh->set_vert_coord( vert_index, glm::vec3(offset.x, 0, offset.z));
                        mesh->set_vert_normal(vert_index, (row == 1) ?
                                glm::vec3(0, -1, 0) : glm::normalize(offset));
                    }
                    break;
                case 3: // top side rim
                case 4: // top rim
                    {
                        glm::vec3 offset = euler_to_offset(glm::vec3(
                                0,
                                0,                                     // pitch
                                static_cast<float>(col) / cols * 360)) // yaw
                                *radius;
                        mesh->set_vert_coord( vert_index, glm::vec3(offset.x, height, offset.z));
                        mesh->set_vert_normal(vert_index, (row == 4) ?
                                glm::vec3(0, 1, 0) : glm::normalize(offset));
                    }
                    break;
                case 5: // top
                    mesh->set_vert_coord( vert_index, glm::vec3(0, height, 0));
                    mesh->set_vert_normal(vert_index, glm::vec3(0, 1, 0));
                    break;
            }
            mesh->set_vert_tangent(vert_index, euler_to_offset(glm::vec3(
                    0,
                    0,                                            // pitch
                    static_cast<float>(col) / cols * 360 + 90))); // yaw
            vert_index++;
        }
    }

    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_cone(std::string name,
                                    int         slices,
                                    float       radius,
                                    float       height)
{
    int       cols = slices;
    int       rows = 3;
    MeshBase* mesh = cast_mesh_base(create_grid(name, cols, rows));

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    float rim_y_offset = radius * sin(HALF_PI - atan(height / radius));
    int vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            switch(row) {
                case 0: // bottom
                    mesh->set_vert_coord( vert_index, glm::vec3(0,  0, 0));
                    mesh->set_vert_normal(vert_index, glm::vec3(0, -1, 0));
                    break;
                case 1: // bottom rim
                case 2: // side rim
                    {
                        glm::vec3 offset = euler_to_offset(glm::vec3(
                                0,
                                0,                                     // pitch
                                static_cast<float>(col) / cols * 360)) // yaw
                                *radius;
                        mesh->set_vert_coord( vert_index, offset);
                        mesh->set_vert_normal(vert_index, (row == 1) ?
                                glm::vec3(0, -1, 0) : glm::normalize(offset + glm::vec3(0, rim_y_offset, 0)));
                    }
                    break;
                case 3: // tip
                    {
                        glm::vec3 offset = euler_to_offset(glm::vec3(
                                0,
                                0,                                     // pitch
                                static_cast<float>(col) / cols * 360)) // yaw
                                *radius;
                        mesh->set_vert_coord( vert_index, glm::vec3(0, height, 0));
                        mesh->set_vert_normal(vert_index, glm::normalize(offset + glm::vec3(0, rim_y_offset, 0)));
                    }
                    break;
            }
            mesh->set_vert_tangent(vert_index, euler_to_offset(glm::vec3(
                    0,
                    0,                                            // pitch
                    static_cast<float>(col) / cols * 360 + 90))); // yaw
            vert_index++;
        }
    }

    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_torus(std::string name,
                                     int         slices,
                                     int         stacks,
                                     float       radius_major,
                                     float       radius_minor)
{
    int       cols = slices;
    int       rows = stacks;
    MeshBase* mesh = cast_mesh_base(create_grid(name, cols, rows));

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    int vert_index = 0;
    for(int row = 0; row <= rows; row++) {
        for(int col = 0; col <= cols; col++) {
            glm::vec3 normal_major = euler_to_offset(glm::vec3(
                    0,
                    0,                                      // pitch
                    static_cast<float>(col) / cols * 360)); // yaw
            glm::vec3 normal_minor = euler_to_offset(glm::vec3(
                    0,
                    -(static_cast<float>(row) / rows * 360 - 180), // pitch
                    static_cast<float>(col) / cols * 360));        // yaw
            mesh->set_vert_coord(  vert_index, normal_major * radius_major + normal_minor * radius_minor);
            mesh->set_vert_normal( vert_index, normal_minor);
            mesh->set_vert_tangent(vert_index, euler_to_offset(glm::vec3(
                    0,
                    0,                                            // pitch
                    static_cast<float>(col) / cols * 360 + 90))); // yaw
            vert_index++;
        }
    }

    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_box(std::string name,
                                   float       width,
                                   float       height,
                                   float       length)
{
    MeshBase* mesh = alloc_mesh_base(name, 24, 12);

    // ==============================
    // init mesh vertex/normal coords
    // ==============================

    // right
    mesh->set_vert_coord(0, glm::vec3(0, 0, 0));
    mesh->set_vert_coord(1, glm::vec3(0, 0, 1));
    mesh->set_vert_coord(2, glm::vec3(0, 1, 1));
    mesh->set_vert_coord(3, glm::vec3(0, 1, 0));
    for(int i = 0; i < 4; i++) {
        mesh->set_vert_normal( 0 * 4 + i, glm::vec3(-1, 0, 0));
        mesh->set_vert_tangent(0 * 4 + i, glm::vec3( 0, 0, 1));
    }

    // front
    mesh->set_vert_coord(4, glm::vec3(0, 0, 1));
    mesh->set_vert_coord(5, glm::vec3(1, 0, 1));
    mesh->set_vert_coord(6, glm::vec3(1, 1, 1));
    mesh->set_vert_coord(7, glm::vec3(0, 1, 1));
    for(int j = 0; j < 4; j++) {
        mesh->set_vert_normal( 1 * 4 + j, glm::vec3(0, 0, 1));
        mesh->set_vert_tangent(1 * 4 + j, glm::vec3(1, 0, 0));
    }

    // left
    mesh->set_vert_coord( 8, glm::vec3(1, 0, 1));
    mesh->set_vert_coord( 9, glm::vec3(1, 0, 0));
    mesh->set_vert_coord(10, glm::vec3(1, 1, 0));
    mesh->set_vert_coord(11, glm::vec3(1, 1, 1));
    for(int k = 0; k < 4; k++) {
        mesh->set_vert_normal( 2 * 4 + k, glm::vec3(1, 0,  0));
        mesh->set_vert_tangent(2 * 4 + k, glm::vec3(0, 0, -1));
    }

    // back
    mesh->set_vert_coord(12, glm::vec3(1, 0, 0));
    mesh->set_vert_coord(13, glm::vec3(0, 0, 0));
    mesh->set_vert_coord(14, glm::vec3(0, 1, 0));
    mesh->set_vert_coord(15, glm::vec3(1, 1, 0));
    for(int p = 0; p < 4; p++) {
        mesh->set_vert_normal( 3 * 4 + p, glm::vec3( 0, 0, -1));
        mesh->set_vert_tangent(3 * 4 + p, glm::vec3(-1, 0,  0));
    }

    // top
    mesh->set_vert_coord(16, glm::vec3(1, 1, 0));
    mesh->set_vert_coord(17, glm::vec3(0, 1, 0));
    mesh->set_vert_coord(18, glm::vec3(0, 1, 1));
    mesh->set_vert_coord(19, glm::vec3(1, 1, 1));
    for(int q = 0; q < 4; q++) {
        mesh->set_vert_normal( 4 * 4 + q, glm::vec3( 0, 1, 0));
        mesh->set_vert_tangent(4 * 4 + q, glm::vec3(-1, 0, 0));
    }

    // bottom
    mesh->set_vert_coord(20, glm::vec3(0, 0, 0));
    mesh->set_vert_coord(21, glm::vec3(1, 0, 0));
    mesh->set_vert_coord(22, glm::vec3(1, 0, 1));
    mesh->set_vert_coord(23, glm::vec3(0, 0, 1));
    for(int r = 0; r < 4; r++) {
        mesh->set_vert_normal( 5 * 4 + r, glm::vec3(0, -1, 0));
        mesh->set_vert_tangent(5 * 4 + r, glm::vec3(1,  0, 0));
    }

    glm::mat4 scale_transform = glm::scale(glm::mat4(1), glm::vec3(width, height, length));
    size_t num_vertex = mesh->get_num_vertex();
    for(int t = 1; t < static_cast<int>(num_vertex); t++) {
        mesh->set_vert_coord(t, glm::vec3(glm::vec4(mesh->get_vert_coord(t), 1) * scale_transform));
    }

    // ========================
    // init mesh texture coords
    // ========================

    // right
    mesh->set_tex_coord(0, glm::vec2(0, 0));
    mesh->set_tex_coord(1, glm::vec2(1, 0));
    mesh->set_tex_coord(2, glm::vec2(1, 1));
    mesh->set_tex_coord(3, glm::vec2(0, 1));

    for(int u = 1; u < 6; u++) {
        mesh->set_tex_coord(u * 4 + 0, mesh->get_tex_coord(0));
        mesh->set_tex_coord(u * 4 + 1, mesh->get_tex_coord(1));
        mesh->set_tex_coord(u * 4 + 2, mesh->get_tex_coord(2));
        mesh->set_tex_coord(u * 4 + 3, mesh->get_tex_coord(3));
    }

    // ==========================
    // init mesh triangle indices
    // ==========================

    // right
    mesh->set_tri_indices(0, glm::ivec3(0, 1, 2));
    mesh->set_tri_indices(1, glm::ivec3(2, 3, 0));

    // front
    mesh->set_tri_indices(2, glm::ivec3(4, 5, 6));
    mesh->set_tri_indices(3, glm::ivec3(6, 7, 4));

    // left
    mesh->set_tri_indices(4, glm::ivec3(8,  9, 10));
    mesh->set_tri_indices(5, glm::ivec3(10, 11, 8));

    // back
    mesh->set_tri_indices(6, glm::ivec3(12, 13, 14));
    mesh->set_tri_indices(7, glm::ivec3(14, 15, 12));

    // top
    mesh->set_tri_indices(8, glm::ivec3(16, 17, 18));
    mesh->set_tri_indices(9, glm::ivec3(18, 19, 16));

    // bottom
    mesh->set_tri_indices(10, glm::ivec3(20, 21, 22));
    mesh->set_tri_indices(11, glm::ivec3(22, 23, 20));

    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_tetrahedron(std::string name,
                                           float       width,
                                           float       height,
                                           float       length)
{
    MeshBase* mesh = alloc_mesh_base(name, 12, 4);

    // triangle opposite to origin
    mesh->set_vert_coord(0, glm::vec3(1, 0, 0));
    mesh->set_vert_coord(1, glm::vec3(0, 0, 1));
    mesh->set_vert_coord(2, glm::vec3(0, 1, 0));

    // triangle starting at x
    mesh->set_vert_coord(3, glm::vec3(1, 0, 0));
    mesh->set_vert_coord(4, glm::vec3(0, 1, 0));
    mesh->set_vert_coord(5, glm::vec3(1, 1, 1));

    // triangle starting at y
    mesh->set_vert_coord(6, glm::vec3(0, 1, 0));
    mesh->set_vert_coord(7, glm::vec3(0, 0, 1));
    mesh->set_vert_coord(8, glm::vec3(1, 1, 1));

    // triangle starting at z
    mesh->set_vert_coord(9,  glm::vec3(0, 0, 1));
    mesh->set_vert_coord(10, glm::vec3(1, 0, 0));
    mesh->set_vert_coord(11, glm::vec3(1, 1, 1));

    mesh->set_tri_indices(0, glm::ivec3(0, 1,  2));
    mesh->set_tri_indices(1, glm::ivec3(3, 4,  5));
    mesh->set_tri_indices(2, glm::ivec3(6, 7,  8));
    mesh->set_tri_indices(3, glm::ivec3(9, 10, 11));

    mesh->update_normals_and_tangents();
    mesh->update_bbox();

    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_geosphere(std::string name,
                                         float       radius,
                                         int         tessellation_iters)
{
    MeshBase* mesh = cast_mesh_base(create_sphere(name, 4, 2, radius));
    mesh->center_axis();
    for(int i = 0; i < tessellation_iters; i++) {
        mesh_tessellate(mesh, TESSELLATION_TYPE_EDGE_CENTER, true);
        size_t num_vertex = mesh->get_num_vertex();
        for(int j = 0; j < static_cast<int>(num_vertex); j++) {
            mesh->set_vert_coord(j, glm::normalize(mesh->get_vert_coord(j)) * radius);
        }
        mesh->center_axis();
    }
    return cast_mesh(mesh);
}

Mesh* PrimitiveFactory::create_diamond_brilliant_cut(std::string name,
                                                     float       radius,
                                                     float       table_radius,
                                                     float       height,
                                                     float       crown_height_to_total_height_ratio,
                                                     float       upper_girdle_height_to_crown_height_ratio,
                                                     float       lower_girdle_depth_to_pavilion_depth_ratio,
                                                     float       girdle_thick_part_thickness,
                                                     float       girdle_thin_part_thickness)
{
    // table:         8      triangles
    // star:          8      triangles
    // kite:          8 * 2  triangles
    // upper-girdle:  16     triangles
    // girdle:        16 * 2 triangles
    // lower-girdle:  16     triangles
    // pavilion main: 8 * 2  triangles
    // ===============================
    // total (faces):    16 * 7 = 112
    // total (vertices): 112 * 3 = 336

    int       num_vertex = 336;
    int       num_tri    = 112;
    MeshBase* mesh       = alloc_mesh_base(name, num_vertex, num_tri);

    float crown_height   = height * crown_height_to_total_height_ratio;
    float pavilion_depth = height - crown_height;

    float upper_girdle_inner_rim_y      = pavilion_depth + crown_height * upper_girdle_height_to_crown_height_ratio;
    float upper_girdle_inner_rim_radius = radius - (radius - table_radius) * upper_girdle_height_to_crown_height_ratio;

    float girdle_thick_part_top_y    = pavilion_depth + girdle_thick_part_thickness * 0.5;
    float girdle_thick_part_bottom_y = pavilion_depth - girdle_thick_part_thickness * 0.5;
    float girdle_thin_part_top_y     = pavilion_depth + girdle_thin_part_thickness * 0.5;
    float girdle_thin_part_bottom_y  = pavilion_depth - girdle_thin_part_thickness * 0.5;

    float lower_girdle_inner_rim_y      = pavilion_depth * (1 - lower_girdle_depth_to_pavilion_depth_ratio);
    float lower_girdle_inner_rim_radius = radius * lower_girdle_depth_to_pavilion_depth_ratio;

    upper_girdle_inner_rim_radius /= cos(PI * 0.125);
    lower_girdle_inner_rim_radius /= cos(PI * 0.125);

    int vert_index = 0;
    int tri_index  = 0;

    // table: 8 triangles
    for(int i = 0; i < 8; i++) {
        glm::vec3 p1 = glm::vec3(0, height, 0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * table_radius
                        + glm::vec3(0, height, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * table_radius
                        + glm::vec3(0, height, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    // star: 8 triangles
    for(int i = 0; i < 8; i++) {
        glm::vec3 p1 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * table_radius
                        + glm::vec3(0, height, 0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * upper_girdle_inner_rim_radius
                        + glm::vec3(0, upper_girdle_inner_rim_y, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * table_radius
                        + glm::vec3(0, height, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    // kite: 8 * 2 triangles
    for(int i = 0; i < 8; i++) {
        // half kite 1
        glm::vec3 p1 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * table_radius
                        + glm::vec3(0, height, 0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_top_y, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * upper_girdle_inner_rim_radius
                        + glm::vec3(0, upper_girdle_inner_rim_y, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;

        // half kite 2
        glm::vec3 p4 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * table_radius
                        + glm::vec3(0, height, 0);
        glm::vec3 p5 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * upper_girdle_inner_rim_radius
                        + glm::vec3(0, upper_girdle_inner_rim_y, 0);
        glm::vec3 p6 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_top_y, 0);
        mesh->set_vert_coord( vert_index + 0, p4);
        mesh->set_vert_coord( vert_index + 1, p5);
        mesh->set_vert_coord( vert_index + 2, p6);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    // upper-girdle: 16 triangles
    for(int i = 0; i < 8; i++) {
        // half upper-girdle 1
        glm::vec3 p1 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * upper_girdle_inner_rim_radius
                        + glm::vec3(0, upper_girdle_inner_rim_y, 0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_top_y, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thin_part_top_y, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;

        // half upper-girdle 2
        glm::vec3 p4 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * upper_girdle_inner_rim_radius
                        + glm::vec3(0, upper_girdle_inner_rim_y, 0);
        glm::vec3 p5 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thin_part_top_y, 0);
        glm::vec3 p6 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_top_y, 0);
        mesh->set_vert_coord( vert_index + 0, p4);
        mesh->set_vert_coord( vert_index + 1, p5);
        mesh->set_vert_coord( vert_index + 2, p6);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    // girdle: 16 * 2 triangles
    for(int i = 0; i < 16; i++) {
        float left_top_y     = (i % 2)       ? girdle_thin_part_top_y    : girdle_thick_part_top_y;
        float left_bottom_y  = (i % 2)       ? girdle_thin_part_bottom_y : girdle_thick_part_bottom_y;
        float right_top_y    = ((i + 1) % 2) ? girdle_thin_part_top_y    : girdle_thick_part_top_y;
        float right_bottom_y = ((i + 1) % 2) ? girdle_thin_part_bottom_y : girdle_thick_part_bottom_y;

        // top half
        glm::vec3 p1 = euler_to_offset(glm::vec3(
                0,
                0,                                     // pitch
                static_cast<float>(i + 1) / 16 * 360)) // yaw
                        * radius
                        + glm::vec3(0, right_top_y, 0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                 // pitch
                static_cast<float>(i) / 16 * 360)) // yaw
                        * radius
                        + glm::vec3(0, left_top_y, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                 // pitch
                static_cast<float>(i) / 16 * 360)) // yaw
                        * radius
                        + glm::vec3(0, left_bottom_y, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;

        // bottom half
        glm::vec3 p4 = euler_to_offset(glm::vec3(
                0,
                0,                                 // pitch
                static_cast<float>(i) / 16 * 360)) // yaw
                        * radius
                        + glm::vec3(0, left_bottom_y, 0);
        glm::vec3 p5 = euler_to_offset(glm::vec3(
                0,
                0,                                     // pitch
                static_cast<float>(i + 1) / 16 * 360)) // yaw
                        * radius
                        + glm::vec3(0, right_bottom_y, 0);
        glm::vec3 p6 = euler_to_offset(glm::vec3(
                0,
                0,                                     // pitch
                static_cast<float>(i + 1) / 16 * 360)) // yaw
                        * radius
                        + glm::vec3(0, right_top_y, 0);
        mesh->set_vert_coord( vert_index + 0, p4);
        mesh->set_vert_coord( vert_index + 1, p5);
        mesh->set_vert_coord( vert_index + 2, p6);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    // lower-girdle: 16 triangles
    for(int i = 0; i < 8; i++) {
        // half lower-girdle 1
        glm::vec3 p1 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * lower_girdle_inner_rim_radius
                        + glm::vec3(0, lower_girdle_inner_rim_y, 0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thin_part_bottom_y, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_bottom_y, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;

        // half lower-girdle 2
        glm::vec3 p4 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * lower_girdle_inner_rim_radius
                        + glm::vec3(0, lower_girdle_inner_rim_y, 0);
        glm::vec3 p5 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_bottom_y, 0);
        glm::vec3 p6 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thin_part_bottom_y, 0);
        mesh->set_vert_coord( vert_index + 0, p4);
        mesh->set_vert_coord( vert_index + 1, p5);
        mesh->set_vert_coord( vert_index + 2, p6);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    // pavilion main: 8 * 2 triangles
    for(int i = 0; i < 8; i++) {
        // half pavilion main 1
        glm::vec3 p1 = glm::vec3(0);
        glm::vec3 p2 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * lower_girdle_inner_rim_radius
                        + glm::vec3(0, lower_girdle_inner_rim_y, 0);
        glm::vec3 p3 = euler_to_offset(glm::vec3(
                0,
                0,                                // pitch
                static_cast<float>(i) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_bottom_y, 0);
        mesh->set_vert_coord( vert_index + 0, p1);
        mesh->set_vert_coord( vert_index + 1, p2);
        mesh->set_vert_coord( vert_index + 2, p3);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;

        // half pavilion main 2
        glm::vec3 p4 = glm::vec3(0);
        glm::vec3 p5 = euler_to_offset(glm::vec3(
                0,
                0,                                    // pitch
                static_cast<float>(i + 1) / 8 * 360)) // yaw
                        * radius
                        + glm::vec3(0, girdle_thick_part_bottom_y, 0);
        glm::vec3 p6 = euler_to_offset(glm::vec3(
                0,
                0,                                      // pitch
                static_cast<float>(i + 0.5) / 8 * 360)) // yaw
                        * lower_girdle_inner_rim_radius
                        + glm::vec3(0, lower_girdle_inner_rim_y, 0);
        mesh->set_vert_coord( vert_index + 0, p4);
        mesh->set_vert_coord( vert_index + 1, p5);
        mesh->set_vert_coord( vert_index + 2, p6);
        mesh->set_tri_indices(tri_index++, glm::ivec3(vert_index + 0, vert_index + 1, vert_index + 2));
        vert_index += 3;
    }

    mesh->update_normals_and_tangents();
    mesh->update_bbox();

    assert(vert_index == num_vertex);
    assert(tri_index  == num_tri);
    return cast_mesh(mesh);
}

}
