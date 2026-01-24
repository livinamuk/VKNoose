#pragma once
#include "vk_types.h"

#include "API/Vulkan/Types/vk_shader.h"
#include <string>
#include <vector>

struct HellRaytracer {		
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	AllocatedBufferOLD raygenShaderBindingTable;
	AllocatedBufferOLD missShaderBindingTable;
	AllocatedBufferOLD hitShaderBindingTable;
	VkShaderModule rayGenShader = nullptr;
	VkShaderModule rayMissShader = nullptr;
	VkShaderModule closestHitShader = nullptr;
	VkShaderModule rayshadowMissShader = nullptr;
	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};


	void SetShaders(const std::string& rayGen, const std::vector<std::string>& miss, const std::vector<std::string>& hit);

	void CreatePipeline(VkDevice device, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t maxRecursionDepth);
	void Cleanup(VkDevice device, VmaAllocator allocator);
	void DestroyShaders(VkDevice device);
	void CreateShaderBindingTable(VkDevice device, VmaAllocator allocator, VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties);

	uint64_t get_buffer_device_address(VkDevice device, VkBuffer buffer);
	uint32_t alignedSize(uint32_t value, uint32_t alignment);
};
