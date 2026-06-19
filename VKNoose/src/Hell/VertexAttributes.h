#pragma once

#include "glm/gtx/hash.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

enum class VertexAttributeType {
    Float,
    Int,
    UnsignedInt
};

struct VertexAttribute {
    uint32_t location = 0;
    int32_t componentCount = 0;
    VertexAttributeType type = VertexAttributeType::Float;
    bool normalized = false;
    size_t offset = 0;
};

struct VertexLayoutDescription {
    size_t stride = 0;
    std::span<const VertexAttribute> attributes;
};

struct Vertex2D {
    glm::vec2 position;
    glm::vec2 uv = glm::vec2(0);
    glm::vec4 color = glm::vec4(1);
    int32_t textureIndex = 0;

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 4> attributes = {
            VertexAttribute { 0, 2, VertexAttributeType::Float, false, offsetof(Vertex2D, position) },
            VertexAttribute { 1, 2, VertexAttributeType::Float, false, offsetof(Vertex2D, uv) },
            VertexAttribute { 2, 4, VertexAttributeType::Float, false, offsetof(Vertex2D, color) },
            VertexAttribute { 3, 1, VertexAttributeType::Int, false, offsetof(Vertex2D, textureIndex) }
        };

        return { sizeof(Vertex2D), attributes };
    }
};

struct VertexDebug {
    glm::vec3 position = glm::vec3(0);

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 1> attributes = {
            VertexAttribute { 0, 3, VertexAttributeType::Float, false, offsetof(VertexDebug, position) }
        };

        return { sizeof(VertexDebug), attributes };
    }
};

struct Vertex {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
    glm::vec3 tangent = glm::vec3(0);

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 4> attributes = {
            VertexAttribute { 0, 3, VertexAttributeType::Float, false, offsetof(Vertex, position) },
            VertexAttribute { 1, 3, VertexAttributeType::Float, false, offsetof(Vertex, normal) },
            VertexAttribute { 2, 2, VertexAttributeType::Float, false, offsetof(Vertex, uv) },
            VertexAttribute { 3, 3, VertexAttributeType::Float, false, offsetof(Vertex, tangent) }
        };

        return { sizeof(Vertex), attributes };
    }

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && uv == other.uv;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}