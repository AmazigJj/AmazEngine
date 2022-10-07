#include "Collision.h"

#include "Objects.h"
#include <vector>


namespace amaz {
	std::vector<Triangle> tris;

	bool rayVsAABB(Ray ray, AABB aabb, glm::vec3& colPoint, glm::vec3& colNormal, float& t) {
		glm::vec3 t_near = (aabb.a - ray.origin) / ray.dir;

		glm::vec3 t_far = (aabb.b - ray.origin) / ray.dir;

		if (t_near.x > t_far.x) std::swap(t_near.x, t_far.x);
		if (t_near.y > t_far.y) std::swap(t_near.y, t_far.y);
		if (t_near.z > t_far.z) std::swap(t_near.z, t_far.z);

		float t_hit_near = std::max(std::max(t_near.x, t_near.y), t_near.z);
		float t_hit_far = std::min(std::min(t_far.x, t_far.y), t_far.z);

		if (t_hit_far < 0) return false;

		colPoint = ray.origin + t_hit_near * ray.dir;

		if (t_hit_near == t_near.x) {
			if (ray.dir.x < 0)
				colNormal = { 1.f, 0.f, 0.f };
			else
				colNormal = { -1.f, 0.f, 0.f };
		}
		else if (t_hit_near == t_near.y) {
			if (ray.dir.y < 0)
				colNormal = { 0.f, 1.f, 0.f };
			else
				colNormal = { 0.f, -1.f, 0.f };
		}
		else if (t_hit_near == t_near.z) {
			if (ray.dir.z < 0)
				colNormal = { 0.f, 0.f, 1.f };
			else
				colNormal = { 0.f, 0.f, -1.f };
		}

		t = t_hit_near;

		return true;
	}

	bool LineVsAABB(Line line, AABB aabb) {
		glm::vec3 colPoint;
		glm::vec3 colNormal;
		float t;
		return rayVsAABB({ line.a, line.b - line.a }, aabb, colPoint, colNormal, t) ? (t >= 0 && t <= 1) : false;
	}

	bool TriangleVsAABB(Triangle tri, AABB aabb) {
		return LineVsAABB({ tri.a, tri.b }, aabb) || LineVsAABB({ tri.b, tri.c }, aabb) || LineVsAABB({ tri.c, tri.a }, aabb);
	}

	bool AABBvsAABB(AABB a, AABB b) {
		return (((a.a.x <= b.b.x && a.b.x >= b.a.x) || (a.b.x <= b.a.x && a.a.x >= b.b.x)) || ((a.a.x <= b.a.x && a.b.x >= b.b.x) || (a.b.x <= b.b.x && a.a.x >= b.a.x))) &&
			(((a.a.y <= b.b.y && a.b.y >= b.a.y) || (a.b.y <= b.a.y && a.a.y >= b.b.y)) || ((a.a.y <= b.a.y && a.b.y >= b.b.y) || (a.b.y <= b.b.y && a.a.y >= b.a.y))) &&
			(((a.a.z <= b.b.z && a.b.z >= b.a.z) || (a.b.z <= b.a.z && a.a.z >= b.b.z)) || ((a.a.z <= b.a.z && a.b.z >= b.b.z) || (a.b.z <= b.b.z && a.a.z >= b.a.z)));
	}

	float calcArea(AABB aabb) {
		glm::vec3 min = {
			std::min(aabb.a.x, aabb.b.x),
			std::min(aabb.a.y, aabb.b.y),
			std::min(aabb.a.z, aabb.b.z)
		};
		glm::vec3 max = {
			std::max(aabb.a.x, aabb.b.x),
			std::max(aabb.a.y, aabb.b.y),
			std::max(aabb.a.z, aabb.b.z)
		};

		return std::abs(min.x - max.x) * std::abs(min.x - max.x) * std::abs(min.x - max.x);
	}

	AABB getAABBFromTriangle(Triangle tri) {
		glm::vec3 min = {
			std::min(std::min(tri.a.x, tri.b.x), tri.c.x),
			std::min(std::min(tri.a.y, tri.b.y), tri.c.y),
			std::min(std::min(tri.a.z, tri.b.z), tri.c.z)
		};

		glm::vec3 max = {
			std::max(std::max(tri.a.x, tri.b.x), tri.c.x),
			std::max(std::max(tri.a.y, tri.b.y), tri.c.y),
			std::max(std::max(tri.a.z, tri.b.z), tri.c.z)
		};

		return { min, max };
	}

	AABB getAABBFromTriangle(size_t tri) {
		return getAABBFromTriangle(tris[tri]);
	}

	Triangle getTriFromId(size_t id) {
		if (id > tris.size()) return {};
		return tris[id];
	}

	size_t trisCount() {
		return tris.size();
	}

	size_t registerTri(Triangle tri) {
		int id = tris.size();
		tris.push_back(tri);
		return id;
	}




}