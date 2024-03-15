#ifndef SHADERS_H
#define SHADERS_H

#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "vertex.h"

struct BoundBufferInfo {
    std::vector<VkBuffer> buffers;
    VkDeviceSize offset;
    VkDeviceSize range;
};

struct BoundImageInfo {
    VkImageView imageView;
    VkSampler sampler;
    VkImageLayout imageLayout;
};

template<typename V = vox::Vertex>
class Shader {
    std::string id;

public:
    Shader() = default;

    Shader(
        std::string id,
        std::optional<std::vector<char>> vertex_shader_code,
        std::optional<std::vector<char>> fragment_shader_code)
        : id(std::move(id)),
          vertexShaderCode(std::move(vertex_shader_code)),
          fragmentShaderCode(std::move(fragment_shader_code)) {
    }

    Shader(Shader&& other) noexcept
        : id(std::move(other.id)),
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

    std::optional<std::vector<char>> vertexShaderCode;
    std::optional<std::vector<char>> fragmentShaderCode;

    std::optional<VkShaderModule> vertexShaderModule;
    std::optional<VkShaderModule> fragmentShaderModule;

    std::optional<VkDescriptorSetLayout> descriptorSetLayout;

    std::vector<VkDescriptorSet> descriptorSets;

    std::map<uint32_t, std::optional<BoundBufferInfo>> boundBuffers;
    std::map<uint32_t, std::optional<BoundImageInfo>> boundImages;

    void buildVertexShaderModule(std::function<VkShaderModule(const std::vector<char>&)> buildShaderModule);
    void buildFragmentShaderModule(std::function<VkShaderModule(const std::vector<char>&)> buildShaderModule);

    void destroyVertexShaderModule(const VkDevice& device) const;
    void destroyFragmentShaderModule(const VkDevice& device) const;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    [[nodiscard]] VkPipelineShaderStageCreateInfo buildVertexShaderStageCreateInfo() const;
    [[nodiscard]] VkPipelineShaderStageCreateInfo buildFragmentShaderStageCreateInfo() const;

    void buildDescriptorSetLayout(const VkDevice& device);
    void buildDescriptorSets(const VkDevice& device, const VkDescriptorPool& descriptorPool, uint32_t amount);

    void allocateBuffer(const uint32_t binding);
    void allocateImage(const uint32_t binding);

    void bindBuffer(const uint32_t binding, const std::vector<VkBuffer> buffers, const VkDeviceSize offset, const VkDeviceSize range);
    void bindImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);

    void updateDescriptorSets(const VkDevice& device);
};

template<typename V>
void Shader<V>::buildVertexShaderModule(std::function<VkShaderModule(const std::vector<char> &)> buildShaderModule) {
    if (this->vertexShaderCode.has_value()) {
        this->vertexShaderModule = buildShaderModule(this->vertexShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Vertex shader code not present! ID: " + this->id);
    }
}

template<typename V>
void Shader<V>::buildFragmentShaderModule(std::function<VkShaderModule(const std::vector<char> &)> buildShaderModule) {
    if (this->fragmentShaderCode.has_value()) {
        this->fragmentShaderModule = buildShaderModule(this->fragmentShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Fragment shader code not present! ID: " + this->id);
    }
}

template<typename V>
void Shader<V>::destroyVertexShaderModule(const VkDevice& device) const {
    if (this->vertexShaderModule.has_value()) {
        vkDestroyShaderModule(device, this->vertexShaderModule.value(), nullptr);
    }
}

template<typename V>
void Shader<V>::destroyFragmentShaderModule(const VkDevice& device) const {
    if (this->fragmentShaderModule.has_value()) {
        vkDestroyShaderModule(device, this->fragmentShaderModule.value(), nullptr);
    }
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
VkPipelineShaderStageCreateInfo Shader<V>::buildVertexShaderStageCreateInfo() const {
    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    createInfo.module = vertexShaderModule.value();
    createInfo.pName = "main";

    return createInfo;
}

template<typename V>
VkPipelineShaderStageCreateInfo Shader<V>::buildFragmentShaderStageCreateInfo() const {
    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    createInfo.module = fragmentShaderModule.value();
    createInfo.pName = "main";

    return createInfo;
}

template<typename V>
void Shader<V>::buildDescriptorSetLayout(const VkDevice &device) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (const auto& bufferInfo : boundBuffers) {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = bufferInfo.first;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;

        bindings.push_back(layoutBinding);
    }

    for (const auto& imageInfo : boundImages) {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = imageInfo.first;
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
        throw std::runtime_error("[Shader] Failed to create descriptor set layout!");
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
        throw std::runtime_error("[Shader] Descriptor set layout not present!");
    }

    descriptorSets.resize(amount);

    if (VK_SUCCESS != vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data())) {
        throw std::runtime_error("[Shader] Failed to allocate descriptor sets!");
    }

    this->updateDescriptorSets(device);
}

template<typename V>
void Shader<V>::allocateBuffer(const uint32_t binding) {
    boundBuffers[binding] = std::nullopt;
}

template<typename V>
void Shader<V>::allocateImage(const uint32_t binding) {
    boundImages[binding] = std::nullopt;
}

template<typename V>
void Shader<V>::bindBuffer(const uint32_t binding, const std::vector<VkBuffer> buffers, const VkDeviceSize offset, const VkDeviceSize range) {
    boundBuffers[binding] = { buffers, offset, range };
}

template<typename V>
void Shader<V>::bindImage(const uint32_t binding, const VkImageView imageView, const VkSampler sampler, const VkImageLayout imageLayout) {
    boundImages[binding] = { imageView, sampler, imageLayout };
}

template<typename V>
void Shader<V>::updateDescriptorSets(const VkDevice &device) {
    const auto amount = descriptorSets.size();

    for (auto i = 0; i < amount; ++i) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        for (const auto& [binding, buffer] : boundBuffers) {
            VkDescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = buffer->buffers[i];
            bufferInfo.offset = buffer->offset;
            bufferInfo.range = buffer->range;

            VkWriteDescriptorSet writeDescriptorSet;
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i]; // Assume one descriptor set for simplicity
            writeDescriptorSet.dstBinding = binding;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.pBufferInfo = &bufferInfo;

            descriptorWrites.push_back(writeDescriptorSet);
        }

        for (const auto& [binding, image] : boundImages) {
            VkDescriptorImageInfo imageInfo;
            imageInfo.sampler = image->sampler;
            imageInfo.imageView = image->imageView;
            imageInfo.imageLayout = image->imageLayout;

            VkWriteDescriptorSet writeDescriptorSet;
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i]; // Assume one descriptor set for simplicity
            writeDescriptorSet.dstBinding = binding;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.pImageInfo = &imageInfo;

            descriptorWrites.push_back(writeDescriptorSet);
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }
}

#endif //SHADERS_H
