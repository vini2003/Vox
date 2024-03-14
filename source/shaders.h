//
// Created by vini2003 on 13/03/2024.
//

#ifndef SHADERS_H
#define SHADERS_H
#include <string>
#include <vulkan/vulkan_core.h>

class Shader {
private:
    const std::string id;
    const std::string entrypoint;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
    // Which others do I need?
public:
    std::string getId() const {
        return id;
    }

    std::string getEntrypoint() const {
        return entrypoint;
    }

    VkDescriptorSetLayoutCreateInfo getDescriptorSetLayoutCreateInfo() const {
        return descriptorSetLayoutCreateInfo;
    }

    VkDescriptorSetLayout getDescriptorSetLayout() const {
        return descriptorSetLayout;
    }

    VkDescriptorSet getDescriptorSet() const {
        return descriptorSet;
    }

    VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo() const {
        return pipelineShaderStageCreateInfo;
    }
};


#endif //SHADERS_H
