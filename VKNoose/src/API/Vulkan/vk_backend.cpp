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

		create_sampler();

		if (!VulkanRenderer::Init()) return false;
		if (!VulkanPipelineManager::Init()) return false;

		AssetManager::Init();
		AssetManager::LoadFont();
		AssetManager::LoadHardcodedMesh();

		upload_meshes();

		// Sampler
		VkDescriptorImageInfo samplerImageInfo = {};
		samplerImageInfo.sampler = _sampler;
		// CHANGE: Manager handles the actual set update
		VulkanDescriptorManager::GetStaticDescriptorSet().Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);

		// All textures
		VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
		for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
			textureImageInfo[i].sampler = nullptr;
			textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureImageInfo[i].imageView = (i < AssetManager::GetNumberOfTextures()) ? AssetManager::GetTexture(i)->imageView : AssetManager::GetTexture(0)->imageView;
		}

		VulkanDescriptorManager::GetStaticDescriptorSet().Update(GetDevice(), 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

		create_buffers();

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
		
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			_frames[i]._sceneTLAS = VulkanRaytracingManager::CreateTopLevelAS(Scene::GetMeshInstancesForSceneAccelerationStructure());
			_frames[i]._inventoryTLAS = VulkanRaytracingManager::CreateTopLevelAS(Scene::GetMeshInstancesForInventoryAccelerationStructure());
		}

		init_raytracing();
		update_static_descriptor_set();
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

	// 2. Cleanup Pipelines
	//_pipelines.textBlitter.Cleanup(GetDevice());
	//_pipelines.composite.Cleanup(GetDevice());
	//_pipelines.lines.Cleanup(GetDevice());

	// 4. Cleanup Raytracing
	cleanup_raytracing();

	// 6. Cleanup Mesh Buffers
	for (MeshOLD& mesh : AssetManager::GetMeshList()) {
		vmaDestroyBuffer(GetAllocator(), mesh.m_transformBuffer.m_buffer, mesh.m_transformBuffer.m_allocation);
		vmaDestroyBuffer(GetAllocator(), mesh.m_vertexBuffer.m_buffer, mesh.m_vertexBuffer.m_allocation);
		if (mesh.m_indexCount > 0) {
			vmaDestroyBuffer(GetAllocator(), mesh.m_indexBuffer.m_buffer, mesh.m_indexBuffer.m_allocation);
		}

		mesh.m_accelerationStructure.Cleanup(GetDevice());
	}
	vmaDestroyBuffer(GetAllocator(), _lineListMesh.m_vertexBuffer.m_buffer, _lineListMesh.m_vertexBuffer.m_allocation);

	// ADD THIS: Cleanup per-frame buffers
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		_frames[i]._sceneCamDataBuffer.Destroy(GetAllocator());
		_frames[i]._inventoryCamDataBuffer.Destroy(GetAllocator());
		_frames[i]._meshInstances2DBuffer.Destroy(GetAllocator());
		_frames[i]._meshInstancesSceneBuffer.Destroy(GetAllocator());
		_frames[i]._meshInstancesInventoryBuffer.Destroy(GetAllocator());
		_frames[i]._lightRenderInfoBuffer.Destroy(GetAllocator());
		_frames[i]._lightRenderInfoBufferInventory.Destroy(GetAllocator());

		_frames[i]._sceneTLAS.Cleanup(GetDevice());
		_frames[i]._inventoryTLAS.Cleanup(GetDevice());
	}

	// 7. Trigger Manager Cleanup in reverse initialization order
	VulkanDescriptorManager::Cleanup();
	VulkanCommandManager::Cleanup();
	VulkanSyncManager::Cleanup();
	VulkanSwapchainManager::Cleanup();

	vkDestroySampler(GetDevice(), _sampler, nullptr);

	// Core teardown
	VulkanMemoryManager::Cleanup();
	VulkanDeviceManager::Cleanup();
	VulkanInstanceManager::Cleanup();

	glfwDestroyWindow(_window);
	glfwTerminate();
}

uint32_t alignedSize(uint32_t value, uint32_t alignment) {
	return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanBackEnd::init_raytracing() {
	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& _rayTracingPipelineProperties = VulkanDeviceManager::GetRayTracingPipelineProperties();

	// CHANGE: Get standardized layouts from the Descriptor Manager instead of _frames[0]
	std::vector<VkDescriptorSetLayout> rtDescriptorSetLayouts = {
		VulkanDescriptorManager::GetDynamicSetLayout(),
		VulkanDescriptorManager::GetStaticSetLayout(),
		VulkanDescriptorManager::GetSamplerSetLayout()
	};

	_raytracerPath.CreatePipeline(GetDevice(), rtDescriptorSetLayouts, 2);
	_raytracerPath.CreateShaderBindingTable(GetDevice(), GetAllocator(), _rayTracingPipelineProperties);

	_raytracerMousePick.CreatePipeline(GetDevice(), rtDescriptorSetLayouts, 2);
	_raytracerMousePick.CreateShaderBindingTable(GetDevice(), GetAllocator(), _rayTracingPipelineProperties);
}


void VulkanBackEnd::cleanup_raytracing() {
	// RT cleanup
	//vmaDestroyBuffer(GetAllocator(), _rtVertexBuffer.m_buffer, _rtVertexBuffer.m_allocation);
	//vmaDestroyBuffer(GetAllocator(), _rtIndexBuffer.m_buffer, _rtIndexBuffer.m_allocation);
	//vmaDestroyBuffer(GetAllocator(), _mousePickResultBuffer.m_buffer, _mousePickResultBuffer.m_allocation);
	//
	//vmaDestroyBuffer(GetAllocator(), _mousePickResultCPUBuffer.m_buffer, _mousePickResultCPUBuffer.m_allocation);

	//vmaDestroyBuffer(GetAllocator(), _frames[0]._sceneTLAS.buffer.m_buffer, _frames[0]._sceneTLAS.buffer.m_allocation);
	//vmaDestroyBuffer(GetAllocator(), _frames[1]._sceneTLAS.buffer.m_buffer, _frames[1]._sceneTLAS.buffer.m_allocation);
	//vmaDestroyBuffer(GetAllocator(), _frames[0]._inventoryTLAS.buffer.m_buffer, _frames[0]._inventoryTLAS.buffer.m_allocation);
	//vmaDestroyBuffer(GetAllocator(), _frames[1]._inventoryTLAS.buffer.m_buffer, _frames[1]._inventoryTLAS.buffer.m_allocation);
	//
	//vkDestroyAccelerationStructureKHR(GetDevice(), _frames[0]._sceneTLAS.handle, nullptr);
	//vkDestroyAccelerationStructureKHR(GetDevice(), _frames[1]._sceneTLAS.handle, nullptr);
	//vkDestroyAccelerationStructureKHR(GetDevice(), _frames[0]._inventoryTLAS.handle, nullptr);
	//vkDestroyAccelerationStructureKHR(GetDevice(), _frames[1]._inventoryTLAS.handle, nullptr);

	_raytracerPath.Cleanup(GetDevice(), GetAllocator());
	_raytracerMousePick.Cleanup(GetDevice(), GetAllocator());
}

void VulkanBackEnd::RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer) {
	AllocatedImage* loadingTarget = VulkanResourceManager::GetAllocatedImage("LoadingScreen");
	VulkanPipeline* textBlitterPipeline = VulkanPipelineManager::GetPipeline("TextBlitter");

	if (!loadingTarget) return;
	if (!textBlitterPipeline) return;

	uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;

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

//	cmd_BindPipeline(commandBuffer, _pipelines.textBlitter);

	// CHANGE: Use Descriptor Manager to get the per-frame dynamic set and the global static set
	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	HellDescriptorSet& staticSet = VulkanDescriptorManager::GetStaticDescriptorSet();

	//cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, dynamicSet);
	//cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, staticSet);


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

	uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	FrameData& frame = _frames[frameIndex];

	VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

	// Wait for the GPU to finish the last frame using this FrameData slot
	VulkanSyncManager::WaitForRenderFence(frameIndex);

	// Reset the fence before submitting new work
	VulkanSyncManager::ResetRenderFence(frameIndex);

	// Update 2D buffers and descriptor sets
	UpdateBuffers2D();

	// Retrieve the set from the Descriptor Manager and update it
	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	dynamicSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._meshInstances2DBuffer.buffer);

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

	_frameNumber++;
}


void VulkanBackEnd::RenderGameFrame() {
	if (ProgramIsMinimized()) return;

	AllocatedImage* presentAllocatedImage = VulkanResourceManager::GetAllocatedImage("Present");
	if (!presentAllocatedImage) return;

	TextBlitter::Update(GameData::GetDeltaTime(), presentAllocatedImage->GetWidth(), presentAllocatedImage->GetHeight());

	uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	FrameData& frame = _frames[frameIndex];

	VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

	VulkanSyncManager::WaitForRenderFence(frameIndex);

	{
		frame._sceneTLAS = VulkanRaytracingManager::CreateTopLevelAS(Scene::GetMeshInstancesForSceneAccelerationStructure());
		frame._inventoryTLAS = VulkanRaytracingManager::CreateTopLevelAS(Scene::GetMeshInstancesForInventoryAccelerationStructure());

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
		uint32_t mousePickResult[2];
		void* mappedData;
		VulkanResourceManager::GetBuffer("MousePick_CPU")->Map(&mappedData);
		memcpy(mousePickResult, mappedData, sizeof(uint32_t) * 2);
		Scene::StoreMousePickResult(mousePickResult[0], mousePickResult[1]);
	}
	else {
		Scene::StoreMousePickResult(-1, -1);
	}

	_frameNumber++;
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

FrameData& VulkanBackEnd::get_current_frame() {
	return _frames[_frameNumber % FRAME_OVERLAP];
}


FrameData& VulkanBackEnd::get_last_frame() {
	return _frames[(_frameNumber - 1) % 2];
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
			mesh.m_accelerationStructure = VulkanRaytracingManager::CreateBottomLevelAS(&mesh);
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

		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &mesh.m_vertexBuffer.m_buffer, &mesh.m_vertexBuffer.m_allocation, nullptr));

		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, mesh.m_vertexBuffer.m_buffer, 1, &copy);
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

		VK_CHECK(vmaCreateBuffer(GetAllocator(), &indexBufferInfo, &vmaallocInfo, &mesh.m_indexBuffer.m_buffer, &mesh.m_indexBuffer.m_allocation, nullptr));

		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, mesh.m_indexBuffer.m_buffer, 1, &copy);
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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &mesh.m_transformBuffer.m_buffer, &mesh.m_transformBuffer.m_allocation, nullptr));

		VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, mesh.m_transformBuffer.m_buffer, 1, &copy);
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

size_t VulkanBackEnd::pad_uniform_buffer_size(size_t originalSize) {
	const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();

	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}




void VulkanBackEnd::create_sampler() {
	const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();

	// Good as place as any for a turret
	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);//VK_FILTER_LINEAR VK_FILTER_NEAREST
	samplerInfo.magFilter = VK_FILTER_LINEAR; //  VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_LINEAR; //  VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // VK_SAMPLER_MIPMAP_MODE_NEAREST VK_SAMPLER_MIPMAP_MODE_LINEAR
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 12.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;;
	samplerInfo.anisotropyEnable = VK_TRUE;
	vkCreateSampler(GetDevice(), &samplerInfo, nullptr, &_sampler);

    std::cout << "created sampler\n";
}



uint32_t VulkanBackEnd::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) {
	const VkPhysicalDeviceMemoryProperties& memoryProperties = VulkanDeviceManager::GetMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				if (memTypeFound) {
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}
	if (memTypeFound) {
		*memTypeFound = false;
		return 0;
	}
	else {
		throw std::runtime_error("Could not find a matching memory type");
	}
}



void VulkanBackEnd::create_rt_buffers()
{
	// Get the raw geometry data from the asset manager
	std::vector<Vertex>& vertices = AssetManager::GetVertices_TEMPORARY();
	std::vector<uint32_t>& indices = AssetManager::GetIndices_TEMPORARY();

	// Define flags for raytracing geometry buffers
	// These allow the buffer to be used for AS builds and as a storage buffer in shaders
	VkBufferUsageFlags rtGeometryUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

	// Create the high-level VulkanBuffer objects via the resource manager
	// These are GPU_ONLY for optimal traversal performance
	VulkanResourceManager::CreateBuffer("RT_VertexBuffer", vertices.size() * sizeof(Vertex), rtGeometryUsage, VMA_MEMORY_USAGE_GPU_ONLY);
	VulkanResourceManager::CreateBuffer("RT_IndexBuffer", indices.size() * sizeof(uint32_t), rtGeometryUsage, VMA_MEMORY_USAGE_GPU_ONLY);

	// Upload the data using the internal staging logic
	// This handles the creation and destruction of the temporary staging buffer automatically
	VulkanResourceManager::GetBuffer("RT_VertexBuffer")->UploadData(vertices.data(), vertices.size() * sizeof(Vertex));
	VulkanResourceManager::GetBuffer("RT_IndexBuffer")->UploadData(indices.data(), indices.size() * sizeof(uint32_t));

	// Refactored Mouse Pick Buffers
	VkDeviceSize pickBufferSize = sizeof(uint32_t) * 2;

	// GPU buffer for the raytracing shader to write into
	VulkanResourceManager::CreateBuffer("MousePick_GPU", pickBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO);

	// CPU buffer for the backend to read from
	// MAPPED and HOST_ACCESS_SEQUENTIAL_WRITE to keep the pointer valid permanently
	VulkanResourceManager::CreateBuffer("MousePick_CPU", pickBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
}




void VulkanBackEnd::create_buffers() {
	// Dynamic
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		_frames[i]._sceneCamDataBuffer.Create(GetAllocator(), sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		_frames[i]._inventoryCamDataBuffer.Create(GetAllocator(), sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		_frames[i]._meshInstances2DBuffer.Create(GetAllocator(), sizeof(GPUObjectData2D) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		_frames[i]._meshInstancesSceneBuffer.Create(GetAllocator(), sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		_frames[i]._meshInstancesInventoryBuffer.Create(GetAllocator(), sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		_frames[i]._lightRenderInfoBuffer.Create(GetAllocator(), sizeof(LightRenderInfo) * MAX_LIGHTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		_frames[i]._lightRenderInfoBufferInventory.Create(GetAllocator(), sizeof(LightRenderInfo) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
	
	// Static
	// none as of yet. besides TLAS but that is created elsewhere.
    std::cout << "created buffers\n";
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
	get_current_frame()._meshInstances2DBuffer.MapRange(GetAllocator(), RasterRenderer::_instanceData2D, sizeof(GPUObjectData2D) * RasterRenderer::instanceCount);
}

void VulkanBackEnd::UpdateBuffers() {

	CameraData camData;
	camData.proj = GameData::GetProjectionMatrix();
	camData.view = GameData::GetViewMatrix();
	camData.projInverse = glm::inverse(camData.proj);
	camData.viewInverse = glm::inverse(camData.view);
	camData.viewPos = glm::vec4(GameData::GetCameraPosition(), 1.0f);
	camData.vertexSize = sizeof(Vertex);
	camData.frameIndex = _frameIndex;
	camData.inventoryOpen = (GameData::inventoryOpen) ? 1: 0;;
	get_current_frame()._sceneCamDataBuffer.Map(GetAllocator(), &camData);

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
	get_current_frame()._inventoryCamDataBuffer.Map(GetAllocator(), &inventoryCamData);

	// 3D instance data
	std::vector<MeshInstance> meshInstances = Scene::GetSceneMeshInstances(_debugScene);
	get_current_frame()._meshInstancesSceneBuffer.MapRange(GetAllocator(), meshInstances.data(), sizeof(MeshInstance) * meshInstances.size());

	// 3D instance data
	std::vector<MeshInstance> inventoryMeshInstances = Scene::GetInventoryMeshInstances(_debugScene);
	get_current_frame()._meshInstancesInventoryBuffer.MapRange(GetAllocator(), inventoryMeshInstances.data(), sizeof(MeshInstance) * inventoryMeshInstances.size());

	// Light render info
	std::vector<LightRenderInfo> lightRenderInfo = Scene::GetLightRenderInfo();
	get_current_frame()._lightRenderInfoBuffer.MapRange(GetAllocator(), lightRenderInfo.data(), sizeof(LightRenderInfo) * lightRenderInfo.size());
	std::vector<LightRenderInfo> lightRenderInfoInventory = Scene::GetLightRenderInfoInventory();
	get_current_frame()._lightRenderInfoBufferInventory.MapRange(GetAllocator(), lightRenderInfoInventory.data(), sizeof(LightRenderInfo) * lightRenderInfoInventory.size());
}

void VulkanBackEnd::update_static_descriptor_set() {
	AllocatedImage* rtFirstHitColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Color");
	AllocatedImage* rtFirstHitNormalsAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Normals");
	AllocatedImage* rtFirstHitBaseColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_BaseColor");
	AllocatedImage* rtSecondHitColorAllocatedImage = VulkanResourceManager::GetAllocatedImage("RT_SecondHit_Color");
	AllocatedImage* gBufferNormalAllocatedImage = VulkanResourceManager::GetAllocatedImage("GBuffer_Normal");
	AllocatedImage* gBufferRmaAllocatedImage = VulkanResourceManager::GetAllocatedImage("GBuffer_RMA");
	AllocatedImage* laptopDisplayAllocatedImage = VulkanResourceManager::GetAllocatedImage("LaptopDisplay");
	AllocatedImage* presentAllocatedImage = VulkanResourceManager::GetAllocatedImage("Present");
	AllocatedImage* depthGBufferAllocatedImage = VulkanResourceManager::GetAllocatedImage("Depth_GBuffer");

	VulkanBuffer* rtVertexBuffer = VulkanResourceManager::GetBuffer("RT_VertexBuffer");
	VulkanBuffer* rtIndexBuffer = VulkanResourceManager::GetBuffer("RT_IndexBuffer");
	VulkanBuffer* mousePickGPU = VulkanResourceManager::GetBuffer("MousePick_GPU");

	// Get descriptor sets from the manager
	HellDescriptorSet& staticSet = VulkanDescriptorManager::GetStaticDescriptorSet();
	HellDescriptorSet& samplerSet = VulkanDescriptorManager::GetSamplerDescriptorSet();

	// 1. Sampler binding
	VkDescriptorImageInfo samplerImageInfo = {};
	samplerImageInfo.sampler = _sampler;
	staticSet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);

	// 2. All textures array
	VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
		textureImageInfo[i].sampler = nullptr;
		textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureImageInfo[i].imageView = (i < AssetManager::GetNumberOfTextures()) ? AssetManager::GetTexture(i)->imageView : AssetManager::GetTexture(0)->imageView;
	}
	staticSet.Update(GetDevice(), 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

	// 3. Global vertex and index data
	if (rtVertexBuffer && rtIndexBuffer) {
		staticSet.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, rtVertexBuffer->GetBuffer());
		staticSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, rtIndexBuffer->GetBuffer());
	}

	// 4. Raytracing storage images
	VkDescriptorImageInfo storageImageDescriptor{};
	storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	storageImageDescriptor.imageView = rtFirstHitColorAllocatedImage->GetImageView();
	staticSet.Update(GetDevice(), 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

	// 5. 1x1 mouse picking buffer
	staticSet.Update(GetDevice(), 5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mousePickGPU->GetBuffer());

	// 6. Additional RT hit info
	storageImageDescriptor.imageView = rtFirstHitNormalsAllocatedImage->GetImageView();
	staticSet.Update(GetDevice(), 6, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

	storageImageDescriptor.imageView = rtFirstHitBaseColorAllocatedImage->GetImageView();
	staticSet.Update(GetDevice(), 7, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

	storageImageDescriptor.imageView = rtSecondHitColorAllocatedImage->GetImageView();
	staticSet.Update(GetDevice(), 8, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

	// 7. Transition layouts for General layout requirements
	VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
		laptopDisplayAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		gBufferNormalAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		gBufferRmaAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		depthGBufferAllocatedImage->TransitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		});

	// 8. Update Sampler Sets
	VkDescriptorImageInfo samplerSetImageInfo{};
	samplerSetImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	samplerSetImageInfo.sampler = _sampler;

	samplerSetImageInfo.imageView = rtFirstHitColorAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerSetImageInfo);

	samplerSetImageInfo.imageView = rtFirstHitNormalsAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerSetImageInfo);

	samplerSetImageInfo.imageView = rtFirstHitBaseColorAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerSetImageInfo);

	samplerSetImageInfo.imageView = rtSecondHitColorAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerSetImageInfo);

	samplerSetImageInfo.imageView = laptopDisplayAllocatedImage->GetImageView();
	samplerSet.Update(GetDevice(), 7, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &samplerSetImageInfo);
}

void VulkanBackEnd::UpdateDynamicDescriptorSet() {
	uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	FrameData& frame = _frames[frameIndex];

	// Get the per-frame sets from the manager
	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	HellDescriptorSet& dynamicSetInventory = VulkanDescriptorManager::GetDynamicInventoryDescriptorSet(frameIndex);

	// Update Dynamic Inventory Set
	dynamicSetInventory.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &frame._inventoryTLAS.handle);
	dynamicSetInventory.Update(GetDevice(), 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame._inventoryCamDataBuffer.buffer);
	dynamicSetInventory.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._meshInstancesInventoryBuffer.buffer);
	dynamicSetInventory.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._meshInstances2DBuffer.buffer);
	dynamicSetInventory.Update(GetDevice(), 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._lightRenderInfoBufferInventory.buffer);

	// Update Dynamic Scene Set
	dynamicSet.Update(GetDevice(), 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &frame._sceneTLAS.handle);
	dynamicSet.Update(GetDevice(), 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame._sceneCamDataBuffer.buffer);
	dynamicSet.Update(GetDevice(), 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._meshInstancesSceneBuffer.buffer);
	dynamicSet.Update(GetDevice(), 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._meshInstances2DBuffer.buffer);
	dynamicSet.Update(GetDevice(), 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame._lightRenderInfoBuffer.buffer);
}

void VulkanBackEnd::build_rt_command_buffers(int swapchainIndex) {
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

	uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
	VkCommandBufferBeginInfo cmdBufInfo = vkinit::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

	HellDescriptorSet& dynamicSet = VulkanDescriptorManager::GetDynamicDescriptorSet(frameIndex);
	HellDescriptorSet& dynamicSetInventory = VulkanDescriptorManager::GetDynamicInventoryDescriptorSet(frameIndex);
	HellDescriptorSet& staticSet = VulkanDescriptorManager::GetStaticDescriptorSet();
	HellDescriptorSet& samplerSet = VulkanDescriptorManager::GetSamplerDescriptorSet();

	if (_usePathRayTracer) {
		cmd_BindRayTracingPipeline(commandBuffer, _raytracerPath.pipeline);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 0, dynamicSet);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 1, staticSet);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 2, samplerSet);

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
		//cmd_BindPipeline(commandBuffer, _pipelines.textBlitter);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, dynamicSet);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, staticSet);

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
	//		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, &dynamicSet.handle, 0, nullptr);
	//		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 1, 1, &staticSet.handle, 0, nullptr);

//			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.lines._handle);
			glm::mat4 projection = glm::perspective(GameData::_cameraZoom, 1700.f / 900.f, 0.01f, 100.0f);
			glm::mat4 view = GameData::GetPlayer().m_camera.GetViewMatrix();
			projection[1][1] *= -1;
			LineShaderPushConstants constants;
			constants.transformation = projection * view;
			//vkCmdPushConstants(commandBuffer, _pipelines.lines._layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LineShaderPushConstants), &constants);
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
		VulkanBuffer* gpuBuffer = VulkanResourceManager::GetBuffer("MousePick_GPU");
		VulkanBuffer* cpuBuffer = VulkanResourceManager::GetBuffer("MousePick_CPU");

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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &_lineListMesh.m_vertexBuffer.m_buffer, &_lineListMesh.m_vertexBuffer.m_allocation, nullptr));
		add_debug_name(_lineListMesh.m_vertexBuffer.m_buffer, "_lineListMesh._vertexBuffer");
		// Name the mesh
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		nameInfo.objectHandle = (uint64_t)_lineListMesh.m_vertexBuffer.m_buffer;
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
			vkCmdCopyBuffer(cmd, stagingBuffer.m_buffer, _lineListMesh.m_vertexBuffer.m_buffer, 1, &copy);
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
