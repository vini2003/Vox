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
#include <array>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>



#include "VkUtils.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class VkApp {
public:
	void run();

	void vkApiPutTriangle(VkUtils::VkTriangle& vertices);

	bool vkFramebufferResized = false;
private:
	const std::vector<const char*> vkValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> vkDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const int DEFAULT_WIDTH = 800;
	const int DEFAULT_HEIGHT = 600;

	const std::string MODEL_PATH = "models/viking_room.obj";
	const std::string TEXTURE_PATH = "textures/viking_room.png";

	const int MAX_FLAMES_IN_FLIGHT = 2;

	const char* NAME = "Vulkan";
	const char* ENGINE = "None";

	uint32_t currentFrame = 0;

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

	VkDescriptorSetLayout vkDescriptorSetLayout;

	std::vector<VkDescriptorSet> vkDescriptorSets;

	VkDescriptorPool vkDescriptorPool;
	VkPipelineLayout vkPipelineLayout;
	VkPipeline vkGraphicsPipeline;

	VkCommandPool vkCommandPool;
	VkCommandPool vkCommandPoolShort;

	VkBuffer vkVertexBuffer;
	VkDeviceMemory vkVertexBufferMemory;
	VkBuffer vkIndexBuffer;
	VkDeviceMemory vkIndexBufferMemory;

	VkImage vkTextureImage;
	VkDeviceMemory vkTextureImageMemory;
	VkImageView vkTextureImageView;
	VkSampler vkTextureSampler;

	VkImage vkDepthImage;
	VkDeviceMemory vkDepthImageMemory;
	VkImageView vkDepthImageView;
	VkSampler vkDepthSampler; // TODO

	std::vector<VkBuffer> vkUniformBuffers;
	std::vector<VkDeviceMemory> vkUniformBufferMemories;
	std::vector<void*> vkUniformBuffersMapped;

	std::vector<VkUtils::VkVertex> vkVertices = {};
	std::vector<uint32_t> vkIndices = {};

	std::vector<VkCommandBuffer> vkCommandBuffers;

	std::vector<VkSemaphore> vkImageAvailableSemaphores;
	std::vector<VkSemaphore> vkRenderFinishedSemaphores;

	std::vector<VkFence> vkInFlightFences;

	bool checkVkValidationLayers();

	VkShaderModule buildShaderModule(const std::vector<char>& rawShader);

	void buildDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkResult buildBuffer(VkUtils::VkBufferBuildInfo buildInfo);
	VkResult buildImage(uint32_t vkWidth, uint32_t vkHeight, VkFormat vkFormat, VkImageTiling vkTiling, VkImageUsageFlags vkUsage, VkMemoryPropertyFlags vkProperties, VkImage& vkImage, VkDeviceMemory& vkImageMemory);
	VkResult buildImageView(VkImage vkImage, VkFormat vkFormat, VkImageAspectFlags vkAspect, VkImageView& vkImageView);

	VkResult copyBuffer(VkBuffer vkSourceBuffer, VkBuffer vkDestinationBuffer, VkDeviceSize vkBufferSize);

	VkResult copyBufferToImage(VkBuffer vkBuffer, VkImage vkImage, uint32_t vkWidth, uint32_t vkHeight);

	VkResult buildPool(VkUtils::VkCommandPooolBuildInfo buildInfo);

	VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormatsAvailable);

	VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& vkPresentModesAvailable);

	VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesAvailable);

	std::vector<VkExtensionProperties> getVkExtensionsAvailable();

	std::vector<const char*> getGlfwExtensionsRequired();

	VkUtils::VkSwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice vkPhysicalDevice);

	bool getExtensionSupport(VkPhysicalDevice vkPhysicalDevice);

	uint32_t getMemoryType(uint32_t vkTypeFilter, VkMemoryPropertyFlags vkFlagProperties);

	VkUtils::VkQueueFamilyIndices getQueueFamilies(VkPhysicalDevice vkPhysicalDevice);

	std::map<std::string, std::vector<char>> getShaders();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& vkCandidates, VkImageTiling vkTiling, VkFormatFeatureFlags vkFeatures);

	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat vkFormat);

	void loadModel();

	void initGlfw();
	void initVk();
	void initImGui();
	void initVkInstance();
	void initVkDebugMessenger();
	void initVkSurface();
	void initVkPhysicalDevice();
	void initVkLogicalDevice();
	void initVkSwapchain();
	void initVkImageViews();
	void initVkRenderPass();
	void initVkDescriptorSetLayout();
	void initVkGraphicsPipeline();
	void initVkFramebuffers();
	void initVkCommandPools();
	void initVkVertexBuffer();
	void initVkIndexBuffer();
	void initVkUniformBuffers();
	void initVkDescriptorPool();
	void initVkDescriptorSets();
	void initVkDepthResources();
	void initVkTextureImage();
	void initVkTextureImageView();
	void initVkTextureSampler();
	void initVkCommandBuffers();
	void initVkSemaphores();

	void updateVkUniformBuffer(uint32_t currentImage);

	void executeImmediateCommand(std::function<void(VkCommandBuffer cmd)> &&function);

	void transitionVkImageLayout(VkImage vkImage, VkFormat vkFormat, VkImageLayout vkOldLayout, VkImageLayout vkNewLayout);

	void freeVkSwapchain();
	void resetVkSwapchain();

	void draw();
	void loop();
	void free();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
};