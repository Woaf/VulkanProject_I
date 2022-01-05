#include "VulkanRenderer.h"

#include <algorithm>

VulkanRenderer::VulkanRenderer ()
{
}


int VulkanRenderer::InitRenderer (GLFWwindow *newWindow)
{
	window = newWindow;

	try {
		CreateInstance ();
		CreateSurface ();
		GetPhysicalDevice ();
		CreateLogicalDevice ();
		CreateSwapchain ();
		CreateRenderPass ();
		CreateGraphicsPipeline ();
		CreateFrameBuffers ();
		CreateCommandPool ();
		CreateCommandBuffers ();
		RecordCommands ();
		CreateSynchronization ();
	} catch (const std::runtime_error& runtimeError) {
		std::cerr << "Error: " << runtimeError.what () << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}


void VulkanRenderer::CleanUp ()
{
	vkDeviceWaitIdle (mainDevice.logicalDevice);

	for (size_t i = 0; i < MaxFrameDraws; ++i) {
		vkDestroySemaphore (mainDevice.logicalDevice, rendersFinished[i], nullptr);
		vkDestroySemaphore (mainDevice.logicalDevice, imagesAvailable[i], nullptr);
		vkDestroyFence (mainDevice.logicalDevice, drawFences[i], nullptr);
	}

	vkDestroyCommandPool (mainDevice.logicalDevice, graphicsCommandPool, nullptr);
	for (auto framebuffer : swapchainFrameBuffers) {
		vkDestroyFramebuffer (mainDevice.logicalDevice, framebuffer, nullptr);
	}
	vkDestroyPipeline (mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout (mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass (mainDevice.logicalDevice, renderPass, nullptr);
	for (auto image : swapchainImages) {
		vkDestroyImageView (mainDevice.logicalDevice, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR (mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR (instance, surface, nullptr);
	vkDestroyDevice (mainDevice.logicalDevice, nullptr);
	vkDestroyInstance (instance, nullptr);
}


VulkanRenderer::~VulkanRenderer ()
{
}


void VulkanRenderer::CreateInstance ()
{
	VkApplicationInfo applicationInfo {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "Vulkan App";
	applicationInfo.applicationVersion = VK_MAKE_VERSION (1, 0, 0);
	applicationInfo.pEngineName = "No engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION (1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &applicationInfo;

	std::vector<const char*> instanceExtensions;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);
	for (size_t i = 0; i < glfwExtensionCount; ++i) {
		instanceExtensions.push_back (glfwExtensions [i]);
	}

	if (!CheckInstanceExtensionSupport (&instanceExtensions)) {
		throw std::runtime_error ("Vk instnace does not support required extensions...");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t> (instanceExtensions.size ());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data ();

	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	VkResult result = vkCreateInstance (&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a vulkan instance...");
	}
}


bool VulkanRenderer::CheckInstanceExtensionSupport (const std::vector<const char*>* checkExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions (extensionCount);
	vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, extensions.data ());

	for (const auto& checkExtension : *checkExtensions) {
		bool hasExtension = false;
		for (const auto& extension : extensions) {
			if (strcmp (checkExtension, extension.extensionName) == 0) {
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension) {
			return false;
		}
	}

	return true;
}


void VulkanRenderer::GetPhysicalDevice ()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices (instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error ("Cannot find GPU that support Vulkan instance...");
	}

	std::vector<VkPhysicalDevice> deviceList (deviceCount);
	vkEnumeratePhysicalDevices (instance, &deviceCount, deviceList.data ());

	for (const auto& device : deviceList) {
		if (CheckDeviceSuitable (device)) {
			mainDevice.physicalDevice = device;
			break;
		}
	}
}


bool VulkanRenderer::CheckDeviceSuitable (VkPhysicalDevice device)
{
	/*
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties (device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures (device, &deviceFeatures);
	*/

	QueueFamilyIndices indices = GetQueueFamilies (device);

	bool extensionSupported = CheckDeviceExtensionSupport (device);

	bool swapchainValid = false;
	if (extensionSupported) {
		SwapchainDetails swapchainDetails = GetSwapchainDetails (device);
		swapchainValid = !swapchainDetails.formats.empty () && !swapchainDetails.presentationModes.empty ();
	}

	return indices.IsValid () && extensionSupported && swapchainValid ;
}


QueueFamilyIndices VulkanRenderer::GetQueueFamilies (VkPhysicalDevice device )
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties (device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList (queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties (device, &queueFamilyCount, queueFamilyList.data ());

	int deviceLocation = 0;
	for (const auto& queueFamily : queueFamilyList) {
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.graphicsFamily = deviceLocation;
		}

		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR (device, deviceLocation, surface, &presentationSupport);
		if (queueFamily.queueCount > 0 && presentationSupport) {
			indices.presentationFamily = deviceLocation;
		}

		if (indices.IsValid ()) {
			break;
		}

		++deviceLocation;
	}

	return indices;
}


void VulkanRenderer::CreateLogicalDevice ()
{
	QueueFamilyIndices indices = GetQueueFamilies (mainDevice.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices {indices.graphicsFamily, indices.graphicsFamily};

	for (int queueFamilyIndex : queueFamilyIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.emplace_back (queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t> (queueCreateInfos.size ());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtensions.size ());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data ();

	VkPhysicalDeviceFeatures deviceFeatures {};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	VkResult result = vkCreateDevice (mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a logical device...");
	}

	vkGetDeviceQueue (mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue (mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}


bool VulkanRenderer::CheckDeviceExtensionSupport (VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0) {
		return false;
	}

	std::vector<VkExtensionProperties> extensions (extensionCount);
	vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, extensions.data ());

	for (const auto& deviceExtension : deviceExtensions) {
		bool hasExtension = false;
		for (const auto& extension : extensions) {
			if (strcmp (deviceExtension, extension.extensionName) == 0) {
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension) {
			return false;
		}
	}

	return true;
}


void VulkanRenderer::CreateSurface ()
{
	VkResult result = glfwCreateWindowSurface (instance, window, nullptr, &surface);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a surface...");
	}
}


VkSurfaceFormatKHR VulkanRenderer::ChooseBestSurfaceFormat (const std::vector<VkSurfaceFormatKHR> &formats)
{
	if (formats.size () == 1 && formats.at (0).format == VK_FORMAT_UNDEFINED) {
		return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const auto& format : formats) {
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats.at (0);
}


VkPresentModeKHR VulkanRenderer::ChooseBestPresentationMode (const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const auto& presentationMode : presentationModes) {
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentationMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


SwapchainDetails VulkanRenderer::GetSwapchainDetails (VkPhysicalDevice device)
{
	SwapchainDetails swapchainDetails {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR (device, surface, &swapchainDetails.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		swapchainDetails.formats.resize (formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &formatCount, swapchainDetails.formats.data());
	}

	uint32_t presentationModesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &presentationModesCount, nullptr);
	if (presentationModesCount != 0) {
		swapchainDetails.presentationModes.resize (presentationModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &presentationModesCount, swapchainDetails.presentationModes.data ());
	}

	return swapchainDetails;
}


void VulkanRenderer::CreateSwapchain ()
{
	SwapchainDetails swapchainDetails = GetSwapchainDetails (mainDevice.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseBestSurfaceFormat (swapchainDetails.formats);
	VkPresentModeKHR presentMode = ChooseBestPresentationMode (swapchainDetails.presentationModes);
	VkExtent2D extent = ChooseSwapExtent (swapchainDetails.surfaceCapabilities);

	uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount + 1;

	if (swapchainDetails.surfaceCapabilities.maxImageCount > 0 &&
		swapchainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapchainDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.preTransform = swapchainDetails.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.clipped = VK_TRUE;

	QueueFamilyIndices indices = GetQueueFamilies (mainDevice.physicalDevice);

	if (indices.graphicsFamily != indices.presentationFamily) {
		uint32_t queueFamilyIndices[] = {
				(uint32_t) indices.graphicsFamily,
				(uint32_t) indices.presentationFamily
		};

		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR (mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a swapchain...");
	}

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	uint32_t swapchainImageCount = 0;
	vkGetSwapchainImagesKHR (mainDevice.logicalDevice, swapchain, &swapchainImageCount, nullptr);
	std::vector<VkImage> images (swapchainImageCount);
	vkGetSwapchainImagesKHR (mainDevice.logicalDevice, swapchain, &swapchainImageCount, images.data ());

	for (VkImage image : images) {
		SwapchainImage swapchainImage {};
		swapchainImage.image = image;
		swapchainImage.imageView = CreateImageView (image, swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		swapchainImages.emplace_back (swapchainImage);
	}
}


VkExtent2D VulkanRenderer::ChooseSwapExtent (const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max ()) {
		return surfaceCapabilities.currentExtent;
	} else {
		int width;
		int height;
		glfwGetFramebufferSize (window, &width, &height);

		VkExtent2D newExtent {};
		newExtent.width = static_cast<uint32_t> (width);
		newExtent.height = static_cast<uint32_t> (height);

		newExtent.width = std::max (surfaceCapabilities.minImageExtent.width, std::min (surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max (surfaceCapabilities.minImageExtent.height, std::min (surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}


VkImageView VulkanRenderer::CreateImageView (VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VkResult result = vkCreateImageView (mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create an image view..");
	}

	return imageView;
}


void VulkanRenderer::CreateGraphicsPipeline ()
{
	auto vertexShaderCode = ReadFile ("../Shaders/vert.spv");
	auto fragmentShaderCode = ReadFile ("../Shaders/frag.spv");

	VkShaderModule vertexShaderModule = CreateShaderModule (vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule (fragmentShaderCode);

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] {vertexShaderCreateInfo, fragmentShaderCreateInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float> (swapchainExtent.width);
	viewport.height = static_cast<float> (swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor {};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportCreateInfo {};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	// dynamic part of the pipeline
	/*
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back (VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back (VK_DYNAMIC_STATE_SCISSOR);

	VkPipelineDynamicStateCreateInfo dynamicStateCrateInfo;
	dynamicStateCrateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCrateInfo.dynamicStateCount = static_cast<uint32_t> (dynamicStateEnables.size ());
	dynamicStateCrateInfo.pDynamicStates = dynamicStateEnables.data ();
	*/

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout (mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create pipeline layout...");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;

	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines (mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a graphics pipeline...");
	}

	vkDestroyShaderModule (mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule (mainDevice.logicalDevice, vertexShaderModule, nullptr);
}


VkShaderModule VulkanRenderer::CreateShaderModule (const std::vector<char> &code)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size ();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*> (code.data ());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule (mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a shader module...");
	}

	return shaderModule;
}


void VulkanRenderer::CreateRenderPass ()
{
	VkAttachmentDescription colorAttachment {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;

	std::array<VkSubpassDependency, 2> subpassDependencies;
	// Transition must happen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// ... but before...
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// Transition must happen after...
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// but before...
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t> (subpassDependencies.size ());
	renderPassCreateInfo.pDependencies = subpassDependencies.data ();

	VkResult result = vkCreateRenderPass (mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a render pass...");
	}
}


void VulkanRenderer::CreateFrameBuffers ()
{
	swapchainFrameBuffers.resize (swapchainImages.size ());
	for (size_t i = 0; i < swapchainFrameBuffers.size (); ++i) {
		std::array<VkImageView, 1> attachments = {
			swapchainImages[i].imageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t> (attachments.size ());
		framebufferCreateInfo.pAttachments = attachments.data ();
		framebufferCreateInfo.width = swapchainExtent.width;
		framebufferCreateInfo.height = swapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		VkResult result = vkCreateFramebuffer (mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &swapchainFrameBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error ("Failed to create framebuffer...");
		}
	}
}


void VulkanRenderer::CreateCommandPool ()
{
	QueueFamilyIndices queueFamilyIndices = GetQueueFamilies (mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	VkResult result = vkCreateCommandPool (mainDevice.logicalDevice, &poolCreateInfo, nullptr, &graphicsCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a command pool...");
	}
}


void VulkanRenderer::CreateCommandBuffers ()
{
	commandBuffers.resize (swapchainFrameBuffers.size ());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = graphicsCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t> (commandBuffers.size ());

	VkResult result = vkAllocateCommandBuffers (mainDevice.logicalDevice, &commandBufferAllocateInfo, commandBuffers.data ());
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to allocate command buffers...");
	}
}


void VulkanRenderer::RecordCommands ()
{
	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkRenderPassBeginInfo renderPassBeginInfo {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset = {0, 0};
	renderPassBeginInfo.renderArea.extent = swapchainExtent;
	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.64f, 1.0f}
	};
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.clearValueCount = 1;

	for (size_t i = 0; i < commandBuffers.size (); ++i) {
		renderPassBeginInfo.framebuffer = swapchainFrameBuffers[i];

		VkResult result = vkBeginCommandBuffer (commandBuffers[i], &beginInfo);
		if (result != VK_SUCCESS) {
			throw std::runtime_error ("Failed to start recording a command buffer...");
		}

			vkCmdBeginRenderPass (commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline (commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				vkCmdDraw (commandBuffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass (commandBuffers[i]);

		result = vkEndCommandBuffer (commandBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error ("Failed to stop recording a command buffer...");
		}
	}
}


void VulkanRenderer::Draw ()
{
	vkWaitForFences (mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max ());
	vkResetFences (mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR (mainDevice.logicalDevice,
						   swapchain,
						   std::numeric_limits<uint64_t>::max (),
						   imagesAvailable[currentFrame],
						   VK_NULL_HANDLE,
						   &imageIndex);

	VkSubmitInfo submitInfo {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imagesAvailable[currentFrame];
	VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &rendersFinished[currentFrame];

 	VkResult result = vkQueueSubmit (graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to submit command buffer to queue...");
	}

	VkPresentInfoKHR presentInfo {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &rendersFinished[currentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR (presentationQueue, &presentInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to present an image...");
	}

	currentFrame = (currentFrame + 1) % MaxFrameDraws;
}


void VulkanRenderer::CreateSynchronization ()
{
	imagesAvailable.resize (MaxFrameDraws);
	rendersFinished.resize (MaxFrameDraws);
	drawFences.resize (MaxFrameDraws);

	VkSemaphoreCreateInfo semaphoreCreateInfo {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MaxFrameDraws; ++i) {
		if (vkCreateSemaphore (mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imagesAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore (mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &rendersFinished[i]) != VK_SUCCESS ||
			vkCreateFence (mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error ("Failed to create a semaphore / fence...");
		}
	}

}
