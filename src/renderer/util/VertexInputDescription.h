#pragma once

#include <vector>

namespace amaz::eng {

enum class ImageFormats {
	R32G32B32_SFLOAT = 106
};

struct VertexInputAttribute {
	ImageFormats format;
	uint32_t offset;
};

struct VertexInputBinding {
	std::vector<VertexInputAttribute> attributes;
	uint32_t stride;
};

}