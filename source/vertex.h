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

namespace vox {
    struct VertexAttribute {
        size_t offset;
        VkFormat format;
    };

    template<typename Derived>
    class VertexBase {
    public:
        static VkVertexInputBindingDescription getBindingDescription() {
            return {
                .binding = 0, // TODO: Update this to use the correct binding.
                .stride = sizeof(Derived),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
            const std::array attributes = Derived::attributes;

            std::vector<VkVertexInputAttributeDescription> descriptions;

            for (auto i = 0; i < attributes.size(); ++i) {
                descriptions.push_back({
                    .location = static_cast<uint32_t>(i),
                    .binding = 0, // TODO: Update this to use the correct binding.
                    .format = attributes[i].format,
                    .offset = static_cast<uint32_t>(attributes[i].offset) // TODO: Check if we need to static_cast.
                });
            }
            return descriptions;
        }
    };

    class Vertex : public VertexBase<Vertex> {
    public:
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static const std::array<VertexAttribute, 3> attributes;
    };

    inline bool operator==(const Vertex& lhs, const Vertex& rhs) {
        return lhs.pos == rhs.pos && lhs.color == rhs.color && lhs.texCoord == rhs.texCoord;
    }
}

namespace std {
    template<> struct hash<vox::Vertex> {
        size_t operator()(vox::Vertex const& vertex) const noexcept {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

#endif //VERTEX_H
