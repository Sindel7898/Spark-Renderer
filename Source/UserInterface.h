#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "BufferManager.h"
#include "imgui.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui_internal.h>

class VulkanContext;
class Window;
class BufferManager;

class UserInterface
{
public:
    UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager);

    void RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex);
    ImageData* CreateViewPortRenderTexture(uint32_t X, uint32_t Y);
    vk::Extent3D GetRenderTextureExtent();
    void DrawUi(bool& bRecreateDepth);
    void ImguiViewPortRenderTextureSizeDecider(bool& bRecreateDepth);

    BufferManager* buffermanager = nullptr;
    ImageData ImguiViewPortRenderTextureData;

private:
    void InitImgui();
    void SetupDockingEnvironment();

    vk::DescriptorPool  ImGuiDescriptorPool = nullptr;
    VulkanContext* vulkancontext = nullptr;
    Window* window = nullptr;
    VkDescriptorSet RenderTextureId;
    vk::Extent3D RenderTextureExtent = (0, 0, 0);
};

struct UserInterfaceDeleter {

    void operator()(UserInterface* UserInterface) const {

        if (UserInterface) {

             ImGui_ImplVulkan_Shutdown();
             ImGui_ImplGlfw_Shutdown();
             ImGui::DestroyContext(); 
             UserInterface->buffermanager->DestroyImage(UserInterface->ImguiViewPortRenderTextureData);

        }
    }
};