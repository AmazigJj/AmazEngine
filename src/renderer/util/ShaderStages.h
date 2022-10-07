#pragma once

namespace amaz::eng {

// same as https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderStageFlagBits.html
enum class ShaderStages {
	VERTEX = 1,
	TESSELATION_CONTROL = 2,
	TESSELLATION_EVALUATION = 4,
	GEOMETRY = 8,
	FRAGMENT = 16,
	COMPUTE = 32,

};

constexpr ShaderStages operator|(ShaderStages a, ShaderStages b) {
    return static_cast<ShaderStages>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr ShaderStages operator&(ShaderStages a, ShaderStages b) {
    return static_cast<ShaderStages>(static_cast<int>(a) & static_cast<int>(b));
}

}