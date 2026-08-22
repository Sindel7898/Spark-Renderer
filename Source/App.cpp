#define _CRTDBG_MAP_ALLOC
#include "App.h"
#include <optional>
#include <vk_mem_alloc.h>
#include "Window.h"
#include "Camera.h"
#include "BufferManager.h"
#include "VulkanContext.h"
#include "FramesPerSecondCounter.h"
#include "Light.h"
#include "CombinedResult_FullScreenQuad.h"
#include "SSGI.h"
#include <crtdbg.h>
#include "SkyBox.h"
#include "Model.h"
#include "UserInterface.h"
#include "Pipeline_Manager.h"
#include "FXAA_FullScreenQuad.h"
#include "SSR_FullScreenQuad.h"
#include "SSAO_FullScreenQuad.h"
#include "Lighting_RTX.h"
#include "DynamicDiffuse_RTGI.h"
#include "ReSTIR_DI.h"
#include <pix.h>
#include "Tracy.hpp"
#include <random>
#include "NvdiaDLSS_Intergration.h"
#include <iostream>

#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)

App::App() : window(1920, 1080, "Spark Renderer")          
, DLSS_Intergration()                            
, vulkanContext(window, DLSS_Intergration)       
, bufferManger(&vulkanContext)                   
, camera(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, window.GetWindow())
, userinterface(&vulkanContext, &window, &bufferManger)
, pipelineManager(&vulkanContext)
{
	DLSS_Intergration.initializePointers(&bufferManger, &vulkanContext, &camera);

	glfwSetWindowUserPointer(window.GetWindow(), this);

	createSyncObjects();
	createCommandPool();

	skyBox = std::shared_ptr<SkyBox>(new SkyBox(&vulkanContext, commandPool, &camera, &bufferManger), SkyBoxDeleter);

	createDescriptorPool();

	LoadAllObjects();

	bufferManger.CreateSharedBuffers(commandPool);

	SwitchScene(0);

	lighting_RTX            = std::make_unique<Lighting_RTX>                 (&bufferManger, &vulkanContext, &camera, commandPool, skyBox.get());
	ssao_FullScreenQuad     = std::make_unique<SSA0_FullScreenQuad>          (&bufferManger, &vulkanContext, &camera, commandPool);
	fxaa_FullScreenQuad     = std::make_unique<FXAA_FullScreenQuad>          (&bufferManger, &vulkanContext, &camera, commandPool);
	Combined_FullScreenQuad = std::make_unique<CombinedResult_FullScreenQuad>(&bufferManger, &vulkanContext, &camera, commandPool, lighting_RTX.get());
	SSGI_FullScreenQuad     = std::make_unique<SSGI>                         (&bufferManger, &vulkanContext, &camera, commandPool, lighting_RTX.get());
	dynamicDiffuse_RTGI     = std::make_unique<DynamicDiffuse_RTGI>          ("../Textures/Sphere/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger, skyBox.get(), lighting_RTX.get());
	Restir_DI               = std::make_unique<ReSTIR_DI>                    (&vulkanContext, commandPool, &camera, &bufferManger, lighting_RTX.get(), SSGI_FullScreenQuad.get(), dynamicDiffuse_RTGI.get());

	createCommandBuffer();
	CreateGraphicsPipeline();
	createShaderBindingTable();
	createDepthTextureImage();
	createGBuffer();

	recreateSwapChain();
	CreateDebugUtils();
}

App::~App()
{
	vulkanContext.LogicalDevice.waitIdle();

	DestroyBuffers();

	if (!commandBuffers.empty()) {
		vulkanContext.LogicalDevice.freeCommandBuffers(commandPool, commandBuffers);
		commandBuffers.clear();
	}

	vulkanContext.LogicalDevice.destroyDescriptorPool(DescriptorPool);
	vulkanContext.LogicalDevice.destroyCommandPool(commandPool);
	destroyPipeline();
	DestroySyncObjects();

	vkb::destroy_debug_utils_messenger(vulkanContext.VulkanInstance, vulkanContext.Debug_Messenger);

#ifndef NDEBUG
	_CrtDumpMemoryLeaks();  
#endif
}

void App::Run()
{
	FramesPerSecondCounter fpsCounter(0.1f);

	while (!window.shouldClose())
	{
		for (auto& model : Models) {
			model->UpdateHistory();
		}

		glfwPollEvents();
		CalculateFps(fpsCounter);

		camera.OnFrameStart();
		camera.Update(deltaTime); 

		if (camera.GetViewMatrix() != camera.GetPrevViewMatrix()) {
			vulkanContext.ResetFrameCount();
		}

		userinterface.DrawUi(this, skyBox.get(), &vulkanContext);

		vulkanContext.UpdateFrameCount();
		Draw();

		userinterface.SaveNVPerf();
	}
	vulkanContext.LogicalDevice.waitIdle();
}

void App::CalculateFps(FramesPerSecondCounter& fpsCounter)
{
	const double newTimeStamp = glfwGetTime();
	deltaTime = static_cast<float>(newTimeStamp - LasttimeStamp);
	LasttimeStamp = newTimeStamp;
	fpsCounter.tick(deltaTime);
}

void App::Draw()
{
	vulkanContext.LogicalDevice.waitForFences(1, &waitFences[currentFrame], vk::True, UINT64_MAX);

	uint32_t imageIndex;
	try {
		vulkanContext.LogicalDevice.acquireNextImageKHR(
			vulkanContext.swapChain,
			UINT64_MAX,
			presentCompleteSemaphores[currentFrame],
			nullptr,
			&imageIndex
		);
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		std::cerr << "Attempting to recreate swap chain..." << std::endl;
		recreateSwapChain();
		framebufferResized = false;
		return;
	}

	vulkanContext.LogicalDevice.resetFences(1, &waitFences[currentFrame]);

	updateUniformBuffer(currentFrame);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	vk::Semaphore waitSemaphores[] = { presentCompleteSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eAllCommands };
	vk::Semaphore submitSemaphores[] = { renderCompleteSemaphores[currentFrame] };

	vk::SubmitInfo submitInfo{};
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = submitSemaphores;

	if (vulkanContext.graphicsQueue.submit(1, &submitInfo, waitFences[currentFrame]) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit draw commands");
	}

	VkSwapchainKHR swapchains[] = { (VkSwapchainKHR)vulkanContext.swapChain };
	VkSemaphore waitSems[] = { (VkSemaphore)submitSemaphores[0] };

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSems;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	try {
		vk::Result result = vulkanContext.presentQueue.presentKHR(presentInfo);
	}
	catch (const vk::OutOfDateKHRError& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		std::cerr << "Attempting to recreate swap chain..." << std::endl;
		recreateSwapChain();
		framebufferResized = false;
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	ImGui::EndFrame();
}

void App::LoadAllObjects()
{
	// Sponza Scene Setup
	auto Sponza = std::shared_ptr<Model>(new Model("../Textures/PBR_Sponza/Sponza.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto Bunny = std::shared_ptr<Model>(new Model("../Textures/Bunny2/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto Dragon = std::shared_ptr<Model>(new Model("../Textures/Dragon/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	Bunny->Instances[0]->SetPostion(glm::vec3(-14.224, -0.329, 0.357));
	Bunny->Instances[0]->SetRotation(glm::vec3(-179.999, -33.858, -179.999));
	Bunny->Instances[0]->SetScale(glm::vec3(0.120, 0.120, 0.120));
	Bunny->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Dragon->Instances[0]->SetPostion(glm::vec3(7.153, -0.523, -2.166));
	Dragon->Instances[0]->SetRotation(glm::vec3(179.997, 44.147, 179.993));
	Dragon->Instances[0]->SetScale(glm::vec3(0.120, 0.120, 0.120));
	Dragon->Instances[0]->CubeMapReflectiveSwitch(false);
	Dragon->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Sponza->Instances[0]->SetScale(glm::vec3(5.000, 5.000, 5.000));
	Sponza->Instances[0]->CubeMapReflectiveSwitch(false);
	Sponza->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny->createDescriptorSets(DescriptorPool);
	Dragon->createDescriptorSets(DescriptorPool);
	Sponza->createDescriptorSets(DescriptorPool);

	SponzaSceneModels.push_back(std::move(Bunny));
	SponzaSceneModels.push_back(std::move(Dragon));
	SponzaSceneModels.push_back(std::move(Sponza));

	// Cornell Scene Setup
	auto Bunny2 = std::shared_ptr<Model>(new Model("../Textures/Bunny2/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto Dragon2 = std::shared_ptr<Model>(new Model("../Textures/Dragon/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto CornelBox = std::shared_ptr<Model>(new Model("../Textures/CornelBox/Cornel.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	Bunny2->Instances[0]->SetPostion(glm::vec3(-5.936, 3.043, -9.525));
	Bunny2->Instances[0]->SetRotation(glm::vec3(-179.998, -0.000, -180.000));
	Bunny2->Instances[0]->SetScale(glm::vec3(0.082, 0.082, 0.082));
	Bunny2->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny2->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Dragon2->Instances[0]->SetPostion(glm::vec3(6.087, 13.277, 3.776));
	Dragon2->Instances[0]->SetRotation(glm::vec3(-179.987, -49.785, 179.967));
	Dragon2->Instances[0]->SetScale(glm::vec3(0.082, 0.082, 0.082));
	Dragon2->Instances[0]->CubeMapReflectiveSwitch(false);
	Dragon2->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	CornelBox->Instances[0]->SetPostion(glm::vec3(0, 0, 0));
	CornelBox->Instances[0]->SetScale(glm::vec3(1.5, 1.5, 1.5));
	CornelBox->Instances[0]->CubeMapReflectiveSwitch(false);
	CornelBox->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny2->createDescriptorSets(DescriptorPool);
	Dragon2->createDescriptorSets(DescriptorPool);
	CornelBox->createDescriptorSets(DescriptorPool);

	CornelSceneModels.push_back(std::move(Bunny2));
	CornelSceneModels.push_back(std::move(Dragon2));
	CornelSceneModels.push_back(std::move(CornelBox));

	// Alt Cornell Scene Setup
	auto Bunny3 = std::shared_ptr<Model>(new Model("../Textures/Bunny2/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto AltCornelBox = std::shared_ptr<Model>(new Model("../Textures/EmptyCornel/Cornel.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	Bunny3->Instances[0]->SetPostion(glm::vec3(-1.185, -3.895, -1.218));
	Bunny3->Instances[0]->SetRotation(glm::vec3(-179.998, -0.000, -180.000));
	Bunny3->Instances[0]->SetScale(glm::vec3(0.190, 0.190, 0.190));
	Bunny3->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny3->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	AltCornelBox->Instances[0]->SetPostion(glm::vec3(0, 0, 0));
	AltCornelBox->Instances[0]->SetScale(glm::vec3(1.5, 1.5, 1.5));
	AltCornelBox->Instances[0]->CubeMapReflectiveSwitch(false);
	AltCornelBox->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny3->createDescriptorSets(DescriptorPool);
	AltCornelBox->createDescriptorSets(DescriptorPool);

	AltCornelSceneModels.push_back(std::move(Bunny3));
	AltCornelSceneModels.push_back(std::move(AltCornelBox));

	// Alt 2 Cornell Scene Setup
	auto Bunny4 = std::shared_ptr<Model>(new Model("../Textures/Bunny2/WhiteBunny.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto AltCornelBoxColorFull = std::shared_ptr<Model>(new Model("../Textures/EmptyCornel/Cornel_ColorFull.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	Bunny4->Instances[0]->SetPostion(glm::vec3(-1.185, -3.895, -1.218));
	Bunny4->Instances[0]->SetRotation(glm::vec3(-179.998, -0.000, -180.000));
	Bunny4->Instances[0]->SetScale(glm::vec3(0.190, 0.190, 0.190));
	Bunny4->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny4->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	AltCornelBoxColorFull->Instances[0]->SetPostion(glm::vec3(0, 0, 0));
	AltCornelBoxColorFull->Instances[0]->SetScale(glm::vec3(1.5, 1.5, 1.5));
	AltCornelBoxColorFull->Instances[0]->CubeMapReflectiveSwitch(false);
	AltCornelBoxColorFull->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny4->createDescriptorSets(DescriptorPool);
	AltCornelBoxColorFull->createDescriptorSets(DescriptorPool);

	Alt_2_CornelSceneModels.push_back(std::move(Bunny4));
	Alt_2_CornelSceneModels.push_back(std::move(AltCornelBoxColorFull));
}

void App::SpawnLights(int NumOfLights) {
	vulkanContext.LogicalDevice.waitIdle();
	UserInterfaceItems.clear();

	switch (currentSceneIndex) {
	case 0: for (auto& m : CornelSceneModels) UserInterfaceItems.push_back(m.get()); break;
	case 1: for (auto& m : SponzaSceneModels) UserInterfaceItems.push_back(m.get()); break;
	case 2: for (auto& m : AltCornelSceneModels) UserInterfaceItems.push_back(m.get()); break;
	case 3: for (auto& m : Alt_2_CornelSceneModels) UserInterfaceItems.push_back(m.get()); break;
	}

	lights.clear();
	lights.reserve(NumOfLights);

	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> disXZ(-20.0f, 20.0f);
	std::uniform_real_distribution<float> disY(0.0f, 40.0f);
	std::uniform_real_distribution<float> disc(0.0f, 1.0f);

	for (int i = 0; i < NumOfLights; i++) {
		auto light = std::make_unique<Light>(&vulkanContext, commandPool, &camera, &bufferManger);

		light->SetPosition(glm::vec3(disXZ(gen), disY(gen), disXZ(gen)));
		light->color = glm::vec3(disc(gen), disc(gen), disc(gen));
		light->lightIntensity = 0.3f;
		light->CastShadow = true;
		light->createDescriptorSets(DescriptorPool);

		UserInterfaceItems.push_back(light.get());
		lights.push_back(std::move(light));
	}

	UpdateRayTracingDescriptors();
	userinterface.SetLightCount(static_cast<int>(lights.size()));
	vulkanContext.ResetFrameCount();
}

void App::SwitchScene(int index)
{
	vulkanContext.LogicalDevice.waitIdle();

	Models.clear();
	UserInterfaceItems.clear();
	lights.clear(); 

	currentSceneIndex = index;

	if (index == 0)
	{
		for (auto& model : CornelSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 3;
		lights.reserve(LightCount);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> disXZ(-20, 20);
		std::uniform_real_distribution<float> disY(0, 40);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			auto light = std::make_unique<Light>(&vulkanContext, commandPool, &camera, &bufferManger);

			float randX = disXZ(gen);
			float randY = disY(gen);
			float randZ = disXZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);
			lights.push_back(std::move(light));
		}

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 10;
			dynamicDiffuse_RTGI->NumOfProbesY = 10;
			dynamicDiffuse_RTGI->NumOfProbesZ = 10;
			dynamicDiffuse_RTGI->RaysPerProbe = 128;
			dynamicDiffuse_RTGI->ProbeOffset  = glm::vec3(3.000, 3.000, 3.00);
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-13.000, -4.000, -15);
		}

		camera.SetPosition(glm::vec3{ -0.896284, 12.566, -37.7205 });
		camera.SetRotation(89.1, -2.5);
		DefferedDecider = 3;
	}
	else if (index == 1) 
	{
		for (auto& model : SponzaSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 1;
		lights.reserve(LightCount);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> disXZ(-20, 20);
		std::uniform_real_distribution<float> disY(0, 40);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			auto light = std::make_unique<Light>(&vulkanContext, commandPool, &camera, &bufferManger);

			float randX = disXZ(gen);
			float randY = disY(gen);
			float randZ = disXZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);
			lights.push_back(std::move(light));
		}

		lights[0]->lightType = 0;
		lights[0]->SetPosition(glm::vec3(-8.404, -89.175, -1.344));
		lights[0]->color = glm::vec3(1, 1, 1);

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 12;
			dynamicDiffuse_RTGI->NumOfProbesY = 5;
			dynamicDiffuse_RTGI->NumOfProbesZ = 11;
			dynamicDiffuse_RTGI->RaysPerProbe = 188;
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-55.011, 1.300, -23.000);
			dynamicDiffuse_RTGI->ProbeOffset = glm::vec3(9.000, 10.000, 4.300);
		}

		camera.SetPosition(glm::vec3{ 32.9095, 15.871, -0.912267 });
		camera.SetRotation(-178.2, -13.4);
		DefferedDecider = 2;
	}
	else if (index == 2)
	{
		for (auto& model : AltCornelSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 1;
		lights.reserve(LightCount);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> disXZ(-20, 20);
		std::uniform_real_distribution<float> disY(0, 40);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			auto light = std::make_unique<Light>(&vulkanContext, commandPool, &camera, &bufferManger);

			float randX = disXZ(gen);
			float randY = disY(gen);
			float randZ = disXZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);
			lights.push_back(std::move(light));
		}

		lights[0]->lightType = 0;
		lights[0]->SetPosition(glm::vec3(127.853, -215.645, 455.232));
		lights[0]->color = glm::vec3(1, 1, 1);
		lights[0]->lightIntensity = 1.5f;

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 10;
			dynamicDiffuse_RTGI->NumOfProbesY = 10;
			dynamicDiffuse_RTGI->NumOfProbesZ = 10;
			dynamicDiffuse_RTGI->RaysPerProbe = 128;
			dynamicDiffuse_RTGI->ProbeOffset = glm::vec3(3.000, 3.100, 3.000);
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-13.000, -3.500, -15.000);
		}

		camera.SetPosition(glm::vec3{ -0.896284, 12.566, -37.7205 });
		camera.SetRotation(89.1, -2.5);
		DefferedDecider = 3;
	}
	else if (index == 3)
	{
		for (auto& model : Alt_2_CornelSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 1;
		lights.reserve(LightCount);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> disX(-20, 20);
		std::uniform_real_distribution<float> disZ(-80, 0);
		std::uniform_real_distribution<float> disY(0, 10);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			auto light = std::make_unique<Light>(&vulkanContext, commandPool, &camera, &bufferManger);

			float randX = disX(gen);
			float randY = disY(gen);
			float randZ = disZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);
			lights.push_back(std::move(light));
		}

		lights[0]->lightType = 0;
		lights[0]->SetPosition(glm::vec3(127.853, -215.645, 455.232));
		lights[0]->color = glm::vec3(1, 1, 1);
		lights[0]->lightIntensity = 1.5f;

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 10;
			dynamicDiffuse_RTGI->NumOfProbesY = 10;
			dynamicDiffuse_RTGI->NumOfProbesZ = 10;
			dynamicDiffuse_RTGI->RaysPerProbe = 128;
			dynamicDiffuse_RTGI->ProbeOffset = glm::vec3(3.000, 3.100, 3.000);
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-13.000, -3.500, -15.000);
		}

		camera.SetPosition(glm::vec3{ -0.896284, 12.566, -37.7205 });
		camera.SetRotation(89.1, -2.5);
		DefferedDecider = 2;
	}

	for (auto& l : lights) {
		UserInterfaceItems.push_back(l.get());
	}

	DestroyTLAS();
	createTLAS();
	UpdateRayTracingDescriptors();

	if (RT_ShadowsPassPipeline) {
		updateUniformBuffer(currentFrame); 
		vulkanContext.ResetFrameCount();
	}
}

void App::CreateDebugUtils()
{
	Gbuffer_Label.pLabelName                    = "Gbuffer Pass";
	SSAO_Label.pLabelName                       = "SSAO Pass";
	RTShadows_Label.pLabelName                  = "Raytraced Shadow Pass";
	DirectLighting_Label.pLabelName             = "DirectLighting Pass";
	SSGI_Label.pLabelName                       = "SSGI Pass";
	FXAA_Label.pLabelName                       = "FXAA Pass";
	RTReflections_Label.pLabelName              = "Raytraced Reflections Pass";
	DDGI_Grid_Generation_Label.pLabelName       = "DDGI_Grid_Generation_Label";
	DDGI_Trace_Ray_Label.pLabelName             = "DDGI_Trace_Ray_Label";
	DDGI_Directions_Generation_Label.pLabelName = "DDGI_Directions_Generation_Label";
	DDGI_Calculate_Irradiance_Label.pLabelName  = "DDGI_Calculate_Irradiance_Label";
	DDGI_Sample_From_PorbeLabel.pLabelName      = "DDGI_Sample_From_PorbeLabel";
	DDGI_Update_Probe_Status_Label.pLabelName   = "DDGI_Update_Probe_Status_Label";
	ReSTIR_Label.pLabelName                     = "ReSTIR_Label";
	RayReconstruction.pLabelName                = "Ray_Reconstruction";
}
