#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "util/ShaderStages.h"
#include "util/VertexInputDescription.h"

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

struct Viewport {
	float    x;
    float    y;
    float    width;
    float    height;
    float    minDepth;
    float    maxDepth;
};

struct XY {
	uint32_t x, y;
};

struct Rect2D {
	XY offset, extent;
};

enum CompareOp {
	NEVER = 0,
	LESS = 1,
	EQUAL = 2,
	LESS_OR_EQUAL = 3,
	GREATER = 4,
	NOT_EQUAL = 5,
	GREATER_OR_EQUAL = 6,
	ALWAYS = 7
};

enum class Topology {
	TRIANGLE_LIST = 3
};

class PipelineBuilder {
public:

	std::vector<ShaderStageInfo> _shaderStages;
	//VkPipelineVertexInputStateCreateInfo _vertexInputInfo; // TODO: replace with our own struct
	std::vector<VertexInputBinding> _inputBindings;

	Topology _topology = Topology::TRIANGLE_LIST; // TODO: replace with our own topology enum
	Viewport _viewport;
	Rect2D _scissor;
	

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

	bool _depthTest;
	bool _depthWrite;
	CompareOp _compareOp;

	PipelineBuilder& addShader(ShaderStageInfo shader);
	PipelineBuilder& clearShaders();
	PipelineBuilder& addVertexBinding(VertexInputBinding binding);
	PipelineBuilder& clearVertexBindings();
	PipelineBuilder& setTopology(Topology topology);
	PipelineBuilder& setViewport(Viewport viewport);
	PipelineBuilder& setScissor(Rect2D scissor);
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
	PipelineBuilder& setDepthInfo(bool depthTest, bool depthWrite, CompareOp compareOp);

};

}