#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct alignas(16)  ModelVertex {

	alignas(16) glm::vec3 vert;
	alignas(16) glm::vec2 text;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec3 tangent;
	alignas(16) glm::vec3 bitangent;

	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};

		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(ModelVertex);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 5> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 5> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(ModelVertex, vert);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(ModelVertex, text);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[2].offset = offsetof(ModelVertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[3].offset = offsetof(ModelVertex, tangent);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[4].offset = offsetof(ModelVertex, bitangent);

		return attributeDescriptions;
	}
};

struct VertexOnly {

	glm::vec3 vert;

	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};

		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(VertexOnly);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 1> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(VertexOnly, vert);

		return attributeDescriptions;
	}
};


struct FullScreenQuadDescription {

	glm::vec2 vert;
	glm::vec2 text;

	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};

		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(FullScreenQuadDescription);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 2> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(FullScreenQuadDescription, vert);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(FullScreenQuadDescription, text);

		return attributeDescriptions;
	}
};