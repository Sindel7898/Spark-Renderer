#pragma once
#include "VulkanContext.h"
#include"ShaderHelper.h"
#include"VertexInputLayouts.h"

struct FullScreen_Quad_Pipeline_Data
{
	vk::PipelineLayout FQ_PipelineLayout;
	vk::Pipeline       FQ_Pipeline;

};
class PipelineManager
{
  public:
      PipelineManager(VulkanContext* vulkanContextRef);

	  FullScreen_Quad_Pipeline_Data create_FQ_Pipeline(std::string PathToFragmentShader, vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo, vk::PipelineLayoutCreateInfo pipelineLayoutInfo);




      void CreateGraphicsPipeline();


	  vk::ShaderModule createShaderModule(const std::vector<char>& code);
	  vk::Pipeline createGraphicsPipeline(vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo, vk::PipelineShaderStageCreateInfo ShaderStages[], vk::PipelineVertexInputStateCreateInfo* vertexInputInfo,
		  vk::PipelineInputAssemblyStateCreateInfo* inputAssembleInfo, vk::PipelineViewportStateCreateInfo viewportState, vk::PipelineRasterizationStateCreateInfo rasterizerinfo,
		  vk::PipelineMultisampleStateCreateInfo multisampling, vk::PipelineDepthStencilStateCreateInfo depthStencilState, vk::PipelineColorBlendStateCreateInfo colorBlend,
		  vk::PipelineDynamicStateCreateInfo DynamicState, vk::PipelineLayout& pipelineLayout, int numOfShaderStages = 2);


	  vk::Pipeline createRayTracingGraphicsPipeline(vk::PipelineLayout pipelineLayout, std::vector<vk::PipelineShaderStageCreateInfo> ShaderStage, std::vector<vk::RayTracingShaderGroupCreateInfoKHR> RayTracingshaderGroups);

private:
	
	VulkanContext* vulkanContext = nullptr;
};

//static inline PipelineManagerDeleter(PipelineManager* PipelineManager) {
//
//    
//}
