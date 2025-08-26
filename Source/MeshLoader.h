#pragma once
   
#include "VertexInputLayouts.h"          
#include <vector>
#include <string>             
#include"glm/glm.hpp"

struct Primitive {
	uint32_t firstIndex;
	uint32_t indexCount;
	int32_t materialIndex;
};

struct Node {

	Node* parent = nullptr;
	std::vector<Node*> children;
	glm::mat4 matrix;
	std::string name;
	Primitive meshPrimitive;

	~Node() {
		for (auto child : children) {
			delete child;
		}
		children.clear();
	}
};

struct StoredModelData
{
	std::vector<ModelVertex>  VertexData;
	std::vector<uint32_t >    IndexData;
	std::vector<Node*> nodes;
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

	Node* loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& model, std::vector<ModelVertex>& vertices, std::vector<uint32_t>& indices, Node* Patent);


private:
	std::string FilePath;
	std::string err;
	std::string warn;
};


