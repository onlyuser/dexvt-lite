#ifndef VT_MODIFIERS_H_
#define VT_MODIFIERS_H_

#include <glm/glm.hpp>

namespace vt {

class MeshBase;
class Scene;

enum tessellation_t {
    TESSELLATION_EDGE_CENTER,
    TESSELLATION_TRI_CENTER
};

void mesh_attach(Scene* scene, MeshBase* mesh1, MeshBase* mesh2);
void mesh_apply_ripple(MeshBase* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase, bool smooth);
void mesh_tessellate(MeshBase* mesh, tessellation_t tessellation, bool smooth);

}

#endif
