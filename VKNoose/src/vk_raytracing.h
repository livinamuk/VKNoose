#pragma once
#include "vk_types.h"

struct HellRaytracer {		
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	AllocatedBuffer raygenShaderBindingTable;
	AllocatedBuffer missShaderBindingTable;
	AllocatedBuffer hitShaderBindingTable;
	VkShaderModule rayGenShader = nullptr;
	VkShaderModule rayMissShader = nullptr;
	VkShaderModule closestHitShader = nullptr;
	VkShaderModule rayshadowMissShader = nullptr;
	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

	void LoadShaders(VkDevice device, std::string rayGenPath, std::string missPath, std::string shadowMissPath, std::string closestHitPath);
	void CreatePipeline(VkDevice device, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t maxRecursionDepth);
	void Cleanup(VkDevice device, VmaAllocator allocator);
	void DestroyShaders(VkDevice device);
	void CreateShaderBindingTable(VkDevice device, VmaAllocator allocator, VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties);

	uint64_t get_buffer_device_address(VkDevice device, VkBuffer buffer);
	uint32_t alignedSize(uint32_t value, uint32_t alignment);

	// This is terrible. You are reloading these function pointers for each Hellraytracter instance
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
	PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
	PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin;
	PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd;
	PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert;
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
};
