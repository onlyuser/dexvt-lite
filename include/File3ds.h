#ifndef H_FILE3DS
#define H_FILE3DS

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

class File3ds
{
public:
    static bool load3ds(std::string filename, int index, std::vector<Mesh*>* meshes);

private:
	static uint32_t enterChunk(FILE *stream, uint32_t chunkID, uint32_t chunkEnd);
	static void readVertList(FILE *stream, Mesh *mesh);
	static void readFaceList(FILE *stream, Mesh *mesh);
	static uint16_t readShort(FILE *stream);
	static uint32_t readLong(FILE *stream);
};

}

#endif
