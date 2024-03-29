#pragma once

namespace amaz::eng {

// same as https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorType.html
enum class BindingType {
	SAMPLER = 0,
	IMAGE_SAMPLER = 1,
	SAMPLED_IMAGE = 2,
	STORAGE_IMAGE = 3,
	UNIFORM_TEXEL_BUFFER = 4,
	STORAGE_TEXEL_BUFFER = 5,
	UNIFORM_BUFFER = 6,
	STORAGE_BUFFER = 7,
	UNIFORM_BUFFER_DYNAMIC = 8,
	STORAGE_BUFFER_DYNAMIC = 9,
	INPUT_ATTACHMENT = 10
};



}