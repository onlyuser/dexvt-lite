// This file is part of dexvt-lite.
// -- 3D Inverse Kinematics (Cyclic Coordinate Descent) with Constraints
// Copyright (C) 2018 onlyuser <mailto:onlyuser@gmail.com>
//
// dexvt-lite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dexvt-lite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dexvt-lite.  If not, see <http://www.gnu.org/licenses/>.

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
