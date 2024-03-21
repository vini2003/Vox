#include <set>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "application.h"

#include "constants.h"
#include "shader.h"
#include "util.h"

namespace vox {
	void Application::initGlfw() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindow = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, NAME, nullptr, nullptr);
		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, handleFramebufferResize);
		glfwSetScrollCallback(glfwWindow, handleMouseScroll);
	}

	void Application::initVk() {
		initVkInstance();
		initDebugMessenger();
		initSurface();
		initPhysicalDevice();
		initLogicalDevice();
		initSwapchain();
		initShaders();
		initImageViews();
		initRenderPass();
		initDescriptorSetLayouts();
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
		initUniformBufferObjects();
		initDescriptorPool();
		initDescriptorSets();
		initCommandBuffers();
		initSyncObjects();
	}

	void Application::executeImmediateCommand(std::function<void(VkCommandBuffer commandBuffer)> &&function) {
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

	void Application::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		executeImmediateCommand([&](const auto& commandBuffer) {
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
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


	void Application::initImGui() {
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

		if (VK_SUCCESS != vkCreateDescriptorPool(mainLogicalDevice, &poolInfo, nullptr, &imguiDescriptorPool)) {
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
		initInfo.DescriptorPool = imguiDescriptorPool;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo, renderPass);

		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->Clear();

		io.Fonts->AddFontFromFileTTF("fonts/arial.ttf", 16.0f);

		io.Fonts->Build();

		initImGuiStyle();
	}

	void Application::initImGuiStyle(){
		ImGuiStyle& style = ImGui::GetStyle();

		style.Alpha = 1.0f;
		style.DisabledAlpha = 1.0f;
		style.WindowPadding = ImVec2(12.0f, 12.0f);
		style.WindowRounding = 11.5f;
		style.WindowBorderSize = 0.0f;
		style.WindowMinSize = ImVec2(20.0f, 20.0f);
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style.WindowMenuButtonPosition = ImGuiDir_Right;
		style.ChildRounding = 0.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupRounding = 0.0f;
		style.PopupBorderSize = 1.0f;
		style.FramePadding = ImVec2(20.0f, 3.400000095367432f);
		style.FrameRounding = 11.89999961853027f;
		style.FrameBorderSize = 0.0f;
		style.ItemSpacing = ImVec2(4.300000190734863f, 5.5f);
		style.ItemInnerSpacing = ImVec2(7.099999904632568f, 1.799999952316284f);
		style.CellPadding = ImVec2(12.10000038146973f, 9.199999809265137f);
		style.IndentSpacing = 0.0f;
		style.ColumnsMinSpacing = 4.900000095367432f;
		style.ScrollbarSize = 11.60000038146973f;
		style.ScrollbarRounding = 15.89999961853027f;
		style.GrabMinSize = 3.700000047683716f;
		style.GrabRounding = 20.0f;
		style.TabRounding = 0.0f;
		style.TabBorderSize = 0.0f;
		style.TabMinWidthForCloseButton = 0.0f;
		style.ColorButtonPosition = ImGuiDir_Right;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

		style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09411764889955521f, 0.1019607856869698f, 0.1176470592617989f, 1.0f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1137254908680916f, 0.125490203499794f, 0.1529411822557449f, 1.0f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.7960784435272217f, 0.4980392158031464f, 1.0f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1803921610116959f, 0.1882352977991104f, 0.196078434586525f, 1.0f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1529411822557449f, 0.1529411822557449f, 0.1529411822557449f, 1.0f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.1411764770746231f, 0.1647058874368668f, 0.2078431397676468f, 1.0f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.105882354080677f, 0.105882354080677f, 0.105882354080677f, 1.0f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.1294117718935013f, 0.1490196138620377f, 0.1921568661928177f, 1.0f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.125490203499794f, 0.2745098173618317f, 0.572549045085907f, 1.0f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95686274766922f, 0.95686274766922f, 0.95686274766922f, 1.0f);
		style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
		style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
		style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2666666805744171f, 0.2901960909366608f, 1.0f, 1.0f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
	}


	void Application::initVkInstance() {
		if (enableValidationLayers && !checkValidationLayers()) {
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
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

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

	void Application::initDebugMessenger() {
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		buildDebugMessengerCreateInfo(createInfo);

		if (VK_SUCCESS != createDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger)) {
			throw std::runtime_error("Failed to set up debug messenger!");
		}

		std::cout << "[Vulkan] Initialized debug messenger.\n" << std::flush;
	}

	void Application::initShaders() {
		std::map<std::string, ShaderMetadata> shaderMetadata;

		std::map<std::string, std::vector<char>> shaderVertexCode;
		std::map<std::string, std::vector<char>> shaderFragmentCode;

		const std::filesystem::directory_iterator shaderCodeIterator("shaders/spirv");

		for (const auto& shaderEntry : shaderCodeIterator) {
			if (shaderEntry.path().extension() == ".vert" || shaderEntry.path().extension() == ".frag") {
				std::ifstream shaderFile(shaderEntry.path(), std::ios::ate | std::ios::binary);

				if (!shaderFile.is_open()) {
					throw std::runtime_error("[Vulkan] Failed to load shader file: " + shaderEntry.path().filename().string() + "\n");
				}

				std::vector<char> buffer(shaderEntry.file_size());
				shaderFile.seekg(0);
				shaderFile.read(buffer.data(), shaderEntry.file_size());
				shaderFile.close();

				std::string id = shaderEntry.path().stem().string();

				if (shaderEntry.path().extension() == ".vert") {
					shaderVertexCode[id] = buffer;
				} else if (shaderEntry.path().extension() == ".frag") {
					shaderFragmentCode[id] = buffer;
				}

				std::cout << "[Vulkan] Loaded shader file: " << shaderEntry.path().filename().string() << "\n" << std::flush;
			}
		}

		const std::filesystem::directory_iterator shaderMetadataIterator("shaders/metadata");

		for (const auto& metadataEntry : shaderMetadataIterator) {
			if (metadataEntry.path().extension() == ".json") {
				std::ifstream metadataFile(metadataEntry.path());

				if (!metadataFile.is_open()) {
					throw std::runtime_error("[Vulkan] Failed to load shader metadata file: " + metadataEntry.path().filename().string() + "\n");
				}

				nlohmann::json metadata;
				metadataFile >> metadata;
				metadataFile.close();

				std::string id = metadataEntry.path().stem().string();
				shaderMetadata[id] = metadata.get<ShaderMetadata>();

				std::cout << "[Vulkan] Loaded shader metadata file: " << id << ".json\n" << std::flush;
			}
		}


		for (const auto& [id, metadata] : shaderMetadata) {
			shaders[id] = Shader<>(id, metadata, shaderVertexCode[metadata.vertex], shaderFragmentCode[metadata.fragment]);
		}
	}

	void Application::initSurface() {
		if (VK_SUCCESS != glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &surface)) {
			throw std::runtime_error("Failed to create window surface!");
		}

		std::cout << "[Vulkan] Initialized surface.\n" << std::flush;
	}

	void Application::initPhysicalDevice() {
		uint32_t deviceCount = 0;

		if (VK_SUCCESS != vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr) || deviceCount == 0) {
			throw std::runtime_error("Failed to find number of GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);

		if (VK_SUCCESS != vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data())) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		for (const auto& device : devices) {
			if (hasRequiredFeatures(device)) {
				mainPhysicalDevice = device;

				VkPhysicalDeviceProperties deviceProperties;
				vkGetPhysicalDeviceProperties(mainPhysicalDevice, &deviceProperties);

				std::cout << "[Vulkan] Physical device selected: " << deviceProperties.deviceName << "\n" << std::flush;

				break;
			}
		}

		std::cout << "[Vulkan] Initialized physical device.\n" << std::flush;
	}

	void Application::initLogicalDevice() {
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
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
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

	void Application::initSwapchain() {
		const auto [surfaceCapabilites, surfaceFormats, presentModes] = getSwapChainSupport(mainPhysicalDevice);

		const auto [format, colorSpace] = selectSwapSurfaceFormat(surfaceFormats);

		const auto presentMode = selectSwapPresentMode(presentModes);
		const auto extent = selectSwapExtent(surfaceCapabilites);

		uint32_t minImageCount = surfaceCapabilites.minImageCount + 1;

		if (surfaceCapabilites.maxImageCount > 0 && minImageCount > surfaceCapabilites.maxImageCount) {
			minImageCount = surfaceCapabilites.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = minImageCount;
		createInfo.imageFormat = format;
		createInfo.imageColorSpace = colorSpace;
		createInfo.imageExtent = extent;
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

		createInfo.preTransform = surfaceCapabilites.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (VK_SUCCESS != vkCreateSwapchainKHR(mainLogicalDevice, &createInfo, nullptr, &swapchain)) {
			throw std::runtime_error("[Vulkan] Failed to create swap chain!");
		}

		std::cout << "[Vulkan] Initialized swapchain.\n" << std::flush;

		if (VK_SUCCESS != vkGetSwapchainImagesKHR(mainLogicalDevice, swapchain, &minImageCount, nullptr)) {
			throw std::runtime_error("[Vulkan] Failed to get swap chain images!");
		}

		swapchainImages.resize(minImageCount);

		if (VK_SUCCESS != vkGetSwapchainImagesKHR(mainLogicalDevice, swapchain, &minImageCount, swapchainImages.data())) {
			throw std::runtime_error("[Vulkan] Failed to get swap chain images!");
		}

		swapchainImageFormat = format;
		swapchainExtent = extent;
	}

	void Application::initImageViews() {
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

	void Application::initRenderPass() {
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

	void Application::initDescriptorSetLayouts() {
		for (auto& [id, shader] : shaders) {
			shader.reserveBuffer(0);
			shader.reserveSampler(1);
			shader.reserveBuffer();

			shader.buildDescriptorSetLayout(mainLogicalDevice);
		}
	}

	void Application::initPipeline() {
		// TODO: Implement shader module metadata. This would be useful for entrynames, etc.
		// TODO: Another issue is, how do we dictate the attributes, descriptor layouts, etc? This is
		// TODO: why Minecraft has the shader JSON files.
		for (auto& [id, shader] : shaders) {
			auto buildShaderModuleLambda = [&](const std::vector<char>& code) -> VkShaderModule {
				return buildShaderModule(code);
			};

			const auto vertexShaderCode = shader.getVertexShaderCode();
			const auto fragmentShaderCode = shader.getFragmentShaderCode();

			if (!vertexShaderCode.has_value()) {
				throw std::runtime_error("[Vulkan] Vertex shader code not found for shader: " + id);
			}

			if (!fragmentShaderCode.has_value()) {
				throw std::runtime_error("[Vulkan] Fragment shader code not found for shader: " + id);
			}

			const auto vertexShaderModule = buildShaderModule(vertexShaderCode.value());
			const auto fragmentShaderModule = buildShaderModule(fragmentShaderCode.value());

			shader.setVertexShaderCode(std::nullopt); // Free the memory used by the vector.
			shader.setFragmentShaderCode(std::nullopt); // Free the memory used by the vector.

			shader.setVertexShaderModule(vertexShaderModule);
			shader.setFragmentShaderModule(fragmentShaderModule);

			const auto vertexShaderStageCreateInfo = buildPipelineShaderStageCreateInfo(vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
			const auto fragmentShaderStageCreateInfo = buildPipelineShaderStageCreateInfo(fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);

			const auto shaderStageCreateInfos = std::array { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;

			const auto vertexBindingDescription = shader.getBindingDescription();
			const auto vertexAttributeDescription = shader.getAttributeDescriptions();

			vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;

			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
			inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

			VkViewport viewport = {
				.x = 0.0f,
				.y = 0.0f,
				.width = static_cast<float>(swapchainExtent.width),
				.height = static_cast<float>(swapchainExtent.height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			};

			VkRect2D scissor = {
				.offset = { 0, 0 },
				.extent = swapchainExtent
			};

			const auto viewportStateCreateInfo = buildPipelineViewportStateCreateInfo(&viewport, &scissor);
			const auto rasterizationStateCreateInfo = buildPipelineRasterizationStateCreateInfo();
			const auto multisamplerStateCreateInfo = buildPipelineMultisampleStateCreateInfo();
			const auto colorBlendAttachmentState = buildPipelineColorBlendAttachmentState();
			const auto colorBlendStateCreateInfo = buildPipelineColorBlendStateCreateInfo(&colorBlendAttachmentState);
			const auto depthStencilStateCreateInfo = buildPipelineDepthStencilStateCreateInfo();

			if (!shader.getDescriptorSetLayout().has_value()) {
				throw std::runtime_error("[Vulkan] Descriptor set layout not found for shader: " + id);
			}

			auto descriptorSetLayout = shader.getDescriptorSetLayout().value();

			VkPipelineLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutCreateInfo.setLayoutCount = 1;
			layoutCreateInfo.pSetLayouts = &descriptorSetLayout;

			if (VK_SUCCESS != vkCreatePipelineLayout(mainLogicalDevice, &layoutCreateInfo, nullptr, &pipelineLayouts[id])) {
				throw std::runtime_error("[Vulkan] Failed to create pipeline layout for shader: " + id);
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
			pipelineCreateInfo.layout = pipelineLayouts[id];
			pipelineCreateInfo.renderPass = renderPass;
			pipelineCreateInfo.subpass = 0;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineCreateInfo.basePipelineIndex = -1;

			if (VK_SUCCESS != vkCreateGraphicsPipelines(mainLogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelines[id])) {
				throw std::runtime_error("[Vulkan] Failed to create graphics pipeline for shader: " + id);
			}

			std::cout << "[Vulkan] Pipeline initialization succeeded for shader: " + id + "\n" << std::flush;

			vkDestroyShaderModule(mainLogicalDevice, vertexShaderModule, nullptr);
			vkDestroyShaderModule(mainLogicalDevice, fragmentShaderModule, nullptr);
		}
	}

	void Application::initFramebuffers() {
		swapchainFramebuffers.resize(swapchainImageViews.size());

		for (auto i = 0; i < swapchainFramebuffers.size(); ++i) {
			if (VK_SUCCESS != buildFramebuffer(&swapchainFramebuffers[i], renderPass, { swapchainImageViews[i], depthImageView }, swapchainExtent.width, swapchainExtent.height)) {
				throw std::runtime_error("[Vulkan] Failed to create framebuffer!");
			}
		}

		std::cout << "[Vulkan] Framebuffer initialization succeeded.\n" << std::flush;
	}

	void Application::initCommandPools() {
		const auto [graphicsFamily, presentFamily] = getQueueFamilies(mainPhysicalDevice);

		if (VK_SUCCESS != buildCommandPool(&commandPool, graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) {
			throw std::runtime_error("[Vulkan] Failed to create command pool!");
		}

		if (VK_SUCCESS != buildCommandPool(&shortCommandPool, graphicsFamily.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)) {
			throw std::runtime_error("[Vulkan] Failed to create short command pool!");
		}
	}

	void Application::initVertexBuffer() {
		if (VK_SUCCESS != buildVertexBuffer(&vertexBuffer, &vertexBufferMemory, vertices)) {
			throw std::runtime_error("[Vulkan] Failed to create vertex buffer!");
		}
	}

	void Application::initIndexBuffer() {
		if (VK_SUCCESS != buildIndexBuffer(&indexBuffer, &indexBufferMemory, indices)) {
			throw std::runtime_error("[Vulkan] Failed to create index buffer!");
		}
	}

	void Application::initDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> sizes = {};
		sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 5); // TODO: Adjust!
		sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 5); // TODO: Adjust!

		VkDescriptorPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = sizes.size();
		createInfo.pPoolSizes = sizes.data();

		createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 5); // TODO: Adjust!

		if (VK_SUCCESS != vkCreateDescriptorPool(mainLogicalDevice, &createInfo, nullptr, &descriptorPool)) {
			throw std::runtime_error("[Vulkan] Failed to create descriptor pool!");
		}
	}

	void Application::initDescriptorSets() {
		for (auto& [id, shader] : shaders) {
			shader.buildDescriptorSets(mainLogicalDevice, descriptorPool, MAX_FRAMES_IN_FLIGHT);
		}
	}

	void Application::initDepthResources() {
		const auto depthFormat = findDepthFormat();

		if (VK_SUCCESS != buildImage(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory)) {
			throw std::runtime_error("[Vulkan] Failed to create depth image!");
		}

		if (VK_SUCCESS != buildImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView)) {
			throw std::runtime_error("[Vulkan] Failed to create depth image view!");
		}
	}

	void Application::initTextureImage() {
		if (VK_SUCCESS != buildTextureImage(TEXTURE_PATH, textureImage, textureImageMemory)) {
			throw std::runtime_error("[Vulkan] Failed to create texture image!");
		}
	}

	void Application::initTextureImageView() {
		if (VK_SUCCESS != buildImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView)) {
			throw std::runtime_error("[Vulkan] Failed to create texture image view!");
		}
	}

	void Application::initTextureSampler() {
		if (VK_SUCCESS != buildSampler(&textureSampler)) {
			throw std::runtime_error("[Vulkan] Failed to create texture sampler!");
		}
	}

	void Application::initUniformBuffers() {
		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBufferMemories.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (VK_SUCCESS != buildUniformBuffer<UniformBufferObject>(&uniformBuffers[i], &uniformBufferMemories[i])) {
				throw std::runtime_error("[Vulkan] Failed to create uniform buffer!");
			}

			if (VK_SUCCESS != vkMapMemory(mainLogicalDevice, uniformBufferMemories[i], 0, sizeof(UniformBufferObject), 0, &uniformBuffersMapped[i])) {
				throw std::runtime_error("[Vulkan] Failed to map uniform buffer memory!");
			}
		}

		for (auto& [id, shader] : shaders) {
			auto uniformBufferPtrs = std::vector<VkBuffer*>(3);
			auto uniformBufferMemoryPtrs = std::vector<VkDeviceMemory*>(3);

			std::ranges::transform(uniformBuffers, uniformBufferPtrs.begin(), [](auto& buffer) { return &buffer; });
			std::ranges::transform(uniformBufferMemories, uniformBufferMemoryPtrs.begin(), [](auto& memory) { return &memory; });

			shader.bindBuffer(0, uniformBufferPtrs, 0, sizeof(UniformBufferObject), uniformBufferMemoryPtrs, uniformBuffersMapped);
			shader.bindSampler(1, &textureImageView, &textureSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			auto buildBufferLambda = [&](VkBuffer* buffer, VkDeviceMemory* bufferMemory, VkDeviceSize size) -> VkResult {
				return buildUniformBuffer(buffer, bufferMemory, size);
			};

			shader.buildBuffers(buildBufferLambda);
		}
	}

	void Application::initUniformBufferObjects() {
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / static_cast<float>(swapchainExtent.height), 0.1f, 10.0f);

		ubo.proj[1][1] *= -1;
	}

	void Application::initCommandBuffers() {
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (VK_SUCCESS != vkAllocateCommandBuffers(mainLogicalDevice, &allocInfo, commandBuffers.data())) {
			throw std::runtime_error("[Vulkan] Failed to allocate command buffers!");
		}

		std::cout << "[Vulkan] Command buffer allocation succeeded!\n" << std::flush;
	}

	void Application::initSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

	void Application::updateUniformBuffers(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		const auto currentTime = std::chrono::high_resolution_clock::now();
		const auto timeDelta = std::chrono::duration<float>(currentTime - startTime).count();

		handleInput(glfwWindow, timeDelta);

		ubo.view = camera.getViewMatrix();

		const auto aspectRatio = static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height);

		ubo.proj = camera.getProjectionMatrix(aspectRatio);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

		for (auto &shader: shaders | std::views::values) {
			shader.setUniform("decay", 4.5f);
			shader.setUniform("colorModulation", glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));

			shader.uploadUniforms(mainLogicalDevice, currentImage);
		}
	}

	bool Application::checkValidationLayers() const {
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

		for (const auto& layerName : validationLayers) {
			if (std::ranges::find(availableLayerNames, layerName) == availableLayerNames.end()) {
				std::cout << "[Vulkan] Validation layer verification failed.\n" << std::flush;
				return false;
			}
		}

		std::cout << "[Vulkan] Validation layers successfully verified.\n" << std::flush;

		return true;
	}

	void Application::buildDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;
	}

	VkShaderModule Application::buildShaderModule(const std::vector<char>& rawShader) const {
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

	VkPipelineShaderStageCreateInfo Application::buildPipelineShaderStageCreateInfo(const VkShaderModule shaderModule, const VkShaderStageFlagBits stage) {
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage = stage;
		info.module = shaderModule;
		info.pName = "main";
		return info;
	}

	VkPipelineViewportStateCreateInfo Application::buildPipelineViewportStateCreateInfo(const VkViewport* viewport, const VkRect2D* scissor) {
	    VkPipelineViewportStateCreateInfo info{};
	    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	    info.viewportCount = 1;
	    info.pViewports = viewport;
	    info.scissorCount = 1;
	    info.pScissors = scissor;
	    return info;
	}

	VkPipelineRasterizationStateCreateInfo Application::buildPipelineRasterizationStateCreateInfo() {
	    VkPipelineRasterizationStateCreateInfo info{};
	    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	    info.depthClampEnable = VK_FALSE;
	    info.rasterizerDiscardEnable = VK_FALSE;
	    info.polygonMode = VK_POLYGON_MODE_FILL;
	    info.lineWidth = 1.0f;
	    info.cullMode = VK_CULL_MODE_BACK_BIT;
	    info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	    info.depthBiasEnable = VK_FALSE;
	    info.depthBiasConstantFactor = 0.0f;
	    info.depthBiasClamp = 0.0f;
	    info.depthBiasSlopeFactor = 0.0f;
	    return info;
	}

	VkPipelineMultisampleStateCreateInfo Application::buildPipelineMultisampleStateCreateInfo() {
	    VkPipelineMultisampleStateCreateInfo info{};
	    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	    info.sampleShadingEnable = VK_FALSE;
	    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	    info.minSampleShading = 1.0f;
	    info.pSampleMask = nullptr;
	    info.alphaToCoverageEnable = VK_FALSE;
	    info.alphaToOneEnable = VK_FALSE;
	    return info;
	}

	VkPipelineColorBlendAttachmentState Application::buildPipelineColorBlendAttachmentState() {
	    VkPipelineColorBlendAttachmentState attachment{};
	    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	    attachment.blendEnable = VK_TRUE;
	    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	    attachment.colorBlendOp = VK_BLEND_OP_ADD;
	    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	    return attachment;
	}

	VkPipelineColorBlendStateCreateInfo Application::buildPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState *colorBlendAttachmentState) {
	    VkPipelineColorBlendStateCreateInfo info{};
	    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	    info.logicOpEnable = VK_FALSE;
	    info.logicOp = VK_LOGIC_OP_COPY;
	    info.attachmentCount = 1;
	    info.pAttachments = colorBlendAttachmentState;
	    info.blendConstants[0] = 0.0f;
	    info.blendConstants[1] = 0.0f;
	    info.blendConstants[2] = 0.0f;
	    info.blendConstants[3] = 0.0f;
	    return info;
	}

	VkPipelineDepthStencilStateCreateInfo Application::buildPipelineDepthStencilStateCreateInfo() {
	    VkPipelineDepthStencilStateCreateInfo info{};
	    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	    info.depthTestEnable = VK_TRUE;
	    info.depthWriteEnable = VK_TRUE;
	    info.depthCompareOp = VK_COMPARE_OP_LESS;
	    info.depthBoundsTestEnable = VK_FALSE;
	    info.minDepthBounds = 0.0f;
	    info.maxDepthBounds = 1.0f;
	    info.stencilTestEnable = VK_FALSE;
	    info.front = {};
	    info.back = {};
	    return info;
	}

	VkResult Application::buildBuffer(
		VkBuffer* buffer,
		VkDeviceMemory* bufferMemory,
		const VkDeviceSize deviceSize,
		const VkBufferUsageFlags bufferUsageFlags,
		const VkMemoryPropertyFlags memoryPropertyFlags
	) {
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = deviceSize;
		bufferCreateInfo.usage = bufferUsageFlags;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (const auto result = vkCreateBuffer(mainLogicalDevice, &bufferCreateInfo, nullptr, buffer);
			result != VK_SUCCESS) {
			std::cerr << "[Vulkan] Failed to create buffer!\n" << std::flush;
			return result;
		}

		std::cout << "[Vulkan] Buffer initialization succeeded.\n" << std::flush;

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mainLogicalDevice, *buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = getMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

		if (const auto result = vkAllocateMemory(mainLogicalDevice, &allocInfo, nullptr, bufferMemory);
			result != VK_SUCCESS) {
			std::cerr << "[Vulkan] Failed to allocate buffer memory!\n" << std::flush;
			return result;
		}

		std::cout << "[Vulkan] Buffer memory allocation succeeded.\n" << std::flush;

		vkBindBufferMemory(mainLogicalDevice, *buffer, *bufferMemory, 0);

		return VK_SUCCESS;
	}

	VkResult Application::buildImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory) {
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

	VkResult Application::buildImageView(VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags, VkImageView& imageView) {
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

	VkResult Application::buildTextureImage(const std::string& imagePath, VkImage& textureImage, VkDeviceMemory& textureImageMemory) {
	    int textureWidth;
		int textureHeight;
		int textureChannels;

	    stbi_uc* pixels = stbi_load(imagePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

	    if (!pixels) {
	        throw std::runtime_error("[STB] Failed to load texture image!");
	    }

	    const VkDeviceSize imageSize = textureWidth * textureHeight * 4; // Assuming 4 bytes per pixel (RGBA).

	    VkBuffer stagingBuffer;
	    VkDeviceMemory stagingBufferMemory;

	    if (const auto result = buildBuffer(&stagingBuffer, &stagingBufferMemory, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			result != VK_SUCCESS) {
			return result;
		}

	    void* data;

	    if (const auto result = vkMapMemory(mainLogicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	    	result != VK_SUCCESS) {
		    return result;
	    }

	    memcpy(data, pixels, imageSize);
	    vkUnmapMemory(mainLogicalDevice, stagingBufferMemory);

	    stbi_image_free(pixels);

	    if (const auto result = buildImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
	    	result != VK_SUCCESS) {
		    return result;
	    }

	    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		if (const auto result = copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
			result != VK_SUCCESS) {
			return result;
		}

	    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	    vkDestroyBuffer(mainLogicalDevice, stagingBuffer, nullptr);
	    vkFreeMemory(mainLogicalDevice, stagingBufferMemory, nullptr);

	    return VK_SUCCESS;
	}

	VkResult Application::buildUniformBuffer(VkBuffer*buffer, VkDeviceMemory*bufferMemory, const VkDeviceSize size) {
		return buildBuffer(buffer, bufferMemory, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer destBuffer, VkDeviceSize bufferSize) {
		executeImmediateCommand([&](const auto& commandBuffer) {
			VkBufferCopy bufferCopy = {};
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = bufferSize;

			vkCmdCopyBuffer(commandBuffer, srcBuffer, destBuffer, 1, &bufferCopy);
		});
	}

	VkResult Application::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		executeImmediateCommand([&](const auto& commandBuffer) {
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

	VkResult Application::buildSampler(
		VkSampler* sampler,
		const VkFilter magFilter,
		const VkFilter minFilter,
		const VkSamplerAddressMode addressMode,
		const float maxAnisotropy,
		const VkBorderColor borderColor,
		const bool compareEnable,
		const VkCompareOp compareOp,
		const VkSamplerMipmapMode mipmapMode,
		const float mipLodBias,
		const float minLod,
		const float maxLod) const {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = magFilter;
		samplerInfo.minFilter = minFilter;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = maxAnisotropy;
		samplerInfo.borderColor = borderColor;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = compareEnable ? VK_TRUE : VK_FALSE;
		samplerInfo.compareOp = compareOp;
		samplerInfo.mipmapMode = mipmapMode;
		samplerInfo.mipLodBias = mipLodBias;
		samplerInfo.minLod = minLod;
		samplerInfo.maxLod = maxLod;

		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(mainPhysicalDevice, &properties);

		if (samplerInfo.maxAnisotropy > properties.limits.maxSamplerAnisotropy) {
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		}

		return vkCreateSampler(mainLogicalDevice, &samplerInfo, nullptr, sampler);
	}

	VkResult Application::buildCommandPool(VkCommandPool*pool, const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags) const {
		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		poolCreateInfo.flags = flags;

		if (const auto result = vkCreateCommandPool(mainLogicalDevice, &poolCreateInfo, nullptr, pool);
			result != VK_SUCCESS) {
			return result;
		}

		return VK_SUCCESS;
	}

	VkResult Application::buildFramebuffer(VkFramebuffer *framebuffer, VkRenderPass renderPass, const std::vector<VkImageView> &attachments, const uint32_t width, const uint32_t height, const uint32_t layers) const {
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = width;
		framebufferCreateInfo.height = height;
		framebufferCreateInfo.layers = layers;

		if (const auto result = vkCreateFramebuffer(mainLogicalDevice, &framebufferCreateInfo, nullptr, framebuffer);
			result != VK_SUCCESS) {
			return result;
		}

		return VK_SUCCESS;
	}

	VkSurfaceFormatKHR Application::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& surfaceFormats) {
		if (const auto it = std::ranges::find_if(surfaceFormats, [](const auto& surfaceFormat) {
			return surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}); it != surfaceFormats.end()) {
			std::cout << "[Vulkan] Chose surface format: " << it->format << "\n" << std::flush;
			return *it;
		}

		std::cout << "[Vulkan] Chose surface format: " << surfaceFormats[0].format << "\n" << std::flush;

		return surfaceFormats[0];
	}

	VkPresentModeKHR Application::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
		if (const auto it = std::ranges::find_if(presentModes, [](const auto& presentMode) {
			return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
		}); it != presentModes.end()) {
			std::cout << "[Vulkan] Chose present mode: " << *it << "\n" << std::flush;
			return *it;
		}

		std::cout << "[Vulkan] Chose present mode: " << VK_PRESENT_MODE_FIFO_KHR << "\n" << std::flush;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Application::selectSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
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

	std::vector<VkExtensionProperties> Application::getVkExtensionsAvailable() {
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

	std::vector<const char*> Application::getGlfwExtensionsRequired() {
		uint32_t extensionCount = 0;
		const char **pExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		std::vector extensions(pExtensions, pExtensions + extensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		std::cout << "[GLFW] Required extensions successfully verified.\n" << std::flush;

		return extensions;
	}

	SwapChainSupport Application::getSwapChainSupport(VkPhysicalDevice physicalDevice) {
		SwapChainSupport swapChainSupportDetails;

		if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainSupportDetails.surfaceCapabilites)) {
			throw std::runtime_error("[Vulkan] Failed to get physical device surface capabilities!");
		}

		uint32_t surfaceFormatCount;
		if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
			throw std::runtime_error("[Vulkan] Failed to get physical device surface formats!");
		}

		if (surfaceFormatCount != 0) {
			swapChainSupportDetails.surfaceFormats.resize(surfaceFormatCount);

			if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, swapChainSupportDetails.surfaceFormats.data())) {
				throw std::runtime_error("[Vulkan] Failed to get physical device surface formats!");
			}
		}

		uint32_t vkPresentModeCount;
		if (VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &vkPresentModeCount, nullptr)) {
			throw std::runtime_error("[Vulkan] Failed to get physical device surface present modes!");
		}

		if (vkPresentModeCount != 0) {
			swapChainSupportDetails.presentModes.resize(vkPresentModeCount);

			if (VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &vkPresentModeCount, swapChainSupportDetails.presentModes.data())) {
				throw std::runtime_error("[Vulkan] Failed to get physical device surface present modes!");
			}
		}

		return swapChainSupportDetails;
	}

	bool Application::hasSamplerAnisotropySupport(VkPhysicalDeviceFeatures physicalDeviceFeatures) {
		return physicalDeviceFeatures.samplerAnisotropy;
	}

	bool Application::hasExtensionSupport(VkPhysicalDevice vkPhysicalDevice) {
		uint32_t extensionCount;
		if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, nullptr)) {
			throw std::runtime_error("[Vulkan] Failed to enumerate device extension properties!");
		}

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data())) {
			throw std::runtime_error("[Vulkan] Failed to enumerate device extension properties!");
		}

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

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

	bool Application::hasRequiredFeatures(VkPhysicalDevice physicalDevice) {
		VkPhysicalDeviceFeatures supportedFeatures = {};
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		return getQueueFamilies(physicalDevice).areValid() && hasExtensionSupport(physicalDevice) && getSwapChainSupport(physicalDevice).isValid() && hasSamplerAnisotropySupport(supportedFeatures);
	}

	uint32_t Application::getMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryPropertyFlags) {
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

	QueueFamilies Application::getQueueFamilies(VkPhysicalDevice physicalDevice) {
		QueueFamilies queueFamilyIndices;

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

			if (queueFamilyIndices.areValid()) break;
		}

		std::cout << "[Vulkan] Queue family verificaiton finished.\n" << std::flush;

		return queueFamilyIndices;
	}

	void Application::run() {
		initGlfw();

		initVk();
		initImGui();

		loop();
		free();
	}

	void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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

		for (const auto& [id, shader] : shaders) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[id]);

			const VkBuffer vertexBuffers[] = { vertexBuffer };
			const VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts[id], 0, 1, &shader.getDescriptorSets()[currentFrame], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		vkCmdEndRenderPass(commandBuffer);

		if (VK_SUCCESS != vkEndCommandBuffer(commandBuffer)) {
			throw std::runtime_error("[Vulkan] Failed to record command buffer!");
		}
	}

	VkFormat Application::findSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling imageTiling, VkFormatFeatureFlags formatFatureFlags) {
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

	VkFormat Application::findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool Application::hasStencilComponent(VkFormat vkFormat) {
		return vkFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vkFormat == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void Application::loadModel() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const auto& [name, mesh, lines, points] : shapes) {
			for (const auto& [vertexIndex, normalIndex, texCoordIndex] : mesh.indices) {
				Vertex vertex = {};

				vertex.pos = {
					attrib.vertices[3 * vertexIndex + 0],
					attrib.vertices[3 * vertexIndex + 1],
					attrib.vertices[3 * vertexIndex + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * texCoordIndex + 0],
					1.0f - attrib.texcoords[2 * texCoordIndex + 1]
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

	void Application::handleInput(GLFWwindow *window, const float timeDelta) {
		camera.handleKeyboardInput(window, timeDelta);
		camera.handleMouseInput(window);
	}

	void Application::handleMouseScroll(GLFWwindow *window, double x, double y) {
		auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));

		application->camera.handleMouseScroll(window, x, y);
	}

	void Application::handleFramebufferResize(GLFWwindow *window, int width, int height) {
		auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
		application->framebufferResized = true;

		application->camera.setFieldOfView(glm::radians(45.0f));
		application->ubo.proj = glm::perspective(application->camera.getFieldOfView(), width / (float) height, 0.1f, 10.0f);
	}

	void Application::draw() {
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

		drawImGui();

		if (VK_SUCCESS != vkWaitForFences(mainLogicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX)) {
			throw std::runtime_error("[Vulkan] Failed to wait for fence!");
		}

		if (VK_SUCCESS != vkResetCommandBuffer(commandBuffers[currentFrame], 0)) {
			throw std::runtime_error("[Vulkan] Failed to reset command buffer!");
		}

		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		updateUniformBuffers(currentFrame);

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

		if (const auto result = vkQueuePresentKHR(presentQueue, &presentInfo); result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			resetVkSwapchain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("[Vulkan] Failed to present swap chain image!");
		}

		if (VK_SUCCESS != vkQueueWaitIdle(presentQueue)) {
			throw std::runtime_error("[Vulkan] Failed to wait for queue to become idle!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Application::drawImGui() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Camera");

		auto tempPosition = camera.getPosition();
		if (ImGui::SliderFloat3("Position", glm::value_ptr(tempPosition), -10.0f, 10.0f)) {
			camera.setPosition(tempPosition);
		}

		auto movementSpeed = camera.getMovementSpeed();
		if (ImGui::SliderFloat("Movement Speed", &movementSpeed, 0.1f, 10.0f)) {
			camera.setMovementSpeed(movementSpeed);
		}

		auto mouseSenstivity = camera.getMouseSensitivity();
		if (ImGui::SliderFloat("Mouse Sensitivity", &mouseSenstivity, 0.1f, 1.0f)) {
			camera.setMouseSensitivity(mouseSenstivity);
		}

		auto fieldOfView = camera.getFieldOfView();
		if (ImGui::SliderFloat("Field of View", &fieldOfView, 1.0f, 180.0f)) {
			camera.setFieldOfView(fieldOfView);
		}

		ImGui::End();

		ImGui::Render();
	}

	void Application::loop() {
		while (!glfwWindowShouldClose(glfwWindow)) {
			glfwPollEvents();

			// if mouse disabled, disable imgui mouse input
			if (glfwGetInputMode(glfwWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
			} else {
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}

			draw();
		}
	}

	void Application::free() {
	    ImGui_ImplVulkan_Shutdown();
	    ImGui_ImplGlfw_Shutdown();

	    ImGui::DestroyContext();

	    vkDeviceWaitIdle(mainLogicalDevice);

	    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

	    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	        vkDestroyBuffer(mainLogicalDevice, uniformBuffers[i], nullptr);
	        vkFreeMemory(mainLogicalDevice, uniformBufferMemories[i], nullptr);
	    }

		for (const auto &shader: shaders | std::views::values) {
			shader.destroyOwnedBuffers(mainLogicalDevice);
			shader.destroyOwnedBufferMemories(mainLogicalDevice);

			shader.destroyOwnedDescriptorSetLayot(mainLogicalDevice);
		}

		vkDestroyDescriptorPool(mainLogicalDevice, descriptorPool, nullptr);
		vkDestroyDescriptorPool(mainLogicalDevice, imguiDescriptorPool, nullptr);

		for (const auto &pipeline: pipelines | std::views::values) {
			vkDestroyPipeline(mainLogicalDevice, pipeline, nullptr);
		}

		for (const auto &pipelineLayout: pipelineLayouts | std::views::values) {
			vkDestroyPipelineLayout(mainLogicalDevice, pipelineLayout, nullptr);
		}

	    vkDestroyRenderPass(mainLogicalDevice, renderPass, nullptr);

	    vkDestroyDevice(mainLogicalDevice, nullptr);

	    vkDestroySurfaceKHR(vkInstance, surface, nullptr);

	    if (enableValidationLayers) {
	        destroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
	    }

	    vkDestroyInstance(vkInstance, nullptr);

	    glfwDestroyWindow(glfwWindow);

	    glfwTerminate();
	}

	void Application::freeVkSwapchain() {
	    for (const auto imageView : swapchainImageViews) {
	        vkDestroyImageView(mainLogicalDevice, imageView, nullptr);
	    }

	    vkDestroySwapchainKHR(mainLogicalDevice, swapchain, nullptr);

	    for (const auto framebuffer : swapchainFramebuffers) {
	        vkDestroyFramebuffer(mainLogicalDevice, framebuffer, nullptr);
	    }
	}


	void Application::resetVkSwapchain() {
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

	VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
		std::cerr << "[Vulkan] Validation layer: " << callbackData->pMessage << "\n" << std::flush;

		return VK_FALSE;
	}
}