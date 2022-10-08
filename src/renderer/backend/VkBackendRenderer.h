#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "../PipelineBuilder.h"

namespace amaz::eng {
class VkBackendRenderer {
public:
	VkBackendRenderer(VkDevice device) : _device(device) {}
	VkPipeline buildPipeline(PipelineBuilder& builder, VkRenderPass pass, VkPipelineLayout layout);

private:
	VkDevice _device;
};

}