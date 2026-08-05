#include "coc.hpp"
#include "common/common.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#else
#include <vulkan/vulkan_wayland.h>
#endif
namespace Engine {
void*      data = nullptr;
VkInstance create_instance() {
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
	VkInstance instance;
	VkResult   res;
	if ((res = vkCreateInstance(&instance_info, nullptr, &instance)) != VK_SUCCESS) {
		if (res == VK_ERROR_LAYER_NOT_PRESENT)
			error("Layer not present");
		error("Failed to create Vulkan instance");
	}
	return instance;
}
VkPhysicalDevice create_physical_device(VkInstance instance) {
	uint32_t physical_device_count = 0;
	if (vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr) != VK_SUCCESS || physical_device_count == 0)
		error("No Vulkan devices found");
	VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
	if (vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices) != VK_SUCCESS)
		error("Failed to get Vulkan physical devices");
	VkPhysicalDevice physical_device = *physical_devices;
	delete[] physical_devices;
	return physical_device;
}
uint32_t find_queue_family_index(VkPhysicalDevice physical_device) {
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
			return queue_family_index;
		}
	}
	error("Queue family not found");
	return UINT32_MAX;
}
VkDevice create_device(VkPhysicalDevice physical_device, uint32_t queue_family_index) {
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
	VkDevice device;
	if (vkCreateDevice(physical_device, &device_info, nullptr, &device) != VK_SUCCESS)
		error("Failed to create Vulkan device");
	return device;
}
VkQueue create_queue(VkDevice device, uint32_t queue_family_index) {
	VkDeviceQueueInfo2 queue_info2{};
	queue_info2.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	queue_info2.queueFamilyIndex = queue_family_index;
	queue_info2.queueIndex       = 0;
	VkQueue queue;
	vkGetDeviceQueue2(device, &queue_info2, &queue);
	return queue;
}
VkShaderModule create_shader_module(VkDevice device) {
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
	return shader_module;
}
VkPipelineLayout create_pipeline_layout(VkDevice device) {
	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount         = 0;
	layout_info.pSetLayouts            = nullptr;
	layout_info.pushConstantRangeCount = 0;
	layout_info.pPushConstantRanges    = nullptr;
	VkPipelineLayout pipeline_layout;
	if (vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		error("Failed to create Vulkan Pipeline layout");
	return pipeline_layout;
}
VkPipeline create_pipeline(VkDevice device, VkShaderModule shader_module, VkPipelineLayout pipeline_layout) {
	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage               = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module              = shader_module;
	vert_shader_stage_info.pName               = "vertMain";
	vert_shader_stage_info.pSpecializationInfo = nullptr;
	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage                         = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module                        = shader_module;
	frag_shader_stage_info.pName                         = "fragMain";
	frag_shader_stage_info.pSpecializationInfo           = nullptr;
	VkPipelineShaderStageCreateInfo shader_stage_infos[] = {vert_shader_stage_info, frag_shader_stage_info};
	VkVertexInputBindingDescription binding_description  = Vertex::get_binding_description();
	std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = Vertex::get_attribute_descriptions();
	VkPipelineVertexInputStateCreateInfo             vertex_input_state_info{};
	vertex_input_state_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_info.vertexBindingDescriptionCount   = 1;
	vertex_input_state_info.pVertexBindingDescriptions      = &binding_description;
	vertex_input_state_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
	vertex_input_state_info.pVertexAttributeDescriptions    = attribute_descriptions.data();
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
	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
		error("Failed to create Vulkan Pipeline");
	return pipeline;
}
VkCommandPool create_command_pool(VkDevice device, uint32_t queue_family_index) {
	VkCommandPoolCreateInfo command_pool_info{};
	command_pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.flags            = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = queue_family_index;
	VkCommandPool command_pool;
	if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS)
		error("Failed to create Vulkan command pool");
	return command_pool;
}
VkBuffer create_vertex_buffer(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_family_index) {
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size                  = sizeof(Vertex) * verticies.size();
	buffer_info.usage                 = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode           = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.queueFamilyIndexCount = 1;
	buffer_info.pQueueFamilyIndices   = &queue_family_index;
	VkBuffer vertex_buffer;
	if (vkCreateBuffer(device, &buffer_info, nullptr, &vertex_buffer) != VK_SUCCESS)
		error("Failed to create Vulkan vertex buffer");
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, vertex_buffer, &memory_requirements);
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
	VkMemoryPropertyFlags memory_property_flags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	                                              | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t index;
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
		if ((memory_requirements.memoryTypeBits & (1 << i))
		    && (memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags) {
			index = i;
			goto index_found;
		}
	}
	error("Could not find Vulkan memory type");
	return VK_NULL_HANDLE;
index_found:
	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = index;
	VkDeviceMemory device_memory;
	if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &device_memory) != VK_SUCCESS)
		error("Failed to allocate vertex buffer memory");
	if (vkBindBufferMemory(device, vertex_buffer, device_memory, 0) != VK_SUCCESS)
		error("Failed to bind vertex buffer memory");
	if (vkMapMemory(device, device_memory, 0, sizeof(Vertex) * verticies.size(), 0, &data) != VK_SUCCESS)
		error("Failed to map vertex buffer memory");
	std::memcpy(data, verticies.data(), sizeof(Vertex) * verticies.size());
	return vertex_buffer;
}
} // namespace Engine
