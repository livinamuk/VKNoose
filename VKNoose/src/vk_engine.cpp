#include "vk_engine.h"
#include <chrono> 
#include <fstream> 
#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_tools.h"
#include "VkBootstrap.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "Util.h"

#include "Game/AssetManager.h"
#include "Game/Scene.h"

#define NOOSE_PI 3.14159265359f

#ifdef NDEBUG
const bool enableValidationLayers2 = false;
#else
const bool enableValidationLayers2 = true;
#endif

const bool printAvaliableExtensions = false;
const bool enableValidationLayers = true;

//we want to immediately abort when there is an error. In normal engines this would give an error message to the user, or perform a dump of state.
using namespace std;
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

void VulkanEngine::init()
{	
	_gameData.player.m_position = glm::vec3(0, 0, -2);

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	_window = glfwCreateWindow(_windowExtent.width, _windowExtent.height, "Fuck", nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

	AssetManager::LoadAssets();

	init_vulkan();
	init_swapchain();
	create_render_targets();
	create_command_buffers();

	init_sync_structures();
	init_descriptors();
	load_shaders();
	create_pipelines();
	create_pipelines_2();
	load_images();
	upload_meshes();
	init_scene();

	//std::cout << "rayTracingPipelineProperties.shaderGroupBaseAlignment is: " << rayTracingPipelineProperties.shaderGroupBaseAlignment << "\n";

	Scene::Init();
	init_raytracing();






	/*
	VkDescriptorImageInfo	descriptorImageInfos[TEXTURE_ARRAY_SIZE];

	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
		descriptorImageInfos[i].sampler = nullptr;
		descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfos[i].imageView = AssetManager::GetTexture(0)->imageView; // fill with dummy
	}

	for (uint32_t i = 0; i < AssetManager::GetNumberOfTextures(); ++i) {
		descriptorImageInfos[i].sampler = nullptr;
		descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfos[i].imageView = AssetManager::GetTexture(i)->imageView;
	}
	*/


	Input::Init(_windowExtent.width, _windowExtent.height);
	Audio::Init();
}

void VulkanEngine::cleanup_shaders()
{
	vkDestroyShaderModule(_device, _meshVertShader, nullptr);
	vkDestroyShaderModule(_device, _colorMeshShader, nullptr);
	vkDestroyShaderModule(_device, _texturedMeshShader, nullptr);
	vkDestroyShaderModule(_device, _text_blitter_vertex_shader, nullptr);
	vkDestroyShaderModule(_device, _text_blitter_fragment_shader, nullptr);
	vkDestroyShaderModule(_device, _rayGenShader, nullptr);
	vkDestroyShaderModule(_device, _rayMissShader, nullptr);
	vkDestroyShaderModule(_device, _closestHitShader, nullptr);
	vkDestroyShaderModule(_device, _rayshadowMissShader, nullptr);
}

void VulkanEngine::cleanup()
{
	//make sure the gpu has stopped doing its things
	vkDeviceWaitIdle(_device);

	cleanup_shaders();

	// Pipelines
	vkDestroyPipeline(_device, _textblitterPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _textblitterPipelineLayout, nullptr);

	// Command pools
	vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
	vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);

	// Frame data
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
		vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
		vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
		vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
		vmaDestroyBuffer(_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);
		vmaDestroyBuffer(_allocator, _frames[i].objectBuffer._buffer, _frames[i].objectBuffer._allocation);
	}

	// Render targets
	vkDestroyImageView(_device, _renderTargetImageView, nullptr);
	vkDestroyImageView(_device, _depthImageView, nullptr);
	vmaDestroyImage(_allocator, _renderTargetImage._image, _renderTargetImage._allocation);
	vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);

	// Swapchain
	for (int i = 0; i < _swapchainImageViews.size(); i++) {
		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// Mesh buffers
	std::vector<Model*> models = AssetManager::GetAllModels();
	for (Model* model : models) {
		for (auto& mesh : model->_meshes) {
			vmaDestroyBuffer(_allocator, mesh._transformBuffer._buffer, mesh._transformBuffer._allocation);
			vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
			if (mesh._indices.size()) {
				vmaDestroyBuffer(_allocator, mesh._indexBuffer._buffer, mesh._indexBuffer._allocation);
			}	
			vmaDestroyBuffer(_allocator, mesh._accelerationStructure.buffer._buffer, mesh._accelerationStructure.buffer._allocation);
			vkDestroyAccelerationStructureKHR(_device, mesh._accelerationStructure.handle, nullptr);
		}
	}

	// Textures
	for (int i = 0; i < AssetManager::GetNumberOfTextures(); i++) {
		vmaDestroyImage(_allocator, AssetManager::GetTexture(i)->image._image, AssetManager::GetTexture(i)->image._allocation);
		vkDestroyImageView(_device, AssetManager::GetTexture(i)->imageView, nullptr);
	}

	// Descriptors
	vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);
	vkDestroyDescriptorSetLayout(_device, _objectSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, _globalSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, _singleTextureSetLayout, nullptr);
	vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

	// new destroys
	vkDestroyDescriptorSetLayout(_device, _textureArrayDescriptorLayout, nullptr);
	vkDestroySampler(_device, _sampler, nullptr);

	cleanup_raytracing();

	vmaDestroyAllocator(_allocator);

	// pipeline etc
	/*for (auto& it : _materials) {
		Material* material = &it.second;
		vkDestroyPipeline(_device, material->pipeline, nullptr);
		vkDestroyPipelineLayout(_device, material->pipelineLayout, nullptr);
	}*/

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyDevice(_device, nullptr);

	vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
	vkDestroyInstance(_instance, nullptr);
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void VulkanEngine::init_raytracing()
{
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

	// Get ray tracing pipeline properties, which will be used later on in the sample
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(_chosenGPU, &deviceProperties2);

	// Get acceleration structure properties, which will be used later on in the sample
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &accelerationStructureFeatures;
	vkGetPhysicalDeviceFeatures2(_chosenGPU, &deviceFeatures2);

	init_rt_scene();
//	createBottomLevelAccelerationStructure();
	createTopLevelAccelerationStructure();

	createStorageImage();
	createUniformBuffer();
	createRayTracingPipeline();
	createShaderBindingTable();
	createDescriptorSets();
}

void VulkanEngine::cleanup_raytracing()
{
	// RT cleanup
	vmaDestroyBuffer(_allocator, _rtVertexBuffer._buffer, _rtVertexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _rtIndexBuffer._buffer, _rtIndexBuffer._allocation);
	vmaDestroyBuffer(_allocator, _topLevelAS.buffer._buffer, _topLevelAS.buffer._allocation);
	vkDestroyAccelerationStructureKHR(_device, _topLevelAS.handle, nullptr);
	vmaDestroyBuffer(_allocator, _frames[0]._uboBuffer_rayTracing._buffer, _frames[0]._uboBuffer_rayTracing._allocation);
	vmaDestroyBuffer(_allocator, _frames[1]._uboBuffer_rayTracing._buffer, _frames[1]._uboBuffer_rayTracing._allocation);
	vmaDestroyImage(_allocator, _storageImage.image._image, _storageImage.image._allocation);
	vkDestroyImageView(_device, _storageImage.view, nullptr);
	vkDestroyDescriptorPool(_device, _rtDescriptorPool, nullptr); 
	vmaDestroyBuffer(_allocator, _raygenShaderBindingTable._buffer, _raygenShaderBindingTable._allocation);
	vmaDestroyBuffer(_allocator, _missShaderBindingTable._buffer, _missShaderBindingTable._allocation);
	vmaDestroyBuffer(_allocator, _hitShaderBindingTable._buffer, _hitShaderBindingTable._allocation);
	vmaDestroyBuffer(_allocator, _rtObjectDescBuffer._buffer, _rtObjectDescBuffer._allocation);
	vkDestroyDescriptorSetLayout(_device, _rtDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, _rtObjectDescDescriptorSetLayout, nullptr);
	vkDestroyPipeline(_device, _rtPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _rtPipelineLayout, nullptr); 
}

void VulkanEngine::create_command_buffers()
{
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		//allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._commandBuffer));
	}

	//create command pool for upload context
	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);

	//create command buffer for upload context
	VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_uploadContext._commandBuffer));
}

void VulkanEngine::update()
{
	for (int j = 0; j < _renderables.size(); j++) {
		RenderObject& object = _renderables[j];
		float a = 0.05;
		if (object.spin) {
			object.transform.rotation.x += a;
			object.transform.rotation.y += a;
			object.transform.rotation.z += a;
		}
	}

	Scene::Update();


	int width, height;

	glfwGetFramebufferSize(_window, &width, &height);




	//camera projection
	if (Input::RightMouseDown()) {
		_cameraZoom -= 0.085f;
	}
	else {
		_cameraZoom += 0.085f;
	}
	_cameraZoom = std::min(1.0f, _cameraZoom);
	_cameraZoom = std::max(0.7f, _cameraZoom);
}

void VulkanEngine::draw()
{
	//std::cout << "Drawing frame: " << _frameNumber << "\n";

	// Skip if window is miniized
	int width, height;
	glfwGetFramebufferSize(_window, &width, &height);
	while (width == 0 || height == 0) {
		return;
	}

	vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000);
	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex);

	// Handle resize
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_dynamic_swapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkCommandBuffer commandBuffer = _frames[_frameNumber % FRAME_OVERLAP]._commandBuffer;


	vkDeviceWaitIdle(_device);

	result = vkResetFences(_device, 1, &get_current_frame()._renderFence);
	if (result != VK_SUCCESS) {
		std::cout << "ResetFences failed with: " << result << "\n";
	}

	//VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));
	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

	//bool traceDemRays = true;
	//if (traceDemRays) {
		build_rt_command_buffers(swapchainImageIndex);
	//}
	//else {
	//	build_command_buffers(swapchainImageIndex);
	//}

	VkSubmitInfo submit = vkinit::submit_info(&commandBuffer);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

	VkPresentInfoKHR presentInfo = vkinit::present_info();
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;
	result = vkQueuePresentKHR(_graphicsQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _frameBufferResized) {
		_frameBufferResized = false;
		recreate_dynamic_swapchain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	_frameNumber++;
}

void VulkanEngine::run()
{
	auto currentTime = std::chrono::high_resolution_clock::now();

	while (!_shouldClose && !glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			_shouldClose = true;
		}

		if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			_selectedShader += 1;
			if (_selectedShader > 1) {
				_selectedShader = 0;
			}
		}

		Input::Update(_window);
		Audio::Update();
		Scene::Update();
		TextBlitter::Update();

		static double lastTime = glfwGetTime();
		double deltaTime = glfwGetTime() - lastTime;
		lastTime = glfwGetTime();

		_gameData.player.UpdateMovement(deltaTime);
		_gameData.player.UpdateMouselook();
		_gameData.player.UpdateCamera();


		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime =
			std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;


		int width, height;
		glfwGetFramebufferSize(_window, &width, &height);
		while (width == 0 || height == 0) {
		//	return;
		}


		if (Input::KeyPressed(HELL_KEY_1)) {
			//TextBlitter::Type("I don't feel comfortable in this place.");
			TextBlitter::Type("I don't feel comfortable here.");
			Audio::PlayAudio("UI_Select.wav", 0.9f);
		}
		if (Input::KeyPressed(HELL_KEY_2)) {
			TextBlitter::Type("My wife, I haven't seen her.");
			Audio::PlayAudio("UI_Select.wav", 0.9f);
		}
		if (Input::KeyPressed(HELL_KEY_3)) {
			TextBlitter::Type("");
		}

		if (Input::KeyPressed(HELL_KEY_F)) {
			static bool _windowed = true;
			_windowed = !_windowed;
			if (!_windowed) {
				GLFWmonitor* monitor = glfwGetPrimaryMonitor();
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				_windowExtent = { (unsigned int)mode->width , (unsigned int)mode->height };
				glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->width, mode->refreshRate);
				glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				recreate_dynamic_swapchain();
				_frameBufferResized = false;
			}
			else {
				glfwSetWindowMonitor(_window, nullptr, 0, 0, 1700, 900, 0);
				glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				recreate_dynamic_swapchain();
				_frameBufferResized = false;
			}
		}

		if (Input::KeyPressed(HELL_KEY_H)) {
			hotload_shaders();
		}

		update();
		draw();
	}
}

FrameData& VulkanEngine::get_current_frame()
{
	return _frames[_frameNumber % FRAME_OVERLAP];
}


FrameData& VulkanEngine::get_last_frame()
{
	return _frames[(_frameNumber - 1) % 2];
}



void VulkanEngine::init_vulkan()
{
	vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(enableValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance 
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);
	//SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	//use vkbootstrap to select a gpu. 
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	// Hides shader warnings for unused varibles
	selector.add_required_extension(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
	// Dynamic rendering
	selector.add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	// Ray tracing related extensions required by this sample
	selector.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	selector.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	// Required by VK_KHR_acceleration_structure
	selector.add_required_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	selector.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	selector.add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	// Required for VK_KHR_ray_tracing_pipeline
	selector.add_required_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	// Required by VK_KHR_spirv_1_4
	selector.add_required_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	// aftermath
	selector.add_required_extension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
	selector.add_required_extension(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);


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


	//create the final vulkan device

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	// store these for some ray tracing stuff.
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &_memoryProperties);





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
	// Create a VkDeviceCreateInfo structure
	/*VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();
	*/



	vkb::Device vkbDevice = deviceBuilder.build().value();


	// Get the VkDevice handle used in the rest of a vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	rayTracingPipelineProperties = {};
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(_chosenGPU, &deviceProperties2);

	// use vkbootstrap to get a Graphics queue
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	//initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	vmaCreateAllocator(&allocatorInfo, &_allocator);

	for (int i = 0; i < 10; i++) {
		//	std::cout << "                                 bbb\n";
	}


	_mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(_allocator);
		});

	vkGetPhysicalDeviceProperties(_chosenGPU, &_gpuProperties);

	//std::cout << "The gpu has a minimum buffer alignement of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;

	for (int i = 0; i < 10; i++) {
		//std::cout << "hellooo\n";
	}

	auto instanceVersion = VK_API_VERSION_1_0;
	auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
	if (vkEnumerateInstanceVersion) {
		vkEnumerateInstanceVersion(&instanceVersion);
	}

	// 3 macros to extract version info
	uint32_t major = VK_VERSION_MAJOR(instanceVersion);
	uint32_t minor = VK_VERSION_MINOR(instanceVersion);
	uint32_t patch = VK_VERSION_PATCH(instanceVersion);

	cout << "Vulkan: " << major << "." << minor << "." << patch << "\n\n";

	if (printAvaliableExtensions) {
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
}

void VulkanEngine::init_swapchain()
{
	int width;
	int height;
	glfwGetFramebufferSize(_window, &width, &height);
	_windowExtent.width = width;
	_windowExtent.height = height;

	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

	VkSurfaceFormatKHR format;
	format.colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	format.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	//swapchainBuilder.set_desired_format(format);

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		//use vsync present mode
		.set_desired_format(format)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) // added so you can blit into the swapchain
		.build()
		.value();


	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	_swachainImageFormat = vkbSwapchain.image_format;

	//depth image size will match the window
	VkExtent3D depthImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	//hardcoding the depth format to 32 bit float
	_depthFormat = VK_FORMAT_D32_SFLOAT;

	//the depth image will be a image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, _renderTargetExtent);

	//for the depth image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

	//build a image-view for the depth image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);;

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));
}

void VulkanEngine::create_render_targets()
{
	//renderTargetExtent.width = _windowExtent.width;
	//renderTargetExtent.height = _windowExtent.height;

	//VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageCreateInfo img_info = vkinit::image_create_info(
		format,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		_renderTargetExtent);

	VmaAllocationCreateInfo img_allocinfo = {};
	img_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	img_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &img_info, &img_allocinfo, &_renderTargetImage._image, &_renderTargetImage._allocation, nullptr);

	//build a image-view for the depth image to use for rendering
	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, _renderTargetImage._image, VK_IMAGE_ASPECT_COLOR_BIT);;

	VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &_renderTargetImageView));
}

void VulkanEngine::recreate_dynamic_swapchain()
{
	std::cout << "Recreating swapchain...\n";

	while (_windowExtent.width == 0 || _windowExtent.height == 0) {
		int width, height;
		glfwGetFramebufferSize(_window, &width, &height);
		_windowExtent.width = width;
		_windowExtent.height = height;
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(_device);

	for (int i = 0; i < _swapchainImages.size(); i++) {
		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
	vkDestroyImageView(_device, _depthImageView, nullptr);
	vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);

	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	init_swapchain();
}


void VulkanEngine::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
	}
	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
	VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
}

void VulkanEngine::hotload_shaders()
{
	std::cout << "Hotloading shaders...\n";

	vkDeviceWaitIdle(_device);

	cleanup_shaders();

	//vkDestroySampler(_device, _sampler, nullptr);

	// pipeline etc
	/*for (auto& it : _materials) {
		Material* material = &it.second;
		vkDestroyPipeline(_device, material->pipeline, nullptr);
		vkDestroyPipelineLayout(_device, material->pipelineLayout, nullptr);
	}*/
	vkDestroyPipeline(_device, _textblitterPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _textblitterPipelineLayout, nullptr);

	//vmaDestroyBuffer(_allocator, _raygenShaderBindingTable._buffer, _raygenShaderBindingTable._allocation);
	//vmaDestroyBuffer(_allocator, _missShaderBindingTable._buffer, _missShaderBindingTable._allocation);
	//vmaDestroyBuffer(_allocator, _hitShaderBindingTable._buffer, _hitShaderBindingTable._allocation);
	vkDestroyDescriptorSetLayout(_device, _rtDescriptorSetLayout, nullptr);
	//vkDestroyDescriptorSetLayout(_device, _rtDescriptorSetLayout_1, nullptr);
	vkDestroyPipeline(_device, _rtPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _rtPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(_device, _rtObjectDescDescriptorSetLayout, nullptr);

	load_shaders();
	create_pipelines();
	create_pipelines_2();

	// ray tracing pipeline shit
	createRayTracingPipeline();

	init_scene();
}


void VulkanEngine::load_shaders()
{
	load_shader(_device, "tri_mesh_ssbo.vert", VK_SHADER_STAGE_VERTEX_BIT, &_meshVertShader);
	load_shader(_device, "default_lit.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_colorMeshShader);
	load_shader(_device, "textured_lit.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_texturedMeshShader);

	load_shader(_device, "text_blitter.vert", VK_SHADER_STAGE_VERTEX_BIT, &_text_blitter_vertex_shader);
	load_shader(_device, "text_blitter.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_text_blitter_fragment_shader);

	_rtShaderGroups.clear();
	_rtShaderStages.clear();

	// Ray generation group
	{
		load_shader(_device, "raygen.rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR, &_rayGenShader);
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = 0;// static_cast<uint32_t>(_rtShaderStages.size());
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		_rtShaderGroups.push_back(shaderGroup);

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		pipelineShaderStageCreateInfo.module = _rayGenShader;
		pipelineShaderStageCreateInfo.pNext = nullptr;
		pipelineShaderStageCreateInfo.pName = "main";
		_rtShaderStages.emplace_back(pipelineShaderStageCreateInfo);
	}

	// Miss group
	{
		load_shader(_device, "miss.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR, &_rayMissShader);
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = 1;// static_cast<uint32_t>(_rtShaderStages.size());
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		_rtShaderGroups.push_back(shaderGroup);

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		pipelineShaderStageCreateInfo.module = _rayMissShader;
		pipelineShaderStageCreateInfo.pNext = nullptr;
		pipelineShaderStageCreateInfo.pName = "main";
		_rtShaderStages.emplace_back(pipelineShaderStageCreateInfo);

		// Second shader for shadows
		load_shader(_device, "shadow.rmiss", VK_SHADER_STAGE_MISS_BIT_KHR, &_rayshadowMissShader);
		shaderGroup.generalShader = 2;// static_cast<uint32_t>(_rtShaderStages.size());
		_rtShaderGroups.push_back(shaderGroup);

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo2 = {};
		pipelineShaderStageCreateInfo2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo2.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		pipelineShaderStageCreateInfo2.module = _rayshadowMissShader;
		pipelineShaderStageCreateInfo2.pNext = nullptr;
		pipelineShaderStageCreateInfo2.pName = "main";
		_rtShaderStages.emplace_back(pipelineShaderStageCreateInfo2);

	}

	// Closest hit group
	{
		load_shader(_device, "closesthit.rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, &_closestHitShader);
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = 3;// static_cast<uint32_t>(_rtShaderStages.size());
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		_rtShaderGroups.push_back(shaderGroup);

		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		pipelineShaderStageCreateInfo.module = _closestHitShader;
		pipelineShaderStageCreateInfo.pNext = nullptr;
		pipelineShaderStageCreateInfo.pName = "main";
		_rtShaderStages.emplace_back(pipelineShaderStageCreateInfo);
	}
}


void VulkanEngine::create_pipelines()
{
	
}



void VulkanEngine::create_pipelines_2()
{
	VkDescriptorSetLayout texturedSetLayouts[] = { _objectSetLayout, _textureArrayDescriptorLayout };

	VertexInputDescription vertexDescription = Vertex::get_vertex_description_position_and_texcoords_only();

	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = texturedSetLayouts;

		VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_textblitterPipelineLayout));

		PipelineBuilder pipelineBuilder;
		pipelineBuilder._pipelineLayout = _textblitterPipelineLayout;
		pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
		pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
		pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
		pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
		pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
		pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
		pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
		pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
		pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
		pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, _text_blitter_vertex_shader));
		pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, _text_blitter_fragment_shader));
		//pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _closestHitShader));

		_textblitterPipeline = pipelineBuilder.build_dynamic_rendering_pipeline(_device, &_swachainImageFormat, _depthFormat);
	}
}

VkPipeline PipelineBuilder::build_dynamic_rendering_pipeline(VkDevice device, const VkFormat* swapchainFormat, VkFormat depthFormat)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineColorBlendAttachmentState att_state0 = {};
	att_state0.colorWriteMask = 0xF;
	att_state0.blendEnable = VK_TRUE;
	att_state0.colorBlendOp = VK_BLEND_OP_ADD;
	att_state0.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	att_state0.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	att_state0.alphaBlendOp = VK_BLEND_OP_ADD;
	att_state0.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	att_state0.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &att_state0;

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	//build the actual pipeline
	//we now use all of the info structs we have been writing into into this one to create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = _shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDynamicState = &dynamicState;

	// New create info to define color, depth and stencil attachments at pipeline create time
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &format;// swapchainFormat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
	pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;// depthFormat;
	// Chain into the pipeline creat einfo
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;

	//its easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		std::cout << "failed to create pipline\n";
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	else
	{
		return newPipeline;
	}
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
	//make viewport state from our stored viewport and scissor.
		//at the moment we wont support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	//viewportState.pViewports = &_viewport;
	viewportState.scissorCount = 1;
	//viewportState.pScissors = &_scissor;


	VkPipelineColorBlendAttachmentState att_state0 = {};
	att_state0.colorWriteMask = 0xF;
	att_state0.blendEnable = VK_TRUE;
	att_state0.colorBlendOp = VK_BLEND_OP_ADD;
	att_state0.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	att_state0.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	att_state0.alphaBlendOp = VK_BLEND_OP_ADD;
	att_state0.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	att_state0.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;



	//setup dummy color blending. We arent using transparent objects yet
	//the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &att_state0;// _colorBlendAttachment;



	std::vector<VkDynamicState> dynamicStates = {
		  VK_DYNAMIC_STATE_VIEWPORT,
		  VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();


	//build the actual pipeline
	//we now use all of the info structs we have been writing into into this one to create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = _shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	//
	pipelineInfo.pDynamicState = &dynamicState;

	//its easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(
		device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		std::cout << "failed to create pipline\n";
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	else
	{
		return newPipeline;
	}
}


void VulkanEngine::draw_quad(Transform transform, Texture* texture)
{

}



void VulkanEngine::upload_meshes() {
	std::vector<Model*> models = AssetManager::GetAllModels();
	for (Model* model : models) {
		for (auto& mesh : model->_meshes) {
			upload_mesh(mesh);
		}
	}
}

void VulkanEngine::build_bottom_level_acceleration_structures() {
	std::vector<Model*> models = AssetManager::GetAllModels();
	for (Model* model : models) {
		for (auto& mesh : model->_meshes) {
			mesh._accelerationStructure = createBottomLevelAccelerationStructure(&mesh);
		}
	}
}


void VulkanEngine::load_texture(std::string filepath)
{
	Texture texture;
	AssetManager::load_image_from_file(*this, filepath.c_str(), texture, VkFormat::VK_FORMAT_R8G8B8A8_SRGB, true);
	AssetManager::AddTexture(texture);
}

void VulkanEngine::load_images()
{
	// Get the width and height of each character, used for text blitting. Probably move this into TextBlitter::Init()
	TextBlitter::_charExtents.clear();
	for (size_t i = 1; i <= 90; i++) {
		load_texture("res/textures/char_" + std::to_string(i) + ".png");
		TextBlitter::_charExtents.push_back({ AssetManager::GetTexture(i - 1)->_width, AssetManager::GetTexture(i - 1)->_height });
	}

	AssetManager::LoadTextures(*this); // Loads everything in res/textures, excludes those already loaded above. Later you will use this.
	AssetManager::BuildMaterials();

	VkDescriptorImageInfo	descriptorImageInfos[TEXTURE_ARRAY_SIZE];

	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
		descriptorImageInfos[i].sampler = nullptr;
		descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfos[i].imageView = AssetManager::GetTexture(0)->imageView; // fill with dummy
	}

	for (uint32_t i = 0; i < AssetManager::GetNumberOfTextures(); ++i) {
		descriptorImageInfos[i].sampler = nullptr;
		descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfos[i].imageView = AssetManager::GetTexture(i)->imageView;
	}

	VkWriteDescriptorSet writes[2];

	VkDescriptorImageInfo samplerInfo = {};
	samplerInfo.sampler = _sampler;

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstBinding = 0;
	writes[0].dstArrayElement = 0;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	writes[0].descriptorCount = 1;
	writes[0].dstSet = _textureArrayDescriptorSet;
	writes[0].pBufferInfo = 0;
	writes[0].pImageInfo = &samplerInfo;

	writes[1] = {};
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstBinding = 1;
	writes[1].dstArrayElement = 0;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	writes[1].descriptorCount = TEXTURE_ARRAY_SIZE;
	writes[1].pBufferInfo = 0;
	writes[1].dstSet = _textureArrayDescriptorSet;
	writes[1].pImageInfo = descriptorImageInfos;

	vkUpdateDescriptorSets(_device, 2, writes, 0, nullptr);

	//std::cout << "Loaded " << _loadedTextures.size() << " textures\n";
}

void VulkanEngine::upload_mesh(Mesh& mesh)
{
	//std::cout << mesh._filename << " has " << mesh._vertices.size() << " vertices and " << mesh._indices.size() << " indices " << "\n";
	// Vertices
	{
		const size_t bufferSize = mesh._vertices.size() * sizeof(Vertex);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

		void* data;
		vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
		memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
		vmaUnmapMemory(_allocator, stagingBuffer._allocation);

		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;
		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage =
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;

		VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &mesh._vertexBuffer._buffer, &mesh._vertexBuffer._allocation, nullptr));

		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy);
			});

		vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
	}

	// Indices
	if (mesh._indices.size())
	{
		const size_t bufferSize = mesh._indices.size() * sizeof(uint32_t);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

		void* data;
		vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
		memcpy(data, mesh._indices.data(), mesh._indices.size() * sizeof(uint32_t));
		vmaUnmapMemory(_allocator, stagingBuffer._allocation);

		VkBufferCreateInfo indexBufferInfo = {};
		indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexBufferInfo.pNext = nullptr;
		indexBufferInfo.size = bufferSize;
		indexBufferInfo.usage =
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;

		VK_CHECK(vmaCreateBuffer(_allocator, &indexBufferInfo, &vmaallocInfo, &mesh._indexBuffer._buffer, &mesh._indexBuffer._allocation, nullptr));

		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._indexBuffer._buffer, 1, &copy);
			});

		vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
	}
	// Transforms
	{
		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f
		};

		const size_t bufferSize = sizeof(transformMatrix);;
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		void* data;
		vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
		memcpy(data, &transformMatrix, bufferSize);
		vmaUnmapMemory(_allocator, stagingBuffer._allocation);
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;// VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &mesh._transformBuffer._buffer, &mesh._transformBuffer._allocation, nullptr));
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._transformBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
	}
}



void VulkanEngine::init_scene()
{
	_renderables.clear();
	/*

	_renderables.push_back(map);

	for (int x = -15; x <= -4; x++) {
		for (int y = -5; y <= 5; y++) {
			for (int z = -10; z <= 10; z++) {

				//if (abs(x) < 2 && abs(z) < 3 && y < 3)
				//	continue;

				RenderObject tri;
				tri.mesh = get_mesh("triangle");
				tri.material = get_material("defaultmesh");

				tri.transform.position = glm::vec3(x * 3, y * 3, z * 3);
				tri.transform.scale = glm::vec3(0.5);
				tri.transform.rotation.x += Util::RandomFloat(-NOOSE_PI, NOOSE_PI);
				tri.transform.rotation.y += Util::RandomFloat(-NOOSE_PI, NOOSE_PI);
				tri.transform.rotation.z += Util::RandomFloat(-NOOSE_PI, NOOSE_PI);
				tri.spin = true;
				_renderables.push_back(tri);
			}
		}
	}


	Material* texturedMat = get_material("texturedmesh");

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_singleTextureSetLayout;

	vkAllocateDescriptorSets(_device, &allocInfo, &texturedMat->textureSet);

	VkDescriptorImageInfo imageBufferInfo;
	imageBufferInfo.sampler = _sampler;
	//imageBufferInfo.imageView = _loadedTextures["lost_empire-RGBA"].imageView;
	imageBufferInfo.imageView = _loadedTextures[0].imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet texture1 = {};
	texture1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	texture1.pNext = nullptr;
	texture1.dstBinding = 0;
	texture1.dstSet = texturedMat->textureSet;
	texture1.descriptorCount = 1;
	texture1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture1.pImageInfo = &imageBufferInfo;
	vkUpdateDescriptorSets(_device, 1, &texture1, 0, nullptr);
	*/






	/*	VkDescriptorImageInfo	descriptorImageInfos[TEXTURE_ARRAY_SIZE];
		for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
			descriptorImageInfos[i].sampler = nullptr;
			descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptorImageInfos[i].imageView = _loadedTextures[i].imageView;
		}

		VkWriteDescriptorSet setWrites[2];
		setWrites[1] = {};
		setWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrites[1].dstBinding = 1;
		setWrites[1].dstArrayElement = 0;
		setWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		setWrites[1].descriptorCount = TEXTURE_ARRAY_SIZE;
		setWrites[1].pBufferInfo = 0;
		setWrites[1].dstSet = texturedMat->textureSet;//demoData.descriptorSet;
		setWrites[1].pImageInfo = descriptorImageInfos;// demoData.descriptorImageInfos;


		*/










}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
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

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
		&newBuffer._buffer,
		&newBuffer._allocation,
		nullptr));

	return newBuffer;
}

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}



void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = _uploadContext._commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);

	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void VulkanEngine::init_descriptors()
{
	// Good as place as any for a turret
	//VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 12.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.maxAnisotropy = _gpuProperties.limits.maxSamplerAnisotropy;;
	samplerInfo.anisotropyEnable = VK_TRUE;
	vkCreateSampler(_device, &samplerInfo, nullptr, &_sampler);

	//create a descriptor pool that will hold 10 uniform buffers
	std::vector<VkDescriptorPoolSize> sizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = 10;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool);

	VkDescriptorSetLayoutBinding cameraBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VkDescriptorSetLayoutBinding sceneBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);


	VkDescriptorSetLayoutBinding bindings[] = { cameraBind,sceneBind };

	VkDescriptorSetLayoutCreateInfo setinfo = {};
	setinfo.bindingCount = 2;
	setinfo.flags = 0;
	setinfo.pNext = nullptr;
	setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setinfo.pBindings = bindings;

	vkCreateDescriptorSetLayout(_device, &setinfo, nullptr, &_globalSetLayout);

	VkDescriptorSetLayoutBinding objectBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set2info = {};
	set2info.bindingCount = 1;
	set2info.flags = 0;
	set2info.pNext = nullptr;
	set2info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set2info.pBindings = &objectBind;

	vkCreateDescriptorSetLayout(_device, &set2info, nullptr, &_objectSetLayout);


	VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set3info = {};
	set3info.bindingCount = 1;
	set3info.flags = 0;
	set3info.pNext = nullptr;
	set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set3info.pBindings = &textureBind;

	vkCreateDescriptorSetLayout(_device, &set3info, nullptr, &_singleTextureSetLayout);


	// Texture array
	{
		// Layout 

		VkDescriptorSetLayoutBinding textureArrayLayoutBindings[2];
		textureArrayLayoutBindings[0] = vkinit::descriptor_set_layout_binding2(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, 1);
		textureArrayLayoutBindings[1] = vkinit::descriptor_set_layout_binding2(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1, TEXTURE_ARRAY_SIZE);

		VkDescriptorSetLayoutCreateInfo setinfo = {};
		setinfo.bindingCount = 2;
		setinfo.flags = 0;
		setinfo.pNext = nullptr;
		setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setinfo.pBindings = textureArrayLayoutBindings;

		VkResult res = vkCreateDescriptorSetLayout(_device, &setinfo, nullptr, &_textureArrayDescriptorLayout);
		if (res != VK_SUCCESS) {
			std::cout << "Error creating desc set layout\n";
		}

		// Set

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_textureArrayDescriptorLayout;
		vkAllocateDescriptorSets(_device, &allocInfo, &_textureArrayDescriptorSet);
	}












	const size_t sceneParamBufferSize = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));

	_sceneParameterBuffer = create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);


	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		_frames[i].cameraBuffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		//const int MAX_OBJECTS = 10000;
		_frames[i].objectBuffer = create_buffer(sizeof(GPUObjectData) * MAX_RENDER_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_globalSetLayout;

		vkAllocateDescriptorSets(_device, &allocInfo, &_frames[i].globalDescriptor);

		VkDescriptorSetAllocateInfo objectSetAlloc = {};
		objectSetAlloc.pNext = nullptr;
		objectSetAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		objectSetAlloc.descriptorPool = _descriptorPool;
		objectSetAlloc.descriptorSetCount = 1;
		objectSetAlloc.pSetLayouts = &_objectSetLayout;

		vkAllocateDescriptorSets(_device, &objectSetAlloc, &_frames[i].objectDescriptor);

		VkDescriptorBufferInfo cameraInfo;
		cameraInfo.buffer = _frames[i].cameraBuffer._buffer;
		cameraInfo.offset = 0;
		cameraInfo.range = sizeof(GPUCameraData);

		VkDescriptorBufferInfo sceneInfo;
		sceneInfo.buffer = _sceneParameterBuffer._buffer;
		sceneInfo.offset = 0;
		sceneInfo.range = sizeof(GPUSceneData);

		VkDescriptorBufferInfo objectBufferInfo;
		objectBufferInfo.buffer = _frames[i].objectBuffer._buffer;
		objectBufferInfo.offset = 0;
		objectBufferInfo.range = sizeof(GPUObjectData) * MAX_RENDER_OBJECTS;


		VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[i].globalDescriptor, &cameraInfo, 0);

		VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, _frames[i].globalDescriptor, &sceneInfo, 1);

		VkWriteDescriptorSet objectWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _frames[i].objectDescriptor, &objectBufferInfo, 0);

		VkWriteDescriptorSet setWrites[] = { cameraWrite,sceneWrite,objectWrite };

		vkUpdateDescriptorSets(_device, 3, setWrites, 0, nullptr);
	}


	_mainDeletionQueue.push_function([&]() {

		vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

		vkDestroyDescriptorSetLayout(_device, _objectSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _globalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _singleTextureSetLayout, nullptr);

		vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vmaDestroyBuffer(_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);

			vmaDestroyBuffer(_allocator, _frames[i].objectBuffer._buffer, _frames[i].objectBuffer._allocation);
		}
		});

}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto enginePointer = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
	enginePointer->_frameBufferResized = true;
}

uint32_t VulkanEngine::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
{
	for (uint32_t i = 0; i < _memoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
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


void VulkanEngine::createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferCreateInfo, &vmaallocInfo, &accelerationStructure.buffer._buffer, &accelerationStructure.buffer._allocation, nullptr));

	/*VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(_device, accelerationStructure.buffer._buffer, &memoryRequirements);

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(_device, &memoryAllocateInfo, nullptr, &accelerationStructure.memory));
	VK_CHECK(vkBindBufferMemory(_device, accelerationStructure.buffer._buffer, accelerationStructure.memory, 0));*/

	//VmaAllocationCreateInfo vmaallocInfo = {};
	//vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	//vmaCreateBuffer(_allocator, &bufferCreateInfo, &vmaallocInfo, &accelerationStructure.buffer._buffer, &allocation, nullptr);
}

uint64_t VulkanEngine::get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(_device, &bufferDeviceAI);
}

RayTracingScratchBuffer VulkanEngine::createScratchBuffer(VkDeviceSize size)
{
	RayTracingScratchBuffer scratchBuffer{};

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	VK_CHECK(vmaCreateBuffer(_allocator, &bufferCreateInfo, &vmaallocInfo, &scratchBuffer.handle._buffer, &scratchBuffer.handle._allocation, nullptr));

	VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
	bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAddressInfo.buffer = scratchBuffer.handle._buffer;
	scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(_device, &bufferDeviceAddressInfo);

	return scratchBuffer;
}

void VulkanEngine::init_rt_scene()
{
	std::vector<Model*> models = AssetManager::GetAllModels();
	std::vector<Mesh*> meshes;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;


	build_bottom_level_acceleration_structures();


	// Make a flat array, aka vector, of all meshes in the scene
	for (GameObject& gameObject : Scene::GetGameObjects()) {
		for (Mesh& mesh : gameObject._model->_meshes) {
			meshes.push_back(&mesh);
		}
	}

	for (int i = 0; i < meshes.size(); i++) {
		Mesh* mesh = meshes[i];
		for (int j = 0; j < mesh->_vertices.size(); j++) {
			vertices.push_back(mesh->_vertices[j]);
		}
		for (int j = 0; j < mesh->_indices.size(); j++) {
			indices.push_back(mesh->_indices[j]);
		}
	}

	// Vertices
	{
		const size_t bufferSize = vertices.size() * sizeof(Vertex);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		void* data;
		vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
		memcpy(data, vertices.data(), bufferSize);
		vmaUnmapMemory(_allocator, stagingBuffer._allocation);
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;
		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &_rtVertexBuffer._buffer, &_rtVertexBuffer._allocation, nullptr));
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtVertexBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

		int objectCount = 1;
	}

	// Indices
	{
		const size_t bufferSize = indices.size() * sizeof(uint32_t);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		void* data;
		vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
		memcpy(data, indices.data(), bufferSize);
		vmaUnmapMemory(_allocator, stagingBuffer._allocation);
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = bufferSize;
		bufferInfo.usage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_rtIndexBuffer._buffer, &_rtIndexBuffer._allocation, nullptr));
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtIndexBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
	}

	_vertexBufferDeviceAddress.deviceAddress = get_buffer_device_address(_rtVertexBuffer._buffer);
	_indexBufferDeviceAddress.deviceAddress = get_buffer_device_address(_rtIndexBuffer._buffer);
}
	 
AccelerationStructure VulkanEngine::createBottomLevelAccelerationStructure(Mesh* mesh)
{
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = get_buffer_device_address(mesh->_vertexBuffer._buffer);
	indexBufferDeviceAddress.deviceAddress = get_buffer_device_address(mesh->_indexBuffer._buffer);
	transformBufferDeviceAddress.deviceAddress = get_buffer_device_address(mesh->_transformBuffer._buffer);

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
	accelerationStructureGeometry.geometry.triangles.maxVertex = mesh->_vertices.size();
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
	accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	const uint32_t numTriangles = mesh->_indices.size() / 3;
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		_device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	AccelerationStructure bottomLevelAS{};
	createAccelerationStructureBuffer(bottomLevelAS, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer._buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(_device, &accelerationStructureCreateInfo, nullptr, &bottomLevelAS.handle);

	// Create a small scratch buffer used during build of the bottom level acceleration structure
	RayTracingScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	immediate_submit([=](VkCommandBuffer cmd) {
		vkCmdBuildAccelerationStructuresKHR(
			cmd,
			1,
			&accelerationBuildGeometryInfo,
			accelerationBuildStructureRangeInfos.data());
		});

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.handle;
	bottomLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(_device, &accelerationDeviceAddressInfo);

	vmaDestroyBuffer(_allocator, scratchBuffer.handle._buffer, scratchBuffer.handle._allocation);

	return bottomLevelAS;
}


void VulkanEngine::createTopLevelAccelerationStructure()
{
	size_t numMeshes = 0;
	std::vector<GameObject>& gameObjects = Scene::GetGameObjects();

	for (GameObject& gameObject : Scene::GetGameObjects()) {
		for (int i = 0; i < gameObject._model->_meshes.size(); i++) {
			numMeshes++;
		}
	}

	// create the BLAS instances within TLAS, one for each game object
	std::vector<VkAccelerationStructureInstanceKHR> instances(numMeshes, VkAccelerationStructureInstanceKHR{});
	
	int meshCounter = 0;
	for (GameObject& gameObject : Scene::GetGameObjects()) {
		for (int i = 0; i < gameObject._model->_meshes.size(); i++) {

			Mesh& mesh = gameObject._model->_meshes[i];

			VkTransformMatrixKHR transformMatrix = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f };

			glm::mat4 modelMatrix = gameObject._worldTransform.to_mat4();
			modelMatrix = glm::transpose(modelMatrix);

			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 4; y++) {
					transformMatrix.matrix[x][y] = modelMatrix[x][y];
				}
			}

			VkAccelerationStructureInstanceKHR& instance = instances[meshCounter];
			instance.transform = transformMatrix;
			instance.instanceCustomIndex = meshCounter;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instance.accelerationStructureReference = mesh._accelerationStructure.deviceAddress;

			meshCounter++;
		}
	}
	const size_t bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);	
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	AllocatedBuffer stagingBuffer;

	VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
	void* data;
	vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
	memcpy(data, instances.data(), bufferSize);
	vmaUnmapMemory(_allocator, stagingBuffer._allocation);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &_rtInstancesBuffer._buffer, &_rtInstancesBuffer._allocation, nullptr));

	immediate_submit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtInstancesBuffer._buffer, 1, &copy);
		});
	vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);


	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = get_buffer_device_address(_rtInstancesBuffer._buffer);
	//instanceDataDeviceAddresses.push_back(instanceDataDeviceAddress);

	VkAccelerationStructureGeometryInstancesDataKHR tlasInstancesInfo = {};
	tlasInstancesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	tlasInstancesInfo.data.deviceAddress = get_buffer_device_address(_rtInstancesBuffer._buffer);

	VkAccelerationStructureGeometryKHR  accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances = tlasInstancesInfo;

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &accelerationStructureGeometry;

	const uint32_t numInstances = static_cast<uint32_t>(instances.size());
	//std::cout << "numInstances: " << numInstances << "\n";

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		_device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildInfo,
		&numInstances,
		&accelerationStructureBuildSizesInfo);

	createAccelerationStructureBuffer(_topLevelAS, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = _topLevelAS.buffer._buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(_device, &accelerationStructureCreateInfo, nullptr, &_topLevelAS.handle);

	// Create a small scratch buffer used during build of the top level acceleration structure
	RayTracingScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = _topLevelAS.handle;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numInstances;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };


	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	immediate_submit([=](VkCommandBuffer cmd) {
		vkCmdBuildAccelerationStructuresKHR(
			cmd,
			1,
			&accelerationBuildGeometryInfo,
			accelerationBuildStructureRangeInfos.data());
		});

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = _topLevelAS.handle;
	_topLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(_device, &accelerationDeviceAddressInfo);

	vmaDestroyBuffer(_allocator, scratchBuffer.handle._buffer, scratchBuffer.handle._allocation);
	
	// deal with this
	vmaDestroyBuffer(_allocator, _rtInstancesBuffer._buffer, _rtInstancesBuffer._allocation);
}

void VulkanEngine::updateTLASdescriptorSet() {

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &_topLevelAS.handle;

	VkWriteDescriptorSet accelerationStructureWrite{};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = _rtDescriptorSet;
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	VkDescriptorImageInfo storageImageDescriptor{};
	storageImageDescriptor.imageView = _storageImage.view;
	storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	//VkDescriptorBufferInfo vertexBufferDescriptor{ chairMesh->_vertexBuffer._buffer, 0, VK_WHOLE_SIZE };
	//VkDescriptorBufferInfo indexBufferDescriptor{ chairMesh->_indexBuffer._buffer, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo vertexBufferDescriptor{ _rtVertexBuffer._buffer, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo indexBufferDescriptor{ _rtIndexBuffer._buffer, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet resultImageWrite = vkinit::write_descriptor_set(_rtDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = { accelerationStructureWrite };
	vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}


void VulkanEngine::createStorageImage()
{
	VkImageCreateInfo img_info = {};
	img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	img_info.imageType = VK_IMAGE_TYPE_2D;
	img_info.extent.width = _renderTargetExtent.width;
	img_info.extent.height = _renderTargetExtent.height;
	img_info.extent.depth = 1;
	img_info.mipLevels = 1;
	img_info.arrayLayers = 1;
	img_info.samples = VK_SAMPLE_COUNT_1_BIT;
	img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_info.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

	VmaAllocationCreateInfo img_allocinfo = {};
	img_allocinfo.usage = VMA_MEMORY_USAGE_AUTO;
	img_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vmaCreateImage(_allocator, &img_info, &img_allocinfo, &_storageImage.image._image, &_storageImage.image._allocation, nullptr);

	VkImageViewCreateInfo view_info = VkImageViewCreateInfo{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = VK_FORMAT_R8G8B8A8_UNORM; //_swachainImageFormat; Sascha used swapchain image format, but im having a clash between it (VK_FORMAT_B8G8R8A8_SRGB) and the above format
	view_info.subresourceRange = {};
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.image = _storageImage.image._image;
	//view_info.flags = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

	VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &_storageImage.view));

	immediate_submit([=](VkCommandBuffer cmd) {
		vktools::setImageLayout(cmd, _storageImage.image._image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		});
}

void VulkanEngine::createUniformBuffer()
{
	/*VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = sizeof(_uniformData);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_AUTO;
	//allocinfo.flags = VMA_MEMORY_USAGE_CPU_TO_GPU; // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; these r from the SASCHA example
	*/
	//VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &allocinfo, &_uboBuffer._buffer, &_uboBuffer._allocation, nullptr));

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo.size = sizeof(RTUniformData);
	createInfo.pNext = nullptr;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	for (int i = 0; i < FRAME_OVERLAP; i++) {

		VK_CHECK(vmaCreateBuffer(_allocator, &createInfo, &vmaallocInfo, &_frames[i]._uboBuffer_rayTracing._buffer, &_frames[i]._uboBuffer_rayTracing._allocation, nullptr));
		//_frames[i]._uboBuffer_rayTracing = create_buffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		_frames[i]._uboBuffer_rayTracing_descriptor.buffer = _frames[i]._uboBuffer_rayTracing._buffer;
		_frames[i]._uboBuffer_rayTracing_descriptor.range = VK_WHOLE_SIZE;
		_frames[i]._uboBuffer_rayTracing_descriptor.offset = 0;

	}
	//vkMapMemory(_device, memory, offset, size, 0, &mapped);

	//VK_CHECK_RESULT(ubo.map());

	updateUniformBuffers();
}

void VulkanEngine::updateUniformBuffers()
{
	glm::mat4 projection = glm::perspective(_cameraZoom, 1700.f / 900.f, 0.01f, 100.0f);
	glm::mat4 view = _gameData.player.m_camera.GetViewMatrix();

	_uniformData.projInverse = glm::inverse(projection);
	_uniformData.viewInverse = glm::inverse(view);
	_uniformData.viewPos = glm::vec4(_gameData.player.m_camera.m_viewPos, 1.0);
	_uniformData.vertexSize = sizeof(Vertex);

	void* data;
	vmaMapMemory(_allocator, get_current_frame()._uboBuffer_rayTracing._allocation, &data);
	memcpy(data, &_uniformData, sizeof(RTUniformData));
	vmaUnmapMemory(_allocator, get_current_frame()._uboBuffer_rayTracing._allocation);
}

void VulkanEngine::createRayTracingPipeline()
{
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
	accelerationStructureLayoutBinding.binding = 0;
	accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	accelerationStructureLayoutBinding.descriptorCount = 1;
	accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
	resultImageLayoutBinding.binding = 1;
	resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageLayoutBinding.descriptorCount = 1;
	resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	VkDescriptorSetLayoutBinding uniformBufferBinding{};
	uniformBufferBinding.binding = 2;
	uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferBinding.descriptorCount = 1;
	uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding vertexBufferBinding{};
	vertexBufferBinding.binding = 3;
	vertexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertexBufferBinding.descriptorCount = 1;
	vertexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding indexBufferBinding{};
	indexBufferBinding.binding = 4;
	indexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	indexBufferBinding.descriptorCount = 1;
	indexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings({
		accelerationStructureLayoutBinding,
		resultImageLayoutBinding,
		uniformBufferBinding,
		vertexBufferBinding,
		indexBufferBinding
		});

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorSetLayoutCreateInfo, nullptr, &_rtDescriptorSetLayout));


	// vertex data binding
	{
		VkDescriptorSetLayoutBinding objectDescBinding{};
		objectDescBinding.binding = 0;
		objectDescBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		objectDescBinding.descriptorCount = 1;
		objectDescBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(1);
		descriptorSetLayoutCreateInfo.pBindings = &objectDescBinding;
		VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorSetLayoutCreateInfo, nullptr, &_rtObjectDescDescriptorSetLayout));
	}


	/*VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCI_1{};
	descriptorSetlayoutCI_1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetlayoutCI_1.bindingCount = static_cast<uint32_t>(bindings2.size());
	descriptorSetlayoutCI_1.pBindings = bindings2.data();
	VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorSetlayoutCI_1, nullptr, &_rtDescriptorSetLayout_1));*/

	//VkDescriptorSetLayout rtDescriptorSetLayouts[] = { _rtDescriptorSetLayout_0, _rtDescriptorSetLayout_1 };
	VkDescriptorSetLayout rtDescriptorSetLayouts[] = { _rtDescriptorSetLayout, _textureArrayDescriptorLayout, _rtObjectDescDescriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pipelineLayoutCreateInfo.pSetLayouts = rtDescriptorSetLayouts;
	VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &_rtPipelineLayout));

	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
	rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCI.stageCount = static_cast<uint32_t>(_rtShaderStages.size());
	rayTracingPipelineCI.pStages = _rtShaderStages.data();
	rayTracingPipelineCI.groupCount = static_cast<uint32_t>(_rtShaderGroups.size());
	rayTracingPipelineCI.pGroups = _rtShaderGroups.data();
	rayTracingPipelineCI.maxPipelineRayRecursionDepth = 2;
	rayTracingPipelineCI.layout = _rtPipelineLayout;
	//rayTracingPipelineCI.basePipelineIndex = 0;
	VK_CHECK(vkCreateRayTracingPipelinesKHR(_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &_rtPipeline));
}

uint32_t alignedSize(uint32_t value, uint32_t alignment) {
	return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanEngine::createShaderBindingTable()
{
	const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	const uint32_t groupCount = static_cast<uint32_t>(_rtShaderGroups.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(_device, _rtPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

	const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.usage = bufferUsageFlags;// VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	//createInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; // this might not actually be the solution, you added it on a whim kinda
	createInfo.size = handleSize;
	createInfo.pNext = nullptr;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vmaallocInfo.preferredFlags = memoryUsageFlags;

	VK_CHECK(vmaCreateBuffer(_allocator, &createInfo, &vmaallocInfo, &_raygenShaderBindingTable._buffer, &_raygenShaderBindingTable._allocation, nullptr));
	VK_CHECK(vmaCreateBuffer(_allocator, &createInfo, &vmaallocInfo, &_missShaderBindingTable._buffer, &_missShaderBindingTable._allocation, nullptr));
	VK_CHECK(vmaCreateBuffer(_allocator, &createInfo, &vmaallocInfo, &_hitShaderBindingTable._buffer, &_hitShaderBindingTable._allocation, nullptr));

	// Copy handles
	void* data;
	vmaMapMemory(_allocator, _raygenShaderBindingTable._allocation, &data);
	memcpy(data, shaderHandleStorage.data(), handleSize);
	vmaUnmapMemory(_allocator, _raygenShaderBindingTable._allocation);

	vmaMapMemory(_allocator, _missShaderBindingTable._allocation, &data);
	memcpy(data, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
	vmaUnmapMemory(_allocator, _missShaderBindingTable._allocation);

	vmaMapMemory(_allocator, _hitShaderBindingTable._allocation, &data);
	memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
	vmaUnmapMemory(_allocator, _hitShaderBindingTable._allocation);
}



void VulkanEngine::createDescriptorSets()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100}
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vkinit::descriptor_pool_create_info(poolSizes, 3);
	VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo, nullptr, &_rtDescriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vkinit::descriptor_set_allocate_info(_rtDescriptorPool, &_rtDescriptorSetLayout, 1);
	VK_CHECK(vkAllocateDescriptorSets(_device, &descriptorSetAllocateInfo, &_rtDescriptorSet));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo2 = vkinit::descriptor_set_allocate_info(_rtDescriptorPool, &_rtObjectDescDescriptorSetLayout, 1);
	VK_CHECK(vkAllocateDescriptorSets(_device, &descriptorSetAllocateInfo2, &_rtObjectDescDescriptorSet));

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &_topLevelAS.handle;

	VkWriteDescriptorSet accelerationStructureWrite{};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = _rtDescriptorSet;
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	VkDescriptorImageInfo storageImageDescriptor{};
	storageImageDescriptor.imageView = _storageImage.view;
	storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	//VkDescriptorBufferInfo vertexBufferDescriptor{ chairMesh->_vertexBuffer._buffer, 0, VK_WHOLE_SIZE };
	//VkDescriptorBufferInfo indexBufferDescriptor{ chairMesh->_indexBuffer._buffer, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo vertexBufferDescriptor{ _rtVertexBuffer._buffer, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo indexBufferDescriptor{ _rtIndexBuffer._buffer, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet resultImageWrite = vkinit::write_descriptor_set(_rtDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
	VkWriteDescriptorSet uniformBufferWrite = vkinit::write_descriptor_set(_rtDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &get_current_frame()._uboBuffer_rayTracing_descriptor);
	VkWriteDescriptorSet vertexBufferWrite = vkinit::write_descriptor_set(_rtDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &vertexBufferDescriptor);
	VkWriteDescriptorSet indexBufferWrite = vkinit::write_descriptor_set(_rtDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &indexBufferDescriptor);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		accelerationStructureWrite,
		resultImageWrite,
		uniformBufferWrite,
		vertexBufferWrite,
		indexBufferWrite
	};
	vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

	int vertexOffest = 0;
	int indexOffset = 0;
	std::vector<ObjDesc> objectDescs;

	for (GameObject& gameObject : Scene::GetGameObjects()) {

		for (int i = 0; i < gameObject._model->_meshes.size(); i++) {

			Material* material = gameObject._meshMaterials[i];

			if (!material) {
				std::cout << "Game object with model \"" << gameObject._model->_meshes[i]._filename << "\" is missing it's material for mesh " << i << "\n";
			}

			ObjDesc objectDesc;
			objectDesc.worldMatrix = gameObject._worldTransform.to_mat4();
			objectDesc.basecolorIndex = material->_basecolor;
			objectDesc.normalIndex = material->_normal;
			objectDesc.rmaIndex = material->_rma;
			objectDesc.vertexOffset = vertexOffest;
			objectDesc.indexOffset = indexOffset;
			objectDescs.push_back(objectDesc);

			Mesh& mesh = gameObject._model->_meshes[i];
			vertexOffest += mesh._vertices.size();
			indexOffset += mesh._indices.size();

		}
	}
	



		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		createInfo.size = sizeof(ObjDesc) * objectDescs.size();
		createInfo.pNext = nullptr;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		vmaallocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VK_CHECK(vmaCreateBuffer(_allocator, &createInfo, &vmaallocInfo, &_rtObjectDescBuffer._buffer, &_rtObjectDescBuffer._allocation, nullptr));

		// Copy handles
		void* data;
		vmaMapMemory(_allocator, _rtObjectDescBuffer._allocation, &data);
		memcpy(data, objectDescs.data(), sizeof(ObjDesc) * objectDescs.size());
		vmaUnmapMemory(_allocator, _rtObjectDescBuffer._allocation);

		//  Now update the descriptor set with a handle to that buffer
		VkDescriptorBufferInfo descriptorBufferInfo = { _rtObjectDescBuffer._buffer, 0, VK_WHOLE_SIZE };
		VkWriteDescriptorSet write = vkinit::write_descriptor_set(_rtObjectDescDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &descriptorBufferInfo);
		vkUpdateDescriptorSets(_device, 1, &write, 0, VK_NULL_HANDLE);
}

/*uint64_t getBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}*/

void VulkanEngine::build_rt_command_buffers(int swapchainIndex)
{
	vmaDestroyBuffer(_allocator, _topLevelAS.buffer._buffer, _topLevelAS.buffer._allocation);
	vkDestroyAccelerationStructureKHR(_device, _topLevelAS.handle, nullptr);
	createTopLevelAccelerationStructure();
	updateTLASdescriptorSet();

	// First update the ubo matrices
	{
		glm::mat4 viewMatrix = _gameData.player.m_camera.GetViewMatrix();
		glm::mat4 projectionMatrix = glm::perspective(_cameraZoom, 1700.f / 900.f, 0.01f, 100.0f);
		projectionMatrix[1][1] *= -1;

		_uniformData.projInverse = glm::inverse(projectionMatrix);
		_uniformData.viewInverse = glm::inverse(viewMatrix);
		_uniformData.viewPos = glm::vec4(_gameData.player.m_camera.m_viewPos, 1.0f);
		_uniformData.vertexSize = sizeof(Vertex);

		void* data;
		vmaMapMemory(_allocator, get_current_frame()._uboBuffer_rayTracing._allocation, &data);
		memcpy(data, &_uniformData, sizeof(RTUniformData));
		vmaUnmapMemory(_allocator, get_current_frame()._uboBuffer_rayTracing._allocation);
	}

	// Now fill your command buffer
	int32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	VkCommandBuffer commandBuffer = _frames[frameIndex]._commandBuffer;
	VkCommandBufferBeginInfo cmdBufInfo = vkinit::command_buffer_begin_info();

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

	//Setup the buffer regions pointing to the shaders in our shader binding table
	const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	raygenShaderSbtEntry.deviceAddress = get_buffer_device_address(_raygenShaderBindingTable._buffer);
	raygenShaderSbtEntry.stride = handleSizeAligned;
	raygenShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	missShaderSbtEntry.deviceAddress = get_buffer_device_address(_missShaderBindingTable._buffer);
	missShaderSbtEntry.stride = handleSizeAligned;
	missShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitShaderSbtEntry.deviceAddress = get_buffer_device_address(_hitShaderBindingTable._buffer);
	hitShaderSbtEntry.stride = handleSizeAligned;
	hitShaderSbtEntry.size = handleSizeAligned;

	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

	//Dispatch the ray tracing commands
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rtPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rtPipelineLayout, 0, 1, &_rtDescriptorSet, 0, 0);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rtPipelineLayout, 1, 1, &_textureArrayDescriptorSet, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rtPipelineLayout, 2, 1, &_rtObjectDescDescriptorSet, 0, nullptr);

	vkCmdTraceRaysKHR(
		commandBuffer,
		&raygenShaderSbtEntry,
		&missShaderSbtEntry,
		&hitShaderSbtEntry,
		&callableShaderSbtEntry,
		_renderTargetExtent.width,
		_renderTargetExtent.height,
		1);


	// Ray tracing done 


	// Prepare for UI //


	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	{

		VkImageMemoryBarrier imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageBarrier.image = _storageImage.image._image;
		imageBarrier.subresourceRange = range;
		imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
	}

	{

		VkImageMemoryBarrier imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageBarrier.image = _storageImage.image._image;
		imageBarrier.subresourceRange = range;
		imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
	}


	// draw UI //
	VkRenderingAttachmentInfoKHR colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	//colorAttachment.imageView = _swapchainImageViews[swapchainImageIndex];
	colorAttachment.imageView = _storageImage.view;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingAttachmentInfoKHR depthStencilAttachment{};
	depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depthStencilAttachment.imageView = _depthImageView;
	depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.renderArea = { 0, 0, _renderTargetExtent.width, _renderTargetExtent.height };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;
	renderingInfo.pStencilAttachment = nullptr;

	void* objectData;
	vmaMapMemory(_allocator, get_current_frame().objectBuffer._allocation, &objectData);
	GPUObjectData* objectSSBO = (GPUObjectData*)objectData;
	/*for (int j = 0; j < _renderables.size(); j++) {
		objectSSBO[j].modelMatrix = _renderables[j].transform.to_mat4();
	}*/
	for (int i = 0; i < TextBlitter::_objectData.size(); i++) {
		int index = _renderables.size() + i;
		objectSSBO[index] = TextBlitter::_objectData[i];
	}
	vmaUnmapMemory(_allocator, get_current_frame().objectBuffer._allocation);


	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	// Set viewport size
	VkViewport viewport = vkinit::viewport((float)_renderTargetExtent.width, (float)_renderTargetExtent.height, 0.0f, 1.0f);
	VkRect2D scissor = vkinit::rect2D(_renderTargetExtent.width, _renderTargetExtent.height, 0, 0);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//Material* colorMaterial = get_material("texturedmesh");

	Model* model = AssetManager::GetModel("quad");
	Mesh* quadMesh = &model->_meshes[0];
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _textblitterPipeline);
	uint32_t uniform_offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _textblitterPipelineLayout, 0, 1, &get_current_frame().objectDescriptor, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _textblitterPipelineLayout, 1, 1, &_textureArrayDescriptorSet, 0, nullptr);

	int firstCharIndex = _renderables.size();

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &quadMesh->_vertexBuffer._buffer, &offset);

	//we can now draw

	for (int i = 0; i < TextBlitter::_objectData.size(); i++) {
		quadMesh->draw(commandBuffer, firstCharIndex + i);
		//vkCmdDrawIndexed(commandBuffer, quadMesh->_indices.size(), 1, 0, firstCharIndex + i);
	}




	vkCmdEndRendering(commandBuffer);

	// now prepare for to blit from storage image to swapchain


	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageBarrier.image = _storageImage.image._image;
	imageBarrier.subresourceRange = range;
	imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

	VkImageMemoryBarrier swapChainBarrier = {};
	swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapChainBarrier.image = _swapchainImages[swapchainIndex];
	swapChainBarrier.subresourceRange = range;
	swapChainBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	swapChainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);




	VkImageBlit region;
	region.srcOffsets[0].x = 0;
	region.srcOffsets[0].y = 0;
	region.srcOffsets[0].z = 0;
	//region.srcOffsets[1].x = _loadedTextures["empire_diffuse"].image._width;
	//region.srcOffsets[1].y = _loadedTextures["empire_diffuse"].image._height;
	region.srcOffsets[1].x = _renderTargetExtent.width;
	region.srcOffsets[1].y = _renderTargetExtent.height;
	region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
	region.dstOffsets[0].x = 0;
	region.dstOffsets[0].y = 0;
	region.dstOffsets[0].z = 0;
	region.dstOffsets[1].x = _windowExtent.width;
	region.dstOffsets[1].y = _windowExtent.height;
	region.dstOffsets[1].z = 1;
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.mipLevel = 0;
	region.dstSubresource.baseArrayLayer = 0;
	region.dstSubresource.layerCount = 1;
	VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	uint32_t regionCount = 1;
	VkFilter filter = VK_FILTER_NEAREST;

	vkCmdBlitImage(commandBuffer, _storageImage.image._image, srcLayout, _swapchainImages[swapchainIndex], dstLayout, regionCount, &region, filter);

	{
		VkImageMemoryBarrier imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageBarrier.image = _storageImage.image._image;
		imageBarrier.subresourceRange = range;
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

		VkImageMemoryBarrier swapChainBarrier = {};
		swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapChainBarrier.image = _swapchainImages[swapchainIndex];
		swapChainBarrier.subresourceRange = range;
		swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		swapChainBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
	}

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}