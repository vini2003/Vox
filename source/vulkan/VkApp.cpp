#pragma once

#include <cstring>

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
	initVkFramebuffers();
	initVkCommandPools();
	initVkVertexBuffer();
	initVkIndexBuffer();
	initVkUniformBuffers();
	initVkDescriptorPool();
	initVkDescriptorSets();
	initVkCommandBuffers();
	initVkSemaphores();
}

void VkApp::executeImmediateCommand(std::function<void(VkCommandBuffer cmd)> &&function) {
	// Allocate a temporary command buffer
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vkCommandPoolShort; // Assuming this is a pool for transient command buffers
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vkMainLogicalDevice, &allocInfo, &commandBuffer);

	// Begin command buffer recording
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	// Execute the provided function
	function(commandBuffer);

	// End command buffer recording
	vkEndCommandBuffer(commandBuffer);

	// Submit the command buffer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkGraphicsQueue);

	// Cleanup
	vkFreeCommandBuffers(vkMainLogicalDevice, vkCommandPoolShort, 1, &commandBuffer);
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

	std::cout << "[Vulkan] ImGui descriptor pool creation succeeded.\n";

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

	std::cout << "[GLFW] Required extensions:\n";
	std::ranges::for_each(glfwExtensionsRequired, [](const char* glfwExtension) {
		std::cout << "\t - " << glfwExtension << "\n";
	});

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensionsRequired.size());
	createInfo.ppEnabledExtensionNames = glfwExtensionsRequired.data();

	std::vector<VkExtensionProperties> vkExtensionsRequired = getVkExtensionsAvailable();

	std::cout << "[Vulkan] Available extensions:\n";
	std::ranges::for_each(vkExtensionsRequired, [](const VkExtensionProperties &vkExtension) {
		std::cout << "\t - " << vkExtension.extensionName << "\n";
	});

	const auto result = vkCreateInstance(&createInfo, nullptr, &vkInstance);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkInstance!");
	}

	std::cout << "[Vulkan] Initialized instance.\n";
}

void VkApp::initVkDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	buildDebugMessengerCreateInfo(createInfo);

	const auto result = VkUtils::createDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger!");
	}

	std::cout << "[Vulkan] Initialized debug messenger.\n";
}

void VkApp::initVkSurface() {
	const auto result = glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &vkSurface);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}

	std::cout << "[Vulkan] Initialized surface.\n";
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
	} else {
		createInfo.enabledLayerCount = 0;
	}

	const auto result = vkCreateDevice(vkMainPhysicalDevice, &createInfo, nullptr, &vkMainLogicalDevice);

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

	std::cout << "[Vulkan] Initialized swapchain.\n";

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

		const auto result = vkCreateImageView(vkMainLogicalDevice, &createInfo, nullptr, &vkSwapchainImageViews[i]);

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

	VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
	vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	vkDescriptorSetLayoutCreateInfo.bindingCount = 1;
	vkDescriptorSetLayoutCreateInfo.pBindings = &vkUboLayoutBinding;

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

	VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
	vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkPipelineLayoutInfo.setLayoutCount = 1;
	vkPipelineLayoutInfo.pSetLayouts = &vkDescriptorSetLayout;

	const auto resultA = vkCreatePipelineLayout(vkMainLogicalDevice, &vkPipelineLayoutInfo, nullptr, &vkPipelineLayout);

	if (resultA != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create pipeline layout!");
	}

	std::cout << "[Vulkan] Pipeline layout initialization succeeded.\n";

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

	const auto resultB = vkCreateGraphicsPipelines(vkMainLogicalDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, nullptr, &vkGraphicsPipeline);

	if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create graphics pipeline!");
	}

	std::cout << "[Vulkan] Pipeline initialization succeeded.\n";

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

		const auto result = vkCreateFramebuffer(vkMainLogicalDevice, &vkFramebufferInfo, nullptr, &vkSwapchainFramebuffers[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to create framebuffer!");
		}

		++i;
	});

	std::cout << "[Vulkan] Framebuffer initialization succeeded.\n";
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
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(vkSwapchainImages.size());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

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

	for (size_t i = 0; i < vkSwapchainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vkUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = vkDescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(vkMainLogicalDevice, 1, &descriptorWrite, 0, nullptr);
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

	std::cout << "[Vulkan] Command buffer allocation succeeded!\n";
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

	std::cout << "[Vulkan] Semaphore creation succeeded.\n";
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
		std::cout << "[Vulkan] Available layer: " << vkLayer.layerName << "\n";
	});

	std::ranges::for_each(vkValidationLayers, [&](const std::string &vkLayerName) {
		if (std::ranges::find(vkAvailableLayerNames, vkLayerName) != vkAvailableLayerNames.end()) {
			std::cout << "[Vulkan] Validation layer verification failed.\n";
			return false;
		}

		return true;
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

	const auto result = vkCreateShaderModule(vkMainLogicalDevice, &createInfo, nullptr, &vkShaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkna] Failed to create shader module!");
	}

	std::cout << "[Vulkan] Built shader with " << rawShader.size() << " bytes.\n";
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
		throw std::runtime_error("[Vulkan] Failed to create vertex buffer!");
	}

	std::cout << "[Vulkan] Vertex buffer initialization succeeded.\n";

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkMainLogicalDevice, *buildInfo.vkBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, buildInfo.vkProperties);

	const auto resultB = vkAllocateMemory(vkMainLogicalDevice, &allocInfo, nullptr, buildInfo.vkBufferMemory);

	if (resultB != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to allocate vertex buffer memory!");
	}

	std::cout << "[Vulkan] Vertex buffer memory allocation succeeded.\n";

	vkBindBufferMemory(vkMainLogicalDevice, *buildInfo.vkBuffer, *buildInfo.vkBufferMemory, 0);

	return VK_SUCCESS;
}

VkResult VkApp::copyBuffer(VkBuffer vkSourceBuffer, VkBuffer vkDestinationBuffer, VkDeviceSize vkBufferSize) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vkCommandPoolShort;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer vkTempBuffer;
	vkAllocateCommandBuffers(vkMainLogicalDevice, &allocInfo, &vkTempBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(vkTempBuffer, &beginInfo);

	VkBufferCopy vkCopyRegion = {};
	vkCopyRegion.srcOffset = 0;
	vkCopyRegion.dstOffset = 0;
	vkCopyRegion.size = vkBufferSize;
	vkCmdCopyBuffer(vkTempBuffer, vkSourceBuffer, vkDestinationBuffer, 1, &vkCopyRegion);

	vkEndCommandBuffer(vkTempBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkTempBuffer;

	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkGraphicsQueue);

	vkFreeCommandBuffers(vkMainLogicalDevice, vkCommandPoolShort, 1, &vkTempBuffer);

	return VK_SUCCESS;
}

VkResult VkApp::buildPool(VkUtils::VkCommandPooolBuildInfo buildInfo) {
	const auto result = vkCreateCommandPool(buildInfo.vkLogicalDevice, buildInfo.vkCommandPoolInfo, buildInfo.vkAllocationCallbacks, buildInfo.vkCommandPool);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to create command pool!");
	}

	std::cout << "[Vulkan] Command pool initialization succeeded!\n";

	return VK_SUCCESS;
}

VkSurfaceFormatKHR VkApp::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormatsAvailable) {
	const auto it = std::ranges::find_if(vkSurfaceFormatsAvailable, [](const VkSurfaceFormatKHR& vkSurfaceFormatAvailable) {
		return vkSurfaceFormatAvailable.format == VK_FORMAT_B8G8R8A8_UNORM && vkSurfaceFormatAvailable.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	});

	if (it != vkSurfaceFormatsAvailable.end()) {
		std::cout << "[Vulkan] Chose surface format: " << it->format << "\n";
		return *it;
	}

	std::cout << "[Vulkan] Chose surface format: " << vkSurfaceFormatsAvailable[0].format << "\n";

	return vkSurfaceFormatsAvailable[0];
}

VkPresentModeKHR VkApp::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& vkPresentModesAvailable) {
	const auto it = std::ranges::find_if(vkPresentModesAvailable, [](const VkPresentModeKHR& vkPresentModeAvailable) {
		return vkPresentModeAvailable == VK_PRESENT_MODE_MAILBOX_KHR;
	});

	if (it != vkPresentModesAvailable.end()) {
		std::cout << "[Vulkan] Chose present mode: " << *it << "\n";
		return *it;
	}

	std::cout << "[Vulkan] Chose present mode: " << VK_PRESENT_MODE_FIFO_KHR << "\n";

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VkApp::selectSwapExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesAvailable) {
	if (vkSurfaceCapabilitiesAvailable.currentExtent.width != UINT32_MAX) {
		std::cout << "[Vulkan] Chose swap dimensions: " << vkSurfaceCapabilitiesAvailable.currentExtent.width << " x " << vkSurfaceCapabilitiesAvailable.currentExtent.height << "\n";
		return vkSurfaceCapabilitiesAvailable.currentExtent;
	}

	int width;
	int height;

	glfwGetFramebufferSize(glfwWindow, &width, &height);

	VkExtent2D vkExtentActual = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	std::cout << "[Vulkan] Chose swap dimensions: " << vkExtentActual.width << " x " << vkExtentActual.height << "\n";

	return vkExtentActual;
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

	const char **glfwExtensionsArray = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector glfwExtensions(glfwExtensionsArray, glfwExtensionsArray + glfwExtensionCount);

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

	std::ranges::for_each(vkAvailableExtensions, [&](VkExtensionProperties vkExtension) {
		requiredExtensions.erase(vkExtension.extensionName);
	});

	if (requiredExtensions.empty()) {
		std::cout << "[Vulkan] Extension support verification succeeded.\n";
		return true;
	}

	std::cout << "[Vulkan] Extension support verification failed.\n";

	return false;
}

uint32_t VkApp::getMemoryType(uint32_t vkTypeFilter, VkMemoryPropertyFlags vkFlagProperties) {
	VkPhysicalDeviceMemoryProperties vkMemProperties;
	vkGetPhysicalDeviceMemoryProperties(vkMainPhysicalDevice, &vkMemProperties);

	for (uint32_t i = 0; i < vkMemProperties.memoryTypeCount; i++) {
		if ((vkTypeFilter & (1 << i)) && (vkMemProperties.memoryTypes[i].propertyFlags & vkFlagProperties) == vkFlagProperties) {
			std::cout << "[Vulkan] Found compatible memory type.\n";
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

	std::cout << "[Vulkan] Queue family verificaiton finished.\n";

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
			std::cout << "[Vulkan] Loaded shader file: " << shaderEntry.path().filename().string() << "\n";
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

	VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);

	VkBuffer vertexBuffers[] = {vkVertexBuffer};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &vkDescriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vkIndices.size()), 1, 0, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to record command buffer!");
	}
}

void VkApp::draw() {
	uint32_t imageIndex;

	const auto result = vkAcquireNextImageKHR(vkMainLogicalDevice, vkSwapchain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to acquire swap chain image!");
	}

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::Button("Meow", ImVec2(50, 50));

	ImGui::Render();

	vkResetCommandBuffer(vkCommandBuffers[currentFrame], 0);
	recordCommandBuffer(vkCommandBuffers[currentFrame], imageIndex);

	updateVkUniformBuffer(currentFrame);

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

	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {vkSwapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(vkPresentQueue, &presentInfo);
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

void VkApp::vkApiPutTriangle(VkUtils::VkTriangle& triangle) {
	for (auto vkVertex : { triangle.vA, triangle.vB, triangle.vC }) {
		vkVertices.push_back(vkVertex);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	std::cerr << "Validation layer: " << callbackData->pMessage << std::endl;

	return VK_FALSE;
}
