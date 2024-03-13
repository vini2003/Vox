#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <cstring>
#include <unordered_map>

#include "VkApp.hpp"
#include "VkApi.hpp"

void VkApp::initGlfw() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindow = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, NAME, nullptr, nullptr);
	glfwSetWindowUserPointer(glfwWindow, this);

	glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* window, int width, int height) {
		auto* application = static_cast<VkApp*>(glfwGetWindowUserPointer(window));
		application->vkFramebufferResized = true;
	});
}

void VkApp::initVk() {
	initVkInstance();
	initVkDebugMessenger();
	initVkSurface();
	initVkPhysicalDevice();
	initVkLogicalDevice();
	initVkSwapchain();
	initVkImageViews();
	initVkRenderPass();
	initVkDescriptorSetLayout();
	initVkGraphicsPipeline();
	initVkCommandPools();
	initVkDepthResources();
	initVkFramebuffers();
	initVkTextureImage();
	initVkTextureImageView();
	initVkTextureSampler();
	loadModel();
	initVkVertexBuffer();
	initVkIndexBuffer();
	initVkUniformBuffers();
	initVkDescriptorPool();
	initVkDescriptorSets();
	initVkCommandBuffers();
	initVkSemaphores();
}

void VkApp::executeImmediateCommand(std::function<void(VkCommandBuffer cmd)> &&function) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vkCommandPoolShort;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vkMainLogicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	function(commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkGraphicsQueue);

	vkFreeCommandBuffers(vkMainLogicalDevice, vkCommandPoolShort, 1, &commandBuffer);
}

void VkApp::transitionVkImageLayout(VkImage vkImage, VkFormat vkFormat, VkImageLayout vkOldLayout, VkImageLayout vkNewLayout) {
	executeImmediateCommand([&](VkCommandBuffer commandBuffer) {
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = vkOldLayout;
		barrier.newLayout = vkNewLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = vkImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (vkOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && vkNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (vkOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && vkNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	});
}


void VkApp::initImGui() {
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	VkDescriptorPool pool;

	const auto result = vkCreateDescriptorPool(vkMainLogicalDevice, &poolInfo, nullptr, &pool);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create ImGui descriptor pool!");
	}

	std::cout << "[Vulkan] ImGui descriptor pool creation succeeded.\n" << std::flush;

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true); // TODO: Check if install_callbacks is needed.

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = vkInstance;
	initInfo.PhysicalDevice = vkMainPhysicalDevice;
	initInfo.Device = vkMainLogicalDevice;
	initInfo.QueueFamily = getQueueFamilies(vkMainPhysicalDevice).graphicsFamily.value(); // TODO: Check if this works.
	initInfo.Queue = vkGraphicsQueue;
	initInfo.DescriptorPool = pool;
	initInfo.MinImageCount = 3;
	initInfo.ImageCount = 3;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo, vkRenderPass);
}


void VkApp::initVkInstance() {
	if (enableValidationLayers && !checkVkValidationLayers()) {
		throw std::runtime_error("Requested validation layers not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = ENGINE;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(vkValidationLayers.size());
		createInfo.ppEnabledLayerNames = vkValidationLayers.data();

		buildDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	std::vector<const char*> glfwExtensionsRequired = getGlfwExtensionsRequired();

	std::cout << "[GLFW] Required extensions:\n" << std::flush;
	std::ranges::for_each(glfwExtensionsRequired, [](const char* glfwExtension) {
		std::cout << "\t - " << glfwExtension << "\n" << std::flush;
	});

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensionsRequired.size());
	createInfo.ppEnabledExtensionNames = glfwExtensionsRequired.data();

	std::vector<VkExtensionProperties> vkExtensionsRequired = getVkExtensionsAvailable();

	std::cout << "[Vulkan] Available extensions:\n" << std::flush;
	std::ranges::for_each(vkExtensionsRequired, [](const VkExtensionProperties &vkExtension) {
		std::cout << "\t - " << vkExtension.extensionName << "\n" << std::flush;
	});

	const auto result = vkCreateInstance(&createInfo, nullptr, &vkInstance);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkInstance!");
	}

	std::cout << "[Vulkan] Initialized instance.\n" << std::flush;
}

void VkApp::initVkDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	buildDebugMessengerCreateInfo(createInfo);

	const auto result = VkUtils::createDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger!");
	}

	std::cout << "[Vulkan] Initialized debug messenger.\n" << std::flush;
}

void VkApp::initVkSurface() {
	const auto result = glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &vkSurface);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}

	std::cout << "[Vulkan] Initialized surface.\n" << std::flush;
}

void VkApp::initVkPhysicalDevice() {
	uint32_t vkDeviceCount = 0;
	vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, nullptr);

	if (vkDeviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> vkDevices(vkDeviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, vkDevices.data());

	std::for_each(vkDevices.begin(), vkDevices.end(), [&](VkPhysicalDevice vkPhysicalDevice) {
		if ([&]()->bool {
			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &supportedFeatures);

			return getQueueFamilies(vkPhysicalDevice).isValid() && getExtensionSupport(vkPhysicalDevice) && getSwapChainSupport(vkPhysicalDevice).isValid() && supportedFeatures.samplerAnisotropy;
		}()) {
			vkMainPhysicalDevice = vkPhysicalDevice;

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(vkMainPhysicalDevice, &deviceProperties);

			std::cout << "[Vulkan] Physical device selected: " << deviceProperties.deviceName << "\n" << std::flush;

			return false;
		}

		return true;
	});

	std::cout << "[Vulkan] Initialized physical device.\n" << std::flush;
}

void VkApp::initVkLogicalDevice() {
	VkUtils::VkQueueFamilyIndices indices = getQueueFamilies(vkMainPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}


	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(vkDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = vkDeviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(vkValidationLayers.size());
		createInfo.ppEnabledLayerNames = vkValidationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	const auto result = vkCreateDevice(vkMainPhysicalDevice, &createInfo, nullptr, &vkMainLogicalDevice);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(vkMainLogicalDevice, indices.graphicsFamily.value(), 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkMainLogicalDevice, indices.presentFamily.value(), 0, &vkPresentQueue);

	std::cout << "[Vulkan] Initialized logical device.\n" << std::flush;
}

void VkApp::initVkSwapchain() {
	VkUtils::VkSwapChainSupportDetails vkDetails = getSwapChainSupport(vkMainPhysicalDevice);

	VkSurfaceFormatKHR vkSurfaceFormat = selectSwapSurfaceFormat(vkDetails.vkSurfaceFormats);
	VkPresentModeKHR vkPresentMode = selectSwapPresentMode(vkDetails.vkPresentModes);
	VkExtent2D vkExtent = selectSwapExtent(vkDetails.vkSurfaceCapabilites);

	uint32_t vkMinImageCount = vkDetails.vkSurfaceCapabilites.minImageCount + 1;

	if (vkDetails.vkSurfaceCapabilites.maxImageCount > 0 && vkMinImageCount > vkDetails.vkSurfaceCapabilites.maxImageCount) {
		vkMinImageCount = vkDetails.vkSurfaceCapabilites.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = vkSurface;
	createInfo.minImageCount = vkMinImageCount;
	createInfo.imageFormat = vkSurfaceFormat.format;
	createInfo.imageColorSpace = vkSurfaceFormat.colorSpace;
	createInfo.imageExtent = vkExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkUtils::VkQueueFamilyIndices vkIndices = getQueueFamilies(vkMainPhysicalDevice);
	uint32_t queueFamilyIndices[] = { vkIndices.graphicsFamily.value(), vkIndices.presentFamily.value() };

	if (vkIndices.graphicsFamily != vkIndices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = vkDetails.vkSurfaceCapabilites.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = vkPresentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	const auto result = vkCreateSwapchainKHR(vkMainLogicalDevice, &createInfo, nullptr, &vkSwapchain);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create swap chain!");
	}

	std::cout << "[Vulkan] Initialized swapchain.\n" << std::flush;

	vkGetSwapchainImagesKHR(vkMainLogicalDevice, vkSwapchain, &vkMinImageCount, nullptr);
	vkSwapchainImages.resize(vkMinImageCount);
	vkGetSwapchainImagesKHR(vkMainLogicalDevice, vkSwapchain, &vkMinImageCount, vkSwapchainImages.data());

	vkSwapchainImageFormat = vkSurfaceFormat.format;
	vkSwapchainExtent = vkExtent;
}

void VkApp::initVkImageViews() {
	vkSwapchainImageViews.resize(vkSwapchainImages.size());

	int i = 0;

	std::ranges::for_each(vkSwapchainImages, [&](VkImage vkImage) {
		VkImageView imageView;

		const auto result = buildImageView(vkSwapchainImages[i], vkSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, imageView);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create image views!");
		}

		vkSwapchainImageViews[i] = imageView;

		++i;
	});

	std::cout << "[Vulkan] Image view creation successfull.\n" << std::flush;
}

void VkApp::initVkRenderPass() {
	VkAttachmentDescription vkColorAttachmentDescription = {};
	vkColorAttachmentDescription.format = vkSwapchainImageFormat;
	vkColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	vkColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	vkColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	vkColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	vkColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vkColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription vkDepthAttachmentDescription = {};
	vkDepthAttachmentDescription.format = findDepthFormat();
	vkDepthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	vkDepthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	vkDepthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vkDepthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	vkDepthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vkDepthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkDepthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference vkColorAttachmentReference = {};
	vkColorAttachmentReference.attachment = 0;
	vkColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference vkDepthAttachmentReference = {};
	vkDepthAttachmentReference.attachment = 1;
	vkDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &vkColorAttachmentReference;\
	subpass.pDepthStencilAttachment = &vkDepthAttachmentReference;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array attachments = { vkColorAttachmentDescription, vkDepthAttachmentDescription };

	VkRenderPassCreateInfo vkRenderPassInfo = {};
	vkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	vkRenderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	vkRenderPassInfo.pAttachments = attachments.data();
	vkRenderPassInfo.subpassCount = 1;
	vkRenderPassInfo.pSubpasses = &subpass;
	vkRenderPassInfo.dependencyCount = 1;
	vkRenderPassInfo.pDependencies = &dependency;

	const auto result = vkCreateRenderPass(vkMainLogicalDevice, &vkRenderPassInfo, nullptr, &vkRenderPass);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create render pass!");
	}
}

void VkApp::initVkDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding vkUboLayoutBinding = {};
	vkUboLayoutBinding.binding = 0;
	vkUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vkUboLayoutBinding.descriptorCount = 1;
	vkUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	vkUboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding vkSamplerLayoutBinding = {};
	vkSamplerLayoutBinding.binding = 1;
	vkSamplerLayoutBinding.descriptorCount = 1;
	vkSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	vkSamplerLayoutBinding.pImmutableSamplers = nullptr;
	vkSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	const auto bindings = std::array { vkUboLayoutBinding, vkSamplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
	vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	vkDescriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	vkDescriptorSetLayoutCreateInfo.pBindings = bindings.data();

	const auto resultA = vkCreateDescriptorSetLayout(vkMainLogicalDevice, &vkDescriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);

	if (resultA != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create descriptor set layout!");
	}
}

void VkApp::initVkGraphicsPipeline() {
	std::map<std::string, std::vector<char>> rawShaders = getShaders();

	VkShaderModule vkVertShaderModule = buildShaderModule(rawShaders["vert.spv"]);
	VkShaderModule vkFragShaderModule = buildShaderModule(rawShaders["frag.spv"]);

	VkPipelineShaderStageCreateInfo vkVertShaderStageInfo = {};
	vkVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vkVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vkVertShaderStageInfo.module = vkVertShaderModule;
	vkVertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo vkFragShaderStageInfo = {};
	vkFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vkFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vkFragShaderStageInfo.module = vkFragShaderModule;
	vkFragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo vkShaderStages[] = { vkVertShaderStageInfo, vkFragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vkVertexInputInfo = {};
	vkVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = VkUtils::VkVertex::getBindingDescription();
	auto attributeDescription = VkUtils::VkVertex::getAttributeDescription();

	vkVertexInputInfo.vertexBindingDescriptionCount = 1;
	vkVertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vkVertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vkVertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo vkInputAssemblyInfo = {};
	vkInputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vkInputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vkInputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport vkViewport = {};
	vkViewport.x = 0.0f;
	vkViewport.y = 0.0f;
	vkViewport.width = (float)vkSwapchainExtent.width;
	vkViewport.height = (float)vkSwapchainExtent.height;
	vkViewport.minDepth = 0.0f;
	vkViewport.maxDepth = 1.0f;

	VkRect2D vkScissor = {};
	vkScissor.offset = { 0, 0 };
	vkScissor.extent = vkSwapchainExtent;

	VkPipelineViewportStateCreateInfo vkViewportStateInfo = {};
	vkViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vkViewportStateInfo.viewportCount = 1;
	vkViewportStateInfo.pViewports = &vkViewport;
	vkViewportStateInfo.scissorCount = 1;
	vkViewportStateInfo.pScissors = &vkScissor;

	VkPipelineRasterizationStateCreateInfo vkRasterizerInfo = {};
	vkRasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	vkRasterizerInfo.depthClampEnable = VK_FALSE;
	vkRasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	vkRasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
	vkRasterizerInfo.lineWidth = 1.0f;
	vkRasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	vkRasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	vkRasterizerInfo.depthBiasEnable = VK_FALSE;
	vkRasterizerInfo.depthBiasConstantFactor = 0.0f;
	vkRasterizerInfo.depthBiasClamp = 0.0f;
	vkRasterizerInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo vkMultisamplingInfo = {};
	vkMultisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	vkMultisamplingInfo.sampleShadingEnable = VK_FALSE;
	vkMultisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	vkMultisamplingInfo.minSampleShading = 1.0f;
	vkMultisamplingInfo.pSampleMask = nullptr;
	vkMultisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	vkMultisamplingInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState vkColorBlendAttachmentInfo = {};
	vkColorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	vkColorBlendAttachmentInfo.blendEnable = VK_TRUE;
	vkColorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	vkColorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	vkColorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
	vkColorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	vkColorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	vkColorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo vkColorBlendingInfo = {};
	vkColorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	vkColorBlendingInfo.logicOpEnable = VK_FALSE;
	vkColorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	vkColorBlendingInfo.attachmentCount = 1;
	vkColorBlendingInfo.pAttachments = &vkColorBlendAttachmentInfo;
	vkColorBlendingInfo.blendConstants[0] = 0.0f;
	vkColorBlendingInfo.blendConstants[1] = 0.0f;
	vkColorBlendingInfo.blendConstants[2] = 0.0f;
	vkColorBlendingInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo vkDepthStencilInfo = {};
	vkDepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	vkDepthStencilInfo.depthTestEnable = VK_TRUE;
	vkDepthStencilInfo.depthWriteEnable = VK_TRUE;
	vkDepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	vkDepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	vkDepthStencilInfo.minDepthBounds = 0.0f;
	vkDepthStencilInfo.maxDepthBounds = 1.0f;
	vkDepthStencilInfo.stencilTestEnable = VK_FALSE;
	vkDepthStencilInfo.front = {};
	vkDepthStencilInfo.back = {};

	VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
	vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkPipelineLayoutInfo.setLayoutCount = 1;
	vkPipelineLayoutInfo.pSetLayouts = &vkDescriptorSetLayout;

	const auto resultA = vkCreatePipelineLayout(vkMainLogicalDevice, &vkPipelineLayoutInfo, nullptr, &vkPipelineLayout);

	if (resultA != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create pipeline layout!");
	}

	std::cout << "[Vulkan] Pipeline layout initialization succeeded.\n" << std::flush;

	VkGraphicsPipelineCreateInfo vkPipelineInfo = {};
	vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkPipelineInfo.stageCount = 2;
	vkPipelineInfo.pStages = vkShaderStages;
	vkPipelineInfo.pVertexInputState = &vkVertexInputInfo;
	vkPipelineInfo.pInputAssemblyState = &vkInputAssemblyInfo;
	vkPipelineInfo.pViewportState = &vkViewportStateInfo;
	vkPipelineInfo.pRasterizationState = &vkRasterizerInfo;
	vkPipelineInfo.pMultisampleState = &vkMultisamplingInfo;
	vkPipelineInfo.pDepthStencilState = &vkDepthStencilInfo;
	vkPipelineInfo.pColorBlendState = &vkColorBlendingInfo;
	vkPipelineInfo.pDynamicState = nullptr;
	vkPipelineInfo.layout = vkPipelineLayout;
	vkPipelineInfo.renderPass = vkRenderPass;
	vkPipelineInfo.subpass = 0;
	vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	vkPipelineInfo.basePipelineIndex = -1;

	const auto resultB = vkCreateGraphicsPipelines(vkMainLogicalDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, nullptr, &vkGraphicsPipeline);

	if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create graphics pipeline!");
	}

	std::cout << "[Vulkan] Pipeline initialization succeeded.\n" << std::flush;

	vkDestroyShaderModule(vkMainLogicalDevice, vkFragShaderModule, nullptr);
	vkDestroyShaderModule(vkMainLogicalDevice, vkVertShaderModule, nullptr);
}

void VkApp::initVkFramebuffers() {
	vkSwapchainFramebuffers.resize(vkSwapchainImageViews.size());

	int i = 0;
	std::for_each(vkSwapchainImageViews.begin(), vkSwapchainImageViews.end(), [&](VkImageView vkImageView) {
		std::array vkAttachments = { vkImageView, vkDepthImageView };

		VkFramebufferCreateInfo vkFramebufferInfo = {};
		vkFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		vkFramebufferInfo.renderPass = vkRenderPass;
		vkFramebufferInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
		vkFramebufferInfo.pAttachments = vkAttachments.data();
		vkFramebufferInfo.width = vkSwapchainExtent.width;
		vkFramebufferInfo.height = vkSwapchainExtent.height;
		vkFramebufferInfo.layers = 1;

		const auto result = vkCreateFramebuffer(vkMainLogicalDevice, &vkFramebufferInfo, nullptr, &vkSwapchainFramebuffers[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create framebuffer!");
		}

		++i;
	});

	std::cout << "[Vulkan] Framebuffer initialization succeeded.\n" << std::flush;
}

void VkApp::initVkCommandPools() {
	VkUtils::VkQueueFamilyIndices queueFamilyIndices = getQueueFamilies(vkMainPhysicalDevice);

	VkCommandPoolCreateInfo vkCommandPoolInfo = {};
	vkCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	vkCommandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	vkCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkUtils::VkCommandPooolBuildInfo buildInfo = {};
	buildInfo.vkLogicalDevice = vkMainLogicalDevice;
	buildInfo.vkCommandPoolInfo = &vkCommandPoolInfo;
	buildInfo.vkAllocationCallbacks = nullptr;
	buildInfo.vkCommandPool = &vkCommandPool;

	buildPool(buildInfo);

	vkCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	buildInfo.vkCommandPool = &vkCommandPoolShort;

	buildPool(buildInfo);
}

void VkApp::initVkVertexBuffer() {
	VkBuffer vkStagingBuffer;
	VkDeviceMemory vkStagingBufferMemory;

	VkUtils::VkBufferBuildInfo buildInfo = {};
	buildInfo.vkSize = sizeof(vkVertices[0]) * vkVertices.size();
	buildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	buildInfo.vkBuffer = &vkStagingBuffer;
	buildInfo.vkBufferMemory = &vkStagingBufferMemory;

	buildBuffer(buildInfo);

	void* data;
	vkMapMemory(vkMainLogicalDevice, vkStagingBufferMemory, 0, buildInfo.vkSize, 0, &data);

	memcpy(data, vkVertices.data(), (size_t) buildInfo.vkSize);

	vkUnmapMemory(vkMainLogicalDevice, vkStagingBufferMemory);

	buildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buildInfo.vkProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	buildInfo.vkBuffer = &vkVertexBuffer;
	buildInfo.vkBufferMemory = &vkVertexBufferMemory;

	buildBuffer(buildInfo);

	copyBuffer(vkStagingBuffer, vkVertexBuffer, buildInfo.vkSize);

	vkDestroyBuffer(vkMainLogicalDevice, vkStagingBuffer, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkStagingBufferMemory, nullptr);
}

void VkApp::initVkIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(vkIndices[0]) * vkIndices.size();

	VkBuffer vkStagingBuffer;
	VkDeviceMemory vkStagingBufferMemory;

	VkUtils::VkBufferBuildInfo buildInfo = {};
	buildInfo.vkSize = bufferSize;
	buildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	buildInfo.vkBuffer = &vkStagingBuffer;
	buildInfo.vkBufferMemory = &vkStagingBufferMemory;

	buildBuffer(buildInfo);

	void* data;
	vkMapMemory(vkMainLogicalDevice, vkStagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vkIndices.data(), bufferSize);
	vkUnmapMemory(vkMainLogicalDevice, vkStagingBufferMemory);

	buildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	buildInfo.vkProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	buildInfo.vkBuffer = &vkIndexBuffer;
	buildInfo.vkBufferMemory = &vkIndexBufferMemory;

	buildBuffer(buildInfo);

	copyBuffer(vkStagingBuffer, vkIndexBuffer, bufferSize);

	vkDestroyBuffer(vkMainLogicalDevice, vkStagingBuffer, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkStagingBufferMemory, nullptr);
}

void VkApp::initVkDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(vkSwapchainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(vkSwapchainImages.size());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();

	poolInfo.maxSets = static_cast<uint32_t>(vkSwapchainImages.size());

	const auto result = vkCreateDescriptorPool(vkMainLogicalDevice, &poolInfo, nullptr, &vkDescriptorPool);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create descriptor pool!");
	}
}

void VkApp::initVkDescriptorSets() {
	std::vector layouts(vkSwapchainImages.size(), vkDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vkDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(vkSwapchainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	vkDescriptorSets.resize(vkSwapchainImages.size());

	if (vkAllocateDescriptorSets(vkMainLogicalDevice, &allocInfo, vkDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (auto i = 0; i < vkSwapchainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = vkUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = vkTextureImageView;
		imageInfo.sampler = vkTextureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = vkDescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = vkDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = vkDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(vkMainLogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void VkApp::initVkDepthResources() {
	const auto depthFormat = findDepthFormat();

	buildImage(vkSwapchainExtent.width, vkSwapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkDepthImage, vkDepthImageMemory);

	buildImageView(vkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, vkDepthImageView);
}

void VkApp::initVkTextureImage() {
	int textureWidht;
	int textureHeight;
	int textureChannels;

	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &textureWidht, &textureHeight, &textureChannels, STBI_rgb_alpha);

	VkDeviceSize vkImageSize = textureWidht * textureHeight * 4;

	if (!pixels) {
		throw std::runtime_error("[STB] Failed to load texture image!");
	}

	VkBuffer vkStagingBuffer;
	VkDeviceMemory vkStagingBufferMemory;

	VkUtils::VkBufferBuildInfo buildInfo = {};
	buildInfo.vkSize = vkImageSize;
	buildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	buildInfo.vkBuffer = &vkStagingBuffer;
	buildInfo.vkBufferMemory = &vkStagingBufferMemory;

	buildBuffer(buildInfo);

	void* data;
	vkMapMemory(vkMainLogicalDevice, vkStagingBufferMemory, 0, vkImageSize, 0, &data);
	memcpy(data, pixels, vkImageSize);
	vkUnmapMemory(vkMainLogicalDevice, vkStagingBufferMemory);

	stbi_image_free(pixels);

	buildImage(textureWidht, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkTextureImage, vkTextureImageMemory);
	transitionVkImageLayout(vkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(vkStagingBuffer, vkTextureImage, static_cast<uint32_t>(textureWidht), static_cast<uint32_t>(textureHeight));
	transitionVkImageLayout(vkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vkMainLogicalDevice, vkStagingBuffer, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkStagingBufferMemory, nullptr);
}

void VkApp::initVkTextureImageView() {
	const auto result = buildImageView(vkTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, vkTextureImageView);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create texture image view!");
	}
}

void VkApp::initVkTextureSampler() {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties = {};
	vkGetPhysicalDeviceProperties(vkMainPhysicalDevice, &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	const auto result = vkCreateSampler(vkMainLogicalDevice, &samplerInfo, nullptr, &vkTextureSampler);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create texture sampler!");
	}
}

void VkApp::initVkUniformBuffers() {
	const auto bufferSize = sizeof(UniformBufferObject);

	vkUniformBuffers.resize(vkSwapchainImages.size());
	vkUniformBufferMemories.resize(vkSwapchainImages.size());
	vkUniformBuffersMapped.resize(vkSwapchainImages.size());

	for (size_t i = 0; i < vkSwapchainImages.size(); i++) {
		VkUtils::VkBufferBuildInfo buildInfo = {};
		buildInfo.vkSize = bufferSize;
		buildInfo.vkUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		buildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		buildInfo.vkBuffer = &vkUniformBuffers[i];
		buildInfo.vkBufferMemory = &vkUniformBufferMemories[i];

		buildBuffer(buildInfo);

		vkMapMemory(vkMainLogicalDevice, vkUniformBufferMemories[i], 0, bufferSize, 0, &vkUniformBuffersMapped[i]);
	}
}

void VkApp::initVkCommandBuffers() {
	vkCommandBuffers.resize(vkSwapchainFramebuffers.size());

	VkCommandBufferAllocateInfo vkCommandAllocateInfo = {};
	vkCommandAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkCommandAllocateInfo.commandPool = vkCommandPool;
	vkCommandAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkCommandAllocateInfo.commandBufferCount = static_cast<uint32_t>(vkCommandBuffers.size());

	const auto result = vkAllocateCommandBuffers(vkMainLogicalDevice, &vkCommandAllocateInfo, vkCommandBuffers.data());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to allocate command buffers!");
	}

	std::cout << "[Vulkan] Command buffer allocation succeeded!\n" << std::flush;
}

void VkApp::initVkSemaphores() {
	vkImageAvailableSemaphores.resize(vkSwapchainFramebuffers.size());
	vkRenderFinishedSemaphores.resize(vkSwapchainFramebuffers.size());

	vkInFlightFences.resize(vkSwapchainFramebuffers.size());

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < vkSwapchainFramebuffers.size(); i++) {
		const auto resultA = vkCreateSemaphore(vkMainLogicalDevice, &semaphoreInfo, nullptr, &vkImageAvailableSemaphores[i]);
		const auto resultB = vkCreateSemaphore(vkMainLogicalDevice, &semaphoreInfo, nullptr, &vkRenderFinishedSemaphores[i]);
		const auto resultC = vkCreateFence(vkMainLogicalDevice, &fenceInfo, nullptr, &vkInFlightFences[i]);

		if (resultA != VK_SUCCESS || resultB != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create semaphore(s)!");
		}

		if (resultC != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create fence(s)!");
		}
	}

	std::cout << "[Vulkan] Semaphore creation succeeded.\n" << std::flush;
}

void VkApp::updateVkUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vkSwapchainExtent.width / static_cast<float>(vkSwapchainExtent.height), 0.1f, 10.0f);

	ubo.proj[1][1] *= -1;

	memcpy(vkUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

bool VkApp::checkVkValidationLayers() {
	uint32_t vkLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);

	std::vector<VkLayerProperties> vkAvailableLayers(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkAvailableLayers.data());

	std::vector<std::string> vkAvailableLayerNames;
	std::ranges::for_each(vkAvailableLayers, [&](VkLayerProperties vkLayer) {
		vkAvailableLayerNames.emplace_back(vkLayer.layerName);
		std::cout << "[Vulkan] Available layer: " << vkLayer.layerName << "\n" << std::flush;
	});

	std::ranges::for_each(vkValidationLayers, [&](const std::string &vkLayerName) {
		if (std::ranges::find(vkAvailableLayerNames, vkLayerName) != vkAvailableLayerNames.end()) {
			std::cout << "[Vulkan] Validation layer verification failed.\n" << std::flush;
			return false;
		}

		return true;
	});

	std::cout << "[Vulkan] Validation layers successfully verified.\n" << std::flush;

	return true;
}

void VkApp::buildDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

VkShaderModule VkApp::buildShaderModule(const std::vector<char>& rawShader) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = rawShader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(rawShader.data());

	VkShaderModule vkShaderModule;

	const auto result = vkCreateShaderModule(vkMainLogicalDevice, &createInfo, nullptr, &vkShaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkna] Failed to create shader module!");
	}

	std::cout << "[Vulkan] Built shader with " << rawShader.size() << " bytes.\n" << std::flush;
	return vkShaderModule;
}

VkResult VkApp::buildBuffer(VkUtils::VkBufferBuildInfo buildInfo) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = buildInfo.vkSize;
	bufferInfo.usage = buildInfo.vkUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	const auto resultA = vkCreateBuffer(vkMainLogicalDevice, &bufferInfo, nullptr, buildInfo.vkBuffer);

	if (resultA != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create buffer!");
	}

	std::cout << "[Vulkan] Buffer initialization succeeded.\n" << std::flush;

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkMainLogicalDevice, *buildInfo.vkBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, buildInfo.vkProperties);

	const auto resultB = vkAllocateMemory(vkMainLogicalDevice, &allocInfo, nullptr, buildInfo.vkBufferMemory);

	if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to allocate buffer memory!");
	}

	std::cout << "[Vulkan] Buffer memory allocation succeeded.\n" << std::flush;

	vkBindBufferMemory(vkMainLogicalDevice, *buildInfo.vkBuffer, *buildInfo.vkBufferMemory, 0);

	return VK_SUCCESS;
}

VkResult VkApp::buildImage(uint32_t vkWidth, uint32_t vkHeight, VkFormat vkFormat, VkImageTiling vkTiling,
 VkImageUsageFlags vkUsage, VkMemoryPropertyFlags vkProperties, VkImage& vkImage, VkDeviceMemory& vkImageMemory) {

	VkImageCreateInfo vkImageCreateInfo = {};
	vkImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	vkImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	vkImageCreateInfo.extent.width = vkWidth;
	vkImageCreateInfo.extent.height = vkHeight;
	vkImageCreateInfo.extent.depth = 1;
	vkImageCreateInfo.mipLevels = 1;
	vkImageCreateInfo.arrayLayers = 1;
	vkImageCreateInfo.format = vkFormat;
	vkImageCreateInfo.tiling = vkTiling;
	vkImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkImageCreateInfo.usage = vkUsage;
	vkImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	vkImageCreateInfo.flags = 0;

	const auto resultA = vkCreateImage(vkMainLogicalDevice, &vkImageCreateInfo, nullptr, &vkImage);

	if (resultA != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create image!");
	}

	VkMemoryRequirements vkMemoryRequirements;
	vkGetImageMemoryRequirements(vkMainLogicalDevice, vkImage, &vkMemoryRequirements);

	VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
	vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
	vkMemoryAllocateInfo.memoryTypeIndex = getMemoryType(vkMemoryRequirements.memoryTypeBits, vkProperties);

	const auto resultB = vkAllocateMemory(vkMainLogicalDevice, &vkMemoryAllocateInfo, nullptr, &vkImageMemory);

	if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to allocate image memory!");
	}

	vkBindImageMemory(vkMainLogicalDevice, vkImage, vkImageMemory, 0);

	return VK_SUCCESS;
}

VkResult VkApp::buildImageView(VkImage vkImage, VkFormat vkFormat, VkImageAspectFlags vkAspect, VkImageView& vkImageView) {
	VkImageViewCreateInfo vkImageViewCreateInfo = {};
	vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	vkImageViewCreateInfo.image = vkImage;
	vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	vkImageViewCreateInfo.format = vkFormat;
	vkImageViewCreateInfo.subresourceRange.aspectMask = vkAspect;
	vkImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	vkImageViewCreateInfo.subresourceRange.levelCount = 1;
	vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	vkImageViewCreateInfo.subresourceRange.layerCount = 1;

	return vkCreateImageView(vkMainLogicalDevice, &vkImageViewCreateInfo, nullptr, &vkImageView);
}

VkResult VkApp::copyBuffer(VkBuffer vkSourceBuffer, VkBuffer vkDestinationBuffer, VkDeviceSize vkBufferSize) {
	executeImmediateCommand([&](VkCommandBuffer commandBuffer) {
		VkBufferCopy vkCopyRegion = {};
		vkCopyRegion.srcOffset = 0;
		vkCopyRegion.dstOffset = 0;
		vkCopyRegion.size = vkBufferSize;

		vkCmdCopyBuffer(commandBuffer, vkSourceBuffer, vkDestinationBuffer, 1, &vkCopyRegion);
	});

	return VK_SUCCESS;
}

VkResult VkApp::copyBufferToImage(VkBuffer vkBuffer, VkImage vkImage, uint32_t vkWidth, uint32_t vkHeight) {
	executeImmediateCommand([&](VkCommandBuffer commandBuffer) {
		VkBufferImageCopy vkRegion = {};
		vkRegion.bufferOffset = 0;
		vkRegion.bufferRowLength = 0;
		vkRegion.bufferImageHeight = 0;
		vkRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkRegion.imageSubresource.mipLevel = 0;
		vkRegion.imageSubresource.baseArrayLayer = 0;
		vkRegion.imageSubresource.layerCount = 1;
		vkRegion.imageOffset = {0, 0, 0};
		vkRegion.imageExtent = {
			vkWidth,
			vkHeight,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			vkBuffer,
			vkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&vkRegion
		);
	});

	return VK_SUCCESS;
}

VkResult VkApp::buildPool(VkUtils::VkCommandPooolBuildInfo buildInfo) {
	const auto result = vkCreateCommandPool(buildInfo.vkLogicalDevice, buildInfo.vkCommandPoolInfo, buildInfo.vkAllocationCallbacks, buildInfo.vkCommandPool);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create command pool!");
	}

	std::cout << "[Vulkan] Command pool initialization succeeded!\n" << std::flush;

	return VK_SUCCESS;
}

VkSurfaceFormatKHR VkApp::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormatsAvailable) {
	const auto it = std::ranges::find_if(vkSurfaceFormatsAvailable, [](const VkSurfaceFormatKHR& vkSurfaceFormatAvailable) {
		return vkSurfaceFormatAvailable.format == VK_FORMAT_B8G8R8A8_SRGB && vkSurfaceFormatAvailable.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	});

	if (it != vkSurfaceFormatsAvailable.end()) {
		std::cout << "[Vulkan] Chose surface format: " << it->format << "\n" << std::flush;
		return *it;
	}

	std::cout << "[Vulkan] Chose surface format: " << vkSurfaceFormatsAvailable[0].format << "\n" << std::flush;

	return vkSurfaceFormatsAvailable[0];
}

VkPresentModeKHR VkApp::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& vkPresentModesAvailable) {
	const auto it = std::ranges::find_if(vkPresentModesAvailable, [](const VkPresentModeKHR& vkPresentModeAvailable) {
		return vkPresentModeAvailable == VK_PRESENT_MODE_MAILBOX_KHR;
	});

	if (it != vkPresentModesAvailable.end()) {
		std::cout << "[Vulkan] Chose present mode: " << *it << "\n" << std::flush;
		return *it;
	}

	std::cout << "[Vulkan] Chose present mode: " << VK_PRESENT_MODE_FIFO_KHR << "\n" << std::flush;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VkApp::selectSwapExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesAvailable) {
	if (vkSurfaceCapabilitiesAvailable.currentExtent.width != UINT32_MAX) {
		std::cout << "[Vulkan] Chose swap dimensions: " << vkSurfaceCapabilitiesAvailable.currentExtent.width << " x " << vkSurfaceCapabilitiesAvailable.currentExtent.height << "\n" << std::flush;
		return vkSurfaceCapabilitiesAvailable.currentExtent;
	}

	int width;
	int height;

	glfwGetFramebufferSize(glfwWindow, &width, &height);

	VkExtent2D vkExtentActual = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	std::cout << "[Vulkan] Chose swap dimensions: " << vkExtentActual.width << " x " << vkExtentActual.height << "\n" << std::flush;

	return vkExtentActual;
}

std::vector<VkExtensionProperties> VkApp::getVkExtensionsAvailable() {
	uint32_t vkExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);

	std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());

	std::cout << "[Vulkan] Available extensions successfully verified.\n" << std::flush;

	return vkExtensions;
}

std::vector<const char*> VkApp::getGlfwExtensionsRequired() {
	uint32_t glfwExtensionCount = 0;

	const char **glfwExtensionsArray = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector glfwExtensions(glfwExtensionsArray, glfwExtensionsArray + glfwExtensionCount);

	if (enableValidationLayers) {
		glfwExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::cout << "[GLFW] Required extensions successfully verified.\n" << std::flush;

	return glfwExtensions;
}

VkUtils::VkSwapChainSupportDetails VkApp::getSwapChainSupport(VkPhysicalDevice vkPhysicalDevice) {
	VkUtils::VkSwapChainSupportDetails vkDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &vkDetails.vkSurfaceCapabilites);

	uint32_t vkSurfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &vkSurfaceFormatCount, nullptr);

	if (vkSurfaceFormatCount != 0) {
		vkDetails.vkSurfaceFormats.resize(vkSurfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &vkSurfaceFormatCount, vkDetails.vkSurfaceFormats.data());
	}

	uint32_t vkPresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &vkPresentModeCount, nullptr);

	if (vkPresentModeCount != 0) {
		vkDetails.vkPresentModes.resize(vkPresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &vkPresentModeCount, vkDetails.vkPresentModes.data());
	}

	return vkDetails;
}

bool VkApp::getExtensionSupport(VkPhysicalDevice vkPhysicalDevice) {
	uint32_t vkExtensionCount;
	vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &vkExtensionCount, nullptr);

	std::vector<VkExtensionProperties> vkAvailableExtensions(vkExtensionCount);
	vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &vkExtensionCount, vkAvailableExtensions.data());

	std::set<std::string> requiredExtensions(vkDeviceExtensions.begin(), vkDeviceExtensions.end());

	std::ranges::for_each(vkAvailableExtensions, [&](VkExtensionProperties vkExtension) {
		requiredExtensions.erase(vkExtension.extensionName);
	});

	if (requiredExtensions.empty()) {
		std::cout << "[Vulkan] Extension support verification succeeded.\n" << std::flush;
		return true;
	}

	std::cout << "[Vulkan] Extension support verification failed.\n" << std::flush;

	return false;
}

uint32_t VkApp::getMemoryType(uint32_t vkTypeFilter, VkMemoryPropertyFlags vkFlagProperties) {
	VkPhysicalDeviceMemoryProperties vkMemProperties;
	vkGetPhysicalDeviceMemoryProperties(vkMainPhysicalDevice, &vkMemProperties);

	for (uint32_t i = 0; i < vkMemProperties.memoryTypeCount; i++) {
		if ((vkTypeFilter & (1 << i)) && (vkMemProperties.memoryTypes[i].propertyFlags & vkFlagProperties) == vkFlagProperties) {
			std::cout << "[Vulkan] Found compatible memory type.\n" << std::flush;
			return i;
		}
	}

	throw std::runtime_error("[Vulkan] Failed to find suitable memory type!");
}

VkUtils::VkQueueFamilyIndices VkApp::getQueueFamilies(VkPhysicalDevice vkPhysicalDevice) {
	VkUtils::VkQueueFamilyIndices vkIndices;

	uint32_t vkQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> vkQueueFamilies(vkQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, vkQueueFamilies.data());

	int i = 0;

	std::ranges::for_each(vkQueueFamilies, [&](VkQueueFamilyProperties vkQueueFamily) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, vkSurface, &presentSupport);

		if (vkQueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) vkIndices.graphicsFamily = i;
		if (presentSupport) vkIndices.presentFamily = i;

		if (vkIndices.isValid()) return false;

		++i;

		return true;
	});

	std::cout << "[Vulkan] Queue family verificaiton finished.\n" << std::flush;

	return vkIndices;
}

std::map<std::string, std::vector<char>> VkApp::getShaders() {
	std::map<std::string, std::vector<char>> shaders;
	std::filesystem::directory_iterator shaderIterator("shaders");

	for (const auto& shaderEntry : shaderIterator) {
		if (shaderEntry.path().extension() == ".spv") {
			std::ifstream shaderFile(shaderEntry.path(), std::ios::ate | std::ios::binary);

			if (!shaderFile.is_open()) {
				throw std::runtime_error("[Vulkan] Failed to load shader file: " + shaderEntry.path().filename().string() + "\n");
			}

			std::vector<char> buffer(shaderEntry.file_size());
			shaderFile.seekg(0);
			shaderFile.read(buffer.data(), shaderEntry.file_size());
			shaderFile.close();
			shaders[shaderEntry.path().filename().string()] = buffer;
			std::cout << "[Vulkan] Loaded shader file: " << shaderEntry.path().filename().string() << "\n" << std::flush;
		}
	}

	return shaders;
}

void VkApp::run() {
	initGlfw();

	VkApi vkApi;
	vkApi.vkApp = this;

	VkUtils::VkTriangle t1  = {
		{{-0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f, 0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}}
	};

	VkUtils::VkTriangle t2 = {
		{{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}}
	};

	vkApi.putTriangle(t1);
	vkApi.putTriangle(t2);

	initVk();
	initImGui();

	loop();
	free();
}

void VkApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = vkRenderPass;
	renderPassInfo.framebuffer = vkSwapchainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = vkSwapchainExtent;

	std::array<VkClearValue, 2> clearValues = { };
	clearValues[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);

	VkBuffer vertexBuffers[] = {vkVertexBuffer};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &vkDescriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vkIndices.size()), 1, 0, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to record command buffer!");
	}
}

VkFormat VkApp::findSupportedFormat(const std::vector<VkFormat> &vkCandidates, VkImageTiling vkTiling, VkFormatFeatureFlags vkFeatures) {
	for (const auto& vkFormat : vkCandidates) {
		VkFormatProperties vkProperties;
		vkGetPhysicalDeviceFormatProperties(vkMainPhysicalDevice, vkFormat, &vkProperties);

		if (vkTiling == VK_IMAGE_TILING_LINEAR && (vkProperties.linearTilingFeatures & vkFeatures) == vkFeatures) {
			return vkFormat;
		}

		if (vkTiling == VK_IMAGE_TILING_OPTIMAL && (vkProperties.optimalTilingFeatures & vkFeatures) == vkFeatures) {
			return vkFormat;
		}
	}

	throw std::runtime_error("[Vulkan] Failed to find supported format!");
}

VkFormat VkApp::findDepthFormat() {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool VkApp::hasStencilComponent(VkFormat vkFormat) {
	return vkFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vkFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VkApp::loadModel() {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<VkUtils::VkVertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			VkUtils::VkVertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = {1.0f, 1.0f, 1.0f};

			vkVertices.push_back(vertex);

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vkVertices.size());
				vkVertices.push_back(vertex);
			}

			vkIndices.push_back(uniqueVertices[vertex]);
		}
	}
}

void VkApp::draw() {
	uint32_t imageIndex;

	vkWaitForFences(vkMainLogicalDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	const auto result = vkAcquireNextImageKHR(vkMainLogicalDevice, vkSwapchain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		resetVkSwapchain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("[Vulkan] Failed to acquire swap chain image!");
	}

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::Button("Meow", ImVec2(50, 50));

	ImGui::Render();

	vkWaitForFences(vkMainLogicalDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetCommandBuffer(vkCommandBuffers[currentFrame], 0);

	recordCommandBuffer(vkCommandBuffers[currentFrame], imageIndex);

	updateVkUniformBuffer(currentFrame);

	vkResetFences(vkMainLogicalDevice, 1, &vkInFlightFences[currentFrame]);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {vkImageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = {vkRenderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, vkInFlightFences[currentFrame]);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {vkSwapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	const auto resultB = vkQueuePresentKHR(vkPresentQueue, &presentInfo);

	if (resultB == VK_ERROR_OUT_OF_DATE_KHR || resultB == VK_SUBOPTIMAL_KHR || vkFramebufferResized) {
		vkFramebufferResized = false;

		resetVkSwapchain();
	} else if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to present swap chain image!");
	}


	vkQueueWaitIdle(vkPresentQueue);

	currentFrame = (currentFrame + 1) % (vkSwapchainFramebuffers.size());
}

void VkApp::loop() {
	while (!glfwWindowShouldClose(glfwWindow)) {
		glfwPollEvents();

		draw();
	}
}

void VkApp::free() {
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	freeVkSwapchain();

	vkDestroySampler(vkMainLogicalDevice, vkTextureSampler, nullptr);

	vkDestroyImageView(vkMainLogicalDevice, vkTextureImageView, nullptr);

	vkDestroyImage(vkMainLogicalDevice, vkTextureImage, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkTextureImageMemory, nullptr);

	vkDestroyImageView(vkMainLogicalDevice, vkDepthImageView, nullptr);

	vkDestroyImage(vkMainLogicalDevice, vkDepthImage, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkDepthImageMemory, nullptr);

	vkDestroyDescriptorPool(vkMainLogicalDevice, vkDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(vkMainLogicalDevice, vkDescriptorSetLayout, nullptr);

	vkDestroyBuffer(vkMainLogicalDevice, vkVertexBuffer, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkVertexBufferMemory, nullptr);

	vkDestroyBuffer(vkMainLogicalDevice, vkIndexBuffer, nullptr);
	vkFreeMemory(vkMainLogicalDevice, vkIndexBufferMemory, nullptr);

	std::ranges::for_each(vkImageAvailableSemaphores, [&](VkSemaphore vkSemaphore) {
		vkDestroySemaphore(vkMainLogicalDevice, vkSemaphore, nullptr);
	});

	std::ranges::for_each(vkRenderFinishedSemaphores, [&](VkSemaphore vkSemaphore) {
		vkDestroySemaphore(vkMainLogicalDevice, vkSemaphore, nullptr);
	});

	std::ranges::for_each(vkInFlightFences, [&](VkFence vkFence) {
		vkDestroyFence(vkMainLogicalDevice, vkFence, nullptr);
	});

	std::ranges::for_each(vkUniformBuffers, [&](VkBuffer vkBuffer) {
		vkDestroyBuffer(vkMainLogicalDevice, vkBuffer, nullptr);
	});

	// TODO: Destroy command buffers?

	vkDestroyCommandPool(vkMainLogicalDevice, vkCommandPool, nullptr);

	vkDestroyDevice(vkMainLogicalDevice, nullptr);

	if (enableValidationLayers) {
		VkUtils::destroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
	vkDestroyInstance(vkInstance, nullptr);

	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

void VkApp::freeVkSwapchain() {
	for (VkFramebuffer vkFramebuffer : vkSwapchainFramebuffers) {
		vkDestroyFramebuffer(vkMainLogicalDevice, vkFramebuffer, nullptr);
	}

	for (VkImageView vkImageView : vkSwapchainImageViews) {
		vkDestroyImageView(vkMainLogicalDevice, vkImageView, nullptr);
	}

	vkDestroySwapchainKHR(vkMainLogicalDevice, vkSwapchain, nullptr);
}

void VkApp::resetVkSwapchain() {
	int width = 0, height = 0;

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(glfwWindow, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vkMainLogicalDevice);

	freeVkSwapchain();

	initVkSwapchain();
	initVkImageViews();
	initVkDepthResources();
	initVkFramebuffers();
}

void VkApp::vkApiPutTriangle(VkUtils::VkTriangle& triangle) {
	for (auto vkVertex : { triangle.vA, triangle.vB, triangle.vC }) {
		vkVertices.push_back(vkVertex);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	std::cerr << "Validation layer: " << callbackData->pMessage << std::endl;

	return VK_FALSE;
}
