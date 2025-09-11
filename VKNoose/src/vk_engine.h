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
#include "Input/Input.h"
#include "Renderer/Shader.h"
#include "UI/TextBlitter.h"

#include "Renderer/Pipeline.hpp"

constexpr unsigned int FRAME_OVERLAP = 2;

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
#define MAX_RENDER_OBJECTS_2D 5000
#define MAX_LIGHTS 16

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

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


namespace Vulkan {

	inline bool _loaded = false;

	void InitMinimum();
	void CreateWindow();
	void CreateInstance();
	void SelectPhysicalDevice(); 
	void LoadShaders();
	void CreateSwapchain();
	
	void cleanup_shaders();
	void hotload_shaders();

	void RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer);
	void BlitRenderTargetIntoSwapChain(VkCommandBuffer commandBuffer, RenderTarget& renderTarget, uint32_t swapchainImageIndex);
	void PrepareSwapchainForPresent(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);

	void ToggleFullscreen();
	//bool ProgramShouldClose();
	bool ProgramIsMinimized();

	void UpdateBuffers2D();

	void LoadNextItem();

	void AddLoadingText(std::string text);

	GLFWwindow* GetWindow();

	inline bool _forceCloseWindow { false };

	inline struct RenderTargets {
		RenderTarget present;
		RenderTarget rt_first_hit_color;
		RenderTarget rt_first_hit_normals;
		RenderTarget rt_first_hit_base_color;
		RenderTarget rt_second_hit_color;
		RenderTarget gBufferBasecolor;
		RenderTarget gBufferNormal;
		RenderTarget gBufferRMA;
		RenderTarget laptopDisplay;
		RenderTarget composite;
		RenderTarget denoiseTextureA;
		RenderTarget denoiseTextureB;
		RenderTarget denoiseTextureC;
	} _renderTargets;

	inline struct Pipelines {
		Pipeline composite;
		Pipeline textBlitter;
		Pipeline lines;
		Pipeline denoisePassA;
		Pipeline denoisePassB;
		Pipeline denoisePassC;
		Pipeline denoiseBlurHorizontal;
		Pipeline denoiseBlurVertical;
	} _pipelines;

	// Shaders
	inline VkShaderModule _gbuffer_vertex_shader = nullptr;
	inline VkShaderModule _gbuffer_fragment_shader = nullptr;
	inline VkShaderModule _solid_color_vertex_shader = nullptr;
	inline VkShaderModule _solid_color_fragment_shader = nullptr;
	inline VkShaderModule _text_blitter_vertex_shader = nullptr;
	inline VkShaderModule _text_blitter_fragment_shader = nullptr;
	inline VkShaderModule _denoise_pass_A_vertex_shader = nullptr;
	inline VkShaderModule _denoise_pass_A_fragment_shader = nullptr;
	inline VkShaderModule _denoise_pass_B_vertex_shader = nullptr;
	inline VkShaderModule _denoise_pass_B_fragment_shader = nullptr;
	inline VkShaderModule _denoise_pass_C_vertex_shader = nullptr;
	inline VkShaderModule _denoise_pass_C_fragment_shader = nullptr;
	inline VkShaderModule _blur_vertical_vertex_shader = nullptr;
	inline VkShaderModule _blur_vertical_fragment_shader = nullptr;
	inline VkShaderModule _blur_horizontal_vertex_shader = nullptr;
	inline VkShaderModule _blur_horizontal_fragment_shader = nullptr;
	inline VkShaderModule _composite_vertex_shader = nullptr;
	inline VkShaderModule _composite_fragment_shader = nullptr;


	VmaAllocator GetAllocator();
	VkDevice GetDevice();


	void create_buffers();
	void UpdateBuffers();
	void update_static_descriptor_set();
	void UpdateDynamicDescriptorSet();
	
	// Commands
	void cmd_SetViewportSize(VkCommandBuffer commandBuffer, int width, int height);
	void cmd_SetViewportSize(VkCommandBuffer commandBuffer, RenderTarget renderTarget);
	void cmd_BindPipeline(VkCommandBuffer commandBuffer, Pipeline& pipeline);
	void cmd_BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, HellDescriptorSet& descriptorSet);
	void cmd_BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);
	void cmd_BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, HellDescriptorSet& descriptorSet);

	inline uint32_t _frameIndex;

	inline HellDescriptorSet _staticDescriptorSet;
	inline HellDescriptorSet _dynamicDescriptorSet;
	inline HellDescriptorSet _dynamicDescriptorSetInventory;
	inline HellDescriptorSet _samplerDescriptorSet;
	//inline HellDescriptorSet _denoiseATextureDescriptorSet;
	//inline HellDescriptorSet _denoiseBTextureDescriptorSet;
	
	inline VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};

	inline VkSampler _sampler;
		 
	// Render target shit
	inline VkExtent2D _currentWindowExtent{ 512 , 288  };
	//VkExtent2D _fullscreenModeExtent{ 512 , 288  };
	inline const VkExtent2D _windowedModeExtent{ 512 * 4, 288 * 4 };
	inline const VkExtent3D _renderTargetPresentExtent = { 512 , 288  , 1 };
	



	inline int _frameNumber{ 0 };
	//int _selectedShader{ 0 };
	inline DebugMode _debugMode = DebugMode::NONE;

	inline VkInstance _instance;
	inline VkPhysicalDevice _physicalDevice;

	inline VkPhysicalDeviceProperties _gpuProperties;

	inline FrameData _frames[FRAME_OVERLAP];

	inline VkQueue _graphicsQueue;
	inline uint32_t _graphicsQueueFamily;

	inline VkSurfaceKHR _surface;
	inline VkSwapchainKHR _swapchain;
	inline VkFormat _swachainImageFormat;

	inline std::vector<VkFramebuffer> _framebuffers;
	inline std::vector<VkImage> _swapchainImages;
	inline std::vector<VkImageView> _swapchainImageViews;

	inline HellDepthTarget _presentDepthTarget;
	inline HellDepthTarget _gbufferDepthTarget;

	inline VkFormat _depthFormat;

	inline VkDescriptorPool _descriptorPool;

	inline UploadContext _uploadContext;



	//initializes everything in the engine
	
	void init_raytracing();
	void Cleanup();
	void RenderGameFrame();
	void RenderLoadingFrame();

	// Pipelines


	//inline VkPipeline _linelistPipeline;
	//inline VkPipelineLayout _linelistPipelineLayout;

	inline FrameData& get_current_frame();
	inline FrameData& get_last_frame();

	inline Mesh _lineListMesh;


	inline HellRaytracer _raytracer;
	inline HellRaytracer _raytracerPath;
	inline HellRaytracer _raytracerMousePick;

	inline bool _collisionEnabled = true;
	inline bool _debugScene = false;
	inline bool _renderGBuffer = false;// true;
	inline bool _usePathRayTracer = true;// true;

		// Ray tracing

		RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
		uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr);
		inline VkPhysicalDeviceMemoryProperties _memoryProperties;

		inline PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
		inline PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		inline PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		inline PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
		inline PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
		inline PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
		inline PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
		inline PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		inline PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		inline PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
		inline PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
		inline PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
		inline PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin;
		inline PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd;
		inline PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert;
		inline PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

		inline VkPhysicalDeviceRayTracingPipelinePropertiesKHR  _rayTracingPipelineProperties{};
		inline VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

			void create_rt_buffers();
			inline AccelerationStructure createBottomLevelAccelerationStructure(Mesh* mesh);
			void create_top_level_acceleration_structure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS);
			
			void build_rt_command_buffers(int swapchainIndex);
			inline AllocatedBuffer _rtVertexBuffer;
			inline AllocatedBuffer _rtIndexBuffer;
			inline AllocatedBuffer _mousePickResultBuffer;
			inline AllocatedBuffer _mousePickResultCPUBuffer; // for mouse picking
			inline uint32_t _rtIndexCount;

			inline VkDeviceOrHostAddressConstKHR _vertexBufferDeviceAddress{};
			inline VkDeviceOrHostAddressConstKHR _indexBufferDeviceAddress{};
			inline VkDeviceOrHostAddressConstKHR _transformBufferDeviceAddress{};

			inline AllocatedBuffer _rtInstancesBuffer;



	
	//bool was_frame_buffer_resized() { return _frameBufferResized; }
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

	//void init_commands();
	void create_sync_structures();
	void create_descriptors();
	void create_sampler();
	void upload_meshes();
	void upload_mesh(Mesh& mesh);

	void recreate_dynamic_swapchain();


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