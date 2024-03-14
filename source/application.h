#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <functional>
#include <vector>
#include <filesystem>
#include <map>
#include <cstring>
#include <unordered_map>
#include <set>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "util.h"

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

class Application {
public:
	void run();

	bool framebufferResized = false;
private:
	const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
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

	VkSurfaceKHR surface;

	VkPhysicalDevice mainPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mainLogicalDevice = VK_NULL_HANDLE;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;

	VkRenderPass renderPass;

	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<VkDescriptorSet> descriptorSets;

	VkDescriptorPool descriptorPool;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkCommandPool commandPool;
	VkCommandPool shortCommandPool;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkSampler depthSampler; // TODO

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBufferMemories;
	std::vector<void*> uniformBuffersMapped;

	std::vector<VkUtils::VkVertex> vertices = {};
	std::vector<uint32_t> indices = {};

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;

	bool checkVkValidationLayers();

	VkShaderModule buildShaderModule(const std::vector<char>& rawShader);

	void buildDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkResult buildBuffer(VkUtils::VkBufferBuildInfo buildInfo);
	VkResult buildImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory);
	VkResult buildImageView(VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags, VkImageView& imageView);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer destBuffer, VkDeviceSize bufferSize);

	VkResult copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkResult buildPool(VkUtils::VkCommandPooolBuildInfo buildInfo);

	VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats);

	VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);

	VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	std::vector<VkExtensionProperties> getVkExtensionsAvailable();

	std::vector<const char*> getGlfwExtensionsRequired();

	VkUtils::VkSwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice physicalDevice);

	bool hasSamplerAnisotropySupport(VkPhysicalDeviceFeatures physicalDeviceFeatures);

	bool hasExtensionSupport(VkPhysicalDevice vkPhysicalDevice);

	bool hasRequiredFeatures(VkPhysicalDevice physicalDevice);

	uint32_t getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);

	VkUtils::VkQueueFamilyIndices getQueueFamilies(VkPhysicalDevice physicalDevice);

	std::map<std::string, std::vector<char>> getShaders();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling imageTiling, VkFormatFeatureFlags formatFatureFlags);

	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat vkFormat);

	void loadModel();

	void initGlfw();
	void initVk();
	void initVkInstance();
	void initImGui();
	void initDebugMessenger();
	void initSurface();
	void initPhysicalDevice();
	void initLogicalDevice();
	void initSwapchain();
	void initImageViews();
	void initRenderPass();
	void initDescriptorSetLayout();
	void initPipeline();
	void initFramebuffers();
	void initCommandPools();
	void initVertexBuffer();
	void initIndexBuffer();
	void initUniformBuffers();
	void initDescriptorPool();
	void initDescriptorSets();
	void initDepthResources();
	void initTextureImage();
	void initTextureImageView();
	void initTextureSampler();
	void initCommandBuffers();
	void initSyncObjects();

	void updateVkUniformBuffer(uint32_t currentImage);

	void executeImmediateCommand(std::function<void(VkCommandBuffer cmd)> &&function);

	void transitionVkImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void freeVkSwapchain();
	void resetVkSwapchain();

	void draw();
	void loop();
	void free();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
};