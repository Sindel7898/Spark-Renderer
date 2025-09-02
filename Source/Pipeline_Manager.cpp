#include "Pipeline_Manager.h"

PipelineManager::PipelineManager(VulkanContext* vulkanContextRef)
{
	vulkanContext = vulkanContextRef;

}

FullScreen_Quad_Pipeline_Data PipelineManager::create_FQ_Pipeline(std::string PathToFragmentShader, vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo,vk::PipelineLayoutCreateInfo pipelineLayoutInfo)
{

	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	depthStencilState.depthTestEnable = vk::False;
	depthStencilState.depthWriteEnable = vk::False;
	depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.stencilTestEnable = vk::False;


	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

	/////////////////////////////////////////////////////////////////////////////
	vk::Viewport viewport{};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setHeight((float)vulkanContext->swapchainExtent.height);
	viewport.setWidth((float)vulkanContext->swapchainExtent.width);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Offset2D scissorOffset = { 0,0 };

	vk::Rect2D scissor{};
	scissor.setOffset(scissorOffset);
	scissor.setExtent(vulkanContext->swapchainExtent);

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.setViewportCount(1);
	viewportState.setViewportCount(1);
	viewportState.setScissorCount(1);
	viewportState.setViewports(viewport);
	viewportState.setScissors(scissor);
	////////////////////////////////////////////////////////////////////////////////

	// Rasteriser information
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

	//Multi Sampling/
	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = vk::False;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = vk::False;
	multisampling.alphaToOneEnable = vk::False;

	///////////// Color blending *COME BACK TO THIS////////////////////
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

	auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
	auto FragShaderCode = readFile(PathToFragmentShader);

	VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
	VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

	vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
	FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	FragmentShaderStageInfo.module = FragShaderModule;
	FragmentShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };

	auto BindDesctiptions      = FullScreenQuadDescription::GetBindingDescription();
	auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexBindingDescriptionCount(1);
	vertexInputInfo.setVertexAttributeDescriptionCount(2);
	vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
	vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());



	vk::PipelineLayout	FQ_PipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

	vk::Pipeline FQ_Pipeline = createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
		viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, FQ_PipelineLayout);

	vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
	vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	FullScreen_Quad_Pipeline_Data resultPipline;
	resultPipline.FQ_Pipeline = FQ_Pipeline;
	resultPipline.FQ_PipelineLayout = FQ_PipelineLayout;

	return resultPipline;
}


vk::Pipeline PipelineManager::createGraphicsPipeline(vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo, vk::PipelineShaderStageCreateInfo ShaderStages[], vk::PipelineVertexInputStateCreateInfo* vertexInputInfo, vk::PipelineInputAssemblyStateCreateInfo* inputAssembleInfo, vk::PipelineViewportStateCreateInfo viewportState, vk::PipelineRasterizationStateCreateInfo rasterizerinfo, vk::PipelineMultisampleStateCreateInfo multisampling, vk::PipelineDepthStencilStateCreateInfo depthStencilState, vk::PipelineColorBlendStateCreateInfo colorBlend, vk::PipelineDynamicStateCreateInfo DynamicState, vk::PipelineLayout& pipelineLayout, int numOfShaderStages)
{

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &pipelineRenderingCreateInfo; // Add this line
	pipelineInfo.stageCount = numOfShaderStages;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = vertexInputInfo;
	pipelineInfo.pInputAssemblyState = inputAssembleInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerinfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &DynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;


	vk::Pipeline graphicsPipeline;
	vk::Result result = vulkanContext->LogicalDevice.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	return graphicsPipeline;
}

vk::Pipeline PipelineManager::createRayTracingGraphicsPipeline(vk::PipelineLayout pipelineLayout, std::vector<vk::PipelineShaderStageCreateInfo> ShaderStage, std::vector<vk::RayTracingShaderGroupCreateInfoKHR> RayTracingshaderGroups)
{
	vk::RayTracingPipelineCreateInfoKHR rtPipelineInfo{};
	rtPipelineInfo.stageCount = ShaderStage.size();
	rtPipelineInfo.pStages = ShaderStage.data();
	rtPipelineInfo.groupCount = RayTracingshaderGroups.size();
	rtPipelineInfo.pGroups = RayTracingshaderGroups.data();
	rtPipelineInfo.maxPipelineRayRecursionDepth = 3;            // typical minimum  *******REMEMBER TO ASK GPU INSTEAD********
	rtPipelineInfo.layout = pipelineLayout;

	VkRayTracingPipelineCreateInfoKHR rtinfo = rtPipelineInfo;

	VkPipeline TEMP_graphicsPipeline;

	VkResult result = vulkanContext->vkCreateRayTracingPipelinesKHR(vulkanContext->LogicalDevice, nullptr, nullptr, 1, &rtinfo, nullptr, &TEMP_graphicsPipeline);

	if (result != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Ray traced graphics pipeline!");
	}

	vk::Pipeline graphicsPipeline = TEMP_graphicsPipeline;

	return graphicsPipeline;
}



vk::ShaderModule PipelineManager::createShaderModule(const std::vector<char>& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	vk::ShaderModule Shader;

	if (vulkanContext->LogicalDevice.createShaderModule(&createInfo, nullptr, &Shader) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return Shader;
}