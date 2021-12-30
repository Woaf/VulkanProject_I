#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <glm/mat4x4.hpp>

int main ()
{
	glfwInit ();
	glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* mainWindow = glfwCreateWindow (1280, 720, "Test window", nullptr, nullptr);

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, nullptr);

	std::cout << "Extension count: " << extensionCount << std::endl;

	glm::mat4 matrix (1.0f);
	glm::vec4 vector (1.0f);
	auto testresult = matrix * vector;


	while (!glfwWindowShouldClose (mainWindow)) {
		glfwPollEvents ();
		glfwSwapBuffers (mainWindow);
	}

	glfwDestroyWindow (mainWindow);
	glfwTerminate ();

	return 0;
}
