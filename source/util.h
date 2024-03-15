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

