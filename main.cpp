#include <iostream>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"

GLFWwindow* mainWindow;
VulkanRenderer vkRenderer;

static void InitWindow (const std::string& windowTitle = "Default title", const int width = 800, const int height = 600)
{
	glfwInit ();

	glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

	mainWindow = glfwCreateWindow (width, height, windowTitle.c_str(), nullptr, nullptr);
}

int main ()
{
	InitWindow ();

	if (vkRenderer.InitRenderer (mainWindow) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	while (!glfwWindowShouldClose (mainWindow)) {
		glfwPollEvents ();
	}

	vkRenderer.CleanUp ();

	glfwDestroyWindow (mainWindow);
	glfwTerminate ();

	return 0;
}
