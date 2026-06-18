#include "vk_renderer.h"
#include "API/Vulkan/Managers/vk_device_manager.h" // Refactor so I'm not needed here
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"

#include "AssetManagement/AssetManager.h" // Maybe clean me out of here too?

namespace VulkanRenderer {

    void UpdateBindlessTexturesDescriptorSets() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");

        if (!staticDescriptorSet) return;
        if (!linearSampler) return;

        // Samplers
        staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER);

        // // Textures
        for (uint32_t i = 0; i < AssetManager::GetNumberOfTextures(); ++i) {
            VkImageView view = AssetManager::GetTextureByIndexOLD(i)->imageView;
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
        }

        // Finalize all writes
        staticDescriptorSet->Update();
    }

    void UpdateRenderTargetsDescriptorSets() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!staticDescriptorSet) return;

        AllocatedImage* rtFirstHitColor = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Color");
        AllocatedImage* rtFirstHitNormals = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_Normals");
        AllocatedImage* rtFirstHitBaseColor = VulkanResourceManager::GetAllocatedImage("RT_FirstHit_BaseColor");
        AllocatedImage* rtSecondHitColor = VulkanResourceManager::GetAllocatedImage("RT_SecondHit_Color");
        AllocatedImage* gBufferBaseColor = VulkanResourceManager::GetAllocatedImage("GBuffer_BaseColor");
        AllocatedImage* gBufferNormal = VulkanResourceManager::GetAllocatedImage("GBuffer_Normal");
        AllocatedImage* gBufferRma = VulkanResourceManager::GetAllocatedImage("GBuffer_RMA");
        AllocatedImage* laptopDisplay = VulkanResourceManager::GetAllocatedImage("LaptopDisplay");
        AllocatedImage* composite = VulkanResourceManager::GetAllocatedImage("Composite");
        AllocatedImage* present = VulkanResourceManager::GetAllocatedImage("Present");
        AllocatedImage* loadingScreen = VulkanResourceManager::GetAllocatedImage("LoadingScreen");
        AllocatedImage* depthPresent = VulkanResourceManager::GetAllocatedImage("Depth_Present");
        AllocatedImage* depthGBuffer = VulkanResourceManager::GetAllocatedImage("Depth_GBuffer");

        // Render Targets
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, rtFirstHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_FIRST_HIT_COLOR);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, rtFirstHitNormals->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_FIRST_HIT_NORMALS);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, rtFirstHitBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_FIRST_HIT_BASE);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, rtSecondHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_SECOND_HIT_COLOR);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, gBufferBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_GBUFFER_BASE);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, gBufferNormal->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_GBUFFER_NORMAL);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, gBufferRma->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_GBUFFER_RMA);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, laptopDisplay->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_LAPTOP);
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, composite->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_COMPOSITE);

        // Depth targets use specific depth layout
        staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, depthGBuffer->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, RT_IDX_DEPTH_GBUFFER);

        // Storage Images RGBA32F
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtFirstHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_FIRST_COLOR);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtFirstHitNormals->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_FIRST_NORMALS);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtFirstHitBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_FIRST_BASE);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA32F, rtSecondHitColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_RT_SECOND_COLOR);

        // Storage Images RGBA16F

        // Storage Images RGBA8
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, gBufferBaseColor->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_GBUFFER_BASE);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, gBufferNormal->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_GBUFFER_NORMAL);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, gBufferRma->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_GBUFFER_RMA);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, laptopDisplay->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_LAPTOP);
        staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA8, composite->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMG_IDX_COMPOSITE);

        // Finalize all writes
        staticDescriptorSet->Update();
    }

    void UpdateTLASDescriptorSets() {
        VulkanDescriptorSet* sceneSet = VulkanResourceManager::GetDescriptorSet("SceneTLASDescriptorSet");
        VulkanDescriptorSet* inventorySet = VulkanResourceManager::GetDescriptorSet("InventoryTLASDescriptorSet");

        if (!sceneSet) return;
        if (!inventorySet) return;

        uint32_t frameIndex = VulkanRenderer::GetCurrentFrameIndex();
        VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();

        VulkanAccelerationStructure* sceneTLAS = VulkanResourceManager::GetAccelerationStructure(frameData.tlas.scene);
        sceneSet->WriteAccelerationStructure(DESC_IDX_TLAS, sceneTLAS->GetHandle());
        sceneSet->Update();

        VulkanAccelerationStructure* inventoryTLAS = VulkanResourceManager::GetAccelerationStructure(frameData.tlas.inventory);
        inventorySet->WriteAccelerationStructure(DESC_IDX_TLAS, inventoryTLAS->GetHandle());
        inventorySet->Update();
    }
}