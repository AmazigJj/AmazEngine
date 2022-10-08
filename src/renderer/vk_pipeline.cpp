#include "vk_pipeline.h"
#include <iostream>
#include <vulkan/vulkan_core.h>

namespace amaz::eng {

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
		_sampleShading = true;
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

