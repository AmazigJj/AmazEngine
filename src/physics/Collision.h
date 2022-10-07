#pragma once

#include "Objects.h"

namespace amaz {
	bool rayVsAABB(Ray ray, AABB aabb, glm::vec3& colPoint, glm::vec3& colNormal, float& t);
	bool LineVsAABB(Line line, AABB aabb);
	bool TriangleVsAABB(Triangle tri, AABB aabb);
	bool AABBvsAABB(AABB a, AABB b);
	float calcArea(AABB aabb);
	AABB getAABBFromTriangle(Triangle tri);
	AABB getAABBFromTriangle(size_t tri);
	Triangle getTriFromId(size_t id);
	size_t trisCount();
	size_t registerTri(Triangle tri);
}