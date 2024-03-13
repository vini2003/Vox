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
#include <vector>

namespace VkUtils {
	struct VkQueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isValid();
	};

	struct VkSwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR vkSurfaceCapabilites;
		std::vector<VkSurfaceFormatKHR> vkSurfaceFormats;
		std::vector<VkPresentModeKHR> vkPresentModes;

		bool isValid();
	};

	struct VkBufferBuildInfo {
		VkDeviceSize vkSize;
		VkBufferUsageFlags vkUsage;
		VkMemoryPropertyFlags vkProperties;
		VkBuffer* vkBuffer;
		VkDeviceMemory* vkBufferMemory;
	};

	struct VkCommandPooolBuildInfo {
		VkDevice vkLogicalDevice;
		VkCommandPoolCreateInfo* vkCommandPoolInfo;
		VkAllocationCallbacks* vkAllocationCallbacks;
		VkCommandPool* vkCommandPool;
	};

	struct VkVertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription();
		
		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription();

		bool operator==(const VkVertex& other) const;
	};

	struct VkTriangle {
		VkVertex vA, vB, vC;
	};

	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
}

namespace std {
	template<> struct hash<VkUtils::VkVertex> {
		size_t operator()(VkUtils::VkVertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				   (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}