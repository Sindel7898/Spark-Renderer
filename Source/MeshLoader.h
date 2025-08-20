#pragma once
   
#include "VertexInputLayouts.h"          
#include <vector>
#include <string>             
#include"glm/glm.hpp"

struct StoredModelData
{
	std::vector<ModelVertex> VertexData;
	std::vector<uint32_t >    IndexData;
	glm::mat4 modelMatrix;
};

namespace tinygltf {
	class Model;
	class TinyGLTF;
	class Primitive;
	class Node;
}

typedef unsigned char stbi_uc;

struct StoredImageData
{
	stbi_uc* imageData = nullptr;
	int      imageHeight= 0;
	int      imageWidth = 0 ;

};

class MeshLoader
{
public:

	MeshLoader();
	void LoadModel(const std::string& pFile);
	void LoadMaterials(const std::string& pFile, tinygltf::Model& model);

	void ProcessNode(const tinygltf::Node& node, const tinygltf::Model& model);

	void ProcessMesh(const tinygltf::Node& inputNode, const tinygltf::Model& model);


private:
	std::string FilePath;
	std::string err;
	std::string warn;
};


