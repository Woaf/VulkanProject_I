#pragma once

#ifndef VULKANPROJECT_I_UTILITIES_H
#define VULKANPROJECT_I_UTILITIES_H


const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentationFamily = -1;

	bool IsValid () {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};


struct SwapchainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentationModes;
};


struct SwapchainImage {
	VkImage image;
	VkImageView imageView;
};

#endif //VULKANPROJECT_I_UTILITIES_H
