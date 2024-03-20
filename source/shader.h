#ifndef SHADERS_H
#define SHADERS_H

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <vulkan/vulkan_core.h>

#include "util.h"
#include "vertex.h"

struct ShaderMetadataAttribute {
    std::string name;
    std::string type;
};

struct ShaderMetadataSampler {
    std::string name;
    std::string type;
};

struct ShaderMetadataUniform {
    std::string name;
    std::string type;

    [[nodiscard]] VkFormat format() const {
        return vox::GLMTypeToVkFormat(type);
    }
};

struct ShaderMetadata {
    std::string vertex;
    std::string fragment;

    std::vector<ShaderMetadataAttribute> attributes;
    std::vector<ShaderMetadataSampler> samplers;
    std::vector<ShaderMetadataUniform> uniforms;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShaderMetadataAttribute, name, type);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShaderMetadataSampler, name, type);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShaderMetadataUniform, name, type);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShaderMetadata, vertex, fragment, attributes, samplers, uniforms);

struct ShaderBoundBufferInfo {
    std::vector<VkBuffer*> buffers;
    VkDeviceSize offset;
    VkDeviceSize range;
    std::vector<VkDeviceMemory*> memories;
    std::vector<void*> mapped;
};

struct ShaderBoundImageInfo {
    VkImageView* imageView;
    VkSampler* sampler;
    VkImageLayout imageLayout;
};

template<typename V = vox::Vertex>
class Shader {
    std::string id;

public:
    Shader() = default;

    Shader(
        std::string id,
        ShaderMetadata metadata,
        std::optional<std::vector<char>> vertex_shader_code,
        std::optional<std::vector<char>> fragment_shader_code)
        : id(std::move(id)),
          metadata(std::move(metadata)),
          vertexShaderCode(std::move(vertex_shader_code)),
          fragmentShaderCode(std::move(fragment_shader_code)) {
    }

    Shader(Shader&& other) noexcept
        : id(std::move(other.id)),
          metadata(std::move(other.metadata)),
          vertexShaderCode(std::move(other.vertexShaderCode)),
          fragmentShaderCode(std::move(other.fragmentShaderCode)),
          vertexShaderModule(std::move(other.vertexShaderModule)),
          fragmentShaderModule(std::move(other.fragmentShaderModule)),
          descriptorSetLayout(std::move(other.descriptorSetLayout)),
          descriptorSets(std::move(other.descriptorSets)),
          boundBuffers(std::move(other.boundBuffers)),
          boundImages(std::move(other.boundImages)) {
        other.descriptorSetLayout.reset();
    }

    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            id = std::move(other.id);
            metadata = std::move(other.metadata);
            vertexShaderCode = std::move(other.vertexShaderCode);
            fragmentShaderCode = std::move(other.fragmentShaderCode);
            vertexShaderModule = std::move(other.vertexShaderModule);
            fragmentShaderModule = std::move(other.fragmentShaderModule);
            descriptorSetLayout = std::move(other.descriptorSetLayout);
            descriptorSets = std::move(other.descriptorSets);
            boundBuffers = std::move(other.boundBuffers);
            boundImages = std::move(other.boundImages);

            other.descriptorSetLayout.reset();
        }
        return *this;
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

private:
    ShaderMetadata metadata;

    std::vector<char> uniformBytes;
    std::map<std::string, size_t> uniformOffsets;

    std::vector<std::unique_ptr<VkBuffer>> uniformBuffers;
    std::vector<std::unique_ptr<VkDeviceMemory>> uniformBufferMemories;

    std::optional<std::vector<char>> vertexShaderCode;
    std::optional<std::vector<char>> fragmentShaderCode;

    std::optional<VkShaderModule> vertexShaderModule;
    std::optional<VkShaderModule> fragmentShaderModule;

    std::optional<VkDescriptorSetLayout> descriptorSetLayout;

    std::vector<VkDescriptorSet> descriptorSets;

    std::map<uint32_t, std::optional<ShaderBoundBufferInfo>> boundBuffers;
    std::map<uint32_t, std::optional<ShaderBoundImageInfo>> boundImages;

public:
    void initUniformBytesAndOffsets();

    VkVertexInputBindingDescription getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    void buildDescriptorSetLayout(const VkDevice& device);
    void buildDescriptorSets(const VkDevice& device, const VkDescriptorPool& descriptorPool, uint32_t amount);

    void buildBuffers(const VkDevice &device, const std::function<VkResult(vox::BufferBuildInfo)> &buildBuffer);

    void reserveBuffer();
    void reserveBuffer(uint32_t binding);
    void reserveSampler(uint32_t binding);

    void bindBuffer(uint32_t binding, const std::vector<VkBuffer*> &buffers, VkDeviceSize offset, VkDeviceSize range, const std::vector<VkDeviceMemory*> &memories, const std::vector<void *> &mapped);
    void bindSampler(uint32_t binding, VkImageView *imageView, VkSampler *sampler, VkImageLayout imageLayout);

    void destroyOwnedDescriptorSetLayot(const VkDevice &device) const;

    void destroyOwnedBuffers(const VkDevice &device) const;
    void destroyOwnedBufferMemories(const VkDevice &device) const;

    void uploadUniforms(const VkDevice &device, uint32_t currentImage);

    void setUniformFloat(const std::string& name, float value);
    void setUniformVec2(const std::string& name, const glm::vec2& value);
    void setUniformVec3(const std::string& name, const glm::vec3& value);
    void setUniformVec4(const std::string& name, const glm::vec4& value);
    void setUniformInt(const std::string& name, int value);
    void setUniformIVec2(const std::string& name, const glm::ivec2& value);
    void setUniformIVec3(const std::string& name, const glm::ivec3& value);
    void setUniformIVec4(const std::string& name, const glm::ivec4& value);
    void setUniformUInt(const std::string& name, unsigned int value);
    void setUniformUVec2(const std::string& name, const glm::uvec2& value);
    void setUniformUVec3(const std::string& name, const glm::uvec3& value);
    void setUniformUVec4(const std::string& name, const glm::uvec4& value);
    void setUniformMat2(const std::string& name, const glm::mat2& value);
    void setUniformMat3(const std::string& name, const glm::mat3& value);
    void setUniformMat4(const std::string& name, const glm::mat4& value);

    [[nodiscard]] std::string getId() const;
    [[nodiscard]] ShaderMetadata getMetadata() const;
    [[nodiscard]] std::vector<char> getUniformBytes() const;
    [[nodiscard]] std::map<std::string, size_t> getUniformOffsets() const;
    [[nodiscard]] std::optional<std::vector<char>> getVertexShaderCode() const;
    [[nodiscard]] std::optional<std::vector<char>> getFragmentShaderCode() const;
    [[nodiscard]] std::optional<VkShaderModule> getVertexShaderModule() const;
    [[nodiscard]] std::optional<VkShaderModule> getFragmentShaderModule() const;
    [[nodiscard]] std::optional<VkDescriptorSetLayout> getDescriptorSetLayout() const;
    [[nodiscard]] std::vector<VkDescriptorSet> getDescriptorSets() const;
    [[nodiscard]] std::map<uint32_t, std::optional<ShaderBoundBufferInfo>> getBoundBuffers() const;
    [[nodiscard]] std::map<uint32_t, std::optional<ShaderBoundImageInfo>> getBoundImages() const;

    void setId(const std::string &id);
    void setMetadata(const ShaderMetadata &metadata);
    void setUniformBytes(const std::vector<char> &uniformBytes);
    void setUniformOffsets(const std::map<std::string, size_t> &uniformOffsets);
    void setVertexShaderCode(const std::optional<std::vector<char>> &vertexShaderCode);
    void setFragmentShaderCode(const std::optional<std::vector<char>> &fragmentShaderCode);
    void setVertexShaderModule(const std::optional<VkShaderModule> &vertexShaderModule);
    void setFragmentShaderModule(const std::optional<VkShaderModule> &fragmentShaderModule);
    void setDescriptorSetLayout(const std::optional<VkDescriptorSetLayout> &descriptorSetLayout);
    void setDescriptorSets(const std::vector<VkDescriptorSet> &descriptorSets);
    void setBoundBuffers(const std::map<uint32_t, std::optional<ShaderBoundBufferInfo>> &boundBuffers);
    void setBoundImages(const std::map<uint32_t, std::optional<ShaderBoundImageInfo>> &boundImages);
};

template<typename V>
std::string Shader<V>::getId() const {
    return id;
}

template<typename V>
ShaderMetadata Shader<V>::getMetadata() const {
    return metadata;
}

template<typename V>
std::vector<char> Shader<V>::getUniformBytes() const {
    return uniformBytes;
}

template<typename V>
std::map<std::string, size_t> Shader<V>::getUniformOffsets() const {
    return uniformOffsets;
}

template<typename V>
std::optional<std::vector<char>> Shader<V>::getVertexShaderCode() const {
    return vertexShaderCode;
}

template<typename V>
std::optional<std::vector<char>> Shader<V>::getFragmentShaderCode() const {
    return fragmentShaderCode;
}

template<typename V>
std::optional<VkShaderModule> Shader<V>::getVertexShaderModule() const {
    return vertexShaderModule;
}

template<typename V>
std::optional<VkShaderModule> Shader<V>::getFragmentShaderModule() const {
    return fragmentShaderModule;
}

template<typename V>
std::optional<VkDescriptorSetLayout> Shader<V>::getDescriptorSetLayout() const {
    return descriptorSetLayout;
}

template<typename V>
std::vector<VkDescriptorSet> Shader<V>::getDescriptorSets() const {
    return descriptorSets;
}

template<typename V>
std::map<uint32_t, std::optional<ShaderBoundBufferInfo>> Shader<V>::getBoundBuffers() const {
    return boundBuffers;
}

template<typename V>
std::map<uint32_t, std::optional<ShaderBoundImageInfo>> Shader<V>::getBoundImages() const {
    return boundImages;
}

template<typename V>
void Shader<V>::setId(const std::string &id) {
    this->id = id;
}

template<typename V>
void Shader<V>::setMetadata(const ShaderMetadata &metadata) {
    this->metadata = metadata;
}

template<typename V>
void Shader<V>::setUniformBytes(const std::vector<char> &uniformBytes) {
    this->uniformBytes = uniformBytes;
}

template<typename V>
void Shader<V>::setUniformOffsets(const std::map<std::string, size_t> &uniformOffsets) {
    this->uniformOffsets = uniformOffsets;
}

template<typename V>
void Shader<V>::setVertexShaderCode(const std::optional<std::vector<char>> &vertexShaderCode) {
    this->vertexShaderCode = vertexShaderCode;
}

template<typename V>
void Shader<V>::setFragmentShaderCode(const std::optional<std::vector<char>> &fragmentShaderCode) {
    this->fragmentShaderCode = fragmentShaderCode;
}

template<typename V>
void Shader<V>::setVertexShaderModule(const std::optional<VkShaderModule> &vertexShaderModule) {
    this->vertexShaderModule = vertexShaderModule;
}

template<typename V>
void Shader<V>::setFragmentShaderModule(const std::optional<VkShaderModule> &fragmentShaderModule) {
    this->fragmentShaderModule = fragmentShaderModule;
}

template<typename V>
void Shader<V>::setDescriptorSetLayout(const std::optional<VkDescriptorSetLayout> &descriptorSetLayout) {
    this->descriptorSetLayout = descriptorSetLayout;
}

template<typename V>
void Shader<V>::setDescriptorSets(const std::vector<VkDescriptorSet> &descriptorSets) {
    this->descriptorSets = descriptorSets;
}

template<typename V>
void Shader<V>::setBoundBuffers(const std::map<uint32_t, std::optional<ShaderBoundBufferInfo>> &boundBuffers) {
    this->boundBuffers = boundBuffers;
}

template<typename V>
void Shader<V>::setBoundImages(const std::map<uint32_t, std::optional<ShaderBoundImageInfo>> &boundImages) {
    this->boundImages = boundImages;
}

template<typename V>
void Shader<V>::initUniformBytesAndOffsets() {
    size_t currentOffset = 0;

    for (const auto& [name, type] : metadata.uniforms) {
        if (name == "ubo") continue;

        const size_t alignment = vox::GLMTypeAlignment(type);
        currentOffset = vox::GLMTypeAlignUp(currentOffset, alignment);

        uniformOffsets[name] = currentOffset;
        currentOffset += vox::GLMTypeAlignment(type);
    }

    uniformBytes.resize(currentOffset);
}

template<typename V>
VkVertexInputBindingDescription Shader<V>::getBindingDescription() {
    return V::getBindingDescription();
}

template<typename V>
std::vector<VkVertexInputAttributeDescription> Shader<V>::getAttributeDescriptions() {
    return V::getAttributeDescriptions();
}

template<typename V>
void Shader<V>::buildDescriptorSetLayout(const VkDevice &device) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (const auto &binding: boundBuffers | std::views::keys) {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        bindings.push_back(layoutBinding);
    }

    for (const auto &binding: boundImages | std::views::keys) {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        bindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCreateInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (VK_SUCCESS != vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout)) {
        throw std::runtime_error("[Shader] Failed to create descriptor set layout for shader: " + id);
    }

    this->descriptorSetLayout = descriptorSetLayout;
}

template<typename V>
void Shader<V>::buildDescriptorSets(const VkDevice &device, const VkDescriptorPool &descriptorPool, uint32_t amount) {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = amount;

    const auto descriptorSetLayouts = std::vector(amount, descriptorSetLayout.value());

    if (this->descriptorSetLayout.has_value()) {
        allocInfo.pSetLayouts = descriptorSetLayouts.data();
    } else {
        throw std::runtime_error("[Shader] Descriptor set layout not present in shader: " + id);
    }

    descriptorSets.resize(amount);

    if (VK_SUCCESS != vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data())) {
        throw std::runtime_error("[Shader] Failed to allocate descriptor sets for shader: " + id);
    }

    for (auto i = 0; i < amount; ++i) {
        std::map<uint32_t, VkDescriptorBufferInfo> bufferInfos;

        std::vector<VkWriteDescriptorSet> descriptorWrites;

        for (const auto& [binding, buffer] : boundBuffers) {
            VkDescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = *buffer->buffers[i];
            bufferInfo.offset = buffer->offset;
            bufferInfo.range = buffer->range;

            bufferInfos[binding] = bufferInfo;

            VkWriteDescriptorSet writeDescriptorSet;
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i];
            writeDescriptorSet.dstBinding = binding;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.pBufferInfo = &bufferInfos[binding];
            writeDescriptorSet.pNext = nullptr;

            descriptorWrites.push_back(writeDescriptorSet);
        }

        for (const auto& [binding, image] : boundImages) {
            VkDescriptorImageInfo imageInfo;
            imageInfo.sampler = *image->sampler;
            imageInfo.imageView = *image->imageView;
            imageInfo.imageLayout = image->imageLayout;

            VkWriteDescriptorSet writeDescriptorSet;
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i];
            writeDescriptorSet.dstBinding = binding;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.pImageInfo = &imageInfo;
            writeDescriptorSet.pNext = nullptr;

            descriptorWrites.push_back(writeDescriptorSet);
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }
}

template<typename V>
void Shader<V>::reserveBuffer() {
    reserveBuffer(2); // "Extras" uniform block.
}

template<typename V>
void Shader<V>::buildBuffers(const VkDevice& device, const std::function<VkResult(vox::BufferBuildInfo)> &buildBuffer) {
    initUniformBytesAndOffsets();

    auto buffers = std::vector<VkBuffer*>(3);
    auto bufferMemories = std::vector<VkDeviceMemory*>(3);

    const auto buffersMapped = std::vector<void*>(3);

    for (auto i = 0; i < 3; ++i) {
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;

        vox::BufferBuildInfo buildInfo = {};

        buildInfo.deviceSize = uniformBytes.size();

        buildInfo.bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buildInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        buildInfo.buffer = &buffer;
        buildInfo.bufferMemory = &bufferMemory;

        if (VK_SUCCESS != buildBuffer(buildInfo)) {
            throw std::runtime_error("[Shader] Failed to build buffer for shader: " + id);
        }

        uniformBuffers.push_back(std::make_unique<VkBuffer>(buffer));
        uniformBufferMemories.push_back(std::make_unique<VkDeviceMemory>(bufferMemory));

        buffers[i] = uniformBuffers[i].get();
        bufferMemories[i] = uniformBufferMemories[i].get();
    }

    boundBuffers[2] = { buffers, 0, uniformBytes.size(), bufferMemories, buffersMapped };
}

template<typename V>
void Shader<V>::reserveBuffer(const uint32_t binding) {
    boundBuffers[binding] = std::nullopt;
}

template<typename V>
void Shader<V>::reserveSampler(const uint32_t binding) {
    boundImages[binding] = std::nullopt;
}

template<typename V>
void Shader<V>::bindBuffer(const uint32_t binding, const std::vector<VkBuffer*>& buffers, const VkDeviceSize offset, const VkDeviceSize range, const std::vector<VkDeviceMemory*>& memories, const std::vector<void*>& mapped) {
    boundBuffers[binding] = { buffers, offset, range, memories, mapped };
}

template<typename V>
void Shader<V>::bindSampler(const uint32_t binding, VkImageView* imageView, VkSampler* sampler, const VkImageLayout imageLayout) {
    boundImages[binding] = { imageView, sampler, imageLayout };
}

template<typename V>
void Shader<V>::destroyOwnedDescriptorSetLayot(const VkDevice &device) const {
    if (descriptorSetLayout.has_value()) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout.value(), nullptr);
    }
}

template<typename V>
void Shader<V>::destroyOwnedBuffers(const VkDevice& device) const {
    for (auto& buffer : uniformBuffers) {
        vkDestroyBuffer(device, *buffer, nullptr);
    }
}

template<typename V>
void Shader<V>::destroyOwnedBufferMemories(const VkDevice &device) const {
    for (auto& memory : uniformBufferMemories) {
        vkFreeMemory(device, *memory, nullptr);
    }
}

template<typename V>
void Shader<V>::uploadUniforms(const VkDevice& device, uint32_t currentImage) {
    void* data;
    vkMapMemory(device, *boundBuffers[2]->memories[currentImage], 0, uniformBytes.size(), 0, &data);
    memcpy(data, uniformBytes.data(), uniformBytes.size());
    vkUnmapMemory(device, *boundBuffers[2]->memories[currentImage]);
}

template<typename V>
void Shader<V>::setUniformFloat(const std::string& name, const float value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], &value, sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformVec2(const std::string& name, const glm::vec2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformVec3(const std::string& name, const glm::vec3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformVec4(const std::string& name, const glm::vec4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformInt(const std::string& name, const int value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], &value, sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformIVec2(const std::string& name, const glm::ivec2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformIVec3(const std::string& name, const glm::ivec3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformIVec4(const std::string& name, const glm::ivec4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUInt(const std::string& name, const unsigned int value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], &value, sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUVec2(const std::string& name, const glm::uvec2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUVec3(const std::string& name, const glm::uvec3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUVec4(const std::string& name, const glm::uvec4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformMat2(const std::string& name, const glm::mat2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformMat3(const std::string& name, const glm::mat3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformMat4(const std::string& name, const glm::mat4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBytes[it->second], glm::value_ptr(value), sizeof(value));
    }
}

#endif //SHADERS_H
