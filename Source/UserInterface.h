#pragma once

#define ENABLE_NVPERF 1

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
#if ENABLE_NVPERF
#include "NvPerfPeriodicSamplerVulkan.h"
#endif

class Window;
class BufferManager;
class Camera;
class Model;
class Light;
class SSA0_FullScreenQuad;
class App;
class SkyBox;

#if ENABLE_NVPERF
using namespace nv::perf;
using namespace nv::perf::sampler;
using namespace nv::perf::mini_trace;
#endif

class UserInterface
{
public:
    UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager);

    void InitNVPerf();
    void SaveNVPerf();
    void RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex, ImageData& DrawingImage);

    void DrawUi(App* appref, SkyBox* skyBox, VulkanContext* vulkanContext);

    float CalculateDistanceInScreenSpace(glm::mat4 CameraProjection, glm::mat4 cameraview, glm::vec3 position);
    void SetLightCount(int count) { NumberOfLights = count; }


    vk::Extent3D GetRenderTextureExtent();

    BufferManager* buffermanager = nullptr;
    VulkanContext* vulkancontext = nullptr;
    vk::DescriptorPool  ImGuiDescriptorPool = nullptr;

    void CleanUp();
    ~UserInterface();

    static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 80.0f);

#if ENABLE_NVPERF
    std::vector<APITracerVulkan> m_apiTracers;
    bool m_nvperfReady = false;
#endif

    bool Save_To_File = false;

    // Panel visibility
    bool showOutliner = true;
    bool showDetails = true;
    bool showSettings = true;
    bool showDDGIAtlas = false;
    bool showPerformanceOverlay = true;

private:
    void InitImgui();
    void ApplyModernTheme();
    void SetupDockingEnvironment();
    void DrawMenuBar(App* appref, SkyBox* skyBox, VulkanContext* vulkanContext);
    void DrawViewportToolbar(App* appref, VulkanContext* vulkanContext);

    Window* window = nullptr;
    vk::Extent3D RenderTextureExtent = {0, 0, 0};

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

    std::vector<std::string> GlobalIllumination_Solution{ "DDGI","PT","None" };
    std::string currentGI_Solution = "DDGI";

    ImVec2 viewportSize;
    glm::mat4 LastModelMatrix;

    int NumberOfLights = 0;
    int DLSSFRAMELIMIT = 10;

#if ENABLE_NVPERF
    PeriodicSamplerOneShotVulkan m_periodicSamplerOneShot;
#endif

    std::vector<size_t> m_frameLevelTraceIndice;
    std::string m_outputDirectory;
};

static inline void UserInterfaceDeleter(UserInterface* userInterface) {
    if (userInterface) {
        userInterface->CleanUp();
        delete userInterface;
    }
};