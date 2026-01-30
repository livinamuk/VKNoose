#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Renderer/vk_device_addresses.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"

#include "AssetManagement/Assetmanager.h"

#include "Hell/Core/Logging.h"
#include "Hell/Constants.h"
#include "Hell/Types.h"

namespace VulkanRenderer {
	void CreateFrameData();
	void CreatePipelines();
	void CreateRenderTargets();
	void CreateSamplers();
	void CreateStaticDescriptorSet();
	void LoadShaders();
	void UpdateStaticDescriptorSet();

	VulkanFrameData g_frameData[FRAME_OVERLAP];
	uint32_t g_frameNumber = 0;

	uint64_t g_staticDescriptorSet = 0;

	uint64_t g_vertexBuffer = 0;
	uint64_t g_indexBuffer = 0;
	uint64_t g_transformBuffer = 0;

	bool Init() {
		LoadShaders();
		CreateSamplers();
		CreateRenderTargets();
		CreatePipelines();
		CreateStaticDescriptorSet();
		CreateFrameData();
		UpdateStaticDescriptorSet();
		return true;
	}

	void LoadShaders() {
		VulkanResourceManager::CreateShader("TextBlitter", { "vk_text_blitter.vert", "vk_text_blitter.frag" });
		VulkanResourceManager::CreateShader("SolidColor", { "vk_solid_color.vert", "vk_solid_color.frag" });
		VulkanResourceManager::CreateShader("GBuffer", { "vk_gbuffer.vert", "vk_gbuffer.frag" });
		VulkanResourceManager::CreateShader("Composite", { "vk_composite.vert", "vk_composite.frag" });

		// Path Tracer Raytracing Shaders
		VulkanResourceManager::CreateShader("Path_RayGen", { "path_raygen.rgen" });
		VulkanResourceManager::CreateShader("Path_Miss", { "path_miss.rmiss" });
		VulkanResourceManager::CreateShader("Path_Shadow", { "path_shadow.rmiss" });
		VulkanResourceManager::CreateShader("Path_Hit", { "path_closesthit.rchit" });

		// Mouse Pick Raytracing Shaders
		VulkanResourceManager::CreateShader("Mouse_RayGen", { "mousepick_raygen.rgen" });
		VulkanResourceManager::CreateShader("Mouse_Miss", { "mousepick_miss.rmiss" });
		VulkanResourceManager::CreateShader("Mouse_Hit", { "mousepick_closesthit.rchit" });
	}

	void CreateSamplers() {
		const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();
		int maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		VulkanResourceManager::CreateSampler("Linear", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, maxAnisotropy);
		VulkanResourceManager::CreateSampler("Nearest", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, maxAnisotropy);
	}

	void CreateRenderTargets() {
		// Present resoltion
		uint32_t width = PRESENT_WIDTH;
		uint32_t height = PRESENT_HEIGHT;

		// Raytrace resolution
		int scale = 2;
		uint32_t rtWidth = PRESENT_WIDTH * scale;
		uint32_t rtHeight = PRESENT_HEIGHT * scale;

		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VulkanResourceManager::CreateAllocatedImage("LoadingScreen", 1024, 576, VK_FORMAT_R8G8B8A8_UNORM, usage);

		// Raytracing Storage Images (Using R32G32B32A32_SFLOAT to match shader expectations)
		VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_Color", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
		VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_Normals", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
		VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_BaseColor", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
		VulkanResourceManager::CreateAllocatedImage("RT_SecondHit_Color", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);

		// GBuffer Targets
		VulkanResourceManager::CreateAllocatedImage("GBuffer_BaseColor", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("GBuffer_Normal", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("GBuffer_RMA", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);

		// UI and Display Targets
		VulkanResourceManager::CreateAllocatedImage("LaptopDisplay", LAPTOP_DISPLAY_WIDTH, LAPTOP_DISPLAY_HEIGHT, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("Composite", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("Present", width, height, VK_FORMAT_R8G8B8A8_UNORM, usage);

		// Depth Targets
		VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VulkanResourceManager::CreateAllocatedImage("Depth_Present", width, height, VK_FORMAT_D32_SFLOAT, depthUsage);
		VulkanResourceManager::CreateAllocatedImage("Depth_GBuffer", rtWidth, rtHeight, VK_FORMAT_D32_SFLOAT, depthUsage);

	}

	void CreatePipelines() {
		VkDevice device = VulkanDeviceManager::GetDevice();

	}

	void CreateStaticDescriptorSet() {
		VkDevice device = VulkanBackEnd::GetDevice();

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			{ DESC_IDX_SAMPLERS, VK_DESCRIPTOR_TYPE_SAMPLER, 16, VK_SHADER_STAGE_ALL },
			{ DESC_IDX_TEXTURES, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000, VK_SHADER_STAGE_ALL },
			{ DESC_IDX_UBOS,     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128, VK_SHADER_STAGE_ALL },
			{ DESC_IDX_SSBOS,    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ALL },

			// Grouped Storage Images by Format
			{ DESC_IDX_STORAGE_IMAGES_RGBA32F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
			{ DESC_IDX_STORAGE_IMAGES_RGBA16F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
			{ DESC_IDX_STORAGE_IMAGES_RGBA8,   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },

			// Legacy
			{ DESC_IDX_VERTICES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL },
			{ DESC_IDX_INDICES,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL }
		};

		std::vector<VkDescriptorBindingFlags> flags(bindings.size(), 0);
		flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
		flagsInfo.bindingCount = (uint32_t)flags.size();
		flagsInfo.pBindingFlags = flags.data();

		VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.pNext = &flagsInfo;
		layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings = bindings.data();

		g_staticDescriptorSet = VulkanResourceManager::CreateDescriptorSet(layoutInfo);
	}

	void UpdateStaticDescriptorSet() {
		// currently done in VulkanBackend because of some bullshit with g_vertexBuffer and g_indexBuffer
	}

	void UploadGlobalGeometry() {
		const std::vector<Vertex>& vertices = AssetManager::GetVertices();
		const std::vector<uint32_t>& indices = AssetManager::GetIndices();

		std::vector<Vertex>& vertices2 = AssetManager::GetVertices_TEMPORARY();
		std::vector<uint32_t>& indices2 = AssetManager::GetIndices_TEMPORARY();

		// Define the usage flags once
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		// Create and upload Vertex Buffer
		g_vertexBuffer = VulkanResourceManager::CreateBuffer(vertices.size() * sizeof(Vertex), usage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::UploadBufferData(g_vertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex));

		// Create and upload Index Buffer
		g_indexBuffer = VulkanResourceManager::CreateBuffer(indices.size() * sizeof(uint32_t), usage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::UploadBufferData(g_indexBuffer, indices.data(), indices.size() * sizeof(uint32_t));

		VkBufferUsageFlags transformUsage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		glm::mat3x4 identity(1.0f);

		g_transformBuffer = VulkanResourceManager::CreateBuffer(sizeof(glm::mat3x4), transformUsage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::UploadBufferData(g_transformBuffer, &identity, sizeof(glm::mat3x4));
	}

	void BuildAllBLAS() {
		for (Mesh& mesh : AssetManager::GetMeshes()) {
			if (mesh.indexCount == 0) continue;
			//mesh.m_vulkanAccelerationStructure = VulkanRaytracingManager::CreateBottomLevelAS(&mesh);
		}
	}

	void CreateFrameData() {
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VulkanFrameData& frameData = g_frameData[i];
		
			VkBufferUsageFlags usageUniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VkBufferUsageFlags usageStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;
			VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		
			// Buffers
			frameData.buffers.sceneCameraData = VulkanResourceManager::CreateBuffer(sizeof(CameraData), usageUniform, vmaUsage, vmaFlags);
			frameData.buffers.inventoryCameraData = VulkanResourceManager::CreateBuffer(sizeof(CameraData), usageUniform, vmaUsage, vmaFlags);
			frameData.buffers.uiInstances = VulkanResourceManager::CreateBuffer(sizeof(GPUObjectData2D) * MAX_RENDER_OBJECTS_2D, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.sceneInstances = VulkanResourceManager::CreateBuffer(sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.inventoryInstances = VulkanResourceManager::CreateBuffer(sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.sceneLights = VulkanResourceManager::CreateBuffer(sizeof(LightRenderInfo) * MAX_LIGHTS, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.inventoryLights = VulkanResourceManager::CreateBuffer(sizeof(LightRenderInfo) * 2, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.deviceAddressTable = VulkanResourceManager::CreateBuffer(sizeof(DynamicDeviceAddresses), usageStorage, vmaUsage, vmaFlags);
		
			// TLAS
			frameData.tlas.scene = VulkanResourceManager::CreateAccelerationStructure();
			frameData.tlas.inventory = VulkanResourceManager::CreateAccelerationStructure();

			// Buffer addresses
			VkDescriptorSetLayoutBinding addressTableBinding = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
			std::vector<VkDescriptorSetLayoutBinding> bindings = { addressTableBinding };

			VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			layoutInfo.bindingCount = (uint32_t)bindings.size();
			layoutInfo.pBindings = bindings.data();

			frameData.dynamicDescriptorSet = VulkanResourceManager::CreateDescriptorSet(layoutInfo);
		}
	}

	void UpdateDynamicDescriptorSet() {
		VulkanFrameData& frameData = GetCurrentFrameData();

		DynamicDeviceAddresses addresses;
		addresses.sceneCameraData = VulkanResourceManager::GetBuffer(frameData.buffers.sceneCameraData)->GetDeviceAddress();
		addresses.sceneInstances = VulkanResourceManager::GetBuffer(frameData.buffers.sceneInstances)->GetDeviceAddress();
		addresses.sceneLights = VulkanResourceManager::GetBuffer(frameData.buffers.sceneLights)->GetDeviceAddress();
		addresses.inventoryCameraData = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryCameraData)->GetDeviceAddress();
		addresses.inventoryInstances = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryInstances)->GetDeviceAddress();
		addresses.inventoryLights = VulkanResourceManager::GetBuffer(frameData.buffers.inventoryLights)->GetDeviceAddress();
		addresses.uiInstances = VulkanResourceManager::GetBuffer(frameData.buffers.uiInstances)->GetDeviceAddress();

		VulkanBuffer* addressTableBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.deviceAddressTable);
		addressTableBuffer->UpdateData(&addresses, sizeof(DynamicDeviceAddresses));

		VulkanDescriptorSet* dynamicDescriptorSet = VulkanResourceManager::GetDescriptorSet(frameData.dynamicDescriptorSet);
		dynamicDescriptorSet->WriteBuffer(0, addressTableBuffer->GetBuffer(), sizeof(DynamicDeviceAddresses), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		dynamicDescriptorSet->Update();
	}

	void Cleanup() {
		// Manually cleanup the BLAS for each mesh because they aren't stored in the ResourceManager
		for (Mesh& mesh : AssetManager::GetMeshes()) {
			mesh.m_vulkanAccelerationStructure.Cleanup();
		}
	}

	VulkanBuffer* GetVertexBuffer() {
		return VulkanResourceManager::GetBuffer(g_indexBuffer);
	}

	VulkanBuffer* GetIndexBuffer() {
		return VulkanResourceManager::GetBuffer(g_indexBuffer);
	}

	VulkanFrameData& GetCurrentFrameData() {
		return g_frameData[g_frameNumber % FRAME_OVERLAP];
	}

	VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex) {
		return g_frameData[frameIndex]; // Warning: No bounds check. Remove this whole function when you can!
	}

	uint32_t GetCurrentFrameIndex() {
		return g_frameNumber % FRAME_OVERLAP;
	}

	void IncrementFrame() {
		g_frameNumber++;
	}

	VulkanDescriptorSet& GetStaticDescriptorSet() {
		if (VulkanDescriptorSet* descriptorSet = VulkanResourceManager::GetDescriptorSet(g_staticDescriptorSet)) {
			return *descriptorSet;
		}
		else {
			VulkanDescriptorSet invalid;
			Logging::Error() << "VulkanRenderer::GetStaticDescriptorSet() failed because the VulkanDescriptorSet does not exist\n";
		}
	}
}