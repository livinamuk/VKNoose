#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_command_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_sync_manager.h"
#include "API/Vulkan/Managers/vk_raytracing_manager.h"
#include "API/Vulkan/Managers/vk_swapchain_manager.h"

#include "API/Vulkan/vk_backend.h"

#include "Game/Scene.h"

namespace VulkanRenderer {

    void BeginFrame() {
        uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
        VulkanSyncManager::WaitForRenderFence(frameIndex);
    }

    bool AcquireSwapchainImage(uint32_t& swapchainImageIndex) {
        uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();
        VkSemaphore presentSemaphore = VulkanSyncManager::GetPresentSemaphore(frameIndex);

        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            return false;
        }

        return true;
    }

    void EndFrame(uint32_t swapchainImageIndex, AllocatedImage& presentImage, VkFilter blitFilter) {
        uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
        VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);
        VkQueue graphicsQueue = VulkanDeviceManager::GetGraphicsQueue();
        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();
        VkSemaphore presentSemaphore = VulkanSyncManager::GetPresentSemaphore(frameIndex);
        VkSemaphore renderFinishedSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frameIndex, swapchainImageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        VulkanRenderer::BlitAllocatedImageToSwapchain(commandBuffer, presentImage, swapchainImageIndex, blitFilter);
        VulkanBackEnd::PrepareSwapchainForPresent(commandBuffer, swapchainImageIndex);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &presentSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

        VulkanSyncManager::ResetRenderFence(frameIndex);
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VulkanSyncManager::GetRenderFence(frameIndex));

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pImageIndices = &swapchainImageIndex;

        VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
        }

        VulkanRenderer::IncrementFrame();
    }

    void RenderLoadingScreen() {
        if (VulkanBackEnd::ProgramIsMinimized()) return;

        AllocatedImage* loadingTarget = VulkanResourceManager::GetAllocatedImage("LoadingScreen");
        if (!loadingTarget) return;

        VulkanBackEnd::AddDebugText();
        TextBlitter::Update(GameData::GetDeltaTime(), loadingTarget->GetWidth(), loadingTarget->GetHeight());

        BeginFrame();
        VulkanBackEnd::UpdateBuffers2D();

        uint32_t swapchainImageIndex;
        if (!AcquireSwapchainImage(swapchainImageIndex)) return;

        uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
        VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

        vkResetCommandBuffer(commandBuffer, 0);
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VulkanBackEnd::RecordAssetLoadingRenderCommands(commandBuffer);

        EndFrame(swapchainImageIndex, *loadingTarget, VK_FILTER_LINEAR);
    }

    void RenderGame() {
        if (VulkanBackEnd::ProgramIsMinimized()) return;

        AllocatedImage* presentAllocatedImage = VulkanResourceManager::GetAllocatedImage("Present");
        if (!presentAllocatedImage) return;

        TextBlitter::Update(GameData::GetDeltaTime(), presentAllocatedImage->GetWidth(), presentAllocatedImage->GetHeight());

        BeginFrame();

        VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();
        VulkanRaytracingManager::CreateTLAS(frameData.tlas.scene, Scene::GetMeshInstancesForSceneAccelerationStructure());
        VulkanRaytracingManager::CreateTLAS(frameData.tlas.inventory, Scene::GetMeshInstancesForInventoryAccelerationStructure());

        VulkanBackEnd::get_required_lines();
        VulkanBackEnd::UpdateBuffers();
        VulkanBackEnd::UpdateBuffers2D();

        VulkanRenderer::UpdateTLASDescriptorSets();

        uint32_t swapchainImageIndex;
        if (!AcquireSwapchainImage(swapchainImageIndex)) return;

        VulkanBackEnd::build_rt_command_buffers();
        EndFrame(swapchainImageIndex, *presentAllocatedImage, VK_FILTER_NEAREST);

        if (!GameData::inventoryOpen) {
            VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(frameData.buffers.mousePickBufferCPU);
            if (buffer) {
                uint32_t mousePickResult[2];
                void* mappedData;
                buffer->Map(&mappedData);
                memcpy(mousePickResult, mappedData, sizeof(uint32_t) * 2);
                Scene::StoreMousePickResult(mousePickResult[0], mousePickResult[1]);
            }
        }
        else {
            Scene::StoreMousePickResult(-1, -1);
        }
    }
}