/*#include "vulkan.h"
#include "VkBootstrap.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Vulkan {

	VkInstance _instance;
	VkDevice _device;
	VkPhysicalDevice _phyicalDevice;
	VkDebugUtilsMessengerEXT _debugMessenger;
	GLFWwindow* _window;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	vkb::Instance _bootstrapInstance;
	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	VmaAllocator _allocator;
	VkPhysicalDeviceProperties _physicalDeviceProperties;

	VkExtent2D _currentWindowSize = { 512 , 288 };
	VkExtent2D _windowedSize = { 512 * 4, 288 * 4 };
	VkExtent2D _presentSize = { 512, 288 };
	bool _frameBufferResized = false;
	bool _enableValidationLayers = true;


	// Ray tracing

	//RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
	//uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr) const;
	VkPhysicalDeviceMemoryProperties _memoryProperties;
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
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  _rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		bool isComplete() {
			return graphicsFamily.has_value();
		}
	};

	const std::vector<const char*> _validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void InitWindow();
	void CreateInstance();
	void SetupDebugMessenger();
	bool CheckValidationLayerSupport();
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	void SelectPhysicalDevice();

	void Vulkan::Init() {

		InitWindow();
		CreateInstance();
		SelectPhysicalDevice();
	}

	void Vulkan::CleanUp() {
		vkDestroyInstance(_instance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
		vmaDestroyAllocator(_allocator);
	}

	void InitWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		_window = glfwCreateWindow(_currentWindowSize.width, _currentWindowSize.height, "Fuck", nullptr, nullptr);
		glfwSetWindowUserPointer(_window, nullptr);
		glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
	}

	void CreateInstance() {

		if (_enableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		vkb::InstanceBuilder builder;
		builder.set_app_name("Example Vulkan Application");
		builder.request_validation_layers(_enableValidationLayers);
		builder.use_default_debug_messenger();
		builder.require_api_version(1, 3, 0);

		_bootstrapInstance = builder.build().value();
		_instance = _bootstrapInstance.instance;
		_debugMessenger = _bootstrapInstance.debug_messenger;

		glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);
	}

	void SelectPhysicalDevice() {

		//use vkbootstrap to select a gpu. 
		vkb::PhysicalDeviceSelector selector{ _bootstrapInstance };

		selector.add_required_extension(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);					// Hides shader warnings for unused varibles
		selector.add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);				// Dynamic rendering
		selector.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);			// Ray tracing related extensions required by this sample
		selector.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);			// Ray tracing related extensions required by this sample
		selector.add_required_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);			// Required by VK_KHR_acceleration_structure
		selector.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);		// Required by VK_KHR_acceleration_structure
		selector.add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);				// Required by VK_KHR_acceleration_structure
		selector.add_required_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);						// Required for VK_KHR_ray_tracing_pipeline
		selector.add_required_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);			// Required by VK_KHR_spirv_1_4
		selector.add_required_extension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);	// aftermath
		selector.add_required_extension(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);		// aftermath

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = nullptr;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

		VkPhysicalDeviceFeatures features = {};
		features.samplerAnisotropy = true;
		features.shaderInt64 = true;
		selector.set_required_features(features);

		VkPhysicalDeviceVulkan12Features features12 = {};
		features12.shaderSampledImageArrayNonUniformIndexing = true;
		features12.runtimeDescriptorArray = true;
		features12.descriptorBindingVariableDescriptorCount = true;
		features12.descriptorBindingPartiallyBound = true;
		features12.descriptorIndexing = true;
		features12.bufferDeviceAddress = true;
		selector.set_required_features_12(features12);

		VkPhysicalDeviceVulkan13Features features13 = {};
		features13.maintenance4 = true;
		features13.dynamicRendering = true;
		features13.pNext = &enabledAccelerationStructureFeatures;	// you probably want to confirm this chaining of pNexts works when shit goes wrong.
		selector.set_required_features_13(features13);

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_surface(_surface)
			.select()
			.value();

		vkb::DeviceBuilder deviceBuilder { physicalDevice };

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &_memoryProperties);	// store the memory properties for some ray tracing stuff later

		// Query available device extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
		// Check if extension is supported
		bool maintenance4ExtensionSupported = false;
		for (const auto& extension : availableExtensions) {
			if (std::string(extension.extensionName) == VK_KHR_MAINTENANCE_4_EXTENSION_NAME) {
				maintenance4ExtensionSupported = true;
				std::cout << "VK_KHR_maintenance4 is supported\n";
				break;
			}
		}
		// Specify the extensions to enable
		std::vector<const char*> enabledExtensions = {
			VK_KHR_MAINTENANCE_4_EXTENSION_NAME
		};
		if (!maintenance4ExtensionSupported) {
			throw std::runtime_error("Required extension not supported");
		}

		vkb::Device vkbDevice = deviceBuilder.build().value();
		_device = vkbDevice.device;
		_physicalDevice = physicalDevice.physical_device;
		_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		//initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _physicalDevice;
		allocatorInfo.device = _device;
		allocatorInfo.instance = _instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocatorInfo, &_allocator);
		vkGetPhysicalDeviceProperties(_physicalDevice, &_physicalDeviceProperties);

		auto instanceVersion = VK_API_VERSION_1_0;
		auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
		if (vkEnumerateInstanceVersion) {
			vkEnumerateInstanceVersion(&instanceVersion);
		}

		// 3 macros to extract version info
		uint32_t major = VK_VERSION_MAJOR(instanceVersion);
		uint32_t minor = VK_VERSION_MINOR(instanceVersion);
		uint32_t patch = VK_VERSION_PATCH(instanceVersion);

		std::cout << "Vulkan: " << major << "." << minor << "." << patch << "\n\n";

		if (false) {
			uint32_t deviceExtensionCount = 0;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
			std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());
			std::cout << "Available device extensions:\n";
			for (const auto& extension : deviceExtensions) {
				std::cout << ' ' << extension.extensionName << "\n";
			}
			std::cout << "\n";
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
			std::cout << "Available instance extensions:\n";
			for (const auto& extension : extensions) {
				std::cout << ' ' << extension.extensionName << "\n";
			}
			std::cout << "\n";
		}

		// Get the ray tracing and accelertion structure related function pointers required by this sample
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(_device, "vkGetBufferDeviceAddressKHR"));
		vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device, "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(_device, "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(_device, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(_device, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(_device, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(_device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(_device, "vkCreateRayTracingPipelinesKHR"));

		// Debug marker shit
		vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(_device, "vkSetDebugUtilsObjectNameEXT"));

		// Get ray tracing pipeline properties, which will be used later on in the sample
		_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 deviceProperties2{};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &_rayTracingPipelineProperties;
		vkGetPhysicalDeviceProperties2(_physicalDevice, &deviceProperties2);

		// Get acceleration structure properties, which will be used later on in the sample
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &accelerationStructureFeatures;
		vkGetPhysicalDeviceFeatures2(_physicalDevice, &deviceFeatures2);
	}

	// Callbacks
	void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		_frameBufferResized = true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	bool CheckValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		for (const char* layerName : _validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}
	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);
		return indices.isComplete();
	}
}*/