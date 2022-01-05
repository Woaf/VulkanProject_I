#include <iostream>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"

GLFWwindow* mainWindow;
VulkanRenderer vkRenderer;

static void HandleKeyboardInput (GLFWwindow* window, int key, int status, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose (window, GLFW_TRUE);
	} else {
		std::cout << "Key> " << key << std::endl;
	}
}

static void InitWindow (const std::string& windowTitle = "Default title", const int width = 800, const int height = 600)
{
	glfwInit ();

	glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

	mainWindow = glfwCreateWindow (width, height, windowTitle.c_str(), nullptr, nullptr);
	glfwSetKeyCallback (mainWindow, HandleKeyboardInput);
}

int main ()
{
	InitWindow ("MoltenVK window", 600, 600);

	if (vkRenderer.InitRenderer (mainWindow) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	while (!glfwWindowShouldClose (mainWindow)) {
		glfwPollEvents ();
		vkRenderer.Draw ();
	}

	vkRenderer.CleanUp ();

	glfwDestroyWindow (mainWindow);
	glfwTerminate ();

	return 0;
}
