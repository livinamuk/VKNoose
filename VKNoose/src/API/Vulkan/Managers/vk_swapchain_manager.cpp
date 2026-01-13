#include "vk_swapchain_manager.h"
#include "vk_device_manager.h"
#include "vk_instance_manager.h"
#include "BackEnd/GLFWIntegration.h"

#include "VkBootstrap.h"

#include <iostream>

namespace VulkanSwapchainManager {
	VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
	VkFormat g_swachainImageFormat;
	std::vector<VkImage> g_swapchainImages;
	std::vector<VkImageView> g_swapchainImageViews;

	bool Init() {
		CreateSwapchain();
		std::cout << "VulkanSwapchainManager::Init()\n";
		return true;
	}

	void RecreateSwapchain() {
		GLFWIntegration::WaitUntilNotMinimized();
		vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());

		for (int i = 0; i < g_swapchainImages.size(); i++) {
			vkDestroyImageView(VulkanDeviceManager::GetDevice(), g_swapchainImageViews[i], nullptr);
		}

		if (g_swapchain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(VulkanDeviceManager::GetDevice(), g_swapchain, nullptr);
		}

		CreateSwapchain();
		std::cout << "VulkanSwapchainManager::RecreateSwapchain()\n";
	}

	void CreateSwapchain() {
		uint32_t width = GLFWIntegration::GetCurrentWindowWidth();
		uint32_t height = GLFWIntegration::GetCurrentWindowHeight();

		VkSurfaceFormatKHR format;
		format.colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		format.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;

		vkb::SwapchainBuilder swapchainBuilder(VulkanDeviceManager::GetPhysicalDevice(), VulkanDeviceManager::GetDevice(), VulkanInstanceManager::GetSurface());
		swapchainBuilder.set_desired_format(format);
		swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
		swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
		swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
		swapchainBuilder.set_desired_extent(width, height);
		swapchainBuilder.set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		vkb::Swapchain vkbSwapchain = swapchainBuilder.build().value();
		g_swapchain = vkbSwapchain.swapchain;
		g_swapchainImages = vkbSwapchain.get_images().value();
		g_swapchainImageViews = vkbSwapchain.get_image_views().value();
		g_swachainImageFormat = vkbSwapchain.image_format;
	}

	void CleanUp() {

	}

	VkSwapchainKHR GetSwapchain() { return g_swapchain; }
	VkFormat GetSwapchainImageFormat() { return g_swachainImageFormat; }
	std::vector<VkImage>& GetSwapchainImages() { return g_swapchainImages; }
	std::vector<VkImageView>& GetSwapchainImageViews() { return g_swapchainImageViews; }
}