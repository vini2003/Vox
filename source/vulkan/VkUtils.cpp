#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <array>

#include "VkUtils.hpp"

bool VkUtils::VkQueueFamilyIndices::isValid() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

bool VkUtils::VkSwapChainSupportDetails::isValid() {
	return !vkSurfaceFormats.empty() && !vkPresentModes.empty();
}

VkVertexInputBindingDescription VkUtils::VkVertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VkUtils::VkVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> VkUtils::VkVertex::getAttributeDescription() {
	std::array<VkVertexInputAttributeDescription, 2> attributeDescription;

	attributeDescription[0].binding = 0;
	attributeDescription[0].location = 0;
	attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[0].offset = offsetof(VkUtils::VkVertex, vkPos);
	attributeDescription[1].binding = 0;
	attributeDescription[1].location = 1;
	attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[1].offset = offsetof(VkUtils::VkVertex, vkCol);

	return attributeDescription;
}

VkResult VkUtils::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VkUtils::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
