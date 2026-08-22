#pragma once
   
#include "VertexInputLayouts.h"          
#include <vector>
#include <string>             
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct Primitive {
	uint32_t  indicesStart  = 0;
	uint32_t  numIndices    = 0;
	uint32_t  materialIndex = 0;
};

struct Node {
	std::string name = "Node";
	uint32_t id = 0;
	Node* parent = nullptr;
	std::vector<std::shared_ptr<Node>> children;
	std::vector<Primitive> meshPrimitives; 

	glm::vec3 translation = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in degrees
	glm::vec3 scale = glm::vec3(1.0f);

	glm::mat4 matrix = glm::mat4(1.0f);
	glm::mat4 prevMatrix = glm::mat4(1.0f);
	glm::mat4 initialMatrix = glm::mat4(1.0f);

	void UpdateLocalMatrix()
	{
		glm::mat4 m = glm::mat4(1.0f);
		m = glm::translate(m, translation);
		m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		m = glm::scale(m, scale);
		matrix = m;
	}

	void DecomposeLocalMatrix()
	{
		glm::vec3 Newscale(1.0f);
		glm::quat Newrotation(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 Newtranslation(0.0f);
		glm::vec3 Newskew(0.0f);
		glm::vec4 Newperspective(0.0f);

		glm::decompose(matrix, Newscale, Newrotation, Newtranslation, Newskew, Newperspective);

		translation = Newtranslation;
		rotation = glm::degrees(glm::eulerAngles(Newrotation));
		scale = Newscale;
	}

	void SetLocalMatrix(const glm::mat4& m)
	{
		matrix = m;
		DecomposeLocalMatrix();
	}

	void ResetTransform()
	{
		matrix = initialMatrix;
		DecomposeLocalMatrix();
	}

	glm::mat4 GetParentWorldMatrix(const glm::mat4& modelTransform) const
	{
		if (parent) {
			return parent->GetWorldMatrix(modelTransform);
		}
		return modelTransform;
	}

	glm::mat4 GetWorldMatrix(const glm::mat4& modelTransform) const
	{
		if (parent) {
			return parent->GetWorldMatrix(modelTransform) * matrix;
		}
		return modelTransform * matrix;
	}
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
	class Material;

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

	StoredImageData ReadTexture(const tinygltf::Material& gltfMaterial, std::string TextureType, tinygltf::Model& model);


	std::unique_ptr<Node> loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& model, std::vector<ModelVertex>& vertices, std::vector<uint32_t>& indices, Node* Patent);

	Primitive ProcessPrimitive(const tinygltf::Primitive& glTFPrimitive, const tinygltf::Model& model, std::vector<ModelVertex>& outVertices, std::vector<uint32_t>& outIndices);


private:
	std::string FilePath;
	std::string err;
	std::string warn;
};


