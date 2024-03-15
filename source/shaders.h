//
// Created by vini2003 on 13/03/2024.
//

#ifndef SHADERS_H
#define SHADERS_H
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "util.h"
#include "vertex.h"

class Application;


template<typename V = vox::Vertex>
class Shader {
    std::shared_ptr<Application> application;

    const std::string id;

public:
    explicit Shader(const std::string &id)
        : id(id) {
    }

    // std::map<int, Uniform> uniforms;

    std::optional<std::filesystem::path> vertexShaderPath;
    std::optional<std::filesystem::path> fragmentShaderPath;

    std::optional<std::vector<char>> vertexShaderCode;
    std::optional<std::vector<char>> fragmentShaderCode;

    std::optional<VkShaderModule> vertexShaderModule;
    std::optional<VkShaderModule> fragmentShaderModule;

    void buildVertexShaderModule(VkDevice device);
    void buildFragmentShaderModule(VkDevice device);

    static VkVertexInputBindingDescription getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    VkPipelineShaderStageCreateInfo buildVertexShaderStageCreateInfo();
    VkPipelineShaderStageCreateInfo buildFragmentShaderStageCreateInfo();

    void destroyVertexShaderModule(VkDevice device);
    void destroyFragmentShaderModule(VkDevice device);
};

template<typename V>
VkVertexInputBindingDescription Shader<V>::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(V);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

template<typename V, std::size_t... Is>
auto generateAttributeDescriptions(std::index_sequence<Is...>) {
    std::vector<VkVertexInputAttributeDescription> descriptions;

    auto addDescription = [&](auto Index) {
        using Attribute = typename std::tuple_element<Index, typename vox::VertexTraits<V>::Attributes>::type;
        descriptions.push_back(VkVertexInputAttributeDescription {
            .location = Index,
            .binding = 0,
            // TODO: Base this on the binding. This also means that we need to support multiple bindings.
            // TODO: And that we must save the binding number from getBindingDescription. But how do we define that?! What is it even?!
            .format = toVkFormat<typename Attribute::type>(),
            .offset = Attribute::offset
        });
    };

    (addDescription(std::integral_constant<std::size_t, Is>{}), ...);

    return descriptions;
}

template<typename V>
std::vector<VkVertexInputAttributeDescription> Shader<V>::getAttributeDescriptions() {
    return generateAttributeDescriptions<V>(std::make_index_sequence<vox::VertexTraits<V>::count>{});
}


#endif //SHADERS_H
