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
class SkyBox;

class UserInterface
{
public:
    UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager);

    void RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex, ImageData& DrawingImage);

    void DrawUi(App* appref, SkyBox* skyBox, VulkanContext* vulkanContext);

    float CalculateDistanceInScreenSpace(glm::mat4 CameraProjection, glm::mat4 cameraview, glm::vec3 position);
    void SetLightCount(int count) { NumberOfLights = count; }


    vk::Extent3D GetRenderTextureExtent();

    BufferManager* buffermanager = nullptr;
    // VkDescriptorSet RenderTextureId;
    VulkanContext* vulkancontext = nullptr;
    vk::DescriptorPool  ImGuiDescriptorPool = nullptr;


    void CleanUp();

    ~UserInterface();


private:
    void InitImgui();
    void SetupDockingEnvironment();


    Window* window = nullptr;
    vk::Extent3D RenderTextureExtent = (0, 0, 0);

    int UserInterfaceItemsIndex = -1;
    int selectedLightIndex = -1;
    int SelectedInstanceIndex = -1;


    bool useSnap = false;
    float snap[3] = { 1.f, 1.f, 1.f };

    ImGuizmo::OPERATION currentGizmoOperation;
    ImGuizmo::MODE currentGizmoMode;

    std::vector<std::string> Passes{"SSGI Pass","DDGI Pass","ReSTIR DI","Lighting Pass"};
    std::string currentPass = "Lighting Pass";

    std::vector<std::string> items{ "Directional", "Point" };
    std::string currentItem = "Point";

    std::vector<std::string> SkyBoxs{ "Day Sky", "Church", "Night Sky","City","Black"};
    std::string currentSkyBox = "Day Sky";

    std::vector<std::string> DDGI_Vertex_Options{ "First Vertex", "Second Vertex" };
    std::string currentDDGIVertex = "First Vertex";

    std::vector<std::string> GlobalIllumination_Solution{ "DDGI", "SSGI","DDGI + SSGI","PT"};
    std::string currentGI_Solution = "DDGI";

    ImVec2 viewportSize;

    glm::mat4 LastModelMatrix;

	int NumberOfLights = 0;

    int DLSSFRAMELIMIT = 10;

};

static inline void UserInterfaceDeleter(UserInterface* userInterface) {

    if (userInterface) {

        userInterface->CleanUp();
        delete userInterface;
    }

};