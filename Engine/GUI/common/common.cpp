#include "common/common.hpp"
#include "coc.hpp"
#include "linux/linux.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
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
VkPipeline         pipeline                     = VK_NULL_HANDLE;
VkQueue            queue                        = VK_NULL_HANDLE;
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
  VkApplicationInfo app_info{};
  app_info.sType                          = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext                          = nullptr;
  app_info.pApplicationName               = "Conflict of Countries";
  app_info.applicationVersion             = VK_MAKE_VERSION(0, 0, 0);
  app_info.pEngineName                    = "No Engine";
  app_info.engineVersion                  = VK_MAKE_VERSION(0, 0, 0);
  app_info.apiVersion                     = VK_API_VERSION_1_4;
  const char* const instance_extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
	    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
	    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
	};
#ifdef NDEBUG
	constexpr const char* const* layers      = nullptr;
	constexpr size_t             layer_count = 0;
#else
	const char* const layers[]    = {"VK_LAYER_KHRONOS_validation"};
	constexpr size_t  layer_count = sizeof(layers) / sizeof(const char*);
#endif
	VkInstanceCreateInfo instance_info{};
	instance_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pNext                   = nullptr;
	instance_info.pApplicationInfo        = &app_info;
	instance_info.enabledLayerCount       = layer_count;
	instance_info.ppEnabledLayerNames     = layers;
	instance_info.enabledExtensionCount   = sizeof(instance_extensions) / sizeof(const char*);
	instance_info.ppEnabledExtensionNames = instance_extensions;
	if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS)
		error("Failed to create Vulkan instance");
	uint32_t physical_device_count = 0;
	if (vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr) != VK_SUCCESS || physical_device_count == 0)
		error("No Vulkan devices found");
	VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
	if (vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices) != VK_SUCCESS)
		error("Failed to get Vulkan physical devices");
	physical_device = *physical_devices;
	delete[] physical_devices;
	uint32_t queue_family_property_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_family_property_count, nullptr);
	VkQueueFamilyProperties2* queue_family_properties = new VkQueueFamilyProperties2[queue_family_property_count];
	for (uint32_t i = 0; i < queue_family_property_count; i++) {
		queue_family_properties[i].sType                 = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		queue_family_properties[i].pNext                 = VK_NULL_HANDLE;
		queue_family_properties[i].queueFamilyProperties = {};
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_family_property_count, queue_family_properties);
	uint32_t queue_family_index;
	for (uint32_t i = 0; i < queue_family_property_count; i++) {
		if (queue_family_properties[i].queueFamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT) {
			queue_family_index = i;
			goto queue_family_found;
		}
	}
	error("Queue family not found");
	return;
queue_family_found:
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT physical_device_extended_dynamic_state_features_ext{};
	physical_device_extended_dynamic_state_features_ext.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	physical_device_extended_dynamic_state_features_ext.extendedDynamicState = VK_TRUE;
	VkPhysicalDeviceVulkan13Features physical_device_vulkan13_features{};
	physical_device_vulkan13_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	physical_device_vulkan13_features.pNext            = &physical_device_extended_dynamic_state_features_ext;
	physical_device_vulkan13_features.synchronization2 = VK_TRUE;
	physical_device_vulkan13_features.dynamicRendering = VK_TRUE;
	VkPhysicalDeviceVulkan11Features physical_device_vulkan11_features{};
	physical_device_vulkan11_features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	physical_device_vulkan11_features.pNext                = &physical_device_vulkan13_features;
	physical_device_vulkan11_features.shaderDrawParameters = VK_TRUE;
	VkPhysicalDeviceFeatures2 physical_device_features2{};
	physical_device_features2.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physical_device_features2.pNext        = &physical_device_vulkan11_features;
	float                   queue_priority = 1.0f;
	VkDeviceQueueCreateInfo device_queue_info{};
	device_queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.queueFamilyIndex = queue_family_index;
	device_queue_info.queueCount       = 1;
	device_queue_info.pQueuePriorities = &queue_priority;
	const char* device_extensions[]    = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
	VkDeviceCreateInfo device_info{};
	device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext                   = &physical_device_features2;
	device_info.queueCreateInfoCount    = 1;
	device_info.pQueueCreateInfos       = &device_queue_info;
	device_info.enabledExtensionCount   = 1;
	device_info.ppEnabledExtensionNames = device_extensions;
	device_info.pEnabledFeatures        = VK_NULL_HANDLE;
	if (vkCreateDevice(physical_device, &device_info, nullptr, &device) != VK_SUCCESS)
		error("Failed to create Vulkan device");
	VkDeviceQueueInfo2 queue_info2{};
	queue_info2.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	queue_info2.queueFamilyIndex = queue_family_index;
	queue_info2.queueIndex       = 0;
	vkGetDeviceQueue2(device, &queue_info2, &queue);
	FILE* shader_file = std::fopen("shader.spv", "rb");
	if (shader_file == nullptr)
		error("Failed to open shader file");
	if (std::fseek(shader_file, 0, SEEK_END))
		error("std::fseek() failed");
	size_t shader_size;
	{
		long shader_size_long = std::ftell(shader_file);
		if (shader_size_long == -1L)
			error("std::ftell() failed");
		shader_size = shader_size_long;
	}
	if (shader_size % 4 != 0)
		error("Bad shader file");
	std::rewind(shader_file);
	uint32_t* shader_data = new uint32_t[shader_size / sizeof(uint32_t)];
	if (std::fread(shader_data, 1, shader_size, shader_file) != shader_size)
		error("std::fread() failed");
	static_cast<void>(std::fclose(shader_file));
	VkShaderModuleCreateInfo shader_module_info{};
	shader_module_info.sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.codeSize  = shader_size;
	shader_module_info.pCode     = shader_data;
	VkShaderModule shader_module = VK_NULL_HANDLE;
	if (vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module) != VK_SUCCESS)
		error("Failed to load shaders");
	delete[] shader_data;
	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage               = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module              = shader_module;
	vert_shader_stage_info.pName               = "vertMain";
	vert_shader_stage_info.pSpecializationInfo = nullptr;
	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType                              = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage                              = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module                             = shader_module;
	frag_shader_stage_info.pName                              = "fragMain";
	frag_shader_stage_info.pSpecializationInfo                = nullptr;
	VkPipelineShaderStageCreateInfo      shader_stage_infos[] = {vert_shader_stage_info, frag_shader_stage_info};
	VkPipelineVertexInputStateCreateInfo vertex_input_state_info{};
	vertex_input_state_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_info.vertexBindingDescriptionCount   = 0;
	vertex_input_state_info.pVertexAttributeDescriptions    = nullptr;
	vertex_input_state_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_info.pVertexAttributeDescriptions    = nullptr;
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{};
	input_assembly_state_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_info.topology               = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_info.primitiveRestartEnable = VK_FALSE;
	VkPipelineViewportStateCreateInfo viewport_state_info{};
	viewport_state_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.viewportCount = 1;
	viewport_state_info.pViewports    = nullptr;
	viewport_state_info.scissorCount  = 1;
	viewport_state_info.pScissors     = nullptr;
	VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
	rasterization_state_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_info.depthClampEnable        = VK_FALSE;
	rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_info.polygonMode             = VkPolygonMode::VK_POLYGON_MODE_FILL;
	rasterization_state_info.cullMode                = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	rasterization_state_info.frontFace               = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
	rasterization_state_info.depthBiasEnable         = VK_FALSE;
	rasterization_state_info.depthBiasConstantFactor = 0.0f;
	rasterization_state_info.depthBiasClamp          = 0.0f;
	rasterization_state_info.depthBiasSlopeFactor    = 1.0f;
	rasterization_state_info.lineWidth               = 1.0f;
	VkPipelineMultisampleStateCreateInfo multisample_state_info{};
	multisample_state_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_info.rasterizationSamples  = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	multisample_state_info.sampleShadingEnable   = VK_FALSE;
	multisample_state_info.minSampleShading      = 0.0f;
	multisample_state_info.pSampleMask           = nullptr;
	multisample_state_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_info.alphaToOneEnable      = VK_FALSE;
	VkPipelineColorBlendAttachmentState color_blend_attachement{};
	color_blend_attachement.blendEnable         = VK_FALSE;
	color_blend_attachement.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
	color_blend_attachement.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
	color_blend_attachement.colorBlendOp        = VkBlendOp::VK_BLEND_OP_ADD;
	color_blend_attachement.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
	color_blend_attachement.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
	color_blend_attachement.alphaBlendOp        = VkBlendOp::VK_BLEND_OP_ADD;
	color_blend_attachement.colorWriteMask =
	    VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
	    | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
	color_blend_state_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_info.logicOpEnable              = VK_FALSE;
	color_blend_state_info.logicOp                    = VkLogicOp::VK_LOGIC_OP_COPY;
	color_blend_state_info.attachmentCount            = 1;
	color_blend_state_info.pAttachments               = &color_blend_attachement;
	VkDynamicState                   dynamic_states[] = {VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
	                                                     VkDynamicState::VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state_info{};
	dynamic_state_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
	dynamic_state_info.pDynamicStates    = dynamic_states;
	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount         = 0;
	layout_info.pSetLayouts            = nullptr;
	layout_info.pushConstantRangeCount = 0;
	layout_info.pPushConstantRanges    = nullptr;
	VkPipelineLayout pipeline_layout   = VK_NULL_HANDLE;
	if (vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		error("Failed to create Vulkan Pipeline layout");
	VkFormat                      format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
	VkPipelineRenderingCreateInfo rendering_info{};
	rendering_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	rendering_info.viewMask                = 0;
	rendering_info.colorAttachmentCount    = 1;
	rendering_info.pColorAttachmentFormats = &format;
	rendering_info.depthAttachmentFormat   = VkFormat::VK_FORMAT_UNDEFINED;
	rendering_info.stencilAttachmentFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
	graphics_pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_info.pNext               = &rendering_info;
	graphics_pipeline_info.stageCount          = 2;
	graphics_pipeline_info.pStages             = shader_stage_infos;
	graphics_pipeline_info.pVertexInputState   = &vertex_input_state_info;
	graphics_pipeline_info.pInputAssemblyState = &input_assembly_state_info;
	graphics_pipeline_info.pTessellationState  = nullptr;
	graphics_pipeline_info.pViewportState      = &viewport_state_info;
	graphics_pipeline_info.pRasterizationState = &rasterization_state_info;
	graphics_pipeline_info.pMultisampleState   = &multisample_state_info;
	graphics_pipeline_info.pDepthStencilState  = nullptr;
	graphics_pipeline_info.pColorBlendState    = &color_blend_state_info;
	graphics_pipeline_info.pDynamicState       = &dynamic_state_info;
	graphics_pipeline_info.layout              = pipeline_layout;
	graphics_pipeline_info.renderPass          = nullptr;
	graphics_pipeline_info.subpass             = 0;
	graphics_pipeline_info.basePipelineHandle  = nullptr;
	graphics_pipeline_info.basePipelineIndex   = 0;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
		error("Failed to create Vulkan Pipeline");
	VkCommandPoolCreateInfo command_pool_info{};
	command_pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.flags            = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = queue_family_index;
	if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS)
		error("Failed to create Vulkan command pool");
}
void resize(uint32_t width, uint32_t height) {
	// std::fprintf(stderr, "width: %d, height: %d\n", width, height);
	VkSurfaceCapabilitiesKHR capabilities{};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface_khr, &capabilities) != VK_SUCCESS)
		error("Failed to get Vulkan surface capabilities");
	width  = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	uint32_t surface_format_count = 0;
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
			goto surface_format_found;
		}
	}
	error("Failed to find suitable Vulkan surface format");
surface_format_found:
	uint32_t present_mode_count = 0;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, nullptr)
	    != VK_SUCCESS)
		error("Failed to get Vulkan surface present modes");
	VkPresentModeKHR* present_modes = new VkPresentModeKHR[present_mode_count];
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, present_modes)
	    != VK_SUCCESS)
		error("Failed to get Vulkan surface present modes");
	for (uint32_t i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = present_modes[i];
			delete[] present_modes;
			goto present_mode_found;
		}
	}
	error("Failed to find suitable Vulkan surface present mode");
present_mode_found:
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
void deinit_vulkan() {}
} // namespace Engine
