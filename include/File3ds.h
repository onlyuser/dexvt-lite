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

#ifndef VT_FILE_3DS_H_
#define VT_FILE_3DS_H_

#include <stdio.h>
#include <vector>
#include <string>

#define MAIN3DS 0x4D4D
#define EDIT3DS 0x3D3D
#define EDIT_OBJECT 0x4000
#define OBJ_TRIMESH 0x4100
#define TRI_VERTEXL 0x4110
#define TRI_FACEL 0x4120

namespace vt {

class Mesh;
class MeshBase;

class File3ds
{
public:
    static bool load3ds(const std::string& filename, int index, std::vector<Mesh*>* meshes);

private:
    static bool load3ds_impl(const std::string& filename, int index, std::vector<MeshBase*>* meshes);
	static uint32_t enter_chunk(FILE* stream, uint32_t chunk_id, uint32_t chunk_end);
	static void read_vertices(FILE* stream, MeshBase* mesh);
	static void read_faces(FILE* stream, MeshBase* mesh);
	static uint16_t read_short(FILE* stream);
	static uint32_t read_long(FILE* stream);
};

}

#endif
