#include "common/common.hpp"
#include "coc.hpp"
#include <algorithm>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#else
#include <vulkan/vulkan_wayland.h>
#endif
namespace Engine {
VkInstance         instance                     = VK_NULL_HANDLE;
VkPhysicalDevice   physical_device              = VK_NULL_HANDLE;
VkDevice           device                       = VK_NULL_HANDLE;
VkQueue            queue                        = VK_NULL_HANDLE;
VkPipeline         pipeline                     = VK_NULL_HANDLE;
VkCommandPool      command_pool                 = VK_NULL_HANDLE;
VkSurfaceKHR       surface_khr                  = VK_NULL_HANDLE;
VkSwapchainKHR     swapchain_khr                = VK_NULL_HANDLE;
Synchronization    synchronization[frame_count] = {{}, {}};
uint8_t            current_frame                = 0;
VkSurfaceFormatKHR surface_format               = {};
VkPresentModeKHR   present_mode                 = {};
uint32_t           swapchain_image_count        = 0;
VkImage*           swapchain_images             = nullptr;
VkImageView*       swapchain_image_views        = nullptr;
void               init_vulkan() {
  instance                         = create_instance();
  physical_device                  = create_physical_device(instance);
  uint32_t queue_family_index      = find_queue_family_index(physical_device);
  device                           = create_device(physical_device, queue_family_index);
  queue                            = create_queue(device, queue_family_index);
  VkShaderModule   shader_module   = create_shader_module(device);
  VkPipelineLayout pipeline_layout = create_pipeline_layout(device);
  pipeline                         = create_pipeline(device, shader_module, pipeline_layout);
  command_pool                     = create_command_pool(device, queue_family_index);
  vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  vkDestroyShaderModule(device, shader_module, nullptr);
}
void deinit_vulkan() {
	vkQueueWaitIdle(queue);
	for (uint32_t i = 0; i < swapchain_image_count; i++)
		vkDestroyImageView(device, swapchain_image_views[i], nullptr);
	if (swapchain_image_views != nullptr) {
		delete[] swapchain_image_views;
		swapchain_image_views = nullptr;
	}
	if (swapchain_images != nullptr) {
		swapchain_images = nullptr;
		delete[] swapchain_images;
	}
	swapchain_image_count = 0;
	present_mode          = {};
	surface_format        = {};
	for (uint8_t i = 0; i < frame_count; i++) {
		vkDestroySemaphore(device, synchronization[i].present_complete, nullptr);
		synchronization[i].present_complete = VK_NULL_HANDLE;
		vkDestroySemaphore(device, synchronization[i].render_finished, nullptr);
		synchronization[i].render_finished = VK_NULL_HANDLE;
		vkDestroyFence(device, synchronization[i].draw_fence, nullptr);
		synchronization[i].draw_fence = VK_NULL_HANDLE;
		vkFreeCommandBuffers(device, command_pool, 1, &synchronization[i].command_buffer);
		synchronization[i].command_buffer = VK_NULL_HANDLE;
	}
	vkDestroySwapchainKHR(device, swapchain_khr, nullptr);
	swapchain_khr = VK_NULL_HANDLE;
	vkDestroySurfaceKHR(instance, surface_khr, nullptr);
	surface_khr = VK_NULL_HANDLE;
	vkDestroyCommandPool(device, command_pool, nullptr);
	command_pool = VK_NULL_HANDLE;
	vkDestroyPipeline(device, pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;
	queue    = VK_NULL_HANDLE;
	vkDestroyDevice(device, nullptr);
	physical_device = VK_NULL_HANDLE;
	vkDestroyInstance(instance, nullptr);
	instance = VK_NULL_HANDLE;
}
void resize(uint32_t width, uint32_t height) {
	VkSurfaceCapabilitiesKHR capabilities{};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface_khr, &capabilities) != VK_SUCCESS)
		error("Failed to get Vulkan surface capabilities");
	width  = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	get_surface_format(surface_format);
	present_mode = get_present_mode();
	vkQueueWaitIdle(queue);
	current_frame = 0;
	for (uint32_t i = 0; i < swapchain_image_count; i++)
		vkDestroyImageView(device, swapchain_image_views[i], nullptr);
	if (swapchain_images != nullptr)
		delete[] swapchain_images;
	if (swapchain_image_views != nullptr)
		delete[] swapchain_image_views;
	VkSwapchainKHR           swapchain_old = swapchain_khr;
	VkSwapchainCreateInfoKHR swapchain_info{};
	swapchain_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface               = surface_khr;
	swapchain_info.minImageCount         = capabilities.minImageCount;
	swapchain_info.imageFormat           = surface_format.format;
	swapchain_info.imageColorSpace       = surface_format.colorSpace;
	swapchain_info.imageExtent           = VkExtent2D{.width = width, .height = height};
	swapchain_info.imageArrayLayers      = 1;
	swapchain_info.imageUsage            = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode      = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = 0;
	swapchain_info.pQueueFamilyIndices   = nullptr;
	swapchain_info.preTransform          = capabilities.currentTransform;
	swapchain_info.compositeAlpha        = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode           = present_mode;
	swapchain_info.clipped               = VK_TRUE;
	swapchain_info.oldSwapchain          = swapchain_old;
	if (vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain_khr) != VK_SUCCESS)
		error("Failed to create Vulkan swapchain");
	vkDestroySwapchainKHR(device, swapchain_old, nullptr);
	swapchain_image_count = 0;
	if (vkGetSwapchainImagesKHR(device, swapchain_khr, &swapchain_image_count, nullptr) != VK_SUCCESS)
		error("Failed to get Vulkan swapchain images");
	swapchain_images = new VkImage[swapchain_image_count];
	if (vkGetSwapchainImagesKHR(device, swapchain_khr, &swapchain_image_count, swapchain_images) != VK_SUCCESS)
		error("Failed to get Vulkan swapchain images");
	swapchain_image_views = new VkImageView[swapchain_image_count];
	VkImageViewCreateInfo image_view_info{};
	image_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.viewType                        = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format                          = surface_format.format;
	image_view_info.components.r                    = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.components.g                    = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.components.b                    = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.components.a                    = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_info.subresourceRange.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_info.subresourceRange.baseMipLevel   = 0;
	image_view_info.subresourceRange.levelCount     = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount     = 1;
	for (uint32_t i = 0; i < swapchain_image_count; i++) {
		image_view_info.image = swapchain_images[i];
		if (vkCreateImageView(device, &image_view_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS)
			error("Failed to create Vulkan image view");
	}
	for (uint8_t i = 0; i < frame_count; i++) {
		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (synchronization[i].present_complete == VK_NULL_HANDLE
		    && vkCreateSemaphore(device, &semaphore_info, nullptr, &synchronization[i].present_complete) != VK_SUCCESS)
			error("Failed to create Vulkan semaphore");
		if (synchronization[i].render_finished == VK_NULL_HANDLE
		    && vkCreateSemaphore(device, &semaphore_info, nullptr, &synchronization[i].render_finished) != VK_SUCCESS)
			error("Failed to create Vulkan semaphore");
		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;
		if (synchronization[i].draw_fence == VK_NULL_HANDLE
		    && vkCreateFence(device, &fence_info, nullptr, &synchronization[i].draw_fence) != VK_SUCCESS)
			error("Failed to create Vulkan fence");
		VkCommandBufferAllocateInfo command_buffer_info{};
		command_buffer_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_info.commandPool        = command_pool;
		command_buffer_info.level              = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_info.commandBufferCount = 1;
		if (synchronization[i].command_buffer == VK_NULL_HANDLE
		    && vkAllocateCommandBuffers(device, &command_buffer_info, &synchronization[i].command_buffer) != VK_SUCCESS)
			error("Failed to create Vulkan fence");
	}
}
void draw(uint32_t width, uint32_t height) {
	if (vkWaitForFences(device, 1, &synchronization[current_frame].draw_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
		error("Failed to wait for Vulkan fences");
	if (vkResetFences(device, 1, &synchronization[current_frame].draw_fence) != VK_SUCCESS)
		error("Failed to reset Vulkan fences");
	VkAcquireNextImageInfoKHR next_image_info{};
	next_image_info.sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	next_image_info.swapchain  = swapchain_khr;
	next_image_info.timeout    = UINT64_MAX;
	next_image_info.semaphore  = synchronization[current_frame].present_complete;
	next_image_info.fence      = VK_NULL_HANDLE;
	next_image_info.deviceMask = 1;
	uint32_t image_index       = 0;
	if (vkAcquireNextImage2KHR(device, &next_image_info, &image_index) != VK_SUCCESS)
		error("Failed to acquire next Vulkan image");
	if (vkResetCommandBuffer(synchronization[current_frame].command_buffer, 0) != VK_SUCCESS)
		error("Failed to reset command buffer");
	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(synchronization[current_frame].command_buffer, &command_buffer_begin_info) != VK_SUCCESS)
		error("Failed to begin command buffer");
	VkImageMemoryBarrier2 barrier1{};
	barrier1.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier1.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier1.srcAccessMask                   = 0;
	barrier1.dstStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier1.dstAccessMask                   = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier1.oldLayout                       = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	barrier1.newLayout                       = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier1.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier1.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier1.image                           = swapchain_images[image_index];
	barrier1.subresourceRange.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	barrier1.subresourceRange.baseMipLevel   = 0;
	barrier1.subresourceRange.levelCount     = 1;
	barrier1.subresourceRange.baseArrayLayer = 0;
	barrier1.subresourceRange.layerCount     = 1;
	VkDependencyInfo dependency_info1{};
	dependency_info1.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info1.dependencyFlags          = 0;
	dependency_info1.memoryBarrierCount       = 0;
	dependency_info1.pMemoryBarriers          = nullptr;
	dependency_info1.bufferMemoryBarrierCount = 0;
	dependency_info1.pBufferMemoryBarriers    = nullptr;
	dependency_info1.imageMemoryBarrierCount  = 1;
	dependency_info1.pImageMemoryBarriers     = &barrier1;
	vkCmdPipelineBarrier2(synchronization[current_frame].command_buffer, &dependency_info1);
	VkClearValue clear_color{};
	clear_color.color.float32[0] = 0.0f;
	clear_color.color.float32[1] = 0.0f;
	clear_color.color.float32[2] = 0.0f;
	clear_color.color.float32[3] = 0.0f;
	VkRenderingAttachmentInfo rendering_attachment_info{};
	rendering_attachment_info.sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	rendering_attachment_info.imageView          = swapchain_image_views[image_index];
	rendering_attachment_info.imageLayout        = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	rendering_attachment_info.resolveMode        = VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE;
	rendering_attachment_info.resolveImageView   = VK_NULL_HANDLE;
	rendering_attachment_info.resolveImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	rendering_attachment_info.loadOp             = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
	rendering_attachment_info.storeOp            = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
	rendering_attachment_info.clearValue         = clear_color;
	VkRenderingInfo rendering_info{};
	rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea.offset    = VkOffset2D{.x = 0, .y = 0};
	rendering_info.renderArea.extent    = VkExtent2D{.width = width, .height = height};
	rendering_info.layerCount           = 1;
	rendering_info.viewMask             = 0;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments    = &rendering_attachment_info;
	rendering_info.pDepthAttachment     = nullptr;
	rendering_info.pStencilAttachment   = nullptr;
	vkCmdBeginRendering(synchronization[current_frame].command_buffer, &rendering_info);
	vkCmdBindPipeline(synchronization[current_frame].command_buffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  pipeline);
	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(width);
	viewport.height   = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(synchronization[current_frame].command_buffer, 0, 1, &viewport);
	VkRect2D scissor{};
	scissor.offset = VkOffset2D{.x = 0, .y = 0};
	scissor.extent = VkExtent2D{.width = width, .height = height};
	vkCmdSetScissor(synchronization[current_frame].command_buffer, 0, 1, &scissor);
	vkCmdDraw(synchronization[current_frame].command_buffer, 3, 1, 0, 0);
	vkCmdEndRendering(synchronization[current_frame].command_buffer);
	VkImageMemoryBarrier2 barrier2{};
	barrier2.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier2.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier2.srcAccessMask                   = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier2.dstStageMask                    = VK_PIPELINE_STAGE_2_NONE;
	barrier2.dstAccessMask                   = 0;
	barrier2.oldLayout                       = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier2.newLayout                       = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier2.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier2.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier2.image                           = swapchain_images[image_index];
	barrier2.subresourceRange.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	barrier2.subresourceRange.baseMipLevel   = 0;
	barrier2.subresourceRange.levelCount     = 1;
	barrier2.subresourceRange.baseArrayLayer = 0;
	barrier2.subresourceRange.layerCount     = 1;
	VkDependencyInfo dependency_info2{};
	dependency_info2.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info2.dependencyFlags          = 0;
	dependency_info2.memoryBarrierCount       = 0;
	dependency_info2.pMemoryBarriers          = nullptr;
	dependency_info2.bufferMemoryBarrierCount = 0;
	dependency_info2.pBufferMemoryBarriers    = nullptr;
	dependency_info2.imageMemoryBarrierCount  = 1;
	dependency_info2.pImageMemoryBarriers     = &barrier2;
	vkCmdPipelineBarrier2(synchronization[current_frame].command_buffer, &dependency_info2);
	if (vkEndCommandBuffer(synchronization[current_frame].command_buffer) != VK_SUCCESS)
		error("Failed to end Vulkan command buffer");
	VkSemaphoreSubmitInfo wait_semaphore_submit_info{};
	wait_semaphore_submit_info.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_info.semaphore   = synchronization[current_frame].present_complete;
	wait_semaphore_submit_info.value       = 0;
	wait_semaphore_submit_info.deviceIndex = 0;
	VkCommandBufferSubmitInfo command_buffer_submit_info{};
	command_buffer_submit_info.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command_buffer_submit_info.commandBuffer = synchronization[current_frame].command_buffer;
	command_buffer_submit_info.deviceMask    = 0;
	VkSemaphoreSubmitInfo signal_semaphore_submit_info{};
	signal_semaphore_submit_info.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal_semaphore_submit_info.semaphore   = synchronization[current_frame].render_finished;
	signal_semaphore_submit_info.value       = 0;
	signal_semaphore_submit_info.deviceIndex = 0;
	VkSubmitInfo2 submit_info{};
	submit_info.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.waitSemaphoreInfoCount   = 1;
	submit_info.pWaitSemaphoreInfos      = &wait_semaphore_submit_info;
	submit_info.commandBufferInfoCount   = 1;
	submit_info.pCommandBufferInfos      = &command_buffer_submit_info;
	submit_info.signalSemaphoreInfoCount = 1;
	submit_info.pSignalSemaphoreInfos    = &signal_semaphore_submit_info;
	if (vkQueueSubmit2(queue, 1, &submit_info, synchronization[current_frame].draw_fence) != VK_SUCCESS)
		error("Failed to submit to Vulkan queue");
	VkPresentInfoKHR present_info{};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &synchronization[current_frame].render_finished;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &swapchain_khr;
	present_info.pImageIndices      = &image_index;
	present_info.pResults           = nullptr;
	if (vkQueuePresentKHR(queue, &present_info) != VK_SUCCESS)
		error("Failed to present");
	current_frame = (current_frame + 1) % frame_count;
}
} // namespace Engine
