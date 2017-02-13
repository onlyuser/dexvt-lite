#ifndef VT_MESH_IFACE_H_
#define VT_MESH_IFACE_H_

#include <BBoxObject.h>
#include <glm/glm.hpp>

namespace vt {

class MeshIFace
{
public:
    virtual ~MeshIFace() {}
    virtual void       resize(size_t num_vertex, size_t num_tri, bool preserve_mesh_geometry = false) = 0;
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
    virtual glm::ivec3 get_tri_indices(int index) const = 0;
    virtual void       set_tri_indices(int index, glm::ivec3 indices) = 0;
    virtual void       update_bbox() = 0;
    virtual void       update_normals_and_tangents() = 0;
    virtual void       get_min_max(glm::vec3* min, glm::vec3* max) const = 0;
    virtual void       set_axis(glm::vec3 axis) = 0;
    virtual void       center_axis(BBoxObject::align_t align = BBoxObject::ALIGN_CENTER) = 0;
};

}

#endif
