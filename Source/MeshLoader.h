#pragma once

#include "assimp\Importer.hpp"      // C++ importer interface
#include "assimp\scene.h"           // Output data structure
#include "assimp\postprocess.h"     // Post processing flags
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"

struct ModelVertex {

	glm::vec3 vert;
	glm::vec2 text;

	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};

		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(ModelVertex);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 2> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(ModelVertex, vert);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(ModelVertex, text);

		return attributeDescriptions;
	}
};

struct SkyBoxVertex {

	glm::vec3 vert;

	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};

		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(SkyBoxVertex);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 1> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(SkyBoxVertex, vert);

		return attributeDescriptions;
	}
};

class MeshLoader
{
public:

	MeshLoader();
	void LoadModel(const std::string& pFile);

	std::vector<ModelVertex>& GetVertices();
	std::vector<uint16_t>& GetIndices();

private:
	Assimp::Importer importer;
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<ModelVertex> vertices;
	std::vector<uint16_t> indices;
};