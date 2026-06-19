#pragma once
#include "vk_common.h"

#include "vk_types.h"
#include "vk_mesh.h"

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
#include "UI/TextBlitter.h"

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
	void ToggleFullscreen();
	bool ProgramIsMinimized();
	void LoadNextItem();
    void AddLoadingText(const std::string& text);
}


namespace VulkanBackEnd {


	inline bool _loaded = false;

	

	void RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer);
	void PrepareSwapchainForPresent(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);


	void UpdateBuffers2D();


	GLFWwindow* GetWindow();

	inline bool _forceCloseWindow { false };


	void UpdateBuffers();
	
	// Commands
	void cmd_SetViewportSize(VkCommandBuffer commandBuffer, int width, int height);
	void cmd_BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);

	inline uint32_t _frameIndex;

	inline VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR{};

		 
	inline const VkExtent2D _windowedModeExtent{ 512 * 4, 288 * 4 };
	inline const VkExtent3D _renderTargetPresentExtent = { 512 , 288  , 1 };
	


	inline DebugMode _debugMode = DebugMode::NONE;


	


	inline std::vector<VkFramebuffer> _framebuffers;

	inline VkFormat _depthFormat;

	
	inline MeshOLD _lineListMesh;

	inline bool _collisionEnabled = true;
	inline bool _debugScene = false;
	inline bool _renderGBuffer = false;// true;

	
			void build_rt_command_buffers();
			inline uint32_t _rtIndexCount;



			inline AllocatedBufferOLD _rtInstancesBuffer;


	AllocatedBufferOLD create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags = 0);


	void upload_meshes();
	void upload_mesh(MeshOLD& mesh);

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
