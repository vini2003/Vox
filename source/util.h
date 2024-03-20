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
#include <string>
#include <vector>

namespace vox {
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

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct QueueFamilies {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool areValid() const;
	};

	struct SwapChainSupport {
		VkSurfaceCapabilitiesKHR surfaceCapabilites;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;

		[[nodiscard]] bool isValid() const;
	};

	VkFormat GLMTypeToVkFormat(const std::string& type);

	size_t GLMTypeSize(const std::string& type);
	size_t GLMTypeAlignment(const std::string& type);
	size_t GLMTypeAlignUp(size_t size, size_t alignment);

	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
}

