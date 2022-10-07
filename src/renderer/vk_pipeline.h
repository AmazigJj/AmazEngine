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
	bool _clampDepth = false;
	bool _rasterizerDiscard = false;
	bool _wireFrame = false;
	PrimitiveFacing _facing = PrimitiveFacing::COUNTER_CLOCKWISE;
	CullingMode _cullMode = CullingMode::NONE;
	bool _depthBias = false;
	float _depthBiasConstant = 0.f;
	float _depthBiasClamp = 0.f;
	float _depthBiasSlope = 0.f;

	DynamicPipelineState _dynamicState;
	
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	//VkPipelineMultisampleStateCreateInfo _multisampling;
	MSAASamples _samples;
	bool _sampleShading;
	float _minSampleShading;

	VkPipelineLayout _pipelineLayout;

	VkPipelineDepthStencilStateCreateInfo _depthStencil;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass, bool colorBlendingEnabled = true);
	PipelineBuilder& addShader(ShaderStageInfo shader);
	PipelineBuilder& setTopology(VkPrimitiveTopology topology);
	PipelineBuilder& setCullmode(CullingMode mode);
	PipelineBuilder& addDynamicState(DynamicPipelineState mode);
	PipelineBuilder& removeDynamicState(DynamicPipelineState mode);
	PipelineBuilder& clearDynamicState();
};

}