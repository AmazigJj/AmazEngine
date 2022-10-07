#pragma once
#include <vector>
#include <string>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "vk_types.h"

struct VertexInputDescription {

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec3 color;
    alignas(8) glm::vec2 uv;
    static VertexInputDescription get_vertex_description();
	bool operator==(const Vertex& other) const;
};



struct Mesh {
    std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;

    AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;

    bool load_from_obj(std::string filename);
	bool load_from_gltf(std::string filename);
};