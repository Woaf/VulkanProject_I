#pragma once

#ifndef VULKANPROJECT_I_UTILITIES_H
#define VULKANPROJECT_I_UTILITIES_H

#include <fstream>


constexpr int MaxFrameDraws = 2;


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


static std::vector<char> ReadFile (const std::string& fileName)
{
	std::ifstream file (fileName, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error ("Failed to open a file...");
	}

	size_t fileSize = static_cast<size_t> (file.tellg ());
	std::vector<char> fileBuffer (fileSize);

	file.seekg (0);

	file.read (fileBuffer.data (), fileSize);

	file.close ();

	return fileBuffer;
}


#endif //VULKANPROJECT_I_UTILITIES_H
