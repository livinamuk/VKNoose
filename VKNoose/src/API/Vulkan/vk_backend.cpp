#include "vk_backend.h"
#include <chrono> 
#include <fstream> 
#include "vk_types.h"
#include "vk_textures.h"
#include "vk_tools.h"
#include "Util/Util.h"
 
#include "AssetManagement/AssetManager.h"
#include "Game/Scene.h"
#include "Game/Laptop.h"
#include "Renderer/RasterRenderer.h"
#include "Profiler.h"

#include "API/Vulkan/Managers/vk_command_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_instance_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_raytracing_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_swapchain_manager.h"
#include "API/Vulkan/Managers/vk_sync_manager.h"

#include "API/Vulkan/Renderer/vk_descriptor_indices.h"
#include "API/Vulkan/Renderer/vk_renderer.h"
#include "API/Vulkan/Renderer/vk_push_constants.h"

#include "BackEnd/GLFWIntegration.h"

#include "Hell/Logging.h"

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

namespace VulkanBackEnd {

	bool VulkanBackEnd::InitMinimum() {
		if (!VulkanInstanceManager::Init())  return false;
		if (!VulkanDeviceManager::Init())    return false;
		if (!VulkanMemoryManager::Init())    return false;
		if (!VulkanSwapchainManager::Init()) return false;
		if (!VulkanSyncManager::Init())      return false;
		if (!VulkanCommandManager::Init())   return false;
		if (!VulkanRenderer::Init())         return false;

		AssetManager::Init();
		AssetManager::LoadFont();
		AssetManager::LoadHardcodedMesh();

		VulkanRenderer::UpdateBindlessTexturesDescriptorSets();
		VulkanBackEnd::ToggleFullscreen();

		Logging::Init() << "VulkanBackEnd::InitMinimum()\n";
		return true;
	}
}


#include <set>


void VulkanBackEnd::AddLoadingText(const std::string& text) {
	_loadingText.push_back(text);
}

void VulkanBackEnd::LoadNextItem() {

	// These return false if there is nothing to load
	// meaning it will work its way down this function each game loop until everything is loaded

	if (AssetManager::LoadNextTexture())
		return;

	if (AssetManager::LoadNextModel()) 
		return;

	static bool rtSetup = false;
	static bool rtSetupMSG = false;
	if (!rtSetupMSG) {
		rtSetupMSG = true;
		AddLoadingText("Initializing raytracing...");
		return;
	}
	if (!rtSetup) {
		rtSetup = true;
		AssetManager::BuildMaterials();
		Laptop::Init();
		Audio::Init();
		Scene::Init();

        vkDeviceWaitIdle(GetDevice());

        VulkanRenderer::UpdateBindlessTexturesDescriptorSets();
        VulkanRenderer::UpdateRenderTargetsDescriptorSets();

		Input::SetMousePos(_windowedModeExtent.width / 2, _windowedModeExtent.height / 2);
	}

	_loaded = true;
	TextBlitter::ResetDebugText();
}

void VulkanBackEnd::Cleanup() {
	GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();

	vkDeviceWaitIdle(GetDevice());

	VulkanResourceManager::Cleanup();

	// Cleanup legacy textures
	for (int i = 0; i < AssetManager::GetNumberOfTextures(); i++) {
		TextureOLD* tex = AssetManager::GetTextureByIndexOLD(i);
		if (tex) {
			if (tex->imageView != VK_NULL_HANDLE) {
				vkDestroyImageView(GetDevice(), tex->imageView, nullptr);
				tex->imageView = VK_NULL_HANDLE;
			}

			if (tex->image._image != VK_NULL_HANDLE) {
				vmaDestroyImage(GetAllocator(), tex->image._image, tex->image._allocation);
				tex->image._image = VK_NULL_HANDLE;
				tex->image._allocation = VK_NULL_HANDLE;
			}
		}
	}

	if (_lineListMesh.m_vertexBufferOLD.m_buffer != VK_NULL_HANDLE) {
		vmaDestroyBuffer(GetAllocator(), _lineListMesh.m_vertexBufferOLD.m_buffer, _lineListMesh.m_vertexBufferOLD.m_allocation);
		_lineListMesh.m_vertexBufferOLD.m_buffer = VK_NULL_HANDLE;
		_lineListMesh.m_vertexBufferOLD.m_allocation = VK_NULL_HANDLE;
	}

	VulkanCommandManager::Cleanup();
	VulkanSyncManager::Cleanup();
	VulkanSwapchainManager::Cleanup();
	VulkanMemoryManager::Cleanup();
	VulkanDeviceManager::Cleanup();
	VulkanInstanceManager::Cleanup();

	glfwDestroyWindow(_window);
	glfwTerminate();
}

void VulkanBackEnd::RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer) {
	AllocatedImage* loadingTarget = VulkanResourceManager::GetAllocatedImage("LoadingScreen");

	if (!loadingTarget) return;

    VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();
	uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();

	VkRenderingAttachmentInfo renderingAttachmentInfo = {};
	renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	renderingAttachmentInfo.imageView = loadingTarget->GetImageView();
	renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	renderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	renderingAttachmentInfo.clearValue = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = { 0, 0, (uint32_t)loadingTarget->GetWidth(), (uint32_t)loadingTarget->GetHeight() };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &renderingAttachmentInfo;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	cmd_SetViewportSize(commandBuffer, loadingTarget->GetWidth(), loadingTarget->GetHeight());

	// Pipeline
    VulkanPipeline* textBlitterPipeline = VulkanResourceManager::GetPipeline("TextBlitter");
    if (!textBlitterPipeline) return;
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetHandle());

	// Static descriptor set
    VulkanDescriptorSet* staticSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, staticSet->GetHandlePtr(), 0, nullptr);

    // Push constant
    UIPushConstant pushConstant{};
    pushConstant.instancesDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.uiInstances)->GetDeviceAddress();
    vkCmdPushConstants(commandBuffer, textBlitterPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstant), &pushConstant);

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

AllocatedBufferOLD VulkanBackEnd::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags) {
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

void VulkanBackEnd::UpdateBuffers2D() {

	// Queue all text characters for rendering
	for (auto& instanceInfo : TextBlitter::_objectData) {
		RasterRenderer::SubmitUI(instanceInfo.index_basecolor, instanceInfo.index_color, instanceInfo.modelMatrix, RasterRenderer::Destination::MAIN_UI, instanceInfo.xClipMin, instanceInfo.xClipMax, instanceInfo.yClipMin, instanceInfo.yClipMax); // Todo: You are storing color in the normals. Probably not a major deal but could be confusing at some point down the line.
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

void DrawMesh(VkCommandBuffer commandBuffer, uint32_t meshIndex, uint32_t firstInstance) {
    Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
    if (!mesh) return;

    if (mesh->GetVertexCount() == 0) return;
    if (mesh->GetIndexCount() == 0) return;

    VkDeviceSize offset = 0;

    VulkanBuffer* vertexBuffer = VulkanRenderer::GetVertexBuffer();
    VulkanBuffer* indexBuffer = VulkanRenderer::GetIndexBuffer();
    VkBuffer vertexBufferPtr = vertexBuffer->GetBuffer();
    VkBuffer indexBufferPtr = indexBuffer->GetBuffer();

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBufferPtr, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBufferPtr, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->GetIndexCount()), 1, mesh->GetBaseIndex(), mesh->GetBaseVertex(), firstInstance);
}

void DrawMesh(VkCommandBuffer commandBuffer, Mesh* mesh, uint32_t firstInstance) {
    if (!mesh || mesh->GetVertexCount() == 0 || mesh->GetIndexCount() == 0) return;

    VulkanBuffer* vertexBuffer = VulkanRenderer::GetVertexBuffer();
    VulkanBuffer* indexBuffer = VulkanRenderer::GetIndexBuffer();
    VkBuffer vertexBufferPtr = vertexBuffer->GetBuffer();
    VkBuffer indexBufferPtr = indexBuffer->GetBuffer();

    VkDeviceSize offset = 0;

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBufferPtr, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBufferPtr, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->GetIndexCount()), 1, 0, 0, firstInstance);
}

void VulkanBackEnd::build_rt_command_buffers() {
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

    VulkanDescriptorSet* staticSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
    VulkanDescriptorSet* sceneTlasSet = VulkanResourceManager::GetDescriptorSet("SceneTLASDescriptorSet");
    VulkanDescriptorSet* inventoryTlasSet = VulkanResourceManager::GetDescriptorSet("InventoryTLASDescriptorSet");

	VulkanPipeline* compositePipeline = VulkanResourceManager::GetPipeline("Composite");
	VulkanPipeline* linesPipeline = VulkanResourceManager::GetPipeline("Lines");
    VulkanPipeline* textBlitterPipeline = VulkanResourceManager::GetPipeline("TextBlitter");
    VulkanRaytracingPipeline* mousePickPipeline = VulkanResourceManager::GetRaytracingPipeline("MousePick");
    VulkanRaytracingPipeline* pathPipeline = VulkanResourceManager::GetRaytracingPipeline("PathTrace");

    if (!staticSet) return;
    if (!sceneTlasSet) return;
    if (!inventoryTlasSet) return;
    if (!compositePipeline) return;
    if (!linesPipeline) return;
    if (!textBlitterPipeline) return;
    if (!pathPipeline) return;
    if (!mousePickPipeline) return;

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkResetCommandBuffer(commandBuffer, 0);
	vkBeginCommandBuffer(commandBuffer, &cmdBufInfo);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pathPipeline->GetHandle());

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pathPipeline->GetLayout(), 0, 1, sceneTlasSet->GetHandlePtr(), 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pathPipeline->GetLayout(), 1, 1, staticSet->GetHandlePtr(), 0, nullptr);

	rtFirstHitColorAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
	rtFirstHitNormalsAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
	rtSecondHitColorAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
	rtFirstHitBaseColorAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);

    // Main scene push constant
    ScenePushConstants pushConstant{};
    pushConstant.instancesDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.sceneInstances)->GetDeviceAddress();
    pushConstant.lightsDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.sceneLights)->GetDeviceAddress();
    pushConstant.cameraDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.sceneCameraData)->GetDeviceAddress();
    pushConstant.lightCount = 2;
    vkCmdPushConstants(commandBuffer,pathPipeline->GetLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, sizeof(ScenePushConstants), &pushConstant);

	//vkCmdTraceRaysKHR(commandBuffer, &_raytracerPath.raygenShaderSbtEntry, &_raytracerPath.missShaderSbtEntry, &_raytracerPath.hitShaderSbtEntry, &_raytracerPath.callableShaderSbtEntry, rtFirstHitColorAllocatedImage->GetWidth(), rtFirstHitColorAllocatedImage->GetHeight(), 1);

	// Trace rays
    const VulkanShaderBindingTable& pathSbt = pathPipeline->GetShaderBindingTable();
    vkCmdTraceRaysKHR(commandBuffer, &pathSbt.raygen, &pathSbt.miss, &pathSbt.hit, &pathSbt.callable, rtFirstHitColorAllocatedImage->GetWidth(), rtFirstHitColorAllocatedImage->GetHeight(), 1);

	if (GameData::inventoryOpen) {
		// Push constant
        ScenePushConstants pushConstant{};
        pushConstant.instancesDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryInstances)->GetDeviceAddress();
        pushConstant.lightsDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryLights)->GetDeviceAddress();
        pushConstant.cameraDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryCameraData)->GetDeviceAddress();
        pushConstant.lightCount = 2;
        vkCmdPushConstants(commandBuffer, pathPipeline->GetLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, sizeof(ScenePushConstants), &pushConstant);

		// Descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pathPipeline->GetLayout(), 0, 1, inventoryTlasSet->GetHandlePtr(), 0, nullptr);

		// Trace rays
        vkCmdTraceRaysKHR(commandBuffer, &pathSbt.raygen, &pathSbt.miss, &pathSbt.hit, &pathSbt.callable, rtFirstHitColorAllocatedImage->GetWidth(), rtFirstHitColorAllocatedImage->GetHeight(), 1);
	}

	// Mouse pick
	{
        MousePickPushConstants pushConstant{};
        pushConstant.cameraDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.sceneCameraData)->GetDeviceAddress();
        pushConstant.mousePickBufferAddress = VulkanResourceManager::GetBuffer(frameData.buffers.mousePickBufferGPU)->GetDeviceAddress();
        vkCmdPushConstants(commandBuffer, mousePickPipeline->GetLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(MousePickPushConstants), &pushConstant);

        cmd_BindRayTracingPipeline(commandBuffer, mousePickPipeline->GetHandle());

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mousePickPipeline->GetLayout(), 0, 1, sceneTlasSet->GetHandlePtr(), 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mousePickPipeline->GetLayout(), 1, 1, staticSet->GetHandlePtr(), 0, nullptr);

        const VulkanShaderBindingTable& mouseSbt = mousePickPipeline->GetShaderBindingTable();
        vkCmdTraceRaysKHR(commandBuffer, &mouseSbt.raygen, &mouseSbt.miss, &mouseSbt.hit, &mouseSbt.callable, 1, 1, 1);
	}

	// Laptop display rendering
	{
		TextureOLD* bg_texture = AssetManager::GetTextureByNameOLD("OS_bg");
		if (bg_texture) {
			// Sync laptopDisplayAllocatedImage for WRITING (it's the destination of the blit)
			laptopDisplayAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

			VkImageBlit blitRegion{};
			blitRegion.srcOffsets[1] = { (int32_t)bg_texture->_width, (int32_t)bg_texture->_height, 1 };
			blitRegion.dstOffsets[1] = { (int32_t)laptopDisplayAllocatedImage->GetWidth(), (int32_t)laptopDisplayAllocatedImage->GetHeight(), 1 };
			blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

			// Use GENERAL for both layouts now
			vkCmdBlitImage(commandBuffer, bg_texture->image._image, VK_IMAGE_LAYOUT_GENERAL, laptopDisplayAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &blitRegion, VK_FILTER_NEAREST);

			// If bg_texture also has a Sync method, use it instead of insertImageBarrier
			laptopDisplayAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

			// Sync laptopDisplayAllocatedImage to receive color attachment writes
			laptopDisplayAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

			VkRenderingAttachmentInfo laptopColorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			laptopColorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // Stay General
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
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, staticSet->GetHandlePtr(), 0, nullptr);

            // Push constant
            UIPushConstant pushConstant{};
            pushConstant.instancesDeviceAddress = VulkanResourceManager::GetBuffer(frameData.buffers.uiInstances)->GetDeviceAddress();
            vkCmdPushConstants(commandBuffer, textBlitterPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstant), &pushConstant);

			for (int i = 0; i < RasterRenderer::instanceCount; i++) {
				if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::LAPTOP_DISPLAY)
					RasterRenderer::DrawMesh(commandBuffer, i);
			}
			vkCmdEndRendering(commandBuffer);

			// Final sync for the next raytracing pass
			laptopDisplayAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
		}
	}

	// Composite Pass
	{
		compositeAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

		VkRenderingAttachmentInfo compositeAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		compositeAttachment.imageView = compositeAllocatedImage->GetImageView();
		compositeAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline->GetLayout(), 0, 1, staticSet->GetHandlePtr(), 0, nullptr);

        uint32_t meshIndex = AssetManager::GetModelByName("fullscreen_quad")->GetMeshIndices()[0];
		DrawMesh(commandBuffer, meshIndex, 0);

		vkCmdEndRendering(commandBuffer);

		// Blit Composite to Present
		//compositeAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		//presentAllocatedImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		compositeAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		presentAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

		VkImageBlit blitRegion{};
		blitRegion.srcOffsets[1] = { (int32_t)compositeAllocatedImage->GetWidth(), (int32_t)compositeAllocatedImage->GetHeight(), 1 };
		blitRegion.dstOffsets[1] = { (int32_t)presentAllocatedImage->GetWidth(), (int32_t)presentAllocatedImage->GetHeight(), 1 };
		blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		vkCmdBlitImage(commandBuffer, compositeAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, presentAllocatedImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &blitRegion, VK_FILTER_LINEAR);
	}

	// Main UI Pass
	{
		presentAllocatedImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

		VkRenderingAttachmentInfo uiAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		uiAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textBlitterPipeline->GetLayout(), 0, 1, staticSet->GetHandlePtr(), 0, nullptr);

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

            if (_lineListMesh.GetVertexCount() == 0) return;
            if (_lineListMesh.GetIndexCount() == 0) return;

            VulkanBuffer* vertexBuffer = VulkanRenderer::GetVertexBuffer();
            VulkanBuffer* indexBuffer = VulkanRenderer::GetIndexBuffer();
            VkBuffer vertexBufferPtr = vertexBuffer->GetBuffer();
            VkBuffer indexBufferPtr = indexBuffer->GetBuffer();

            VkDeviceSize offset = 0;
            uint32_t firstInstance = 0;

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBufferPtr, &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexBufferPtr, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_lineListMesh.GetIndexCount()), 1, 0, 0, firstInstance);
		}
		vkCmdEndRendering(commandBuffer);
	}

	if (!GameData::inventoryOpen) {
		VulkanBuffer* gpuBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.mousePickBufferGPU);
		VulkanBuffer* cpuBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.mousePickBufferCPU);

		VkBufferCopy pickCopy = { 0, 0, sizeof(uint32_t) * 2 };
		vkCmdCopyBuffer(commandBuffer, gpuBuffer->GetBuffer(), cpuBuffer->GetBuffer(), 1, &pickCopy);
	}
}

void VulkanBackEnd::AddDebugText() {

	TextBlitter::ResetDebugText();

    std::vector<std::string> newLoadingText;
    newLoadingText.insert(newLoadingText.end(), AssetManager::GetLoadLog().begin(), AssetManager::GetLoadLog().end());
	newLoadingText.insert(newLoadingText.end(), _loadingText.begin(), _loadingText.end());

	//for (const std::string& t : newLoadingText) {
	//	std::cout << t << "\n";
	//}
	//std::cout << "\n";

	if (!_loaded) {
		int begin = std::max(0, (int)newLoadingText.size() - 36);
		for (int i = begin; i < newLoadingText.size(); i++) {
			TextBlitter::AddDebugText(newLoadingText[i]);
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
		TextBlitter::AddDebugText("Inventory");
		for (int i=0; i < GameData::GetInventoryItemCount(); i++) {
			TextBlitter::AddDebugText("[g]" + GameData::GetInventoryItemNameByIndex(i, true) + "[w]");
		}
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

void VulkanBackEnd::cmd_BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
}

GLFWwindow* VulkanBackEnd::GetWindow() {
    GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();

	return _window;
}

std::string errorString(VkResult errorCode) {
    switch (errorCode) {
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}
