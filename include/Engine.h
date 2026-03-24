#pragma once
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Engine
{
public:
	Engine();
	~Engine();

	void init();

	void run();

	void cleanup();


private:
	int screenWidth = 800;
	int screenHeight = 600;
	GLFWwindow* _window;
};

