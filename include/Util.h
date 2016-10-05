#ifndef VT_UTIL_H_
#define VT_UTIL_H_

#include <glm/glm.hpp>
#include <vector>
#include <string>

#define EPSILON 0.0001

#define SIGN(x) (!(x) ? 0 : (((x) > 0) ? 1 : -1))

#ifdef NO_GLM_CONSTANTS
    #warning "Disabling glm header <glm/gtx/constants.hpp>"
    #define PI      3.1415926
    #define HALF_PI (PI*0.5)
#else
    #include <glm/gtc/constants.hpp>
    #define PI      glm::pi<float>()
    #define HALF_PI glm::half_pi<float>()
#endif

#if GLM_VERSION >= 96
    // glm::rotate changed from degrees to radians in GLM 0.9.6
    // https://glm.g-truc.net/0.9.6/updates.html
    #define GLM_ROTATE(m, a, v) glm::rotate((m), glm::radians(a), (v))
#else
    #define GLM_ROTATE(m, a, v) glm::rotate((m), (a), (v))
#endif

#define ORIENT_ROLL(v)  v[0]
#define ORIENT_PITCH(v) v[1]
#define ORIENT_YAW(v)   v[2]

#define VEC_LEFT    glm::vec3(1, 0, 0)
#define VEC_UP      glm::vec3(0, 1, 0)
#define VEC_FORWARD glm::vec3(0, 0, 1)

namespace vt {

class Mesh;

glm::vec3 orient_to_offset(glm::vec3 orient);
glm::vec3 offset_to_orient(glm::vec3 offset);
void mesh_apply_ripple(Mesh* mesh, glm::vec3 origin, float amplitude, float wavelength, float phase);
bool read_file(std::string filename, std::string &s);
bool regexp(std::string &s, std::string pattern, std::vector<std::string*> &cap_groups, size_t *start_pos);
bool regexp(std::string &s, std::string pattern, std::vector<std::string*> &cap_groups);
bool regexp(std::string &s, std::string pattern, int nmatch, ...);

}

#endif
