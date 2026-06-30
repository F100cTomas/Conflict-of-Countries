#include "coc.hpp"
#include "common.hpp"
#include <vulkan/vulkan_core.h>
namespace Engine {
void get_surface_format(VkSurfaceFormatKHR& surface_format) {
	uint32_t surface_format_count;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface_khr, &surface_format_count, nullptr) != VK_SUCCESS)
		error("Failed to get Vulkan surface formats");
	VkSurfaceFormatKHR* surface_formats = new VkSurfaceFormatKHR[surface_format_count];
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface_khr, &surface_format_count, surface_formats)
	    != VK_SUCCESS)
		error("Failed to get Vulkan surface formats");
	for (uint32_t i = 0; i < surface_format_count; i++) {
		if (surface_formats[i].format == VkFormat::VK_FORMAT_B8G8R8A8_SRGB
		    && surface_formats[i].colorSpace == VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surface_format = surface_formats[i];
			delete[] surface_formats;
			return;
		}
	}
	error("Failed to find suitable Vulkan surface format");
}
VkPresentModeKHR get_present_mode() {
	uint32_t present_mode_count;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, nullptr)
	    != VK_SUCCESS)
		error("Failed to get Vulkan surface present modes");
	VkPresentModeKHR* present_modes = new VkPresentModeKHR[present_mode_count];
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, present_modes)
	    != VK_SUCCESS)
		error("Failed to get Vulkan surface present modes");
	for (uint32_t i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR) {
			VkPresentModeKHR present_mode = present_modes[i];
			delete[] present_modes;
			return present_mode;
		}
	}
	VkPresentModeKHR present_mode = present_modes[0];
	delete[] present_modes;
	return present_mode;
}
} // namespace Engine
