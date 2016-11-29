#include <Mesh.h>
#include <NamedObject.h>
#include <Buffer.h>
#include <Material.h>
#include <Texture.h>
#include <Util.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <iostream>

namespace vt {

Mesh::Mesh(
        std::string name,
        size_t      num_vertex,
        size_t      num_tri)
    : NamedObject(name),
      m_num_vertex(num_vertex),
      m_num_tri(num_tri),
      m_visible(true),
      m_vbo_vert_coords(NULL),
      m_vbo_vert_normal(NULL),
      m_vbo_vert_tangent(NULL),
      m_vbo_tex_coords(NULL),
      m_ibo_tri_indices(NULL),
      m_buffers_already_init(false),
      m_material(NULL),
      m_shader_context(NULL),
      m_normal_shader_context(NULL),
      m_wireframe_shader_context(NULL),
      m_ssao_shader_context(NULL),
      m_texture_index(-1),
      m_texture2_index(-1),
      m_bump_texture_index(-1),
      m_env_map_texture_index(-1),
      m_random_texture_index(-1),
      m_frontface_depth_overlay_texture_index(-1),
      m_reflect_to_refract_ratio(1) // 100% reflective
{
    m_vert_coords   = new GLfloat[num_vertex*3];
    m_vert_normal   = new GLfloat[num_vertex*3];
    m_vert_tangent  = new GLfloat[num_vertex*3];
    m_tex_coords    = new GLfloat[num_vertex*2];
    m_tri_indices   = new GLushort[num_tri*3];
    m_ambient_color = new GLfloat[3];
    m_ambient_color[0] = 1;
    m_ambient_color[1] = 1;
    m_ambient_color[2] = 1;
}

Mesh::~Mesh()
{
    if(m_vert_coords)              { delete []m_vert_coords; }
    if(m_vert_normal)              { delete []m_vert_normal; }
    if(m_vert_tangent)             { delete []m_vert_tangent; }
    if(m_tex_coords)               { delete []m_tex_coords; }
    if(m_tri_indices)              { delete []m_tri_indices; }
    if(m_ambient_color)            { delete []m_ambient_color; }
    if(m_vbo_vert_coords)          { delete m_vbo_vert_coords; }
    if(m_vbo_vert_normal)          { delete m_vbo_vert_normal; }
    if(m_vbo_vert_tangent)         { delete m_vbo_vert_tangent; }
    if(m_vbo_tex_coords)           { delete m_vbo_tex_coords; }
    if(m_ibo_tri_indices)          { delete m_ibo_tri_indices; }
    if(m_shader_context)           { delete m_shader_context; }
    if(m_normal_shader_context)    { delete m_normal_shader_context; }
    if(m_wireframe_shader_context) { delete m_wireframe_shader_context; }
    if(m_ssao_shader_context)      { delete m_ssao_shader_context; }
}

void Mesh::set_origin(glm::vec3 origin)
{
    m_origin = origin;
    set_need_update_xform();
}

void Mesh::set_orient(glm::vec3 orient)
{
    m_orient = orient;
    set_need_update_xform();
}

glm::vec3 Mesh::get_vert_coord(int index) const
{
    int offset = index*3;
    return glm::vec3(
            m_vert_coords[offset+0],
            m_vert_coords[offset+1],
            m_vert_coords[offset+2]);
}

void Mesh::set_vert_coord(int index, glm::vec3 coord)
{
    int offset = index*3;
    m_vert_coords[offset+0] = coord.x;
    m_vert_coords[offset+1] = coord.y;
    m_vert_coords[offset+2] = coord.z;
}

glm::vec3 Mesh::get_vert_normal(int index) const
{
    int offset = index*3;
    return glm::vec3(
            m_vert_normal[offset+0],
            m_vert_normal[offset+1],
            m_vert_normal[offset+2]);
}

void Mesh::set_vert_normal(int index, glm::vec3 normal)
{
    int offset = index*3;
    m_vert_normal[offset+0] = normal.x;
    m_vert_normal[offset+1] = normal.y;
    m_vert_normal[offset+2] = normal.z;
}

glm::vec3 Mesh::get_vert_tangent(int index) const
{
    int offset = index*3;
    return glm::vec3(
            m_vert_tangent[offset+0],
            m_vert_tangent[offset+1],
            m_vert_tangent[offset+2]);
}

void Mesh::set_vert_tangent(int index, glm::vec3 tangent)
{
    int offset = index*3;
    m_vert_tangent[offset+0] = tangent.x;
    m_vert_tangent[offset+1] = tangent.y;
    m_vert_tangent[offset+2] = tangent.z;
}

glm::vec2 Mesh::get_tex_coord(int index) const
{
    int offset = index*2;
    return glm::vec2(
            m_tex_coords[offset+0],
            m_tex_coords[offset+1]);
}

void Mesh::set_tex_coord(int index, glm::vec2 coord)
{
    int offset = index*2;
    m_tex_coords[offset+0] = coord.x;
    m_tex_coords[offset+1] = coord.y;
}

glm::uvec3 Mesh::get_tri_indices(int index) const
{
    int offset = index*3;
    return glm::uvec3(
            m_tri_indices[offset+0],
            m_tri_indices[offset+1],
            m_tri_indices[offset+2]);
}

void Mesh::set_tri_indices(int index, glm::uvec3 indices)
{
    int offset = index*3;
    m_tri_indices[offset+0] = indices[0];
    m_tri_indices[offset+1] = indices[1];
    m_tri_indices[offset+2] = indices[2];
}

void Mesh::update_bbox()
{
    m_min = m_max = get_vert_coord(0);
    for(int i = 1; i < static_cast<int>(m_num_vertex); i++) {
        glm::vec3 cur = get_vert_coord(i);
        m_max = glm::max(m_max, cur);
        m_min = glm::min(m_min, cur);
    }
}

void Mesh::xform_vertices(glm::mat4 xform)
{
    for(int i = 0; i < static_cast<int>(m_num_vertex); i++) {
        set_vert_coord(i, glm::vec3(xform*glm::vec4(get_vert_coord(i), 1)));
    }
    update_bbox();
}

void Mesh::update_normals_and_tangents()
{
    for(int i=0; i<static_cast<int>(m_num_tri); i++) {
        glm::uvec3 tri_indices = get_tri_indices(i);
        glm::vec3 p0 = get_vert_coord(tri_indices[0]);
        glm::vec3 p1 = get_vert_coord(tri_indices[1]);
        glm::vec3 p2 = get_vert_coord(tri_indices[2]);
        glm::vec3 e1 = glm::normalize(p1-p0);
        glm::vec3 e2 = glm::normalize(p2-p0);
        glm::vec3 n = glm::normalize(glm::cross(e1, e2));
        for(int j=0; j<3; j++) {
            set_vert_normal( tri_indices[j], n);
            set_vert_tangent(tri_indices[j], e1);
        }
    }
}

// NOTE: strangely required by pure virtual (already defined in base class!)
void Mesh::get_min_max(glm::vec3* min, glm::vec3* max) const
{
    BBoxObject::get_min_max(min, max);
}

void Mesh::init_buffers()
{
    if(m_buffers_already_init) {
        return;
    }
    m_vbo_vert_coords  = new Buffer(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_num_vertex*3, m_vert_coords);
    m_vbo_vert_normal  = new Buffer(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_num_vertex*3, m_vert_normal);
    m_vbo_vert_tangent = new Buffer(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_num_vertex*3, m_vert_tangent);
    m_vbo_tex_coords   = new Buffer(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_num_vertex*2, m_tex_coords);
    m_ibo_tri_indices  = new Buffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*m_num_tri*3, m_tri_indices);
    m_buffers_already_init = true;
}

void Mesh::update_buffers() const
{
    if(!m_buffers_already_init) {
        return;
    }
    m_vbo_vert_coords->update();
    m_vbo_vert_normal->update();
    m_vbo_vert_tangent->update();
    m_vbo_tex_coords->update();
    m_ibo_tri_indices->update();
}

Buffer* Mesh::get_vbo_vert_coords()
{
    init_buffers();
    return m_vbo_vert_coords;
}

Buffer* Mesh::get_vbo_vert_normal()
{
    init_buffers();
    return m_vbo_vert_normal;
}

Buffer* Mesh::get_vbo_vert_tangent()
{
    init_buffers();
    return m_vbo_vert_tangent;
}

Buffer* Mesh::get_vbo_tex_coords()
{
    init_buffers();
    return m_vbo_tex_coords;
}

Buffer* Mesh::get_ibo_tri_indices()
{
    init_buffers();
    return m_ibo_tri_indices;
}

void Mesh::set_material(Material* material)
{
    // NOTE: texture index for same texture varies from material to material
    if(material == m_material) {
        return;
    }
    std::string texture_name;
    if(m_material) {
        vt::Texture* texture = m_material->get_texture_by_index(m_texture_index);
        if(texture) {
            texture_name = texture->name();
        }
    }
    if(m_shader_context) {
        delete m_shader_context;
        m_shader_context = NULL;
    }
    m_material = material;
    m_texture_index = material ? material->get_texture_index_by_name(texture_name) : -1;
}

ShaderContext* Mesh::get_shader_context()
{
    if(m_shader_context || !m_material) { // FIX-ME! -- potential bug if Material not set
        return m_shader_context;
    }
    m_shader_context = new ShaderContext(
            m_material,
            get_vbo_vert_coords(),
            get_vbo_vert_normal(),
            get_vbo_vert_tangent(),
            get_vbo_tex_coords(),
            get_ibo_tri_indices());
    return m_shader_context;
}

ShaderContext* Mesh::get_normal_shader_context(Material* normal_material)
{
    if(m_normal_shader_context || !normal_material) { // FIX-ME! -- potential bug if Material not set
        return m_normal_shader_context;
    }
    m_normal_shader_context = new ShaderContext(
            normal_material,
            get_vbo_vert_coords(),
            get_vbo_vert_normal(),
            get_vbo_vert_tangent(),
            get_vbo_tex_coords(),
            get_ibo_tri_indices());
    return m_normal_shader_context;
}

ShaderContext* Mesh::get_wireframe_shader_context(Material* wireframe_material)
{
    if(m_wireframe_shader_context || !wireframe_material) { // FIX-ME! -- potential bug if Material not set
        return m_wireframe_shader_context;
    }
    m_wireframe_shader_context = new ShaderContext(
            wireframe_material,
            get_vbo_vert_coords(),
            get_vbo_vert_normal(),
            get_vbo_vert_tangent(),
            get_vbo_tex_coords(),
            get_ibo_tri_indices());
    return m_wireframe_shader_context;
}

ShaderContext* Mesh::get_ssao_shader_context(Material* ssao_material)
{
    if(m_ssao_shader_context || !ssao_material) { // FIX-ME! -- potential bug if Material not set
        return m_ssao_shader_context;
    }
    m_ssao_shader_context = new ShaderContext(
            ssao_material,
            get_vbo_vert_coords(),
            get_vbo_vert_normal(),
            get_vbo_vert_tangent(),
            get_vbo_tex_coords(),
            get_ibo_tri_indices());
    return m_ssao_shader_context;
}

glm::vec3 Mesh::get_ambient_color() const
{
    return glm::vec3(
            m_ambient_color[0],
            m_ambient_color[1],
            m_ambient_color[2]);
}

void Mesh::set_ambient_color(glm::vec3 ambient_color)
{
    m_ambient_color[0] = ambient_color.r;
    m_ambient_color[1] = ambient_color.g;
    m_ambient_color[2] = ambient_color.b;
}

void Mesh::set_axis(glm::vec3 axis)
{
    glm::vec3 local_axis = glm::vec3(glm::inverse(get_xform()) * glm::vec4(axis, 1));
    for(int i = 0; i < static_cast<int>(m_num_vertex); i++) {
        set_vert_coord(i, get_vert_coord(i) - local_axis);
    }
    update_normals_and_tangents();
    update_bbox();
    if(get_parent()) {
        m_origin = glm::vec3(glm::inverse(get_parent()->get_xform()) * glm::vec4(axis, 1));
    } else {
        m_origin = axis;
    }
    set_need_update_xform();
}

void Mesh::center_axis(align_t align)
{
    set_axis(glm::vec3(get_xform() * glm::vec4(get_center(align), 1)));
}

void Mesh::point_at(glm::vec3 p)
{
    glm::vec3 local_p;
    if(get_parent()) {
        local_p = glm::vec3(glm::inverse(get_parent()->get_xform()) * glm::vec4(p, 1));
    } else {
        local_p = p;
    }
    m_orient = offset_to_orient(local_p - m_origin);
    set_need_update_xform();
}

void Mesh::update_xform()
{
    glm::mat4 translate_xform = glm::translate(glm::mat4(1), m_origin);
    glm::mat4 rotate_xform =
            GLM_ROTATE(glm::mat4(1), static_cast<float>(ORIENT_YAW(m_orient)),   VEC_UP) *     // Y axis
            GLM_ROTATE(glm::mat4(1), static_cast<float>(ORIENT_PITCH(m_orient)), VEC_LEFT) *   // X axis
            GLM_ROTATE(glm::mat4(1), static_cast<float>(ORIENT_ROLL(m_orient)),  VEC_FORWARD); // Z axis
    glm::mat4 scale_xform = glm::scale(glm::mat4(1), m_scale);
    m_xform = translate_xform * rotate_xform * scale_xform;
}

MeshIFace* alloc_meshiface(std::string name, size_t num_vertex, size_t num_tri)
{
    return new Mesh(name, num_vertex, num_tri);
}

Mesh* downcast_meshiface_to_mesh(MeshIFace* mesh)
{
    return dynamic_cast<Mesh*>(mesh);
}

MeshIFace* upcast_mesh_to_meshiface(Mesh* mesh)
{
    return dynamic_cast<MeshIFace*>(mesh);
}

}
