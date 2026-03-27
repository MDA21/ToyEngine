#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Engine.h>
#include <VkBootstrap.h>

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

	init_vulkan();
	init_swapchain();

}

void Engine::run() {
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
	}
}

void Engine::cleanup() {
	_mainDeletionQueue.flush();
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Engine::init_vulkan() {
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Toy Engine")
		.request_validation_layers(true)
		.require_api_version(1, 3, 0)
		.build();

	if (!inst_ret) {
		std::cerr << "致命错误：无法创建 Vulkan 实例！原因: " << inst_ret.error().message() << "\n";
		throw std::runtime_error("Vulkan Instance creation failed!");
	}


	vkb::Instance vkb_inst = inst_ret.value();

	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;



	_mainDeletionQueue.push_function([=]() {
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);
		vkDestroyInstance(_instance, nullptr);
		});

	if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	_mainDeletionQueue.push_function([=]() {
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		});

	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physical_device = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_surface(_surface)
		.select()
		.value();

	_chosenGPU = physical_device.physical_device;

	vkb::DeviceBuilder device_builder{ physical_device };
	vkb::Device vkb_device = device_builder.build().value();
	_device = vkb_device.device;

	_mainDeletionQueue.push_function([=]() {
		vkDestroyDevice(_device, nullptr);

		});

	_graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();


	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.physicalDevice = _chosenGPU;
	allocatorCreateInfo.device = _device;
	allocatorCreateInfo.instance = _instance;
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

	vmaCreateAllocator(&allocatorCreateInfo, &_allocator);

	_mainDeletionQueue.push_function([=]() {
		vmaDestroyAllocator(_allocator);
		});

}

void Engine::init_swapchain() {
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU, _device, _surface };
	vkb::Result<vkb::Swapchain> swap_ret = swapchainBuilder
		.use_default_format_selection()
		.set_desired_extent(screenWidth, screenHeight)
		.build();
	if (!swap_ret) {
		std::cerr << "致命错误：无法创建交换链！原因: " << swap_ret.error().message() << "\n";
		throw std::runtime_error("Failed to create swapchain!");
	}

	vkb::Swapchain vkb_swapchain = swap_ret.value();
	_swapchain = vkb_swapchain.swapchain;
	_swapchainImageFormat = vkb_swapchain.image_format;
	_swapchainExtent = vkb_swapchain.extent;

	_swapchainImages = vkb_swapchain.get_images().value();
	_swapchainImageViews = vkb_swapchain.get_image_views().value();

	_mainDeletionQueue.push_function([=]() {
		for (auto it = _swapchainImageViews.begin(); it != _swapchainImageViews.end(); ++it) {
			vkDestroyImageView(_device, *it, nullptr);
		}
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
		});
}