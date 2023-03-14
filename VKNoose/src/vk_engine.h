#pragma once

#include "vk_types.h"
#include "vk_mesh.h"

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
#include "Game/Player.h"
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
	VkPipeline build_dynamic_rendering_pipeline(VkDevice device, const VkFormat* swapchainFormat, VkFormat depthFormat);
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


struct Texture {
	AllocatedImage image;
	VkImageView imageView;
	int _width = 0;
	int _height = 0;
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


struct Material {
	VkDescriptorSet textureSet{ VK_NULL_HANDLE };
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;
	Material* material;
	Transform transform;
	bool spin = false;
};

#define TEXTURE_ARRAY_SIZE 128
#define MAX_RENDER_OBJECTS 10000

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _frameDeletionQueue;

	VkCommandPool _commandPool;
	//VkCommandBuffer _mainCommandBuffer;
	VkCommandBuffer _commandBuffer;

	AllocatedBuffer cameraBuffer;
	VkDescriptorSet globalDescriptor;

	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;

	AllocatedBuffer _uboBuffer_rayTracing;
	VkDescriptorBufferInfo _uboBuffer_rayTracing_descriptor;
};

struct GPUCameraData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
	glm::vec4 viewPos;
};


struct GPUSceneData {
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};


struct RayTracingScratchBuffer
{
	uint64_t deviceAddress = 0;
	AllocatedBuffer handle;// VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

// Ray tracing acceleration structure
struct AccelerationStructure {
	VkAccelerationStructureKHR handle;
	uint64_t deviceAddress = 0;
	VkDeviceMemory memory;
	//VkBuffer buffer;
	AllocatedBuffer buffer;
};

struct GameData {
	Player player;
};

class VulkanEngine {
public:




	//PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
	//PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};

	GameData _gameData;


	VkSampler _sampler; 
	VkDescriptorSetLayout _textureArrayDescriptorLayout;
	VkDescriptorSet	_textureArrayDescriptorSet;

	bool _shouldClose{ false };

	VkExtent2D _windowExtent{ 1700 , 900 };
	VkExtent3D _renderTargetExtent = { 512, 288, 1 };

	GLFWwindow* _window;

	int _frameNumber{ 0 };
	int _selectedShader{ 0 };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	VkPhysicalDeviceProperties _gpuProperties;

	FrameData _frames[FRAME_OVERLAP];

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkRenderPass _renderPass;

	VkSurfaceKHR _surface;
	//std::unique_ptr<VkSwapchainKHR> _swapchain; 
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator; //vma lib allocator


	VkImageView    _renderTargetImageView;
	AllocatedImage _renderTargetImage;

	//depth resources
	VkImageView _depthImageView;
	AllocatedImage _depthImage;

	//the format for the depth image
	VkFormat _depthFormat;

	VkDescriptorPool _descriptorPool;

	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _objectSetLayout;
	VkDescriptorSetLayout _singleTextureSetLayout;
	VkDescriptorSetLayout _bindlessTextureSetLayout;

	GPUSceneData _sceneParameters;
	AllocatedBuffer _sceneParameterBuffer;

	UploadContext _uploadContext;

	VkShaderModule _colorMeshShader;
	VkShaderModule _texturedMeshShader;
	VkShaderModule _meshVertShader;

	VkShaderModule _text_blitter_vertex_shader;
	VkShaderModule _text_blitter_fragment_shader;

	//initializes everything in the engine
	void init();
	void cleanup();
	void draw();
	void update();
	void run();

	// Pipelines
	VkPipeline _textblitterPipeline;
	VkPipelineLayout _textblitterPipelineLayout;

	FrameData& get_current_frame();
	FrameData& get_last_frame();


	float _cameraZoom = 1.0f;// glm::radians(70.f);

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

			VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
			VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

			//VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
			//VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
			//VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};

			//kPhysicalDeviceRayTracingPipelinePropertiesKHR _rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
			AccelerationStructure _bottomLevelAS{};
			AccelerationStructure _topLevelAS{};
			void createBottomLevelAccelerationStructure();
			void createTopLevelAccelerationStructure();
			void createStorageImage();
			void createUniformBuffer();
			void createRayTracingPipeline();
			void createShaderBindingTable();
			void createDescriptorSets();
			void build_rt_command_buffers(int swapchainIndex);
			void updateUniformBuffers();
			//vks::Buffer vertexBuffer;
			//vks::Buffer indexBuffer;
			AllocatedBuffer _rtVertexBuffer;
			AllocatedBuffer _rtIndexBuffer;
			AllocatedBuffer _rtTransformBuffer;
			uint32_t _rtIndexCount;
			//vks::Buffer transformBuffer;
			std::vector<VkRayTracingShaderGroupCreateInfoKHR> _rtShaderGroups{};
			std::vector<VkPipelineShaderStageCreateInfo> _rtShaderStages;
			VkPipeline _rtPipeline;
			VkPipelineLayout _rtPipelineLayout;
			VkDescriptorSet _rtDescriptorSet;
			VkDescriptorSet _rtVertexBufferDescriptorSet;
			//VkDescriptorSet _rtDescriptorSet_1;
			VkDescriptorSetLayout _rtDescriptorSetLayout;
			VkDescriptorSetLayout _rtVertexBufferDescriptorSetLayout;
			//VkDescriptorSetLayout _rtDescriptorSetLayout_1;
			VkShaderModule _rayGenShader;
			VkShaderModule _rayMissShader;
			VkShaderModule _closestHitShader;

			AllocatedBuffer _raygenShaderBindingTable;
			AllocatedBuffer _missShaderBindingTable;
			AllocatedBuffer _hitShaderBindingTable;
			VkDescriptorPool _rtDescriptorPool = VK_NULL_HANDLE;

			//vks::Buffer raygenShaderBindingTable;
			//vks::Buffer missShaderBindingTable;
			//vks::Buffer hitShaderBindingTable;

			struct StorageImage {
				VkDeviceMemory memory;
				AllocatedImage image;
				//VkImage image;
				VkImageView view;
				VkFormat format;
			} _storageImage;

			struct RTUniformData {
				glm::mat4 viewInverse;
				glm::mat4 projInverse;
				glm::vec4 viewPos;
			} _uniformData;




	//vks::Buffer ubo;

	//VkPipeline pipeline;
	//VkPipelineLayout pipelineLayout;
	//VkDescriptorSet descriptorSet;
	//VkDescriptorSetLayout descriptorSetLayout;




	//default array of renderable objects
	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;
	//std::unordered_map<std::string, Texture> _loadedTextures;
	std::vector<Texture> _loadedTextures;
	 
	//functions


	bool _frameBufferResized = false;
	bool was_frame_buffer_resized() { return _frameBufferResized; }
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	//create material and add it to the map
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);

	//returns nullptr if it cant be found
	Material* get_material(const std::string& name);

	//returns nullptr if it cant be found
	Mesh* get_mesh(const std::string& name);

	//our draw function
	//void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	size_t pad_uniform_buffer_size(size_t originalSize);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	void submit_barrier_command_swapchain_to_transfer_dst_optimal();
	void submit_barrier_command_swapchain_to_present_src_khr();

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void init_scene();
	void init_descriptors();
	//bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule, VkShaderModuleCreateInfo createInfo = VkShaderModuleCreateInfo());
	void load_meshes();
	void load_images();
	void upload_mesh(Mesh& mesh);
	void load_texture(std::string filepath);

	void recreate_dynamic_swapchain();
	void load_shaders();
	void cleanup_shaders();
	void hotload_shaders();

	void create_command_buffers();
	void create_pipelines();
	void create_pipelines_2();
	void build_command_buffers(int swapchainImageIndex);
	void create_render_targets();
	void draw_quad(Transform transform, Texture* texture);

	void init_raytracing();
	void cleanup_rayracing();
	uint64_t get_buffer_device_address(VkBuffer buffer);
	void createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
};
