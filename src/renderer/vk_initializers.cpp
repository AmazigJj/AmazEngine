#include "vk_initializers.h"
#include "vk_mesh.h"

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/) {
	
	return {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,

		.flags = flags,
		.queueFamilyIndex = queueFamilyIndex
	};
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count /*= 1*/, VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/) {
	
	return {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,

		.commandPool = pool,
		.level = level,
		.commandBufferCount = count
	};
}

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {

	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,

		.stage = stage,
		.module = shaderModule,
		.pName = "main"
	};
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info() {

	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,

		.vertexBindingDescriptionCount = 0,
		.vertexAttributeDescriptionCount = 0
	};
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info(VertexInputDescription& vertexInput) {

	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.vertexBindingDescriptionCount = (uint32_t)vertexInput.bindings.size(),
		.pVertexBindingDescriptions = vertexInput.bindings.data(),
		.vertexAttributeDescriptionCount = (uint32_t)vertexInput.attributes.size(),
		.pVertexAttributeDescriptions = vertexInput.attributes.data()
	};
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info(VkPrimitiveTopology topology) {
	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,

		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(VkPolygonMode polygonMode, VkCullModeFlags cullMode /* = VK_CULL_MODE_BACK_BIT */) {
	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,

		.depthClampEnable = VK_FALSE,
		//discards all primitives before the rasterization stage if enabled which we don't want
		.rasterizerDiscardEnable = VK_FALSE,

		.polygonMode = polygonMode,
		.cullMode = cullMode,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		//depth bias
		.depthBiasEnable = VK_TRUE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.f,
		.lineWidth = 1.0f
	};
}

VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info() {

	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,

		//multisampling defaulted to no multisampling (1 sample per pixel)
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {

	return {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info(std::span<VkDescriptorSetLayout> setLayouts, std::span<VkPushConstantRange> pushConstants) {

	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,

		.flags = 0,
		.setLayoutCount = (uint32_t)setLayouts.size(),
		.pSetLayouts = setLayouts.data(),
		.pushConstantRangeCount = (uint32_t)pushConstants.size(),
		.pPushConstantRanges = pushConstants.data()
	};
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {

	return {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags
	};
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {

	return {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags
	};
}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VkImageCreateFlags flags /* = 0*/, uint32_t arrayLayers /* = 1 */, uint32_t mipLevels /* = 1 */) {

	return { 
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,

		.flags = flags,

		.imageType = VK_IMAGE_TYPE_2D,

		.format = format,
		.extent = extent,

		.mipLevels = mipLevels,
		.arrayLayers = arrayLayers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usageFlags
	};
}

VkImageViewCreateInfo vkinit::imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, VkImageViewType viewType /* = VK_IMAGE_VIEW_TYPE_2D*/) {

	return {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,

		.image = image,
		.viewType = viewType,
		.format = format,
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
}

VkPipelineDepthStencilStateCreateInfo vkinit::depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp) {
	return {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,

		.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE,
		.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE,
		.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f, // Optional
		.maxDepthBounds = 1.0f, // Optional
	};
}


VkDescriptorSetLayoutBinding vkinit::descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t count /* = 1*/) {

	return {
		.binding = binding,
		.descriptorType = type,
		.descriptorCount = count,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr,
	};
}

VkWriteDescriptorSet vkinit::write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding)
{
	return {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,

		.dstSet = dstSet,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = bufferInfo
	};
}

VkDescriptorBufferInfo vkinit::descriptorBufferInfo(AllocatedBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
	return {
		.buffer = buffer._buffer,
		.offset = 0,
		.range = range
	};
}

VkWriteDescriptorSet vkinit::write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding)
{
	return {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,

		.dstSet = dstSet,
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = imageInfo
	};
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;

	info.pInheritanceInfo = nullptr;
	info.flags = flags;
	return info;
}

VkSubmitInfo vkinit::submit_info(VkCommandBuffer* cmd)
{
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;

	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.pWaitDstStageMask = nullptr;
	info.commandBufferCount = 1;
	info.pCommandBuffers = cmd;
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;

	return info;
}

VkSamplerCreateInfo vkinit::sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode /* = VK_SAMPLER_ADDRESS_MODE_REPEAT */) {
	return {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,

		.magFilter = filters,
		.minFilter = filters,
		.addressModeU = samplerAddressMode,
		.addressModeV = samplerAddressMode,
		.addressModeW = samplerAddressMode
	};
}

VkFramebufferCreateInfo vkinit::frameBufferCreateInfo(VkRenderPass renderPass, uint32_t width, uint32_t height) {
	return {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,

		.renderPass = renderPass,
		.attachmentCount = 1,
		.width = width,
		.height = height,
		.layers = 1
	};
}

VkRenderPassCreateInfo vkinit::renderPassCreateInfo(std::span<VkAttachmentDescription> attachments, std::span<VkSubpassDescription> subpasses,
	std::span<VkSubpassDependency> dependencies) {
	return {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,

		//connect the color attachment to the info
		.attachmentCount = (uint32_t)attachments.size(),
		.pAttachments = attachments.data(),

		//connect the subpass to the info
		.subpassCount = (uint32_t)subpasses.size(),
		.pSubpasses = subpasses.data(),

		//connect the dependencies to the info
		.dependencyCount = (uint32_t)dependencies.size(),
		.pDependencies = dependencies.data()
	};
}

VkAttachmentDescription vkinit::attachmentDescription(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
	VkImageLayout initialLayout, VkImageLayout finalLayout) {

	return {
		.format = format,
		.samples = samples,

		.loadOp = loadOp,
		.storeOp = storeOp,

		//we don't care about stencil
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		
		.initialLayout = initialLayout,
		.finalLayout = finalLayout
	};
}

VkSubpassDescription vkinit::subpassDescription(const VkPipelineBindPoint& bindPoint, std::span<VkAttachmentReference> colourAttachments, const VkAttachmentReference& depthAttachment) {
	return {
		.pipelineBindPoint = bindPoint,
		.colorAttachmentCount = (uint32_t)colourAttachments.size(),
		.pColorAttachments = colourAttachments.data(),
		.pDepthStencilAttachment = &depthAttachment
	};
}

VkSubpassDependency vkinit::subpassDependency(uint32_t srcPass, uint32_t dstPass, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess) {
	
	return {
		.srcSubpass = srcPass,
		.dstSubpass = dstPass,
		.srcStageMask = srcStage,
		.dstStageMask = dstStage,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess
	};
}

VkBufferMemoryBarrier vkinit::bufferBarrier(VkBuffer buffer, uint32_t queue, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags) {
	VkBufferMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,

		.srcAccessMask = srcAccessFlags,
		.dstAccessMask = dstAccessFlags,
		.srcQueueFamilyIndex = queue,
		.dstQueueFamilyIndex = queue,
		.buffer = buffer,
		.size = VK_WHOLE_SIZE,
	};

	return barrier;
}

VkImageMemoryBarrier vkinit::imageBarrier(VkImage image, uint32_t queue, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags, 
	VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask) {
	
	VkImageSubresourceRange subResourceRange = {
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	return {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,

		.srcAccessMask = srcAccessFlags,
		.dstAccessMask = dstAccessFlags,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = queue,
		.dstQueueFamilyIndex = queue,
		.image = image,
		.subresourceRange = subResourceRange
	};
}

VkRenderingAttachmentInfo vkinit::renderingAttachmentInfo(VkImageView imageView, VkImageLayout imageLayout, 
    VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearValue clearValue) {
    
    return {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .pNext = nullptr,
        
        .imageView = imageView,
        .imageLayout = imageLayout,
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue = clearValue
    };
}

VkRenderingInfo vkinit::renderingInfo(std::span<VkRenderingAttachmentInfo> colorAttachments, VkRenderingAttachmentInfo* depthAttachment,
    VkRenderingAttachmentInfo* stencilAttachment, VkRect2D renderArea) {
    
    return {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .pNext = nullptr,
        
        .flags = 0,
        .renderArea = renderArea,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = (uint32_t)colorAttachments.size(),
        .pColorAttachments = colorAttachments.data(),
        .pDepthAttachment = depthAttachment,
        .pStencilAttachment = stencilAttachment
    };
}

