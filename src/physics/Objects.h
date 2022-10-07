#pragma once

#include <glm/glm.hpp>

struct Line {
	glm::vec3 a, b;
};

struct Ray {
	glm::vec3 origin, dir;
};

struct Triangle {
	glm::vec3 a, b, c;
};

struct AABB {
	glm::vec3 a, b;
};

struct Capsule {
	glm::vec3 tip, base;
	float radius;
};

struct Sphere {
	glm::vec3 pos;
	float radius;
};