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
#include <nlohmann/json.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    std::vector<VkBuffer> buffers;
    VkDeviceSize offset;
    VkDeviceSize range;
    std::vector<VkDeviceMemory> memories;
    std::vector<void*> mapped;
};

struct ShaderBoundImageInfo {
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

    ShaderMetadata metadata;

    std::vector<char> uniformBlob;
    std::map<std::string, size_t> uniformOffsets;

    std::optional<std::vector<char>> vertexShaderCode;
    std::optional<std::vector<char>> fragmentShaderCode;

    std::optional<VkShaderModule> vertexShaderModule;
    std::optional<VkShaderModule> fragmentShaderModule;

    std::optional<VkDescriptorSetLayout> descriptorSetLayout;

    std::vector<VkDescriptorSet> descriptorSets;

    std::map<uint32_t, std::optional<ShaderBoundBufferInfo>> boundBuffers;
    std::map<uint32_t, std::optional<ShaderBoundImageInfo>> boundImages;

    void initializeUniformBlobAndOffsets();

    void buildVertexShaderModule(const std::function<VkShaderModule(const std::vector<char>&)>& buildShaderModule);
    void buildFragmentShaderModule(const std::function<VkShaderModule(const std::vector<char>&)>& buildShaderModule);

    void destroyVertexShaderModule(const VkDevice& device) const;
    void destroyFragmentShaderModule(const VkDevice& device) const;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    [[nodiscard]] VkPipelineShaderStageCreateInfo buildVertexShaderStageCreateInfo() const;
    [[nodiscard]] VkPipelineShaderStageCreateInfo buildFragmentShaderStageCreateInfo() const;

    void buildDescriptorSetLayout(const VkDevice& device);
    void buildDescriptorSets(const VkDevice& device, const VkDescriptorPool& descriptorPool, uint32_t amount);

    void allocateBuffers();

    // VkResult Application::buildBuffer(vox::BufferBuildInfo buildInfo)
    void buildBuffers(const VkDevice &device, const std::function<VkResult(vox::BufferBuildInfo)> &buildBuffer);

    void allocateBuffer(uint32_t binding);
    void allocateSampler(uint32_t binding);

    void bindBuffer(const uint32_t binding, const std::vector<VkBuffer> buffers, const VkDeviceSize offset, const VkDeviceSize range, std::
                    vector<VkDeviceMemory> memories, std::vector<void *> mapped);
    void bindSampler(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);

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
};

template<typename V>
void Shader<V>::initializeUniformBlobAndOffsets() {
    size_t currentOffset = 0;

    for (const auto& [name, type] : metadata.uniforms) {
        if (name == "ubo") continue;

        const size_t alignment = vox::GLMTypeAlignment(type);
        currentOffset = vox::GLMTypeAlignUp(currentOffset, alignment);

        uniformOffsets[name] = currentOffset;
        currentOffset += vox::GLMTypeAlignment(type);
    }

    uniformBlob.resize(currentOffset);
}

template<typename V>
void Shader<V>::buildVertexShaderModule(const std::function<VkShaderModule(const std::vector<char> &)>& buildShaderModule) {
    if (this->vertexShaderCode.has_value()) {
        this->vertexShaderModule = buildShaderModule(this->vertexShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Vertex shader code not present! ID: " + this->id);
    }
}

template<typename V>
void Shader<V>::buildFragmentShaderModule(const std::function<VkShaderModule(const std::vector<char> &)>& buildShaderModule) {
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

    for (auto i = 0; i < amount; ++i) {
        std::map<uint32_t, VkDescriptorBufferInfo> bufferInfos;

        std::vector<VkWriteDescriptorSet> descriptorWrites;

        for (const auto& [binding, buffer] : boundBuffers) {
            VkDescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = buffer->buffers[i];
            bufferInfo.offset = buffer->offset;
            bufferInfo.range = buffer->range;

            bufferInfos[binding] = bufferInfo;

            VkWriteDescriptorSet writeDescriptorSet;
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i]; // Assume one descriptor set for simplicity
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
            writeDescriptorSet.pNext = nullptr;

            descriptorWrites.push_back(writeDescriptorSet);
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }
}

template<typename V>
void Shader<V>::allocateBuffers() {
    allocateBuffer(2); // Extra uniform block.
}

template<typename V>
void Shader<V>::buildBuffers(const VkDevice& device, const std::function<VkResult(vox::BufferBuildInfo)> &buildBuffer) {
    initializeUniformBlobAndOffsets();

    // Iterate through metadata.buffers, remove "model", "proj" and "view" as they're already contained in the UBO.
    // Then, find the other ones and create buffers for them.

    vox::BufferBuildInfo buildInfo = {};

    // Device size is the size of the binary blob.
    // TODO: Check if this size calculation works. It should, it's in bytes.
    buildInfo.deviceSize = uniformBlob.size();

    buildInfo.bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buildInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    allocateBuffer(2);

    auto buffers = std::vector<VkBuffer>(3);
    auto bufferMemories = std::vector<VkDeviceMemory>(3);
    auto buffersMapped = std::vector<void*>(3);

    for (auto i = 0; i < 3; ++i) {
        // TODO: Check if we can reuse buildInfo.
        buildInfo.buffer = &buffers[i];
        buildInfo.bufferMemory = &bufferMemories[i];

        if (VK_SUCCESS != buildBuffer(buildInfo)) {
            throw std::runtime_error("[Shader] Failed to build buffer!");
        }

        // TODO: Check if this size calculation works. It should, it's in bytes.
        boundBuffers[2] = { buffers, 0, uniformBlob.size(), bufferMemories, buffersMapped };
    }
}

/**
 * So, for tomorrow. As it stands, I have created a system that allows me to dynamically create
 * buffers for uniforms. But I thought we could have uniforms like in OpenGL, with a single type.
 * Turns out in Vulkan you need them to be in a block, like in UniformBufferObject.
 * The problem is, this requires me to know the size of the elements in C++ and correctly
 * bind them to the block. What needs to be done is to create a new block for the extra data
 * that contains all of the extra uniforms (sort of "ubo" + "uniforms" in GLSL), and update that.
 * That'll require me to calculate the size and offsets of the data programatically in C++,
 * assembling a "struct" of sorts... but not real. I need a "virtual struct" - an aligned
 * bit of memory that contains the data, aligned uniformly, which can be updated independently.
 * This may be an use for offsets? For example, if I had a glm::vec4 | glm::vec2 | glm::vec4 in
 * memory, then I know that the size is sizeof(glm::vec4) + sizeof(glm::vec2) + sizeof(glm::vec4),
 * and the data could be updated by assembling a binary blob and updating the memory regions accordingly.
 * I would also need to have them aligned, perhaps using alignas or calculating the alignment based on size.
 * Aftewards I could then upload the data at once, despite not having the actual struct for calculations.
 * And in the shader there would be no difference, since I'd be receiving the same binary blob as
 * an aligned struct.
 *
 * UPDATE - I have an idea. I've created the binary blob and the required functions.
 * What must now be done is to create a single new buffer for the extra uniform, which contains
 * all of the data. Also, the descriptor layout and such must be updated to reflect the binary blob.
 * Given we know the types of the uniforms, we can calculate the size and offsets of the data
 * when the shader is initialized and use this for the calculations.
 *
 * UPDATE - It mostly works. Mostly - the upload doesn't work, so multiplying by colorModulation in the shader doesn't
 * result in any color. However, removing that multiplication makes it work.
 * Therefore, the current issue is likely something to do with the data's formatting for the upload?
 */
template<typename V>
void Shader<V>::allocateBuffer(const uint32_t binding) {
    boundBuffers[binding] = std::nullopt;
}

template<typename V>
void Shader<V>::allocateSampler(const uint32_t binding) {
    boundImages[binding] = std::nullopt;
}

template<typename V>
void Shader<V>::bindBuffer(const uint32_t binding, const std::vector<VkBuffer> buffers, const VkDeviceSize offset, const VkDeviceSize range, std::vector<VkDeviceMemory> memories, std::vector<void*> mapped) {
    boundBuffers[binding] = { buffers, offset, range, memories, mapped };
}

template<typename V>
void Shader<V>::bindSampler(const uint32_t binding, const VkImageView imageView, const VkSampler sampler, const VkImageLayout imageLayout) {
    boundImages[binding] = { imageView, sampler, imageLayout };
}

template<typename V>
void Shader<V>::uploadUniforms(const VkDevice& device, uint32_t currentImage) {
    void* data;
    vkMapMemory(device, boundBuffers[2]->memories[currentImage], 0, uniformBlob.size(), 0, &data);
    memcpy(data, uniformBlob.data(), uniformBlob.size());
    vkUnmapMemory(device, boundBuffers[2]->memories[currentImage]);

    struct Extras {
        alignas(16) glm::vec4 colorModulation;
        alignas(16) float decay;
    };

    auto extras = Extras { glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f };

    // Helper function to print the bytes
    auto printBytes = [](const void* ptr, std::size_t size) {
        const unsigned char* p = static_cast<const unsigned char*>(ptr);
        for (std::size_t i = 0; i < size; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)p[i];
            if (i < size - 1) std::cout << " ";
        }
        std::cout << std::dec << std::endl; // Switch back to decimal for any further output
    };

    // Print the content of extras as bytes
    std::cout << "Extras bytes: ";
    printBytes(&extras, sizeof(extras));

    // Print the content of uniformBlob as bytes
    std::cout << "uniformBlob bytes: ";
    printBytes(uniformBlob.data(), uniformBlob.size());

    int a = 1;
}

template<typename V>
void Shader<V>::setUniformFloat(const std::string& name, const float value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], &value, sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformVec2(const std::string& name, const glm::vec2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformVec3(const std::string& name, const glm::vec3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformVec4(const std::string& name, const glm::vec4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformInt(const std::string& name, const int value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], &value, sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformIVec2(const std::string& name, const glm::ivec2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformIVec3(const std::string& name, const glm::ivec3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformIVec4(const std::string& name, const glm::ivec4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUInt(const std::string& name, const unsigned int value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], &value, sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUVec2(const std::string& name, const glm::uvec2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUVec3(const std::string& name, const glm::uvec3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformUVec4(const std::string& name, const glm::uvec4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformMat2(const std::string& name, const glm::mat2& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformMat3(const std::string& name, const glm::mat3& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

template<typename V>
void Shader<V>::setUniformMat4(const std::string& name, const glm::mat4& value) {
    if (const auto it = uniformOffsets.find(name); it != uniformOffsets.end()) {
        memcpy(&uniformBlob[it->second], glm::value_ptr(value), sizeof(value));
    }
}

#endif //SHADERS_H
