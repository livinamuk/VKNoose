#pragma once
#include "API/Vulkan/vk_common.h"
#include "Noose/Types.h"
#include <vector>

#include "Common.h"

template<typename T>
struct VulkanVertexDescription;

template<>
struct VulkanVertexDescription<Vertex> {
    static VkVertexInputBindingDescription GetBinding() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributes() {
        std::vector<VkVertexInputAttributeDescription> attributes(4);

        // pos 
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = offsetof(Vertex, position);

        // normal 
        attributes[1].binding = 0;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[1].offset = offsetof(Vertex, normal);

        // uv
        attributes[2].binding = 0;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[2].offset = offsetof(Vertex, uv);

        // tangent 
        attributes[3].binding = 0;
        attributes[3].location = 3;
        attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[3].offset = offsetof(Vertex, tangent);

        return attributes;
    }
};

template<>
struct VulkanVertexDescription<Vertex2D> {
    static VkVertexInputBindingDescription GetBinding() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        //binding.stride = sizeof(Vertex2D);
        binding.stride = sizeof(Vertex); // The original large stride
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributes() {
        std::vector<VkVertexInputAttributeDescription> attributes(2);

        // pos 
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = offsetof(Vertex2D, position);

        // uv
        attributes[1].binding = 0;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[1].offset = offsetof(Vertex2D, uv);

        return attributes;
    }
};

template<>
struct VulkanVertexDescription<VertexDebug> {
    static VkVertexInputBindingDescription GetBinding() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        //binding.stride = sizeof(VertexDebug);
        binding.stride = sizeof(Vertex); // The original large stride
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributes() {
        VkVertexInputAttributeDescription attr{};
        attr.binding = 0;
        attr.location = 0;
        attr.format = VK_FORMAT_R32G32B32_SFLOAT;
        attr.offset = offsetof(VertexDebug, position);
        return { attr };
    }
};