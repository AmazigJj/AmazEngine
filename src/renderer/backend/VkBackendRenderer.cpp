#include "VkBackendRenderer.h"

#include <iostream>
#include <vulkan/vulkan_core.h>

namespace amaz::eng {
	VkPipeline VkBackendRenderer::buildPipeline(PipelineBuilder& builder, VkRenderPass pass, VkPipelineLayout layout) {
		//make viewport state from our stored viewport and scissor.
		//at the moment we won't support multiple viewports or scissors
		VkPipelineViewportStateCreateInfo viewportState = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,

			.viewportCount = 1,
			.pViewports = (VkViewport*)&builder._viewport,
			.scissorCount = 1,
			.pScissors = (VkRect2D*)&builder._scissor
		};

		// TODO: add blending support
		VkPipelineColorBlendAttachmentState colorBlendAttachment {
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		//setup dummy color blending. We aren't using transparent objects yet
		//the blending is just "no blend", but we do write to the color attachment
		VkPipelineColorBlendStateCreateInfo colorBlending = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,

			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment
		};

		std::vector<VkDynamicState> dynamicStates;

		if (builder._dynamicState & DynamicPipelineState::VIEWPORT) {
			dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		}

		if (builder._dynamicState & DynamicPipelineState::SCISSOR) {
			dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
		}

		if (builder._dynamicState & DynamicPipelineState::DEPTH_BIAS) {
			dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		}

		VkPipelineDynamicStateCreateInfo dynamicState = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,

			.dynamicStateCount = (uint32_t)dynamicStates.size(),
			.pDynamicStates = dynamicStates.data(),
		};

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
		for (auto& shader : builder._shaderStages) {
			shaderStageInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = (VkShaderStageFlagBits)shader.stage,
				.module = shader.module,
				.pName = "main"
			});
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.topology = (VkPrimitiveTopology)builder._topology,
			// unnecessary, only using tris
			.primitiveRestartEnable = false
		};

		VkPipelineRasterizationStateCreateInfo rasterizer {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = builder._clampDepth,
			.rasterizerDiscardEnable = builder._rasterizerDiscard,
			.polygonMode = builder._wireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
			.cullMode = (VkCullModeFlags)builder._cullMode,
			.frontFace = (VkFrontFace)(uint32_t)builder._facing,
			.depthBiasEnable = builder._depthBias,
			.depthBiasConstantFactor = builder._depthBiasConstant,
			.depthBiasClamp = builder._depthBiasClamp,
			.depthBiasSlopeFactor = builder._depthBiasSlope
		};

		VkPipelineMultisampleStateCreateInfo multisamplingInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,

			.flags = 0,
			.rasterizationSamples = (VkSampleCountFlagBits)builder._samples,
			.sampleShadingEnable = builder._sampleShading,
			.minSampleShading = builder._minSampleShading
		};

		std::vector<VkVertexInputBindingDescription> bindings;
    	std::vector<VkVertexInputAttributeDescription> attributes;

		for (uint32_t i = 0; i < builder._inputBindings.size(); i++) {
			auto inputBinding =  builder._inputBindings[i];
			bindings.push_back({
				.binding = i,
				.stride = inputBinding.stride,
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			});
			for (uint32_t j = 0; j < inputBinding.attributes.size(); j++) {
				auto inputAttribute = inputBinding.attributes[j];

				attributes.push_back({
					.location = j,
					.binding = i,
					.format = (VkFormat)inputAttribute.format,
					.offset = inputAttribute.offset
				});

			}
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.vertexBindingDescriptionCount = (uint32_t)bindings.size(),
			.pVertexBindingDescriptions = bindings.data(),
			.vertexAttributeDescriptionCount = (uint32_t)attributes.size(),
			.pVertexAttributeDescriptions = attributes.data()
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,

			.depthTestEnable = builder._depthTest,
			.depthWriteEnable = builder._depthWrite,
			.depthCompareOp = (VkCompareOp)builder._compareOp,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.minDepthBounds = 0.0f, // Optional
			.maxDepthBounds = 1.0f, // Optional
		};

		//build the actual pipeline
		//we now use all of the info structs we have been writing into this one to create the pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,

			.stageCount = (uint32_t)shaderStageInfos.size(),
			.pStages = shaderStageInfos.data(),
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssemblyInfo,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisamplingInfo,
			.pDepthStencilState = &depthStencilInfo,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = layout,
			.renderPass = pass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE
		};

		VkPipeline newPipeline;
		if (vkCreateGraphicsPipelines(
			_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
			std::cout << "failed to create pipeline\n";
			return VK_NULL_HANDLE; // failed to create graphics pipeline
		}
		return newPipeline;
	}
}