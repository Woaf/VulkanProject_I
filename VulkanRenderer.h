#pragma once

#ifndef VULKANPROJECT_I_VULKANRENDERER_H
#define VULKANPROJECT_I_VULKANRENDERER_H

#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer ();

	int InitRenderer (GLFWwindow* newWindow);
	void Draw ();
	void CleanUp ();

	~VulkanRenderer ();

private:
	GLFWwindow* window;

	int currentFrame = 0;

	// vk components
		// main components
	VkInstance instance;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<SwapchainImage> swapchainImages;
	std::vector<VkFramebuffer> swapchainFrameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

		// pools
	VkCommandPool graphicsCommandPool;

		// utility components
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

		// synch objects
	std::vector<VkSemaphore> imagesAvailable;
	std::vector<VkSemaphore> rendersFinished;
	std::vector<VkFence> drawFences;


	// vk functions
	void CreateInstance ();
	void CreateLogicalDevice ();
	void CreateSurface ();
	void CreateSwapchain ();
	void CreateRenderPass ();
	void CreateGraphicsPipeline ();
	void CreateFrameBuffers ();
	void CreateCommandPool ();
	void CreateCommandBuffers ();
	void CreateSynchronization ();

	// get methods
	void GetPhysicalDevice ();
	QueueFamilyIndices GetQueueFamilies (VkPhysicalDevice device);
	SwapchainDetails GetSwapchainDetails (VkPhysicalDevice device);

	// record functions
	void RecordCommands ();

	// util functions
	bool CheckInstanceExtensionSupport (const std::vector<const char*>* checkExtensions);
	bool CheckDeviceExtensionSupport (VkPhysicalDevice device);
	bool CheckDeviceSuitable (VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseBestSurfaceFormat (const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode (const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D ChooseSwapExtent (const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView CreateImageView (VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule (const std::vector<char>& code);
};


#endif //VULKANPROJECT_I_VULKANRENDERER_H
