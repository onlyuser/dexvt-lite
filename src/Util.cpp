#include <Util.h>
#include <Mesh.h>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <math.h>

namespace vt {

glm::vec3 orient_to_offset(glm::vec3 orient)
{
    glm::mat4 pitch = GLM_ROTATE(
            glm::mat4(1),
            ORIENT_PITCH(orient), VEC_LEFT);
    glm::mat4 yaw = GLM_ROTATE(
            glm::mat4(1),
            ORIENT_YAW(orient), VEC_UP);

    return glm::vec3(yaw*pitch*glm::vec4(VEC_FORWARD, 1));
}

glm::vec3 offset_to_orient(glm::vec3 offset)
{
    offset = glm::normalize(offset);
    glm::vec3 t(offset.x, 0, offset.z); // flattened offset
    t = glm::normalize(t);
    glm::vec3 r(
        0,
        glm::angle(t, offset),
        glm::angle(t, VEC_FORWARD));
    if(static_cast<float>(fabs(offset.x)) < EPSILON && static_cast<float>(fabs(offset.z)) < EPSILON) {
        ORIENT_PITCH(r) = -SIGN(offset.y)*glm::radians(90.0f);
        ORIENT_YAW(r) = 0; // undefined
        return r;
    }
    if(offset.x < 0) ORIENT_YAW(r)   *= -1;
    if(offset.y > 0) ORIENT_PITCH(r) *= -1;
    return r;
}

void mesh_apply_ripple(Mesh* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase)
{
    for(int i = 0; i < static_cast<int>(mesh->get_num_vertex()); i++) {
        glm::vec3 pos = mesh->get_vert_coord(i);
        glm::vec3 new_pos = pos;
        new_pos.y = origin.y +
                static_cast<float>(sin(glm::distance(
                        glm::vec2(origin.x, origin.z),
                        glm::vec2(pos.x, pos.z))/(wavelength/(PI*2)) + phase))*amplitude;
        mesh->set_vert_coord(i, new_pos);
    }

    mesh->update_normals_and_tangents();
    mesh->update_bbox();
}

}
