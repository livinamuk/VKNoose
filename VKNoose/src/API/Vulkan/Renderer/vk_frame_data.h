#pragma once
#include <cstdint>

struct VulkanFrameData {
	struct Buffers {
		uint64_t inventoryCameraData = 0;
		uint64_t inventoryInstances = 0;
		uint64_t inventoryLights = 0;
		uint64_t sceneCameraData = 0;
		uint64_t sceneInstances = 0;
		uint64_t sceneLights = 0;
		uint64_t uiInstances = 0;
		uint64_t deviceAddressTable = 0;
	} buffers;

	struct TLAS {
		uint64_t scene = 0;
		uint64_t inventory = 0;
	} tlas;

	uint64_t dynamicDescriptorSet = 0;
};