#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Engine.h>
#include <VkBootstrap.h>

#include <vulkan_tool.h>

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
	init_commands();
	init_sync_structures();

}

void Engine::run() {
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		draw();
	}
}

void Engine::cleanup() {
	_mainDeletionQueue.flush();
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Engine::draw() {

	FrameData& currentFrame = get_current_frame();

	uint64_t TIME_OUT = 100000000;
	vkWaitForFences(_device, 1, &currentFrame.renderFence, VK_TRUE, TIME_OUT);

	vkResetFences(_device, 1, &currentFrame.renderFence);

	uint32_t swapchainImageIndex;
	vkAcquireNextImageKHR(_device, _swapchain, TIME_OUT, currentFrame.swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

	vkResetCommandBuffer(currentFrame.mainCommandBuffer, 0);

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	

	vkBeginCommandBuffer(currentFrame.mainCommandBuffer, &cmdBeginInfo);

	//......

	vkEndCommandBuffer(currentFrame.mainCommandBuffer);

	VkCommandBufferSubmitInfo cmdInfo{};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	cmdInfo.commandBuffer = currentFrame.mainCommandBuffer;

	VkSemaphoreSubmitInfo waitInfo{};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitInfo.semaphore = currentFrame.swapchainSemaphore;
	waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;

	VkSemaphoreSubmitInfo signalInfo{};
	signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalInfo.semaphore = _renderSemaphores[swapchainImageIndex];
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

	VkSubmitInfo2 submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit.waitSemaphoreInfoCount = 1;
	submit.pWaitSemaphoreInfos = &waitInfo;
	submit.signalSemaphoreInfoCount = 1;
	submit.pSignalSemaphoreInfos = &signalInfo;
	submit.commandBufferInfoCount = 1;
	submit.pCommandBufferInfos = &cmdInfo;

	vkQueueSubmit2(_graphicsQueue, 1, &submit, currentFrame.renderFence);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_renderSemaphores[swapchainImageIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.pImageIndices = &swapchainImageIndex;

	vkQueuePresentKHR(_graphicsQueue, &presentInfo);

	_frameNumber++;
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

	_renderSemaphores.resize(_swapchainImages.size());

	VkSemaphoreCreateInfo semInfo{};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < _swapchainImages.size(); i++) {
		vkCreateSemaphore(_device, &semInfo, nullptr, &_renderSemaphores[i]);
	}

	_mainDeletionQueue.push_function([=]() {
		for (auto it = _swapchainImageViews.begin(); it != _swapchainImageViews.end(); ++it) {
			vkDestroyImageView(_device, *it, nullptr);
		}
		for (auto sem : _renderSemaphores) {
			vkDestroySemaphore(_device, sem, nullptr);
		}
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
		});
}

void Engine::init_commands() {
	VkCommandPoolCreateInfo cmdCreateInfo{};
	cmdCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cmdCreateInfo.queueFamilyIndex = _graphicsQueueFamily;

	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		vkCreateCommandPool(_device, &cmdCreateInfo, nullptr, &_frames[i].commandPool);

		VkCommandBufferAllocateInfo cmdAllocInfo{};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.commandPool = _frames[i].commandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i].mainCommandBuffer);
	}


	

	_mainDeletionQueue.push_function([this]() {
		for (int i = 0; i < FRAME_OVERLAP; ++i)
		vkDestroyCommandPool(_device, _frames[i].commandPool, nullptr);
		});
}

void Engine::init_sync_structures() {
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i].renderFence);
	}
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = 0;

	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i].swapchainSemaphore);
	}
	

	_mainDeletionQueue.push_function([this]() {
		for (int i = 0; i < FRAME_OVERLAP; ++i) {
			vkDestroyFence(_device, _frames[i].renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i].swapchainSemaphore, nullptr);
		}
		});
}