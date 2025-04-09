#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "BufferManager.h"
#include "imgui.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include "vulkanContext.h"
#include "ImGuizmo.h"

class Window;
class BufferManager;
class Camera;
class Model;

class UserInterface
{
public:
    UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager);

    void RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex);
    void DrawUi(bool& bRecreateDepth, Camera* camera, Model* selectedModel);
    ImageData* CreateViewPortRenderTexture(uint32_t X, uint32_t Y);
    vk::Extent3D GetRenderTextureExtent();
    void ImguiViewPortRenderTextureSizeDecider(bool& bRecreateDepth);

    BufferManager* buffermanager = nullptr;
    ImageData ImguiViewPortRenderTextureData;
    VkDescriptorSet RenderTextureId;
    VulkanContext* vulkancontext = nullptr;
    vk::DescriptorPool  ImGuiDescriptorPool = nullptr;


private:
    void InitImgui();
    void SetupDockingEnvironment();

    Window* window = nullptr;
    vk::Extent3D RenderTextureExtent = (0, 0, 0);



    bool useSnap = false;
    float snap[3] = { 1.f, 1.f, 1.f };
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
};

struct UserInterfaceDeleter {

    void operator()(UserInterface* userInterface) const {

        if (userInterface) {
            userInterface->vulkancontext->LogicalDevice.waitIdle();
             ImGui_ImplVulkan_RemoveTexture(userInterface->RenderTextureId);
             ImGui_ImplVulkan_Shutdown();
             ImGui_ImplGlfw_Shutdown();
             ImGui::DestroyContext(); 
             userInterface->vulkancontext->LogicalDevice.destroyDescriptorPool(userInterface->ImGuiDescriptorPool);
             userInterface->buffermanager->DestroyImage(userInterface->ImguiViewPortRenderTextureData);

        }
    }
};