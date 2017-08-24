#ifndef VT_VAR_UNIFORM_H_
#define VT_VAR_UNIFORM_H_

#include <IdentObject.h>
#include <GL/glew.h>

namespace vt {

class Program;

class VarUniform : public IdentObject
{
public:
    VarUniform(const Program* program, const GLchar* name);
    virtual ~VarUniform();
    void uniform_1f(GLfloat v0) const;
    void uniform_2f(GLfloat v0, GLfloat v1) const;
    void uniform_3f(GLfloat v0, GLfloat v1, GLfloat v2) const;
    void uniform_4f(GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const;
    void uniform_1i(GLint v0) const;
    void uniform_2i(GLint v0, GLint v1) const;
    void uniform_3i(GLint v0, GLint v1, GLint v2) const;
    void uniform_4i(GLint v0, GLint v1, GLint v2, GLint v3) const;
    void uniform_1ui(GLuint v0) const;
    void uniform_2ui(GLuint v0, GLuint v1) const;
    void uniform_3ui(GLuint v0, GLuint v1, GLuint v2) const;
    void uniform_4ui(GLuint v0, GLuint v1, GLuint v2, GLuint v3) const;
    void uniform_1fv(GLsizei count, const GLfloat* value) const;
    void uniform_2fv(GLsizei count, const GLfloat* value) const;
    void uniform_3fv(GLsizei count, const GLfloat* value) const;
    void uniform_4fv(GLsizei count, const GLfloat* value) const;
    void uniform_1iv(GLsizei count, const GLint* value) const;
    void uniform_2iv(GLsizei count, const GLint* value) const;
    void uniform_3iv(GLsizei count, const GLint* value) const;
    void uniform_4iv(GLsizei count, const GLint* value) const;
    void uniform_1uiv(GLsizei count, const GLuint* value) const;
    void uniform_2uiv(GLsizei count, const GLuint* value) const;
    void uniform_3uiv(GLsizei count, const GLuint* value) const;
    void uniform_4uiv(GLsizei count, const GLuint* value) const;
    void uniform_matrix_2fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_3fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_4fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_2x3fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_3x2fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_2x4fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_4x2fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_3x4fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
    void uniform_matrix_4x3fv(GLsizei count, GLboolean transpose, const GLfloat* value) const;
};

}

#endif
