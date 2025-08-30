#pragma once
   
#include "VertexInputLayouts.h"          
#include <vector>
#include <string>             
#include"glm/glm.hpp"
#include <memory>

struct Primitive {
	uint32_t  indicesStart = 0;
	uint32_t  numIndices    = 0;
	uint32_t  materialIndex = 0;
};

struct Node {

	Node* parent = nullptr;
	std::vector<std::shared_ptr<Node>> children;
	std::vector<Primitive> meshPrimitives; 
	glm::mat4 matrix = glm::mat4(1.0f);

};

struct StoredModelData
{
	std::vector<ModelVertex>  VertexData;
	std::vector<uint32_t >    IndexData;
	std::vector<std::shared_ptr<Node>> nodes;
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

	std::unique_ptr<Node> loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& model, std::vector<ModelVertex>& vertices, std::vector<uint32_t>& indices, Node* Patent);


private:
	std::string FilePath;
	std::string err;
	std::string warn;
};


