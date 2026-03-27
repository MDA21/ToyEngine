#pragma once
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <queue>
#include <functional>
#include <vk_mem_alloc.h>

struct deletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()> function) {
		deletors.push_back(std::move(function));
	}

	void flush() {
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)();
		}
		deletors.clear();
	}
};

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

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	deletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	VkExtent2D _swapchainExtent;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	void init_vulkan();

	void init_swapchain();

};

