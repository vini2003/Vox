#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "VkApp.hpp"

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
	initDebugMessenger();
	initSurface();
	initPhysicalDevice();
	initLogicalDevice();
	initSwapchain();
	initImageViews();
	initRenderPass();
	initDescriptorSetLayout();
	initPipeline();
	initCommandPools();
	initDepthResources();
	initFramebuffers();
	initTextureImage();
	initTextureImageView();
	initTextureSampler();
	loadModel();
	initVertexBuffer();
	initIndexBuffer();
	initUniformBuffers();
	initDescriptorPool();
	initDescriptorSets();
	initCommandBuffers();
	initSyncObjects();
}

void VkApp::executeImmediateCommand(std::function<void(VkCommandBuffer cmd)> &&function) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = shortCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mainLogicalDevice, &allocInfo, &commandBuffer);

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

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(mainLogicalDevice, shortCommandPool, 1, &commandBuffer);
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

	if (VK_SUCCESS != vkCreateDescriptorPool(mainLogicalDevice, &poolInfo, nullptr, &pool)) {
		throw std::runtime_error("[Vulkan] Failed to create ImGui descriptor pool!");
	}

	std::cout << "[Vulkan] ImGui descriptor pool creation succeeded.\n" << std::flush;

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true); // TODO: Check if install_callbacks is needed.

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = vkInstance;
	initInfo.PhysicalDevice = mainPhysicalDevice;
	initInfo.Device = mainLogicalDevice;
	initInfo.QueueFamily = getQueueFamilies(mainPhysicalDevice).graphicsFamily.value(); // TODO: Check if this works.
	initInfo.Queue = graphicsQueue;
	initInfo.DescriptorPool = pool;
	initInfo.MinImageCount = 3;
	initInfo.ImageCount = 3;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo, renderPass);
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

	for (const auto& glfwExtension : glfwExtensionsRequired) {
		std::cout << "\t - " << glfwExtension << "\n" << std::flush;
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensionsRequired.size());
	createInfo.ppEnabledExtensionNames = glfwExtensionsRequired.data();

	std::vector<VkExtensionProperties> vkExtensionsRequired = getVkExtensionsAvailable();

	std::cout << "[Vulkan] Available extensions:\n" << std::flush;

	for (const auto& [extensionName, specVersion] : vkExtensionsRequired) {
		std::cout << "\t - " << extensionName << "\n" << std::flush;
	}

	if (VK_SUCCESS != vkCreateInstance(&createInfo, nullptr, &vkInstance)) {
		throw std::runtime_error("Failed to create VkInstance!");
	}

	std::cout << "[Vulkan] Initialized instance.\n" << std::flush;
}

void VkApp::initDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	buildDebugMessengerCreateInfo(createInfo);

	if (VK_SUCCESS != VkUtils::createDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger)) {
		throw std::runtime_error("Failed to set up debug messenger!");
	}

	std::cout << "[Vulkan] Initialized debug messenger.\n" << std::flush;
}

void VkApp::initSurface() {
	if (VK_SUCCESS != glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &surface)) {
		throw std::runtime_error("Failed to create window surface!");
	}

	std::cout << "[Vulkan] Initialized surface.\n" << std::flush;
}

void VkApp::initPhysicalDevice() {
	uint32_t vkDeviceCount = 0;

	if (VK_SUCCESS != vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, nullptr) || vkDeviceCount == 0) {
		throw std::runtime_error("Failed to find number of GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> vkDevices(vkDeviceCount);

	if (VK_SUCCESS != vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, vkDevices.data())) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	for (const auto& vkPhysicalDevice : vkDevices) {
		if (hasRequiredFeatures(vkPhysicalDevice)) {
			mainPhysicalDevice = vkPhysicalDevice;

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(mainPhysicalDevice, &deviceProperties);

			std::cout << "[Vulkan] Physical device selected: " << deviceProperties.deviceName << "\n" << std::flush;

			break;
		}
	}

	std::cout << "[Vulkan] Initialized physical device.\n" << std::flush;
}

void VkApp::initLogicalDevice() {
	const auto [graphicsFamily, presentFamily] = getQueueFamilies(mainPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set uniqueQueueFamilies = { graphicsFamily.value(), presentFamily.value() };

	constexpr auto queuePriority = 1.0f;

	for (const auto& queueFamily : uniqueQueueFamilies) {
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

	if (VK_SUCCESS != vkCreateDevice(mainPhysicalDevice, &createInfo, nullptr, &mainLogicalDevice)) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(mainLogicalDevice, graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(mainLogicalDevice, presentFamily.value(), 0, &presentQueue);

	std::cout << "[Vulkan] Initialized logical device.\n" << std::flush;
}

void VkApp::initSwapchain() {
	const auto [vkSurfaceCapabilites, vkSurfaceFormats, vkPresentModes] = getSwapChainSupport(mainPhysicalDevice);

	const auto [format, colorSpace] = selectSwapSurfaceFormat(vkSurfaceFormats);

	const auto vkPresentMode = selectSwapPresentMode(vkPresentModes);
	const auto vkExtent = selectSwapExtent(vkSurfaceCapabilites);

	uint32_t vkMinImageCount = vkSurfaceCapabilites.minImageCount + 1;

	if (vkSurfaceCapabilites.maxImageCount > 0 && vkMinImageCount > vkSurfaceCapabilites.maxImageCount) {
		vkMinImageCount = vkSurfaceCapabilites.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = vkMinImageCount;
	createInfo.imageFormat = format;
	createInfo.imageColorSpace = colorSpace;
	createInfo.imageExtent = vkExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto [graphicsFamily, presentFamily] = getQueueFamilies(mainPhysicalDevice);

	const uint32_t queueFamilyIndices[] = { graphicsFamily.value(), presentFamily.value() };

	if (graphicsFamily != presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = vkSurfaceCapabilites.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = vkPresentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (VK_SUCCESS != vkCreateSwapchainKHR(mainLogicalDevice, &createInfo, nullptr, &swapchain)) {
		throw std::runtime_error("[Vulkan] Failed to create swap chain!");
	}

	std::cout << "[Vulkan] Initialized swapchain.\n" << std::flush;

	if (VK_SUCCESS != vkGetSwapchainImagesKHR(mainLogicalDevice, swapchain, &vkMinImageCount, nullptr)) {
		throw std::runtime_error("[Vulkan] Failed to get swap chain images!");
	}

	swapchainImages.resize(vkMinImageCount);

	if (VK_SUCCESS != vkGetSwapchainImagesKHR(mainLogicalDevice, swapchain, &vkMinImageCount, swapchainImages.data())) {
		throw std::runtime_error("[Vulkan] Failed to get swap chain images!");
	}

	swapchainImageFormat = format;
	swapchainExtent = vkExtent;
}

void VkApp::initImageViews() {
	swapchainImageViews.resize(swapchainImages.size());

	for (auto i = 0; i < swapchainImageViews.size(); ++i) {
		VkImageView imageView;

		if (VK_SUCCESS != buildImageView(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, imageView)) {
			throw std::runtime_error("[Vulkan] Failed to create image views!");
		}

		swapchainImageViews[i] = imageView;
	}

	std::cout << "[Vulkan] Image view creation successfull.\n" << std::flush;
}

void VkApp::initRenderPass() {
	VkAttachmentDescription colorAttDesc = {};
	colorAttDesc.format = swapchainImageFormat;
	colorAttDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttDesc = {};
	depthAttDesc.format = findDepthFormat();
	depthAttDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttRef = {};
	colorAttRef.attachment = 0;
	colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttRef = {};
	depthAttRef.attachment = 1;
	depthAttRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttRef;\
	subpassDesc.pDepthStencilAttachment = &depthAttRef;

	VkSubpassDependency subpassDep = {};
	subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDep.dstSubpass = 0;
	subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDep.srcAccessMask = 0;
	subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	const std::array attachments = { colorAttDesc, depthAttDesc };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDep;

	if (VK_SUCCESS != vkCreateRenderPass(mainLogicalDevice, &renderPassInfo, nullptr, &renderPass)) {
		throw std::runtime_error("[Vulkan] Failed to create render pass!");
	}
}

void VkApp::initDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding descriptorSetUboLayoutBinding = {};
	descriptorSetUboLayoutBinding.binding = 0;
	descriptorSetUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetUboLayoutBinding.descriptorCount = 1;
	descriptorSetUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetUboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding descriptorSetSamplerLayoutBinding = {};
	descriptorSetSamplerLayoutBinding.binding = 1;
	descriptorSetSamplerLayoutBinding.descriptorCount = 1;
	descriptorSetSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetSamplerLayoutBinding.pImmutableSamplers = nullptr;
	descriptorSetSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	const auto bindings = std::array { descriptorSetUboLayoutBinding, descriptorSetSamplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();

	if (VK_SUCCESS != vkCreateDescriptorSetLayout(mainLogicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout)) {
		throw std::runtime_error("[Vulkan] Failed to create descriptor set layout!");
	}
}

void VkApp::initPipeline() {
	std::map<std::string, std::vector<char>> rawShaders = getShaders();

	const auto vkVertShaderModule = buildShaderModule(rawShaders["vert.spv"]);
	const auto vkFragShaderModule = buildShaderModule(rawShaders["frag.spv"]);

	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
	vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageCreateInfo.module = vkVertShaderModule;
	vertexShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
	fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageCreateInfo.module = vkFragShaderModule;
	fragmentShaderStageCreateInfo.pName = "main";

	const std::array shaderStageCreateInfos = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	const auto vertexBindingDescription = VkUtils::VkVertex::getBindingDescription();
	const auto vertexAttributeDescription = VkUtils::VkVertex::getAttributeDescription();

	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplerStateCreateInfo = {};
	multisamplerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplerStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplerStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplerStateCreateInfo.minSampleShading = 1.0f;
	multisamplerStateCreateInfo.pSampleMask = nullptr;
	multisamplerStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplerStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttState = {};
	colorBlendAttState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttState.blendEnable = VK_TRUE;
	colorBlendAttState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	if (VK_SUCCESS != vkCreatePipelineLayout(mainLogicalDevice, &layoutCreateInfo, nullptr, &pipelineLayout)) {
		throw std::runtime_error("[Vulkan] Failed to create pipeline layout!");
	}

	std::cout << "[Vulkan] Pipeline layout initialization succeeded.\n" << std::flush;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplerStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	if (VK_SUCCESS != vkCreateGraphicsPipelines(mainLogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline)) {
		throw std::runtime_error("[Vulkan] Failed to create graphics pipeline!");
	}

	std::cout << "[Vulkan] Pipeline initialization succeeded.\n" << std::flush;

	vkDestroyShaderModule(mainLogicalDevice, vkFragShaderModule, nullptr);
	vkDestroyShaderModule(mainLogicalDevice, vkVertShaderModule, nullptr);
}

void VkApp::initFramebuffers() {
	swapchainFramebuffers.resize(swapchainImageViews.size());

	for (auto i = 0; i < swapchainFramebuffers.size(); ++i) {
		const auto vkImageView = swapchainImageViews[i];

		std::array vkAttachments = { vkImageView, depthImageView };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
		framebufferCreateInfo.pAttachments = vkAttachments.data();
		framebufferCreateInfo.width = swapchainExtent.width;
		framebufferCreateInfo.height = swapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		if (VK_SUCCESS != vkCreateFramebuffer(mainLogicalDevice, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i])) {
			throw std::runtime_error("[Vulkan] Failed to create framebuffer!");
		}
	}

	std::cout << "[Vulkan] Framebuffer initialization succeeded.\n" << std::flush;
}

void VkApp::initCommandPools() {
	const auto [graphicsFamily, presentFamily] = getQueueFamilies(mainPhysicalDevice);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = graphicsFamily.value();
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkUtils::VkCommandPooolBuildInfo commandPoolBuildInfo = {};
	commandPoolBuildInfo.vkLogicalDevice = mainLogicalDevice;
	commandPoolBuildInfo.vkCommandPoolInfo = &commandPoolCreateInfo;
	commandPoolBuildInfo.vkAllocationCallbacks = nullptr;
	commandPoolBuildInfo.vkCommandPool = &commandPool;

	if (VK_SUCCESS != buildPool(commandPoolBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create command pool!");
	}

	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	commandPoolBuildInfo.vkCommandPool = &shortCommandPool;

	if (VK_SUCCESS != buildPool(commandPoolBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create short command pool!");
	}
}

void VkApp::initVertexBuffer() {
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkUtils::VkBufferBuildInfo bufferBuildInfo = {};
	bufferBuildInfo.vkSize = sizeof(vertices[0]) * vertices.size();
	bufferBuildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferBuildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	bufferBuildInfo.vkBuffer = &stagingBuffer;
	bufferBuildInfo.vkBufferMemory = &stagingBufferMemory;

	if (VK_SUCCESS != buildBuffer(bufferBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create staging vertex buffer!");
	}

	void* data;
	vkMapMemory(mainLogicalDevice, stagingBufferMemory, 0, bufferBuildInfo.vkSize, 0, &data);
	memcpy(data, vertices.data(), bufferBuildInfo.vkSize);
	vkUnmapMemory(mainLogicalDevice, stagingBufferMemory);

	bufferBuildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferBuildInfo.vkProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bufferBuildInfo.vkBuffer = &vertexBuffer;
	bufferBuildInfo.vkBufferMemory = &vertexBufferMemory;

	if (VK_SUCCESS != buildBuffer(bufferBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create vertex buffer!");
	}

	copyBuffer(stagingBuffer, vertexBuffer, bufferBuildInfo.vkSize);

	vkDestroyBuffer(mainLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(mainLogicalDevice, stagingBufferMemory, nullptr);
}

void VkApp::initIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkUtils::VkBufferBuildInfo bufferBuildInfo = {};
	bufferBuildInfo.vkSize = bufferSize;
	bufferBuildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferBuildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	bufferBuildInfo.vkBuffer = &stagingBuffer;
	bufferBuildInfo.vkBufferMemory = &stagingBufferMemory;

	if (VK_SUCCESS != buildBuffer(bufferBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create staging index buffer!");
	}

	void* data;

	if (VK_SUCCESS != vkMapMemory(mainLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data)) {
		throw std::runtime_error("[Vulkan] Failed to map memory for index buffer!");
	}

	memcpy(data, indices.data(), bufferSize);

	vkUnmapMemory(mainLogicalDevice, stagingBufferMemory);

	bufferBuildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferBuildInfo.vkProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bufferBuildInfo.vkBuffer = &indexBuffer;
	bufferBuildInfo.vkBufferMemory = &indexBufferMemory;

	if (VK_SUCCESS != buildBuffer(bufferBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create index buffer!");
	}

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(mainLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(mainLogicalDevice, stagingBufferMemory, nullptr);
}

void VkApp::initDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> sizes = {};
	sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sizes[0].descriptorCount = static_cast<uint32_t>(swapchainImages.size());
	sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sizes[1].descriptorCount = static_cast<uint32_t>(swapchainImages.size());

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = sizes.size();
	createInfo.pPoolSizes = sizes.data();

	createInfo.maxSets = static_cast<uint32_t>(swapchainImages.size());

	if (VK_SUCCESS != vkCreateDescriptorPool(mainLogicalDevice, &createInfo, nullptr, &descriptorPool)) {
		throw std::runtime_error("[Vulkan] Failed to create descriptor pool!");
	}
}

void VkApp::initDescriptorSets() {
	const std::vector layouts(swapchainImages.size(), descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapchainImages.size());

	if (VK_SUCCESS != vkAllocateDescriptorSets(mainLogicalDevice, &allocInfo, descriptorSets.data())) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (auto i = 0; i < swapchainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(mainLogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void VkApp::initDepthResources() {
	const auto depthFormat = findDepthFormat();

	if (VK_SUCCESS != buildImage(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory)) {
		throw std::runtime_error("[Vulkan] Failed to create depth image!");
	}

	if (VK_SUCCESS != buildImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView)) {
		throw std::runtime_error("[Vulkan] Failed to create depth image view!");
	}
}

void VkApp::initTextureImage() {
	int textureWidht;
	int textureHeight;
	int textureChannels;

	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &textureWidht, &textureHeight, &textureChannels, STBI_rgb_alpha);

	const VkDeviceSize imageSize = textureWidht * textureHeight * 4;

	if (pixels == nullptr) {
		throw std::runtime_error("[STB] Failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkUtils::VkBufferBuildInfo bufferBuildInfo = {};
	bufferBuildInfo.vkSize = imageSize;
	bufferBuildInfo.vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferBuildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	bufferBuildInfo.vkBuffer = &stagingBuffer;
	bufferBuildInfo.vkBufferMemory = &stagingBufferMemory;

	if (VK_SUCCESS != buildBuffer(bufferBuildInfo)) {
		throw std::runtime_error("[Vulkan] Failed to create staging texture buffer!");
	}

	void* data;

	if (VK_SUCCESS != vkMapMemory(mainLogicalDevice, stagingBufferMemory, 0, imageSize, 0, &data)) {
		throw std::runtime_error("[Vulkan] Failed to map memory for texture buffer!");
	}

	memcpy(data, pixels, imageSize);

	vkUnmapMemory(mainLogicalDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	if (VK_SUCCESS != buildImage(textureWidht, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory)) {
		throw std::runtime_error("[Vulkan] Failed to create texture image!");
	}

	transitionVkImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	if (VK_SUCCESS != copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidht), static_cast<uint32_t>(textureHeight))) {
		throw std::runtime_error("[Vulkan] Failed to copy buffer to image!");
	}

	transitionVkImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(mainLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(mainLogicalDevice, stagingBufferMemory, nullptr);
}

void VkApp::initTextureImageView() {
	if (VK_SUCCESS != buildImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView)) {
		throw std::runtime_error("[Vulkan] Failed to create texture image view!");
	}
}

void VkApp::initTextureSampler() {
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties physicalDeviceProperties = {};
	vkGetPhysicalDeviceProperties(mainPhysicalDevice, &physicalDeviceProperties);

	createInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if (VK_SUCCESS != vkCreateSampler(mainLogicalDevice, &createInfo, nullptr, &textureSampler)) {
		throw std::runtime_error("[Vulkan] Failed to create texture sampler!");
	}
}

void VkApp::initUniformBuffers() {
	uniformBuffers.resize(swapchainImages.size());
	uniformBufferMemories.resize(swapchainImages.size());
	uniformBuffersMapped.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); i++) {
		constexpr auto bufferSize = sizeof(UniformBufferObject);

		VkUtils::VkBufferBuildInfo bufferBuildInfo = {};
		bufferBuildInfo.vkSize = bufferSize;
		bufferBuildInfo.vkUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferBuildInfo.vkProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bufferBuildInfo.vkBuffer = &uniformBuffers[i];
		bufferBuildInfo.vkBufferMemory = &uniformBufferMemories[i];

		if (VK_SUCCESS != buildBuffer(bufferBuildInfo)) {
			throw std::runtime_error("[Vulkan] Failed to create uniform buffer!");
		}

		if (VK_SUCCESS != vkMapMemory(mainLogicalDevice, uniformBufferMemories[i], 0, bufferSize, 0, &uniformBuffersMapped[i])) {
			throw std::runtime_error("[Vulkan] Failed to map uniform buffer memory!");
		}
	}
}

void VkApp::initCommandBuffers() {
	commandBuffers.resize(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (VK_SUCCESS != vkAllocateCommandBuffers(mainLogicalDevice, &allocInfo, commandBuffers.data())) {
		throw std::runtime_error("[Vulkan] Failed to allocate command buffers!");
	}

	std::cout << "[Vulkan] Command buffer allocation succeeded!\n" << std::flush;
}

void VkApp::initSyncObjects() {
	imageAvailableSemaphores.resize(swapchainFramebuffers.size());
	renderFinishedSemaphores.resize(swapchainFramebuffers.size());

	inFlightFences.resize(swapchainFramebuffers.size());

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto i = 0; i < swapchainFramebuffers.size(); i++) {
		if (VK_SUCCESS != vkCreateSemaphore(mainLogicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) ||
			VK_SUCCESS != vkCreateSemaphore(mainLogicalDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i])) {
			throw std::runtime_error("[Vulkan] Failed to create semaphore(s)!");
		}

		if (VK_SUCCESS != vkCreateFence(mainLogicalDevice, &fenceCreateInfo, nullptr, &inFlightFences[i])) {
			throw std::runtime_error("[Vulkan] Failed to create fence(s)!");
		}
	}

	std::cout << "[Vulkan] Semaphore creation succeeded.\n" << std::flush;
}

void VkApp::updateVkUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto time = std::chrono::duration<float>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / static_cast<float>(swapchainExtent.height), 0.1f, 10.0f);

	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

bool VkApp::checkVkValidationLayers() {
	uint32_t layerCount = 0;
	if (VK_SUCCESS != vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) {
		throw std::runtime_error("[Vulkan] Failed to enumerate instance layer properties!");
	}

	std::vector<VkLayerProperties> availableLayers(layerCount);
	if (vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) {
		throw std::runtime_error("[Vulkan] Failed to enumerate instance layer properties!");
	}

	std::vector<std::string> availableLayerNames;

	for (const auto& [layerName, specVersion, implementationVersion, description] : availableLayers) {
		availableLayerNames.emplace_back(layerName);
		std::cout << "[Vulkan] Available layer: " << layerName << "\n" << std::flush;
	}

	for (const auto& layerName : vkValidationLayers) {
		if (std::ranges::find(availableLayerNames, layerName) == availableLayerNames.end()) {
			std::cout << "[Vulkan] Validation layer verification failed.\n" << std::flush;
			return false;
		}
	}

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

	VkShaderModule module;

	if (VK_SUCCESS != vkCreateShaderModule(mainLogicalDevice, &createInfo, nullptr, &module)) {
		throw std::runtime_error("[Vulkna] Failed to create shader module!");
	}

	std::cout << "[Vulkan] Built shader with " << rawShader.size() << " bytes.\n" << std::flush;

	return module;
}

VkResult VkApp::buildBuffer(VkUtils::VkBufferBuildInfo buildInfo) {
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildInfo.vkSize;
	bufferCreateInfo.usage = buildInfo.vkUsage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (const auto result = vkCreateBuffer(mainLogicalDevice, &bufferCreateInfo, nullptr, buildInfo.vkBuffer);
		result != VK_SUCCESS) {
		std::cerr << "[Vulkan] Failed to create buffer!\n" << std::flush;
		return result;
	}

	std::cout << "[Vulkan] Buffer initialization succeeded.\n" << std::flush;

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(mainLogicalDevice, *buildInfo.vkBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, buildInfo.vkProperties);

	if (const auto result = vkAllocateMemory(mainLogicalDevice, &allocInfo, nullptr, buildInfo.vkBufferMemory);
		result != VK_SUCCESS) {
		std::cerr << "[Vulkan] Failed to allocate buffer memory!\n" << std::flush;
		return result;
	}

	std::cout << "[Vulkan] Buffer memory allocation succeeded.\n" << std::flush;

	vkBindBufferMemory(mainLogicalDevice, *buildInfo.vkBuffer, *buildInfo.vkBufferMemory, 0);

	return VK_SUCCESS;
}

VkResult VkApp::buildImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usageFlags;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.flags = 0;

	if (const auto result = vkCreateImage(mainLogicalDevice, &createInfo, nullptr, &image);
		result != VK_SUCCESS) {
		std::cerr << "[Vulkan] Failed to create image!\n" << std::flush;
		return result;
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(mainLogicalDevice, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);

	if (const auto result = vkAllocateMemory(mainLogicalDevice, &memoryAllocInfo, nullptr, &imageMemory);
		result != VK_SUCCESS) {
		std::cerr << "[Vulkan] Failed to allocate image memory!\n" << std::flush;
		return result;
	}

	if (const auto result = vkBindImageMemory(mainLogicalDevice, image, imageMemory, 0);
		result != VK_SUCCESS) {
		std::cerr << "[Vulkan] Failed to bind image memory!\n" << std::flush;
		return result;
	}

	return VK_SUCCESS;
}

VkResult VkApp::buildImageView(VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags, VkImageView& imageView) {
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = imageAspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	return vkCreateImageView(mainLogicalDevice, &createInfo, nullptr, &imageView);
}

void VkApp::copyBuffer(VkBuffer srcBuffer, VkBuffer destBuffer, VkDeviceSize bufferSize) {
	executeImmediateCommand([&](VkCommandBuffer commandBuffer) {
		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = bufferSize;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, destBuffer, 1, &bufferCopy);
	});
}

VkResult VkApp::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	executeImmediateCommand([&](VkCommandBuffer commandBuffer) {
		VkBufferImageCopy bufferImageCopy = {};
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageOffset = {0, 0, 0};
		bufferImageCopy.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferImageCopy
		);
	});

	return VK_SUCCESS;
}

VkResult VkApp::buildPool(VkUtils::VkCommandPooolBuildInfo buildInfo) {
	if (const auto result = vkCreateCommandPool(buildInfo.vkLogicalDevice, buildInfo.vkCommandPoolInfo, buildInfo.vkAllocationCallbacks, buildInfo.vkCommandPool);
		VK_SUCCESS != result) {
		return result;
	}

	std::cout << "[Vulkan] Command pool initialization succeeded!\n" << std::flush;

	return VK_SUCCESS;
}

VkSurfaceFormatKHR VkApp::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) {
	if (const auto it = std::ranges::find_if(surfaceFormats, [](const auto& surfaceFormat) {
		return surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}); it != surfaceFormats.end()) {
		std::cout << "[Vulkan] Chose surface format: " << it->format << "\n" << std::flush;
		return *it;
	}

	std::cout << "[Vulkan] Chose surface format: " << surfaceFormats[0].format << "\n" << std::flush;

	return surfaceFormats[0];
}

VkPresentModeKHR VkApp::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
	if (const auto it = std::ranges::find_if(presentModes, [](const auto& presentMode) {
		return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
	}); it != presentModes.end()) {
		std::cout << "[Vulkan] Chose present mode: " << *it << "\n" << std::flush;
		return *it;
	}

	std::cout << "[Vulkan] Chose present mode: " << VK_PRESENT_MODE_FIFO_KHR << "\n" << std::flush;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VkApp::selectSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
	if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
		std::cout << "[Vulkan] Chose swap dimensions: " << surfaceCapabilities.currentExtent.width << " x " << surfaceCapabilities.currentExtent.height << "\n" << std::flush;
		return surfaceCapabilities.currentExtent;
	}

	int width;
	int height;

	glfwGetFramebufferSize(glfwWindow, &width, &height);

	const VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

	std::cout << "[Vulkan] Chose swap dimensions: " << extent.width << " x " << extent.height << "\n" << std::flush;

	return extent;
}

std::vector<VkExtensionProperties> VkApp::getVkExtensionsAvailable() {
	uint32_t extensionCount = 0;
	if (VK_SUCCESS != vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr)) {
		throw std::runtime_error("[Vulkan] Failed to enumerate instance extension properties!");
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	if (VK_SUCCESS != vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data())) {
		throw std::runtime_error("[Vulkan] Failed to enumerate instance extension properties!");
	}

	std::cout << "[Vulkan] Available extensions successfully verified.\n" << std::flush;

	return extensions;
}

std::vector<const char*> VkApp::getGlfwExtensionsRequired() {
	uint32_t extensionCount = 0;
	const char **pExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	std::vector extensions(pExtensions, pExtensions + extensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::cout << "[GLFW] Required extensions successfully verified.\n" << std::flush;

	return extensions;
}

VkUtils::VkSwapChainSupportDetails VkApp::getSwapChainSupport(VkPhysicalDevice physicalDevice) {
	VkUtils::VkSwapChainSupportDetails swapChainSupportDetails;

	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainSupportDetails.vkSurfaceCapabilites)) {
		throw std::runtime_error("[Vulkan] Failed to get physical device surface capabilities!");
	}

	uint32_t surfaceFormatCount;
	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
		throw std::runtime_error("[Vulkan] Failed to get physical device surface formats!");
	}

	if (surfaceFormatCount != 0) {
		swapChainSupportDetails.vkSurfaceFormats.resize(surfaceFormatCount);

		if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, swapChainSupportDetails.vkSurfaceFormats.data())) {
			throw std::runtime_error("[Vulkan] Failed to get physical device surface formats!");
		}
	}

	uint32_t vkPresentModeCount;
	if (VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &vkPresentModeCount, nullptr)) {
		throw std::runtime_error("[Vulkan] Failed to get physical device surface present modes!");
	}

	if (vkPresentModeCount != 0) {
		swapChainSupportDetails.vkPresentModes.resize(vkPresentModeCount);

		if (VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &vkPresentModeCount, swapChainSupportDetails.vkPresentModes.data())) {
			throw std::runtime_error("[Vulkan] Failed to get physical device surface present modes!");
		}
	}

	return swapChainSupportDetails;
}

bool VkApp::hasSamplerAnisotropySupport(VkPhysicalDeviceFeatures physicalDeviceFeatures) {
	return physicalDeviceFeatures.samplerAnisotropy;
}

bool VkApp::hasExtensionSupport(VkPhysicalDevice vkPhysicalDevice) {
	uint32_t extensionCount;
	if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, nullptr)) {
		throw std::runtime_error("[Vulkan] Failed to enumerate device extension properties!");
	}

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data())) {
		throw std::runtime_error("[Vulkan] Failed to enumerate device extension properties!");
	}

	std::set<std::string> requiredExtensions(vkDeviceExtensions.begin(), vkDeviceExtensions.end());

	for (const auto& [extensionName, specVersion] : availableExtensions) {
		requiredExtensions.erase(extensionName);
	}

	if (requiredExtensions.empty()) {
		std::cout << "[Vulkan] Extension support verification succeeded.\n" << std::flush;
		return true;
	}

	std::cout << "[Vulkan] Extension support verification failed.\n" << std::flush;

	return false;
}

bool VkApp::hasRequiredFeatures(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceFeatures supportedFeatures = {};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return getQueueFamilies(physicalDevice).isValid() && hasExtensionSupport(physicalDevice) && getSwapChainSupport(physicalDevice).isValid() && hasSamplerAnisotropySupport(supportedFeatures);
}

uint32_t VkApp::getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags) {
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(mainPhysicalDevice, &physicalDeviceMemoryProperties);

	for (auto i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags) {
			std::cout << "[Vulkan] Found compatible memory type.\n" << std::flush;
			return i;
		}
	}

	throw std::runtime_error("[Vulkan] Failed to find suitable memory type!");
}

VkUtils::VkQueueFamilyIndices VkApp::getQueueFamilies(VkPhysicalDevice physicalDevice) {
	VkUtils::VkQueueFamilyIndices queueFamilyIndices;

	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

	for (auto i = 0; i < familyCount; i++) {
		VkBool32 presentSupport = false;
		if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport)) {
			throw std::runtime_error("[Vulkan] Failed to get physical device surface support!");
		}

		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) queueFamilyIndices.graphicsFamily = i;
		if (presentSupport) queueFamilyIndices.presentFamily = i;

		if (queueFamilyIndices.isValid()) break;
	}

	std::cout << "[Vulkan] Queue family verificaiton finished.\n" << std::flush;

	return queueFamilyIndices;
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

	initVk();
	initImGui();

	loop();
	free();
}

void VkApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	if (VK_SUCCESS != vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo)) {
		throw std::runtime_error("[Vulkan] Failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = swapchainFramebuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = {0, 0};
	renderPassBeginInfo.renderArea.extent = swapchainExtent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	if (VK_SUCCESS != vkEndCommandBuffer(commandBuffer)) {
		throw std::runtime_error("[Vulkan] Failed to record command buffer!");
	}
}

VkFormat VkApp::findSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling imageTiling, VkFormatFeatureFlags formatFatureFlags) {
	for (const auto& format : formats) {
		VkFormatProperties vkProperties;
		vkGetPhysicalDeviceFormatProperties(mainPhysicalDevice, format, &vkProperties);

		if (imageTiling == VK_IMAGE_TILING_LINEAR && (vkProperties.linearTilingFeatures & formatFatureFlags) == formatFatureFlags) {
			return format;
		}

		if (imageTiling == VK_IMAGE_TILING_OPTIMAL && (vkProperties.optimalTilingFeatures & formatFatureFlags) == formatFatureFlags) {
			return format;
		}
	}

	throw std::runtime_error("[Vulkan] Failed to find supported format!");
}

VkFormat VkApp::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
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

			vertices.push_back(vertex);

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void VkApp::draw() {
	uint32_t imageIndex;

	if (VK_SUCCESS != vkWaitForFences(mainLogicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX)) {
		throw std::runtime_error("[Vulkan] Failed to wait for fence!");
	}

	if (const auto result = vkAcquireNextImageKHR(mainLogicalDevice, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		result == VK_ERROR_OUT_OF_DATE_KHR) {
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

	if (VK_SUCCESS != vkWaitForFences(mainLogicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX)) {
		throw std::runtime_error("[Vulkan] Failed to wait for fence!");
	}

	if (VK_SUCCESS != vkResetCommandBuffer(commandBuffers[currentFrame], 0)) {
		throw std::runtime_error("[Vulkan] Failed to reset command buffer!");
	}

	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	updateVkUniformBuffer(currentFrame);

	if (VK_SUCCESS != vkResetFences(mainLogicalDevice, 1, &inFlightFences[currentFrame])) {
		throw std::runtime_error("[Vulkan] Failed to reset fence!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	const std::vector waitSemaphores = { imageAvailableSemaphores[currentFrame] };
	const std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	const std::vector signalSemaphores = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	if (VK_SUCCESS != vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame])) {
		throw std::runtime_error("[Vulkan] Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();

	const std::vector swapChains = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &imageIndex;

	if (const auto result = vkQueuePresentKHR(presentQueue, &presentInfo); result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vkFramebufferResized) {
		vkFramebufferResized = false;
		resetVkSwapchain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan] Failed to present swap chain image!");
	}

	if (VK_SUCCESS != vkQueueWaitIdle(presentQueue)) {
		throw std::runtime_error("[Vulkan] Failed to wait for queue to become idle!");
	}

	currentFrame = (currentFrame + 1) % (swapchainFramebuffers.size());
}

void VkApp::loop() {
	while (!glfwWindowShouldClose(glfwWindow)) {
		glfwPollEvents();

		draw();
	}
}

void VkApp::free() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    vkDeviceWaitIdle(mainLogicalDevice);

    for (auto i = 0; i < swapchainFramebuffers.size(); i++) {
        vkDestroySemaphore(mainLogicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(mainLogicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(mainLogicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(mainLogicalDevice, commandPool, nullptr);
    vkDestroyCommandPool(mainLogicalDevice, shortCommandPool, nullptr);

	vkDestroyBuffer(mainLogicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(mainLogicalDevice, vertexBufferMemory, nullptr);

	vkDestroyBuffer(mainLogicalDevice, indexBuffer, nullptr);
	vkFreeMemory(mainLogicalDevice, indexBufferMemory, nullptr);

    freeVkSwapchain();

    vkDestroySampler(mainLogicalDevice, textureSampler, nullptr);
    vkDestroyImageView(mainLogicalDevice, textureImageView, nullptr);
    vkDestroyImage(mainLogicalDevice, textureImage, nullptr);
    vkFreeMemory(mainLogicalDevice, textureImageMemory, nullptr);

    vkDestroyImageView(mainLogicalDevice, depthImageView, nullptr);
    vkDestroyImage(mainLogicalDevice, depthImage, nullptr);
    vkFreeMemory(mainLogicalDevice, depthImageMemory, nullptr);

    for (auto i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(mainLogicalDevice, uniformBuffers[i], nullptr);
        vkFreeMemory(mainLogicalDevice, uniformBufferMemories[i], nullptr);
    }

	vkDestroyDescriptorPool(mainLogicalDevice, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(mainLogicalDevice, descriptorSetLayout, nullptr);

    vkDestroyPipeline(mainLogicalDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(mainLogicalDevice, pipelineLayout, nullptr);

    vkDestroyRenderPass(mainLogicalDevice, renderPass, nullptr);

    vkDestroyDevice(mainLogicalDevice, nullptr);

    vkDestroySurfaceKHR(vkInstance, surface, nullptr);

    if (enableValidationLayers) {
        VkUtils::destroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr); // Adjust based on your debug messenger creation method
    }
    vkDestroyInstance(vkInstance, nullptr);

    glfwDestroyWindow(glfwWindow);

    glfwTerminate();
}

void VkApp::freeVkSwapchain() {
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(mainLogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(mainLogicalDevice, swapchain, nullptr);

    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(mainLogicalDevice, framebuffer, nullptr);
    }
}


void VkApp::resetVkSwapchain() {
	int width = 0, height = 0;

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(glfwWindow, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(mainLogicalDevice);

	freeVkSwapchain();

	initSwapchain();
	initImageViews();
	initDepthResources();
	initFramebuffers();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	std::cerr << "Validation layer: " << callbackData->pMessage << std::endl;

	return VK_FALSE;
}
