//
// Created by vini2003 on 13/03/2024.
//

#ifndef SHADERS_H
#define SHADERS_H


#include <stdexcept>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class Shader {
public:
    // Constructor
    Shader(const std::string& id,
           VkShaderStageFlagBits stage,
           const std::vector<char>& spirvCode,
           const std::string& entryPoint = "main")
        : id(id), stage(stage), spirvCode(spirvCode), entryPoint(entryPoint), module(VK_NULL_HANDLE) {}

    // Destructor
    virtual ~Shader() {
        // Cleanup handled outside: VkShaderModule is destroyed when the pipeline is cleaned up
    }

    // Getters
    const std::string& getId() const { return id; }
    VkShaderStageFlagBits getStage() const { return stage; }
    const std::vector<char>& getSpirvCode() const { return spirvCode; }
    const std::string& getEntryPoint() const { return entryPoint; }
    VkShaderModule getModule() const { return module; }

    // Setters
    void setModule(VkShaderModule module) { this->module = module; }

    // Method to create VkShaderModule (should be called after Vulkan device is initialized)
    void createVkShaderModule(VkDevice device) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spirvCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(spirvCode.data());

        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module for " + id);
        }
    }

private:
    std::string id; // Unique identifier for the shader
    VkShaderStageFlagBits stage; // Shader stage (vertex, fragment, etc.)
    std::vector<char> spirvCode; // SPIR-V binary code
    std::string entryPoint; // Entry point for the shader ("main" by default)
    VkShaderModule module; // Vulkan shader module (created from SPIR-V code)
};



#endif //SHADERS_H
