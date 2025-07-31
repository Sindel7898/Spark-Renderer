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
#include <glm/gtc/type_ptr.hpp>

class Window;
class BufferManager;
class Camera;
class Model;
class Light;
class SSA0_FullScreenQuad;
class App;

class UserInterface
{
 public:
    UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager);

    void RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex);
    void DrawUi(App* appref);
    float CalculateDistanceInScreenSpace(glm::mat4 CameraProjection, glm::mat4 cameraview, glm::vec3 position);
  
    

    vk::Extent3D GetRenderTextureExtent();

    BufferManager* buffermanager = nullptr;
   // VkDescriptorSet RenderTextureId;
    VulkanContext* vulkancontext = nullptr;
    vk::DescriptorPool  ImGuiDescriptorPool = nullptr;


 private:
    void InitImgui();
    void SetupDockingEnvironment();

    Window* window = nullptr;
    vk::Extent3D RenderTextureExtent = (0, 0, 0);

    int UserInterfaceItemsIndex = -1;
    int selectedLightIndex = -1;


    bool useSnap = false;
    float snap[3] = { 1.f, 1.f, 1.f };
    
     ImGuizmo::OPERATION currentGizmoOperation;
     ImGuizmo::MODE currentGizmoMode;

     std::vector<std::string> Passes{"Position Pass", "Normal Pass", "Albedo Pass","SSAO Pass", "RT Shadow Pass","Light Pass"};
     std::string currentPass = "Light Pass";

     std::vector<std::string> items{ "Directional", "Point", "Spot" };
     std::string currentItem = "Point";

     ImVec2 viewportSize;

};

static inline void UserInterfaceDeleter(UserInterface* userInterface) {

   if (userInterface) {

       userInterface->vulkancontext->LogicalDevice.waitIdle();
       // ImGui_ImplVulkan_RemoveTexture(userInterface->RenderTextureId);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(); 
        userInterface->vulkancontext->LogicalDevice.destroyDescriptorPool(userInterface->ImGuiDescriptorPool);
   }
 
};