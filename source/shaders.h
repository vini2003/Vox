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

struct ShaderDescriptorInfo {
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
};

struct BoundBufferInfo {
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
};

struct BoundImageInfo {
    VkImageView imageView;
    VkSampler sampler;
    VkImageLayout imageLayout;
};

template<typename V = vox::VertexBase>
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

    std::vector<ShaderDescriptorInfo> descriptorInfos;

    std::vector<VkDescriptorSet> descriptorSets;

    std::map<uint32_t, BoundBufferInfo> boundBuffers;
    std::map<uint32_t, BoundImageInfo> boundImages;

    void buildVertexShaderModule(VkDevice device);
    void buildFragmentShaderModule(VkDevice device);

    void destroyVertexShaderModule(VkDevice device);
    void destroyFragmentShaderModule(VkDevice device);

    static VkVertexInputBindingDescription getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    VkPipelineShaderStageCreateInfo buildVertexShaderStageCreateInfo();
    VkPipelineShaderStageCreateInfo buildFragmentShaderStageCreateInfo();

    VkDescriptorSetLayout buildDescriptorSetLayout(VkDevice device);

    void bindBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
    void bindImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);

    void updateDescriptorSets(VkDevice device);
};

template<typename V>
void Shader<V>::buildVertexShaderModule(VkDevice device) {
    if (this->vertexShaderCode.has_value()) {
        this->vertexShaderModule = application->buildShaderModule(this->vertexShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Vertex shader code not present! ID: " + this->id);
    }
}

template<typename V>
void Shader<V>::buildFragmentShaderModule(VkDevice device) {
    if (this->fragmentShaderCode.has_value()) {
        this->fragmentShaderModule = application->buildShaderModule(this->fragmentShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Fragment shader code not present! ID: " + this->id);
    }
}

template<typename V>
void Shader<V>::destroyVertexShaderModule(VkDevice device) {
    if (this->vertexShaderModule.has_value()) {
        vkDestroyShaderModule(device, this->vertexShaderModule.value(), nullptr);
    }
}

template<typename V>
void Shader<V>::destroyFragmentShaderModule(VkDevice device) {
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
VkPipelineShaderStageCreateInfo Shader<V>::buildVertexShaderStageCreateInfo() {
    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    createInfo.module = vertexShaderModule.value();
    createInfo.pName = "main";

    return createInfo;
}

template<typename V>
VkPipelineShaderStageCreateInfo Shader<V>::buildFragmentShaderStageCreateInfo() {
    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    createInfo.module = fragmentShaderModule.value();
    createInfo.pName = "main";

    return createInfo;
}

template<typename V>
VkDescriptorSetLayout Shader<V>::buildDescriptorSetLayout(VkDevice device) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (const auto& descriptorInfo : this->descriptorInfos) {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = descriptorInfo.binding;
        layoutBinding.descriptorType = descriptorInfo.descriptorType;
        layoutBinding.descriptorCount = 1; // TODO: Check when we'd want more?!
        layoutBinding.stageFlags = descriptorInfo.stageFlags;
        layoutBinding.pImmutableSamplers = nullptr; // Optional

        bindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCreateInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("[Shader] Failed to create descriptor set layout!");
    }

    return descriptorSetLayout;
}


template<typename V>
void Shader<V>::bindBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
    boundBuffers[binding] = {buffer, offset, range};
}

template<typename V>
void Shader<V>::bindImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout) {
    boundImages[binding] = {imageView, sampler, imageLayout};
}

/**
 * Okay, for tomorrow, here's what I'm trying to do right now at 00:04AM.
 * I'm writing a shader class which holds all the necessary information
 * to instantiate and manage a shader. This shader class should allow
 * me to load JSONs that specify the shader paths, uniforms, resources (buffers, samplers).
 *
 * The vertex format is defined by the VertexBase class, which is templated
 * such that classes that extend it have Attributes, and these Attributes
 * are used to generate the bindings. These bindings are used to create a layout.
 *
 * I am currently trying to create a way to bind buffers and samplers
 * to the uniforms defined above. The layout contains the uniform and sampler types,
 * but this is where we need to set their values. In other words, this method
 * updates the values defined by the layout to be used in the shader.
 *
 * The current challenge is figuring out if the two info classes that exist
 * right now are enough to represent all possible resources we need to bind.
 *
 * If they are, then all we need is bindBuffer and bindImage.
 *
 * In the method below, we need to first allocate descriptorSets that
 * we can then populate using the descriptor writes. For each frame in flight,
 * we need to allocate a descriptor set, so it must be a vector.
 *
 * TODO, then, is to figure out how to allocate these descriptor sets, then populate them.
 */
template<typename V>
void Shader<V>::updateDescriptorSets(VkDevice device) {
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    for (const auto& [binding, buffer] : boundBuffers) {
        VkDescriptorBufferInfo bufferInfo = {buffer.buffer, buffer.offset, buffer.range};
        descriptorWrites.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[0], // Assume one descriptor set for simplicity
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo,
        });
    }

    for (const auto& [binding, image] : boundImages) {
        VkDescriptorImageInfo imageInfo = {image.sampler, image.imageView, image.imageLayout};
        descriptorWrites.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[0], // Assume one descriptor set for simplicity
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &imageInfo,
        });
    }

    if (!descriptorWrites.empty()) {
        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}


#endif //SHADERS_H
