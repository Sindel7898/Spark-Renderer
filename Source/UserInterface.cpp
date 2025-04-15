#include "UserInterface.h"
#include <stdexcept>
#include "Window.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"

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
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 100);
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
	init_info.ImageCount = vulkancontext->swapchainImages.size();
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

		// Dock windows
		ImGui::DockBuilderDockWindow("Stats Window", dock_Bottom_id);
		ImGui::DockBuilderDockWindow("Hello, world!", dock_left_id);
		ImGui::DockBuilderDockWindow("Details Panel", dock_right_id);
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

	buffermanager->TransitionImage(CommandBuffer, vulkancontext->swapchainImages[imageIndex], TransitionSwapchainToWriteData);

	ImGui::Render();
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		 {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		 }
	//// Begin rendering for ImGui
	vk::RenderingAttachmentInfoKHR imguiColorAttachment{};
	imguiColorAttachment.imageView = vulkancontext->swapchainImageViews[imageIndex];
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

	buffermanager->TransitionImage(CommandBuffer, vulkancontext->swapchainImages[imageIndex], TransitionSwapchainToPresentData);

	CommandBuffer.end();
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


//call every Frame
void UserInterface::DrawUi(bool& bRecreateDepth,int& DefferedDecider, VkDescriptorSet FinalRenderTextureId, VkDescriptorSet PositionRenderTextureId,
	                                                                  VkDescriptorSet NormalTextureId, VkDescriptorSet AlbedoTextureId,Camera* camera, std::vector<std::shared_ptr<Model>>& Models, std::vector<std::shared_ptr<Light>>& Lights)
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

	if (ImGui::IsKeyPressed(ImGuiKey_4)) {
		DefferedDecider = 1;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_5)) {
		DefferedDecider = 2;

	}
	if (ImGui::IsKeyPressed(ImGuiKey_6)) {
		DefferedDecider = 3;

	}
	if (ImGui::IsKeyPressed(ImGuiKey_7)) {
		DefferedDecider = 4;

	}

	// Your other UI windows...
	{
		ImGui::Begin("Hello, world!");
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		
		ImGui::End();
	}

	// Main viewport with gizmos
	ImGui::SetNextWindowSize(ImVec2(400, 300));
	ImGui::Begin("Main Viewport");
	ImGui::Text("Main ViewPort");

	ImVec2 viewportSize;

	if (ImGui::GetMainViewport())
	{
		viewportSize = ImGui::GetContentRegionAvail();
	}

	if (DefferedDecider == 1)
	{
		ImGui::Image((ImTextureID)FinalRenderTextureId, viewportSize);

	}

	if (DefferedDecider == 2)
	{
		ImGui::Image((ImTextureID)PositionRenderTextureId, viewportSize);

	}

	if (DefferedDecider == 3)
	{
		ImGui::Image((ImTextureID)NormalTextureId, viewportSize);

	}

	if (DefferedDecider == 4)
	{
		ImGui::Image((ImTextureID)AlbedoTextureId, viewportSize);

	}
	//ImguiViewPortRenderTextureSizeDecider(bRecreateDepth);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();


	if (camera && !Models.empty())
	{
		glm::mat4 cameraprojection = camera->GetProjectionMatrix();
		glm::mat4 cameraview = camera->GetViewMatrix();


		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver())
		{
			for (int i = 0; i < Models.size(); i++)
			{
				glm::vec3 ModelPosition = Models[i]->position;

				float distance = CalculateDistanceInScreenSpace(cameraprojection, cameraview, ModelPosition);
				
				if (distance < 100.0f)
				{
					selectedModelIndex = i;
					break;
				}
				else
				{
					selectedModelIndex = -1;
				}
			}

			if (selectedModelIndex <= -1)
			{
				for (int i = 0; i < Lights.size(); i++)
				{
					glm::vec3 LightPosition = Lights[i]->position;
					float distance = CalculateDistanceInScreenSpace(cameraprojection, cameraview, LightPosition);

					if (distance < 100.0f)
					{
						selectedLightIndex = i;
						break; 
					}
				}
			}

		}

		if (selectedModelIndex >= 0 && selectedModelIndex < Models.size())
		{
			auto& model = Models[selectedModelIndex];
			
			glm::mat4 ModelsModelMatrix = model->GetModelMatrix();

			ImGuizmo::Manipulate(glm::value_ptr(cameraview), glm::value_ptr(cameraprojection),
				                 currentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(ModelsModelMatrix));



			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(ModelsModelMatrix), matrixTranslation, matrixRotation, matrixScale);
			ImGui::Begin("Details Panel");

			ImGui::Dummy(ImVec2(0.0f, 20.0f));
			ImGui::Separator();

			ImGui::InputFloat3("Position", matrixTranslation);
			ImGui::InputFloat3("Rotation", matrixRotation);
			ImGui::InputFloat3("Scale", matrixScale);

			ImGui::End();

			ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(ModelsModelMatrix));
			model->SetModelMatrix(ModelsModelMatrix);


			if (ImGuizmo::IsUsing())
			{
				model->SetModelMatrix(ModelsModelMatrix);
			}
		}
		else if (selectedLightIndex >= 0 && selectedLightIndex < Lights.size())
		{
			auto& light = Lights[selectedLightIndex];

			glm::mat4 LightModelMatrix = light->GetModelMatrix();

			ImGuizmo::Manipulate(glm::value_ptr(cameraview), glm::value_ptr(cameraprojection),
				currentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(LightModelMatrix));


			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(LightModelMatrix), matrixTranslation, matrixRotation, matrixScale);
			ImGui::Begin("Details Panel");
			
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
			ImGui::Separator();

			ImGui::InputFloat3("Position", matrixTranslation);
			ImGui::InputFloat3("Rotation", matrixRotation);
			ImGui::InputFloat3("Scale", matrixScale);

			ImGui::Dummy(ImVec2(0.0f, 20.0f));
			ImGui::Separator();


			ImGui::ColorEdit3 ("Color", glm::value_ptr(light->color));
			ImGui::InputFloat("Ambience Value", &light->ambientStrength);
			ImGui::End();

			ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(LightModelMatrix));
			light->SetModelMatrix(LightModelMatrix);

			if (ImGuizmo::IsUsing())
			{
				light->SetModelMatrix(LightModelMatrix);
			}
		}
		else
		{
			ImGui::Begin("Details Panel");

			ImGui::End();
		}
	}

	ImGui::End();

	ImGui::Begin("Stats Window");

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

//void UserInterface::ShowGuizmoToLocation(glm::mat4 CameraProjection, glm::mat4 cameraview, glm::vec3 position)
//{
//	
//}


void UserInterface::ImguiViewPortRenderTextureSizeDecider(bool& bRecreateDepth)
{
	ImVec2 viewportSize;

	if (ImGui::GetMainViewport())
	{
		viewportSize = ImGui::GetContentRegionAvail();
	}

	static ImVec2 lastSize = ImVec2(0.0f, 0.0f);

	if (viewportSize.x != lastSize.x || viewportSize.y != lastSize.y)
	{
		if (viewportSize.x <= 5 || viewportSize.y <= 5)
		{
			viewportSize = ImVec2(5.0f, 5.0f);
		}

		vulkancontext->LogicalDevice.waitIdle();
		//ImGui_ImplVulkan_RemoveTexture(RenderTextureId);
		buffermanager->DestroyImage(ImguiViewPortRenderTextureData);
		CreateViewPortRenderTexture(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));
		bRecreateDepth = true;
		lastSize = viewportSize;
	}

	//ImGui::Image((ImTextureID)RenderTextureId, viewportSize);
}


ImageData* UserInterface::CreateViewPortRenderTexture(uint32_t X, uint32_t Y)
{
	RenderTextureExtent = vk::Extent3D(X, Y,1);
	ImguiViewPortRenderTextureData = buffermanager->CreateImage(RenderTextureExtent, vulkancontext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	ImguiViewPortRenderTextureData.imageView = buffermanager->CreateImageView(ImguiViewPortRenderTextureData.image, vulkancontext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	ImguiViewPortRenderTextureData.imageSampler = buffermanager->CreateImageSampler();

	//RenderTextureId = ImGui_ImplVulkan_AddTexture(ImguiViewPortRenderTextureData.imageSampler,
	//	ImguiViewPortRenderTextureData.imageView,
	//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return &ImguiViewPortRenderTextureData;
}


vk::Extent3D UserInterface::GetRenderTextureExtent()
{
	return RenderTextureExtent;
}