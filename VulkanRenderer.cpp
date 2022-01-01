#include "VulkanRenderer.h"

#include <algorithm>

VulkanRenderer::VulkanRenderer ()
{
}


VulkanRenderer::~VulkanRenderer ()
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
	} catch (const std::runtime_error& runtimeError) {
		std::cerr << "Error: " << runtimeError.what () << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
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


void VulkanRenderer::CleanUp ()
{
	for (auto image : swapchainImages) {
		vkDestroyImageView (mainDevice.logicalDevice, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR (mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR (instance, surface, nullptr);
	vkDestroyDevice (mainDevice.logicalDevice, nullptr);
	vkDestroyInstance (instance, nullptr);
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


void VulkanRenderer::CreateSurface ()
{
	VkResult result = glfwCreateWindowSurface (instance, window, nullptr, &surface);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a surface...");
	}
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
	viewCreateInfo.subresourceRange.levelCount = 1;

	VkImageView imageView;
	VkResult result = vkCreateImageView (mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create an image view..");
	}

	return imageView;
}
