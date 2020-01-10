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

	GLFWwindow* glfwWindow;

	VkInstance vkInstance;

	VkDebugUtilsMessengerEXT debugMessenger;

	VkSurfaceKHR vkSurface;

	VkPhysicalDevice vkMainPhysicalDevice = VK_NULL_HANDLE;
	VkDevice vkMainLogicalDevice = VK_NULL_HANDLE;

	VkQueue vkGraphicsQueue;
	VkQueue vkPresentQueue;

	VkSwapchainKHR vkSwapchain;
	VkFormat vkSwapchainImageFormat;
	VkExtent2D vkSwapchainExtent;
	std::vector<VkImage> vkSwapchainImages;
	std::vector<VkImageView> vkSwapchainImageViews;
	std::vector<VkFramebuffer> vkSwapchainFramebuffers;

	VkRenderPass vkRenderPass;

	VkPipelineLayout vkPipelineLayout;
	VkPipeline vkGraphicsPipeline;

	VkCommandPool vkCommandPool;

	std::vector<VkCommandBuffer> vkCommandBuffers;

	VkSemaphore vkImageAvailableSemaphore;
	VkSemaphore vkRenderFinishedSemaphore;

	bool checkVkValidationLayers();

	VkShaderModule buildShaderModule(const std::vector<char>& rawShader);

	void buildDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormatsAvailable);

	VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& vkPresentModesAvailable);

	VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesAvailable);

	std::vector<VkExtensionProperties> getVkExtensionsAvailable();

	std::vector<const char*> getGlfwExtensionsRequired();

	VkUtils::VkSwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice vkPhysicalDevice);

	bool getExtensionSupport(VkPhysicalDevice vkPhysicalDevice);

	VkUtils::VkQueueFamilyIndices getQueueFamilies(VkPhysicalDevice vkPhysicalDevice);

	std::map<std::string, std::vector<char>> getShaders();

	void initGlfw();
	void initVk();
	void initVkInstance();
	void initVkDebugMessenger();
	void initVkSurface();
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

	void draw();
	void loop();
	void clear();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
};