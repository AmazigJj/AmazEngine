#include <vulkan/vulkan.h>
#include <vector>
#include "util/ShaderStages.h"
namespace amaz::eng {
struct ShaderStageInfo {
	amaz::eng::ShaderStages stage;
	VkShaderModule module;
};

class CullingMode {
public:
	enum {
		NONE = 0,
		FRONT = 1,
		BACK = 2,
		BOTH = 3
	};
	CullingMode(uint32_t value) : _value(value) {};
	constexpr operator uint32_t() {return this->_value;}
private:
	uint32_t _value;
};

class PrimitiveFacing {
public:
	enum {
		COUNTER_CLOCKWISE = 0,
		CLOCKWISE = 1
	};
	PrimitiveFacing(uint32_t value) : _value(value) {};
	constexpr operator uint32_t() {return this->_value;}
private:
	uint32_t _value;
};

class DynamicPipelineState {
public:
	enum {
		VIEWPORT = 1,
		SCISSOR = 2,
		DEPTH_BIAS = 4,
	};
	constexpr DynamicPipelineState(uint32_t value) : _value(value) {}
	constexpr operator uint32_t() {return this->_value;}

private:
	uint32_t _value;
};

enum class MSAASamples {
	ONE = 1,
    TWO = 2,
    FOUR = 4,
    EIGHT = 8,
    SIXTEEN = 16,
    THIRTY_TWO = 32,
    SIXTY_FOUR = 64
};

class PipelineBuilder {
public:

	std::vector<ShaderStageInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo; // TODO: replace with our own struct
	VkPrimitiveTopology _topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // TODO: replace with our own topology enum
	VkViewport _viewport;
	VkRect2D _scissor;

	//VkPipelineRasterizationStateCreateInfo _rasterizer;
	CullingMode _cullMode = CullingMode::NONE;
	PrimitiveFacing _facing = PrimitiveFacing::COUNTER_CLOCKWISE;
	
	bool _rasterizerDiscard = false;
	bool _wireFrame = false;

	bool _depthBias = false;
	float _depthBiasConstant = 0.f;
	float _depthBiasClamp = 0.f;
	float _depthBiasSlope = 0.f;

	bool _clampDepth = false;

	DynamicPipelineState _dynamicState = 0;
	
	float _colorBlend = false;

	MSAASamples _samples = MSAASamples::ONE;
	bool _sampleShading = false;
	float _minSampleShading = 0;

	VkPipelineLayout _pipelineLayout;

	VkPipelineDepthStencilStateCreateInfo _depthStencil;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);

	PipelineBuilder& addShader(ShaderStageInfo shader);
	PipelineBuilder& clearShaders();
	PipelineBuilder& setVertexInput(VkPipelineVertexInputStateCreateInfo info);
	PipelineBuilder& setTopology(VkPrimitiveTopology topology);
	PipelineBuilder& setViewport(VkViewport viewport);
	PipelineBuilder& setScissor(VkRect2D scissor);
	PipelineBuilder& setCullmode(CullingMode mode);
	PipelineBuilder& setFacing(PrimitiveFacing facing);
	PipelineBuilder& enableRasterizerDiscard();
	PipelineBuilder& disableRasterizerDiscard();
	PipelineBuilder& enableWireframe();
	PipelineBuilder& disableWireframe();
	PipelineBuilder& enableDepthBias(float depthBiasConstant, float depthBiasSlope, float depthBiasClamp);
	PipelineBuilder& disableDepthBias();
	PipelineBuilder& enableDepthClamp();
	PipelineBuilder& disableDepthClamp();
	PipelineBuilder& addDynamicState(DynamicPipelineState mode);
	PipelineBuilder& removeDynamicState(DynamicPipelineState mode);
	PipelineBuilder& clearDynamicState();
	PipelineBuilder& enableColorBlending();
	PipelineBuilder& disableColorBlending();
	PipelineBuilder& setMSAASamples(MSAASamples samples);
	PipelineBuilder& enableSampleShading(float minSampleShading);
	PipelineBuilder& disableSampleShading();
	PipelineBuilder& setLayout(VkPipelineLayout layout);
	PipelineBuilder& setDepthStencilInfo(VkPipelineDepthStencilStateCreateInfo depthStencil);

};

}