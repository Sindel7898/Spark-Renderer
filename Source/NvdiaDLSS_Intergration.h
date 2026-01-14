#pragma once
#include <vulkan/vulkan.hpp>
#include <minwindef.h>


class NvdiaDLSS_Intergration
{
public:

	void InitDLSS();
	void CleanUp();
	PFN_vkGetInstanceProcAddr          sl_vkGetInstanceProcAddr = nullptr;
};

static inline void VulkanContextDeleter(NvdiaDLSS_Intergration* nvdiaDLSS_Intergration)
{
	if (nvdiaDLSS_Intergration)
	{
		nvdiaDLSS_Intergration->CleanUp();
		delete nvdiaDLSS_Intergration;
	}
}