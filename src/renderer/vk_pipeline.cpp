#include "vk_pipeline.h"
#include <iostream>
#include <vulkan/vulkan_core.h>

namespace amaz::eng {

	VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass, bool colorBlendingEnabled) {
		//make viewport state from our stored viewport and scissor.
		//at the moment we won't support multiple viewports or scissors
		VkPipelineViewportStateCreateInfo viewportState = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,

			.viewportCount = 1,
			.pViewports = &_viewport,
			.scissorCount = 1,
			.pScissors = &_scissor
		};

		//setup dummy color blending. We aren't using transparent objects yet
		//the blending is just "no blend", but we do write to the color attachment
		// TODO: option for this
		VkPipelineColorBlendStateCreateInfo colorBlending = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,

			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &_colorBlendAttachment
		};

		std::vector<VkDynamicState> dynamicStates;

		if (_dynamicState & DynamicPipelineState::VIEWPORT) {
			dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		}

		if (_dynamicState & DynamicPipelineState::SCISSOR) {
			dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
		}

		if (_dynamicState & DynamicPipelineState::DEPTH_BIAS) {
			dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		}

		VkPipelineDynamicStateCreateInfo dynamicState = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,

			.dynamicStateCount = (uint32_t)dynamicStates.size(),
			.pDynamicStates = dynamicStates.data(),
		};

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
		for (auto& shader : _shaderStages) {
			shaderStageInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = (VkShaderStageFlagBits)shader.stage,
				.module = shader.module
			});
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.topology = _topology,
			// unnecessary, only using tris
			.primitiveRestartEnable = false
		};

		VkPipelineRasterizationStateCreateInfo rasterizer {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = _clampDepth,
			.rasterizerDiscardEnable = _rasterizerDiscard,
			.polygonMode = _wireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
			.cullMode = (VkCullModeFlags)_cullMode,
			.frontFace = (VkFrontFace)(uint32_t)_facing,
			.depthBiasEnable = _depthBias,
			.depthBiasConstantFactor = _depthBiasConstant,
			.depthBiasClamp = _depthBiasClamp,
			.depthBiasSlopeFactor = _depthBiasSlope
		};

		VkPipelineMultisampleStateCreateInfo multisamplingInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,

			.flags = 0,
			.rasterizationSamples = (VkSampleCountFlagBits)_samples,
			.sampleShadingEnable = _sampleShading,
			.minSampleShading = _minSampleShading
		};

		//build the actual pipeline
		//we now use all of the info structs we have been writing into this one to create the pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,

			.stageCount = (uint32_t)shaderStageInfos.size(),
			.pStages = shaderStageInfos.data(),
			.pVertexInputState = &_vertexInputInfo,
			.pInputAssemblyState = &inputAssemblyInfo,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisamplingInfo,
			.pDepthStencilState = &_depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = _pipelineLayout,
			.renderPass = pass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE
		};


		if (colorBlendingEnabled) {
			pipelineInfo.pColorBlendState = &colorBlending;
		}

		//it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
		VkPipeline newPipeline;
		if (vkCreateGraphicsPipelines(
			device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
			std::cout << "failed to create pipeline\n";
			return VK_NULL_HANDLE; // failed to create graphics pipeline
		}
		return newPipeline;
	}

	PipelineBuilder& PipelineBuilder::addShader(ShaderStageInfo shader) {
		_shaderStages.push_back(shader);
		return *this;
	};

}