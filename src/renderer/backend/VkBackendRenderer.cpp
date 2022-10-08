#include "VkBackendRenderer.h"

#include <iostream>

namespace amaz::eng {
	VkPipeline VkBackendRenderer::buildPipeline(PipelineBuilder& builder, VkRenderPass pass) {
		//make viewport state from our stored viewport and scissor.
		//at the moment we won't support multiple viewports or scissors
		VkPipelineViewportStateCreateInfo viewportState = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,

			.viewportCount = 1,
			.pViewports = &builder._viewport,
			.scissorCount = 1,
			.pScissors = &builder._scissor
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
			.topology = builder._topology,
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

		//build the actual pipeline
		//we now use all of the info structs we have been writing into this one to create the pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,

			.stageCount = (uint32_t)shaderStageInfos.size(),
			.pStages = shaderStageInfos.data(),
			.pVertexInputState = &builder._vertexInputInfo,
			.pInputAssemblyState = &inputAssemblyInfo,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisamplingInfo,
			.pDepthStencilState = &builder._depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = builder._pipelineLayout,
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