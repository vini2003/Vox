#ifndef APPLICATION_H
#define APPLICATION_H

#include <iostream>
#include <functional>
#include <vector>
#include <filesystem>
#include <map>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "util.h"
#include "vertex.h"
#include "shader.h"

#ifdef NDEBUG
constexpr auto enableValidationLayers = false;
#else
constexpr auto enableValidationLayers = true;
#endif

namespace vox {
	class Application : public std::enable_shared_from_this<Application> {
	public:
		void run();

		bool framebufferResized = false;

		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		const std::string MODEL_PATH = "models/viking_room.obj";
		const std::string TEXTURE_PATH = "textures/viking_room.png";

		const int MAX_FLAMES_IN_FLIGHT = 2;

		const char* NAME = "Vulkan";
		const char* ENGINE = "None";

		std::map<std::string, Shader<>> shaders = {};

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

		VkDescriptorPool descriptorPool;
		VkDescriptorPool imguiDescriptorPool;

		std::map<std::string, VkPipelineLayout> pipelineLayouts;
		std::map<std::string, VkPipeline> pipelines;

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

		std::vector<Vertex> vertices = {};

		std::vector<uint32_t> indices = {};

		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;

		std::vector<VkFence> inFlightFences;

		Camera camera = {};

		UniformBufferObject ubo = {};

		bool checkValidationLayers() const;

		VkShaderModule buildShaderModule(const std::vector<char>& rawShader) const;

		static VkPipelineShaderStageCreateInfo buildPipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage);
		static VkPipelineViewportStateCreateInfo buildPipelineViewportStateCreateInfo(const VkViewport *viewport, const VkRect2D *scissor);
		static VkPipelineRasterizationStateCreateInfo buildPipelineRasterizationStateCreateInfo();
		static VkPipelineMultisampleStateCreateInfo buildPipelineMultisampleStateCreateInfo();
		static VkPipelineColorBlendAttachmentState buildPipelineColorBlendAttachmentState();
		static VkPipelineColorBlendStateCreateInfo buildPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *colorBlendAttachmentState);
		static VkPipelineDepthStencilStateCreateInfo buildPipelineDepthStencilStateCreateInfo();

		static void buildDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		VkResult buildBuffer(VkBuffer *buffer, VkDeviceMemory *bufferMemory, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
		VkResult buildImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory);
		VkResult buildImageView(VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags, VkImageView& imageView);
		VkResult buildTextureImage(const std::string& imagePath, VkImage& textureImage, VkDeviceMemory& textureImageMemory);

		VkResult buildUniformBuffer(VkBuffer *buffer, VkDeviceMemory *bufferMemory, const VkDeviceSize size);

		template<typename T>
		VkResult buildUniformBuffer(VkBuffer *buffer, VkDeviceMemory *bufferMemory);

		template<class T>
		VkResult buildVertexBuffer(VkBuffer *buffer, VkDeviceMemory *bufferMemory, const std::vector<T> &data);

		template<class T>
		VkResult buildIndexBuffer(VkBuffer *buffer, VkDeviceMemory *bufferMemory, const std::vector<T> &data);

		void copyBuffer(VkBuffer srcBuffer, VkBuffer destBuffer, VkDeviceSize bufferSize);

		VkResult copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkResult buildSampler(VkSampler* sampler, VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float maxAnisotropy = 1.0f, VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, bool compareEnable = false, VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS, VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, float mipLodBias = 0.0f, float minLod = 0.0f, float maxLod = 0.0f) const;

		VkResult buildCommandPool(VkCommandPool *pool, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) const;

		VkResult buildFramebuffer(VkFramebuffer* framebuffer, VkRenderPass renderPass, const std::vector<VkImageView> &attachments, uint32_t width, uint32_t height, uint32_t layers = 1) const;

		VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats);
		VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);

		VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

		std::vector<VkExtensionProperties> getVkExtensionsAvailable();
		std::vector<const char*> getGlfwExtensionsRequired();

		SwapChainSupport getSwapChainSupport(VkPhysicalDevice physicalDevice);

		bool hasSamplerAnisotropySupport(VkPhysicalDeviceFeatures physicalDeviceFeatures);
		bool hasExtensionSupport(VkPhysicalDevice vkPhysicalDevice);
		bool hasRequiredFeatures(VkPhysicalDevice physicalDevice);

		uint32_t getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags);

		QueueFamilies getQueueFamilies(VkPhysicalDevice physicalDevice);

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling imageTiling, VkFormatFeatureFlags formatFatureFlags);

		VkFormat findDepthFormat();

		bool hasStencilComponent(VkFormat vkFormat);

		void loadModel();

		void handleInput(GLFWwindow *window, float timeDelta);
		static void handleMouseScroll(GLFWwindow *window, double x, double y);
		static void handleFramebufferResize(GLFWwindow *window, int width, int height);

		void initGlfw();
		void initVk();
		void initVkInstance();
		void initImGui();

		void initImGuiStyle();

		void initDebugMessenger();
		void initShaders();
		void initSurface();
		void initPhysicalDevice();
		void initLogicalDevice();
		void initSwapchain();
		void initImageViews();
		void initRenderPass();
		void initDescriptorSetLayouts();
		void initPipeline();
		void initFramebuffers();
		void initCommandPools();
		void initVertexBuffer();
		void initIndexBuffer();
		void initUniformBuffers();
		void initUniformBufferObjects();
		void initDescriptorPool();
		void initDescriptorSets();
		void initDepthResources();
		void initTextureImage();
		void initTextureImageView();
		void initTextureSampler();
		void initCommandBuffers();
		void initSyncObjects();

		void updateUniformBuffers(uint32_t currentImage);

		void executeImmediateCommand(std::function<void(VkCommandBuffer cmd)> &&function);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		void freeVkSwapchain();
		void resetVkSwapchain();

		void draw();
		void drawImGui();

		void loop();
		void free();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
	};

	template<typename T>
	VkResult Application::buildUniformBuffer(VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
		return buildUniformBuffer(buffer, bufferMemory, sizeof(T));
	}

	template<typename T>
	VkResult Application::buildVertexBuffer(VkBuffer* buffer, VkDeviceMemory* bufferMemory, const std::vector<T>& data) {
		VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		if (const auto result = buildBuffer(&stagingBuffer, &stagingBufferMemory, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			result != VK_SUCCESS) {
			return result;
		}

		void* mappedData;

		if (const auto result = vkMapMemory(mainLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &mappedData);
			result != VK_SUCCESS) {
			return result;
		}

		memcpy(mappedData, data.data(), bufferSize);
		vkUnmapMemory(mainLogicalDevice, stagingBufferMemory);

		if (const auto result = buildBuffer(buffer, bufferMemory, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			result != VK_SUCCESS) {
			return result;
		}

		copyBuffer(stagingBuffer, *buffer, bufferSize);

		vkDestroyBuffer(mainLogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainLogicalDevice, stagingBufferMemory, nullptr);

		return VK_SUCCESS;
	}

	template<typename T>
	VkResult Application::buildIndexBuffer(VkBuffer* buffer, VkDeviceMemory* bufferMemory, const std::vector<T>& data) {
		const VkDeviceSize bufferSize = sizeof(data[0]) * data.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		if (const auto result = buildBuffer(&stagingBuffer, &stagingBufferMemory, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			result != VK_SUCCESS) {
			return result;
		}

		void* mappedData;

		if (const auto result = vkMapMemory(mainLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &mappedData);
			result != VK_SUCCESS) {
			return result;
		}

		memcpy(mappedData, data.data(), bufferSize);

		vkUnmapMemory(mainLogicalDevice, stagingBufferMemory);

		if (const auto result = buildBuffer(buffer, bufferMemory, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			result != VK_SUCCESS) {
			return result;
		}

		copyBuffer(stagingBuffer, *buffer, bufferSize);

		vkDestroyBuffer(mainLogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainLogicalDevice, stagingBufferMemory, nullptr);

		return VK_SUCCESS;
	}
}

#endif
