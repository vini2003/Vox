//
// Created by vini2003 on 14/03/2024.
//

#ifndef VERTEX_H
#define VERTEX_H
#include <array>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan_core.h>

template<typename T>
constexpr VkFormat toVkFormat();

template<>
constexpr VkFormat toVkFormat<float>() { return VK_FORMAT_R32_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<double>() { return VK_FORMAT_R64_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<int32_t>() { return VK_FORMAT_R32_SINT; }
template<>
constexpr VkFormat toVkFormat<uint32_t>() { return VK_FORMAT_R32_UINT; }

template<>
constexpr VkFormat toVkFormat<glm::vec2>() { return VK_FORMAT_R32G32_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::vec3>() { return VK_FORMAT_R32G32B32_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::vec4>() { return VK_FORMAT_R32G32B32A32_SFLOAT; }

template<>
constexpr VkFormat toVkFormat<glm::ivec2>() { return VK_FORMAT_R32G32_SINT; }
template<>
constexpr VkFormat toVkFormat<glm::ivec3>() { return VK_FORMAT_R32G32B32_SINT; }
template<>
constexpr VkFormat toVkFormat<glm::ivec4>() { return VK_FORMAT_R32G32B32A32_SINT; }

template<>
constexpr VkFormat toVkFormat<glm::uvec2>() { return VK_FORMAT_R32G32_UINT; }
template<>
constexpr VkFormat toVkFormat<glm::uvec3>() { return VK_FORMAT_R32G32B32_UINT; }
template<>
constexpr VkFormat toVkFormat<glm::uvec4>() { return VK_FORMAT_R32G32B32A32_UINT; }

template<>
constexpr VkFormat toVkFormat<glm::dvec2>() { return VK_FORMAT_R64G64_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::dvec3>() { return VK_FORMAT_R64G64B64_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::dvec4>() { return VK_FORMAT_R64G64B64A64_SFLOAT; }

namespace vox {
    template<size_t Offset, typename Type>
    struct VertexAttribute {
        using type = Type;
        static constexpr size_t offset = Offset;
    };

    class Vertex {
    public:
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription();

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        bool operator==(const Vertex& other) const;
    };

    template<typename V>
    struct VertexTraits;

    template<>
    struct VertexTraits<Vertex> {
        using Attributes = std::tuple<
            VertexAttribute<offsetof(Vertex, pos), glm::vec3>,
            VertexAttribute<offsetof(Vertex, color), glm::vec3>,
            VertexAttribute<offsetof(Vertex, texCoord), glm::vec3>
        >;

        static constexpr size_t count = std::tuple_size_v<Attributes>;
    };
}

namespace std {
    template<> struct hash<vox::Vertex> {
        size_t operator()(vox::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

#endif //VERTEX_H
