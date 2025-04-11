#pragma once
   
#include "VertexInputLayouts.h"          
#include <vector>
#include <string>             


namespace tinygltf {
	class Model;
	class TinyGLTF;
	class Primitive;
}

class MeshLoader
{
public:

	MeshLoader();
	void LoadModel(const std::string& pFile);

	void ProcessMesh(const tinygltf::Primitive& primitive, tinygltf::Model& model);

	const std::vector<ModelVertex>& GetVertices();
	const std::vector<uint16_t>& GetIndices();

private:
	std::vector<ModelVertex> vertices;
	std::vector<uint16_t> indices;


	std::string err;
	std::string warn;
};