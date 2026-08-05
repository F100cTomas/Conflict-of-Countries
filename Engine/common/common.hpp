#pragma once
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
namespace Engine {
struct Synchronization {
	VkSemaphore     present_complete = VK_NULL_HANDLE;
	VkSemaphore     render_finished  = VK_NULL_HANDLE;
	VkFence         draw_fence       = VK_NULL_HANDLE;
	VkCommandBuffer command_buffer   = VK_NULL_HANDLE;
};
struct Vertex {
	glm::vec2                                     pos;
	glm::vec3                                     color;
	static inline VkVertexInputBindingDescription get_binding_description() {
		return {.binding = 0, .stride = sizeof(Vertex), .inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX};
	}
	static inline std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() {
		return std::array<VkVertexInputAttributeDescription, 2>(
		    {{.location = 0, .binding = 0, .format = VkFormat::VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, pos)},
		     {.location = 1,
		      .binding  = 0,
		      .format   = VkFormat::VK_FORMAT_R32G32B32_SFLOAT,
		      .offset   = offsetof(Vertex, color)}});
	}
};
constexpr uint8_t          frame_count = 2;
extern VkInstance          instance;
extern VkPhysicalDevice    physical_device;
extern VkDevice            device;
extern VkQueue             queue;
extern VkPipeline          pipeline;
extern VkCommandPool       command_pool;
extern VkBuffer            vertex_buffer;
extern VkSurfaceKHR        surface_khr;
extern VkSwapchainKHR      swapchain_khr;
extern Synchronization     synchronization[frame_count];
extern uint8_t             current_frame;
extern VkSurfaceFormatKHR  surface_format;
extern VkPresentModeKHR    present_mode;
extern uint32_t            swapchain_image_count;
extern VkImage*            swapchain_images;
extern VkImageView*        swapchain_image_views;
inline std::vector<Vertex> verticies = {
    {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, {{1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}, {{-1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},  {{1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},  {{-1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
};
void init_vulkan();
// width and height in physical pixels, not logical
void resize(uint32_t width, uint32_t height);
// width and height in physical pixels, not logical
extern void* data;
void         set_angle(float angle);
void         draw(uint32_t width, uint32_t height);
void         deinit_vulkan();
/*
 * Vulkan object initializers
 */
VkInstance       create_instance();
VkPhysicalDevice create_physical_device(VkInstance instance);
uint32_t         find_queue_family_index(VkPhysicalDevice physical_device);
VkDevice         create_device(VkPhysicalDevice physical_device, uint32_t queue_family_index);
VkQueue          create_queue(VkDevice device, uint32_t queue_family_index);
VkShaderModule   create_shader_module(VkDevice device);
VkPipelineLayout create_pipeline_layout(VkDevice device);
VkPipeline       create_pipeline(VkDevice device, VkShaderModule shader_module, VkPipelineLayout pipeline_layout);
VkCommandPool    create_command_pool(VkDevice device, uint32_t queue_family_index);
VkBuffer         create_vertex_buffer(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_family_index);
/*
 * Vulkan helper functions
 */
void             get_surface_format(VkSurfaceFormatKHR& surface_format);
VkPresentModeKHR get_present_mode();
} // namespace Engine
