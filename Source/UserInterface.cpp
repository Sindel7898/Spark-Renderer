#include "UserInterface.h"
#include <stdexcept>
#include "Window.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"
#include "SSAO_FullScreenQuad.h"
#include "App.h"

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
	pipeline_rendering_create_info.pColorAttachmentFormats = &vulkancontext->swapchainformat;
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
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// Set up the initial layout (only once)
	static bool first_time = true;
	if (first_time)
	{
		first_time = false;

		// Clear out any existing layout
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

		// Split the dockspace
		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
		ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
		ImGuiID dock_Bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, nullptr, &dock_main_id);

		ImGuiID dock_DetailsPanel_Top_id;
		ImGuiID dock_DetailsPanel_Bottom_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.5, nullptr,&dock_DetailsPanel_Top_id);
		// Dock windows
		ImGui::DockBuilderDockWindow("Global Settings", dock_DetailsPanel_Bottom_id);
		ImGui::DockBuilderDockWindow("Details Panel", dock_DetailsPanel_Top_id);
		ImGui::DockBuilderDockWindow("Main Viewport", dock_main_id);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGui::End();
}

void UserInterface::RenderUi(vk::CommandBuffer& CommandBuffer, int imageIndex)
{

	ImageTransitionData TransitionSwapchainToWriteData;
	TransitionSwapchainToWriteData.oldlayout = vk::ImageLayout::eUndefined;
	TransitionSwapchainToWriteData.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitionSwapchainToWriteData.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionSwapchainToWriteData.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitionSwapchainToWriteData.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionSwapchainToWriteData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitionSwapchainToWriteData.AspectFlag = vk::ImageAspectFlagBits::eColor;

	buffermanager->TransitionImage(CommandBuffer, &vulkancontext->swapchainImageData[imageIndex], TransitionSwapchainToWriteData);

	ImGui::Render();

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		 {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		 }
	//// Begin rendering for ImGui
	vk::RenderingAttachmentInfo imguiColorAttachment{};
	imguiColorAttachment.imageView = vulkancontext->swapchainImageData[imageIndex].imageView;
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

	ImageTransitionData TransitionSwapchainToPresentData;
	TransitionSwapchainToPresentData.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitionSwapchainToPresentData.newlayout = vk::ImageLayout::ePresentSrcKHR;
	TransitionSwapchainToPresentData.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitionSwapchainToPresentData.DestinationAccessflag = vk::AccessFlagBits::eMemoryRead;
	TransitionSwapchainToPresentData.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitionSwapchainToPresentData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eBottomOfPipe;
	TransitionSwapchainToPresentData.AspectFlag = vk::ImageAspectFlagBits::eColor;

	buffermanager->TransitionImage(CommandBuffer, &vulkancontext->swapchainImageData[imageIndex], TransitionSwapchainToPresentData);

	CommandBuffer.end();
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void UserInterface::DrawUi(App* appref)
{
	SetupDockingEnvironment();

	// Handle gizmo mode changes
	if (ImGui::IsKeyPressed(ImGuiKey_1)) {
		currentGizmoOperation = ImGuizmo::TRANSLATE;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_2)) {
		currentGizmoOperation = ImGuizmo::ROTATE;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_3)) {
		currentGizmoOperation = ImGuizmo::SCALE;
	}


	{
		ImGui::Begin("Global Settings");

		if (ImGui::Button("Refresh Shaders", ImVec2(100, 30))) {
			appref->recreatePipeline();
		}

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		if (ImGui::BeginCombo("Render Passes", currentPass.c_str()))
		{
			for (int i = 0; i < Passes.size(); i++) {

				bool is_selected = (currentPass == Passes[i]);

				if (ImGui::Selectable(Passes[i].c_str(), is_selected)) {

					currentPass = Passes[i];

					appref->DefferedDecider = i;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Checkbox("FXAA", (bool*)&appref->fxaa_FullScreenQuad->bFXAA);
		ImGui::Checkbox("Wire Frame", &appref->bWireFrame);


		ImGui::Text("SSAO Settings");
		ImGui::InputInt("Kernel Size", &appref->ssao_FullScreenQuad->KernelSize);
		ImGui::InputFloat("radius", &appref->ssao_FullScreenQuad->Radius);
		ImGui::InputFloat("Bias", &appref->ssao_FullScreenQuad->Bias);
		ImGui::Checkbox("SSAO", (bool*)&appref->ssao_FullScreenQuad->bShouldSSAO);

		ImGui::End();
	}

	// Main viewport with gizmos
	ImGui::Begin("Main Viewport");

	if (ImGui::GetMainViewport())
	{
		viewportSize = ImGui::GetContentRegionAvail();
	}

	if (appref->DefferedDecider == 0)
	{
		ImGui::Image((ImTextureID)appref->PositionRenderTextureId, viewportSize);

	}

	if (appref->DefferedDecider == 1)
	{
		ImGui::Image((ImTextureID)appref->NormalTextureId, viewportSize);
	}

	if (appref->DefferedDecider == 2)
	{
		ImGui::Image((ImTextureID)appref->AlbedoTextureId, viewportSize);
	}

	if (appref->DefferedDecider == 3)
	{
		ImGui::Image((ImTextureID)appref->SSAOTextureId, viewportSize);

	}

	if (appref->DefferedDecider == 4)
	{
		ImGui::Image((ImTextureID)appref->Shadow_TextureId, viewportSize);

	}

	if (appref->DefferedDecider == 5)
	{
		ImGui::Image((ImTextureID)appref->FinalRenderTextureId, viewportSize);

	}



	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();


	if (appref->camera && !appref->UserInterfaceItems.empty())
	{
		glm::mat4 cameraprojection = appref->camera->GetProjectionMatrix();
		glm::mat4 cameraview = appref->camera->GetViewMatrix();


		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver())
		{
			for (int i = 0; i < appref->UserInterfaceItems.size(); i++)
			{
				if (appref->UserInterfaceItems[i])
				{
					glm::vec3 UserInterfaceItemsPosition = appref->UserInterfaceItems[i]->position;

					float distance = CalculateDistanceInScreenSpace(cameraprojection, cameraview, UserInterfaceItemsPosition);

					if (distance < 100.0f)
					{
						UserInterfaceItemsIndex = i;
						break;
					}
					else
					{
						UserInterfaceItemsIndex = -1;
					}
				}
			}

		}

		if (UserInterfaceItemsIndex >= 0 && UserInterfaceItemsIndex < appref->UserInterfaceItems.size())
		{
			auto& item = appref->UserInterfaceItems[UserInterfaceItemsIndex];

			if (item)
			{
				glm::mat4 ItemsModelMatrix = item->GetModelMatrix();

				ImGuizmo::Manipulate(glm::value_ptr(cameraview), glm::value_ptr(cameraprojection),
					currentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(ItemsModelMatrix));

				float matrixTranslation[3], matrixRotation[3], matrixScale[3];
				ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(ItemsModelMatrix), matrixTranslation, matrixRotation, matrixScale);
				ImGui::Begin("Details Panel");

				ImGui::Dummy(ImVec2(0.0f, 20.0f));
				ImGui::Separator();

				ImGui::InputFloat3("Position", matrixTranslation);
				ImGui::InputFloat3("Rotation", matrixRotation);
				ImGui::InputFloat3("Scale", matrixScale);

				ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(ItemsModelMatrix));
				item->SetModelMatrix(ItemsModelMatrix);


				if (ImGuizmo::IsUsing())
				{
					item->SetModelMatrix(ItemsModelMatrix);
				}

				Model* model =  dynamic_cast<Model*>(item);

				if (model)
				{
					ImGui::Checkbox("Cube Map Reflections", (bool*)&model->bCubeMapReflection);
					ImGui::Checkbox("Screen Space Reflections", (bool*)&model->bScreenSpaceReflection);

				}
			}
		


			Light* light = dynamic_cast<Light*>(item);

			if (light)
			{
				ImGui::ColorEdit3("Color", glm::value_ptr(light->color));
				ImGui::InputFloat("Light Intensity", &light->lightIntensity);
				ImGui::InputFloat("Ambience Value", &light->ambientStrength);

				if (ImGui::BeginCombo("Light Type", currentItem.c_str()))
				{
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
			ImGui::End();
		};
	}
	ImGui::End();

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