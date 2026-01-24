#include "vk_raytracing_manager.h"

#include "API/Vulkan/Managers/vk_command_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/vk_utils.h"
#include "API/Vulkan/vk_mesh.h"
#include "Noose/Types.h"

namespace VulkanRaytracingManager {

    VulkanBuffer CreateScratchBuffer(VkDeviceSize size) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        return VulkanBuffer(size, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    }

    void CreateASBuffer(VulkanAccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        accelerationStructure.buffer = VulkanBuffer(buildSizeInfo.accelerationStructureSize, usage, VMA_MEMORY_USAGE_AUTO);
    }

    VulkanAccelerationStructure CreateTopLevelAS(const std::vector<VkAccelerationStructureInstanceKHR>& instances) {
        if (instances.empty()) {
            return {};
        }
    
        VkDevice device = VulkanDeviceManager::GetDevice();
        const size_t bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    
        // GPU Instance Buffer - replaced manual staging with VulkanBuffer::UploadData
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        VulkanBuffer instanceBuffer(bufferSize, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        instanceBuffer.UploadData(instances.data(), bufferSize);
    
        // Geometry Setup
        VkAccelerationStructureGeometryInstancesDataKHR tlasInstances{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
        tlasInstances.data.deviceAddress = instanceBuffer.GetDeviceAddress();
    
        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.instances = tlasInstances;
    
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
    
        const uint32_t numInstances = static_cast<uint32_t>(instances.size());
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numInstances, &sizeInfo);
    
        // Create TLAS handle and buffer
        VulkanAccelerationStructure tlas{};
        CreateASBuffer(tlas, sizeInfo);
    
        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = tlas.buffer.GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &tlas.handle);
    
        // Build the TLAS using a managed scratch buffer
        VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);
        buildInfo.dstAccelerationStructure = tlas.handle;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();
    
        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ numInstances, 0, 0, 0 };
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pRangeInfos = { &rangeInfo };
    
        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRangeInfos.data());
            });
    
        // Get final address
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = tlas.handle;
        tlas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
    
        // Temporary buffers (instanceBuffer, scratchBuffer) are cleaned up by RAII here
        return tlas;
    }

    VulkanAccelerationStructure VulkanRaytracingManager::CreateBottomLevelAS(MeshOLD* mesh) {
        VkDevice device = VulkanDeviceManager::GetDevice();
    
        // Since Mesh still uses AllocatedBufferOLD, we fetch addresses manually
        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    
        // Using your VulkanUtils helper to get addresses from the raw VkBuffer handles
        vertexBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(device, mesh->m_vertexBuffer.m_buffer);
        indexBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(device, mesh->m_indexBuffer.m_buffer);
        transformBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(device, mesh->m_transformBuffer.m_buffer);
    
        // Standard geometry setup for triangles
        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        geometry.geometry.triangles.maxVertex = mesh->m_vertexCount;
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
        geometry.geometry.triangles.transformData = transformBufferDeviceAddress;
    
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
    
        const uint32_t numTriangles = mesh->m_indexCount / 3;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numTriangles, &sizeInfo);
    
        // Create the BLAS container and its internal VulkanBuffer
        VulkanAccelerationStructure blas{};
        CreateASBuffer(blas, sizeInfo);
    
        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = blas.buffer.GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &blas.handle);
    
        // Managed scratch buffer (VulkanBuffer) still used for the build process
        VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);
    
        buildInfo.dstAccelerationStructure = blas.handle;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();
    
        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ numTriangles, 0, 0, 0 };
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pRangeInfos = { &rangeInfo };
    
        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRangeInfos.data());
            });
    
        // Get final address for the BLAS handle itself
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = blas.handle;
        blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
    
        // scratchBuffer is a VulkanBuffer, so RAII handles cleanup here
        return blas;
    }

    //VulkanAccelerationStructure VulkanRaytracingManager::CreateBottomLevelAS(Mesh* mesh) {
    //    VkDevice device = VulkanDeviceManager::GetDevice();
    //
    //    uint64_t vertexBaseAddress = VulkanGeometryManager::GetVertexBuffer()->GetDeviceAddress();
    //    uint64_t indexBaseAddress = VulkanGeometryManager::GetIndexBuffer()->GetDeviceAddress();
    //    uint64_t transformBaseAddress = VulkanResourceManager::GetBuffer("Global_TransformBuffer")->GetDeviceAddress();
    //
    //    // Calculate the specific mesh addresses using byte offsets
    //    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    //    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    //    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    //
    //    vertexBufferDeviceAddress.deviceAddress = vertexBaseAddress + (mesh->GetBaseVertex() * sizeof(Vertex));
    //    indexBufferDeviceAddress.deviceAddress = indexBaseAddress + (mesh->GetBaseIndex() * sizeof(uint32_t));
    //
    //    // Point all BLAS to the same identity matrix in the global transform buffer
    //    transformBufferDeviceAddress.deviceAddress = transformBaseAddress;
    //
    //    // Geometry Setup
    //    VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    //    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    //    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    //    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    //    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    //    geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    //    geometry.geometry.triangles.maxVertex = mesh->GetVertexCount();
    //    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    //    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    //    geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    //    geometry.geometry.triangles.transformData = transformBufferDeviceAddress;
    //
    //    // Build Information
    //    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    //    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    //    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    //    buildInfo.geometryCount = 1;
    //    buildInfo.pGeometries = &geometry;
    //
    //    const uint32_t numTriangles = mesh->GetIndexCount() / 3;
    //    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    //    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numTriangles, &sizeInfo);
    //
    //    // Create BLAS
    //    VulkanAccelerationStructure blas{};
    //    CreateASBuffer(blas, sizeInfo);
    //
    //    VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    //    createInfo.buffer = blas.buffer.GetBuffer();
    //    createInfo.size = sizeInfo.accelerationStructureSize;
    //    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    //    vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &blas.handle);
    //
    //    // Scratch Buffer and Build
    //    VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);
    //    buildInfo.dstAccelerationStructure = blas.handle;
    //    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    //    buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();
    //
    //    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ numTriangles, 0, 0, 0 };
    //    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pRangeInfos = { &rangeInfo };
    //
    //    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
    //        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRangeInfos.data());
    //        });
    //
    //    // Get final AS address
    //    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    //    addressInfo.accelerationStructure = blas.handle;
    //    blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
    //
    //    return blas;
    //}
}