#include "vk_backend.h"
#include <chrono> 
#include <fstream> 
#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_tools.h"
#include "Util.h"
 
#include "AssetManagement/AssetManager.h"
#include "Game/Scene.h"
#include "Game/Laptop.h"
#include "Renderer/RasterRenderer.h"
#include "Profiler.h"

#include "API/Vulkan/Managers/vk_command_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_descriptor_manager.h"
#include "API/Vulkan/Managers/vk_instance_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_raytracing_manager.h"
#include "API/Vulkan/Managers/vk_pipeline_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_swapchain_manager.h"
#include "API/Vulkan/Managers/vk_sync_manager.h"

#include "API/Vulkan/Renderer/vk_descriptor_indices.h"
#include "API/Vulkan/Renderer/vk_renderer.h"

#include "BackEnd/GLFWIntegration.h"

#define NOOSE_PI 3.14159265359f
const bool _printAvaliableExtensions = false;
float _deltaTime;
std::vector<std::string> _loadingText;

namespace VulkanBackEnd {
	VkDevice GetDevice() { return VulkanDeviceManager::GetDevice(); }
	VkInstance GetInstance() { return VulkanInstanceManager::GetInstance(); }
	VkSurfaceKHR GetSurface() { return VulkanInstanceManager::GetSurface(); }
	VmaAllocator GetAllocator() { return VulkanMemoryManager::GetAllocator(); }
	VkPhysicalDevice GetPhysicalDevice() { return VulkanDeviceManager::GetPhysicalDevice(); }
	VkQueue GetGraphicsQueue() { return VulkanDeviceManager::GetGraphicsQueue(); }
	uint32_t GetGraphicsQueueFamily() { return VulkanDeviceManager::GetGraphicsQueueFamily(); }
	VkQueue GetPresentQueue() { return VulkanDeviceManager::GetPresentQueue(); }
	uint32_t GetPresentQueueFamily() { return VulkanDeviceManager::GetPresentQueueFamily(); }
	VkSwapchainKHR GetSwapchain() { return VulkanSwapchainManager::GetSwapchain(); }
	std::vector<VkImage>& GetSwapchainImages() { return VulkanSwapchainManager::GetSwapchainImages(); }
	std::vector<VkImageView>& GetSwapchainImageViews() { return VulkanSwapchainManager::GetSwapchainImageViews(); }
	VkFormat GetSwapchainImageFormat() { return VulkanSwapchainManager::GetSwapchainImageFormat(); }
}

// Will become VulkanRenderer:
namespace VulkanBackEnd {
	void BlitAllocatedImageToSwapchain(VkCommandBuffer cmd, AllocatedImage& srcImage, uint32_t swapchainIndex);

	uint64_t g_vertexBuffer = 0;
	uint64_t g_indexBuffer = 0;
	uint64_t g_mousePickBufferCPU = 0;
	uint64_t g_mousePickBufferGPU = 0;
}

namespace VulkanBackEnd {

	bool VulkanBackEnd::InitMinimum() {
		if (!VulkanInstanceManager::Init()) return false;
		if (!VulkanDeviceManager::Init()) return false;
		if (!VulkanMemoryManager::Init()) return false;
		if (!VulkanSwapchainManager::Init()) return false;
		if (!VulkanSyncManager::Init()) return false;
		if (!VulkanCommandManager::Init()) return false;
		if (!VulkanDescriptorManager::Init()) return false;
		if (!VulkanRenderer::Init()) return false;
		if (!VulkanPipelineManager::Init()) return false;

		AssetManager::Init();
		AssetManager::LoadFont();
		AssetManager::LoadHardcodedMesh();

		upload_meshes();

		VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");
		if (!linearSampler) return false;


		VulkanDescriptorSet& bindlessSet = VulkanRenderer::GetStaticDescriptorSet();
		HellDescriptorSet& legacySet = VulkanDescriptorManager::GetStaticDescriptorSet();

		VkDescriptorImageInfo samplerImageInfo = {};
		samplerImageInfo.sampler = linearSampler->GetSampler();

		legacySet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);
		bindlessSet.WriteImage(0, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER);

		// All textures
		VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
		for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
			VkImageView imageView = (i < AssetManager::GetNumberOfTextures()) ? AssetManager::GetTexture(i)->imageView : AssetManager::GetTexture(0)->imageView;

			textureImageInfo[i].sampler = nullptr;
			textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureImageInfo[i].imageView = imageView;
		
			bindlessSet.WriteImage(1, imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
		}

		bindlessSet.Update();
		legacySet.Update(GetDevice(), 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

		VulkanBackEnd::ToggleFullscreen();

		std::cout << "Init::Minimum() complete\n";
		return true;
	}
}


#include <set>


void VulkanBackEnd::AddLoadingText(std::string text) {
	_loadingText.push_back(text);
}

void VulkanBackEnd::LoadNextItem() {

	// These return false if there is nothing to load
	// meaning it will work its way down this function each game loop until everything is loaded

	if (AssetManager::LoadNextTexture())
		return;

	if (AssetManager::LoadNextModel()) 
		return;
	else
		upload_meshes();

	static bool shadersLoaded = false;
	static bool shadersLoadedMessageSeen = false;
	if (!shadersLoadedMessageSeen) {
		shadersLoadedMessageSeen = true;
		AddLoadingText("Compiling shaders...");
		return;
	}
	if (!shadersLoaded) {
		shadersLoaded = true;
		LoadLegacyShaders();
	}

	static bool rtSetup = false;
	static bool rtSetupMSG = false;
	if (!rtSetupMSG) {
		rtSetupMSG = true;
		AddLoadingText("Initilizing raytracing...");
		return;
	}
	if (!rtSetup) {
		rtSetup = true;
		AssetManager::BuildMaterials();
		Laptop::Init();
		Audio::Init();
		Scene::Init();						// Scene::Init creates wall geometry, and thus must run before upload_meshes
		create_rt_buffers();
		
		init_raytracing();
		update_static_descriptor_set_old();
		UpdateStaticDescriptorSet();
		Input::SetMousePos(_windowedModeExtent.width / 2, _windowedModeExtent.height / 2);
	}

	static bool meshUploaded = false;
	static bool meshUploadedMSG = false;
	if (!meshUploadedMSG) {
		meshUploadedMSG = true;
		AddLoadingText("Uploading mesh...");
		return;
	}
	if (!meshUploaded) {
		meshUploaded = true;
		upload_meshes();
	}

	_loaded = true;
	TextBlitter::ResetDebugText();
}

void VulkanBackEnd::Cleanup() {
	GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();

	vkDeviceWaitIdle(GetDevice());

	VulkanResourceManager::Cleanup();

	// Cleanup textures properly
	for (int i = 0; i < AssetManager::GetNumberOfTextures(); i++) {
		Texture* tex = AssetManager::GetTexture(i);
		if (tex) {
			if (tex->imageView != VK_NULL_HANDLE) {
				vkDestroyImageView(GetDevice(), tex->imageView, nullptr);
				tex->imageView = VK_NULL_HANDLE;
			}
		}
	}

	VulkanPipelineManager::Cleanup();


	// Cleanup Raytracing
	cleanup_raytracing();

	// Cleanup Mesh Buffers
	for (MeshOLD& mesh : AssetManager::GetMeshList()) {
		vmaDestroyBuffer(GetAllocator(), mesh.m_transformBufferOLD.m_buffer, mesh.m_transformBufferOLD.m_allocation);
		vmaDestroyBuffer(GetAllocator(), mesh.m_vertexBufferOLD.m_buffer, mesh.m_vertexBufferOLD.m_allocation);
		if (mesh.m_indexCount > 0) {
			vmaDestroyBuffer(GetAllocator(), mesh.m_indexBufferOLD.m_buffer, mesh.m_indexBufferOLD.m_allocation);
		}

		//mesh.m_accelerationStructure.Cleanup();
	}
	vmaDestroyBuffer(GetAllocator(), _lineListMesh.m_vertexBufferOLD.m_buffer, _lineListMesh.m_vertexBufferOLD.m_allocation);

	VulkanDescriptorManager::Cleanup();
	VulkanCommandManager::Cleanup();
	VulkanSyncManager::Cleanup();
	VulkanSwapchainManager::Cleanup();
	VulkanMemoryManager::Cleanup();
	VulkanDeviceManager::Cleanup();
	VulkanInstanceManager::Cleanup();

	glfwDestroyWindow(_window);
	glfwTerminate();
}


void VulkanBackEnd::init_raytracing() {
	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& _rayTracingPipelineProperties = VulkanDeviceManager::GetRayTracingPipelineProperties();

	VulkanDescriptorSet& bindlessStaticSet = VulkanRenderer::GetStaticDescriptorSet();

	uint64_t id = VulkanRenderer::GetFrameDataByIndex(0).dynamicDescriptorSet;
	VulkanDescriptorSet* bindlessDynamicSet = VulkanResourceManager::GetDescriptorSet(id);

	std::vector<VkDescriptorSetLayout> rtDescriptorSetLayouts = {
		VulkanDescriptorManager::GetDynamicSetLayout(),
		VulkanDescriptorManager::GetStaticSetLayout(),
		VulkanDescriptorManager::GetSamplerSetLayout(),
		bindlessStaticSet.GetLayout(),
		bindlessDynamicSet->GetLayout()
	};

	_raytracerPath.CreatePipeline(GetDevice(), rtDescriptorSetLayouts, 5);
	_raytracerPath.CreateShaderBindingTable(GetDevice(), GetAllocator(), _rayTracingPipelineProperties);

	_raytracerMousePick.CreatePipeline(GetDevice(), rtDescriptorSetLayouts, 5);
	_raytracerMousePick.CreateShaderBindingTable(GetDevice(), GetAllocator(), _rayTracingPipelineProperties);
}


void VulkanBackEnd::cleanup_raytracing() {
	_raytracerPath.Cleanup(GetDevice(), GetAllocator());
	_raytracerMousePick.Cleanup(GetDevice(), GetAllocator());
}

void VulkanBackEnd::RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer) {
	AllocatedImage* loadingTarget = VulkanResourceManager::GetAllocatedImage("LoadingScreen");
	VulkanPipeline* textBlitterPipeline = VulkanPipelineManager::GetPipeline("TextBlitter");

	if (!loadingTarget) return;
	if (!textBlitterPipeline) return;

	uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();

	// Transition to the layout expected by the rendering info
	loadingTarget->TransitionLayout(
		commandBuffer,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);

	VkRenderingAttachmentInfo colorAttachment = {};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = loadingTarget->GetImageView();
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = { 0, 0, (uint32_t)loadingTarget->GetWidth(), (uint32_t)loadingTarget->GetHeight() };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	cmd_SetViewportSize(commandBuffer, loadingTarget->GetWidth(), loadingTarget->GetHeight());

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetHandle());

	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	HellDescriptorSet& staticSet = VulkanDescriptorManager::GetStaticDescriptorSet();


	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, &dynamicSet.handle, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 1, 1, &staticSet.handle, 0, nullptr);

	for (int i = 0; i < RasterRenderer::instanceCount; i++) {
		if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::MAIN_UI) {
			RasterRenderer::DrawMesh(commandBuffer, i);
		}
	}
	RasterRenderer::ClearQueue();

	vkCmdEndRendering(commandBuffer);
}




void VulkanBackEnd::PrepareSwapchainForPresent(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex) {
	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;
	VkImageMemoryBarrier swapChainBarrier = {};
	swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	swapChainBarrier.image = GetSwapchainImages()[swapchainImageIndex];
	swapChainBarrier.subresourceRange = range;
	swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	swapChainBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
}

void VulkanBackEnd::BlitAllocatedImageToSwapchain(VkCommandBuffer cmd, AllocatedImage& srcImage, uint32_t swapchainIndex) {
	int32_t windowWidth = GLFWIntegration::GetCurrentWindowWidth();
	int32_t windowHeight = GLFWIntegration::GetCurrentWindowHeight();

	// Prepare source image for blit
	srcImage.TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	// Prepare swapchain image for blit
	VkImage swapchainImage = GetSwapchainImages()[swapchainIndex];

	VkImageMemoryBarrier swapchainBarrier = {};
	swapchainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapchainBarrier.image = swapchainImage;
	swapchainBarrier.srcAccessMask = 0;
	swapchainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	swapchainBarrier.subresourceRange.baseMipLevel = 0;
	swapchainBarrier.subresourceRange.levelCount = 1;
	swapchainBarrier.subresourceRange.baseArrayLayer = 0;
	swapchainBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapchainBarrier);

	VkImageBlit blitRegion = {};
	// Source region
	blitRegion.srcOffsets[0] = { 0, 0, 0 };
	blitRegion.srcOffsets[1] = { srcImage.GetWidth(), srcImage.GetHeight(), 1 };
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;

	// Destination region (Swapchain)
	blitRegion.dstOffsets[0] = { 0, 0, 0 };
	blitRegion.dstOffsets[1] = { windowWidth, windowHeight, 1 };
	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.mipLevel = 0;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;

	vkCmdBlitImage(cmd, srcImage.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);
}

void VulkanBackEnd::RenderLoadingFrame() {
	if (ProgramIsMinimized()) {
		return;
	}

	AllocatedImage* loadingTarget = VulkanResourceManager::GetAllocatedImage("LoadingScreen");
	if (!loadingTarget) return;

	AddDebugText();
	TextBlitter::Update(GameData::GetDeltaTime(), loadingTarget->GetWidth(), loadingTarget->GetHeight());

	uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
	VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

	VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

	// Wait for the GPU to finish the last frame using this FrameData slot
	VulkanSyncManager::WaitForRenderFence(frameIndex);

	// Reset the fence before submitting new work
	VulkanSyncManager::ResetRenderFence(frameIndex);

	// Update 2D buffers and descriptor sets
	UpdateBuffers2D();

	// Retrieve the set from the Descriptor Manager and update it
	VulkanBuffer* uiMeshInstancesBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiInstances);

	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	dynamicSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uiMeshInstancesBuffer->GetBuffer());

	// Acquire next image from swapchain
	uint32_t swapchainImageIndex;
	VkSemaphore presentSemaphore = VulkanSyncManager::GetPresentSemaphore(frameIndex);
	VkResult result = vkAcquireNextImageKHR(GetDevice(), GetSwapchain(), UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		VulkanSwapchainManager::RecreateSwapchain();
		return;
	}

	// Record the loading frame commands
	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	RecordAssetLoadingRenderCommands(commandBuffer);

	// Blit the loading target and prepare for present
	BlitAllocatedImageToSwapchain(commandBuffer, *loadingTarget, swapchainImageIndex);
	PrepareSwapchainForPresent(commandBuffer, swapchainImageIndex);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	// Submit the command buffer
	VkSemaphore renderFinishedSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frameIndex, swapchainImageIndex);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &renderFinishedSemaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(GetGraphicsQueue(), 1, &submit, VulkanSyncManager::GetRenderFence(frameIndex)));

	// Present
	VkSwapchainKHR swapchain = GetSwapchain();
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

	result = vkQueuePresentKHR(GetGraphicsQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		VulkanSwapchainManager::RecreateSwapchain();
	}

	VulkanRenderer::IncrementFrame();
}


void VulkanBackEnd::RenderGameFrame() {
	if (ProgramIsMinimized()) return;

	AllocatedImage* presentAllocatedImage = VulkanResourceManager::GetAllocatedImage("Present");
	if (!presentAllocatedImage) return;

	TextBlitter::Update(GameData::GetDeltaTime(), presentAllocatedImage->GetWidth(), presentAllocatedImage->GetHeight());

	uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
	VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

	VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

	VulkanSyncManager::WaitForRenderFence(frameIndex);

	{
		VulkanRaytracingManager::CreateTopLevelAS(frameData.tlas.scene, Scene::GetMeshInstancesForSceneAccelerationStructure());
		VulkanRaytracingManager::CreateTopLevelAS(frameData.tlas.inventory, Scene::GetMeshInstancesForInventoryAccelerationStructure());

		get_required_lines();
		UpdateBuffers();
		UpdateBuffers2D();
		UpdateDynamicDescriptorSet();
	}

	VulkanSyncManager::ResetRenderFence(frameIndex);

	uint32_t swapchainImageIndex;
	VkSemaphore presentSemaphore = VulkanSyncManager::GetPresentSemaphore(frameIndex);
	VkResult result = vkAcquireNextImageKHR(GetDevice(), GetSwapchain(), UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		VulkanSwapchainManager::RecreateSwapchain();
		return;
	}

	// This now uses the command buffer from the manager internally
	build_rt_command_buffers(swapchainImageIndex);

	VkSemaphore renderFinishedSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frameIndex, swapchainImageIndex);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit = vkinit::submit_info(&commandBuffer);
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &renderFinishedSemaphore;

	VK_CHECK(vkQueueSubmit(GetGraphicsQueue(), 1, &submit, VulkanSyncManager::GetRenderFence(frameIndex)));

	VkSwapchainKHR swapchain = GetSwapchain();
	VkPresentInfoKHR presentInfo = vkinit::present_info();
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

	result = vkQueuePresentKHR(GetGraphicsQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		VulkanSwapchainManager::RecreateSwapchain();
	}

	if (!GameData::inventoryOpen) {
		VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(g_mousePickBufferCPU);
		if (buffer) {
			uint32_t mousePickResult[2];
			void* mappedData;
			buffer->Map(&mappedData);
			memcpy(mousePickResult, mappedData, sizeof(uint32_t) * 2);
			Scene::StoreMousePickResult(mousePickResult[0], mousePickResult[1]);
		}
	}
	else {
		Scene::StoreMousePickResult(-1, -1);
	}

	VulkanRenderer::IncrementFrame();
}


void VulkanBackEnd::ToggleFullscreen() {
    GLFWIntegration::ToggleFullscreen();
    VulkanSwapchainManager::RecreateSwapchain();
}


bool VulkanBackEnd::ProgramIsMinimized() {
    GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();

	int width, height;
	glfwGetFramebufferSize(_window, &width, &height);
	return (width == 0 || height == 0);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	Input::_mouseWheelValue = (int)yoffset;
}



void VulkanBackEnd::hotload_shaders()
{
	std::cout << "Hotloading shaders...\n";

	vkDeviceWaitIdle(GetDevice());

	_raytracerPath.Cleanup(GetDevice(), GetAllocator());
	_raytracerMousePick.Cleanup(GetDevice(), GetAllocator());

	LoadLegacyShaders();

	VulkanPipelineManager::ReloadAll();

	init_raytracing();
}


void VulkanBackEnd::LoadLegacyShaders() {
	// Path Tracer
	_raytracerPath.SetShaders("Path_RayGen", { "Path_Miss", "Path_Shadow" }, { "Path_Hit" });

	// Mouse Pick Tracer
	_raytracerMousePick.SetShaders("Mouse_RayGen", {"Mouse_Miss"}, {"Mouse_Hit"});
}


void VulkanBackEnd::upload_meshes() {
	for (MeshOLD& mesh : AssetManager::GetMeshList()) {
		if (!mesh.m_uploadedToGPU) {
			upload_mesh(mesh);
			mesh.m_vulkanAccelerationStructure = VulkanResourceManager::CreateAccelerationStructure();

			//VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(mesh.m_vulkanAccelerationStructure);
			//accelerationStructure->CreateBLAS(mesh.m_vertexBuffer)

			VulkanRaytracingManager::CreateBottomLevelAS(mesh.m_vulkanAccelerationStructure , &mesh);
		}
	}
    std::cout << "uploaded meshes\n";
}

void VulkanBackEnd::upload_mesh(MeshOLD& mesh)
{
	// Vertices
	{
		const size_t bufferSize = mesh.m_vertexCount * sizeof(Vertex);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBufferOLD stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer.m_buffer, &stagingBuffer.m_allocation, nullptr));

		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer.m_allocation, &data);
		memcpy(data, AssetManager::GetVertexPointer(mesh.m_vertexOffset), mesh.m_vertexCount * sizeof(Vertex));
		vmaUnmapMemory(GetAllocator(), stagingBuffer.m_allocation);

		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;
		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage =
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &mesh.m_vertexBufferOLD.m_buffer, &mesh.m_vertexBufferOLD.m_allocation, nullptr));

		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, mesh.m_vertexBufferOLD.m_buffer, 1, &copy);
			});

		vmaDestroyBuffer(GetAllocator(), stagingBuffer.m_buffer, stagingBuffer.m_allocation);
	}

	// Indices
	if (mesh.m_indexCount > 0)
	{
		const size_t bufferSize = mesh.m_indexCount * sizeof(uint32_t);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBufferOLD stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer.m_buffer, &stagingBuffer.m_allocation, nullptr));

		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer.m_allocation, &data);

		memcpy(data, AssetManager::GetIndexPointer(mesh.m_indexOffset), mesh.m_indexCount * sizeof(uint32_t));
		vmaUnmapMemory(GetAllocator(), stagingBuffer.m_allocation);

		VkBufferCreateInfo indexBufferInfo = {};
		indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexBufferInfo.pNext = nullptr;
		indexBufferInfo.size = bufferSize;
		indexBufferInfo.usage =
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VK_CHECK(vmaCreateBuffer(GetAllocator(), &indexBufferInfo, &vmaallocInfo, &mesh.m_indexBufferOLD.m_buffer, &mesh.m_indexBufferOLD.m_allocation, nullptr));

		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, mesh.m_indexBufferOLD.m_buffer, 1, &copy);
			});

		vmaDestroyBuffer(GetAllocator(), stagingBuffer.m_buffer, stagingBuffer.m_allocation);
	}
	// Transforms
	{
		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f
		};

		const size_t bufferSize = sizeof(transformMatrix);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		AllocatedBufferOLD stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer.m_buffer, &stagingBuffer.m_allocation, nullptr));
		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer.m_allocation, &data);
		memcpy(data, &transformMatrix, bufferSize);
		vmaUnmapMemory(GetAllocator(), stagingBuffer.m_allocation);
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &mesh.m_transformBufferOLD.m_buffer, &mesh.m_transformBufferOLD.m_allocation, nullptr));

		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, mesh.m_transformBufferOLD.m_buffer, 1, &copy);
			});

		vmaDestroyBuffer(GetAllocator(), stagingBuffer.m_buffer, stagingBuffer.m_allocation);
	}
	mesh.m_uploadedToGPU = true;
}

AllocatedBufferOLD VulkanBackEnd::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;


	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.requiredFlags = requiredFlags;

	AllocatedBufferOLD newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo,
		&newBuffer.m_buffer,
		&newBuffer.m_allocation,
		nullptr));

	return newBuffer;
}

void VulkanBackEnd::add_debug_name(VkBuffer buffer, const char* name) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
	nameInfo.objectHandle = (uint64_t)buffer;
	nameInfo.pObjectName = name;
	vkSetDebugUtilsObjectNameEXT(GetDevice(), &nameInfo);
}

void VulkanBackEnd::add_debug_name(VkDescriptorSetLayout descriptorSetLayout, const char* name) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	nameInfo.objectHandle = (uint64_t)descriptorSetLayout;
	nameInfo.pObjectName = name;
	vkSetDebugUtilsObjectNameEXT(GetDevice(), &nameInfo);
}


void VulkanBackEnd::create_rt_buffers() {
	// Get the raw geometry data from the asset manager
	std::vector<Vertex>& vertices = AssetManager::GetVertices_TEMPORARY();
	std::vector<uint32_t>& indices = AssetManager::GetIndices_TEMPORARY();

	const std::vector<Vertex>& vertices2 = AssetManager::GetVertices();
	const std::vector<uint32_t>& indices2 = AssetManager::GetIndices();

	std::cout << "vertices: " << vertices.size() << " " << vertices2.size() << "\n";
	std::cout << "indices:  " << indices.size() << " " << indices2.size() << "\n";

	// Define flags for raytracing geometry buffers
	// These allow the buffer to be used for AS builds and as a storage buffer in shaders
	VkBufferUsageFlags rtGeometryUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

	// Create the high-level VulkanBuffer objects via the resource manager
	// These are GPU_ONLY for optimal traversal performance
	g_vertexBuffer = VulkanResourceManager::CreateBuffer(vertices.size() * sizeof(Vertex), rtGeometryUsage, VMA_MEMORY_USAGE_GPU_ONLY);
	g_indexBuffer = VulkanResourceManager::CreateBuffer(indices.size() * sizeof(uint32_t), rtGeometryUsage, VMA_MEMORY_USAGE_GPU_ONLY);

	// Upload the data using the internal staging logic
	// This handles the creation and destruction of the temporary staging buffer automatically
	VulkanResourceManager::UploadBufferData(g_vertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex));
	VulkanResourceManager::UploadBufferData(g_indexBuffer, indices.data(), indices.size() * sizeof(uint32_t));

	// Refactored Mouse Pick Buffers
	VkDeviceSize pickBufferSize = sizeof(uint32_t) * 2;

	// GPU buffer for the raytracing shader to write into
	g_mousePickBufferGPU = VulkanResourceManager::CreateBuffer(pickBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO);

	// CPU buffer for the backend to read from
	// MAPPED and HOST_ACCESS_SEQUENTIAL_WRITE to keep the pointer valid permanently
	g_mousePickBufferCPU = VulkanResourceManager::CreateBuffer(pickBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
}


void VulkanBackEnd::UpdateBuffers2D() {

	// Queue all text characters for rendering
	int quadMeshIndex = AssetManager::GetModel("blitter_quad")->m_meshIndices[0];
	for (auto& instanceInfo : TextBlitter::_objectData) {
			RasterRenderer::SubmitUI(quadMeshIndex, instanceInfo.index_basecolor, instanceInfo.index_color, instanceInfo.modelMatrix, RasterRenderer::Destination::MAIN_UI, instanceInfo.xClipMin, instanceInfo.xClipMax, instanceInfo.yClipMin, instanceInfo.yClipMax); // Todo: You are storing color in the normals. Probably not a major deal but could be confusing at some point down the line.
	}

	if (_loaded) {
		// Add the crosshair
		if (!GameData::inventoryOpen && GameData::GetPlayer().m_camera._state != Camera::State::USING_LAPTOP) {

			std::string cursor = "CrosshairDot";

			// Interactable?
			if (Scene::_hoveredGameObject) {
				if (Scene::_hoveredGameObject->IsInteractable() || Scene::_hoveredGameObject->_interactAffectsThisObjectInstead != "") {
					cursor = "CrosshairSquare";
				}
			}
			// Draw it
			RasterRenderer::DrawQuad(cursor, 512 / 2, 288 / 2, RasterRenderer::Destination::MAIN_UI, true);
		}

		// Laptop UI
		Laptop::PrepareUIForRaster();
	}

	// 2D instance data
	VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiInstances)) {
		size_t instanceCount = RasterRenderer::instanceCount;
		std::vector<MeshInstance> meshInstances = Scene::GetSceneMeshInstances(_debugScene);
		buffer->UpdateData(RasterRenderer::_instanceData2D, sizeof(GPUObjectData2D) * instanceCount);
	}
}

void VulkanBackEnd::UpdateBuffers() {
	VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.sceneCameraData)) {
		CameraData camData;
		camData.proj = GameData::GetProjectionMatrix();
		camData.view = GameData::GetViewMatrix();
		camData.projInverse = glm::inverse(camData.proj);
		camData.viewInverse = glm::inverse(camData.view);
		camData.viewPos = glm::vec4(GameData::GetCameraPosition(), 1.0f);
		camData.vertexSize = sizeof(Vertex);
		camData.frameIndex = _frameIndex;
		camData.inventoryOpen = (GameData::inventoryOpen) ? 1 : 0;

		buffer->UpdateData(&camData, sizeof(CameraData));
	}

	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryCameraData)) {
		Transform cameraTransform;
		cameraTransform.position = glm::vec3(0, -1.125, -1);
		cameraTransform.position = glm::vec3(0, 0, -1);

		Camera camera;
		camera.m_transform = cameraTransform;
		camera.m_viewMatrix = camera.m_transform.to_mat4();
		camera.m_inverseViewMatrix = glm::inverse(camera.m_viewMatrix);
		camera.m_viewPos = camera.m_inverseViewMatrix[3];

		CameraData inventoryCamData;
		inventoryCamData.proj = glm::perspective(1.0f, (float)512 / (float)288, NEAR_PLANE, FAR_PLANE);
		inventoryCamData.proj[1][1] *= -1;
		inventoryCamData.view = camera.m_viewMatrix;
		inventoryCamData.projInverse = glm::inverse(inventoryCamData.proj);
		inventoryCamData.viewInverse = glm::inverse(inventoryCamData.view);
		inventoryCamData.viewPos = glm::vec4(camera.m_viewPos, 1.0f);
		inventoryCamData.vertexSize = sizeof(Vertex);
		inventoryCamData.frameIndex = _frameIndex++;
		inventoryCamData.inventoryOpen = 2; // 2 is actually inventory render
		inventoryCamData.wallPaperALBIndex = AssetManager::GetTextureIndex("WallPaper_ALB");

		buffer->UpdateData(&inventoryCamData, sizeof(CameraData));
	}

	// 3D instance data
	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.sceneInstances)) {
		std::vector<MeshInstance> meshInstances = Scene::GetSceneMeshInstances(_debugScene);
		buffer->UpdateData(meshInstances.data(), sizeof(MeshInstance) * meshInstances.size());
	}

	// 3D instance data
	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryInstances)) {
		std::vector<MeshInstance> inventoryMeshInstances = Scene::GetInventoryMeshInstances(_debugScene);
		buffer->UpdateData(inventoryMeshInstances.data(), sizeof(MeshInstance) * inventoryMeshInstances.size());
	}

	// Light render info
	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.sceneLights)) {
		std::vector<LightRenderInfo> lightRenderInfo = Scene::GetLightRenderInfo();
		buffer->UpdateData(lightRenderInfo.data(), sizeof(LightRenderInfo) * lightRenderInfo.size());
	}
	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryLights)) {
		std::vector<LightRenderInfo> lightRenderInfo = Scene::GetLightRenderInfoInventory();
		buffer->UpdateData(lightRenderInfo.data(), sizeof(LightRenderInfo) * lightRenderInfo.size());
	}
}

void VulkanBackEnd::update_static_descriptor_set_old() {
	AllocatedImage* rtFirstHitColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Color");
	AllocatedImage* rtFirstHitNormalsAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Normals");
	AllocatedImage* rtFirstHitBaseColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_BaseColor");
	AllocatedImage* rtSecondHitColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_SecondHit_Color");
	AllocatedImage* gBufferNormalAllocatedImage = VulkanResourceManager::GetAllocatedImage("GBuffer_Normal");
	AllocatedImage* gBufferRmaAllocatedImage = VulkanResourceManager::GetAllocatedImage("GBuffer_RMA");
	AllocatedImage* laptopDisplayAllocatedImage = VulkanResourceManager::GetAllocatedImage("LaptopDisplay");
	AllocatedImage* presentAllocatedImage = VulkanResourceManager::GetAllocatedImage("Present");
	AllocatedImage* depthGBufferAllocatedImage = VulkanResourceManager::GetAllocatedImage("Depth_GBuffer");

	HellDescriptorSet& samplerSet = VulkanDescriptorManager::GetSamplerDescriptorSet();
	//VulkanDescriptorSet& bindlessSet = VulkanRenderer::GetStaticDescriptorSet();
	HellDescriptorSet& legacySet = VulkanDescriptorManager::GetStaticDescriptorSet();

	VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");
	if (!linearSampler) return;

	// Binding 0: The Sampler
	VkDescriptorImageInfo samplerImageInfo = linearSampler->GetDescriptorInfo();
	legacySet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);

	// Binding 1: All Textures
	VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
		VkImageView view = (i < AssetManager::GetNumberOfTextures()) ? AssetManager::GetTexture(i)->imageView : AssetManager::GetTexture(0)->imageView;
		textureImageInfo[i].sampler = nullptr;
		textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureImageInfo[i].imageView = view;

		//bindlessSet.WriteImage(DESC_IDX_TEXTURES, view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
	}
	legacySet.Update(GetDevice(), 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

	VulkanBuffer* vertexBuffer = VulkanResourceManager::GetBuffer(g_vertexBuffer);
	VulkanBuffer* indexBuffer = VulkanResourceManager::GetBuffer(g_indexBuffer);

	legacySet.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBuffer->GetBuffer());
	legacySet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBuffer->GetBuffer());

	//bindlessSet.WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER);
	//bindlessSet.WriteBuffer(DESC_IDX_VERTICES, vertexBuffer->GetBuffer(), vertexBuffer->GetSize(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	//bindlessSet.WriteBuffer(DESC_IDX_INDICES, indexBuffer->GetBuffer(), indexBuffer->GetSize(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	//
	//// Append Render Targets to the texture array so you can read from them bindlessly
	//
	//bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtFirstHitColorAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DESC_OFFSET_RENDER_TARGET + 0);
	//bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtFirstHitNormalsAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DESC_OFFSET_RENDER_TARGET + 1);
	//bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtFirstHitBaseColorAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DESC_OFFSET_RENDER_TARGET + 2);

	
	// Raytracing storage images
	VkDescriptorImageInfo storageImageDescriptor{};
	storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	// Binding 4: RT First Hit Color
	storageImageDescriptor.imageView = rtFirstHitColorAllocatedImage->GetImageView();
	legacySet.Update(GetDevice(), 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
	//bindlessSet.WriteImage(4, rtFirstHitColorAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	// Binding 5: Mouse pick buffer
	if (VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(g_mousePickBufferGPU)) {
		legacySet.Update(GetDevice(), 5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer->GetBuffer());
		//bindlessSet.WriteBuffer(5, buffer->GetBuffer(), buffer->GetSize(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	// Binding 6: RT First Hit Normals
	storageImageDescriptor.imageView = rtFirstHitNormalsAllocatedImage->GetImageView();
	legacySet.Update(GetDevice(), 6, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
	//bindlessSet.WriteImage(6, rtFirstHitNormalsAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	// Binding 7: RT First Hit BaseColor
	storageImageDescriptor.imageView = rtFirstHitBaseColorAllocatedImage->GetImageView();
	legacySet.Update(GetDevice(), 7, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
	//bindlessSet.WriteImage(7, rtFirstHitBaseColorAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	// Binding 8: RT Second Hit Color
	storageImageDescriptor.imageView = rtSecondHitColorAllocatedImage->GetImageView();
	legacySet.Update(GetDevice(), 8, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
	//bindlessSet.WriteImage(8, rtSecondHitColorAllocatedImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	// Flush all staged writes to the GPU
	//bindlessSet.Update();

	// Transition layouts for General layout requirements
	VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
		laptopDisplayAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		gBufferNormalAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		gBufferRmaAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		depthGBufferAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		});

	// Update Sampler Sets (Traditional Combined Samplers)
	VkDescriptorImageInfo staticSamplerImageInfo = linearSampler->GetDescriptorInfo();
	staticSamplerImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	staticSamplerImageInfo.imageView = rtFirstHitColorAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &staticSamplerImageInfo);

	staticSamplerImageInfo.imageView = rtFirstHitNormalsAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &staticSamplerImageInfo);

	staticSamplerImageInfo.imageView = rtFirstHitBaseColorAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &staticSamplerImageInfo);

	staticSamplerImageInfo.imageView = rtSecondHitColorAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &staticSamplerImageInfo);

	staticSamplerImageInfo.imageView = laptopDisplayAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 7, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &staticSamplerImageInfo);
}

void VulkanBackEnd::UpdateStaticDescriptorSet() {
	AllocatedImage* rtFirstHitColor = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Color");
	AllocatedImage* rtFirstHitNormals = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Normals");
	AllocatedImage* rtFirstHitBaseColor = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_BaseColor");
	AllocatedImage* rtSecondHitColor = VulkanResourceManager::GetAllocatedImage("RT_SecondHit_Color");
	AllocatedImage* gBufferBaseColor = VulkanResourceManager::GetAllocatedImage("GBuffer_BaseColor");
	AllocatedImage* gBufferNormal = VulkanResourceManager::GetAllocatedImage("GBuffer_Normal");
	AllocatedImage* gBufferRma = VulkanResourceManager::GetAllocatedImage("GBuffer_RMA");
	AllocatedImage* laptopDisplay = VulkanResourceManager::GetAllocatedImage("LaptopDisplay");
	AllocatedImage* composite = VulkanResourceManager::GetAllocatedImage("Composite");
	AllocatedImage* present = VulkanResourceManager::GetAllocatedImage("Present");
	AllocatedImage* loadingScreen = VulkanResourceManager::GetAllocatedImage("LoadingScreen");
	AllocatedImage* depthPresent = VulkanResourceManager::GetAllocatedImage("Depth_Present");
	AllocatedImage* depthGBuffer = VulkanResourceManager::GetAllocatedImage("Depth_GBuffer");

	VulkanDescriptorSet& bindlessSet = VulkanRenderer::GetStaticDescriptorSet();
	VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");
	if (!linearSampler) return;
	
	// Samplers
	bindlessSet.WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER);

	// // Textures
	uint32_t assetTextureCount = AssetManager::GetNumberOfTextures();
	for (uint32_t i = 0; i < assetTextureCount; ++i) {
		VkImageView view = AssetManager::GetTexture(i)->imageView;
		bindlessSet.WriteImage(DESC_IDX_TEXTURES, view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
	}

	// Render Targets
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtFirstHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_FIRST_HIT_COLOR);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtFirstHitNormals->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_FIRST_HIT_NORMALS);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtFirstHitBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_FIRST_HIT_BASE);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, rtSecondHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_SECOND_HIT_COLOR);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, gBufferBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_GBUFFER_BASE);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, gBufferNormal->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_GBUFFER_NORMAL);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, gBufferRma->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_GBUFFER_RMA);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, laptopDisplay->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_LAPTOP);
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, composite->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_COMPOSITE);

	// Depth targets use specific depth layout
	bindlessSet.WriteImage(DESC_IDX_TEXTURES, depthGBuffer->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_DEPTH_GBUFFER);

	// Storage Images RGBA32F
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtFirstHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_FIRST_COLOR);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtFirstHitNormals->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_FIRST_NORMALS);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtFirstHitBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_FIRST_BASE);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtSecondHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_SECOND_COLOR);

	// Storage Images RGBA16F

	// Storage Images RGBA8
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, gBufferBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_GBUFFER_BASE);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, gBufferNormal->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_GBUFFER_NORMAL);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, gBufferRma->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_GBUFFER_RMA);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, laptopDisplay->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_LAPTOP);
	bindlessSet.WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, composite->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_COMPOSITE);

	// REMOVE ME WHEN YOU CAN!!! or maybe Keep me in here??? and only use the SceneData variant for raytracing shaders.. but why else would u ever want to access these?
	VulkanBuffer* vertexBuffer = VulkanResourceManager::GetBuffer(g_vertexBuffer);
	VulkanBuffer* indexBuffer = VulkanResourceManager::GetBuffer(g_indexBuffer);
	if (vertexBuffer && indexBuffer) {
		bindlessSet.WriteBuffer(DESC_IDX_VERTICES, vertexBuffer->GetBuffer(), vertexBuffer->GetSize(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		bindlessSet.WriteBuffer(DESC_IDX_INDICES, indexBuffer->GetBuffer(), indexBuffer->GetSize(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	// Finalize all writes
	bindlessSet.Update();
}

void VulkanBackEnd::UpdateDynamicDescriptorSet() {
	uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
	VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

	VulkanAccelerationStructure* sceneTlas = VulkanResourceManager::GetAccelerationStructure(frameData.tlas.scene);
	VulkanAccelerationStructure* inventoryTLAS = VulkanResourceManager::GetAccelerationStructure(frameData.tlas.inventory);

	VulkanBuffer* sceneCameraDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.sceneCameraData);
	VulkanBuffer* sceneInstancesBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.sceneInstances);
	VulkanBuffer* sceneLightsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.sceneLights);

	VulkanBuffer* inventoryCameraDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryCameraData);
	VulkanBuffer* inventoryInstancesBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryInstances);
	VulkanBuffer* inventoryLightsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryLights);

	VulkanBuffer* uiInstancesBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiInstances);

	// Get the per-frame sets from the manager
	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	HellDescriptorSet& dynamicSetInventory = VulkanDescriptorManager::GetDynamicInventoryDescriptorSet(frameIndex);

	// Update Dynamic Inventory Set
	dynamicSetInventory.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &inventoryTLAS->m_handle);
	dynamicSetInventory.Update(GetDevice(), 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, inventoryCameraDataBuffer->GetBuffer());
	dynamicSetInventory.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, inventoryInstancesBuffer->GetBuffer());
	dynamicSetInventory.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uiInstancesBuffer->GetBuffer());
	dynamicSetInventory.Update(GetDevice(), 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, inventoryLightsBuffer->GetBuffer());

	// Update Dynamic Scene Set
	dynamicSet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &sceneTlas->m_handle);
	dynamicSet.Update(GetDevice(), 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sceneCameraDataBuffer->GetBuffer());
	dynamicSet.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sceneInstancesBuffer->GetBuffer());
	dynamicSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uiInstancesBuffer->GetBuffer());
	dynamicSet.Update(GetDevice(), 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sceneLightsBuffer->GetBuffer());






	VulkanRenderer::UpdateDynamicDescriptorSet();
}

void VulkanBackEnd::build_rt_command_buffers(int swapchainIndex) {
	uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
	VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

	VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

	AllocatedImage* rtFirstHitColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Color");
	AllocatedImage* rtFirstHitNormalsAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Normals");
	AllocatedImage* rtFirstHitBaseColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_BaseColor");
	AllocatedImage* rtSecondHitColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_SecondHit_Color");
	AllocatedImage* laptopDisplayAllocatedImage = VulkanResourceManager::GetAllocatedImage("LaptopDisplay");
	AllocatedImage* compositeAllocatedImage = VulkanResourceManager::GetAllocatedImage("Composite");
	AllocatedImage* presentAllocatedImage = VulkanResourceManager::GetAllocatedImage("Present");

	VulkanPipeline* compositePipeline = VulkanPipelineManager::GetPipeline("Composite");
	VulkanPipeline* linesPipeline = VulkanPipelineManager::GetPipeline("Lines");
	VulkanPipeline* textBlitterPipeline = VulkanPipelineManager::GetPipeline("TextBlitter");


	if (!linesPipeline) return;
	if (!compositePipeline) return;

	uint32_t currentWindowWidth = GLFWIntegration::GetCurrentWindowWidth();
	uint32_t currentWindowHeight = GLFWIntegration::GetCurrentWindowHeight();


	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
	VkCommandBufferBeginInfo cmdBufInfo = vkinit::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	HellDescriptorSet& dynamicSetInventory = VulkanDescriptorManager::GetDynamicInventoryDescriptorSet(frameIndex);
	HellDescriptorSet& staticSet = VulkanDescriptorManager::GetStaticDescriptorSet();
	HellDescriptorSet& samplerSet = VulkanDescriptorManager::GetSamplerDescriptorSet();



	VulkanDescriptorSet& bindlessStaticSet = VulkanRenderer::GetStaticDescriptorSet();
	VulkanDescriptorSet* dynamicDescriptorSet = VulkanResourceManager::GetDescriptorSet(frameData.dynamicDescriptorSet);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracerPath.pipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracerPath.pipelineLayout, 0, 1, &dynamicSet.handle, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracerPath.pipelineLayout, 1, 1, &staticSet.handle, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracerPath.pipelineLayout, 2, 1, &samplerSet.handle, 0, nullptr);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracerPath.pipelineLayout, 3, 1, bindlessStaticSet.GetHandlePtr(), 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracerPath.pipelineLayout, 4, 1, dynamicDescriptorSet->GetHandlePtr(), 0, nullptr);

	// Transition RT images to GENERAL for storage write
	rtFirstHitColorAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
	rtFirstHitNormalsAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
	rtSecondHitColorAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
	rtFirstHitBaseColorAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	vkCmdTraceRaysKHR(commandBuffer, &_raytracerPath.raygenShaderSbtEntry, &_raytracerPath.missShaderSbtEntry, &_raytracerPath.hitShaderSbtEntry, &_raytracerPath.callableShaderSbtEntry, rtFirstHitColorAllocatedImage->GetWidth(), rtFirstHitColorAllocatedImage->GetHeight(), 1);

	if (GameData::inventoryOpen) {
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 0, dynamicSetInventory);
		vkCmdTraceRaysKHR(commandBuffer, &_raytracerPath.raygenShaderSbtEntry, &_raytracerPath.missShaderSbtEntry, &_raytracerPath.hitShaderSbtEntry, &_raytracerPath.callableShaderSbtEntry, rtFirstHitColorAllocatedImage->GetWidth(), rtFirstHitColorAllocatedImage->GetHeight(), 1);
	}



	// Mouse pick
	cmd_BindRayTracingPipeline(commandBuffer, _raytracerMousePick.pipeline);
	cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerMousePick.pipelineLayout, 0, dynamicSet);
	cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerMousePick.pipelineLayout, 1, staticSet);
	vkCmdTraceRaysKHR(commandBuffer, &_raytracerMousePick.raygenShaderSbtEntry, &_raytracerMousePick.missShaderSbtEntry, &_raytracerMousePick.hitShaderSbtEntry, &_raytracerMousePick.callableShaderSbtEntry, 1, 1, 1);

	// Laptop display rendering
	{
		Texture* bg_texture = AssetManager::GetTexture("OS_bg");
		if (bg_texture) {
			bg_texture->insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			laptopDisplayAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			
			VkImageBlit blitRegion{};
			blitRegion.srcOffsets[1] = { (int32_t)bg_texture->_width, (int32_t)bg_texture->_height, 1 };
			blitRegion.dstOffsets[1] = { (int32_t)laptopDisplayAllocatedImage->GetWidth(), (int32_t)laptopDisplayAllocatedImage->GetHeight(), 1 };
			blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			vkCmdBlitImage(commandBuffer, bg_texture->image._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, laptopDisplayAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);

			bg_texture->insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			// Transition to ATTACHMENT_OPTIMAL for Rendering
			laptopDisplayAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

			VkRenderingAttachmentInfo laptopColorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			laptopColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			laptopColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			laptopColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			laptopColorAttachment.imageView = laptopDisplayAllocatedImage->GetImageView();

			VkRenderingInfo laptopRenderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
			laptopRenderingInfo.renderArea = { 0, 0, (uint32_t)laptopDisplayAllocatedImage->GetWidth(), (uint32_t)laptopDisplayAllocatedImage->GetHeight() };
			laptopRenderingInfo.layerCount = 1;
			laptopRenderingInfo.colorAttachmentCount = 1;
			laptopRenderingInfo.pColorAttachments = &laptopColorAttachment;

			vkCmdBeginRendering(commandBuffer, &laptopRenderingInfo);
			cmd_SetViewportSize(commandBuffer, laptopDisplayAllocatedImage->GetWidth(), laptopDisplayAllocatedImage->GetHeight());

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetHandle());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, &dynamicSet.handle, 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 1, 1, &staticSet.handle, 0, nullptr);

			//cmd_BindPipeline(commandBuffer, _pipelines.textBlitter);
			//cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, dynamicSet);
			//cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, staticSet);
			for (int i = 0; i < RasterRenderer::instanceCount; i++) {
				if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::LAPTOP_DISPLAY)
					RasterRenderer::DrawMesh(commandBuffer, i);
			}
			vkCmdEndRendering(commandBuffer);

			laptopDisplayAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

		}
	}

	// Composite Pass
	{
		compositeAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		VkRenderingAttachmentInfo compositeAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		compositeAttachment.imageView = compositeAllocatedImage->GetImageView();
		compositeAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		compositeAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		compositeAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		compositeAttachment.clearValue = { 0.2f, 1.0f, 0.0f, 0.0f };

		VkRenderingInfo compositeRenderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
		compositeRenderingInfo.renderArea = { 0, 0, (uint32_t)compositeAllocatedImage->GetWidth(), (uint32_t)compositeAllocatedImage->GetHeight() };
		compositeRenderingInfo.layerCount = 1;
		compositeRenderingInfo.colorAttachmentCount = 1;
		compositeRenderingInfo.pColorAttachments = &compositeAttachment;

		vkCmdBeginRendering(commandBuffer, &compositeRenderingInfo);
		cmd_SetViewportSize(commandBuffer, compositeAllocatedImage->GetWidth(), compositeAllocatedImage->GetHeight());

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline->GetHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline->GetLayout(), 0, 1, &dynamicSet.handle, 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline->GetLayout(), 1, 1, &staticSet.handle, 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline->GetLayout(), 2, 1, &samplerSet.handle, 0, nullptr);

		//cmd_BindPipeline(commandBuffer, _pipelines.composite);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 0, dynamicSet);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 1, staticSet);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 2, samplerSet);
		AssetManager::GetMesh(AssetManager::GetModel("fullscreen_quad")->m_meshIndices[0])->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);

		// Blit Composite to Present
		compositeAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		presentAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkImageBlit blitRegion{};
		blitRegion.srcOffsets[1] = { (int32_t)compositeAllocatedImage->GetWidth(), (int32_t)compositeAllocatedImage->GetHeight(), 1 };
		blitRegion.dstOffsets[1] = { (int32_t)presentAllocatedImage->GetWidth(), (int32_t)presentAllocatedImage->GetHeight(), 1 };
		blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		vkCmdBlitImage(commandBuffer, compositeAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, presentAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);
	}

	// Main UI Pass
	{
		presentAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		VkRenderingAttachmentInfo uiAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		uiAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		uiAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		uiAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		uiAttachment.imageView = presentAllocatedImage->GetImageView();

		VkRenderingInfo uiRenderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
		uiRenderingInfo.renderArea = { 0, 0, (uint32_t)presentAllocatedImage->GetWidth(), (uint32_t)presentAllocatedImage->GetHeight() };
		uiRenderingInfo.layerCount = 1;
		uiRenderingInfo.colorAttachmentCount = 1;
		uiRenderingInfo.pColorAttachments = &uiAttachment;

		vkCmdBeginRendering(commandBuffer, &uiRenderingInfo);
		cmd_SetViewportSize(commandBuffer, presentAllocatedImage->GetWidth(), presentAllocatedImage->GetHeight());
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, &dynamicSet.handle, 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 1, 1, &staticSet.handle, 0, nullptr);

		for (int i = 0; i < RasterRenderer::instanceCount; i++) {
			if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::MAIN_UI)
				RasterRenderer::DrawMesh(commandBuffer, i);
		}
		RasterRenderer::ClearQueue();

		if (_lineListMesh.m_vertexCount > 0 && _debugMode != DebugMode::NONE) {

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linesPipeline->GetHandle());

			glm::mat4 projection = glm::perspective(GameData::_cameraZoom, 1700.f / 900.f, 0.01f, 100.0f);
			glm::mat4 view = GameData::GetPlayer().m_camera.GetViewMatrix();
			projection[1][1] *= -1;
			LineShaderPushConstants constants;
			constants.transformation = projection * view;
			vkCmdPushConstants(commandBuffer, linesPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LineShaderPushConstants), &constants);
			_lineListMesh.draw(commandBuffer, 0);
		}
		vkCmdEndRendering(commandBuffer);
	}

	// Final Swapchain Blit
	{
		presentAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkImageMemoryBarrier swapChainBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapChainBarrier.image = GetSwapchainImages()[swapchainIndex];
		swapChainBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		swapChainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);

		VkImageBlit blitRegion{};
		blitRegion.srcOffsets[1] = { (int32_t)presentAllocatedImage->GetWidth(), (int32_t)presentAllocatedImage->GetHeight(), 1 };
		blitRegion.dstOffsets[1] = { (int32_t)currentWindowWidth, (int32_t)currentWindowHeight, 1 };
		blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		vkCmdBlitImage(commandBuffer, presentAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, GetSwapchainImages()[swapchainIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);

		swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		swapChainBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
	}

	if (!GameData::inventoryOpen) {
		VulkanBuffer* gpuBuffer = VulkanResourceManager::GetBuffer(g_mousePickBufferGPU);
		VulkanBuffer* cpuBuffer = VulkanResourceManager::GetBuffer(g_mousePickBufferCPU);

		VkBufferCopy pickCopy = { 0, 0, sizeof(uint32_t) * 2 };
		vkCmdCopyBuffer(commandBuffer, gpuBuffer->GetBuffer(), cpuBuffer->GetBuffer(), 1, &pickCopy);
	}

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void VulkanBackEnd::AddDebugText() {


	TextBlitter::ResetDebugText();

	if (!_loaded) {
		int begin = std::max(0, (int)_loadingText.size() - 36);
		for (int i = begin; i < _loadingText.size(); i++) {
			TextBlitter::AddDebugText(_loadingText[i]);
		}
	}

	if (_debugMode == DebugMode::RAY) {
		TextBlitter::AddDebugText("Cam pos: " + Util::Vec3ToString(GameData::GetPlayer().m_camera.m_viewPos));
		TextBlitter::AddDebugText("Cam rot: " + Util::Vec3ToString(GameData::GetPlayer().m_camera.m_transform.rotation));
		TextBlitter::AddDebugText("Rayhit BLAS index: " + std::to_string(Scene::_instanceIndex));
		TextBlitter::AddDebugText("Rayhit triangle index: " + std::to_string(Scene::_primitiveIndex));
	}	
	else if (_debugMode == DebugMode::COLLISION) {
		TextBlitter::AddDebugText("Collision world");
	}
	else if (false) {
		//return;
		TextBlitter::AddDebugText("Inventory");
		for (int i=0; i < GameData::GetInventoryItemCount(); i++)
			TextBlitter::AddDebugText("[g]" + GameData::GetInventoryItemNameByIndex(i, true) + "[w]");
	}
}

void VulkanBackEnd::get_required_lines() {

	// Generate buffer shit
	static bool runOnce = true;
	if (runOnce) {
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;
		vertexBufferInfo.size = sizeof(Vertex) * 4096; // number of max lines possible
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &_lineListMesh.m_vertexBufferOLD.m_buffer, &_lineListMesh.m_vertexBufferOLD.m_allocation, nullptr));
		add_debug_name(_lineListMesh.m_vertexBufferOLD.m_buffer, "_lineListMesh._vertexBuffer");
		// Name the mesh
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		nameInfo.objectHandle = (uint64_t)_lineListMesh.m_vertexBufferOLD.m_buffer;
		nameInfo.pObjectName = "Line list mesh";
		vkSetDebugUtilsObjectNameEXT(GetDevice(), &nameInfo);
		runOnce = false;
	}

	std::vector<Vertex> vertices;

	// Ray cast
	if (_debugMode == DebugMode::RAY) {
		if (Scene::_hitTriangleVertices.size() == 3) {
			vertices.push_back(Scene::_hitTriangleVertices[0]);
			vertices.push_back(Scene::_hitTriangleVertices[1]);
			vertices.push_back(Scene::_hitTriangleVertices[1]);
			vertices.push_back(Scene::_hitTriangleVertices[2]);
			vertices.push_back(Scene::_hitTriangleVertices[2]);
			vertices.push_back(Scene::_hitTriangleVertices[0]);
		}
	}
	// Collision world
	else if (_debugMode == DebugMode::COLLISION) {
		vertices = Scene::GetCollisionLineVertices();
	}

	_lineListMesh.m_vertexCount = vertices.size();

	if (vertices.size()) {
		const size_t bufferSize = vertices.size() * sizeof(Vertex);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		AllocatedBufferOLD stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer.m_buffer, &stagingBuffer.m_allocation, nullptr));
		add_debug_name(stagingBuffer.m_buffer, "stagingBuffer");
		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer.m_allocation, &data);
		memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
		vmaUnmapMemory(GetAllocator(), stagingBuffer.m_allocation);

		// Use the Command Manager to submit the transfer
		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, _lineListMesh.m_vertexBufferOLD.m_buffer, 1, &copy);
			});

		vmaDestroyBuffer(GetAllocator(), stagingBuffer.m_buffer, stagingBuffer.m_allocation);
	}
}


void VulkanBackEnd::cmd_SetViewportSize(VkCommandBuffer commandBuffer, int width, int height) {
	VkViewport viewport{};
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = 0.0;
	viewport.maxDepth = 1.0;
	VkRect2D rect2D{};
	rect2D.extent.width = width;
	rect2D.extent.height = height;
	rect2D.offset.x = 0;
	rect2D.offset.y = 0;
	VkRect2D scissor = VkRect2D(rect2D);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanBackEnd::cmd_BindPipeline(VkCommandBuffer commandBuffer, Pipeline& pipeline) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._handle);
}

void VulkanBackEnd::cmd_BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, HellDescriptorSet& descriptorSet) {
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._layout, setIndex, 1, &descriptorSet.handle, 0, nullptr);
}

void VulkanBackEnd::cmd_BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
}

void VulkanBackEnd::cmd_BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, HellDescriptorSet& descriptorSet) {
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, setIndex, 1, &descriptorSet.handle, 0, nullptr);
}

GLFWwindow* VulkanBackEnd::GetWindow() {
    GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();

	return _window;
}
