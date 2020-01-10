#pragma once
#include "VkApp.hpp"

void VkApp::initGlfw() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindow = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, NAME, nullptr, nullptr);
	glfwSetWindowUserPointer(glfwWindow, this);

	glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* window, int width, int height) {
		VkApp* application = reinterpret_cast<VkApp*>(glfwGetWindowUserPointer(window));
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
	initVkGraphicsPipeline();
	initVkFramebuffers();
	initVkCommandPool();
	initVkCommandBuffers();
	initVkSemaphores();
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
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	std::vector<const char*> glfwExtensionsRequired = getGlfwExtensionsRequired();
	std::cout << "[GLFW] Required extensions:\n";
	std::for_each(glfwExtensionsRequired.begin(), glfwExtensionsRequired.end(), [](const char* glfwExtension) {
		std::cout << "\t - " << glfwExtension << "\n";
	});

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensionsRequired.size());
	createInfo.ppEnabledExtensionNames = glfwExtensionsRequired.data();

	std::vector<VkExtensionProperties> vkExtensionsRequired = getVkExtensionsAvailable();
	std::cout << "[Vulkan] Available extensions:\n";
	std::for_each(vkExtensionsRequired.begin(), vkExtensionsRequired.end(), [](VkExtensionProperties vkExtension) {
		std::cout << "\t - " << vkExtension.extensionName << "\n";
	});

	VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkInstance!");
	}
	else {
		std::cout << "[Vulkan] Initialized instance.\n";
	}
}

void VkApp::initVkDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	buildDebugMessengerCreateInfo(createInfo);


	if (VkUtils::createDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger!");
	}
	else {
		std::cout << "[Vulkan] Initialized debug messenger.\n";
	}
}

void VkApp::initVkSurface() {
	VkResult result = glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &vkSurface);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
	else {
		std::cout << "[Vulkan] Initialized surface.\n";
	}
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
			return getQueueFamilies(vkPhysicalDevice).isValid() && getExtensionSupport(vkPhysicalDevice) && getSwapChainSupport(vkPhysicalDevice).isValid();
		}()) {
			vkMainPhysicalDevice = vkPhysicalDevice;

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(vkMainPhysicalDevice, &deviceProperties);

			std::cout << "[Vulkan] Physical device selected: " << deviceProperties.deviceName << "\n";

			return false;
		};
	});

	std::cout << "[Vulkan] Initialized physical device.\n";
}

void VkApp::initVkLogicalDevice() {
	VkUtils::VkQueueFamilyIndices indices = getQueueFamilies(vkMainPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(vkMainPhysicalDevice, &createInfo, nullptr, &vkMainLogicalDevice);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(vkMainLogicalDevice, indices.graphicsFamily.value(), 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkMainLogicalDevice, indices.presentFamily.value(), 0, &vkPresentQueue);

	std::cout << "[Vulkan] Initialized logical device.\n";
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
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = vkDetails.vkSurfaceCapabilites.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = vkPresentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(vkMainLogicalDevice, &createInfo, nullptr, &vkSwapchain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create swap chain!");
	}
	else {
		std::cout << "[Vulkan] Initialized swapchain.\n";
	}

	vkGetSwapchainImagesKHR(vkMainLogicalDevice, vkSwapchain, &vkMinImageCount, nullptr);
	vkSwapchainImages.resize(vkMinImageCount);
	vkGetSwapchainImagesKHR(vkMainLogicalDevice, vkSwapchain, &vkMinImageCount, vkSwapchainImages.data());

	vkSwapchainImageFormat = vkSurfaceFormat.format;
	vkSwapchainExtent = vkExtent;
}

void VkApp::initVkImageViews() {
	vkSwapchainImageViews.resize(vkSwapchainImages.size());

	int i = 0;
	std::for_each(vkSwapchainImages.begin(), vkSwapchainImages.end(), [&](VkImage vkImage) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = vkSwapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = vkSwapchainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(vkMainLogicalDevice, &createInfo, nullptr, &vkSwapchainImageViews[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create image views!");
		}

		++i;
	});

	std::cout << "[Vulkan] Image view creation successfull.\n";
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

	VkAttachmentReference vkColorAttachmentReference = {};
	vkColorAttachmentReference.attachment = 0;
	vkColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &vkColorAttachmentReference;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	VkRenderPassCreateInfo vkRenderPassInfo = {};
	vkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	vkRenderPassInfo.attachmentCount = 1;
	vkRenderPassInfo.pAttachments = &vkColorAttachmentDescription;
	vkRenderPassInfo.subpassCount = 1;
	vkRenderPassInfo.pSubpasses = &subpass;
	vkRenderPassInfo.dependencyCount = 1;
	vkRenderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(vkMainLogicalDevice, &vkRenderPassInfo, nullptr, &vkRenderPass);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create render pass!");
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
	vkVertexInputInfo.vertexBindingDescriptionCount = 0;
	vkVertexInputInfo.pVertexBindingDescriptions = nullptr;
	vkVertexInputInfo.vertexAttributeDescriptionCount = 0;
	vkVertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
	vkRasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

	VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
	vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkPipelineLayoutInfo.setLayoutCount = 0;
	vkPipelineLayoutInfo.pSetLayouts = nullptr;
	vkPipelineLayoutInfo.pushConstantRangeCount = 0;
	vkPipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult resultA = vkCreatePipelineLayout(vkMainLogicalDevice, &vkPipelineLayoutInfo, nullptr, &vkPipelineLayout);

	if (resultA != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create pipeline layout!");
	}
	else {
		std::cout << "[Vulkan] Pipeline layout initialization succeeded.\n";
	}

	VkGraphicsPipelineCreateInfo vkPipelineInfo = {};
	vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkPipelineInfo.stageCount = 2;
	vkPipelineInfo.pStages = vkShaderStages;
	vkPipelineInfo.pVertexInputState = &vkVertexInputInfo;
	vkPipelineInfo.pInputAssemblyState = &vkInputAssemblyInfo;
	vkPipelineInfo.pViewportState = &vkViewportStateInfo;
	vkPipelineInfo.pRasterizationState = &vkRasterizerInfo;
	vkPipelineInfo.pMultisampleState = &vkMultisamplingInfo;
	vkPipelineInfo.pDepthStencilState = nullptr;
	vkPipelineInfo.pColorBlendState = &vkColorBlendingInfo;
	vkPipelineInfo.pDynamicState = nullptr;
	vkPipelineInfo.layout = vkPipelineLayout;
	vkPipelineInfo.renderPass = vkRenderPass;
	vkPipelineInfo.subpass = 0;
	vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	vkPipelineInfo.basePipelineIndex = -1;

	VkResult resultB = vkCreateGraphicsPipelines(vkMainLogicalDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, nullptr, &vkGraphicsPipeline);

	if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create graphics pipeline!");
	}
	else {
		std::cout << "[Vulkan] Pipeline initialization succeeded.\n";
	}

	vkDestroyShaderModule(vkMainLogicalDevice, vkFragShaderModule, nullptr);
	vkDestroyShaderModule(vkMainLogicalDevice, vkVertShaderModule, nullptr);
}

void VkApp::initVkFramebuffers() {
	vkSwapchainFramebuffers.resize(vkSwapchainImageViews.size());

	int i = 0;
	std::for_each(vkSwapchainImageViews.begin(), vkSwapchainImageViews.end(), [&](VkImageView vkImageView) {
		VkImageView vkAttachments[] = {
			vkImageView
		};

		VkFramebufferCreateInfo vkFramebufferInfo = {};
		vkFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		vkFramebufferInfo.renderPass = vkRenderPass;
		vkFramebufferInfo.attachmentCount = 1;
		vkFramebufferInfo.pAttachments = vkAttachments;
		vkFramebufferInfo.width = vkSwapchainExtent.width;
		vkFramebufferInfo.height = vkSwapchainExtent.height;
		vkFramebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(vkMainLogicalDevice, &vkFramebufferInfo, nullptr, &vkSwapchainFramebuffers[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create framebuffer!");
		}

		++i;
	});

	std::cout << "[Vulkan] Framebuffer initialization succeeded.\n";
}

void VkApp::initVkCommandPool() {
	VkUtils::VkQueueFamilyIndices queueFamilyIndices = getQueueFamilies(vkMainPhysicalDevice);

	VkCommandPoolCreateInfo vkCommandPoolInfo = {};
	vkCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	vkCommandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	vkCommandPoolInfo.flags = 0;

	VkResult result = vkCreateCommandPool(vkMainLogicalDevice, &vkCommandPoolInfo, nullptr, &vkCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create command pool!");
	}
	else {
		std::cout << "[Vulkan] Command pool initialization succeeded!\n";
	}
}

void VkApp::initVkCommandBuffers() {
	vkCommandBuffers.resize(vkSwapchainFramebuffers.size());

	VkCommandBufferAllocateInfo vkCommandAllocateInfo = {};
	vkCommandAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkCommandAllocateInfo.commandPool = vkCommandPool;
	vkCommandAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkCommandAllocateInfo.commandBufferCount = (uint32_t)vkCommandBuffers.size();

	VkResult result = vkAllocateCommandBuffers(vkMainLogicalDevice, &vkCommandAllocateInfo, vkCommandBuffers.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to allocate command buffers!");
	}
	else {
		std::cout << "[Vulkan] Command buffer allocation succeeded!\n";
	}

	int i = 0;
	std::for_each(vkCommandBuffers.begin(), vkCommandBuffers.end(), [&](VkCommandBuffer vkCommandBuffer) {
		VkCommandBufferBeginInfo vkCommandBufferInfo = {};
		vkCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkCommandBufferInfo.flags = 0;
		vkCommandBufferInfo.pInheritanceInfo = nullptr;

		VkResult resultA = vkBeginCommandBuffer(vkCommandBuffers[i], &vkCommandBufferInfo);

		if (resultA != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo vkRenderPassInfo = {};
		vkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkRenderPassInfo.renderPass = vkRenderPass;
		vkRenderPassInfo.framebuffer = vkSwapchainFramebuffers[i];
		vkRenderPassInfo.renderArea.offset = { 0, 0 };
		vkRenderPassInfo.renderArea.extent = vkSwapchainExtent;

		VkClearValue vkClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		vkRenderPassInfo.clearValueCount = 1;
		vkRenderPassInfo.pClearValues = &vkClearColor;

		vkCmdBeginRenderPass(vkCommandBuffers[i], &vkRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);
		vkCmdDraw(vkCommandBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(vkCommandBuffers[i]);

		VkResult resultB = vkEndCommandBuffer(vkCommandBuffers[i]);

		if (resultB != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to record command buffer!");
		}
		++i;
	});

	std::cout << "[Vulkan] Command buffer initialization succeeded!\n";
}

void VkApp::initVkSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult resultA = vkCreateSemaphore(vkMainLogicalDevice, &semaphoreInfo, nullptr, &vkImageAvailableSemaphore);
	VkResult resultB = vkCreateSemaphore(vkMainLogicalDevice, &semaphoreInfo, nullptr, &vkRenderFinishedSemaphore);

	if (resultA != VK_SUCCESS || resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create semaphore(s)!");
	} else {
		std::cout << "[Vulkan] Semaphore creation succeeded.\n";
	}
}

bool VkApp::checkVkValidationLayers() {
	uint32_t vkLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);

	std::vector<VkLayerProperties> vkAvailableLayers(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkAvailableLayers.data());

	std::vector<const char*> vkAvailableLayerNames;
	std::for_each(vkAvailableLayers.begin(), vkAvailableLayers.end(), [&](VkLayerProperties vkLayer) {
		vkAvailableLayerNames.push_back(vkLayer.layerName);
	});

	std::for_each(vkValidationLayers.begin(), vkValidationLayers.end(), [&](const char* vkLayerName) {
		if (std::find(vkAvailableLayerNames.begin(), vkAvailableLayerNames.end(), vkLayerName) != vkAvailableLayerNames.end()) {
			std::cout << "[Vulkan] Validation layer verification failed.\n";
			return false;
		}
	});

	std::cout << "[Vulkan] Validation layers successfully verified.\n";

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

	VkResult result = vkCreateShaderModule(vkMainLogicalDevice, &createInfo, nullptr, &vkShaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkna] Failed to create shader module!");
	}
	else {
		std::cout << "[Vulkan] Built shader with " << rawShader.size() << " bytes.\n";
		return vkShaderModule;
	}

}

VkSurfaceFormatKHR VkApp::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormatsAvailable) {
	std::for_each(vkSurfaceFormatsAvailable.begin(), vkSurfaceFormatsAvailable.end(), [&](VkSurfaceFormatKHR vkSurfaceFormatAvailable) {
		if (vkSurfaceFormatAvailable.format == VK_FORMAT_B8G8R8A8_UNORM && vkSurfaceFormatAvailable.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			std::cout << "[Vulkan] Chose surface format: " << vkSurfaceFormatAvailable.format << "\n";
			return vkSurfaceFormatAvailable;
		}
	});

	std::cout << "[Vulkan] Chose surface format: " << vkSurfaceFormatsAvailable[0].format << "\n";
	return vkSurfaceFormatsAvailable[0];
}

VkPresentModeKHR VkApp::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& vkPresentModesAvailable) {
	std::for_each(vkPresentModesAvailable.begin(), vkPresentModesAvailable.end(), [&](VkPresentModeKHR vkPresentModeAvailable) {
		if (vkPresentModeAvailable == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			std::cout << "[Vulkan] Chose present mode: " << vkPresentModeAvailable << "\n";
			return vkPresentModeAvailable;
		}
	});

	std::cout << "[Vulkan] Chose present mode: " << VK_PRESENT_MODE_FIFO_KHR << "\n";
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VkApp::selectSwapExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesAvailable) {
	if (vkSurfaceCapabilitiesAvailable.currentExtent.width != UINT32_MAX) {
		std::cout << "[Vulkan] Chose swap dimensions: " << vkSurfaceCapabilitiesAvailable.currentExtent.width << " x " << vkSurfaceCapabilitiesAvailable.currentExtent.height << "\n";
		return vkSurfaceCapabilitiesAvailable.currentExtent;
	}
	else {
		int width;
		int height;

		glfwGetFramebufferSize(glfwWindow, &width, &height);

		VkExtent2D vkExtentActual = { width, height };

		std::cout << "[Vulkan] Chose swap dimensions: " << vkExtentActual.width << " x " << vkExtentActual.height << "\n";
		return vkExtentActual;
	}
}

std::vector<VkExtensionProperties> VkApp::getVkExtensionsAvailable() {
	uint32_t vkExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);

	std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());

	std::cout << "[Vulkan] Available extensions successfully verified.\n";

	return vkExtensions;
}

std::vector<const char*> VkApp::getGlfwExtensionsRequired() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensionsArray;

	glfwExtensionsArray = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> glfwExtensions(glfwExtensionsArray, glfwExtensionsArray + glfwExtensionCount);

	if (enableValidationLayers) {
		glfwExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::cout << "[GLFW] Required extensions successfully verified.\n";

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

	std::for_each(vkAvailableExtensions.begin(), vkAvailableExtensions.end(), [&](VkExtensionProperties vkExtension) {
		requiredExtensions.erase(vkExtension.extensionName);
	});

	if (requiredExtensions.empty()) {
		std::cout << "[Vulkan] Extension support verification succeeded.\n";
		return true;
	}
	else {
		std::cout << "[Vulkan] Extension support verification failed.\n";
		return false;
	}
}

VkUtils::VkQueueFamilyIndices VkApp::getQueueFamilies(VkPhysicalDevice vkPhysicalDevice) {
	VkUtils::VkQueueFamilyIndices vkIndices;

	uint32_t vkQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> vkQueueFamilies(vkQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, vkQueueFamilies.data());

	int i = 0;
	std::for_each(vkQueueFamilies.begin(), vkQueueFamilies.end(), [&](VkQueueFamilyProperties vkQueueFamily) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, vkSurface, &presentSupport);

		if (vkQueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) vkIndices.graphicsFamily = i;
		if (presentSupport) vkIndices.presentFamily = i;

		if (vkIndices.isValid()) return false;

		++i;
	});

	std::cout << "[Vulkan] Queue family verificaiton finished.\n";
	return vkIndices;
}

std::map<std::string, std::vector<char>> VkApp::getShaders() {
	std::map<std::string, std::vector<char>> shaders;
	std::filesystem::directory_iterator shaderIterator("C:\\Users\\vini2003\\source\\repos\\VukVuk\\VukVuk\\shaders");
	for (std::filesystem::directory_entry shaderEntry : shaderIterator) {
		if (shaderEntry.path().extension() == ".spv") {
			std::ifstream shaderFile(shaderEntry.path(), std::ios::ate | std::ios::binary);

			if (!shaderFile.is_open()) {
				throw std::runtime_error("[Vulkan] Failed to load shader file: " + shaderEntry.path().filename().string() + "\n");
			}
			else {
				std::vector<char> buffer(shaderEntry.file_size());
				shaderFile.seekg(0);
				shaderFile.read(buffer.data(), shaderEntry.file_size());
				shaderFile.close();
				shaders[shaderEntry.path().filename().string()] = buffer;
				std::cout << "[Vulkan] Loaded shader file: " << shaderEntry.path().filename().string() << "\n";
			}
		}
	}
	return shaders;
}

void VkApp::run() {
	initGlfw();
	initVk();
	loop();
	free();
}


void VkApp::draw() {
	uint32_t vkImageIndex;
	VkResult resultA = vkAcquireNextImageKHR(vkMainLogicalDevice, vkSwapchain, UINT64_MAX, vkImageAvailableSemaphore, VK_NULL_HANDLE, &vkImageIndex);

	if (resultA == VK_ERROR_OUT_OF_DATE_KHR || resultA == VK_SUBOPTIMAL_KHR || vkFramebufferResized) {
		vkFramebufferResized = !vkFramebufferResized;
		resetVkSwapchain();
		return;
	} else if (resultA != VK_SUCCESS && resultA != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("[Vulkan] Failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore vkWaitSemaphores[] = { vkImageAvailableSemaphore };
	VkPipelineStageFlags vkWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = vkWaitSemaphores;
	submitInfo.pWaitDstStageMask = vkWaitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffers[vkImageIndex];

	VkSemaphore vkSignalSemaphores[] = { vkRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = vkSignalSemaphores;

	VkResult result = vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.pWaitSemaphores = vkSignalSemaphores;
	presentInfo.waitSemaphoreCount = 1;

	VkSwapchainKHR swapChains[] = { vkSwapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &vkImageIndex;

	presentInfo.pResults = nullptr;

	VkResult resultB = vkQueuePresentKHR(vkPresentQueue, &presentInfo);

	if (resultB == VK_ERROR_OUT_OF_DATE_KHR) {
		resetVkSwapchain();
		return;
	} else if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to present swap chain image!");
	}

	vkQueueWaitIdle(vkPresentQueue);
}

void VkApp::loop() {
	while (!glfwWindowShouldClose(glfwWindow)) {
		glfwPollEvents();
		draw();
	}
}

void VkApp::free() {
	freeVkSwapchain();

	vkDestroySemaphore(vkMainLogicalDevice, vkRenderFinishedSemaphore, nullptr);
	vkDestroySemaphore(vkMainLogicalDevice, vkImageAvailableSemaphore, nullptr);

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

	vkFreeCommandBuffers(vkMainLogicalDevice, vkCommandPool, static_cast<uint32_t>(vkCommandBuffers.size()), vkCommandBuffers.data());
	
	vkDestroyPipeline(vkMainLogicalDevice, vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(vkMainLogicalDevice, vkPipelineLayout, nullptr);
	vkDestroyRenderPass(vkMainLogicalDevice, vkRenderPass, nullptr);

	for (VkImageView vkImageView : vkSwapchainImageViews) {
		vkDestroyImageView(vkMainLogicalDevice, vkImageView, nullptr);
	}

	vkDestroySwapchainKHR(vkMainLogicalDevice, vkSwapchain, nullptr);
}

void VkApp::resetVkSwapchain() {
	for (int width = 0, height = 0; width == 0 || height == 0; glfwGetFramebufferSize(glfwWindow, &width, &height)) {
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vkMainLogicalDevice);

	freeVkSwapchain();

	initVkSwapchain();
	initVkImageViews();
	initVkRenderPass();
	initVkGraphicsPipeline();
	initVkFramebuffers();
	initVkCommandBuffers();
	initVkSemaphores();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	std::cerr << "Validation layer: " << callbackData->pMessage << std::endl;

	return VK_FALSE;
}

