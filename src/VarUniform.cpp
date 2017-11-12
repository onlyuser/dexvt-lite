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

#include <VarUniform.h>
#include <Program.h>
#include <GL/glew.h>
#include <assert.h>

namespace vt {

VarUniform::VarUniform(const Program* program, const GLchar* name)
{
    m_id = glGetUniformLocation(program->id(), name);
    assert(m_id != static_cast<GLuint>(-1));
}

VarUniform::~VarUniform()
{
}

void VarUniform::uniform_1f(GLfloat v0) const
{
    glUniform1f(m_id, v0);
}

void VarUniform::uniform_2f(GLfloat v0, GLfloat v1) const
{
    glUniform2f(m_id, v0, v1);
}

void VarUniform::uniform_3f(GLfloat v0, GLfloat v1, GLfloat v2) const
{
    glUniform3f(m_id, v0, v1, v2);
}

void VarUniform::uniform_4f(GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const
{
    glUniform4f(m_id, v0, v1, v2, v3);
}

void VarUniform::uniform_1i(GLint v0) const
{
    glUniform1i(m_id, v0);
}

void VarUniform::uniform_2i(GLint v0, GLint v1) const
{
    glUniform2i(m_id, v0, v1);
}

void VarUniform::uniform_3i(GLint v0, GLint v1, GLint v2) const
{
    glUniform3i(m_id, v0, v1, v2);
}

void VarUniform::uniform_4i(GLint v0, GLint v1, GLint v2, GLint v3) const
{
    glUniform4i(m_id, v0, v1, v2, v3);
}

void VarUniform::uniform_1ui(GLuint v0) const
{
    glUniform1ui(m_id, v0);
}

void VarUniform::uniform_2ui(GLuint v0, GLuint v1) const
{
    glUniform2ui(m_id, v0, v1);
}

void VarUniform::uniform_3ui(GLuint v0, GLuint v1, GLuint v2) const
{
    glUniform3ui(m_id, v0, v1, v2);
}

void VarUniform::uniform_4ui(GLuint v0, GLuint v1, GLuint v2, GLuint v3) const
{
    glUniform4ui(m_id, v0, v1, v2, v3);
}

void VarUniform::uniform_1fv(GLsizei count, const GLfloat* value) const
{
    glUniform1fv(m_id, count, value);
}

void VarUniform::uniform_2fv(GLsizei count, const GLfloat* value) const
{
    glUniform2fv(m_id, count, value);
}

void VarUniform::uniform_3fv(GLsizei count, const GLfloat* value) const
{
    glUniform3fv(m_id, count, value);
}

void VarUniform::uniform_4fv(GLsizei count, const GLfloat* value) const
{
    glUniform4fv(m_id, count, value);
}

void VarUniform::uniform_1iv(GLsizei count, const GLint* value) const
{
    glUniform1iv(m_id, count, value);
}

void VarUniform::uniform_2iv(GLsizei count, const GLint* value) const
{
    glUniform2iv(m_id, count, value);
}

void VarUniform::uniform_3iv(GLsizei count, const GLint* value) const
{
    glUniform3iv(m_id, count, value);
}

void VarUniform::uniform_4iv(GLsizei count, const GLint* value) const
{
    glUniform4iv(m_id, count, value);
}

void VarUniform::uniform_1uiv(GLsizei count, const GLuint* value) const
{
    glUniform1uiv(m_id, count, value);
}

void VarUniform::uniform_2uiv(GLsizei count, const GLuint* value) const
{
    glUniform2uiv(m_id, count, value);
}

void VarUniform::uniform_3uiv(GLsizei count, const GLuint* value) const
{
    glUniform3uiv(m_id, count, value);
}

void VarUniform::uniform_4uiv(GLsizei count, const GLuint* value) const
{
    glUniform4uiv(m_id, count, value);
}

void VarUniform::uniform_matrix_2fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix2fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_3fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix3fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_4fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix4fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_2x3fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix2x3fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_3x2fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix3x2fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_2x4fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix2x4fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_4x2fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix4x2fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_3x4fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix3x4fv(m_id, count, transpose, value);
}

void VarUniform::uniform_matrix_4x3fv(GLsizei count, GLboolean transpose, const GLfloat* value) const
{
    glUniformMatrix4x3fv(m_id, count, transpose, value);
}

}
