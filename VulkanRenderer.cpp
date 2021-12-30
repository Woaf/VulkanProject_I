//
// Created by Fazekas Bálint on 2021. 12. 30..
//

#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer ()
{
}


VulkanRenderer::~VulkanRenderer ()
{
}


int VulkanRenderer::InitRenderer (GLFWwindow *newWindow) {
	window = newWindow;

	try {
		CreateInstance ();
		GetPhysicalDevice ();
		CreateLogicalDevice ();
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
			if (strcmp (checkExtension, extension.extensionName)) {
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

	return indices.IsValid ();
}


QueueFamilyIndices VulkanRenderer::GetQueueFamilies (VkPhysicalDevice device ) {
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

	VkDeviceQueueCreateInfo queueCreateInfo {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
	queueCreateInfo.queueCount = 1;
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;

	VkDeviceCreateInfo deviceCreateInfo {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = 0;
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;

	VkPhysicalDeviceFeatures deviceFeatures {};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	VkResult result = vkCreateDevice (mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error ("Failed to create a logical device...");
	}

	vkGetDeviceQueue (mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}
