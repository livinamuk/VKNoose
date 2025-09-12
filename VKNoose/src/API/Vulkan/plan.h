#pragma once

/*

 VulkanInstanceManager.h/.cpp
  VulkanDeviceManager.h/.cpp
  VulkanSwapchainManager.h/.cpp
  VulkanCommandManager.h/.cpp
  VulkanSynchronizationManager.h/.cpp
  VulkanMemory.h/.cpp           // VMA or thin alloc
  VulkanDescriptorManager.h/.cpp
  VulkanPipelineManager.h/.cpp  // pipeline layouts, pipelines, shaders
  VulkanBufferManager.h/.cpp
  VulkanTextureManager.h/.cpp   
  VulkanStaging.h/.cpp          // staging uploads
  VulkanFrame.h/.cpp            // per frame structs and helpers
  VulkanUtils.h/.cpp            // small helpers, checks, format picks
  

Responsibilities and init order

        InstanceManager: instance, debug messenger, surface creation helpers
        
        DeviceManager: physical pick, logical device, queues, family indices
        
        SwapchainManager: swapchain, image views, format, extent
        
        CommandManager: command pool per queue family, single time helpers, frame command buffers
        
        SynchronizationManager: fences, semaphores, timeline if used
        
        Memory: VMA allocator or manual wrappers
        
        DescriptorManager: set layouts, pools, alloc, free, update
        
        PipelineManager: shader modules, pipeline layouts, dynamic rendering pipelines
        
        BufferManager: create, map, upload, destroy
        
        TextureManager: AllocateTexture(Texture&), CleanupTexture(Texture&) as you like
        
        Staging: transient upload paths for buffers and images
        
        Frame: per frame state bundle and acquire/submit/present helpers


*/