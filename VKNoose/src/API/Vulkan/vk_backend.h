#pragma once
#include "vk_common.h"

#include "vk_types.h"
#include "vk_mesh.h"
#include "vk_raytracing.h"


#include "Types/vk_acceleration_structure.h" // remove me soon
#include "Renderer/vk_frame_data.h" // remove me soon

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

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct LineShaderPushConstants {
	glm::mat4 transformation;
};

struct RenderObject {
	MeshOLD* mesh;
	Transform transform;
	bool spin = false;
};

#define TEXTURE_ARRAY_SIZE 256
#define MAX_RENDER_OBJECTS_3D 1000
#define MAX_RENDER_OBJECTS_2D 5000
#define MAX_LIGHTS 16



struct RayTracingScratchBufferOLD {
	uint64_t deviceAddress = 0;
	AllocatedBufferOLD handle;// VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};


namespace VulkanBackEnd {
    bool InitMinimum();

	VkDevice GetDevice();
	VkInstance GetInstance();
	VkSurfaceKHR GetSurface();
	VmaAllocator GetAllocator();

	void Cleanup();
	void RenderGameFrame();
	void RenderLoadingFrame();
	void ToggleFullscreen();
	bool ProgramIsMinimized();
	void LoadNextItem();
	void AddLoadingText(std::string text);
}


namespace VulkanBackEnd {


	inline bool _loaded = false;

	
	void LoadLegacyShaders();
	
	void hotload_shaders();

	void RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer);
	void PrepareSwapchainForPresent(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);


	void UpdateBuffers2D();


	GLFWwindow* GetWindow();

	inline bool _forceCloseWindow { false };


	void UpdateBuffers();
	void update_static_descriptor_set_old();
	void UpdateStaticDescriptorSet(); // MOVE ME TO VULKANRENDERER when you can!
	void UpdateDynamicDescriptorSet();
	
	// Commands
	void cmd_SetViewportSize(VkCommandBuffer commandBuffer, int width, int height);
	void cmd_BindPipeline(VkCommandBuffer commandBuffer, Pipeline& pipeline);
	void cmd_BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, HellDescriptorSet& descriptorSet);
	void cmd_BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);
	void cmd_BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, HellDescriptorSet& descriptorSet);

	inline uint32_t _frameIndex;

	inline VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};

		 
	inline const VkExtent2D _windowedModeExtent{ 512 * 4, 288 * 4 };
	inline const VkExtent3D _renderTargetPresentExtent = { 512 , 288  , 1 };
	


	inline DebugMode _debugMode = DebugMode::NONE;


	


	inline std::vector<VkFramebuffer> _framebuffers;

	inline VkFormat _depthFormat;

	
	void init_raytracing();
	

	inline MeshOLD _lineListMesh;

	inline HellRaytracer _raytracerPath;
	inline HellRaytracer _raytracerMousePick;

	inline bool _collisionEnabled = true;
	inline bool _debugScene = false;
	inline bool _renderGBuffer = false;// true;

	
			void create_rt_buffers();
			void build_rt_command_buffers(int swapchainIndex);
			inline uint32_t _rtIndexCount;



			inline AllocatedBufferOLD _rtInstancesBuffer;


	AllocatedBufferOLD create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags = 0);


	void upload_meshes();
	void upload_mesh(MeshOLD& mesh);

	void cleanup_raytracing();
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