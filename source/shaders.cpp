//
// Created by vini2003 on 13/03/2024.
//

#include "shaders.h"

#include "application.h"



VkVertexInputBindingDescription Shader::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(vox::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}
