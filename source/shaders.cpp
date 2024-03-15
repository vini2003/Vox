//
// Created by vini2003 on 13/03/2024.
//

#include "shaders.h"

#include "application.h"

void Shader::buildVertexShaderModule(VkDevice device) {
    if (this->vertexShaderCode.has_value()) {
        this->vertexShaderModule = application->buildShaderModule(this->vertexShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Vertex shader code not present! ID: " + this->id);
    }
}

void Shader::buildFragmentShaderModule(VkDevice device) {
    if (this->fragmentShaderCode.has_value()) {
        this->fragmentShaderModule = application->buildShaderModule(this->fragmentShaderCode.value());
    } else {
        throw std::runtime_error("[Shader] Fragment shader code not present! ID: " + this->id);
    }
}

VkVertexInputBindingDescription Shader::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(vox::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}
