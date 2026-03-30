#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <queue>
#include <functional>
#include <vk_mem_alloc.h>
#include <fstream>

#include "vulkan_tool.h"

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier2 barrier2{};
	barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
	barrier2.oldLayout = currentLayout;
	barrier2.newLayout = newLayout;
	barrier2.image = image;
	barrier2.subresourceRange = range;
	barrier2.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier2.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier2.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
	barrier2.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier2;

	vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

