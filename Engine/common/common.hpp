#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
namespace Engine {
struct Synchronization {
	VkSemaphore     present_complete = VK_NULL_HANDLE;
	VkSemaphore     render_finished  = VK_NULL_HANDLE;
	VkFence         draw_fence       = VK_NULL_HANDLE;
	VkCommandBuffer command_buffer   = VK_NULL_HANDLE;
};
constexpr uint8_t         frame_count = 2;
extern VkInstance         instance;
extern VkPhysicalDevice   physical_device;
extern VkDevice           device;
extern VkPipeline         pipeline;
extern VkQueue            queue;
extern VkCommandPool      command_pool;
extern VkSurfaceKHR       surface_khr;
extern VkSwapchainKHR     swapchain_khr;
extern Synchronization    synchronization[frame_count];
extern uint8_t            current_frame;
extern VkSurfaceFormatKHR surface_format;
extern VkPresentModeKHR   present_mode;
extern uint32_t           swapchain_image_count;
extern VkImage*           swapchain_images;
extern VkImageView*       swapchain_image_views;
void                      init_vulkan();
// width and height in physical pixels, not logical
void resize(uint32_t width, uint32_t height);
// width and height in physical pixels, not logical
void draw(uint32_t width, uint32_t height);
void deinit_vulkan();
} // namespace Engine
