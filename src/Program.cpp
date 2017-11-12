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

#include <Program.h>
#include <Shader.h>
#include <VarAttribute.h>
#include <VarUniform.h>
#include <Util.h>
#include <GL/glew.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <memory.h>
#include <assert.h>

namespace vt {

Program::var_attribute_type_to_name_table_t Program::m_var_attribute_type_to_name_table[] = {
        {Program::var_attribute_type_texcoord,        "texcoord"},
        {Program::var_attribute_type_vertex_normal,   "vertex_normal"},
        {Program::var_attribute_type_vertex_position, "vertex_position"},
        {Program::var_attribute_type_vertex_tangent,  "vertex_tangent"},
        {Program::var_attribute_type_count,           ""},
        };

Program::var_uniform_type_to_name_table_t Program::m_var_uniform_type_to_name_table[] = {
        {Program::var_uniform_type_ambient_color,                   "ambient_color"},
        {Program::var_uniform_type_backface_depth_overlay_texture,  "backface_depth_overlay_texture"},
        {Program::var_uniform_type_backface_normal_overlay_texture, "backface_normal_overlay_texture"},
        {Program::var_uniform_type_bloom_kernel,                    "bloom_kernel"},
        {Program::var_uniform_type_bump_texture,                    "bump_texture"},
        {Program::var_uniform_type_camera_dir,                      "camera_dir"},
        {Program::var_uniform_type_camera_far,                      "camera_far"},
        {Program::var_uniform_type_camera_near,                     "camera_near"},
        {Program::var_uniform_type_camera_pos,                      "camera_pos"},
        {Program::var_uniform_type_color_texture,                   "color_texture"},
        {Program::var_uniform_type_color_texture2,                  "color_texture2"},
        {Program::var_uniform_type_env_map_texture,                 "env_map_texture"},
        {Program::var_uniform_type_frontface_depth_overlay_texture, "frontface_depth_overlay_texture"},
        {Program::var_uniform_type_glow_cutoff_threshold,           "glow_cutoff_threshold"},
        {Program::var_uniform_type_image_res,                       "image_res"},
        {Program::var_uniform_type_inv_normal_transform,            "inv_normal_transform"},
        {Program::var_uniform_type_inv_projection_transform,        "inv_projection_transform"},
        {Program::var_uniform_type_inv_view_proj_transform,         "inv_view_proj_transform"},
        {Program::var_uniform_type_light_color,                     "light_color"},
        {Program::var_uniform_type_light_count,                     "light_count"},
        {Program::var_uniform_type_light_enabled,                   "light_enabled"},
        {Program::var_uniform_type_light_pos,                       "light_pos"},
        {Program::var_uniform_type_model_transform,                 "model_transform"},
        {Program::var_uniform_type_mvp_transform,                   "mvp_transform"},
        {Program::var_uniform_type_normal_transform,                "normal_transform"},
        {Program::var_uniform_type_random_texture,                  "random_texture"},
        {Program::var_uniform_type_reflect_to_refract_ratio,        "reflect_to_refract_ratio"},
        {Program::var_uniform_type_ssao_sample_kernel_pos,          "ssao_sample_kernel_pos"},
        {Program::var_uniform_type_viewport_dim,                    "viewport_dim"},
        {Program::var_uniform_type_view_proj_transform,             "view_proj_transform"},
        {Program::var_uniform_type_count,                           ""}
    };

Program::Program(std::string name)
    : NamedObject(name),
      m_vertex_shader(NULL),
      m_fragment_shader(NULL)
{
    m_id = glCreateProgram();
    memset(m_var_attribute_ids, 0, sizeof(m_var_attribute_ids));
    memset(m_var_uniform_ids, 0, sizeof(m_var_uniform_ids));
}

Program::~Program()
{
    glDeleteProgram(m_id);
}

void Program::attach_shader(Shader* shader)
{
    glAttachShader(m_id, shader->id());
    switch(shader->type()) {
        case GL_VERTEX_SHADER:
            m_vertex_shader = shader;
            break;
        case GL_FRAGMENT_SHADER:
            m_fragment_shader = shader;
            break;
    }
}

bool Program::auto_add_shader_vars()
{
    for(int i = 0; i<2; i++) {
        std::string filename = (i == 0 ? m_vertex_shader : m_fragment_shader)->get_filename();
        std::string file_data;
        if(!read_file(filename, file_data)) {
            return false;
        }
        std::stringstream ss;
        ss << file_data;
        std::string line;
        while(getline(ss, line, '\n')) {
            //std::cout << "LINE: " << line << std::endl;
            std::string type_name;
            std::string var_name;
            if(regexp(line, "attribute[ ]+([^ ]+)[ ]+([^ ;\[]+)[;\[]", 3,
                      NULL,
                      &type_name,
                      &var_name))
            {
                //std::cout << "VAR ATTRIBUTE TYPE: " << type_name << std::endl;
                //std::cout << "VAR ATTRIBUTE NAME: " << var_name << std::endl;
                add_var(Program::VAR_TYPE_ATTRIBUTE, var_name);
            }
            if(regexp(line, "uniform[ ]+([^ ]+)[ ]+([^ ;\[]+)[;\[]", 3,
                      NULL,
                      &type_name,
                      &var_name))
            {
                //std::cout << "VAR UNIFORM TYPE: " << type_name << std::endl;
                //std::cout << "VAR UNIFORM NAME: " << var_name << std::endl;
                add_var(Program::VAR_TYPE_UNIFORM, var_name);
            }
        }
    }
    return true;
}

bool Program::link()
{
    glLinkProgram(m_id);
    GLint link_ok = GL_FALSE;
    get_program_iv(GL_LINK_STATUS, &link_ok);
    if(!auto_add_shader_vars()) {
        return false;
    }
    return (link_ok == GL_TRUE);
}

void Program::use() const
{
    glUseProgram(m_id);
}

VarAttribute* Program::get_var_attribute(const GLchar* name) const
{
    VarAttribute* var_attrribute = new VarAttribute(this, name);
    if(var_attrribute && var_attrribute->id() == static_cast<GLuint>(-1)) {
        fprintf(stderr, "Could not bind attribute %s\n", name);
        delete var_attrribute;
        return NULL;
    }
    return var_attrribute;
}

VarUniform* Program::get_var_uniform(const GLchar* name) const
{
    VarUniform* var_uniform = new VarUniform(this, name);
    if(var_uniform && var_uniform->id() == static_cast<GLuint>(-1)) {
        fprintf(stderr, "Could not bind uniform %s\n", name);
        delete var_uniform;
        return NULL;
    }
    return var_uniform;
}

void Program::get_program_iv(GLenum pname, GLint* params) const
{
    glGetProgramiv(m_id, pname, params);
}

std::string Program::get_var_attribute_name(int id)
{
    assert(id >= 0 && id < var_attribute_type_count);
    return m_var_attribute_type_to_name_table[id].second;
}

std::string Program::get_var_uniform_name(int id)
{
    assert(id >= 0 && id < var_uniform_type_count);
    return m_var_uniform_type_to_name_table[id].second;
}

bool Program::check_var_exists_in_shader(var_type_t var_type, std::string name) const
{
    if(!m_vertex_shader || !m_fragment_shader) {
        return false;
    }
    switch(var_type) {
        case VAR_TYPE_ATTRIBUTE:
            {
                std::string cmd1 = std::string("/bin/grep \"attribute .* ") + name + "[;\[]\" " + m_vertex_shader->get_filename()   + " > /dev/null";
                std::string cmd2 = std::string("/bin/grep \"attribute .* ") + name + "[;\[]\" " + m_fragment_shader->get_filename() + " > /dev/null";
                if(!system(cmd1.c_str()) || !system(cmd2.c_str())) {
                    std::cout << "\tFound attribute var \"" << name << "\"" << std::endl;
                } else {
                    std::cout << "\tError: Cannot find attribute var \"" << name << "\"" << std::endl;
                    return false;
                }
            }
            break;
        case VAR_TYPE_UNIFORM:
            {
                std::string cmd1 = std::string("/bin/grep \"uniform .* ") + name + "[;\[]\" " + m_vertex_shader->get_filename()   + " > /dev/null";
                std::string cmd2 = std::string("/bin/grep \"uniform .* ") + name + "[;\[]\" " + m_fragment_shader->get_filename() + " > /dev/null";
                if(!system(cmd1.c_str()) || !system(cmd2.c_str())) {
                    std::cout << "\tFound uniform var \"" << name << "\"" << std::endl;
                } else {
                    std::cout << "\tError: Cannot find uniform var \"" << name << "\"" << std::endl;
                    return false;
                }
            }
            break;
    }
    return true;
}

bool Program::add_var(var_type_t var_type, std::string name)
{
    if(!check_var_exists_in_shader(var_type, name)) {
        exit(1);
    }
    switch(var_type) {
        case VAR_TYPE_ATTRIBUTE:
            {
                var_attribute_names_t::iterator p = m_var_attribute_names.find(name);
                if(p != m_var_attribute_names.end()) {
                    std::cout << "Program variable \"" << name << "\" already added." << std::endl;
                    return false;
                }
                m_var_attribute_names.insert(name);
                int id = -1;
                for(int i = 0; i < var_attribute_type_count; i++) {
                    if(name == m_var_attribute_type_to_name_table[i].second) {
                        id = m_var_attribute_type_to_name_table[i].first;
                        break;
                    }
                }
                if(id != -1) {
                    m_var_attribute_ids[id] = true;
                }
            }
            break;
        case VAR_TYPE_UNIFORM:
            {
                var_uniform_names_t::iterator p = m_var_uniform_names.find(name);
                if(p != m_var_uniform_names.end()) {
                    std::cout << "Program variable \"" << name << "\" already added." << std::endl;
                    return false;
                }
                m_var_uniform_names.insert(name);
                int id = -1;
                for(int i = 0; i < var_uniform_type_count; i++) {
                    if(name == m_var_uniform_type_to_name_table[i].second) {
                        id = m_var_uniform_type_to_name_table[i].first;
                        break;
                    }
                }
                if(id != -1) {
                    m_var_uniform_ids[id] = true;
                }
            }
            break;
    }
    return true;
}

bool Program::has_var(var_type_t var_type, std::string name) const
{
    switch(var_type) {
        case VAR_TYPE_ATTRIBUTE: return (m_var_attribute_names.find(name) != m_var_attribute_names.end());
        case VAR_TYPE_UNIFORM:   return (m_var_uniform_names.find(name)   != m_var_uniform_names.end());
    }
    return false;
}

bool Program::has_var(var_type_t var_type, int id) const
{
    switch(var_type) {
        case VAR_TYPE_ATTRIBUTE: return m_var_attribute_ids[id];
        case VAR_TYPE_UNIFORM:   return m_var_uniform_ids[id];
    }
    return false;
}

void Program::clear_vars()
{
    m_var_attribute_names.clear();
    m_var_uniform_names.clear();
    for(int i = 0; i < Program::var_attribute_type_count; i++) {
        m_var_attribute_ids[i] = false;
    }
    for(int j = 0; j < Program::var_uniform_type_count; j++) {
        m_var_uniform_ids[j] = false;
    }
}

}
