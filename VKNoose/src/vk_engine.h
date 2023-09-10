#pragma once

#include "vk_types.h"
#include "vk_mesh.h"
#include "vk_raytracing.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "Audio/Audio.h"
#include "Game/GameData.h"
#include "IO/Input.h"
#include "Renderer/Shader.h"
#include "UI/TextBlitter.h"

constexpr unsigned int FRAME_OVERLAP = 2;

class PipelineBuilder {
public:

	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
	VkPipeline build_dynamic_rendering_pipeline(VkDevice device, const VkFormat* swapchainFormat, VkFormat depthFormat, int colorAttachmentCount);
};


struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};


struct CameraData {
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 projInverse;
	glm::mat4 viewInverse;
	glm::vec4 viewPos;
	int32_t vertexSize;
	int32_t frameIndex;
	int32_t inventoryOpen;
	int32_t wallPaperALBIndex;
};


struct UploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct LineShaderPushConstants {
	glm::mat4 transformation;
};

struct RenderObject {
	Mesh* mesh;
	Transform transform;
	bool spin = false;
};

#define TEXTURE_ARRAY_SIZE 256
#define MAX_RENDER_OBJECTS_3D 1000
#define MAX_RENDER_OBJECTS_2D 1000
#define MAX_LIGHTS 16

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _frameDeletionQueue;

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
		
	HellBuffer _sceneCamDataBuffer;
	HellBuffer _inventoryCamDataBuffer;
	HellBuffer _meshInstances2DBuffer;
	HellBuffer _meshInstancesSceneBuffer;
	HellBuffer _meshInstancesInventoryBuffer;
	HellBuffer _lightRenderInfoBuffer;
	HellBuffer _lightRenderInfoBufferInventory;

	AccelerationStructure _sceneTLAS{};
	AccelerationStructure _inventoryTLAS{};
};


struct RayTracingScratchBuffer
{
	uint64_t deviceAddress = 0;
	AllocatedBuffer handle;// VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};


class VulkanEngine {
public:

	void create_buffers();
	void UpdateBuffers();
	void update_static_descriptor_set();
	void UpdateDynamicDescriptorSet();
	
	// Commands
	void cmd_SetViewportSize(VkCommandBuffer commandBuffer, int width, int height);
	void cmd_SetViewportSize(VkCommandBuffer commandBuffer, RenderTarget renderTarget);
	void cmd_BindPipeline(VkCommandBuffer commandBuffer, HellPipeline& pipeline);
	void cmd_BindDescriptorSet(VkCommandBuffer commandBuffer, HellPipeline& pipeline, uint32_t setIndex, HellDescriptorSet& descriptorSet);
	void cmd_BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);
	void cmd_BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, HellDescriptorSet& descriptorSet);

	uint32_t _frameIndex;

	HellDescriptorSet _staticDescriptorSet;
	HellDescriptorSet _dynamicDescriptorSet;
	HellDescriptorSet _dynamicDescriptorSetInventory;
	HellDescriptorSet _samplerDescriptorSet;
	HellDescriptorSet _denoiseATextureDescriptorSet;
	HellDescriptorSet _denoiseBTextureDescriptorSet;

	HellPipeline _textBlitterPipeline;
	HellPipeline _rasterPipeline;
	HellPipeline _denoisePipeline;
	HellPipeline _computePipeline;
	HellPipeline _compositePipeline;

	RenderTarget _renderTargetDenoiseA;
	RenderTarget _renderTargetDenoiseB;
	
	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};

	VkSampler _sampler; 

	bool _shouldClose{ false };
	 
	// Render target shit
	VkExtent2D _currentWindowExtent{ 512 , 288  };
	//VkExtent2D _fullscreenModeExtent{ 512 , 288  };
	const VkExtent2D _windowedModeExtent{ 512 * 4, 288 * 4 };
	const VkExtent3D _renderTargetPresentExtent = { 512 , 288  , 1 };
	
	struct RenderTargets {
		RenderTarget present;
		RenderTarget rt_scene;
		RenderTarget rt_normals;
		RenderTarget rt_indirect_noise;
		//RenderTarget rt_inventory;
		RenderTarget gBufferBasecolor;
		RenderTarget gBufferNormal;
		RenderTarget gBufferRMA;
		RenderTarget laptopDisplay;
		RenderTarget composite;
	} _renderTargets;

	GLFWwindow* _window;

	int _frameNumber{ 0 };
	//int _selectedShader{ 0 };
	DebugMode _debugMode = DebugMode::NONE;

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	VkPhysicalDeviceProperties _gpuProperties;

	FrameData _frames[FRAME_OVERLAP];

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator; //vma lib allocator

	HellDepthTarget _presentDepthTarget;
	HellDepthTarget _gbufferDepthTarget;

	VkFormat _depthFormat;

	VkDescriptorPool _descriptorPool;

	UploadContext _uploadContext;

	// Shaders
	VkShaderModule _gbuffer_vertex_shader = nullptr;
	VkShaderModule _gbuffer_fragment_shader = nullptr;
	VkShaderModule _solid_color_vertex_shader = nullptr;
	VkShaderModule _solid_color_fragment_shader = nullptr;
	VkShaderModule _text_blitter_vertex_shader = nullptr;
	VkShaderModule _text_blitter_fragment_shader = nullptr;
	VkShaderModule _depth_aware_blur_vertex_shader = nullptr;
	VkShaderModule _depth_aware_blur_fragment_shader = nullptr;
	VkShaderModule _denoiser_compute_shader = nullptr;
	VkShaderModule _composite_vertex_shader = nullptr;
	VkShaderModule _composite_fragment_shader = nullptr;

	//initializes everything in the engine
	void init();
	void init_raytracing();
	void cleanup();
	void draw();
	void update(float deltaTime);
	void run();
	void render_loading_screen();

	// Pipelines


	VkPipeline _linelistPipeline;
	VkPipelineLayout _linelistPipelineLayout;

	FrameData& get_current_frame();
	FrameData& get_last_frame();

	Mesh _lineListMesh;


	HellRaytracer _raytracer;
	HellRaytracer _raytracerPath;
	HellRaytracer _raytracerMousePick;

	bool _collisionEnabled = true;
	bool _debugScene = false;
	bool _renderGBuffer = false;// true;
	bool _usePathRayTracer = true;// true;

		// Ray tracing

		RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
		uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr) const;
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

			void create_rt_buffers();
			AccelerationStructure createBottomLevelAccelerationStructure(Mesh* mesh);
			void create_top_level_acceleration_structure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS);
			
			void build_rt_command_buffers(int swapchainIndex);
			AllocatedBuffer _rtVertexBuffer;
			AllocatedBuffer _rtIndexBuffer;
			AllocatedBuffer _mousePickResultBuffer;
			AllocatedBuffer _mousePickResultCPUBuffer; // for mouse picking
			uint32_t _rtIndexCount;

			VkDeviceOrHostAddressConstKHR _vertexBufferDeviceAddress{};
			VkDeviceOrHostAddressConstKHR _indexBufferDeviceAddress{};
			VkDeviceOrHostAddressConstKHR _transformBufferDeviceAddress{};

			AllocatedBuffer _rtInstancesBuffer;



	bool _frameBufferResized = false;
	bool was_frame_buffer_resized() { return _frameBufferResized; }
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


	void blit_render_target(VkCommandBuffer commandBuffer, RenderTarget& source, RenderTarget& destination, VkFilter filter);

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags = 0);

	size_t pad_uniform_buffer_size(size_t originalSize);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	void submit_barrier_command_swapchain_to_transfer_dst_optimal();
	void submit_barrier_command_swapchain_to_present_src_khr();

	//void generate_mipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	//VkCommandBuffer beginSingleTimeCommands();
	//void endSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
	void init_vulkan();
	void create_swapchain();
	//void init_commands();
	void create_sync_structures();
	void create_descriptors();
	void create_sampler();
	void upload_meshes();
	void upload_mesh(Mesh& mesh);

	void recreate_dynamic_swapchain();
	void load_shaders();
	void cleanup_shaders();
	void hotload_shaders();

	void create_command_buffers();
	void create_pipelines();
	void create_render_targets();
	void draw_quad(Transform transform, Texture* texture);

	void cleanup_raytracing();
	uint64_t get_buffer_device_address(VkBuffer buffer);
	void createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	void AddDebugText();
	void add_debug_name(VkBuffer buffer, const char* name);
	void add_debug_name(VkDescriptorSetLayout descriptorSetLayout, const char* name);
	void get_required_lines();
};

inline void PrintMat4(glm::mat4& m) {
	std::cout << "\n";
	for (int x = 0; x < 4; x++) {
		std::cout << "(";
		for (int y = 0; y < 4; y++)
		{
			std::cout << m[x][y] << " ";
		}
		std::cout << ")\n";
	}
}