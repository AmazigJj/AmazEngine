// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vk_types.h"
#include "vk_mesh.h"
#include <span>
#include <vulkan/vulkan_core.h>
#include <optional>

namespace vkinit {

	//vulkan init code goes here
	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(VertexInputDescription& vertexInput);
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode, VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT);
	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
	VkPipelineColorBlendAttachmentState color_blend_attachment_state();
	VkPipelineLayoutCreateInfo pipeline_layout_create_info(std::span<VkDescriptorSetLayout> setLayouts, std::span<VkPushConstantRange> pushConstants);

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

	VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VkImageCreateFlags flags = 0, uint32_t arrayLayers = 1, uint32_t mipLevels = 1);

	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

	VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t count = 1);
	VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkDescriptorBufferInfo descriptorBufferInfo(AllocatedBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
	VkSubmitInfo submit_info(VkCommandBuffer* cmd);

	VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);

	VkFramebufferCreateInfo frameBufferCreateInfo(VkRenderPass renderPass, uint32_t width, uint32_t height);


	VkRenderPassCreateInfo renderPassCreateInfo(std::span<VkAttachmentDescription> attachments, std::span<VkSubpassDescription> subpasses,
		std::span<VkSubpassDependency> dependencies);

	
	VkAttachmentDescription attachmentDescription(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
		VkImageLayout initialLayout, VkImageLayout finalLayout);

	VkSubpassDescription subpassDescription(const VkPipelineBindPoint& bindPoint,  std::span<VkAttachmentReference> colourAttachments,
		const VkAttachmentReference& depthAttachment);

	VkSubpassDependency subpassDependency(uint32_t srcPass, uint32_t dstPass, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccess, VkAccessFlags dstAccess);

	VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, uint32_t queue, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags);

	VkImageMemoryBarrier imageBarrier(VkImage image, uint32_t queue, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags, 
		VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);
        
    VkRenderingAttachmentInfo renderingAttachmentInfo(VkImageView imageView, VkImageLayout imageLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
        VkClearValue clearValue);
    
    VkRenderingInfo renderingInfo(std::span<VkRenderingAttachmentInfo> colorAttachments, VkRenderingAttachmentInfo* depthAttachment,
        VkRenderingAttachmentInfo* stencilAttachment, VkRect2D renderArea);
}

