#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
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

	struct VkVertex {
		glm::vec2 vkPos;
		glm::vec3 vkColor;

		static VkVertexInputBindingDescription getBindingDescription();
		
		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
	};

	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
}