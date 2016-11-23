#ifndef VT_MESH_IFACE_H_
#define VT_MESH_IFACE_H_

#include <glm/glm.hpp>

namespace vt {

class MeshIFace
{
public:
    virtual size_t     get_num_vertex() const = 0;
    virtual size_t     get_num_tri() const = 0;
    virtual glm::vec3  get_vert_coord(int index) const = 0;
    virtual void       set_vert_coord(int index, glm::vec3 coord) = 0;
    virtual glm::vec3  get_vert_normal(int index) const = 0;
    virtual void       set_vert_normal(int index, glm::vec3 normal) = 0;
    virtual glm::vec3  get_vert_tangent(int index) const = 0;
    virtual void       set_vert_tangent(int index, glm::vec3 tangent) = 0;
    virtual glm::vec2  get_tex_coord(int index) const = 0;
    virtual void       set_tex_coord(int index, glm::vec2 coord) = 0;
    virtual glm::uvec3 get_tri_indices(int index) const = 0;
    virtual void       set_tri_indices(int index, glm::uvec3 indices) = 0;
    virtual void       update_bbox() = 0;
    virtual void       update_normals_and_tangents() = 0;
};

}

#endif
