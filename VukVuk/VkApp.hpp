#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <optional>
#include <set>
#include <filesystem>
#include <fstream>
#include <map>

#include "VkUtils.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class VkApp {
public:
	void run();

private:
	const std::vector<const char*> vkValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> vkDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const int WIDTH = 800;
	const int HEIGHT = 600;
	const char* NAME = "Vulkan";
	const char* ENGINE = "None";

	VkSwapchainKHR vkSwapchain;
	std::vector<VkImage> vkSwapchainImages;
	std::vector<VkImageView> vkSwapchainImageViews;
	VkFormat vkSwapchainImageFormat;
	VkExtent2D vkSwapchainExtent;
	VkInstance vkInstance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR vkSurface;
	VkPhysicalDevice vkMainPhysicalDevice = VK_NULL_HANDLE;
	VkDevice vkMainLogicalDevice = VK_NULL_HANDLE;
	VkQueue vkGraphicsQueue;
	VkQueue vkPresentQueue;
	VkPipelineLayout vkPipelineLayout;
	VkRenderPass vkRenderPass;
	VkPipeline vkGraphicsPipeline;
	std::vector<VkFramebuffer> vkSwapchainFramebuffers;
	VkCommandPool vkCommandPool;
	std::vector<VkCommandBuffer> vkCommandBuffers;
	VkSemaphore vkImageAvailableSemaphore;
	VkSemaphore vkRenderFinishedSemaphore;

	GLFWwindow* glfwWindow;

	std::vector<VkExtensionProperties> getVkExtensionsAvailable();

	std::vector<const char*> getGlfwExtensionsRequired();

	bool checkVkValidationLayers();

	void initVkInstance();

	void initVkSurface();

	void initDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void initVkDebugMessenger();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormatsAvailable);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vkPresentModesAvailable);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesAvailable);

	VkUtils::VkSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice vkPhysicalDevice);

	bool queryExtensionSupport(VkPhysicalDevice vkPhysicalDevice);

	VkUtils::VkQueueFamilyIndices queryQueueFamilies(VkPhysicalDevice vkPhysicalDevice);

	std::map<std::string, std::vector<char>> queryShaders();

	VkShaderModule buildShaderModule(const std::vector<char>& rawShader);

	void initVkPhysicalDevice();

	void initVkLogicalDevice();

	void initVkSwapChain();

	void initVkImageViews();

	void initVkRenderPass();

	void initVkGraphicsPipeline();

	void initVkFramebuffers();

	void initVkCommandPool();

	void initVkCommandBuffers();

	void initVkSemaphores();

	void initGlfw();

	void initVk();

	void vkDrawFrame();

	void loop();

	void clear();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
};