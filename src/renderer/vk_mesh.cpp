#include "vk_mesh.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>
#include <iostream>
#include <unordered_map>
#include "vk_types.h"
#include "tiny_gltf.h"

// https://stackoverflow.com/questions/19195183/how-to-properly-hash-the-custom-struct
template <class T>
inline void hash_combine(std::size_t & s, const T & v)
{
  std::hash<T> h;
  s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
}

template<>
struct std::hash<Vertex> {
	size_t operator()(Vertex const& vertex) const {

		size_t result = 0;
		hash_combine<glm::vec3>(result, vertex.position);
		hash_combine<glm::vec3>(result, vertex.normal);
		hash_combine<glm::vec3>(result, vertex.color);
		hash_combine<glm::vec2>(result, vertex.uv);
		return result;
	}
};

bool Mesh::load_from_obj(std::string filename)
{
	//attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib;
	//shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes;
	//materials contains the information about the material of each shape, but we won't use it.
	std::vector<tinyobj::material_t> materials;

	//error and warning output from the load function
	std::string warn;
	std::string err;

	//load the OBJ file
	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), "../assets");
	//make sure to output the warnings to the console, in case there are issues with the file
	if (!warn.empty()) {
		std::cout << "WARN: " << warn << std::endl;
	}
	//if we have any error, print it to the console, and break the mesh loading.
	//This happens if the file can't be found or is malformed
	if (!err.empty()) {
		std::cerr << "ERROR: " << err << std::endl;
		return false;
	}

	std::unordered_map<Vertex, uint32_t> verticesIndex{};

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

			//hardcode loading to triangles
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				//vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

				//vertex normal
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

				//vertex uv
				tinyobj::real_t ux = 0;
				tinyobj::real_t uy = 0;
				if (idx.texcoord_index > -1) {
					ux = attrib.texcoords[2 * idx.texcoord_index + 0];
					uy = attrib.texcoords[2 * idx.texcoord_index + 1];
				}

				Vertex vertex {
					.position = {vx, vy, vz},
					.normal = {nx, ny, nz},
					.color = {nx, ny, nz}, // set color to normal, debug
					.uv = {ux, 1 - uy}
				};

				if (verticesIndex.count(vertex) == 0) {
					verticesIndex[vertex] = _vertices.size();
					_vertices.push_back(vertex);
				}


				// _indices.push_back(_vertices.size());
				// _vertices.push_back(vertex);
				_indices.push_back(verticesIndex[vertex]);

				// _vertices.push_back({
				// 	.position = {vx, vy, vz},
				// 	.normal = {nx, ny, nz},
				// 	.color = {nx, ny, nz}, // set color to normal, debug
				// 	.uv = {ux, 1 - uy}
				// 	});
			}


			index_offset += fv;
		}


	}

	std::cout << "Mesh loaded\n "
		<< _indices.size() << " Indices\n "
		<< _vertices.size() << " Verices\n";

	return true;
}

bool Mesh::load_from_gltf(std::string filename) {
	// tinygltf::Model model;
	// tinygltf::TinyGLTF loader;
	// std::string err;
	// std::string warn;

	// bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	// if (!warn.empty()) {
	// printf("Warn: %s\n", warn.c_str());
	// }

	// if (!err.empty()) {
	// printf("Err: %s\n", err.c_str());
	// }

	// if(!ret){
	// 	std::cout << std::format("Unable to load model: {}\n", filename);
	// 	return 0;
	// }

	return true;
}

bool Vertex::operator==(const Vertex& other) const {
    return position == other.position && normal == other.normal && color == other.color && uv == other.uv;
}

amaz::eng::VertexInputBinding Vertex::getVertexBinding() {
	std::vector<amaz::eng::VertexInputAttribute> attributes;

	attributes.push_back({amaz::eng::ImageFormats::R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributes.push_back({amaz::eng::ImageFormats::R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributes.push_back({amaz::eng::ImageFormats::R32G32B32_SFLOAT, offsetof(Vertex, color)});
	attributes.push_back({amaz::eng::ImageFormats::R32G32B32_SFLOAT, offsetof(Vertex, uv)});

	return {
		.attributes = attributes,
		.stride = sizeof(Vertex)
	};
}	

