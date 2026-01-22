#include "UserInterface.h"
#include <stdexcept>
#include "Window.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"
#include "SSAO_FullScreenQuad.h"
#include "App.h"
#include "FXAA_FullScreenQuad.h"
#include "CombinedResult_FullScreenQuad.h"

UserInterface::UserInterface(VulkanContext* vulkancontextRef, Window* WindowRef, BufferManager* Buffermanager)
{
	vulkancontext = vulkancontextRef;
	window = WindowRef;
	buffermanager = Buffermanager;
	currentGizmoOperation = ImGuizmo::TRANSLATE;
	currentGizmoMode = ImGuizmo::WORLD;
	InitImgui();
}


void UserInterface::InitImgui()
{
	//Imgui Initialisation
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
	///////////////////////////////////////////////////////
	//Imgui Style Setup
	ImGui::StyleColorsDark();


	if (io.ConfigFlags)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 10.0f;

		ImVec4* colors = style.Colors;

		colors[ImGuiCol_WindowBg] = ImVec4{ 0.01f, 0.01f, 0.01f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.00f, 0.00f, 0.00f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.03f, 0.03f, 0.03f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.00f, 0.00f, 0.00f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.03f, 0.03f, 0.03f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.00f, 0.00f, 0.00f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.03f, 0.03f, 0.03f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.07f, 0.07f, 0.07f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.03f, 0.03f, 0.03f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.01f, 0.01f, 0.01f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.01f, 0.01, 0.01f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.00f, 0.00f, 0.00f, 1.0f };

		// Borders
		colors[ImGuiCol_Border] = ImVec4{ 0.08f, 0.08f, 0.08f, 1.0f };
		colors[ImGuiCol_BorderShadow] = ImVec4{ 0.00f, 0.00f, 0.00f, 0.0f };

		// Scrollbars
		colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.01f, 0.01f, 0.01f, 1.0f };
		colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.10f, 0.10f, 0.10f, 1.0f };
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.20f, 0.20f, 0.20f, 1.0f };

		colors[ImGuiCol_CheckMark] = ImVec4{ 0.80f, 0.80f, 0.80f, 1.0f };
		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.50f, 0.50f, 0.50f, 1.0f };

		colors[ImGuiCol_Separator] = ImVec4{ 0.20f, 0.20f, 0.20f, 1.0f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.50f, 0.50f, 0.50f, 1.0f };

		colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f };
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.40f, 0.40f, 0.40f, 1.0f };
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.60f, 0.60f, 0.60f, 1.0f };


		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };

		// Popup
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.05f, 0.05f, 0.05f, 0.94f };


		// Style adjustments
		//style.WindowRounding = 5.3f;
		style.FrameRounding = 2.3f;
		style.ScrollbarRounding = 0;

		style.WindowTitleAlign = ImVec2(0.50f, 0.50f);
		style.WindowPadding = ImVec2(8.0f, 8.0f);
		style.FramePadding = ImVec2(5.0f, 5.0f);
		style.ItemSpacing = ImVec2(6.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
		style.IndentSpacing = 25.0f;
	}
	/////////////////////////////////////////////////////

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
	// Initialize ImGui for Vulkan
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

void UserInterface::SetupDockingEnvironment()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	// Get the main viewport
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Set up the main dockspace window
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	// Set up the window flags
	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;


	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(-0.5f, -0.5f));
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar();

	// Submit the DockSpace
	ImGuiID dock_main_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dock_main_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// Set up the initial layout (only once)
	static bool first_time = true;

    if (first_time)
    {
        first_time = false;

        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

        ImGuiID dock_Settings_id;
        ImGuiID dock_right_temp_id;

        ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.53f, &dock_Settings_id, &dock_right_temp_id);

        ImGuiID dock_DetailsPanel_id;
        ImGuiID dock_OutlinerPanel_id;
        ImGui::DockBuilderSplitNode(dock_right_temp_id, ImGuiDir_Down, 0.6f, &dock_DetailsPanel_id, &dock_OutlinerPanel_id);

        ImGui::DockBuilderDockWindow("Main Viewport", dock_main_id);

        ImGui::DockBuilderDockWindow("Settings", dock_Settings_id);
        ImGui::DockBuilderDockWindow("Details Panel", dock_DetailsPanel_id);

        ImGui::DockBuilderDockWindow("DDGI Visibility Atlas", dock_OutlinerPanel_id);
        ImGui::DockBuilderDockWindow("DDGI Irradiance Atlas", dock_OutlinerPanel_id);
        ImGui::DockBuilderDockWindow("Scene Outliner", dock_OutlinerPanel_id);

        ImGui::DockBuilderFinish(dock_main_id);
    }

	ImGui::End();
}

void UserInterface::RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex,ImageData& DrawingImage)
{

	ImageTransitionData TransitionSwapchainToWriteData;
	TransitionSwapchainToWriteData.oldlayout = vk::ImageLayout::eUndefined;
	TransitionSwapchainToWriteData.newlayout = vk::ImageLayout::eGeneral;
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
	//// Begin rendering for ImGui
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

	vk::Offset2D imguiOffset{};
	imguiOffset.x = 0;
	imguiOffset.y = 0;

	vk::Rect2D ImguiScissor{};
	ImguiScissor.offset = imguiOffset;
	ImguiScissor.extent = vulkancontext->swapchainExtent;

	CommandBuffer.setViewport(0, 1, &ImguiViewPort);
	CommandBuffer.setScissor(0, 1, &ImguiScissor);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);
	CommandBuffer.endRendering();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void UserInterface::DrawUi(App* appref, SkyBox* skyBox)
{
    SetupDockingEnvironment();

    // Handle gizmo mode changes
    if (ImGui::IsKeyPressed(ImGuiKey_1)) { currentGizmoOperation = ImGuizmo::TRANSLATE; }
    if (ImGui::IsKeyPressed(ImGuiKey_2)) { currentGizmoOperation = ImGuizmo::ROTATE; }
    if (ImGui::IsKeyPressed(ImGuiKey_3)) { currentGizmoOperation = ImGuizmo::SCALE; }

    bool isItemSelected = (UserInterfaceItemsIndex >= 0 && UserInterfaceItemsIndex < appref->UserInterfaceItems.size());
    glm::mat4 modelMatrix;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];

    {
        ImGui::Begin("Scene Outliner");

        for (int i = 0; i < appref->UserInterfaceItems.size(); ++i)
        {
            if (!appref->UserInterfaceItems[i]) continue;

            // Try to cast to Model
            Model* model = dynamic_cast<Model*>(appref->UserInterfaceItems[i]);

            if (model)
            {
                // Model with instances
                bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)i,
                    ImGuiTreeNodeFlags_OpenOnArrow | (UserInterfaceItemsIndex == i ? ImGuiTreeNodeFlags_Selected : 0),
                    "Model %d", i);

                if (ImGui::IsItemClicked()) {
                    UserInterfaceItemsIndex = i;
                    SelectedInstanceIndex = -1;
                }

                if (node_open)
                {
                    for (int j = 0; j < model->Instances.size(); ++j)
                    {
                        if (!model->Instances[j]) continue;

                        bool isInstanceSelected = (UserInterfaceItemsIndex == i && SelectedInstanceIndex == j);
                        if (ImGui::Selectable(("Instance " + std::to_string(j)).c_str(), isInstanceSelected))
                        {
                            UserInterfaceItemsIndex = i;
                            SelectedInstanceIndex = j;
                        }
                    }

                    ImGui::TreePop();
                }
            }
            else
            {

                Light* light = dynamic_cast<Light*>(appref->UserInterfaceItems[i]);

                std::string name = "Unknown Item " + std::to_string(i);

                if (light) {
                    name = (light->lightType == 0 ? "Directional Light " : "Point Light ") + std::to_string(i);
                }

                bool isItemSelected = (UserInterfaceItemsIndex == i && SelectedInstanceIndex == -1);

                if (ImGui::Selectable(name.c_str(), isItemSelected))
                {
                    UserInterfaceItemsIndex = i;
                    SelectedInstanceIndex = -1;
                }
            }
        }
        ImGui::End();
    }


    {
        ImGui::Begin("Settings");

        if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("General"))
            {
                ImGui::SeparatorText("Application");

                if (ImGui::BeginCombo("Select Scene", appref->SceneNames[appref->currentSceneIndex].c_str()))
                {
                    for (int i = 0; i < appref->SceneNames.size(); i++)
                    {
                        bool is_selected = (appref->currentSceneIndex == i);
                        if (ImGui::Selectable(appref->SceneNames[i].c_str(), is_selected))
                        {
                            if (appref->currentSceneIndex != i) {
                                 appref->SwitchScene(i);
                            }
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("Refresh Shaders", ImVec2(100, 30))) {

                    appref->recreatePipeline();

                }

                ImGuiIO& io = ImGui::GetIO();
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

                ImGui::SeparatorText("Scene");

                if (ImGui::BeginCombo("Render Passes", currentPass.c_str())) {

                    for (int i = 0; i < Passes.size(); i++) {

                        bool is_selected = (currentPass == Passes[i]);

                        if (ImGui::Selectable(Passes[i].c_str(), is_selected)) {

                            currentPass = Passes[i];
                            appref->DefferedDecider = i;

                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("SkyBox", currentSkyBox.c_str())) {

                    for (int i = 0; i < SkyBoxs.size(); i++) {

                        bool is_selected = (currentSkyBox == SkyBoxs[i]);

                        if (ImGui::Selectable(SkyBoxs[i].c_str(), is_selected)) {

                            currentSkyBox = SkyBoxs[i];
                            skyBox->SkyBoxIndex = i;

                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Rendering"))
            {
                ImGui::SeparatorText("General");
                ImGui::Checkbox("Wire Frame", &appref->bWireFrame);

                ImGui::SeparatorText("Post-Processing");
                ImGui::Checkbox("FXAA", (bool*)&appref->fxaa_FullScreenQuad->bFXAA);

                if (ImGui::CollapsingHeader("Color Settings", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::SliderFloat("Brightness", &appref->Combined_FullScreenQuad->Brightness, 0.0f, 2.0f, "%.2f");
                    ImGui::SliderFloat("Saturation", &appref->Combined_FullScreenQuad->Saturation, 0.0f, 2.0f, "%.2f");
                    ImGui::SliderFloat("Concentration", &appref->Combined_FullScreenQuad->Concentration, 0.0f, 2.0f, "%.2f");
                    ImGui::SliderFloat("Max Gamma", &appref->Combined_FullScreenQuad->MaxGamma, 0.1f, 4.0f, "%.2f");
                    ImGui::SliderFloat("Min Gamma", &appref->Combined_FullScreenQuad->MinGamma, 0.1f, 4.0f, "%.2f");
                    ImGui::SliderFloat("GI Boost", &appref->Combined_FullScreenQuad->GIBoost, 0.1f, 10.0f, "%.2f");
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Lighting"))
            {
                if (ImGui::CollapsingHeader("Ambient Occlusion (SSAO)", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Enable SSAO", (bool*)&appref->ssao_FullScreenQuad->bShouldSSAO);
                    ImGui::InputInt("Kernel Size", &appref->ssao_FullScreenQuad->KernelSize);
                    ImGui::InputFloat("Radius", &appref->ssao_FullScreenQuad->Radius);
                    ImGui::InputFloat("Bias", &appref->ssao_FullScreenQuad->Bias);
                }

                if (ImGui::CollapsingHeader("Global Illumination (DDGI)", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextDisabled("Adjust probe parameters:");
                    ImGui::Checkbox("Draw Debug Probes", (bool*)&appref->dynamicDiffuse_RTGI->DrawDEBUG_Probes);

                    if (appref->dynamicDiffuse_RTGI->DrawDEBUG_Probes) {
                        ImGui::Checkbox("Show Debug Status", (bool*)&appref->dynamicDiffuse_RTGI->ShowDEBUG_Status);
                    }

                    ImGui::Checkbox("Use Infinite Bounces", (bool*)&appref->dynamicDiffuse_RTGI->UseinfiniteBounce);
                    ImGui::SliderFloat("Bounce Multiplier", &appref->dynamicDiffuse_RTGI->infiniteBounceMultiplyer, 0, 1, "%.1f");
                    ImGui::SliderInt("Rays Per Probe", &appref->dynamicDiffuse_RTGI->RaysPerProbe, 1, 300, "%d");
                    ImGui::SliderInt("Sample Count", &appref->dynamicDiffuse_RTGI->SampleCount, 0, 10, "%d");


                    ImGui::SeparatorText("Probe Grid");
                    ImGui::SliderInt("X Count", &appref->dynamicDiffuse_RTGI->NumOfProbesX, 1, 22, "%d");
                    ImGui::SliderInt("Y Count", &appref->dynamicDiffuse_RTGI->NumOfProbesY, 1, 22, "%d");
                    ImGui::SliderInt("Z Count", &appref->dynamicDiffuse_RTGI->NumOfProbesZ, 1, 22, "%d");
                    ImGui::SliderFloat3("Grid Location", glm::value_ptr(appref->dynamicDiffuse_RTGI->GridLocation), -1000, 1000, "%.0005f");
                    ImGui::SliderFloat3("Probe Offset", glm::value_ptr(appref->dynamicDiffuse_RTGI->ProbeOffset), -300, 300, "%.0005f");
                    ImGui::InputFloat3("Grid Location Input", glm::value_ptr(appref->dynamicDiffuse_RTGI->GridLocation));
                    ImGui::InputFloat3("Probe Offset Input", glm::value_ptr(appref->dynamicDiffuse_RTGI->ProbeOffset));

                }

                if (ImGui::CollapsingHeader("ReSTIR DI", ImGuiTreeNodeFlags_DefaultOpen)){
                    ImGui::Checkbox("Enable ReSTIR Temporal Reuse", (bool*)&appref->Restir_DI->bTemporalReuse);
                    ImGui::Checkbox("Enable ReSTIR Spatial  Reuse", (bool*)&appref->Restir_DI->bSpatialReuse);
                    ImGui::Checkbox("Enable ReSTIR DDGI"          , (bool*)&appref->Restir_DI->bDDGI);
                }

                if (ImGui::BeginCombo("DDGI Sampling Vertex", currentDDGIVertex.c_str())) {

                    for (int i = 0; i < DDGI_Vertex_Options.size(); i++) {

                        bool is_selected = (currentDDGIVertex == DDGI_Vertex_Options[i]);

                        if (ImGui::Selectable(DDGI_Vertex_Options[i].c_str(), is_selected)) {

                            currentDDGIVertex = DDGI_Vertex_Options[i];
                            appref->Restir_DI->DDGIVertex = i;

                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("GI Solution", currentGI_Solution.c_str())) {

                    for (int i = 0; i < GlobalIllumination_Solution.size(); i++) {

                        bool is_selected = (currentGI_Solution == GlobalIllumination_Solution[i]);

                        if (ImGui::Selectable(GlobalIllumination_Solution[i].c_str(), is_selected)) {

                            currentGI_Solution = GlobalIllumination_Solution[i];
                            appref->lighting_RTX->GISolutionIndex = i;

                        }
                    }
                    ImGui::EndCombo();
                }


                //if (ImGui::CollapsingHeader("RT Reflections", ImGuiTreeNodeFlags_DefaultOpen)) {
                //    ImGui::Checkbox("Enable Raytraced Reflections", (bool*)&appref->RT_Reflection->bReflections);
                //
                //}

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    {
        ImGui::Begin("Main Viewport");

        ImGui::SetCursorPos(ImVec2(0, 0));

        // 2. Get the image's top-left screen position AND its size
        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        viewportSize = ImGui::GetContentRegionAvail(); // This is the image size

        // Display the appropriate texture based on current render pass
        switch (appref->DefferedDecider) {
                case 0: ImGui::Image((ImTextureID)appref->SSGITextureId, viewportSize); break;
                case 1: ImGui::Image((ImTextureID)appref->Sampled_GI_ID, viewportSize); break;
                case 2: ImGui::Image((ImTextureID)appref->ReSTIR_DITextureId, viewportSize); break;
                case 3: ImGui::Image((ImTextureID)appref->FinalRenderTextureId, viewportSize); break;
        }

        ImGuizmo::SetRect(imageTopLeft.x, imageTopLeft.y, viewportSize.x, viewportSize.y);

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        if (appref->UserInterfaceItems.empty()) {

            ImGui::End();
        }
        else
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

            // Handle gizmo manipulation
            isItemSelected = (UserInterfaceItemsIndex >= 0 && UserInterfaceItemsIndex < appref->UserInterfaceItems.size());

            if (isItemSelected) {

                auto& item = appref->UserInterfaceItems[UserInterfaceItemsIndex];

                if (item)
                {
                    // Get the correct matrix
                    if (SelectedInstanceIndex != -1) {

                        Model* model = dynamic_cast<Model*>(item);

                        if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex]) {

                            modelMatrix = model->Instances[SelectedInstanceIndex]->GetTransformationMatrix();

                        }
                    }
                    else {
                        modelMatrix = item->GetModelMatrix();
                    }

                    // Manipulate in the viewport window
                    ImGuizmo::Manipulate(glm::value_ptr(cameraview), glm::value_ptr(cameraprojection),
                                         currentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(modelMatrix));

                    if (ImGuizmo::IsUsing()) {
                        // Apply changes
                        if (SelectedInstanceIndex != -1) {

                            Model* model = dynamic_cast<Model*>(item);

                            if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex]) {

                                model->Instances[SelectedInstanceIndex]->SetTrasnformationMatrix(modelMatrix);

                            }
                        }
                        else {

                            item->SetModelMatrix(modelMatrix);

                        }
                    }
                    // Decompose matrix for the Details Panel
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix), matrixTranslation, matrixRotation, matrixScale);
                }
                else
                {
                    isItemSelected = false; // Item is null, deselect
                }
            }
        }
        ImGui::End();
    }


    {
        ImGui::Begin("Details Panel");

        if (!isItemSelected)
        {
            ImGui::Text("No item selected.");
        }
        else
        {
            auto& item = appref->UserInterfaceItems[UserInterfaceItemsIndex];

            // Recompose matrix from any changes made in the input boxes
            ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(modelMatrix));

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool matrixChanged = false;
                if (SelectedInstanceIndex != -1)
                {
                    Model* model = dynamic_cast<Model*>(item);
                    if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex])
                    {
                        glm::vec3 pos = model->Instances[SelectedInstanceIndex]->GetPostion();
                        glm::vec3 rot = model->Instances[SelectedInstanceIndex]->GetRotation();
                        glm::vec3 scl = model->Instances[SelectedInstanceIndex]->GetScale();

                        if (ImGui::InputFloat3("Position", glm::value_ptr(pos))) { model->Instances[SelectedInstanceIndex]->SetPostion(pos); }
                        if (ImGui::InputFloat3("Rotation", glm::value_ptr(rot))) { model->Instances[SelectedInstanceIndex]->SetRotation(rot); }
                        if (ImGui::InputFloat3("Scale", glm::value_ptr(scl))) { model->Instances[SelectedInstanceIndex]->SetScale(scl); }
                    }
                }
                else
                {
                    // Handle non-instanced items like lights
                    if (ImGui::InputFloat3("Position", matrixTranslation)) { matrixChanged = true; }
                    if (ImGui::InputFloat3("Rotation", matrixRotation)) { matrixChanged = true; }
                    if (ImGui::InputFloat3("Scale", matrixScale)) { matrixChanged = true; }

                    if (matrixChanged)
                    {
                        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(modelMatrix));
                        item->SetModelMatrix(modelMatrix);
                        LastModelMatrix = modelMatrix; // Update last matrix
                    }
                }
            }

            // --- Model-Specific Section ---
            if (SelectedInstanceIndex != -1)
            {
                Model* model = dynamic_cast<Model*>(item);

                if (model && SelectedInstanceIndex < model->Instances.size() && model->Instances[SelectedInstanceIndex])
                {
                   // if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
                   // {
                   //     //ImGui::Checkbox("Cube Map Reflections", (bool*)&model->Instances[SelectedInstanceIndex]->bCubeMapReflection);
                   //     //ImGui::Checkbox("Screen Space Reflections", (bool*)&model->Instances[SelectedInstanceIndex]->bScreenSpaceReflection);
                   //     model->Instances[SelectedInstanceIndex]->UpdateGPU_ReflectionFlags();
                   // }

                    //if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
                    //{
                    //    if (ImGui::Button("Instantiate")) { model->Instantiate(); }
                    //        ImGui::SameLine();
                    //
                    //    if (ImGui::Button("Destroy"))
                    //    {
                    //        model->Destroy(SelectedInstanceIndex);
                    //        UserInterfaceItemsIndex = -1; 
                    //        SelectedInstanceIndex = -1;
                    //    }
                    //}
                }
            }
            else
            {
                Light* light = dynamic_cast<Light*>(item);

                if (light)
                {
                    if (ImGui::CollapsingHeader("Light Properties", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::ColorEdit3("Color", glm::value_ptr(light->color));
                        ImGui::InputFloat("Light Intensity", &light->lightIntensity);
                        ImGui::InputFloat("Ambience Value", &light->ambientStrength);
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
        }
        ImGui::End();
    }

    {
        ImGui::Begin("DDGI Visibility Atlas");
        viewportSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)appref->DDGIIVisibilityAtlasID, viewportSize);
        ImGui::End();
    }

    {
        ImGui::Begin("DDGI Irradiance Atlas");
        viewportSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)appref->DDGIIrradianceAtlasID, viewportSize);
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
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vulkancontext->LogicalDevice.destroyDescriptorPool(ImGuiDescriptorPool);
}

UserInterface::~UserInterface()
{
	CleanUp();
}