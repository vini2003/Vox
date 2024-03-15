#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <array>

#include "util.h"

bool vox::QueueFamilyIndices::isValid() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

bool vox::SwapChainSupportDetails::isValid() {
	return !surfaceFormats.empty() && !presentModes.empty();
}

VkVertexInputBindingDescription vox::Vertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> vox::Vertex::getAttributeDescription() {
	std::array<VkVertexInputAttributeDescription, 3> attributeDescription = {};

	attributeDescription[0].binding = 0;
	attributeDescription[0].location = 0;
	attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[0].offset = offsetof(vox::Vertex, pos);

	attributeDescription[1].binding = 0;
	attributeDescription[1].location = 1;
	attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[1].offset = offsetof(vox::Vertex, color);

	attributeDescription[2].binding = 0;
	attributeDescription[2].location = 2;
	attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[2].offset = offsetof(vox::Vertex, texCoord);

	return attributeDescription;
}

bool vox::Vertex::operator==(const Vertex &other) const {
	return pos == other.pos && color == other.color && texCoord == other.texCoord;
}

VkResult vox::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vox::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
