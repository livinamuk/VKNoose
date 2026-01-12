#include "vk_backend.h"
#include <chrono> 
#include <fstream> 
#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_tools.h"
#include "Util.h"
 
#include "Game/AssetManager.h"
#include "Game/Scene.h"
#include "Game/Laptop.h"
#include "Renderer/RasterRenderer.h"
#include "Profiler.h"

#include "BackEnd/GLFWIntegration.h"

#include "Managers/vk_device_manager.h"
#include "Managers/vk_instance_manager.h"
#include "Managers/vk_memory_manager.h"

#define NOOSE_PI 3.14159265359f
const bool _printAvaliableExtensions = false;
float _deltaTime;
std::vector<std::string> _loadingText;

RenderTarget _loadingScreenRenderTarget;

namespace VulkanBackEnd {
	VkDevice g_device = VK_NULL_HANDLE;
	VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
	VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
	VkQueue g_graphicsQueue = VK_NULL_HANDLE;
	uint32_t g_graphicsQueueFamily = UINT32_MAX;

	VkDevice GetDevice() { return g_device; }
	VkInstance GetInstance() { return VulkanInstanceManager::GetInstance(); }
	VkSurfaceKHR GetSurface() { return VulkanInstanceManager::GetSurface(); }
	VmaAllocator GetAllocator() { return VulkanMemoryManager::GetAllocator(); }

	const bool g_validationEnabled = true;
}


namespace VulkanBackEnd {
	

    bool VulkanBackEnd::InitMinimum() {


		if (volkInitialize() != VkResult::VK_SUCCESS) {
			throw std::runtime_error("volkInitialize failed");
        }

		if (!VulkanInstanceManager::Init()) return false;

		volkLoadInstance(GetInstance());

		std::cout << "vkEnumeratePhysicalDevices ptr: " << (void*)vkEnumeratePhysicalDevices << "\n";
		if (!vkEnumeratePhysicalDevices) {
			std::cout << "Still null. This TU is not using Volk properly.\n";
			return false;
		}

		//if (!VulkanDeviceManager::Init()) return false;

        SelectPhysicalDevice();

		volkLoadDevice(GetDevice());

		if (!VulkanMemoryManager::Init(g_device, g_physicalDevice)) return false;

        CreateSwapchain();

        load_shader(g_device, "text_blitter.vert", VK_SHADER_STAGE_VERTEX_BIT, &_text_blitter_vertex_shader);
        load_shader(g_device, "text_blitter.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_text_blitter_fragment_shader);

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        _loadingScreenRenderTarget = RenderTarget(g_device, GetAllocator(), format, 512 * 2, 288 * 2, usage, "Loading Screen Render Target");

        create_command_buffers();
        create_sync_structures();
        create_sampler();
        create_descriptors();

        // Create text blitter pipeline and pipeline layout
        _pipelines.textBlitter.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
        _pipelines.textBlitter.PushDescriptorSetLayout(_staticDescriptorSet.layout);
        _pipelines.textBlitter.CreatePipelineLayout(g_device);
        _pipelines.textBlitter.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
        _pipelines.textBlitter.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        _pipelines.textBlitter.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.textBlitter.SetCullModeFlags(VK_CULL_MODE_NONE);
        _pipelines.textBlitter.SetColorBlending(true);
        _pipelines.textBlitter.SetDepthTest(false);
        _pipelines.textBlitter.SetDepthWrite(false);
        _pipelines.textBlitter.Build(g_device, _text_blitter_vertex_shader, _text_blitter_fragment_shader, 1);

        AssetManager::Init();
        AssetManager::LoadFont();
        AssetManager::LoadHardcodedMesh();

        upload_meshes();

        // Sampler
        VkDescriptorImageInfo samplerImageInfo = {};
        samplerImageInfo.sampler = _sampler;
        _staticDescriptorSet.Update(g_device, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);

        // All textures
        VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
        for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
            textureImageInfo[i].sampler = nullptr;
            textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfo[i].imageView = (i < AssetManager::GetNumberOfTextures()) ? AssetManager::GetTexture(i)->imageView : AssetManager::GetTexture(0)->imageView; // Fill with dummy if you excede the amount of textures we loaded off disk. Can't have no junk data.
        }
        _staticDescriptorSet.Update(g_device, 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

        create_buffers();

        bool thereAreFilesToLoad = true;

        VulkanBackEnd::ToggleFullscreen();

        std::cout << "Init::Minimum() complete\n";
        return true;
    }
}


#include <set>


void VulkanBackEnd::SelectPhysicalDevice() {
	VkInstance instance = VulkanInstanceManager::GetInstance();
	VkSurfaceKHR surface = VulkanInstanceManager::GetSurface();

	// Enumerate devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan devices found");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Required extensions
    std::vector<const char*> requiredExtensions = {
        VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
        VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Select first suitable device
    for (auto pd : devices) {
        // Check extensions
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(pd, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> available(extCount);
        vkEnumerateDeviceExtensionProperties(pd, nullptr, &extCount, available.data());

        bool allFound = true;
        for (auto req : requiredExtensions) {
            bool found = false;
            for (auto& av : available) {
                if (strcmp(req, av.extensionName) == 0) { found = true; break; }
            }
            if (!found) { allFound = false; break; }
        }
        if (!allFound) continue;

        // Find queue families
        uint32_t qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> qProps(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &qCount, qProps.data());

        int graphicsFam = -1, presentFam = -1;
        for (uint32_t i = 0; i < qCount; ++i) {
            if (qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFam = i;
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &presentSupport);
            if (presentSupport) presentFam = i;
            if (graphicsFam != -1 && presentFam != -1) break;
        }
        if (graphicsFam == -1 || presentFam == -1) continue;

        // This device is usable
        g_physicalDevice = pd;
        g_graphicsQueueFamily = graphicsFam;

        // Enable features
        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;
        features.shaderInt64 = VK_TRUE;

        VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        features12.runtimeDescriptorArray = VK_TRUE;
        features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        features12.descriptorBindingPartiallyBound = VK_TRUE;
        features12.descriptorIndexing = VK_TRUE;
        features12.bufferDeviceAddress = VK_TRUE;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipeline{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        rtPipeline.rayTracingPipeline = VK_TRUE;

        VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        accel.accelerationStructure = VK_TRUE;
        accel.pNext = &rtPipeline;

        VkPhysicalDeviceVulkan13Features features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        features13.maintenance4 = VK_TRUE;
        features13.dynamicRendering = VK_TRUE;
        features13.pNext = &accel;

        VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        features2.features = features;
        features2.pNext = &features12;
        features12.pNext = &features13;

        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> qInfos;
        std::set<uint32_t> uniqueQ = { (uint32_t)graphicsFam, (uint32_t)presentFam };
        for (uint32_t fam : uniqueQ) {
            VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            qci.queueFamilyIndex = fam;
            qci.queueCount = 1;
            qci.pQueuePriorities = &priority;
            qInfos.push_back(qci);
        }

        VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        dci.queueCreateInfoCount = (uint32_t)qInfos.size();
        dci.pQueueCreateInfos = qInfos.data();
        dci.enabledExtensionCount = (uint32_t)requiredExtensions.size();
        dci.ppEnabledExtensionNames = requiredExtensions.data();
        dci.pNext = &features2;

		VulkanDeviceManager::InitTest(g_physicalDevice);

        if (vkCreateDevice(pd, &dci, nullptr, &g_device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        vkGetDeviceQueue(g_device, graphicsFam, 0, &g_graphicsQueue);
        if (presentFam != graphicsFam) {
            VkQueue presentQueue;
            vkGetDeviceQueue(g_device, presentFam, 0, &presentQueue);
        }




        // Load function pointers (same as before)
        //vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(g_device, "vkGetBufferDeviceAddressKHR"));
        //vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(g_device, "vkCmdBuildAccelerationStructuresKHR"));
        //vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(g_device, "vkBuildAccelerationStructuresKHR"));
        //vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(g_device, "vkCreateAccelerationStructureKHR"));
        //vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(g_device, "vkDestroyAccelerationStructureKHR"));
        //vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(g_device, "vkGetAccelerationStructureBuildSizesKHR"));
        //vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(g_device, "vkGetAccelerationStructureDeviceAddressKHR"));
        //vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(g_device, "vkCmdTraceRaysKHR"));
        //vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(g_device, "vkGetRayTracingShaderGroupHandlesKHR"));
        //vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(g_device, "vkCreateRayTracingPipelinesKHR"));
        //vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(g_device, "vkSetDebugUtilsObjectNameEXT"));

        return; // done, first suitable device
    }

    throw std::runtime_error("No suitable Vulkan device found");

    std::cout << "Selected physical device\n";
}


void VulkanBackEnd::CreateSwapchain() {
	VkInstance instance = VulkanInstanceManager::GetInstance();
	VkSurfaceKHR surface = VulkanInstanceManager::GetSurface();

    GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();



	int width;
	int height;
	glfwGetFramebufferSize(_window, &width, &height);
	_currentWindowExtent.width = width;
	_currentWindowExtent.height = height;

	VkSurfaceFormatKHR format;
	format.colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	format.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

	vkb::SwapchainBuilder swapchainBuilder(g_physicalDevice, g_device, surface);
	swapchainBuilder.set_desired_format(format);
	swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
	swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
	swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
	swapchainBuilder.set_desired_extent(_currentWindowExtent.width, _currentWindowExtent.height);
	swapchainBuilder.set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT); // added so you can blit into the swapchain
	
	vkb::Swapchain vkbSwapchain = swapchainBuilder.build().value();
	g_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();
	_swachainImageFormat = vkbSwapchain.image_format;


    std::cout << "Created swapchain\n";
}


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
		LoadShaders();
	}

	static bool pipelinesCreated = false;
	static bool pipelinesCreatedMessageSeen = false;	
	if (!pipelinesCreatedMessageSeen) {
		pipelinesCreatedMessageSeen = true;
		AddLoadingText("Creating pipelines...");
		return;
	}
	if (!pipelinesCreated) {
		pipelinesCreated = true;
		create_pipelines();;
	}

	static bool renderTargetsCreated = false;
	static bool renderTargetsCreatedMessageSeen = false;
	if (!renderTargetsCreatedMessageSeen) {
		renderTargetsCreatedMessageSeen = true;
		AddLoadingText("Creating render targets...");
		return;
	}
	if (!renderTargetsCreated) {
		renderTargetsCreated = true;
		create_render_targets();
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
		create_top_level_acceleration_structure(Scene::GetMeshInstancesForSceneAccelerationStructure(), _frames[0]._sceneTLAS);
		create_top_level_acceleration_structure(Scene::GetMeshInstancesForInventoryAccelerationStructure(), _frames[0]._inventoryTLAS);
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


void VulkanBackEnd::cleanup_shaders()
{
	vkDestroyShaderModule(g_device, _text_blitter_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _text_blitter_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _solid_color_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _solid_color_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _gbuffer_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _gbuffer_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _blur_horizontal_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _blur_horizontal_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _blur_vertical_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _blur_vertical_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _composite_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _composite_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _denoise_pass_A_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _denoise_pass_A_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _denoise_pass_B_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _denoise_pass_B_fragment_shader, nullptr);
	vkDestroyShaderModule(g_device, _denoise_pass_C_vertex_shader, nullptr);
	vkDestroyShaderModule(g_device, _denoise_pass_C_fragment_shader, nullptr);
}

void VulkanBackEnd::Cleanup()
{
    GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();


	//make sure the gpu has stopped doing its things
	vkDeviceWaitIdle(g_device);

	cleanup_shaders();

	// Pipelines
	_pipelines.textBlitter.Cleanup(g_device);
	_pipelines.composite.Cleanup(g_device);
	_pipelines.lines.Cleanup(g_device);
	_pipelines.denoisePassA.Cleanup(g_device);
	_pipelines.denoisePassB.Cleanup(g_device);
	_pipelines.denoisePassC.Cleanup(g_device);
	_pipelines.denoiseBlurHorizontal.Cleanup(g_device);
	_pipelines.denoiseBlurVertical.Cleanup(g_device);

	// Command pools
	vkDestroyCommandPool(g_device, _uploadContext._commandPool, nullptr);
	vkDestroyFence(g_device, _uploadContext._uploadFence, nullptr);

	// Frame data
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		vkDestroyCommandPool(g_device, _frames[i]._commandPool, nullptr);
		vkDestroyFence(g_device, _frames[i]._renderFence, nullptr);
		vkDestroySemaphore(g_device, _frames[i]._presentSemaphore, nullptr);
		vkDestroySemaphore(g_device, _frames[i]._renderSemaphore, nullptr);
	}
	// Render targets
	_loadingScreenRenderTarget.cleanup(g_device, GetAllocator());
	_renderTargets.present.cleanup(g_device, GetAllocator());
	_renderTargets.gBufferBasecolor.cleanup(g_device, GetAllocator());
	_renderTargets.gBufferNormal.cleanup(g_device, GetAllocator());
	_renderTargets.gBufferRMA.cleanup(g_device, GetAllocator());
	_renderTargets.denoiseTextureA.cleanup(g_device, GetAllocator());
	_renderTargets.denoiseTextureB.cleanup(g_device, GetAllocator());
	_renderTargets.denoiseTextureC.cleanup(g_device, GetAllocator());
	_presentDepthTarget.Cleanup(g_device, GetAllocator());
	_gbufferDepthTarget.Cleanup(g_device, GetAllocator());
	_renderTargets.laptopDisplay.cleanup(g_device, GetAllocator());
	_renderTargets.composite.cleanup(g_device, GetAllocator());
	//_renderTargetDenoiseA.cleanup(_device, _allocator);
	//_renderTargetDenoiseB.cleanup(_device, _allocator);

	// Swapchain
	for (int i = 0; i < _swapchainImageViews.size(); i++) {
		vkDestroyImageView(g_device, _swapchainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(g_device, g_swapchain, nullptr);

	// Mesh buffers
	for (Mesh& mesh : AssetManager::GetMeshList()) {
		vmaDestroyBuffer(GetAllocator(), mesh._transformBuffer._buffer, mesh._transformBuffer._allocation);
		vmaDestroyBuffer(GetAllocator(), mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
		if (mesh._indexCount > 0) {
			vmaDestroyBuffer(GetAllocator(), mesh._indexBuffer._buffer, mesh._indexBuffer._allocation);
		}
		vmaDestroyBuffer(GetAllocator(), mesh._accelerationStructure.buffer._buffer, mesh._accelerationStructure.buffer._allocation);
		vkDestroyAccelerationStructureKHR(g_device, mesh._accelerationStructure.handle, nullptr);
	}
	vmaDestroyBuffer(GetAllocator(), _lineListMesh._vertexBuffer._buffer, _lineListMesh._vertexBuffer._allocation);

	// Buffers
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		_frames[i]._sceneCamDataBuffer.Destroy(GetAllocator());
		_frames[i]._inventoryCamDataBuffer.Destroy(GetAllocator());
		_frames[i]._meshInstances2DBuffer.Destroy(GetAllocator());
		_frames[i]._meshInstancesSceneBuffer.Destroy(GetAllocator());
		_frames[i]._meshInstancesInventoryBuffer.Destroy(GetAllocator());
		_frames[i]._lightRenderInfoBuffer.Destroy(GetAllocator()); 
		_frames[i]._lightRenderInfoBufferInventory.Destroy(GetAllocator());
	}
	// Descriptor sets
	_dynamicDescriptorSet.Destroy(g_device);
	_dynamicDescriptorSetInventory.Destroy(g_device);
	_staticDescriptorSet.Destroy(g_device);
	_samplerDescriptorSet.Destroy(g_device);
	//_denoiseATextureDescriptorSet.Destroy(_device);
	//_denoiseBTextureDescriptorSet.Destroy(_device);

	// Textures
	for (int i = 0; i < AssetManager::GetNumberOfTextures(); i++) {
		vmaDestroyImage(GetAllocator(), AssetManager::GetTexture(i)->image._image, AssetManager::GetTexture(i)->image._allocation);
		vkDestroyImageView(g_device, AssetManager::GetTexture(i)->imageView, nullptr);
	}
	// Raytracing
	cleanup_raytracing();

	// Vulkan shit
	//vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);

	VkDebugUtilsMessengerEXT debugMessenger = VulkanInstanceManager::GetDebugMessenger();
	VkInstance instance = VulkanInstanceManager::GetInstance();
	VkSurfaceKHR surface = VulkanInstanceManager::GetSurface();

	vkDestroyDescriptorPool(g_device, _descriptorPool, nullptr);
	vkDestroySampler(g_device, _sampler, nullptr);
	vmaDestroyAllocator(GetAllocator());
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkb::destroy_debug_utils_messenger(instance, debugMessenger);
	vkDestroyDevice(g_device, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(_window);
	glfwTerminate();
}

uint32_t alignedSize(uint32_t value, uint32_t alignment) {
	return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanBackEnd::init_raytracing() {
	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& _rayTracingPipelineProperties = VulkanDeviceManager::GetRayTracingPipelineProperties();

	std::vector<VkDescriptorSetLayout> rtDescriptorSetLayouts = { _dynamicDescriptorSet.layout, _staticDescriptorSet.layout, _samplerDescriptorSet.layout };

	_raytracer.CreatePipeline(g_device, rtDescriptorSetLayouts, 2);
	_raytracer.CreateShaderBindingTable(g_device, GetAllocator(), _rayTracingPipelineProperties);

	_raytracerPath.CreatePipeline(g_device, rtDescriptorSetLayouts, 2);
	_raytracerPath.CreateShaderBindingTable(g_device, GetAllocator(), _rayTracingPipelineProperties);

	_raytracerMousePick.CreatePipeline(g_device, rtDescriptorSetLayouts, 2);
	_raytracerMousePick.CreateShaderBindingTable(g_device, GetAllocator(), _rayTracingPipelineProperties);	
}


void VulkanBackEnd::cleanup_raytracing()
{
	// RT cleanup
	vmaDestroyBuffer(GetAllocator(), _rtVertexBuffer._buffer, _rtVertexBuffer._allocation);
	vmaDestroyBuffer(GetAllocator(), _rtIndexBuffer._buffer, _rtIndexBuffer._allocation);
	vmaDestroyBuffer(GetAllocator(), _mousePickResultBuffer._buffer, _mousePickResultBuffer._allocation);

	vmaDestroyBuffer(GetAllocator(), _mousePickResultCPUBuffer._buffer, _mousePickResultCPUBuffer._allocation);

	vmaDestroyBuffer(GetAllocator(), _frames[0]._sceneTLAS.buffer._buffer, _frames[0]._sceneTLAS.buffer._allocation);
	vmaDestroyBuffer(GetAllocator(), _frames[1]._sceneTLAS.buffer._buffer, _frames[1]._sceneTLAS.buffer._allocation);
	vmaDestroyBuffer(GetAllocator(), _frames[0]._inventoryTLAS.buffer._buffer, _frames[0]._inventoryTLAS.buffer._allocation);
	vmaDestroyBuffer(GetAllocator(), _frames[1]._inventoryTLAS.buffer._buffer, _frames[1]._inventoryTLAS.buffer._allocation);

	vkDestroyAccelerationStructureKHR(g_device, _frames[0]._sceneTLAS.handle, nullptr);
	vkDestroyAccelerationStructureKHR(g_device, _frames[1]._sceneTLAS.handle, nullptr);
	vkDestroyAccelerationStructureKHR(g_device, _frames[0]._inventoryTLAS.handle, nullptr);
	vkDestroyAccelerationStructureKHR(g_device, _frames[1]._inventoryTLAS.handle, nullptr);

	_renderTargets.rt_first_hit_color.cleanup(g_device, GetAllocator());
	_renderTargets.rt_first_hit_base_color.cleanup(g_device, GetAllocator());
	_renderTargets.rt_first_hit_normals.cleanup(g_device, GetAllocator());
	_renderTargets.rt_second_hit_color.cleanup(g_device, GetAllocator());
	//_renderTargets.rt_inventory.cleanup(_device, _allocator);

	_raytracer.Cleanup(g_device, GetAllocator());
	_raytracerPath.Cleanup(g_device, GetAllocator());
	_raytracerMousePick.Cleanup(g_device, GetAllocator());
}

void VulkanBackEnd::create_command_buffers()
{
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(g_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(g_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		//allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &_frames[i]._commandBuffer));
	}

	//create command pool for upload context
	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(g_graphicsQueueFamily);

	//create command buffer for upload context
	VK_CHECK(vkCreateCommandPool(g_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &_uploadContext._commandBuffer));

    std::cout << "Created command buffers\n";
}



void VulkanBackEnd::RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer) {

	std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
	VkRenderingAttachmentInfoKHR colorAttachment = {};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = { 0.0, 0.0, 0, 0 };
	colorAttachment.imageView = _loadingScreenRenderTarget._view;
	colorAttachments.push_back(colorAttachment);

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.renderArea = { 0, 0, _loadingScreenRenderTarget._extent.width, _loadingScreenRenderTarget._extent.height };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = colorAttachments.size();
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);
	cmd_SetViewportSize(commandBuffer, _loadingScreenRenderTarget);
	cmd_BindPipeline(commandBuffer, _pipelines.textBlitter);
	cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, _dynamicDescriptorSet);
	cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, _staticDescriptorSet);

	// Draw Text plus maybe crosshair
	for (int i = 0; i < RasterRenderer::instanceCount; i++) {
		if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::MAIN_UI)
			RasterRenderer::DrawMesh(commandBuffer, i);
	}
	RasterRenderer::ClearQueue();

	vkCmdEndRendering(commandBuffer);
}


void VulkanBackEnd::BlitRenderTargetIntoSwapChain(VkCommandBuffer commandBuffer, RenderTarget& renderTarget, uint32_t swapchainImageIndex) {
	renderTarget.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;
	VkImageMemoryBarrier swapChainBarrier = {};
	swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapChainBarrier.image = _swapchainImages[swapchainImageIndex];
	swapChainBarrier.subresourceRange = range;
	swapChainBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	swapChainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
	VkImageBlit region;
	region.srcOffsets[0].x = 0;
	region.srcOffsets[0].y = 0;
	region.srcOffsets[0].z = 0;
	region.srcOffsets[1].x = renderTarget._extent.width;
	region.srcOffsets[1].y = renderTarget._extent.height;
	region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
	region.dstOffsets[0].x = 0;
	region.dstOffsets[0].y = 0;
	region.dstOffsets[0].z = 0;
	region.dstOffsets[1].x = _currentWindowExtent.width;
	region.dstOffsets[1].y = _currentWindowExtent.height;
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
	region.srcOffsets[1].x = renderTarget._extent.width;
	region.srcOffsets[1].y = renderTarget._extent.height;
	region.dstOffsets[1].x = _currentWindowExtent.width;
	region.dstOffsets[1].y = _currentWindowExtent.height;
	vkCmdBlitImage(commandBuffer, renderTarget._image, srcLayout, _swapchainImages[swapchainImageIndex], dstLayout, regionCount, &region, VkFilter::VK_FILTER_NEAREST);

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
	swapChainBarrier.image = _swapchainImages[swapchainImageIndex];
	swapChainBarrier.subresourceRange = range;
	swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	swapChainBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
}


void VulkanBackEnd::RenderLoadingFrame()
{
	if (ProgramIsMinimized())
		return;

	AddDebugText();
	TextBlitter::Update(GameData::GetDeltaTime(), _loadingScreenRenderTarget._extent.width, _loadingScreenRenderTarget._extent.height);

	int32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	VkFence renderFence = get_current_frame()._renderFence;
	VkSemaphore presentSemaphore = get_current_frame()._presentSemaphore;
	VkCommandBuffer commandBuffer = _frames[_frameNumber % FRAME_OVERLAP]._commandBuffer;
	VkCommandBufferBeginInfo commandBufferInfo = vkinit::command_buffer_begin_info();

	vkWaitForFences(g_device, 1, &renderFence, VK_TRUE, UINT64_MAX);

	UpdateBuffers2D();
	_dynamicDescriptorSet.Update(g_device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._meshInstances2DBuffer.buffer);

	vkResetFences(g_device, 1, &renderFence);

	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));

	_loadingScreenRenderTarget.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	RecordAssetLoadingRenderCommands(commandBuffer);

	BlitRenderTargetIntoSwapChain(commandBuffer, _loadingScreenRenderTarget, swapchainImageIndex);
	PrepareSwapchainForPresent(commandBuffer, swapchainImageIndex);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submit = vkinit::submit_info(&commandBuffer);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;
	VK_CHECK(vkQueueSubmit(g_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

	VkPresentInfoKHR presentInfo2 = vkinit::present_info();
	presentInfo2.pSwapchains = &g_swapchain;
	presentInfo2.swapchainCount = 1;
	presentInfo2.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo2.waitSemaphoreCount = 1;
	presentInfo2.pImageIndices = &swapchainImageIndex;
	result = vkQueuePresentKHR(g_graphicsQueue, &presentInfo2);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreate_dynamic_swapchain();
	}
}


void VulkanBackEnd::RenderGameFrame()
{
	if (ProgramIsMinimized())
		return;

	TextBlitter::Update(GameData::GetDeltaTime(), _renderTargets.present._extent.width, _renderTargets.present._extent.height);

	int32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	VkFence renderFence = get_current_frame()._renderFence;
	VkSemaphore presentSemaphore = get_current_frame()._presentSemaphore;
	VkCommandBuffer commandBuffer = _frames[_frameNumber % FRAME_OVERLAP]._commandBuffer;
	VkCommandBufferBeginInfo commandBufferInfo = vkinit::command_buffer_begin_info();

	vkWaitForFences(g_device, 1, &renderFence, VK_TRUE, UINT64_MAX);

	// Prep
	{
		// Recreate TLAS for current frame
		vmaDestroyBuffer(GetAllocator(), get_current_frame()._sceneTLAS.buffer._buffer, get_current_frame()._sceneTLAS.buffer._allocation);
		vkDestroyAccelerationStructureKHR(g_device, get_current_frame()._sceneTLAS.handle, nullptr);
		create_top_level_acceleration_structure(Scene::GetMeshInstancesForSceneAccelerationStructure(), get_current_frame()._sceneTLAS);
		vmaDestroyBuffer(GetAllocator(), get_current_frame()._inventoryTLAS.buffer._buffer, get_current_frame()._inventoryTLAS.buffer._allocation);
		vkDestroyAccelerationStructureKHR(g_device, get_current_frame()._inventoryTLAS.handle, nullptr);
		create_top_level_acceleration_structure(Scene::GetMeshInstancesForInventoryAccelerationStructure(), get_current_frame()._inventoryTLAS);

		get_required_lines();
		UpdateBuffers();
		UpdateBuffers2D();
		UpdateDynamicDescriptorSet();
	}

	vkResetFences(g_device, 1, &renderFence);

	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
	//VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));

	//_renderTargets.present.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	//RecordCommandsAssetLoading(commandBuffer);
	build_rt_command_buffers(swapchainImageIndex);

	//BlitRenderTargetIntoSwapChain(commandBuffer, _renderTargets.present, swapchainImageIndex);
	//PrepareSwapchainForPresent(commandBuffer, swapchainImageIndex);

	//VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submit = vkinit::submit_info(&commandBuffer);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;
	VK_CHECK(vkQueueSubmit(g_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

	VkPresentInfoKHR presentInfo2 = vkinit::present_info();
	presentInfo2.pSwapchains = &g_swapchain;
	presentInfo2.swapchainCount = 1;
	presentInfo2.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo2.waitSemaphoreCount = 1;
	presentInfo2.pImageIndices = &swapchainImageIndex;
	result = vkQueuePresentKHR(g_graphicsQueue, &presentInfo2);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreate_dynamic_swapchain();
	}

	_frameNumber++;
}


void VulkanBackEnd::ToggleFullscreen() {
    GLFWIntegration::ToggleFullscreen();

    recreate_dynamic_swapchain();
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






void VulkanBackEnd::create_render_targets()
{
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	//  Present Target
	{
		VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		_renderTargets.present = RenderTarget(g_device, GetAllocator(), format, _renderTargetPresentExtent.width, _renderTargetPresentExtent.height, usageFlags, "present render target");
	}
	int scale = 2;
	uint32_t width = _renderTargets.present._extent.width * scale;
	uint32_t height = _renderTargets.present._extent.height * scale;

	// RT image store 
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;// VK_FORMAT_R8G8B8A8_UNORM;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		_renderTargets.rt_first_hit_color = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "rt first hit color render target");
		_renderTargets.rt_first_hit_normals = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "rt first hit normals render target");
		_renderTargets.rt_first_hit_base_color = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "rt first hit base color render target");
		_renderTargets.rt_second_hit_color = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "rt second hit color render target"); 
	}

	// GBuffer
	{
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		_renderTargets.gBufferBasecolor = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "gbuffer base color render target");
		_renderTargets.gBufferNormal = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "gbuffer base normal render target");
		_renderTargets.gBufferRMA = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "gbuffer base rma render target");
	}	
	// Laptop screen
	{
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		_renderTargets.laptopDisplay = RenderTarget(g_device, GetAllocator(), format, LAPTOP_DISPLAY_WIDTH, LAPTOP_DISPLAY_HEIGHT, usage, "laptop display render target");
	}
	

	

	//depth image size will match the window
	VkExtent3D depthImageExtent = {
		_renderTargets.present._extent.width,
		_renderTargets.present._extent.height,
		1
	};

	//hardcoding the depth format to 32 bit float
	_depthFormat = VK_FORMAT_D32_SFLOAT;

	_presentDepthTarget.Create(g_device, GetAllocator(), VK_FORMAT_D32_SFLOAT, _renderTargets.present._extent);
	_gbufferDepthTarget.Create(g_device, GetAllocator(), VK_FORMAT_D32_SFLOAT, _renderTargets.gBufferNormal._extent);
	
	// Blur target
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	_renderTargets.composite = RenderTarget(g_device, GetAllocator(), format, width, height, usage, "composite render targe");

	float denoiseWidth = _renderTargets.rt_first_hit_color._extent.width;
	float denoiseHeight = _renderTargets.rt_first_hit_color._extent.height;
	_renderTargets.denoiseTextureA = RenderTarget(g_device, GetAllocator(), format, denoiseWidth, denoiseHeight, usage, "denoise A render target");
	_renderTargets.denoiseTextureB = RenderTarget(g_device, GetAllocator(), format, denoiseWidth, denoiseHeight, usage, "denoise B render target");
	_renderTargets.denoiseTextureC = RenderTarget(g_device, GetAllocator(), format, denoiseWidth, denoiseHeight, usage, "denoise C render target");
}

void VulkanBackEnd::recreate_dynamic_swapchain()
{
    GLFWwindow* _window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();



	std::cout << "Recreating swapchain...\n";

	while (_currentWindowExtent.width == 0 || _currentWindowExtent.height == 0) {
		int width, height;
		glfwGetFramebufferSize(_window, &width, &height);
		_currentWindowExtent.width = width;
		_currentWindowExtent.height = height;
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(g_device);

	for (int i = 0; i < _swapchainImages.size(); i++) {
		vkDestroyImageView(g_device, _swapchainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(g_device, g_swapchain, nullptr);

	CreateSwapchain();
}


void VulkanBackEnd::create_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(g_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
		VK_CHECK(vkCreateSemaphore(g_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
		VK_CHECK(vkCreateSemaphore(g_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
	}
	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
	VK_CHECK(vkCreateFence(g_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));

    std::cout << "Created sync structures\n";
}

void VulkanBackEnd::hotload_shaders()
{
	std::cout << "Hotloading shaders...\n";

	vkDeviceWaitIdle(g_device);

	_raytracer.Cleanup(g_device, GetAllocator());
	_raytracerPath.Cleanup(g_device, GetAllocator());
	_raytracerMousePick.Cleanup(g_device, GetAllocator());

	//load_shader(_device, "composite.vert", VK_SHADER_STAGE_VERTEX_BIT, &_composite_vertex_shader);
	//load_shader(_device, "composite.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_composite_fragment_shader);

	//_raytracer.LoadShaders(_device, "raygen.rgen", "miss.rmiss", "shadow.rmiss", "closesthit.rchit");
	//_raytracerPath.LoadShaders(_device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit");
	//_raytracerMousePick.LoadShaders(_device, "mousepick_raygen.rgen", "mousepick_miss.rmiss", "path_shadow.rmiss", "mousepick_closesthit.rchit");

	LoadShaders();

	create_pipelines();

	init_raytracing();
}
/*
void Vulkan::hotload_shaders()
{
	std::cout << "Hotloading shaders...\n";

	vkDeviceWaitIdle(_device);

	_compositePipeline.Cleanup(_device);
	_denoisePipeline.Cleanup(_device);
	_rasterPipeline.Cleanup(_device);
	_textBlitterPipeline.Cleanup(_device);
	_computePipeline.Cleanup(_device);

	vkDestroyPipeline(_device, _linelistPipeline, nullptr);
	vkDestroyPipelineLayout(_device, _linelistPipelineLayout, nullptr);

	_raytracer.Cleanup(_device, _allocator);
	_raytracerPath.Cleanup(_device, _allocator);
	_raytracerMousePick.Cleanup(_device, _allocator);

	LoadShaders();
	create_pipelines();

	init_raytracing();
}*/


void VulkanBackEnd::LoadShaders()
{
	load_shader(g_device, "solid_color.vert", VK_SHADER_STAGE_VERTEX_BIT, &_solid_color_vertex_shader);
	load_shader(g_device, "solid_color.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_solid_color_fragment_shader);

	load_shader(g_device, "gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT, &_gbuffer_vertex_shader);
	load_shader(g_device, "gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_gbuffer_fragment_shader);

	load_shader(g_device, "denoise_pass_A.vert", VK_SHADER_STAGE_VERTEX_BIT, &_denoise_pass_A_vertex_shader);
	load_shader(g_device, "denoise_pass_A.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_denoise_pass_A_fragment_shader);

	load_shader(g_device, "denoise_pass_B.vert", VK_SHADER_STAGE_VERTEX_BIT, &_denoise_pass_B_vertex_shader);
	load_shader(g_device, "denoise_pass_B.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_denoise_pass_B_fragment_shader);

	load_shader(g_device, "denoise_pass_C.vert", VK_SHADER_STAGE_VERTEX_BIT, &_denoise_pass_C_vertex_shader);
	load_shader(g_device, "denoise_pass_C.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_denoise_pass_C_fragment_shader);

	load_shader(g_device, "blur_horizontal.vert", VK_SHADER_STAGE_VERTEX_BIT, &_blur_horizontal_vertex_shader);
	load_shader(g_device, "blur_horizontal.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_blur_horizontal_fragment_shader);

	load_shader(g_device, "blur_vertical.vert", VK_SHADER_STAGE_VERTEX_BIT, &_blur_vertical_vertex_shader);
	load_shader(g_device, "blur_vertical.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_blur_vertical_fragment_shader);

	load_shader(g_device, "composite.vert", VK_SHADER_STAGE_VERTEX_BIT, &_composite_vertex_shader);
	load_shader(g_device, "composite.frag", VK_SHADER_STAGE_FRAGMENT_BIT, &_composite_fragment_shader);
	
	_raytracer.LoadShaders(g_device, "raygen.rgen", "miss.rmiss", "shadow.rmiss", "closesthit.rchit");
	_raytracerPath.LoadShaders(g_device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit");
	_raytracerMousePick.LoadShaders(g_device, "mousepick_raygen.rgen", "mousepick_miss.rmiss", "path_shadow.rmiss", "mousepick_closesthit.rchit");
}


void VulkanBackEnd::create_pipelines()
{
	// Commposite pipeline
	_pipelines.composite.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
	_pipelines.composite.PushDescriptorSetLayout(_staticDescriptorSet.layout);
	_pipelines.composite.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
	_pipelines.composite.CreatePipelineLayout(g_device);
	_pipelines.composite.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
	_pipelines.composite.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	_pipelines.composite.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.composite.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.composite.SetColorBlending(false);
	_pipelines.composite.SetDepthTest(false);
	_pipelines.composite.SetDepthWrite(false);
	_pipelines.composite.Build(g_device, _composite_vertex_shader, _composite_fragment_shader, 1);
	
	// Lines pipeline
	_pipelines.lines.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
	_pipelines.lines.PushDescriptorSetLayout(_staticDescriptorSet.layout);
	_pipelines.lines.SetPushConstantSize(sizeof(LineShaderPushConstants));
	_pipelines.lines.SetPushConstantCount(1);
	_pipelines.lines.CreatePipelineLayout(g_device);
	_pipelines.lines.SetVertexDescription(VertexDescriptionType::POSITION);
	_pipelines.lines.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	_pipelines.lines.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.lines.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.lines.SetColorBlending(false);
	_pipelines.lines.SetDepthTest(false);
	_pipelines.lines.SetDepthWrite(false);
	_pipelines.lines.Build(g_device, _solid_color_vertex_shader, _solid_color_fragment_shader, 1);

	// Denoise A pipeline
	_pipelines.denoisePassA.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
	_pipelines.denoisePassA.CreatePipelineLayout(g_device);
	_pipelines.denoisePassA.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
	_pipelines.denoisePassA.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	_pipelines.denoisePassA.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.denoisePassA.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.denoisePassA.SetColorBlending(false);
	_pipelines.denoisePassA.SetDepthTest(false);
	_pipelines.denoisePassA.SetDepthWrite(false);
	_pipelines.denoisePassA.Build(g_device, _denoise_pass_A_vertex_shader, _denoise_pass_A_fragment_shader, 1);
	
	// Denoise B pipeline
	_pipelines.denoisePassB.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
	_pipelines.denoisePassB.CreatePipelineLayout(g_device);
	_pipelines.denoisePassB.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
	_pipelines.denoisePassB.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	_pipelines.denoisePassB.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.denoisePassB.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.denoisePassB.SetColorBlending(false);
	_pipelines.denoisePassB.SetDepthTest(false);
	_pipelines.denoisePassB.SetDepthWrite(false);
	_pipelines.denoisePassB.Build(g_device, _denoise_pass_B_vertex_shader, _denoise_pass_B_fragment_shader, 1);

	// Denoise C pipeline
	_pipelines.denoisePassC.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
	_pipelines.denoisePassC.CreatePipelineLayout(g_device);
	_pipelines.denoisePassC.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
	_pipelines.denoisePassC.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	_pipelines.denoisePassC.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.denoisePassC.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.denoisePassC.SetColorBlending(false);
	_pipelines.denoisePassC.SetDepthTest(false);
	_pipelines.denoisePassC.SetDepthWrite(false);
	_pipelines.denoisePassC.Build(g_device, _denoise_pass_C_vertex_shader, _denoise_pass_C_fragment_shader, 1);
	
	// Blur horizontal
	_pipelines.denoiseBlurHorizontal.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
	_pipelines.denoiseBlurHorizontal.CreatePipelineLayout(g_device);
	_pipelines.denoiseBlurHorizontal.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
	_pipelines.denoiseBlurHorizontal.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	_pipelines.denoiseBlurHorizontal.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.denoiseBlurHorizontal.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.denoiseBlurHorizontal.SetColorBlending(false);
	_pipelines.denoiseBlurHorizontal.SetDepthTest(false);
	_pipelines.denoiseBlurHorizontal.SetDepthWrite(false);
	_pipelines.denoiseBlurHorizontal.Build(g_device, _blur_horizontal_vertex_shader, _blur_horizontal_fragment_shader, 1);

	// Blur vertical
	_pipelines.denoiseBlurVertical.PushDescriptorSetLayout(_samplerDescriptorSet.layout);
	_pipelines.denoiseBlurVertical.CreatePipelineLayout(g_device);
	_pipelines.denoiseBlurVertical.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
	_pipelines.denoiseBlurVertical.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	_pipelines.denoiseBlurVertical.SetPolygonMode(VK_POLYGON_MODE_FILL);
	_pipelines.denoiseBlurVertical.SetCullModeFlags(VK_CULL_MODE_NONE);
	_pipelines.denoiseBlurVertical.SetColorBlending(false);
	_pipelines.denoiseBlurVertical.SetDepthTest(false);
	_pipelines.denoiseBlurVertical.SetDepthWrite(false);
	_pipelines.denoiseBlurVertical.Build(g_device, _blur_vertical_vertex_shader, _blur_vertical_fragment_shader, 1);
	
	// Raster pipeline
	/* {
		// Text blitter pipeline layout
		_rasterPipeline.PushDescriptorSetLayout(_dynamicDescriptorSet.layout);
		_rasterPipeline.PushDescriptorSetLayout(_staticDescriptorSet.layout);
		_rasterPipeline.CreatePipelineLayout(_device);

		VertexInputDescription vertexDescription = Util::get_vertex_description();
		PipelineBuilder pipelineBuilder;
		pipelineBuilder._pipelineLayout = _rasterPipeline.layout;
		pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
		pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE); // VK_CULL_MODE_NONE
		pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
		pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(false);
		pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
		pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
		pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
		pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
		pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
		pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, _gbuffer_vertex_shader));
		pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, _gbuffer_fragment_shader));

		_rasterPipeline.handle = pipelineBuilder.build_dynamic_rendering_pipeline(_device, _depthFormat, 3);
	}*/
}

void VulkanBackEnd::upload_meshes() {
	for (Mesh& mesh : AssetManager::GetMeshList()) {
		if (!mesh._uploadedToGPU) {
			upload_mesh(mesh);
			mesh._accelerationStructure = createBottomLevelAccelerationStructure(&mesh);
		}
	}
    std::cout << "uploaded meshes\n";
}

void VulkanBackEnd::upload_mesh(Mesh& mesh)
{
	//std::cout << mesh._filename << " has " << mesh._vertices.size() << " vertices and " << mesh._indices.size() << " indices " << "\n";
	// Vertices
	{
		const size_t bufferSize = mesh._vertexCount * sizeof(Vertex);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);
		memcpy(data, AssetManager::GetVertexPointer(mesh._vertexOffset), mesh._vertexCount * sizeof(Vertex));
		vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);

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

		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &mesh._vertexBuffer._buffer, &mesh._vertexBuffer._allocation, nullptr));

		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy);
			});

		vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);
	}

	// Indices
	if (mesh._indexCount > 0)
	{
		const size_t bufferSize = mesh._indexCount * sizeof(uint32_t);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);

		memcpy(data, AssetManager::GetIndexPointer(mesh._indexOffset), mesh._indexCount * sizeof(uint32_t));
		vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);

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

		VK_CHECK(vmaCreateBuffer(GetAllocator(), &indexBufferInfo, &vmaallocInfo, &mesh._indexBuffer._buffer, &mesh._indexBuffer._allocation, nullptr));

		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._indexBuffer._buffer, 1, &copy);
			});

		vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);
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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);
		memcpy(data, &transformMatrix, bufferSize);
		vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;// VMA_MEMORY_USAGE_AUTO;// VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &mesh._transformBuffer._buffer, &mesh._transformBuffer._allocation, nullptr));
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._transformBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);
	}
	mesh._uploadedToGPU = true;
}

AllocatedBuffer VulkanBackEnd::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags)
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

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo,
		&newBuffer._buffer,
		&newBuffer._allocation,
		nullptr));

	return newBuffer;
}

void VulkanBackEnd::add_debug_name(VkBuffer buffer, const char* name) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
	nameInfo.objectHandle = (uint64_t)buffer;
	nameInfo.pObjectName = name;
	vkSetDebugUtilsObjectNameEXT(g_device, &nameInfo);
}

void VulkanBackEnd::add_debug_name(VkDescriptorSetLayout descriptorSetLayout, const char* name) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	nameInfo.objectHandle = (uint64_t)descriptorSetLayout;
	nameInfo.pObjectName = name;
	vkSetDebugUtilsObjectNameEXT(g_device, &nameInfo);
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



void VulkanBackEnd::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = _uploadContext._commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	VK_CHECK(vkQueueSubmit(g_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

	vkWaitForFences(g_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(g_device, 1, &_uploadContext._uploadFence);

	vkResetCommandPool(g_device, _uploadContext._commandPool, 0);
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
	vkCreateSampler(g_device, &samplerInfo, nullptr, &_sampler);

    std::cout << "created sampler\n";
}



void VulkanBackEnd::create_descriptors() {
	std::vector<VkDescriptorPoolSize> sizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,              10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,      10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,              10 },

        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,               8 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,      64 },

        { VK_DESCRIPTOR_TYPE_SAMPLER,                     64 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,               256 },

        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,  4 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 10;
	descriptorPoolCreateInfo.poolSizeCount = (uint32_t)sizes.size();
	descriptorPoolCreateInfo.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(g_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool);

	// Dynamic 
	_dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// acceleration structure
	_dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// camera data
	_dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// all 3D mesh instances
	_dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_VERTEX_BIT);																			// all 2d mesh instances
	_dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);																	// light positions and colors
	_dynamicDescriptorSet.BuildSetLayout(g_device);
	_dynamicDescriptorSet.AllocateSet(g_device, _descriptorPool);
	add_debug_name(_dynamicDescriptorSet.layout, "DynamicDescriptorSetLayout");
	// Dynamic inventory
	_dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// acceleration structure
	_dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// camera data
	_dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// all 3D mesh instances
	_dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_VERTEX_BIT);																			// all 2d mesh instances
	_dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);																	// light positions and colors
	_dynamicDescriptorSetInventory.BuildSetLayout(g_device);
	_dynamicDescriptorSetInventory.AllocateSet(g_device, _descriptorPool);
	add_debug_name(_dynamicDescriptorSetInventory.layout, "_dynamicDescriptorSetInventory");

	// Static 
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 0, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);							// sampler
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, TEXTURE_ARRAY_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);	// all textures
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// all vertices
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// all indices
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: first hit color
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// all indices
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: first hit normals
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 7, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: first hit base color
	_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: second hit color
	_staticDescriptorSet.BuildSetLayout(g_device);
	_staticDescriptorSet.AllocateSet(g_device, _descriptorPool);
	add_debug_name(_staticDescriptorSet.layout, "StaticDescriptorSetLayout");

	// Samplers
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // first hit color
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // first hit normals
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // first hit base color
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // second hit color
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // denoise texture A
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // denoise texture B
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // denoise texture C
	_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // laptop
	_samplerDescriptorSet.BuildSetLayout(g_device);
	_samplerDescriptorSet.AllocateSet(g_device, _descriptorPool);
	add_debug_name(_samplerDescriptorSet.layout, "_samplerDescriptorSet");												// depth aware blur texture

	// Denoiser sampler A
	/*_denoiseATextureDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	_denoiseATextureDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	_denoiseATextureDescriptorSet.BuildSetLayout(_device);
	_denoiseATextureDescriptorSet.AllocateSet(_device, _descriptorPool);
	add_debug_name(_denoiseATextureDescriptorSet.layout, "_denoiseATextureDescriptorSet");
	// Denoiser sampler B
	_denoiseBTextureDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
	_denoiseBTextureDescriptorSet.BuildSetLayout(_device);
	_denoiseBTextureDescriptorSet.AllocateSet(_device, _descriptorPool);
	add_debug_name(_denoiseBTextureDescriptorSet.layout, "_denoiseBTextureDescriptorSet");												// depth aware blur texture*/



    std::cout << "created descriptors\n";
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


void VulkanBackEnd::createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	// Free the memory if it already exists (which is the case if hotloading shaders causes BLAS for each mesh to be generated)
	/*if (accelerationStructure.buffer._buffer != VK_NULL_HANDLE) {
		vmaDestroyBuffer(_allocator, accelerationStructure.buffer._buffer, accelerationStructure.buffer._allocation);
		vkDestroyAccelerationStructureKHR(_device, accelerationStructure.handle, nullptr);
	}*/

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferCreateInfo, &vmaallocInfo, &accelerationStructure.buffer._buffer, &accelerationStructure.buffer._allocation, nullptr));
	add_debug_name(accelerationStructure.buffer._buffer, "Acceleration Structure Buffer");
}

uint64_t VulkanBackEnd::get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(g_device, &bufferDeviceAI);
}

RayTracingScratchBuffer VulkanBackEnd::createScratchBuffer(VkDeviceSize size)
{
	RayTracingScratchBuffer scratchBuffer{};

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferCreateInfo, &vmaallocInfo, &scratchBuffer.handle._buffer, &scratchBuffer.handle._allocation, nullptr));

	VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
	bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAddressInfo.buffer = scratchBuffer.handle._buffer;
	scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(g_device, &bufferDeviceAddressInfo);

	return scratchBuffer;
}

void VulkanBackEnd::create_rt_buffers()
{
	//std::vector<Mesh*> meshes = Scene::GetMeshList();
	//std::vector<Mesh>& meshes = AssetManager::GetMeshList();
	std::vector<Vertex>& vertices = AssetManager::GetVertices_TEMPORARY();
	std::vector<uint32_t>& indices = AssetManager::GetIndices_TEMPORARY();

	{
		// RT Mesh Instance Index Buffer
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = sizeof(uint32_t) * 2;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &_mousePickResultBuffer._buffer, &_mousePickResultBuffer._allocation, nullptr));
		add_debug_name(_mousePickResultBuffer._buffer, "_mousePickResultBuffer");
		//vmaMapMemory(_allocator, _mousePickResultBuffer._allocation, &_mousePickResultBuffer._mapped);

		// Copy in inital values of 0
		uint32_t mousePickResult[2] = { 0, 0 };

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.size = sizeof(uint32_t) * 2;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &_mousePickResultCPUBuffer._buffer, &_mousePickResultCPUBuffer._allocation, nullptr));
		add_debug_name(_mousePickResultCPUBuffer._buffer, "_mousePickResultCPUBuffer");
		vmaMapMemory(GetAllocator(), _mousePickResultCPUBuffer._allocation, &_mousePickResultCPUBuffer._mapped);
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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		add_debug_name(stagingBuffer._buffer, "stagingBuffer");
		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);
		memcpy(data, vertices.data(), bufferSize);
		vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);
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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &_rtVertexBuffer._buffer, &_rtVertexBuffer._allocation, nullptr));
		add_debug_name(_rtVertexBuffer._buffer, "_rtVertexBuffer");
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtVertexBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);

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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		add_debug_name(stagingBuffer._buffer, "stagingBufferIndices");
		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);
		memcpy(data, indices.data(), bufferSize);
		vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);
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
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &_rtIndexBuffer._buffer, &_rtIndexBuffer._allocation, nullptr));
		add_debug_name(_rtIndexBuffer._buffer, "_rtIndexBufferVertices");
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtIndexBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);
	}

	_vertexBufferDeviceAddress.deviceAddress = get_buffer_device_address(_rtVertexBuffer._buffer);
	_indexBufferDeviceAddress.deviceAddress = get_buffer_device_address(_rtIndexBuffer._buffer);
}
	 
AccelerationStructure VulkanBackEnd::createBottomLevelAccelerationStructure(Mesh* mesh)
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
	accelerationStructureGeometry.geometry.triangles.maxVertex = mesh->_vertexCount;
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

	const uint32_t numTriangles = mesh->_indexCount / 3;
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		g_device,
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
	vkCreateAccelerationStructureKHR(g_device, &accelerationStructureCreateInfo, nullptr, &bottomLevelAS.handle);

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
	bottomLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(g_device, &accelerationDeviceAddressInfo);

	vmaDestroyBuffer(GetAllocator(), scratchBuffer.handle._buffer, scratchBuffer.handle._allocation);

	return bottomLevelAS;
}


void VulkanBackEnd::create_top_level_acceleration_structure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS)
{
	if (instances.size() == 0)
		return;

	// create the BLAS instances within TLAS, one for each game object
	const size_t bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);	
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	AllocatedBuffer stagingBuffer;

	VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
	add_debug_name(stagingBuffer._buffer, "stagingBufferTLAS");
	void* data;
	vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);
	memcpy(data, instances.data(), bufferSize);
	vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateBuffer(GetAllocator(), &bufferInfo, &vmaallocInfo, &_rtInstancesBuffer._buffer, &_rtInstancesBuffer._allocation, nullptr));
	add_debug_name(_rtInstancesBuffer._buffer, "_rtInstancesBuffer");

	immediate_submit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _rtInstancesBuffer._buffer, 1, &copy);
		});
	vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);


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
		g_device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildInfo,
		&numInstances,
		&accelerationStructureBuildSizesInfo);

	createAccelerationStructureBuffer(outTLAS, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = outTLAS.buffer._buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(g_device, &accelerationStructureCreateInfo, nullptr, &outTLAS.handle);

	// Create a small scratch buffer used during build of the top level acceleration structure
	RayTracingScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = outTLAS.handle;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numInstances;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	vkDeviceWaitIdle(g_device);

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
	accelerationDeviceAddressInfo.accelerationStructure = outTLAS.handle;
	outTLAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(g_device, &accelerationDeviceAddressInfo);

	vmaDestroyBuffer(GetAllocator(), scratchBuffer.handle._buffer, scratchBuffer.handle._allocation);
	
	// deal with this
	vmaDestroyBuffer(GetAllocator(), _rtInstancesBuffer._buffer, _rtInstancesBuffer._allocation);
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
	int quadMeshIndex = AssetManager::GetModel("blitter_quad")->_meshIndices[0];
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
	
	// Sample
	VkDescriptorImageInfo samplerImageInfo = {};
	samplerImageInfo.sampler = _sampler;
	_staticDescriptorSet.Update(g_device, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);

	// All textures
	VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
	for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
		textureImageInfo[i].sampler = nullptr;
		textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureImageInfo[i].imageView = (i < AssetManager::GetNumberOfTextures()) ? AssetManager::GetTexture(i)->imageView : AssetManager::GetTexture(0)->imageView; // Fill with dummy if you excede the amount of textures we loaded off disk. Can't have no junk data.
	}
	_staticDescriptorSet.Update(g_device, 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);
	
	// All vertex and index data
	_staticDescriptorSet.Update(g_device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _rtVertexBuffer._buffer);
	_staticDescriptorSet.Update(g_device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _rtIndexBuffer._buffer);

	// Raytracing storage image and all vertex/index data
	VkDescriptorImageInfo storageImageDescriptor{};
	storageImageDescriptor.imageView = _renderTargets.rt_first_hit_color._view;
	storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	_staticDescriptorSet.Update(g_device, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

	// 1x1 mouse picking buffer
	_staticDescriptorSet.Update(g_device, 5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _mousePickResultBuffer._buffer);

	// RT normals and depth
	storageImageDescriptor.imageView = _renderTargets.rt_first_hit_normals._view;
	_staticDescriptorSet.Update(g_device, 6, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
	storageImageDescriptor.imageView = _renderTargets.rt_first_hit_base_color._view;
	_staticDescriptorSet.Update(g_device, 7, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
	storageImageDescriptor.imageView = _renderTargets.rt_second_hit_color._view;
	_staticDescriptorSet.Update(g_device, 8, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

	// This below is just so you can bind THE GBUFFER DEPTH TARGET in shaders. Needs layout general
	immediate_submit([=](VkCommandBuffer cmd) {
		_renderTargets.laptopDisplay.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.gBufferNormal.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.gBufferRMA.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_gbufferDepthTarget.InsertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		//_renderTargetDenoiseA.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		});


	// samplers texture
	VkDescriptorImageInfo storageImageDescriptor2{};
	storageImageDescriptor2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	storageImageDescriptor2.sampler = _sampler;

	storageImageDescriptor2.imageView = _renderTargets.rt_first_hit_color._view;
	_samplerDescriptorSet.Update(g_device, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.rt_first_hit_normals._view;
	_samplerDescriptorSet.Update(g_device, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.rt_first_hit_base_color._view;
	_samplerDescriptorSet.Update(g_device, 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.rt_second_hit_color._view;
	_samplerDescriptorSet.Update(g_device, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.denoiseTextureA._view;
	_samplerDescriptorSet.Update(g_device, 4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.denoiseTextureB._view;
	_samplerDescriptorSet.Update(g_device, 5, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.denoiseTextureC._view;
	_samplerDescriptorSet.Update(g_device, 6, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.laptopDisplay._view;
	_samplerDescriptorSet.Update(g_device, 7, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	/*storageImageDescriptor2.imageView = _renderTargets.denoiseA._view;
	_denoiseATextureDescriptorSet.Update(_device, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

	storageImageDescriptor2.imageView = _renderTargets.denoiseB._view;
	_denoiseBTextureDescriptorSet.Update(_device, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);	*/
}

void VulkanBackEnd::UpdateDynamicDescriptorSet() {

	_dynamicDescriptorSetInventory.Update(g_device, 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &get_current_frame()._inventoryTLAS.handle);
	_dynamicDescriptorSetInventory.Update(g_device, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, get_current_frame()._inventoryCamDataBuffer.buffer);
	_dynamicDescriptorSetInventory.Update(g_device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._meshInstancesInventoryBuffer.buffer);
	_dynamicDescriptorSetInventory.Update(g_device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._meshInstances2DBuffer.buffer);
	_dynamicDescriptorSetInventory.Update(g_device, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._lightRenderInfoBufferInventory.buffer);
	
	_dynamicDescriptorSet.Update(g_device, 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &get_current_frame()._sceneTLAS.handle);
	_dynamicDescriptorSet.Update(g_device, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, get_current_frame()._sceneCamDataBuffer.buffer);
	_dynamicDescriptorSet.Update(g_device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._meshInstancesSceneBuffer.buffer);	
	_dynamicDescriptorSet.Update(g_device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._meshInstances2DBuffer.buffer);
	_dynamicDescriptorSet.Update(g_device, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, get_current_frame()._lightRenderInfoBuffer.buffer);
}


void VulkanBackEnd::blit_render_target(VkCommandBuffer commandBuffer, RenderTarget& source, RenderTarget& destination, VkFilter filter) {
	VkImageBlit region;
	region.srcOffsets[0].x = 0;
	region.srcOffsets[0].y = 0;
	region.srcOffsets[0].z = 0;
	region.srcOffsets[1].x = source._extent.width;
	region.srcOffsets[1].y = source._extent.height;
	region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
	region.dstOffsets[0].x = 0;
	region.dstOffsets[0].y = 0;
	region.dstOffsets[0].z = 0;
	region.dstOffsets[1].x = destination._extent.width;
	region.dstOffsets[1].y = destination._extent.height;
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
	vkCmdBlitImage(commandBuffer, source._image, srcLayout, destination._image, dstLayout, regionCount, &region, filter);
}

void VulkanBackEnd::build_rt_command_buffers(int swapchainIndex)
{
	// Now fill your command buffer
	int32_t frameIndex = _frameNumber % FRAME_OVERLAP;
	VkCommandBuffer commandBuffer = _frames[frameIndex]._commandBuffer;
	VkCommandBufferBeginInfo cmdBufInfo = vkinit::command_buffer_begin_info();
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

	if (_usePathRayTracer) {
		cmd_BindRayTracingPipeline(commandBuffer, _raytracerPath.pipeline);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 0, _dynamicDescriptorSet);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 1, _staticDescriptorSet);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 2, _samplerDescriptorSet);

		// Ray trace main image
		_renderTargets.rt_first_hit_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		_renderTargets.rt_first_hit_normals.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		_renderTargets.rt_second_hit_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		_renderTargets.rt_first_hit_base_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		vkCmdTraceRaysKHR(commandBuffer, &_raytracerPath.raygenShaderSbtEntry, &_raytracerPath.missShaderSbtEntry, &_raytracerPath.hitShaderSbtEntry, &_raytracerPath.callableShaderSbtEntry, _renderTargets.rt_first_hit_color._extent.width, _renderTargets.rt_first_hit_color._extent.height, 1);

		// Inventory?
		if (GameData::inventoryOpen) {
			_renderTargets.rt_first_hit_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerPath.pipelineLayout, 0, _dynamicDescriptorSetInventory);	
			vkCmdTraceRaysKHR(commandBuffer, &_raytracerPath.raygenShaderSbtEntry, &_raytracerPath.missShaderSbtEntry, &_raytracerPath.hitShaderSbtEntry, &_raytracerPath.callableShaderSbtEntry, _renderTargets.rt_first_hit_color._extent.width, _renderTargets.rt_first_hit_color._extent.height, 1);
		}


		// Now blit that image into the presentTarget
		//_renderTargets.rt_first_hit_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		//_renderTargets.present.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		//blit_render_target(commandBuffer, _renderTargets.rt_first_hit_color, _renderTargets.present, VkFilter::VK_FILTER_LINEAR);

		// Do camera ray
		//_renderTargets.rt.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		//vkCmdTraceRaysKHR(commandBuffer, &_raytracerPath.raygenShaderSbtEntry, &_raytracerPath.missShaderSbtEntry, &_raytracerPath.hitShaderSbtEntry, &_raytracerPath.callableShaderSbtEntry, 1, 1, 1);
	} 
	else
	{
		/*cmd_BindRayTracingPipeline(commandBuffer, _raytracer.pipeline);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 0, _dynamicDescriptorSet);
		cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 1, _staticDescriptorSet);

		// Ray trace main image
		_renderTargets.rt_scene.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		vkCmdTraceRaysKHR(commandBuffer, &_raytracer.raygenShaderSbtEntry, &_raytracer.missShaderSbtEntry, &_raytracer.hitShaderSbtEntry, &_raytracer.callableShaderSbtEntry, _renderTargets.rt_scene._extent.width, _renderTargets.rt_scene._extent.height, 1);

		// Now blit that image into the presentTarget
		_renderTargets.rt_scene.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		_renderTargets.present.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		blit_render_target(commandBuffer, _renderTargets.rt_scene, _renderTargets.present, VkFilter::VK_FILTER_LINEAR);*/

	}



	// Mouse pick
	_renderTargets.rt_first_hit_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	cmd_BindRayTracingPipeline(commandBuffer, _raytracerMousePick.pipeline);
	cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerMousePick.pipelineLayout, 0, _dynamicDescriptorSet);
	cmd_BindRayTracingDescriptorSet(commandBuffer, _raytracerMousePick.pipelineLayout, 1, _staticDescriptorSet);
	vkCmdTraceRaysKHR(commandBuffer, &_raytracerMousePick.raygenShaderSbtEntry, &_raytracerMousePick.missShaderSbtEntry, &_raytracerMousePick.hitShaderSbtEntry, &_raytracerMousePick.callableShaderSbtEntry, 1, 1, 1);
		

	// UI //
	_renderTargets.present.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);



	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;


	///////////////////////////
	// Render laptop display //

	{
		Texture* bg_texture = AssetManager::GetTexture("OS_bg");

		if (bg_texture) {

			// BARRIERS
			bg_texture->insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			_renderTargets.laptopDisplay.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			// Blit initial background
			VkImageBlit region;
			region.srcOffsets[0].x = 0;
			region.srcOffsets[0].y = 0;
			region.srcOffsets[0].z = 0;
			region.srcOffsets[1].x = bg_texture->_width;
			region.srcOffsets[1].y = bg_texture->_height;;
			region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
			region.dstOffsets[0].x = 0;
			region.dstOffsets[0].y = 0;
			region.dstOffsets[0].z = 0;
			region.dstOffsets[1].x = _renderTargets.laptopDisplay._extent.width;
			region.dstOffsets[1].y = _renderTargets.laptopDisplay._extent.height;
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
			vkCmdBlitImage(commandBuffer, bg_texture->image._image, srcLayout, _renderTargets.laptopDisplay._image, dstLayout, regionCount, &region, VkFilter::VK_FILTER_NEAREST);

			// BARRIERS
			bg_texture->insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			_renderTargets.laptopDisplay.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			// Render LAPTOP UI
			std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
			VkRenderingAttachmentInfoKHR colorAttachment = {};
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue = { 0.2, 1.0, 0, 0 };
			colorAttachment.imageView = _renderTargets.laptopDisplay._view;
			colorAttachments.push_back(colorAttachment);

			VkRenderingInfoKHR renderingInfo{};
			renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
			renderingInfo.renderArea = { 0, 0, _renderTargets.laptopDisplay._extent.width, _renderTargets.laptopDisplay._extent.height };
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = colorAttachments.size();
			renderingInfo.pColorAttachments = colorAttachments.data();
			renderingInfo.pDepthAttachment = nullptr;
			renderingInfo.pStencilAttachment = nullptr;

			vkCmdBeginRendering(commandBuffer, &renderingInfo);
			cmd_SetViewportSize(commandBuffer, _renderTargets.laptopDisplay);
			cmd_BindPipeline(commandBuffer, _pipelines.textBlitter);
			cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, _dynamicDescriptorSet);
			cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, _staticDescriptorSet);

			// Draw Text plus maybe crosshair
			for (int i = 0; i < RasterRenderer::instanceCount; i++) {
				if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::LAPTOP_DISPLAY)
					RasterRenderer::DrawMesh(commandBuffer, i);
			}
			vkCmdEndRendering(commandBuffer);
		}
	}

	/////////////
	// DENOISE //

	{
		// denoise pass A:  read secound hit texture, write to texture A
		VkRenderingAttachmentInfoKHR colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureA._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureA._extent.width, _renderTargets.denoiseTextureA._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.rt_second_hit_color.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureA);
		cmd_BindPipeline(commandBuffer, _pipelines.denoisePassA);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoisePassA, 0, _samplerDescriptorSet);
		int quadMeshIndex = AssetManager::GetModel("fullscreen_quad")->_meshIndices[0];
		Mesh* mesh = AssetManager::GetMesh(quadMeshIndex);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
	
		// now horizontal blur that:  read texture A, write to texture B
		colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureB._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureB._extent.width, _renderTargets.denoiseTextureB._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.denoiseTextureA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureB);
		cmd_BindPipeline(commandBuffer, _pipelines.denoiseBlurHorizontal);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoiseBlurHorizontal, 0, _samplerDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);

		// now vertical blur that: 	read texture B, write to texture C
		colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureC._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureC._extent.width, _renderTargets.denoiseTextureC._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.denoiseTextureB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureC.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureC);
		cmd_BindPipeline(commandBuffer, _pipelines.denoiseBlurVertical);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoiseBlurVertical, 0, _samplerDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
		
		// denoise pass B:  read texture C, write to texture A
		colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureA._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureA._extent.width, _renderTargets.denoiseTextureA._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.denoiseTextureC.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureA);
		cmd_BindPipeline(commandBuffer, _pipelines.denoisePassB);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoisePassB, 0, _samplerDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
		
		
		// now horizontal blur that:  read texture A, write to texture B
		colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureB._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureB._extent.width, _renderTargets.denoiseTextureB._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.denoiseTextureA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureB);
		cmd_BindPipeline(commandBuffer, _pipelines.denoiseBlurHorizontal);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoiseBlurHorizontal, 0, _samplerDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);

		// now vertical blur that: 	read texture B, write to texture C
		colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureC._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureC._extent.width, _renderTargets.denoiseTextureC._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.denoiseTextureB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureC.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureC);
		cmd_BindPipeline(commandBuffer, _pipelines.denoiseBlurVertical);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoiseBlurVertical, 0, _samplerDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
		
		// denoise pass C: read texture C, write to texture B
		colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.denoiseTextureB._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.denoiseTextureB._extent.width, _renderTargets.denoiseTextureB._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargets.denoiseTextureC.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		_renderTargets.denoiseTextureB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.denoiseTextureB);
		cmd_BindPipeline(commandBuffer, _pipelines.denoisePassC);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.denoisePassC, 0, _samplerDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
	}


	/*
	////////////////////
	// Render GBuffer //
	if (false) {
		std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
		VkRenderingAttachmentInfoKHR colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = { 0.2, 1.0, 0, 0 };
		colorAttachment.imageView = _renderTargets.gBufferBasecolor._view;
		colorAttachments.push_back(colorAttachment);
		colorAttachment.imageView = _renderTargets.gBufferNormal._view;
		colorAttachments.push_back(colorAttachment);
		colorAttachment.imageView = _renderTargets.gBufferRMA._view;
		colorAttachments.push_back(colorAttachment);

		VkRenderingAttachmentInfoKHR depthStencilAttachment{};
		depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthStencilAttachment.imageView = _gbufferDepthTarget._view;
		depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
		depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.gBufferNormal._extent.width, _renderTargets.gBufferNormal._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colorAttachments.size();
		renderingInfo.pColorAttachments = colorAttachments.data();
		renderingInfo.pDepthAttachment = &depthStencilAttachment;
		renderingInfo.pStencilAttachment = nullptr;

		_renderTargets.gBufferNormal.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.gBufferBasecolor);
		cmd_BindPipeline(commandBuffer, _rasterPipeline);
		cmd_BindDescriptorSet(commandBuffer, _rasterPipeline, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _rasterPipeline, 1, _staticDescriptorSet);

		if (_renderGBuffer) {
			auto meshes = Scene::GetSceneMeshes(_debugScene);
			for (int i = 0; i < meshes.size(); i++) {
				meshes[i]->draw(commandBuffer, i);
			}
		}

		vkCmdEndRendering(commandBuffer);
	}*/

	//////////////////////
	// Denoise Pass(es) //
	/*if (false)
	{
		
		// begin by blitting the noisy image into denoiseB texture
		_renderTargets.rt_indirect_noise.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		_renderTargetDenoiseB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		blit_render_target(commandBuffer, _renderTargets.rt_indirect_noise, _renderTargetDenoiseB, VkFilter::VK_FILTER_LINEAR);
		
		// return this to general so it can be written by the main RT pass
		_renderTargets.rt_indirect_noise.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	
		// now denoise that image, and output into texture A
		VkRenderingAttachmentInfoKHR colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargetDenoiseA._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargetDenoiseA._extent.width, _renderTargetDenoiseA._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		_renderTargetDenoiseB.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		_renderTargetDenoiseA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargetDenoiseA);
		cmd_BindPipeline(commandBuffer, _denoisePipeline);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 1, _staticDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 2, _samplerDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 3, _denoiseBTextureDescriptorSet);
		int quadMeshIndex = AssetManager::GetModel("fullscreen_quad")->_meshIndices[0];
		Mesh* mesh = AssetManager::GetMesh(quadMeshIndex);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);

		// now setup the ping pong
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargetDenoiseB._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargetDenoiseB._extent.width, _renderTargetDenoiseB._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargetDenoiseB);
		cmd_BindPipeline(commandBuffer, _denoisePipeline);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 1, _staticDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 2, _samplerDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 3, _denoiseATextureDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);

		// now setup the ping pong
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargetDenoiseA._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargetDenoiseA._extent.width, _renderTargetDenoiseA._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargetDenoiseA);
		cmd_BindPipeline(commandBuffer, _denoisePipeline);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 1, _staticDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 2, _samplerDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 3, _denoiseBTextureDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);

		// now setup the ping pong
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargetDenoiseB._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargetDenoiseB._extent.width, _renderTargetDenoiseB._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargetDenoiseB);
		cmd_BindPipeline(commandBuffer, _denoisePipeline);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 1, _staticDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 2, _samplerDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _denoisePipeline, 3, _denoiseATextureDescriptorSet);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
	}*/


	////////////////////
	// Composite Pass //

	{
		VkRenderingAttachmentInfoKHR colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = _renderTargets.composite._view;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = { 0.2, 1.0, 0, 0 };

		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.composite._extent.width, _renderTargets.composite._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = VK_NULL_HANDLE;
		renderingInfo.pStencilAttachment = nullptr;
		
		_renderTargets.composite.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		//_renderTargetDenoiseA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.composite);
		cmd_BindPipeline(commandBuffer, _pipelines.composite);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 1, _staticDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 2, _samplerDescriptorSet);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 3, _denoiseATextureDescriptorSet);
		//cmd_BindDescriptorSet(commandBuffer, _pipelines.composite, 4, _denoiseBTextureDescriptorSet);

		int quadMeshIndex = AssetManager::GetModel("fullscreen_quad")->_meshIndices[0];
		Mesh* mesh = AssetManager::GetMesh(quadMeshIndex);
		mesh->draw(commandBuffer, 0);
		vkCmdEndRendering(commandBuffer);
	}


	// Blit the gbuffer normal texture back into present
	//if (_renderGBuffer) 
	{
		_renderTargets.composite.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		_renderTargets.present.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		blit_render_target(commandBuffer, _renderTargets.composite, _renderTargets.present, VkFilter::VK_FILTER_LINEAR);
	}


	///////////////
	// Render UI //
	{
		std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
		VkRenderingAttachmentInfoKHR colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = { 0.2, 1.0, 0, 0 };
		colorAttachment.imageView = _renderTargets.present._view;
		colorAttachments.push_back(colorAttachment);

		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, _renderTargets.present._extent.width, _renderTargets.present._extent.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colorAttachments.size();
		renderingInfo.pColorAttachments = colorAttachments.data();
		renderingInfo.pDepthAttachment = nullptr;
		renderingInfo.pStencilAttachment = nullptr;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		cmd_SetViewportSize(commandBuffer, _renderTargets.present);
		cmd_BindPipeline(commandBuffer, _pipelines.textBlitter);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, _dynamicDescriptorSet);
		cmd_BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, _staticDescriptorSet);

		// Draw Text plus maybe crosshair
		for (int i = 0; i < RasterRenderer::instanceCount; i++) {
			if (RasterRenderer::_UIToRender[i].destination == RasterRenderer::Destination::MAIN_UI)
				RasterRenderer::DrawMesh(commandBuffer, i);
		}
		RasterRenderer::ClearQueue();

		// Draw debug lines
		if (_lineListMesh._vertexCount > 0 && _debugMode != DebugMode::NONE) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.lines._handle);
			glm::mat4 projection = glm::perspective(GameData::_cameraZoom, 1700.f / 900.f, 0.01f, 100.0f);
			glm::mat4 view = GameData::GetPlayer().m_camera.GetViewMatrix();
			projection[1][1] *= -1;
			LineShaderPushConstants constants;
			constants.transformation = projection * view;;
			vkCmdPushConstants(commandBuffer, _pipelines.lines._layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LineShaderPushConstants), &constants);
			_lineListMesh.draw(commandBuffer, 0);
		}

		vkCmdEndRendering(commandBuffer);
	}

	///////////////////////////////////////////
	// Blit present image into the swapchain //
	{
		_renderTargets.present.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Prepare swapchain as destination for copy
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
		region.srcOffsets[1].x = _renderTargets.present._extent.width;
		region.srcOffsets[1].y = _renderTargets.present._extent.height;
		region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
		region.dstOffsets[0].x = 0;
		region.dstOffsets[0].y = 0;
		region.dstOffsets[0].z = 0;
		region.dstOffsets[1].x = _currentWindowExtent.width;
		region.dstOffsets[1].y = _currentWindowExtent.height;
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

		// Blit the image into the swapchain
		region.srcOffsets[1].x = _renderTargets.present._extent.width;
		region.srcOffsets[1].y = _renderTargets.present._extent.height;
		region.dstOffsets[1].x = _currentWindowExtent.width;
		region.dstOffsets[1].y = _currentWindowExtent.height;
		vkCmdBlitImage(commandBuffer, _renderTargets.present._image, srcLayout, _swapchainImages[swapchainIndex], dstLayout, regionCount, &region, VkFilter::VK_FILTER_NEAREST);

		// Prepare swap chain for present
		swapChainBarrier = {};
		swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapChainBarrier.image = _swapchainImages[swapchainIndex];
		swapChainBarrier.subresourceRange = range;
		swapChainBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		swapChainBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
	}

	// Get the mesh index of whatever triangle the cursor is aiming at
	// I mean, you already have it by this point, the code below is retrieving it from the GPU buffer that contains it
	if (!GameData::inventoryOpen) {
		uint32_t bufferSize = sizeof(uint32_t) * 2;
		VkBufferCopy bufCopy = {
		0, // srcOffset
		0, // dstOffset,
		bufferSize };
		vkCmdCopyBuffer(commandBuffer, _mousePickResultBuffer._buffer, _mousePickResultCPUBuffer._buffer, 1, &bufCopy);
		VK_CHECK(vkEndCommandBuffer(commandBuffer));
		uint32_t mousePickResult[2] = { 2,0 };
		void* data;
		memcpy(mousePickResult, _mousePickResultCPUBuffer._mapped, sizeof(uint32_t) * 2);
		Scene::StoreMousePickResult(mousePickResult[0], mousePickResult[1]);
	}
	else {
		VK_CHECK(vkEndCommandBuffer(commandBuffer));
		Scene::StoreMousePickResult(-1, -1);
	}
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
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &_lineListMesh._vertexBuffer._buffer, &_lineListMesh._vertexBuffer._allocation, nullptr));
		add_debug_name(_lineListMesh._vertexBuffer._buffer, "_lineListMesh._vertexBuffer");
		// Name the mesh
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		nameInfo.objectHandle = (uint64_t)_lineListMesh._vertexBuffer._buffer;
		nameInfo.pObjectName = "Line list mesh";
		vkSetDebugUtilsObjectNameEXT(g_device, &nameInfo); 
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

	_lineListMesh._vertexCount = vertices.size();

	if (vertices.size()){
		const size_t bufferSize = vertices.size() * sizeof(Vertex);
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;
		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		AllocatedBuffer stagingBuffer;
		VK_CHECK(vmaCreateBuffer(GetAllocator(), &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
		add_debug_name(stagingBuffer._buffer, "stagingBuffer");
		void* data;
		vmaMapMemory(GetAllocator(), stagingBuffer._allocation, &data);
		memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
		vmaUnmapMemory(GetAllocator(), stagingBuffer._allocation);
		immediate_submit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _lineListMesh._vertexBuffer._buffer, 1, &copy);
			});
		vmaDestroyBuffer(GetAllocator(), stagingBuffer._buffer, stagingBuffer._allocation);
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

void VulkanBackEnd::cmd_SetViewportSize(VkCommandBuffer commandBuffer, RenderTarget renderTarget) {
	cmd_SetViewportSize(commandBuffer, renderTarget._extent.width, renderTarget._extent.height);
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
