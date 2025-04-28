#pragma once
   
#include "VertexInputLayouts.h"          
#include <vector>
#include <string>             

struct StoredModelData
{
	std::vector<ModelVertex> VertexData;
	std::vector<uint32_t >    IndexData;
};

namespace tinygltf {
	class Model;
	class TinyGLTF;
	class Primitive;
}

typedef unsigned char stbi_uc;

struct StoredImageData
{
	stbi_uc* imageData;
	int      imageHeight;
	int      imageWidth;

};
struct MikkTSpaceUserData {
	std::vector<ModelVertex>& vertices;
	const std::vector<uint32_t>& indices;

	MikkTSpaceUserData(std::vector<ModelVertex>& v, const std::vector<uint32_t>& i)
		: vertices(v), indices(i) {
	}
};

class SMikkTSpaceContext;

class MeshLoader
{
public:

	MeshLoader();
	void LoadModel(const std::string& pFile);

	void ProcessMesh(const tinygltf::Primitive& primitive, tinygltf::Model& model);

	const std::vector<ModelVertex>& GetVertices();
	const std::vector<uint32_t >& GetIndices();


private:
	std::vector<ModelVertex> vertices;
	std::vector<uint32_t > indices;
	
	std::string err;
	std::string warn;
};


int GetNumFaces(const SMikkTSpaceContext* context);
int GetNumVerticesOfFace(const SMikkTSpaceContext*, int);
void GetPosition(const SMikkTSpaceContext* context, float pos[3], int faceIdx, int vertIdx);
void GetNormal(const SMikkTSpaceContext* context, float norm[3], int faceIdx, int vertIdx);
void GetTexCoord(const SMikkTSpaceContext* context, float uv[2], int faceIdx, int vertIdx);
void SetTangent(const SMikkTSpaceContext* context, const float tangent[3], float sign, int faceIdx, int vertIdx);


