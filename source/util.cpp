#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <array>

#include "util.h"

bool vox::QueueFamilyIndices::isValid() const {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

bool vox::SwapChainSupportDetails::isValid() const {
	return !surfaceFormats.empty() && !presentModes.empty();
}

VkFormat vox::GLMTypeToVkFormat(const std::string &type) {
	if (type == "float") return VK_FORMAT_R32_SFLOAT;
	if (type == "double") return VK_FORMAT_R64_SFLOAT;
	if (type == "int") return VK_FORMAT_R32_SINT;
	if (type == "uint") return VK_FORMAT_R32_UINT;

	if (type == "vec2") return VK_FORMAT_R32G32_SFLOAT;
	if (type == "vec3") return VK_FORMAT_R32G32B32_SFLOAT;
	if (type == "vec4") return VK_FORMAT_R32G32B32A32_SFLOAT;
	if (type == "ivec2") return VK_FORMAT_R32G32_SINT;
	if (type == "ivec3") return VK_FORMAT_R32G32B32_SINT;
	if (type == "ivec4") return VK_FORMAT_R32G32B32A32_SINT;
	if (type == "uvec2") return VK_FORMAT_R32G32_UINT;
	if (type == "uvec3") return VK_FORMAT_R32G32B32_UINT;
	if (type == "uvec4") return VK_FORMAT_R32G32B32A32_UINT;
	if (type == "dvec2") return VK_FORMAT_R64G64_SFLOAT;
	if (type == "dvec3") return VK_FORMAT_R64G64B64_SFLOAT;
	if (type == "dvec4") return VK_FORMAT_R64G64B64A64_SFLOAT;

	// Matrices are not directly supported by Vulkan.

	throw std::runtime_error("Unsupported GLM type: " + type);
}

size_t vox::GLMTypeSize(const std::string &type) {
	if (type == "float") return sizeof(float);
	if (type == "double") return sizeof(double);
	if (type == "int") return sizeof(int);
	if (type == "uint") return sizeof(uint32_t);

	if (type == "vec2") return sizeof(glm::vec2);
	if (type == "vec3") return sizeof(glm::vec3);
	if (type == "vec4") return sizeof(glm::vec4);
	if (type == "ivec2") return sizeof(glm::ivec2);
	if (type == "ivec3") return sizeof(glm::ivec3);
	if (type == "ivec4") return sizeof(glm::ivec4);
	if (type == "uvec2") return sizeof(glm::uvec2);
	if (type == "uvec3") return sizeof(glm::uvec3);
	if (type == "uvec4") return sizeof(glm::uvec4);
	if (type == "dvec2") return sizeof(glm::dvec2);
	if (type == "dvec3") return sizeof(glm::dvec3);
	if (type == "dvec4") return sizeof(glm::dvec4);

	if (type == "mat2") return sizeof(glm::mat2);
	if (type == "mat3") return sizeof(glm::mat3);
	if (type == "mat4") return sizeof(glm::mat4);
	if (type == "dmat2") return sizeof(glm::dmat2);
	if (type == "dmat3") return sizeof(glm::dmat3);
	if (type == "dmat4") return sizeof(glm::dmat4);

	if (type == "ubo") return sizeof(UniformBufferObject);

	throw std::runtime_error("Unsupported GLM type: " + type);
}

size_t vox::GLMTypeAlignment(const std::string &type) {
	if (true) {
		return 16; // Screw it. TODO: Remove.
	}

	if (type == "float" || type == "int" || type == "uint") return 4;
	if (type == "vec2" || type == "ivec2" || type == "uvec2" || type == "dvec2") return 8;
	if (type == "vec3" || type == "vec4" || type == "ivec3" || type == "ivec4" ||
	    type == "uvec3" || type == "uvec4" || type == "dvec3" || type == "dvec4") return 16;

	if (type == "mat2" || type == "mat3" || type == "mat4" ||
	    type == "dmat2" || type == "dmat3" || type == "dmat4") return 16;

	throw std::runtime_error("Unsupported GLM type for alignment: " + type);
}

size_t vox::GLMTypeAlignUp(const size_t size, const size_t alignment) {
	if (true) {
		return 16; // Screw it. TODO: Remove.
	}
	
	return (size + alignment - 1) & ~(alignment - 1);
}

std::vector<char> vox::createBinaryBlob(std::initializer_list<std::pair<std::string, const void *>> args) {
	std::vector<char> blob;
	size_t currentOffset = 0;

	for (const auto& [type, data] : args) {
		const auto typeSize = GLMTypeSize(type);
		const auto typeAlignment = GLMTypeAlignment(type);

		currentOffset = GLMTypeAlignUp(currentOffset, typeAlignment);

		blob.resize(currentOffset + typeSize);

		memcpy(&blob[currentOffset], data, typeSize);

		currentOffset += typeSize;
	}

	return blob;
}

VkResult vox::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vox::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
