//
// Created by Fazekas Bálint on 2021. 12. 30..
//

#pragma once

#ifndef VULKANPROJECT_I_VULKANRENDERER_H
#define VULKANPROJECT_I_VULKANRENDERER_H

#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer ();

	int InitRenderer (GLFWwindow* newWindow);
	void CleanUp ();

	~VulkanRenderer ();

private:
	GLFWwindow* window;

	// vk components
	VkInstance instance;

	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;

	VkSurfaceKHR surface;

	// vk functions
	void CreateInstance ();
	void CreateLogicalDevice ();
	void CreateSurface ();

	// get methods
	void GetPhysicalDevice ();
	QueueFamilyIndices GetQueueFamilies (VkPhysicalDevice device);
	SwapchainDetails GetSwapchainDetails (VkPhysicalDevice device);

	// util functions
	bool CheckInstanceExtensionSupport (const std::vector<const char*>* checkExtensions);
	bool CheckDeviceExtensionSupport (VkPhysicalDevice device);
	bool CheckDeviceSuitable (VkPhysicalDevice device);


};


#endif //VULKANPROJECT_I_VULKANRENDERER_H
