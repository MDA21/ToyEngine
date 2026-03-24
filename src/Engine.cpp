#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Engine.h>

Engine::Engine()
{
}

Engine::~Engine()
{
}

void Engine::init() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	_window = glfwCreateWindow(screenWidth, screenHeight, "Engine Toy", nullptr, nullptr);
	if (_window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}

}

void Engine::run() {
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
	}
}

void Engine::cleanup() {
	glfwDestroyWindow(_window);
	glfwTerminate();
}