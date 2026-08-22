#include "App.h"
#include "ShaderHelper.h"
#include "Pipeline_Manager.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Light.h"
#include "Model.h"
#include "SkyBox.h"
#include "SSAO_FullScreenQuad.h"
#include "FXAA_FullScreenQuad.h"
#include "SSGI.h"
#include "CombinedResult_FullScreenQuad.h"
#include "Lighting_RTX.h"
#include "DynamicDiffuse_RTGI.h"
#include "ReSTIR_DI.h"
#include <array>
#include <vector>

void App::CreateGraphicsPipeline()
{
	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext.swapchainformat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext.FindCompatableDepthFormat();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

	vk::Viewport viewport{};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setHeight((float)vulkanContext.swapchainExtent.height);
	viewport.setWidth((float)vulkanContext.swapchainExtent.width);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Offset2D scissorOffset = { 0, 0 };

	vk::Rect2D scissor{};
	scissor.setOffset(scissorOffset);
	scissor.setExtent(vulkanContext.swapchainExtent);

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.setViewportCount(1);
	viewportState.setScissorCount(1);
	viewportState.setViewports(viewport);
	viewportState.setScissors(scissor);

	// Rasterizer information
	vk::PipelineRasterizationStateCreateInfo rasterizerinfo{};
	rasterizerinfo.depthClampEnable = vk::False;
	rasterizerinfo.rasterizerDiscardEnable = vk::False;
	rasterizerinfo.polygonMode = vk::PolygonMode::eFill;
	rasterizerinfo.lineWidth = 1.0f;
	rasterizerinfo.cullMode = vk::CullModeFlagBits::eNone;
	rasterizerinfo.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizerinfo.depthBiasEnable = vk::False;
	rasterizerinfo.depthBiasConstantFactor = 0.0f;
	rasterizerinfo.depthBiasClamp = 0.0f;
	rasterizerinfo.depthBiasSlopeFactor = 0.0f;

	// Multisample
	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = vk::False;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = vk::False;
	multisampling.alphaToOneEnable = vk::False;

	// Color blending
	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = vk::True;
	colorBlendAttachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
	colorBlendAttachment.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
	colorBlendAttachment.setColorBlendOp(vk::BlendOp::eAdd);
	colorBlendAttachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
	colorBlendAttachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
	colorBlendAttachment.setAlphaBlendOp(vk::BlendOp::eAdd);

	vk::PipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.setLogicOpEnable(vk::False);
	colorBlend.logicOp = vk::LogicOp::eCopy;
	colorBlend.setAttachmentCount(1);
	colorBlend.setPAttachments(&colorBlendAttachment);

	std::vector<vk::DynamicState> DynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
		vk::DynamicState::ePolygonModeEXT
	};

	vk::PipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicState.pDynamicStates = DynamicStates.data();

	// 1. FXAA Pipeline
	{
		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext.swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::vec4));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(fxaa_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		FullScreen_Quad_Pipeline_Data Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/FXAA.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);
		FXAAPassPipelineLayout = Temp.FQ_PipelineLayout;
		FXAAPassPipeline = Temp.FQ_Pipeline;
	}

	// 2. SSAO Pipeline
	{
		std::array<vk::Format, 1> colorFormats = { vk::Format::eR8G8B8A8Unorm };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssao_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		FullScreen_Quad_Pipeline_Data Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSAO_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);
		SSAOPipelineLayout = Temp.FQ_PipelineLayout;
		SSAOPipeline = Temp.FQ_Pipeline;
	}

	// 3. SSAO Blur Pipeline
	{
		std::array<vk::Format, 1> colorFormats = { vk::Format::eR8G8B8A8Unorm };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::vec2));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssao_FullScreenQuad->SSAOBlurDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		FullScreen_Quad_Pipeline_Data Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSAOBlur_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);
		SSAOBlurPipelineLayout = Temp.FQ_PipelineLayout;
		SSAOBlurPipeline = Temp.FQ_Pipeline;
	}

	// 4. Combined Lighting Image Pipeline
	{
		std::array<vk::Format, 1> colorFormats = { vk::Format::eR16G16B16A16Sfloat };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(PostProcessSettings));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(Combined_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		FullScreen_Quad_Pipeline_Data Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/CombinedImage.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);
		CombinedImagePipelineLayout = Temp.FQ_PipelineLayout;
		CombinedImagePassPipeline = Temp.FQ_Pipeline;
	}

	// 5. Gamma Correction / IMGUI Pipeline
	{
		std::array<vk::Format, 1> colorFormats = { vk::Format::eB8G8R8A8Unorm };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(Combined_FullScreenQuad->Gamma_Correction_descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		FullScreen_Quad_Pipeline_Data Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/GammaCorrection.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);
		Gamma_Corrected_IMGUI_PipelineLayout = Temp.FQ_PipelineLayout;
		Gamma_Corrected_IMGUI_PassPipeline = Temp.FQ_Pipeline;
	}

	// 6. Light Visualizer Pipeline
	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragmentShaderStageInfo };

		auto BindDesctiptions = VertexOnly::GetBindingDescription();
		auto attributeDescriptions = VertexOnly::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(1);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;
        
		vk::PushConstantRange range = {};
		range.stageFlags = vk::ShaderStageFlagBits::eFragment;
		range.offset = 0;
		range.size = 12;

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(lights[0]->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;
        
		LightpipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		
		vk::Format lightPassFormat = vk::Format::eR16G16B16A16Sfloat;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &lightPassFormat;

		LightgraphicsPipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, LightpipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	// 7. DDGI Probe Visualizer Pipeline
	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Probe.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Probe.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragmentShaderStageInfo };

		auto BindDesctiptions = ModelVertex::GetBindingDescription();
		auto attributeDescriptions = ModelVertex::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(4);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		vk::PushConstantRange range = {};
		range.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		range.offset = 0;
		range.size = sizeof(CameraConstantBuffer);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(dynamicDiffuse_RTGI->ProbeDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		DDGIProbepipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		vk::Format lightPassFormat = vk::Format::eR16G16B16A16Sfloat;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &lightPassFormat;

		DDGIProbePipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, DDGIProbepipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	// 8. SkyBox Pipeline
	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragmentShaderStageInfo };

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::True;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		auto BindDesctiptions = VertexOnly::GetBindingDescription();
		auto attributeDescriptions = VertexOnly::GetAttributeDescription();
         
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(1);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(skyBox->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		vk::Format skyBoxFormat = vk::Format::eR16G16B16A16Sfloat;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &skyBoxFormat;

		SkyBoxpipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SkyBoxgraphicsPipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SkyBoxpipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	// 9. Geometry / G-Buffer Pass Pipeline
	{
		vk::PipelineRasterizationStateCreateInfo rasterizerinfo{};
		rasterizerinfo.depthClampEnable = vk::False;
		rasterizerinfo.rasterizerDiscardEnable = vk::False;
		rasterizerinfo.polygonMode = vk::PolygonMode::eFill;
		rasterizerinfo.lineWidth = 1.0f;
		rasterizerinfo.cullMode = vk::CullModeFlagBits::eNone;
		rasterizerinfo.frontFace = vk::FrontFace::eCounterClockwise;
		rasterizerinfo.depthBiasEnable = vk::False;
		rasterizerinfo.depthBiasConstantFactor = 0.0f;
		rasterizerinfo.depthBiasClamp = 0.0f;
		rasterizerinfo.depthBiasSlopeFactor = 0.0f;

		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/GeometryPass.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/GeometryPass.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragmentShaderStageInfo };

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::True;
		depthStencilState.depthWriteEnable = vk::True;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		auto BindDesctiptions = ModelVertex::GetBindingDescription();
		auto attributeDescriptions = ModelVertex::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(4);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		std::array<vk::Format, 8> colorFormats = {
			vk::Format::eR16G16B16A16Sfloat, // Position
			vk::Format::eR16G16B16A16Sfloat, // Normal
			vk::Format::eR16G16B16A16Sfloat, // ViewSpaceNormal
			vk::Format::eR8G8B8A8Srgb,       // Albedo
			vk::Format::eR8G8B8A8Srgb,       // Emissive
			vk::Format::eR8G8B8A8Unorm,      // Material
			vk::Format::eR16G16B16A16Sfloat, // ReflectionMask
			vk::Format::eR8G8B8A8Unorm
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
		pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext.FindCompatableDepthFormat();

		vk::DescriptorSetLayout setLayouts[] = { Models[0]->descriptorSetLayout };

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::mat4) * 2);
		range.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = setLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		std::array<vk::PipelineColorBlendAttachmentState, 8> colorBlendAttachments = {
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE),
			vk::PipelineColorBlendAttachmentState{}.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA).setBlendEnable(VK_FALSE)
		};

		vk::PipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.setLogicOpEnable(VK_FALSE);
		colorBlend.setAttachmentCount(static_cast<uint32_t>(colorBlendAttachments.size()));
		colorBlend.setPAttachments(colorBlendAttachments.data());

		geometryPassPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		geometryPassPipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, geometryPassPipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	// 10. ReSTIR DI Temporal Ray Tracing Pipeline
	{
		auto RayGen_ShaderCode        = readFile("../Shaders/Compiled_Shader_Files/ReSTIR_DI_Temporal_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIR_DI_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode    = readFile("../Shaders/Compiled_Shader_Files/ReSTIRDI_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);

		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = {
			RayGen_ShaderStageInfo,
			RayClosestHit_ShaderStageInfo,
			RayMiss_ShaderStageInfo
		};

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0;
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1;
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,
			RayClosestHit_GroupInfo,
			Miss_GroupInfo,
			Miss_GroupInfo,
		};

		vk::PushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstant);

		vk::DescriptorSetLayout layouts[2] = { Restir_DI->RayTracingDescriptorSetLayout, Restir_DI->DDGIATLASDescriptorSetLayout };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		ReSTIR_Temporal_RT_PipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		ReSTIR_Temporal_RT_PassPipeline = pipelineManager.createRayTracingGraphicsPipeline(ReSTIR_Temporal_RT_PipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule); 
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
	}

	// 11. ReSTIR DI Spatial Ray Tracing Pipeline
	{
		auto RayGen_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIR_DI_spatial_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIR_DI_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIRDI_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);

		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = {
			RayGen_ShaderStageInfo,
			RayClosestHit_ShaderStageInfo,
			RayMiss_ShaderStageInfo
		};

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0;
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1;
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,
			RayClosestHit_GroupInfo,
			Miss_GroupInfo,
			Miss_GroupInfo,
		};

		vk::PushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstant);

		vk::DescriptorSetLayout layouts[2] = { Restir_DI->RayTracingDescriptorSetLayout, Restir_DI->DDGIATLASDescriptorSetLayout };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		ReSTIR_Spatial_RT_PipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		ReSTIR_SPATIAL_RT_PassPipeline = pipelineManager.createRayTracingGraphicsPipeline(ReSTIR_Spatial_RT_PipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
	}

	// 12. Deferred Direct Lighting & Path Tracing Raygen Pipeline
	{
		auto RayGen_ShaderCode        = readFile("../Shaders/Compiled_Shader_Files/Lighting_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Lighting_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode    = readFile("../Shaders/Compiled_Shader_Files/Lighting_Miss.rmiss.spv");
		auto RayGenMiss2_ShaderCode   = readFile("../Shaders/Compiled_Shader_Files/Lighting_RTGI_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule         = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule  = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule        = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);
		VkShaderModule RayMiss2_ShaderModule       = pipelineManager.createShaderModule(RayGenMiss2_ShaderCode);

		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss2_ShaderStageInfo{};
		RayMiss2_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss2_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss2_ShaderStageInfo.module = RayMiss2_ShaderModule;
		RayMiss2_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = {
			RayGen_ShaderStageInfo,
			RayClosestHit_ShaderStageInfo,
			RayMiss_ShaderStageInfo,
			RayMiss2_ShaderStageInfo
		};

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0;
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1;
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss2_GroupInfo{};
		Miss2_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss2_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss2_GroupInfo.generalShader = 3;
		Miss2_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss2_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss2_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,
			RayClosestHit_GroupInfo,
			Miss_GroupInfo,
			Miss2_GroupInfo,
		};

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(Lightin_RTX_PC));
		range.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &lighting_RTX->descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		DeferedLightingPassPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		DeferedLightingPassPipeline = pipelineManager.createRayTracingGraphicsPipeline(DeferedLightingPassPipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss2_ShaderModule);
	}

	// 13. Dynamic Diffuse Global Illumination (DDGI) Ray Tracing Pipeline
	{
		auto RayGen_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Miss.rmiss.spv");
		auto ShadowMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Shadow_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);
		VkShaderModule ShadowMiss_ShaderModule = pipelineManager.createShaderModule(ShadowMiss_ShaderCode);

		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShadowMiss_ShaderStageInfo{};
		ShadowMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ShadowMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		ShadowMiss_ShaderStageInfo.module = ShadowMiss_ShaderModule;
		ShadowMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = {
			RayGen_ShaderStageInfo,       
			RayClosestHit_ShaderStageInfo,
			RayMiss_ShaderStageInfo,      
			ShadowMiss_ShaderStageInfo    
		};

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0; 
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1; 
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR ShadowMiss_GroupInfo{};
		ShadowMiss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		ShadowMiss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		ShadowMiss_GroupInfo.generalShader = 3; 
		ShadowMiss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		ShadowMiss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		ShadowMiss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,        
			RayClosestHit_GroupInfo, 
			Miss_GroupInfo,          
			ShadowMiss_GroupInfo,    
		};

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(RTpcInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->RaytracingDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		RT_DDGIPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		RT_DDGIPassPipeline = pipelineManager.createRayTracingGraphicsPipeline(RT_DDGIPipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(ShadowMiss_ShaderModule);
	}

	// 14. DDGI Grid Compute Pipeline
	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/Grid.comp.spv");
		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType  = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage  = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName  = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(GridData));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->GridDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		GridComputePipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		GridComputePassPipeline = pipelineManager.creatComputePipeline(GridComputePipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}

	// 15. DDGI Irradiance & Visibility Compute Pipeline
	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/Irradiance_Visibility.comp.spv");
		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(GeneralAtlasInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->ConstructProbeDataDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		IrradianceComputePipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		IrradianceComputePassPipeline = pipelineManager.creatComputePipeline(IrradianceComputePipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}

	// 16. DDGI Probe Status Compute Pipeline
	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/ProbeStatus.comp.spv");
		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(GeneralAtlasInfo_Status));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->ProbeStatusDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		ProbeStatusPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		ProbeStatusComputePassPipeline = pipelineManager.creatComputePipeline(ProbeStatusPipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}

	// 17. DDGI Sample From Probes Compute Pipeline
	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/Sample_GI_Probes.comp.spv");
		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(RTpcInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->DDGISamplingDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		SampleDDGIComputePipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		SampleDDGIComputePassPipeline = pipelineManager.creatComputePipeline(SampleDDGIComputePipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}

	// 18. SSGI Compute Pipeline
	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/SSGI.comp.spv");
		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &SSGI_FullScreenQuad->descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		SSGIPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		SSGIPipeline = pipelineManager.creatComputePipeline(SSGIPipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}
}

uint32_t App::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void App::createShaderBindingTable() {
	// 1. Lighting SBT
	{
		const size_t handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 4;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(DeferedLightingPassPipeline),
			0,
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		Lighting_raygenShaderBindingTableBuffer.BufferID = "Lighting raygen Shader Binding Table Buffer";
		Lighting_missShaderBindingTableBuffer.BufferID   = "Lighting miss Shader Binding Table Buffer";
		Lighting_hitShaderBindingTableBuffer.BufferID    = "Lighting hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&Lighting_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&Lighting_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&Lighting_missShaderBindingTableBuffer, handleSizeAligned * 2, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		std::vector<uint8_t> rayGenData(handleSizeAligned, 0);
		memcpy(rayGenData.data(), shaderHandleStorage.data(), handleSize);
		bufferManger.CopyDataToBuffer(rayGenData.data(), Lighting_raygenShaderBindingTableBuffer);

		std::vector<uint8_t> hitData(handleSizeAligned, 0);
		memcpy(hitData.data(), shaderHandleStorage.data() + handleSize, handleSize);
		bufferManger.CopyDataToBuffer(hitData.data(), Lighting_hitShaderBindingTableBuffer);

		std::vector<uint8_t> missData(handleSizeAligned * 2, 0);
		memcpy(missData.data(), shaderHandleStorage.data() + (handleSize * 2), handleSize);
		memcpy(missData.data() + handleSizeAligned, shaderHandleStorage.data() + (handleSize * 3), handleSize);
		bufferManger.CopyDataToBuffer(missData.data(), Lighting_missShaderBindingTableBuffer);
	}

	// 2. DDGI SBT
	{
		const size_t handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 4;

		std::vector<uint8_t> shaderHandleStorage(groupcount * handleSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(RT_DDGIPassPipeline),
			0,
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		DDGI_raygenShaderBindingTableBuffer.BufferID = "DDGI raygen Shader Binding Table Buffer";
		DDGI_missShaderBindingTableBuffer.BufferID = "DDGI miss Shader Binding Table Buffer";
		DDGI_hitShaderBindingTableBuffer.BufferID = "DDGI hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&DDGI_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&DDGI_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&DDGI_missShaderBindingTableBuffer, handleSizeAligned * 2, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		std::vector<uint8_t> rayGenData(handleSizeAligned, 0);
		memcpy(rayGenData.data(), shaderHandleStorage.data(), handleSize);
		bufferManger.CopyDataToBuffer(rayGenData.data(), DDGI_raygenShaderBindingTableBuffer);

		std::vector<uint8_t> hitData(handleSizeAligned, 0);
		memcpy(hitData.data(), shaderHandleStorage.data() + handleSize, handleSize);
		bufferManger.CopyDataToBuffer(hitData.data(), DDGI_hitShaderBindingTableBuffer);

		std::vector<uint8_t> missData(handleSizeAligned * 2, 0);
		memcpy(missData.data(), shaderHandleStorage.data() + (2 * handleSize), handleSize);
		memcpy(missData.data() + handleSizeAligned, shaderHandleStorage.data() + (3 * handleSize), handleSize);
		bufferManger.CopyDataToBuffer(missData.data(), DDGI_missShaderBindingTableBuffer);
	}

	// 3. ReSTIR Temporal SBT
	{
		const size_t handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 3;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(ReSTIR_Temporal_RT_PassPipeline),
			0,
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		ReSTIR_DI_Temporal_raygenShaderBindingTableBuffer.BufferID = "ReSTIR_DI temporal raygen Shader Binding Table Buffer";
		ReSTIR_DI_Temporal_missShaderBindingTableBuffer.BufferID   = "ReSTIR_DI temporal miss Shader Binding Table Buffer";
		ReSTIR_DI_Temporal_hitShaderBindingTableBuffer.BufferID    = "ReSTIR_DI temporal hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&ReSTIR_DI_Temporal_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&ReSTIR_DI_Temporal_missShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&ReSTIR_DI_Temporal_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data(), ReSTIR_DI_Temporal_raygenShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, ReSTIR_DI_Temporal_hitShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, ReSTIR_DI_Temporal_missShaderBindingTableBuffer);
	}

	// 4. ReSTIR Spatial SBT
	{
		const size_t handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 3;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(ReSTIR_SPATIAL_RT_PassPipeline),
			0,
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		ReSTIR_DI_Spatial_raygenShaderBindingTableBuffer.BufferID = "ReSTIR_DI spatial raygen Shader Binding Table Buffer";
		ReSTIR_DI_Spatial_missShaderBindingTableBuffer.BufferID   = "ReSTIR_DI spatial miss Shader Binding Table Buffer";
		ReSTIR_DI_Spatial_hitShaderBindingTableBuffer.BufferID    = "ReSTIR_DI spatial hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&ReSTIR_DI_Spatial_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&ReSTIR_DI_Spatial_missShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&ReSTIR_DI_Spatial_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data(), ReSTIR_DI_Spatial_raygenShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, ReSTIR_DI_Spatial_hitShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, ReSTIR_DI_Spatial_missShaderBindingTableBuffer);
	}
}

void App::DestroyShaderBindingTable() {
	bufferManger.DestroyBuffer(DDGI_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(DDGI_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(DDGI_hitShaderBindingTableBuffer);

	bufferManger.DestroyBuffer(ReSTIR_DI_Temporal_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(ReSTIR_DI_Temporal_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(ReSTIR_DI_Temporal_hitShaderBindingTableBuffer);

	bufferManger.DestroyBuffer(ReSTIR_DI_Spatial_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(ReSTIR_DI_Spatial_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(ReSTIR_DI_Spatial_hitShaderBindingTableBuffer);

	bufferManger.DestroyBuffer(Lighting_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(Lighting_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(Lighting_hitShaderBindingTableBuffer);
}

void App::destroyPipeline() {
	vk::Pipeline pipelines[] = {
		DeferedLightingPassPipeline, FXAAPassPipeline, LightgraphicsPipeline, SkyBoxgraphicsPipeline,
		geometryPassPipeline, SSAOPipeline, SSAOBlurPipeline, SSRPipeline, RT_ShadowsPassPipeline,
		SSGIPipeline, CombinedImagePassPipeline, Gamma_Corrected_IMGUI_PassPipeline,
		BluredRTreflectionPipeline, DDGIProbePipeline, GridComputePassPipeline, RT_DDGIPassPipeline,
		IrradianceComputePassPipeline, SampleDDGIComputePassPipeline, ProbeStatusComputePassPipeline,
		ReSTIR_Temporal_RT_PassPipeline, ReSTIR_SPATIAL_RT_PassPipeline
	};

	for (auto pipeline : pipelines) {
		if (pipeline) vulkanContext.LogicalDevice.destroyPipeline(pipeline);
	}

	vk::PipelineLayout layouts[] = {
		DeferedLightingPassPipelineLayout, FXAAPassPipelineLayout, LightpipelineLayout,
		SkyBoxpipelineLayout, geometryPassPipelineLayout, SSAOPipelineLayout, SSAOBlurPipelineLayout,
		SSRPipelineLayout, RT_ShadowsPipelineLayout, SSGIPipelineLayout, CombinedImagePipelineLayout,
		BluredRTreflectionsPipelineLayout, DDGIProbepipelineLayout, GridComputePipelineLayout,
		RT_DDGIPipelineLayout, IrradianceComputePipelineLayout, SampleDDGIComputePipelineLayout,
		ProbeStatusPipelineLayout, ReSTIR_Temporal_RT_PipelineLayout, ReSTIR_Spatial_RT_PipelineLayout,
		Gamma_Corrected_IMGUI_PipelineLayout
	};

	for (auto layout : layouts) {
		if (layout) vulkanContext.LogicalDevice.destroyPipelineLayout(layout);
	}
}

void App::recreatePipeline()
{
	vulkanContext.LogicalDevice.waitIdle();
	DestroyShaderBindingTable();
	destroyPipeline();
	CreateGraphicsPipeline();
	createShaderBindingTable();
}
