#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

template<typename T>
constexpr VkFormat toVkFormat();

template<>
constexpr VkFormat toVkFormat<float>() { return VK_FORMAT_R32_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<double>() { return VK_FORMAT_R64_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<int32_t>() { return VK_FORMAT_R32_SINT; }
template<>
constexpr VkFormat toVkFormat<uint32_t>() { return VK_FORMAT_R32_UINT; }

template<>
constexpr VkFormat toVkFormat<glm::vec2>() { return VK_FORMAT_R32G32_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::vec3>() { return VK_FORMAT_R32G32B32_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::vec4>() { return VK_FORMAT_R32G32B32A32_SFLOAT; }

template<>
constexpr VkFormat toVkFormat<glm::ivec2>() { return VK_FORMAT_R32G32_SINT; }
template<>
constexpr VkFormat toVkFormat<glm::ivec3>() { return VK_FORMAT_R32G32B32_SINT; }
template<>
constexpr VkFormat toVkFormat<glm::ivec4>() { return VK_FORMAT_R32G32B32A32_SINT; }

template<>
constexpr VkFormat toVkFormat<glm::uvec2>() { return VK_FORMAT_R32G32_UINT; }
template<>
constexpr VkFormat toVkFormat<glm::uvec3>() { return VK_FORMAT_R32G32B32_UINT; }
template<>
constexpr VkFormat toVkFormat<glm::uvec4>() { return VK_FORMAT_R32G32B32A32_UINT; }

template<>
constexpr VkFormat toVkFormat<glm::dvec2>() { return VK_FORMAT_R64G64_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::dvec3>() { return VK_FORMAT_R64G64B64_SFLOAT; }
template<>
constexpr VkFormat toVkFormat<glm::dvec4>() { return VK_FORMAT_R64G64B64A64_SFLOAT; }

template<>
constexpr VkFormat toVkFormat<glm::bvec2>() { return VK_FORMAT_R8_UINT; } // Boolean vectors are often represented as uints
template<>
constexpr VkFormat toVkFormat<glm::bvec3>() { return VK_FORMAT_R8G8B8_UINT; }
template<>
constexpr VkFormat toVkFormat<glm::bvec4>() { return VK_FORMAT_R8G8B8A8_UINT; }

// Matrix types, simplified as arrays of vectors (no direct Vulkan format)
// Note: Vulkan itself does not have direct matrix formats.
// Here we provide a simplified mapping for the sake of example.
// For matrices, consider how you will use them in your shaders.
template<>
constexpr VkFormat toVkFormat<glm::mat2>() { return VK_FORMAT_R32G32_SFLOAT; } // Simplified; actually handled as 2 vec2s
template<>
constexpr VkFormat toVkFormat<glm::mat3>() { return VK_FORMAT_R32G32B32_SFLOAT; } // Simplified; actually handled as 3 vec3s
template<>
constexpr VkFormat toVkFormat<glm::mat4>() { return VK_FORMAT_R32G32B32A32_SFLOAT; } // Simplified; actually handled as 4 vec4s
template<>
constexpr VkFormat toVkFormat<glm::dmat2>() { return VK_FORMAT_R64G64_SFLOAT; } // Simplified; actually handled as 2 dvec2s
template<>
constexpr VkFormat toVkFormat<glm::dmat3>() { return VK_FORMAT_R64G64B64_SFLOAT; } // Simplified; actually handled as 3 dvec3s
template<>
constexpr VkFormat toVkFormat<glm::dmat4>() { return VK_FORMAT_R64G64B64A64_SFLOAT; } // Simplified; actually handled as 4 dvec4s

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

inline VkFormat GLMTypeToVkFormat(const std::string& type) {
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

	// Matrix types (only covering some common cases for simplicity)
	// Note: Vulkan does not have direct matrix formats; these are handled as arrays of vectors.
	if (type == "mat2") return VK_FORMAT_R32G32_SFLOAT; // This is a simplification
	if (type == "mat3") return VK_FORMAT_R32G32B32_SFLOAT; // This is a simplification
	if (type == "mat4") return VK_FORMAT_R32G32B32A32_SFLOAT; // This is a simplification
	if (type == "dmat2") return VK_FORMAT_R64G64_SFLOAT; // This is a simplification
	if (type == "dmat3") return VK_FORMAT_R64G64B64_SFLOAT; // This is a simplification
	if (type == "dmat4") return VK_FORMAT_R64G64B64A64_SFLOAT; // This is a simplification

	throw std::runtime_error("Unsupported GLM type: " + type);
}

inline size_t GLMTypeSize(const std::string& type) {
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

	// Matrix types (only covering some common cases for simplicity)
	// Note: Vulkan does not have direct matrix formats; these are handled as arrays of vectors.
	if (type == "mat2") return sizeof(glm::mat2); // This is a simplification
	if (type == "mat3") return sizeof(glm::mat3); // This is a simplification
	if (type == "mat4") return sizeof(glm::mat4); // This is a simplification
	if (type == "dmat2") return sizeof(glm::dmat2); // This is a simplification
	if (type == "dmat3") return sizeof(glm::dmat3); // This is a simplification
	if (type == "dmat4") return sizeof(glm::dmat4); // This is a simplification

	if (type == "ubo") return sizeof(UniformBufferObject);

	throw std::runtime_error("Unsupported GLM type: " + type);
}

namespace vox {
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isValid();
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR surfaceCapabilites;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;

		bool isValid();
	};

	struct BufferBuildInfo {
		VkDeviceSize deviceSize;
		VkBufferUsageFlags bufferUsageFlags;
		VkMemoryPropertyFlags memoryPropertyFlags;
		VkBuffer* buffer;
		VkDeviceMemory* bufferMemory;
	};

	struct CommandPooolBuildInfo {
		VkDevice logicalDevice;
		VkCommandPoolCreateInfo* commandPoolInfo;
		VkAllocationCallbacks* allocationCallbacks;
		VkCommandPool* commandPool;
	};

	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
}

