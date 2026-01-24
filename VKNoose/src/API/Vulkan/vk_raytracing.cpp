#include "vk_raytracing.h"
#include "Renderer/Shader.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"

void HellRaytracer::DestroyShaders(VkDevice device) {
	vkDestroyShaderModule(device, rayGenShader, nullptr);
	vkDestroyShaderModule(device, rayMissShader, nullptr);
	vkDestroyShaderModule(device, closestHitShader, nullptr);
	vkDestroyShaderModule(device, rayshadowMissShader, nullptr);

	rayGenShader = VK_NULL_HANDLE;
	rayMissShader = VK_NULL_HANDLE;
	closestHitShader = VK_NULL_HANDLE;
	rayshadowMissShader = VK_NULL_HANDLE;
}

void HellRaytracer::Cleanup(VkDevice device, VmaAllocator allocator)
{
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	pipeline = VK_NULL_HANDLE;
	pipelineLayout = VK_NULL_HANDLE;

	vmaDestroyBuffer(allocator, raygenShaderBindingTable.m_buffer, raygenShaderBindingTable.m_allocation);
	vmaDestroyBuffer(allocator, missShaderBindingTable.m_buffer, missShaderBindingTable.m_allocation);
	vmaDestroyBuffer(allocator, hitShaderBindingTable.m_buffer, hitShaderBindingTable.m_allocation);

	DestroyShaders(device);
}

void HellRaytracer::SetShaders(const std::string& rayGen, const std::vector<std::string>& miss, const std::vector<std::string>& hit) {
	shaderGroups.clear();
	shaderStages.clear();

	// Ray Generation Stage
	VulkanShader* genShader = VulkanResourceManager::GetShader(rayGen);
	if (genShader) {
		shaderStages.push_back(genShader->GetStageCreateInfo(VK_SHADER_STAGE_RAYGEN_BIT_KHR));

		VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(group);
	}

	// Miss Stages
	for (const std::string& missName : miss) {
		VulkanShader* shader = VulkanResourceManager::GetShader(missName);
		if (!shader) continue;

		shaderStages.push_back(shader->GetStageCreateInfo(VK_SHADER_STAGE_MISS_BIT_KHR));

		VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(group);
	}

	// Hit Groups
	for (const std::string& hitName : hit) {
		VulkanShader* shader = VulkanResourceManager::GetShader(hitName);
		if (!shader) continue;

		shaderStages.push_back(shader->GetStageCreateInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

		VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(group);
	}
}


void HellRaytracer::CreatePipeline(VkDevice device, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t maxRecursionDepth ) {

	if (pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, pipeline, nullptr);
	}	
	if (pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
	rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCI.pStages = shaderStages.data();
	rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCI.pGroups = shaderGroups.data();
	rayTracingPipelineCI.maxPipelineRayRecursionDepth = maxRecursionDepth;
	rayTracingPipelineCI.layout = pipelineLayout;
	vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline);
}

uint64_t HellRaytracer::get_buffer_device_address(VkDevice device, VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}


uint32_t HellRaytracer::alignedSize(uint32_t value, uint32_t alignment) {
	return (value + alignment - 1) & ~(alignment - 1);

}

void HellRaytracer::CreateShaderBindingTable(VkDevice device, VmaAllocator allocator, VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties) {
	const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = alignedSize(handleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	// Get the shader group handles from the pipeline
	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

	// Define buffer usage for SBT
	const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	const VmaAllocationCreateInfo vmaallocInfo = {
		0, VMA_MEMORY_USAGE_AUTO, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0, 0, nullptr, nullptr, 0.0f
	};

	// Helper to create and fill an SBT buffer
	auto createSBTBuffer = [&](AllocatedBufferOLD& buffer, uint32_t groupIndex, uint32_t count, const std::string& debugName) {
		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.size = handleSizeAligned * count;
		createInfo.usage = bufferUsageFlags;

		// Cleanup old buffer if it exists (Crucial for hotloading)
		if (buffer.m_buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(allocator, buffer.m_buffer, buffer.m_allocation);
		}

		VK_CHECK(vmaCreateBuffer(allocator, &createInfo, &vmaallocInfo, &buffer.m_buffer, &buffer.m_allocation, nullptr));

		// Copy handles into the buffer
		void* mappedData;
		vmaMapMemory(allocator, buffer.m_allocation, &mappedData);

		// Copy the handles from our storage based on where they sit in the shaderGroups vector
		memcpy(mappedData, shaderHandleStorage.data() + (groupIndex * handleSizeAligned), handleSize * count);
		vmaUnmapMemory(allocator, buffer.m_allocation);

		// Setup the Device Address Region for vkCmdTraceRaysKHR
		VkStridedDeviceAddressRegionKHR region{};
		region.deviceAddress = get_buffer_device_address(device, buffer.m_buffer);
		region.stride = handleSizeAligned;
		region.size = handleSizeAligned * count;
		return region;
	};

	// Map the groups to the SBT entries
	// Index 0 is always RayGen in our SetShaders logic
	raygenShaderSbtEntry = createSBTBuffer(raygenShaderBindingTable, 0, 1, "SBT_RayGen");

	// Miss shaders start at index 1 and match the size of your input miss vector
	// Calculate the count based on how we populated shaderGroups in SetShaders
	uint32_t missCount = 0;
	uint32_t hitCount = 0;
	for (auto& group : shaderGroups) {
		if (group.type == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR && &group != &shaderGroups[0]) missCount++;
		if (group.type == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR) hitCount++;
	}

	missShaderSbtEntry = createSBTBuffer(missShaderBindingTable, 1, missCount, "SBT_Miss");
	hitShaderSbtEntry = createSBTBuffer(hitShaderBindingTable, 1 + missCount, hitCount, "SBT_Hit");

	// Callable is unused for now
	callableShaderSbtEntry = {};
}