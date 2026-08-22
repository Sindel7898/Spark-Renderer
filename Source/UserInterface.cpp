#include "UserInterface.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>
#include "Window.h"
#include "Camera.h"
#include "Model.h"
#include "MeshLoader.h"
#include "Light.h"
#include "SSAO_FullScreenQuad.h"
#include "App.h"
#include "FXAA_FullScreenQuad.h"
#include "CombinedResult_FullScreenQuad.h"

#include <nvperf_host_impl.h>
#include <NvPerfHudDataModel.h>
#include <NvPerfMetricConfigurationsHAL.h>
#include <NvPerfPeriodicSamplerVulkan.h>
#include <NvPerfMiniTraceVulkan.h>
#include <NvPerfHudImPlotRenderer.h>

#define RYML_SINGLE_HDR_DEFINE_NOW
#include <ryml_all.hpp>

UserInterface::UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager)
{
	vulkancontext = vulkancontextRef;
	window = WindowRef;
	buffermanager = Buffermanager;
	currentGizmoOperation = ImGuizmo::TRANSLATE;
	currentGizmoMode = ImGuizmo::WORLD;

    InitImgui();
    InitNVPerf();
}

void UserInterface::InitNVPerf() {
#if ENABLE_NVPERF
    auto onStopSampling = [this](const char* outputDirectory) {
        m_outputDirectory = outputDirectory;
    };

    const size_t numRangesPerFrame = 15;
    const size_t samplingIntervalInNanoSeconds = 1024 * 16;
    const size_t maxIntervalPerFrameInNanoSeconds = 100 * 1000 * 1000; // 100ms
    const size_t numFramesToSample = 20;

    try {
        nv::perf::DeviceIdentifiers deviceIdentifiers = nv::perf::VulkanGetDeviceIdentifiers(
            vulkancontext->VulkanInstance, vulkancontext->PhysicalDevice, vulkancontext->LogicalDevice);

        nv::perf::hud::HudPresets hudPresets;
        hudPresets.Initialize(deviceIdentifiers.pChipName);
        const std::string hudConfigurationName = "Graphics High Speed Triage";
        nv::perf::hud::HudDataModel hudDataModel;
        hudDataModel.Load(hudPresets.GetPreset(hudConfigurationName));

        std::string metricConfigName;
        nv::perf::MetricConfigurations::GetMetricConfigNameBasedOnHudConfigurationName(
            metricConfigName, deviceIdentifiers.pChipName, hudConfigurationName);
        nv::perf::MetricConfigObject metricConfigObject;
        nv::perf::MetricConfigurations::LoadMetricConfigObject(
            metricConfigObject, deviceIdentifiers.pChipName, metricConfigName);

        hudDataModel.Initialize(metricConfigObject);

        bool initOk = m_periodicSamplerOneShot.Initialize(
            vulkancontext->VulkanInstance, vulkancontext->PhysicalDevice, vulkancontext->LogicalDevice,
            samplingIntervalInNanoSeconds, maxIntervalPerFrameInNanoSeconds,
            numFramesToSample, onStopSampling, hudDataModel.GetCounterConfiguration());

        if (!initOk) {
            // NVPA_STATUS_INSUFFICIENT_PRIVILEGE: GPU performance counters require
            // admin rights or a driver registry setting.
            // Run once as admin, or set: HKLM\SYSTEM\CurrentControlSet\Services\nvlddmkm\Global\NVrmAllowedClients = NVSDK_NGX_ApplicationId
            printf("[NVPerf] WARNING: GPU performance counters unavailable (insufficient privilege).\n");
            printf("[NVPerf] Profiling disabled. To enable, run as Administrator or set driver registry key.\n");
            return;
        }

        m_periodicSamplerOneShot.m_outputOption.directoryName = "TEST";
        m_frameLevelTraceIndice.resize(numFramesToSample, 0);
        m_apiTracers.resize(numFramesToSample);

        for (auto& apiTracer : m_apiTracers) {
            apiTracer.Initialize(vulkancontext->VulkanInstance, vulkancontext->PhysicalDevice,
                                 vulkancontext->LogicalDevice, numRangesPerFrame);
        }

        m_nvperfReady = true;
    }
    catch (const std::exception& e) {
        printf("[NVPerf] Initialization failed: %s\n", e.what());
        printf("[NVPerf] Profiling will be unavailable this session.\n");
    }
#endif
}

void UserInterface::SaveNVPerf() {
#if ENABLE_NVPERF
    if (!m_nvperfReady) return; // NVPerf didn't initialize (e.g. insufficient privilege)
    m_periodicSamplerOneShot.OnFrameEnd();

    {
        std::vector<APITraceData> apiTraceData;
        for (auto& apiTracer : m_apiTracers) {
            apiTracer.ResolveQueries(apiTraceData);
        }

        struct PassStats {
            double totalTimeMs = 0.0;
            int count = 0;
            int nestingLevel = 0;
        };

        std::map<std::string, PassStats> aggregatedStats;

        for (const auto& trace : apiTraceData) {
            double startMs = static_cast<double>(trace.startTimestamp) /  1000000.0;
            double endMs   = static_cast<double>(trace.endTimestamp  )  / 1000000.0;

            aggregatedStats[trace.name].totalTimeMs += (endMs - startMs);
            aggregatedStats[trace.name].count++;
            aggregatedStats[trace.name].nestingLevel = trace.nestingLevel;
        }

        std::stringstream yamlStream;

        for (const auto& pair : aggregatedStats) {
            double avgTimeMs = pair.second.totalTimeMs / pair.second.count;

            yamlStream << "  - name: "         << pair.first               << "\n"
                       << "    avgFrameTime: " << avgTimeMs                << "\n"
                       << "    samples: "      << pair.second.count        << "\n"
                       << "    nestingLevel: " << pair.second.nestingLevel << "\n";
        }

        if (Save_To_File) {
            const std::string filename = "traces.yaml";
            std::ofstream file(filename);
            if (file.is_open()) {
                file << yamlStream.str();
            }
            else {
                NV_PERF_LOG_ERR(10, "Failed to open file: %s\n", filename.c_str());
            }
            Save_To_File = false;
        }
    }
#endif
}

void UserInterface::ApplyModernTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding & Sizing
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 5.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 5.0f;

    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 5.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 11.0f;
    style.GrabMinSize       = 10.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;

    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);

    ImVec4* colors = style.Colors;

    // Pitch Obsidian / True-Black Palette
    const ImVec4 bgVoid         = ImVec4(0.020f, 0.020f, 0.025f, 1.00f); // #050506 True Black
    const ImVec4 bgPanel        = ImVec4(0.040f, 0.040f, 0.048f, 1.00f); // #0a0a0c Child Panels
    const ImVec4 bgFrame        = ImVec4(0.065f, 0.065f, 0.078f, 1.00f); // #101014 Input Frames & Combos
    const ImVec4 bgFrameHover   = ImVec4(0.105f, 0.105f, 0.125f, 1.00f);
    const ImVec4 bgFrameActive  = ImVec4(0.145f, 0.145f, 0.175f, 1.00f);
    const ImVec4 bgHeader       = ImVec4(0.055f, 0.055f, 0.068f, 1.00f); // Collapsing Headers
    const ImVec4 bgHeaderHover  = ImVec4(0.095f, 0.095f, 0.118f, 1.00f);
    const ImVec4 accentPrimary  = ImVec4(0.220f, 0.500f, 0.950f, 1.00f); // Electric Spark Blue
    const ImVec4 accentHover    = ImVec4(0.320f, 0.600f, 1.000f, 1.00f);
    const ImVec4 accentActive   = ImVec4(0.160f, 0.400f, 0.820f, 1.00f);
    const ImVec4 textPrimary    = ImVec4(0.920f, 0.930f, 0.950f, 1.00f); // Crisp White/Slate
    const ImVec4 textDim        = ImVec4(0.420f, 0.440f, 0.490f, 1.00f);
    const ImVec4 borderCol      = ImVec4(0.100f, 0.100f, 0.125f, 0.85f); // Subtle Sharp Border
    const ImVec4 borderHover    = ImVec4(0.200f, 0.220f, 0.280f, 1.00f);
    const ImVec4 tabBg          = ImVec4(0.030f, 0.030f, 0.038f, 1.00f);
    const ImVec4 tabActive      = ImVec4(0.070f, 0.070f, 0.088f, 1.00f);
    const ImVec4 menuBg         = ImVec4(0.015f, 0.015f, 0.020f, 1.00f);

    colors[ImGuiCol_Text]                  = textPrimary;
    colors[ImGuiCol_TextDisabled]          = textDim;
    colors[ImGuiCol_WindowBg]              = bgVoid;
    colors[ImGuiCol_ChildBg]               = bgPanel;
    colors[ImGuiCol_PopupBg]               = ImVec4(0.030f, 0.030f, 0.038f, 0.98f);
    colors[ImGuiCol_Border]                = borderCol;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]               = bgFrame;
    colors[ImGuiCol_FrameBgHovered]        = bgFrameHover;
    colors[ImGuiCol_FrameBgActive]         = bgFrameActive;

    colors[ImGuiCol_TitleBg]               = bgVoid;
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.045f, 0.045f, 0.055f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = bgVoid;

    colors[ImGuiCol_MenuBarBg]             = menuBg;
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.015f, 0.015f, 0.020f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.120f, 0.120f, 0.150f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.180f, 0.180f, 0.220f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = accentPrimary;

    colors[ImGuiCol_CheckMark]             = accentPrimary;
    colors[ImGuiCol_SliderGrab]            = accentPrimary;
    colors[ImGuiCol_SliderGrabActive]      = accentActive;

    colors[ImGuiCol_Button]                = bgFrame;
    colors[ImGuiCol_ButtonHovered]         = bgFrameHover;
    colors[ImGuiCol_ButtonActive]          = bgFrameActive;

    colors[ImGuiCol_Header]                = bgHeader;
    colors[ImGuiCol_HeaderHovered]         = bgHeaderHover;
    colors[ImGuiCol_HeaderActive]          = accentPrimary;

    colors[ImGuiCol_Separator]             = borderCol;
    colors[ImGuiCol_SeparatorHovered]      = borderHover;
    colors[ImGuiCol_SeparatorActive]       = accentPrimary;

    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.080f, 0.080f, 0.110f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = accentHover;
    colors[ImGuiCol_ResizeGripActive]      = accentActive;

    colors[ImGuiCol_Tab]                   = tabBg;
    colors[ImGuiCol_TabHovered]            = ImVec4(0.100f, 0.100f, 0.130f, 1.00f);
    colors[ImGuiCol_TabActive]             = tabActive;
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.025f, 0.025f, 0.032f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.050f, 0.050f, 0.065f, 1.00f);

    colors[ImGuiCol_DockingPreview]        = ImVec4(0.220f, 0.500f, 0.950f, 0.35f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.010f, 0.010f, 0.015f, 1.00f);
}

void UserInterface::SetLightingMode(LightingMode mode, App* appref, VulkanContext* vulkanContext)
{
    currentLightingMode = mode;
    switch (mode)
    {
    case LightingMode::PathTraced:
        appref->DefferedDecider = 3;
        appref->lighting_RTX->GISolutionIndex = 1;
        currentPass = "Lighting Pass";
        currentGI_Solution = "PT";
        currentLightingModeName = "Path Traced Lighting";
        break;
    case LightingMode::BruteForce:
        appref->DefferedDecider = 3;
        appref->lighting_RTX->GISolutionIndex = 0;
        currentPass = "Lighting Pass";
        currentGI_Solution = "DDGI";
        currentLightingModeName = "Brute Force Lighting";
        break;
    case LightingMode::ReSTIR:
        appref->DefferedDecider = 2;
        currentPass = "ReSTIR DI";
        currentLightingModeName = "ReSTIR DI";
        break;
    }
    appref->DLSS_Intergration.SceneChangeNotifer = 1;
    vulkanContext->ResetFrameCount();
}

void UserInterface::DrawNodeTree(const std::shared_ptr<Node>& node, Model* model, const std::string& filter, VulkanContext* vulkanContext, App* appref)
{
    if (!node) return;

    std::string displayName = node->name;
    if (displayName.empty()) {
        displayName = "Node_" + std::to_string(node->id);
    }

    bool hasChildren = !node->children.empty();
    bool matchesFilter = filter.empty() || displayName.find(filter) != std::string::npos;

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (selectedNode == node.get()) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isOpened = ImGui::TreeNodeEx((void*)(intptr_t)node->id, nodeFlags, "[Obj] %s", displayName.c_str());

    if (ImGui::IsItemClicked()) {
        selectedNode = node.get();
        selectedModel = model;
        selectedLightIndex = -1;
        for (size_t m = 0; m < appref->UserInterfaceItems.size(); ++m) {
            if (appref->UserInterfaceItems[m] == model) {
                UserInterfaceItemsIndex = static_cast<int>(m);
                SelectedInstanceIndex = 0;
                break;
            }
        }
    }

    if (hasChildren && isOpened) {
        for (const auto& child : node->children) {
            DrawNodeTree(child, model, filter, vulkanContext, appref);
        }
        ImGui::TreePop();
    }
}

void UserInterface::InitImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ApplyModernTheme();

	// Create descriptor pool for ImGui
	std::vector<vk::DescriptorPoolSize> pool_sizes =
	{
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};

	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();

	ImGuiDescriptorPool = vulkancontext->LogicalDevice.createDescriptorPool(pool_info);

	vk::PipelineRenderingCreateInfoKHR pipeline_rendering_create_info;
	pipeline_rendering_create_info.colorAttachmentCount = 1;
    static const vk::Format uiFormat = vk::Format::eR16G16B16A16Sfloat;
    pipeline_rendering_create_info.pColorAttachmentFormats = &uiFormat;
    pipeline_rendering_create_info.depthAttachmentFormat = vk::Format::eUndefined;
	pipeline_rendering_create_info.stencilAttachmentFormat = vk::Format::eUndefined;

	ImGui_ImplGlfw_InitForVulkan(window->GetWindow(), true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkancontext->VulkanInstance;
	init_info.PhysicalDevice = vulkancontext->PhysicalDevice;
	init_info.Device = vulkancontext->LogicalDevice;
	init_info.QueueFamily = vulkancontext->graphicsQueueFamilyIndex;
	init_info.Queue = vulkancontext->graphicsQueue;
	init_info.DescriptorPool = ImGuiDescriptorPool;
	init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
	init_info.ImageCount = vulkancontext->swapchainImageData.size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.UseDynamicRendering = true;
	init_info.PipelineRenderingCreateInfo = pipeline_rendering_create_info;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();
}

bool UserInterface::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
{
    bool changed = false;
    ImGui::PushID(label.c_str());

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

    float lineHeight = ImGui::GetFrameHeight();
    ImVec2 buttonSize = { lineHeight + 2.0f, lineHeight };

    // X Component (Red)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.10f, 0.15f, 1.0f));
    if (ImGui::Button("X", buttonSize)) { values.x = resetValue; changed = true; }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y Component (Green)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.80f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.50f, 0.20f, 1.0f));
    if (ImGui::Button("Y", buttonSize)) { values.y = resetValue; changed = true; }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z Component (Blue)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.40f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.50f, 1.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.30f, 0.70f, 1.0f));
    if (ImGui::Button("Z", buttonSize)) { values.z = resetValue; changed = true; }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) changed = true;
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();

    return changed;
}

void UserInterface::DrawMenuBar(App* appref, SkyBox* skyBox, VulkanContext* vulkanContext)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Reload Shaders", "F5")) {
                appref->recreatePipeline();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Scenes")) {
                for (int i = 0; i < appref->SceneNames.size(); i++) {
                    bool isSelected = (appref->currentSceneIndex == i);
                    if (ImGui::MenuItem(appref->SceneNames[i].c_str(), nullptr, isSelected)) {
                        if (appref->currentSceneIndex != i) {
                            appref->SwitchScene(i);
                            appref->DLSS_Intergration.SceneChangeNotifer = 1;
                        }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(window->GetWindow(), GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene Outliner", nullptr, &showOutliner);
            ImGui::MenuItem("Details Panel", nullptr, &showDetails);
            ImGui::MenuItem("Settings", nullptr, &showSettings);
            ImGui::MenuItem("DDGI Atlases", nullptr, &showDDGIAtlas);
            ImGui::MenuItem("Performance Overlay", nullptr, &showPerformanceOverlay);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Gizmo"))
        {
            if (ImGui::MenuItem("Translate", "1", currentGizmoOperation == ImGuizmo::TRANSLATE)) {
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            }
            if (ImGui::MenuItem("Rotate", "2", currentGizmoOperation == ImGuizmo::ROTATE)) {
                currentGizmoOperation = ImGuizmo::ROTATE;
            }
            if (ImGui::MenuItem("Scale", "3", currentGizmoOperation == ImGuizmo::SCALE)) {
                currentGizmoOperation = ImGuizmo::SCALE;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("World Space", nullptr, currentGizmoMode == ImGuizmo::WORLD)) {
                currentGizmoMode = ImGuizmo::WORLD;
            }
            if (ImGui::MenuItem("Local Space", nullptr, currentGizmoMode == ImGuizmo::LOCAL)) {
                currentGizmoMode = ImGuizmo::LOCAL;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer"))
        {
            if (ImGui::BeginMenu("Lighting Mode"))
            {
                if (ImGui::MenuItem("Path Traced Lighting", nullptr, currentLightingMode == LightingMode::PathTraced)) {
                    SetLightingMode(LightingMode::PathTraced, appref, vulkanContext);
                }
                if (ImGui::MenuItem("Brute Force Lighting", nullptr, currentLightingMode == LightingMode::BruteForce)) {
                    SetLightingMode(LightingMode::BruteForce, appref, vulkanContext);
                }
                if (ImGui::MenuItem("ReSTIR DI", nullptr, currentLightingMode == LightingMode::ReSTIR)) {
                    SetLightingMode(LightingMode::ReSTIR, appref, vulkanContext);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            ImGui::MenuItem("Wireframe Mode", nullptr, &appref->bWireFrame);
            ImGui::MenuItem("DLSS Ray Reconstruction", nullptr, &appref->bUseDLSS);
            ImGui::MenuItem("FXAA Anti-Aliasing", nullptr, (bool*)&appref->fxaa_FullScreenQuad->bFXAA);
            ImGui::MenuItem("Hide Light Indicators", nullptr, &appref->bHideLights);
            ImGui::EndMenu();
        }

        // Top-Right Quick Stats
        ImGuiIO& io = ImGui::GetIO();
        std::string statsStr = "FPS: " + std::to_string(static_cast<int>(io.Framerate)) + " | " + appref->SceneNames[appref->currentSceneIndex];
        float statsWidth = ImGui::CalcTextSize(statsStr.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - statsWidth - 20.0f);
        ImGui::TextDisabled("%s", statsStr.c_str());

        ImGui::EndMainMenuBar();
    }
}

void UserInterface::DrawViewportToolbar(App* appref, VulkanContext* vulkanContext)
{
    ImVec2 toolbarPos = ImGui::GetCursorScreenPos();
    toolbarPos.x += 12.0f;
    toolbarPos.y += 12.0f;

    ImGui::SetNextWindowPos(toolbarPos);
    ImGui::SetNextWindowBgAlpha(0.75f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin("##ViewportToolbar", nullptr, flags))
    {
        auto DrawToolBtn = [&](const char* label, bool active, auto onClick) {
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.52f, 0.95f, 1.0f));
            }
            if (ImGui::Button(label)) {
                onClick();
            }
            if (active) {
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
        };

        DrawToolBtn("Translate [1]", currentGizmoOperation == ImGuizmo::TRANSLATE, [&]() {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        });
        DrawToolBtn("Rotate [2]", currentGizmoOperation == ImGuizmo::ROTATE, [&]() {
            currentGizmoOperation = ImGuizmo::ROTATE;
        });
        DrawToolBtn("Scale [3]", currentGizmoOperation == ImGuizmo::SCALE, [&]() {
            currentGizmoOperation = ImGuizmo::SCALE;
        });

        ImGui::SameLine(0, 10.0f);
        ImGui::TextUnformatted("|");
        ImGui::SameLine(0, 10.0f);

        DrawToolBtn(currentGizmoMode == ImGuizmo::WORLD ? "World" : "Local", false, [&]() {
            currentGizmoMode = (currentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        });

        ImGui::SameLine(0, 10.0f);
        ImGui::TextUnformatted("| Mode:");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(165.0f);
        if (ImGui::BeginCombo("##LightingModeCombo", currentLightingModeName.c_str()))
        {
            for (size_t i = 0; i < LightingModes.size(); i++)
            {
                bool isSelected = (static_cast<size_t>(currentLightingMode) == i);
                if (ImGui::Selectable(LightingModes[i].c_str(), isSelected))
                {
                    SetLightingMode(static_cast<LightingMode>(i), appref, vulkanContext);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine(0, 10.0f);
        ImGui::TextUnformatted("| Pass:");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(130.0f);
        if (ImGui::BeginCombo("##QuickPass", currentPass.c_str()))
        {
            for (int i = 0; i < Passes.size(); i++)
            {
                bool isSelected = (currentPass == Passes[i]);
                if (ImGui::Selectable(Passes[i].c_str(), isSelected))
                {
                    currentPass = Passes[i];
                    appref->DefferedDecider = i;
                    if (i == 2) {
                        currentLightingMode = LightingMode::ReSTIR;
                        currentLightingModeName = "ReSTIR DI";
                    } else if (i == 3) {
                        if (appref->lighting_RTX->GISolutionIndex == 1) {
                            currentLightingMode = LightingMode::PathTraced;
                            currentLightingModeName = "Path Traced Lighting";
                        } else {
                            currentLightingMode = LightingMode::BruteForce;
                            currentLightingModeName = "Brute Force Lighting";
                        }
                    }
                    appref->DLSS_Intergration.SceneChangeNotifer = 1;
                    vulkanContext->ResetFrameCount();
                }
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();
}

void UserInterface::SetupDockingEnvironment()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar();

	ImGuiID dock_main_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dock_main_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	static bool first_time = true;
    if (first_time)
    {
        first_time = false;

        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.28f, nullptr, &dock_main_id);
        ImGuiID dock_right_bottom_id;
        ImGuiID dock_right_top_id;
        ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.50f, &dock_right_bottom_id, &dock_right_top_id);

        ImGuiID dock_outliner_id;
        ImGuiID dock_details_id;
        ImGui::DockBuilderSplitNode(dock_right_top_id, ImGuiDir_Down, 0.55f, &dock_details_id, &dock_outliner_id);

        ImGui::DockBuilderDockWindow("Main Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Scene Outliner", dock_outliner_id);
        ImGui::DockBuilderDockWindow("Details Panel", dock_details_id);
        ImGui::DockBuilderDockWindow("Settings", dock_right_bottom_id);

        ImGui::DockBuilderDockWindow("DDGI Irradiance Atlas", dock_right_bottom_id);
        ImGui::DockBuilderDockWindow("DDGI Visibility Atlas", dock_right_bottom_id);

        ImGui::DockBuilderFinish(dock_main_id);
    }

	ImGui::End();
}

void UserInterface::RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex, ImageData& DrawingImage)
{
	ImageTransitionData TransitionSwapchainToWriteData;
	TransitionSwapchainToWriteData.oldlayout = vk::ImageLayout::eUndefined;
	TransitionSwapchainToWriteData.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitionSwapchainToWriteData.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionSwapchainToWriteData.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitionSwapchainToWriteData.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionSwapchainToWriteData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitionSwapchainToWriteData.AspectFlag = vk::ImageAspectFlagBits::eColor;

	buffermanager->TransitionImage(CommandBuffer, &DrawingImage, TransitionSwapchainToWriteData);

	ImGui::Render();

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	vk::RenderingAttachmentInfo imguiColorAttachment{};
	imguiColorAttachment.imageView = DrawingImage.imageView;
	imguiColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	imguiColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	imguiColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	imguiColorAttachment.clearValue.color = vk::ClearColorValue();

	vk::RenderingInfoKHR imguiRenderingInfo{};
	imguiRenderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
	imguiRenderingInfo.renderArea.extent = vulkancontext->swapchainExtent;
	imguiRenderingInfo.layerCount = 1;
	imguiRenderingInfo.colorAttachmentCount = 1;
	imguiRenderingInfo.pColorAttachments = &imguiColorAttachment;

	CommandBuffer.beginRendering(imguiRenderingInfo);

	vk::Viewport ImguiViewPort{};
	ImguiViewPort.x = 0.0f;
	ImguiViewPort.y = 0.0f;
	ImguiViewPort.width = static_cast<float>(vulkancontext->swapchainExtent.width);
	ImguiViewPort.height = static_cast<float>(vulkancontext->swapchainExtent.height);
	ImguiViewPort.minDepth = 0.0f;
	ImguiViewPort.maxDepth = 1.0f;

	vk::Offset2D imguiOffset{ 0, 0 };
	vk::Rect2D ImguiScissor{};
	ImguiScissor.offset = imguiOffset;
	ImguiScissor.extent = vulkancontext->swapchainExtent;

	CommandBuffer.setViewport(0, 1, &ImguiViewPort);
	CommandBuffer.setScissor(0, 1, &ImguiScissor);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);
	CommandBuffer.endRendering();
}

void UserInterface::DrawUi(App* appref, SkyBox* skyBox, VulkanContext* vulkanContext)
{
    SetupDockingEnvironment();
    DrawMenuBar(appref, skyBox, vulkanContext);

    // Keyboard Shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_1)) { currentGizmoOperation = ImGuizmo::TRANSLATE; }
    if (ImGui::IsKeyPressed(ImGuiKey_2)) { currentGizmoOperation = ImGuizmo::ROTATE; }
    if (ImGui::IsKeyPressed(ImGuiKey_3)) { currentGizmoOperation = ImGuizmo::SCALE; }
    
#if ENABLE_NVPERF
    if (ImGui::IsKeyPressed(ImGuiKey_P)) { 
        m_periodicSamplerOneShot.StartCollectionOnFrameEnd();
        Save_To_File = true;
    }
#endif

    bool isItemSelected = (UserInterfaceItemsIndex >= 0 && UserInterfaceItemsIndex < appref->UserInterfaceItems.size());
    glm::mat4 modelMatrix;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];

    // ==========================================
    // 1. SCENE OUTLINER
    // ==========================================
    if (showOutliner)
    {
        ImGui::Begin("Scene Outliner", &showOutliner);

        static char searchFilter[64] = "";
        ImGui::InputTextWithHint("##SearchFilter", "Search items...", searchFilter, IM_ARRAYSIZE(searchFilter));
        ImGui::Separator();

        for (int i = 0; i < appref->UserInterfaceItems.size(); ++i)
        {
            if (!appref->UserInterfaceItems[i]) continue;

            Model* model = dynamic_cast<Model*>(appref->UserInterfaceItems[i]);

            if (model)
            {
                std::string nodeName = "Model " + std::to_string(i);
                if (!model->GetFilePath().empty()) {
                    std::string fp = model->GetFilePath();
                    size_t slash = fp.find_last_of("/\\");
                    if (slash != std::string::npos) nodeName = fp.substr(slash + 1);
                }
                if (searchFilter[0] != '\0' && nodeName.find(searchFilter) == std::string::npos) continue;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (UserInterfaceItemsIndex == i && selectedNode == nullptr && SelectedInstanceIndex == -1) flags |= ImGuiTreeNodeFlags_Selected;

                bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)(i * 10000), flags, "[M] %s", nodeName.c_str());

                if (ImGui::IsItemClicked()) {
                    UserInterfaceItemsIndex = i;
                    selectedModel = model;
                    selectedNode = nullptr;
                    SelectedInstanceIndex = -1;
                }

                if (node_open)
                {
                    if (model->Instances.size() > 1)
                    {
                        if (ImGui::TreeNodeEx("Instances", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            for (int j = 0; j < model->Instances.size(); ++j)
                            {
                                if (!model->Instances[j]) continue;

                                bool isInstanceSelected = (UserInterfaceItemsIndex == i && SelectedInstanceIndex == j && selectedNode == nullptr);
                                std::string instName = "Instance " + std::to_string(j);

                                if (ImGui::Selectable(instName.c_str(), isInstanceSelected))
                                {
                                    UserInterfaceItemsIndex = i;
                                    selectedModel = model;
                                    SelectedInstanceIndex = j;
                                    selectedNode = nullptr;
                                }
                            }
                            ImGui::TreePop();
                        }
                    }

                    if (model->storedModelData)
                    {
                        if (ImGui::TreeNodeEx("Objects / Nodes", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            for (const auto& rootNode : model->storedModelData->nodes)
                            {
                                DrawNodeTree(rootNode, model, searchFilter, vulkanContext, appref);
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
            }
            else
            {
                Light* light = dynamic_cast<Light*>(appref->UserInterfaceItems[i]);
                std::string name = "Item " + std::to_string(i);
                if (light) {
                    name = (light->lightType == 0 ? "[Dir Light] Light " : "[Point Light] Light ") + std::to_string(i);
                }

                if (searchFilter[0] != '\0' && name.find(searchFilter) == std::string::npos) continue;

                bool isSelected = (UserInterfaceItemsIndex == i && SelectedInstanceIndex == -1 && selectedNode == nullptr);
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    UserInterfaceItemsIndex = i;
                    selectedModel = nullptr;
                    selectedNode = nullptr;
                    SelectedInstanceIndex = -1;
                }
            }
        }
        ImGui::End();
    }

    // ==========================================
    // 2. SETTINGS PANEL
    // ==========================================
    if (showSettings)
    {
        ImGui::Begin("Settings", &showSettings);

        if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None))
        {
            // --- TAB: General & Scene ---
            if (ImGui::BeginTabItem("Environment"))
            {
                ImGui::SeparatorText("Scene Selection");

                if (ImGui::BeginCombo("Active Scene", appref->SceneNames[appref->currentSceneIndex].c_str()))
                {
                    for (int i = 0; i < appref->SceneNames.size(); i++)
                    {
                        bool is_selected = (appref->currentSceneIndex == i);
                        if (ImGui::Selectable(appref->SceneNames[i].c_str(), is_selected))
                        {
                            if (appref->currentSceneIndex != i) {
                                appref->SwitchScene(i);
                                appref->DLSS_Intergration.SceneChangeNotifer = 1;
                            }
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("SkyBox Environment", currentSkyBox.c_str())) {
                    for (int i = 0; i < SkyBoxs.size(); i++) {
                        bool is_selected = (currentSkyBox == SkyBoxs[i]);
                        if (ImGui::Selectable(SkyBoxs[i].c_str(), is_selected)) {
                            currentSkyBox = SkyBoxs[i];
                            skyBox->SkyBoxIndex = i;
                            vulkanContext->ResetFrameCount();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Pipeline Actions");
                if (ImGui::Button("Reload Shaders (F5)", ImVec2(-1, 28))) {
                    appref->recreatePipeline();
                }

                ImGui::EndTabItem();
            }

            // --- TAB: Rendering & Post Processing ---
            if (ImGui::BeginTabItem("Post-Processing"))
            {
                ImGui::SeparatorText("Anti-Aliasing & Upscaling");
                ImGui::Checkbox("FXAA Enabled", (bool*)&appref->fxaa_FullScreenQuad->bFXAA);

                if (ImGui::Checkbox("NVIDIA DLSS (Ray Reconstruction)", (bool*)&appref->bUseDLSS))
                {
                    appref->UpdateTextureID();
                }

                ImGui::SeparatorText("Color Grading");
                ImGui::SliderFloat("Brightness", &appref->Combined_FullScreenQuad->Brightness, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Saturation", &appref->Combined_FullScreenQuad->Saturation, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Contrast", &appref->Combined_FullScreenQuad->Concentration, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Max Gamma", &appref->Combined_FullScreenQuad->MaxGamma, 0.1f, 4.0f, "%.2f");
                ImGui::SliderFloat("Min Gamma", &appref->Combined_FullScreenQuad->MinGamma, 0.1f, 4.0f, "%.2f");
                ImGui::SliderFloat("GI Boost", &appref->Combined_FullScreenQuad->GIBoost, 0.1f, 10.0f, "%.2f");

                ImGui::EndTabItem();
            }

            // --- TAB: Global Illumination & DDGI ---
            if (ImGui::BeginTabItem("Lighting & GI"))
            {
                ImGui::SeparatorText("Primary Lighting Mode");
                int currentModeInt = static_cast<int>(currentLightingMode);
                bool modeChanged = false;
                if (ImGui::RadioButton("Path Traced Lighting", &currentModeInt, 0)) modeChanged = true;
                ImGui::SameLine(0, 15.0f);
                if (ImGui::RadioButton("Brute Force Lighting", &currentModeInt, 1)) modeChanged = true;
                ImGui::SameLine(0, 15.0f);
                if (ImGui::RadioButton("ReSTIR DI", &currentModeInt, 2)) modeChanged = true;

                if (modeChanged) {
                    SetLightingMode(static_cast<LightingMode>(currentModeInt), appref, vulkanContext);
                }

                if (currentLightingMode == LightingMode::PathTraced)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.75f, 1.0f, 1.0f));
                    ImGui::TextUnformatted("Active: Multi-bounce Path Tracing GI with Progressive Temporal Accumulation.");
                    ImGui::PopStyleColor();
                    ImGui::TextDisabled("Accumulated Frames: %d", vulkanContext->frameIndex);
                }
                else if (currentLightingMode == LightingMode::BruteForce)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.90f, 0.55f, 1.0f));
                    ImGui::TextUnformatted("Active: Direct Ray-Traced Lighting with Dynamic Diffuse GI (DDGI).");
                    ImGui::PopStyleColor();
                }
                else if (currentLightingMode == LightingMode::ReSTIR)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.70f, 0.35f, 1.0f));
                    ImGui::TextUnformatted("Active: Spatio-Temporal Reservoir Sampling Direct Illumination (ReSTIR DI).");
                    ImGui::PopStyleColor();
                }

                ImGui::Spacing();
                ImGui::SeparatorText("Scene Lights");
                if (ImGui::SliderInt("Total Lights", &NumberOfLights, 0, MAX_LIGHT_COUNT))
                {
                    appref->SpawnLights(NumberOfLights);
                }
                ImGui::Checkbox("Hide Light Indicators", (bool*)&appref->bHideLights);

                ImGui::SeparatorText("Global Illumination Solution");
                if (ImGui::BeginCombo("GI Method", currentGI_Solution.c_str())) {
                    for (int i = 0; i < GlobalIllumination_Solution.size(); i++) {
                        bool is_selected = (currentGI_Solution == GlobalIllumination_Solution[i]);
                        if (ImGui::Selectable(GlobalIllumination_Solution[i].c_str(), is_selected)) {
                            currentGI_Solution = GlobalIllumination_Solution[i];
                            appref->lighting_RTX->GISolutionIndex = i;
                            if (i == 1) {
                                currentLightingMode = LightingMode::PathTraced;
                                currentLightingModeName = "Path Traced Lighting";
                                appref->DefferedDecider = 3;
                            } else if (i == 0) {
                                currentLightingMode = LightingMode::BruteForce;
                                currentLightingModeName = "Brute Force Lighting";
                                appref->DefferedDecider = 3;
                            }
                            vulkanContext->ResetFrameCount();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::CollapsingHeader("Dynamic Diffuse GI (DDGI)", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Debug Probes", (bool*)&appref->dynamicDiffuse_RTGI->DrawDEBUG_Probes);
                    if (appref->dynamicDiffuse_RTGI->DrawDEBUG_Probes) {
                        ImGui::SameLine();
                        ImGui::Checkbox("Show Status", (bool*)&appref->dynamicDiffuse_RTGI->ShowDEBUG_Status);
                    }

                    ImGui::Checkbox("Infinite Bounces", (bool*)&appref->dynamicDiffuse_RTGI->UseinfiniteBounce);
                    ImGui::SliderFloat("Bounce Weight", &appref->dynamicDiffuse_RTGI->infiniteBounceMultiplyer, 0.0f, 1.0f, "%.2f");
                    ImGui::SliderInt("Rays / Probe", &appref->dynamicDiffuse_RTGI->RaysPerProbe, 8, 256);

                    ImGui::Spacing();
                    ImGui::TextDisabled("Probe Grid Dimensions:");
                    ImGui::SliderInt("X Probes", &appref->dynamicDiffuse_RTGI->NumOfProbesX, 1, 22);
                    ImGui::SliderInt("Y Probes", &appref->dynamicDiffuse_RTGI->NumOfProbesY, 1, 22);
                    ImGui::SliderInt("Z Probes", &appref->dynamicDiffuse_RTGI->NumOfProbesZ, 1, 22);

                    DrawVec3Control("Grid Origin", appref->dynamicDiffuse_RTGI->GridLocation);
                    DrawVec3Control("Probe Spacing", appref->dynamicDiffuse_RTGI->ProbeOffset, 3.0f);
                }

                if (ImGui::CollapsingHeader("ReSTIR Direct Illumination"))
                {
                    ImGui::Checkbox("Temporal Reservoir Reuse", (bool*)&appref->Restir_DI->bTemporalReuse);
                    ImGui::Checkbox("Spatial Reservoir Reuse", (bool*)&appref->Restir_DI->bSpatialReuse);
                    ImGui::Checkbox("ReSTIR DDGI Injection", (bool*)&appref->Restir_DI->bDDGI);
                }

                if (ImGui::CollapsingHeader("Ambient Occlusion (SSAO)"))
                {
                    ImGui::Checkbox("Enable SSAO", (bool*)&appref->ssao_FullScreenQuad->bShouldSSAO);
                    ImGui::SliderInt("Kernel Size", &appref->ssao_FullScreenQuad->KernelSize, 8, 64);
                    ImGui::SliderFloat("Radius", &appref->ssao_FullScreenQuad->Radius, 0.1f, 3.0f, "%.2f");
                    ImGui::SliderFloat("Bias", &appref->ssao_FullScreenQuad->Bias, 0.001f, 0.2f, "%.3f");
                }

                ImGui::EndTabItem();
            }

#if ENABLE_NVPERF
            // --- TAB: Performance Profiler ---
            if (ImGui::BeginTabItem("Profiler"))
            {
                ImGui::SeparatorText("NVIDIA Nsigt Perf");
                if (ImGui::Button("Capture Frame Traces (P)", ImVec2(-1, 30))) {
                    m_periodicSamplerOneShot.StartCollectionOnFrameEnd();
                    Save_To_File = true;
                }
                ImGui::TextDisabled("Output: traces.yaml in working directory");
                ImGui::EndTabItem();
            }
#endif

            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    // ==========================================
    // 3. MAIN VIEWPORT
    // ==========================================
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Main Viewport");
        ImGui::PopStyleVar();

        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        viewportSize = ImGui::GetContentRegionAvail();

        switch (appref->DefferedDecider) {
            case 0: ImGui::Image((ImTextureID)appref->SSGITextureId, viewportSize); break;
            case 1: ImGui::Image((ImTextureID)appref->Sampled_GI_ID, viewportSize); break;
            case 2: ImGui::Image((ImTextureID)appref->ReSTIR_DITextureId, viewportSize); break;
            case 3: ImGui::Image((ImTextureID)appref->FinalRenderTextureId, viewportSize); break;
        }

        // Viewport Overlay Toolbar
        DrawViewportToolbar(appref, vulkanContext);

        // Performance Badge Overlay
        if (showPerformanceOverlay)
        {
            ImVec2 badgePos = imageTopLeft;
            badgePos.x += viewportSize.x - 180.0f;
            badgePos.y += 12.0f;

            ImGui::SetNextWindowPos(badgePos);
            ImGui::SetNextWindowBgAlpha(0.65f);

            ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

            if (ImGui::Begin("##PerfBadge", nullptr, overlayFlags))
            {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "%.1f FPS", io.Framerate);
                ImGui::Text("%.2f ms", 1000.0f / io.Framerate);
                ImGui::TextDisabled("%dx%d", (int)viewportSize.x, (int)viewportSize.y);
            }
            ImGui::End();
        }

        ImGuizmo::SetRect(imageTopLeft.x, imageTopLeft.y, viewportSize.x, viewportSize.y);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        if (!appref->UserInterfaceItems.empty())
        {
            glm::mat4 cameraprojection = appref->camera.GetProjectionMatrix();
            glm::mat4 cameraview = appref->camera.GetViewMatrix();

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver()) {
                SelectedInstanceIndex = -1;
                UserInterfaceItemsIndex = -1;

                for (auto& item : appref->UserInterfaceItems) {
                    Model* model = dynamic_cast<Model*>(item);
                    if (model) {
                        for (size_t i = 0; i < model->Instances.size(); i++) {
                            if (!model->Instances[i]) continue;
                            glm::vec3 ModelInstancePosition = model->Instances[i]->GetPostion();
                            float distance = CalculateDistanceInScreenSpace(cameraprojection, cameraview, ModelInstancePosition);

                            if (distance < 100) {
                                SelectedInstanceIndex = i;
                                UserInterfaceItemsIndex = &item - &appref->UserInterfaceItems[0];
                                break;
                            }
                        }
                        if (SelectedInstanceIndex != -1) break;
                    }
                }

                if (SelectedInstanceIndex == -1) {
                    for (size_t i = 0; i < appref->UserInterfaceItems.size(); i++) {
                        if (!appref->UserInterfaceItems[i]) continue;
                        glm::vec3 itemPosition = appref->UserInterfaceItems[i]->position;
                        float distance = CalculateDistanceInScreenSpace(cameraprojection, cameraview, itemPosition);
                        if (distance < 100.0f) {
                            UserInterfaceItemsIndex = i;
                            break;
                        }
                    }
                }
            }

            isItemSelected = (UserInterfaceItemsIndex >= 0 && UserInterfaceItemsIndex < appref->UserInterfaceItems.size());
            if (selectedNode != nullptr && selectedModel != nullptr)
            {
                glm::mat4 modelTransform = selectedModel->Instances[0]->GetTransformationMatrix();
                modelMatrix = selectedNode->GetWorldMatrix(modelTransform);

                ImGuizmo::Manipulate(glm::value_ptr(cameraview), glm::value_ptr(cameraprojection),
                                     currentGizmoOperation, currentGizmoMode, glm::value_ptr(modelMatrix));

                if (ImGuizmo::IsUsing()) {
                    glm::mat4 parentWorld = selectedNode->GetParentWorldMatrix(modelTransform);
                    glm::mat4 localMatrix = glm::inverse(parentWorld) * modelMatrix;
                    selectedNode->SetLocalMatrix(localMatrix);
                    vulkanContext->ResetFrameCount();
                    appref->DLSS_Intergration.SceneChangeNotifer = 1;
                    appref->dynamicDiffuse_RTGI->RESET_PROBE_STATUS = 1;
                }
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix), matrixTranslation, matrixRotation, matrixScale);
            }
            else if (isItemSelected) {
                auto& item = appref->UserInterfaceItems[UserInterfaceItemsIndex];
                if (item)
                {
                    if (SelectedInstanceIndex != -1) {
                        Model* model = dynamic_cast<Model*>(item);
                        if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex]) {
                            modelMatrix = model->Instances[SelectedInstanceIndex]->GetTransformationMatrix();
                        }
                    }
                    else {
                        modelMatrix = item->GetModelMatrix();
                    }

                    ImGuizmo::Manipulate(glm::value_ptr(cameraview), glm::value_ptr(cameraprojection),
                                         currentGizmoOperation, currentGizmoMode, glm::value_ptr(modelMatrix));

                    if (ImGuizmo::IsUsing()) {
                        if (SelectedInstanceIndex != -1) {
                            Model* model = dynamic_cast<Model*>(item);
                            if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex]) {
                                model->Instances[SelectedInstanceIndex]->SetTrasnformationMatrix(modelMatrix);
                                vulkanContext->ResetFrameCount();
                                appref->DLSS_Intergration.SceneChangeNotifer = 1;
                                appref->dynamicDiffuse_RTGI->RESET_PROBE_STATUS = 1;
                            }
                        }
                        else {
                            item->SetModelMatrix(modelMatrix);
                            vulkanContext->ResetFrameCount();
                            appref->DLSS_Intergration.SceneChangeNotifer = 1;
                            appref->dynamicDiffuse_RTGI->RESET_PROBE_STATUS = 1;
                        }
                    }
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix), matrixTranslation, matrixRotation, matrixScale);
                }
                else {
                    isItemSelected = false;
                }
            }
        }
        ImGui::End();
    }

    // ==========================================
    // 4. DETAILS PANEL
    // ==========================================
    if (showDetails)
    {
        ImGui::Begin("Details Panel", &showDetails);

        if (selectedNode != nullptr && selectedModel != nullptr)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.75f, 1.0f, 1.0f));
            ImGui::TextUnformatted("Selected glTF Object");
            ImGui::PopStyleColor();
            ImGui::Text("Node: %s (ID: %u)", selectedNode->name.c_str(), selectedNode->id);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool changed = false;
                if (DrawVec3Control("Position", selectedNode->translation)) changed = true;
                if (DrawVec3Control("Rotation", selectedNode->rotation)) changed = true;
                if (DrawVec3Control("Scale", selectedNode->scale, 1.0f)) changed = true;

                if (changed) {
                    selectedNode->UpdateLocalMatrix();
                    vulkanContext->ResetFrameCount();
                    appref->DLSS_Intergration.SceneChangeNotifer = 1;
                    appref->dynamicDiffuse_RTGI->RESET_PROBE_STATUS = 1;
                }

                ImGui::Spacing();
                if (ImGui::Button("Reset to glTF Default", ImVec2(-1, 26))) {
                    selectedNode->ResetTransform();
                    vulkanContext->ResetFrameCount();
                    appref->DLSS_Intergration.SceneChangeNotifer = 1;
                    appref->dynamicDiffuse_RTGI->RESET_PROBE_STATUS = 1;
                }
            }

            if (!selectedNode->meshPrimitives.empty())
            {
                if (ImGui::CollapsingHeader("Mesh Primitives", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("Primitives: %zu", selectedNode->meshPrimitives.size());
                    for (size_t p = 0; p < selectedNode->meshPrimitives.size(); p++)
                    {
                        const auto& prim = selectedNode->meshPrimitives[p];
                        ImGui::BulletText("Prim %zu: %u indices, Material %u", p, prim.numIndices, prim.materialIndex);
                    }
                }
            }
        }
        else if (!isItemSelected)
        {
            ImGui::TextDisabled("Select an object or node in the Outliner or Viewport.");
        }
        else
        {
            auto& item = appref->UserInterfaceItems[UserInterfaceItemsIndex];

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (SelectedInstanceIndex != -1)
                {
                    Model* model = dynamic_cast<Model*>(item);
                    if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex])
                    {
                        glm::vec3 pos = model->Instances[SelectedInstanceIndex]->GetPostion();
                        glm::vec3 rot = model->Instances[SelectedInstanceIndex]->GetRotation();
                        glm::vec3 scl = model->Instances[SelectedInstanceIndex]->GetScale();

                        if (DrawVec3Control("Position", pos)) model->Instances[SelectedInstanceIndex]->SetPostion(pos);
                        if (DrawVec3Control("Rotation", rot)) model->Instances[SelectedInstanceIndex]->SetRotation(rot);
                        if (DrawVec3Control("Scale", scl, 1.0f)) model->Instances[SelectedInstanceIndex]->SetScale(scl);
                    }
                }
                else
                {
                    glm::vec3 pos = { matrixTranslation[0], matrixTranslation[1], matrixTranslation[2] };
                    glm::vec3 rot = { matrixRotation[0], matrixRotation[1], matrixRotation[2] };
                    glm::vec3 scl = { matrixScale[0], matrixScale[1], matrixScale[2] };

                    bool changed = false;
                    if (DrawVec3Control("Position", pos)) { matrixTranslation[0] = pos.x; matrixTranslation[1] = pos.y; matrixTranslation[2] = pos.z; changed = true; }
                    if (DrawVec3Control("Rotation", rot)) { matrixRotation[0] = rot.x; matrixRotation[1] = rot.y; matrixRotation[2] = rot.z; changed = true; }
                    if (DrawVec3Control("Scale", scl, 1.0f)) { matrixScale[0] = scl.x; matrixScale[1] = scl.y; matrixScale[2] = scl.z; changed = true; }

                    if (changed)
                    {
                        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(modelMatrix));
                        item->SetModelMatrix(modelMatrix);
                        LastModelMatrix = modelMatrix;
                        vulkanContext->ResetFrameCount();
                        appref->DLSS_Intergration.SceneChangeNotifer = 1;
                        appref->dynamicDiffuse_RTGI->RESET_PROBE_STATUS = 1;
                    }
                }
            }

            Light* light = dynamic_cast<Light*>(item);
            if (light && SelectedInstanceIndex == -1)
            {
                if (ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::ColorEdit3("Color", glm::value_ptr(light->color));
                    ImGui::DragFloat("Intensity", &light->lightIntensity, 0.5f, 0.0f, 100.0f, "%.1f");
                    ImGui::DragFloat("Ambience", &light->ambientStrength, 0.01f, 0.0f, 1.0f, "%.2f");
                    ImGui::Checkbox("Cast Shadow", (bool*)&light->CastShadow);

                    if (ImGui::BeginCombo("Light Type", items[light->lightType].c_str())) {
                        for (int i = 0; i < items.size(); i++) {
                            bool is_selected = (currentItem == items[i]);
                            if (ImGui::Selectable(items[i].c_str(), is_selected)) {
                                currentItem = items[i];
                                light->lightType = i;
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            }
        }
        ImGui::End();
    }

    // ==========================================
    // 5. DDGI ATLAS DEBUG VIEWERS
    // ==========================================
    if (showDDGIAtlas)
    {
        ImGui::Begin("DDGI Visibility Atlas", &showDDGIAtlas);
        ImVec2 atlasSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)appref->DDGIIVisibilityAtlasID, atlasSize);
        ImGui::End();

        ImGui::Begin("DDGI Irradiance Atlas", &showDDGIAtlas);
        atlasSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)appref->DDGIIrradianceAtlasID, atlasSize);
        ImGui::End();
    }
}

float UserInterface::CalculateDistanceInScreenSpace(glm::mat4 CameraProjection, glm::mat4 cameraview, glm::vec3 position)
{
	float windowWidth = (float)ImGui::GetWindowWidth();
	float windowHeight = (float)ImGui::GetWindowHeight();

	ImVec2 viewportPos = ImGui::GetWindowPos();
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, windowWidth, windowHeight);

	ImVec2 mousePos = ImGui::GetMousePos();
	mousePos.x -= viewportPos.x;
	mousePos.y -= viewportPos.y;

	glm::vec4 clipSpacePos = CameraProjection * cameraview * glm::vec4(position, 1.0f);
	glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

	glm::vec2 screenSpacePos;
	screenSpacePos.x = (ndcSpacePos.x + 1.0f) * 0.5f * windowWidth;
	screenSpacePos.y = (1.0f - ndcSpacePos.y) * 0.5f * windowHeight;

	float distance = glm::distance(glm::vec2(mousePos.x, mousePos.y), screenSpacePos);
	return distance;
}

vk::Extent3D UserInterface::GetRenderTextureExtent()
{
	return RenderTextureExtent;
}

void UserInterface::CleanUp()
{
	vulkancontext->LogicalDevice.waitIdle();
#if ENABLE_NVPERF
    m_periodicSamplerOneShot.Reset();
#endif

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
	ImGui::DestroyContext();
	vulkancontext->LogicalDevice.destroyDescriptorPool(ImGuiDescriptorPool);
}

UserInterface::~UserInterface()
{
	CleanUp();
}