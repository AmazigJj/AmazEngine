#include "vk_pipeline.h"
#include <iostream>

namespace amaz::eng {

	VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
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
				.module = shader.module,
				.pName = "main"
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
			.depthBiasSlopeFactor = _depthBiasSlope,
			.lineWidth = 1.0
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
	}

	PipelineBuilder& PipelineBuilder::clearShaders() {
		_shaderStages.clear();
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setVertexInput(VkPipelineVertexInputStateCreateInfo info) {
		_vertexInputInfo = info;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setTopology(VkPrimitiveTopology topology) {
		_topology = topology;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setViewport(VkViewport viewport) {
		_viewport = viewport;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setScissor(VkRect2D scissor) {
		_scissor = scissor;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setCullmode(CullingMode mode) {
		_cullMode = mode;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setFacing(PrimitiveFacing facing) {
		_facing = facing;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::enableRasterizerDiscard() {
		_rasterizerDiscard = true;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::disableRasterizerDiscard() {
		_rasterizerDiscard = false;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::enableWireframe() {
		_wireFrame = true;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::disableWireframe() {
		_wireFrame = false;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::enableDepthBias(float depthBiasConstant, float depthBiasSlope, float depthBiasClamp) {
		_depthBias = true;
		_depthBiasConstant = depthBiasConstant;
		_depthBiasSlope = depthBiasSlope;
		_depthBiasClamp = depthBiasClamp;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::disableDepthBias() {
		_depthBias = false;
		_depthBiasConstant = 0;
		_depthBiasSlope = 0;
		_depthBiasClamp = 0;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::enableDepthClamp() {
		_clampDepth = true;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::disableDepthClamp() {
		_clampDepth = false;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::addDynamicState(DynamicPipelineState mode) {
		_dynamicState = _dynamicState | mode;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::removeDynamicState(DynamicPipelineState mode) {
		_dynamicState = _dynamicState & ~mode;
		return *this;
	}
	
	PipelineBuilder& PipelineBuilder::clearDynamicState() {
		_dynamicState = 0;
		return *this;
	}

	// note: color blending is currently not implemented
	PipelineBuilder& PipelineBuilder::enableColorBlending() {
		std::cout << "Color blending enabled for pipeline, but color blending is not currently implemented";
		_colorBlend = true;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::disableColorBlending() {
		_colorBlend = false;
		return *this;
	}


	PipelineBuilder& PipelineBuilder::setMSAASamples(MSAASamples samples) {
		_samples = samples;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::enableSampleShading(float minSampleShading) {
		_sampleShading = true;
		_minSampleShading = minSampleShading;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::disableSampleShading() {
		_sampleShading = false;
		_minSampleShading = 0;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setLayout(VkPipelineLayout layout) {
		_pipelineLayout = layout;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::setDepthStencilInfo(VkPipelineDepthStencilStateCreateInfo depthStencil) {
		_depthStencil = depthStencil;
		return *this;
	}

}

