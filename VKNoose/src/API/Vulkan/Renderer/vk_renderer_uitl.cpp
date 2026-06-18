#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_raytracing_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_swapchain_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"

#include "AssetManagement/AssetManager.h"
#include "BackEnd/GLFWIntegration.h"
#include "Hell/Core/Logging.h"

namespace VulkanRenderer {

    void BuildAllBLAS() {
        Logging::Init() << "VulkanRenderer::BuildAllBLAS()\n";

        for (Mesh& mesh : AssetManager::GetMeshes()) {
            mesh.m_vulkanAccelerationStructureId = VulkanRaytracingManager::CreateBottomLevelAS(&mesh);
        }
    }

    void BlitAllocatedImageToSwapchain(VkCommandBuffer cmd, AllocatedImage& srcImage, uint32_t swapchainIndex) {
        int32_t windowWidth = GLFWIntegration::GetCurrentWindowWidth();
        int32_t windowHeight = GLFWIntegration::GetCurrentWindowHeight();

        srcImage.Sync(cmd, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        VkImage swapchainImage = VulkanSwapchainManager::GetSwapchainImages()[swapchainIndex];

        VkImageMemoryBarrier2 swapchainBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        swapchainBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        swapchainBarrier.srcAccessMask = 0;
        swapchainBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        swapchainBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapchainBarrier.image = swapchainImage;
        swapchainBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &swapchainBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        VkImageBlit blitRegion = {};
        blitRegion.srcOffsets[1] = { srcImage.GetWidth(), srcImage.GetHeight(), 1 };
        blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.dstOffsets[1] = { windowWidth, windowHeight, 1 };
        blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

        vkCmdBlitImage(cmd, srcImage.GetImage(), VK_IMAGE_LAYOUT_GENERAL, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);
    }

    void HotloadShaders() {
        vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());

        if (!VulkanResourceManager::HotloadShaders()) {
            return;
        }

        VulkanRenderer::RecreatePipelines();
        std::cout << "Hotloaded shaders successfully\n";
    }
}
