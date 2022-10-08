#include "Physics.h"
#include <ranges>
#include "../util/range_view.hpp"

namespace amaz {

	glm::mat4 calcTransformMatrix(glm::vec3 position, float scale, glm::vec3 rotation) {
		auto translation = glm::translate(position);

		auto scaleMatrix = glm::scale(glm::vec3(scale));

		glm::mat4 rotateX = glm::rotate(
			glm::mat4(1.0f),
			glm::radians(rotation.x),
			glm::vec3(1.0f, 0.0f, 0.0f)
		);

		glm::mat4 rotateY = glm::rotate(
			glm::mat4(1.0f),
			glm::radians(rotation.y),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		glm::mat4 rotateZ = glm::rotate(
			glm::mat4(1.0f),
			glm::radians(rotation.z),
			glm::vec3(0.0f, 0.0f, 1.0f)
		);

		glm::mat4 rotate = rotateX * rotateY * rotateY;

		return translation * rotate * scaleMatrix;
	}

	bool ColMesh::loadObj(std::string filename, float scale) {

		tris.clear();

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		//error and warning output from the load function
		std::string warn;
		std::string err;


		tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), "../assets");

		if (!warn.empty()) {
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty()) {
			std::cerr << "ERROR: " << err << std::endl;
			return false;
		}

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

				//hardcode loading to triangles
				int fv = 3;

				std::array<glm::vec3, 3> colVertices;

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

					//vertex position
					tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
					tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
					tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
					//vertex normal
					//tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
					//tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
					//tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
					//vertex uv
					//tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
					//tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];

					colVertices[v] = {
							vx, vy, vz
					};
				}
				index_offset += fv;

				tris.push_back({
						colVertices[0], colVertices[1], colVertices[2]
					});
			}


		}



		return true;
	}
	void ColMesh::moveObj(glm::vec3 position, float scale, glm::vec3 rotation) {
		glm::mat4 transform = calcTransformMatrix(position, scale, rotation);
		for (auto& tri : tris) {
			tri.a = transform * glm::vec4(tri.a, 1.f);
			tri.b = transform * glm::vec4(tri.b, 1.f);
			tri.c = transform * glm::vec4(tri.c, 1.f);
		}
	}

	Physics::Physics() {
		octree = amaz::Octree::create({ { -65536.f, -65536.f, -65536.f }, { 65536.f, 65536.f, 65536.f } });
		initCollision();

	}

	void Physics::initCollision() {

		player = {
			{0.f, 0.8f, 0.f},
			{0.f, -0.8f, 0.f},
			0.4f
		};

		playerSphere = {
			{0.f, 0.f, 0.f},
			0.4f
		};

		_collisionObjects.push_back({
			{-50.0f, -0.5f, -50.0f},
			{50.0f, 0.f, 50.0f}
			});


	}

	void Physics::loadMesh(std::string name, std::string filename, glm::vec3 position, float scale, glm::vec3 rotation) {
		ColMesh mesh{};
		mesh.loadObj(filename);
		mesh.moveObj(position, scale, rotation);

		for (auto tri : mesh.tris) {
			octree->addElement(amaz::registerTri(tri));
		}

		_meshIDs[name] = _meshes.size();
		_meshes.push_back(mesh);
	}

	bool Physics::newResolveCollisions(glm::vec3 pos, glm::vec3& moveVector) {

		glm::vec3 newPos = pos + moveVector;

		Capsule tempPlayer = {
			player.tip + newPos,
			player.base + newPos,
			player.radius
		};

		//TODO: include old pos?
		glm::vec3 playerMax = {
			std::max(tempPlayer.base.x, tempPlayer.tip.x) + tempPlayer.radius,
			std::max(tempPlayer.base.y, tempPlayer.tip.y) + tempPlayer.radius,
			std::max(tempPlayer.base.z, tempPlayer.tip.z) + tempPlayer.radius };
		glm::vec3 playerMin = {
			std::min(tempPlayer.base.x, tempPlayer.tip.x) - tempPlayer.radius,
			std::min(tempPlayer.base.y, tempPlayer.tip.y) - tempPlayer.radius,
			std::min(tempPlayer.base.z, tempPlayer.tip.z) - tempPlayer.radius };

		AABB playerAABB = { playerMin, playerMax };

		uint64_t count = 0;

		auto startTime = std::chrono::high_resolution_clock::now();
		std::deque<size_t> tris;
		tris = octree->getElements(playerAABB, count);
		auto gotTrisTime = std::chrono::high_resolution_clock::now();

		std::vector<std::pair<float, size_t>> collisions;
		SphereCollisionResults result;
		for (size_t triId : tris) {
			if ((result = CapsuleVsTriangle(tempPlayer, amaz::getTriFromId(triId))).collided) {
				collisions.push_back({ result.penetration_depth, triId });
			}
		}

		auto firstCollideTime = std::chrono::high_resolution_clock::now();

		std::sort(collisions.begin(), collisions.end(), [](const auto& a, const auto& b) {
			return a.first > b.first;
			});

		auto sortedTrisTime = std::chrono::high_resolution_clock::now();

		SphereCollisionResults collision;
		for (auto& [f, tri] : collisions) {
			if ((collision = CapsuleVsTriangle(tempPlayer, amaz::getTriFromId(tri))).collided) {

				if (collision.penetration_normal.y > 0.f) {
					isGrounded = true;
				}

				moveVector += (collision.penetration_normal * collision.penetration_depth);
				newPos = pos + moveVector;
				tempPlayer.tip = player.tip + newPos;
				tempPlayer.base = player.base + newPos;

			}
		}

		auto collidedTrisTime = std::chrono::high_resolution_clock::now();

		return true;

	}

	void Physics::stepLogic(Input& input, float seconds) {

		glm::vec3 movementVector = glm::vec3{ 0.f };

		if (input.flying) {

			if (input.moveForward)
				movementVector += glm::normalize(input.camDir) * 0.1f;
			if (input.moveBack)
				movementVector -= glm::normalize(input.camDir) * 0.1f;
			if (input.moveLeft)
				movementVector -= glm::normalize(glm::cross(input.camDir, { 0, 1, 0 })) * 0.1f;
			if (input.moveRight)
				movementVector += glm::normalize(glm::cross(input.camDir, { 0, 1, 0 })) * 0.1f;
			if (input.moveDown)
				movementVector.y -= 0.1f;
			if (input.moveUp)
				movementVector.y += 0.1f;

			if (glm::length(movementVector) > 0.f) {
				movementVector = glm::normalize(movementVector) * (24 * seconds);
				
				input.camPos += movementVector;
			}
		}
		else {
			if (input.moveForward)
				movementVector += glm::normalize(glm::vec3{ input.camDir.x, 0.f, input.camDir.z }) * 0.1f;
			if (input.moveBack)
				movementVector -= glm::normalize(glm::vec3{ input.camDir.x, 0.f, input.camDir.z }) * 0.1f;
			if (input.moveLeft)
				movementVector -= glm::normalize(glm::cross(input.camDir, { 0, 1, 0 })) * 0.1f;
			if (input.moveRight)
				movementVector += glm::normalize(glm::cross(input.camDir, { 0, 1, 0 })) * 0.1f;

			if (glm::length(movementVector) > 0.f)
				movementVector = glm::normalize(movementVector) * (12 * seconds);

			if (input.jump && !jumped) {
				if (isGrounded) {
					jumpTime = 0;
					jumped = true;
					yVelocity += 18;
				}
				else if (jumpTime > 0.2f)
					jumped = true;
				else {
					jumpTime += seconds;
				}
			}

			if (!input.jump) {
				jumped = false;
				jumpTime = 0;
			}

			if (yVelocity > -240.0f) {
				yVelocity -= 36.f * seconds;
			}

			isGrounded = false;

			movementVector += glm::vec3{ 0, yVelocity * seconds, 0 };
			//TODO: Make gravity not slide you down slopes
			newResolveCollisions(input.camPos, movementVector);
			input.camPos += movementVector;


			if (isGrounded) {
				yVelocity = 0;
			}
		}


	}

	float Physics::distBetweenPoints(glm::vec3 a, glm::vec3 b) {
		return glm::sqrt(squaredDistBetweenPoints(a, b));
	}

	float Physics::squaredDistBetweenPoints(glm::vec3 a, glm::vec3 b) {
		glm::vec3 c = glm::abs(b - a);
		return std::pow(c.x, 2) + std::pow(c.y, 2) + std::pow(c.z, 2);
	}

	bool Physics::SphereVsSphere(Sphere a, Sphere b) {
		if (squaredDistBetweenPoints(a.pos, b.pos) < (a.radius + b.radius) * (a.radius + b.radius))
			return true;
		else
			return false;
	}

	glm::vec3 Physics::closestPointOnLine(glm::vec3 a, glm::vec3 b, glm::vec3 point) {
		glm::vec3 ab = b - a;
		float t = dot(point - a, ab) / dot(ab, ab);
		return a + glm::clamp(t, 0.f, 1.f) * ab;
	}

	bool Physics::SphereVsLine(Sphere sphere, glm::vec3 a, glm::vec3 b, glm::vec3& point) {
		point = closestPointOnLine(a, b, sphere.pos);
		glm::vec3 v = sphere.pos - point;
		float distsq = dot(v, v);
		return distsq < std::pow(sphere.radius, 2);
	}

	glm::vec3 Physics::trianglePlaneNormal(Triangle tri) {
		return glm::normalize(glm::cross(tri.b - tri.a, tri.c - tri.a));
	}

	Physics::SphereCollisionResults Physics::SphereVsTriangle(Sphere sphere, Triangle tri) {
		glm::vec3 N = trianglePlaneNormal(tri);
		float dist = glm::dot(sphere.pos - tri.a, N); // absolute distance between sphere and plane
		if (glm::abs(dist) > sphere.radius) // Not on triangles plane, return false
			return { false, {}, 0.f };

		glm::vec3 point0 = sphere.pos - N * dist; // projected sphere center on triangle plane


		// Now determine whether point0 is inside all triangle edges:
		glm::vec3 c0 = glm::cross(point0 - tri.a, tri.b - tri.a);
		glm::vec3 c1 = glm::cross(point0 - tri.b, tri.c - tri.b);
		glm::vec3 c2 = glm::cross(point0 - tri.c, tri.a - tri.c);
		bool inside = glm::dot(c0, N) <= 0 && glm::dot(c1, N) <= 0 && glm::dot(c2, N) <= 0;
		bool intersects;
		glm::vec3 point1, point2, point3;

		if (!inside) {
			intersects = SphereVsLine(sphere, tri.a, tri.b, point1)
				|| SphereVsLine(sphere, tri.b, tri.c, point2)
				|| SphereVsLine(sphere, tri.c, tri.a, point3);
		}
		if (inside || intersects)
		{
			glm::vec3 intersection_vec;

			if (inside)
			{
				intersection_vec = sphere.pos - point0;
			}
			else
			{
				glm::vec3 d = sphere.pos - point1;
				float best_distsq = glm::dot(d, d);
				intersection_vec = d;

				d = sphere.pos - point2;
				float distsq = glm::dot(d, d);
				if (distsq < best_distsq)
				{
					best_distsq = distsq;
					intersection_vec = d;
				}

				d = sphere.pos - point3;
				distsq = glm::dot(d, d);
				if (distsq < best_distsq)
				{
					best_distsq = distsq;
					intersection_vec = d;
				}
			}

			float len = glm::length(intersection_vec);
			//glm::vec3 penetration_normal = intersection_vec / len;  // normalize
			glm::vec3 penetration_normal = N;
			float penetration_depth = sphere.radius - len;
			return { true, penetration_normal, penetration_depth };
		}
		return { false, {}, 0.f };
	}

	CapsuleEndPoints Physics::calcCapsuleEndpoints(Capsule capsule) {
		glm::vec3 normalized = glm::normalize(capsule.tip - capsule.base);
		glm::vec3 lineEndOffset = normalized * capsule.radius;
		glm::vec3 a = capsule.base + lineEndOffset;
		glm::vec3 b = capsule.tip - lineEndOffset;

		return { a, b };
	}

	bool Physics::CapsuleVsSphere(Capsule capsule, Sphere sphere) {
		auto endPoints = calcCapsuleEndpoints(capsule);
		glm::vec3 capsulePoint = closestPointOnLine(endPoints.a, endPoints.b, sphere.pos);

		return SphereVsSphere({ capsulePoint, capsule.radius }, sphere);
	}

	bool Physics::CapsuleVsCapsule(Capsule a, Capsule b) {

		auto endPointsA = calcCapsuleEndpoints(a);
		auto endPointsB = calcCapsuleEndpoints(b);

		// vectors between line endpoints:
		glm::vec3 v0 = endPointsB.a - endPointsA.a;
		glm::vec3 v1 = endPointsB.b - endPointsA.a;
		glm::vec3 v2 = endPointsB.a - endPointsA.b;
		glm::vec3 v3 = endPointsB.b - endPointsA.b;

		float d0 = dot(v0, v0);
		float d1 = dot(v1, v1);
		float d2 = dot(v2, v2);
		float d3 = dot(v3, v3);

		glm::vec3 bestA;
		if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1)
			bestA = endPointsA.b;
		else
			bestA = endPointsA.a;

		glm::vec3 bestB = closestPointOnLine(endPointsB.a, endPointsB.b, bestA);

		glm::vec3 penetration_normal = bestA - bestB;
		float len = glm::length(penetration_normal);
		penetration_normal /= len;  // normalize
		float penetration_depth = a.radius + b.radius - len;
		bool intersects = penetration_depth > 0;

		return intersects;
	}

	glm::vec3 Physics::closestPointOnTriangle(Triangle tri, glm::vec3 point) {
		glm::vec3 N = trianglePlaneNormal(tri);
		glm::vec3 c0 = glm::cross(point - tri.a, tri.b - tri.a);
		glm::vec3 c1 = glm::cross(point - tri.b, tri.c - tri.b);
		glm::vec3 c2 = glm::cross(point - tri.c, tri.a - tri.c);
		bool inside = glm::dot(c0, N) <= 0 && dot(c1, N) <= 0 && dot(c2, N) <= 0;

		glm::vec3 outPoint;
		if (inside)
		{
			outPoint = point;
		}
		else
		{
			// Edge 1:
			glm::vec3 point1 = closestPointOnLine(tri.a, tri.b, point);
			glm::vec3 v1 = point - point1;
			float distsq = glm::dot(v1, v1);
			float best_dist = distsq;
			outPoint = point1;

			// Edge 2:
			glm::vec3 point2 = closestPointOnLine(tri.b, tri.c, point);
			glm::vec3 v2 = point - point2;
			distsq = dot(v2, v2);
			if (distsq < best_dist)
			{
				outPoint = point2;
				best_dist = distsq;
			}

			// Edge 3:
			glm::vec3 point3 = closestPointOnLine(tri.c, tri.a, point);
			glm::vec3 v3 = point - point3;
			distsq = dot(v3, v3);
			if (distsq < best_dist)
			{
				outPoint = point3;
				best_dist = distsq;
			}
		}
		return outPoint;
	}

	Physics::SphereCollisionResults Physics::CapsuleVsTriangle(Capsule capsule, Triangle tri) {
		auto [a, b] = calcCapsuleEndpoints(capsule);
		glm::vec3 normalizedCapsule = glm::normalize(capsule.tip - capsule.base);

		glm::vec3 N = trianglePlaneNormal(tri);

		float absoluteDot = glm::abs(glm::dot(N, normalizedCapsule));

		glm::vec3 center;

		if (absoluteDot == 0) {
			glm::vec3 triBestA = closestPointOnTriangle(tri, a);
			glm::vec3 triBestB = closestPointOnTriangle(tri, b);

			glm::vec3 bestA = closestPointOnLine(a, b, triBestA);
			glm::vec3 bestB = closestPointOnLine(a, b, triBestB);

			if (squaredDistBetweenPoints(bestA, triBestA) < squaredDistBetweenPoints(bestB, triBestB))
				center = bestA;
			else
				center = bestB;


		}
		else {
			float t = glm::dot(N, (tri.a - capsule.base) / absoluteDot);

			//std::cout << t << "\n";
			glm::vec3 line_plane_intersection = capsule.base + normalizedCapsule * t;
			glm::vec3 reference_point = closestPointOnTriangle(tri, line_plane_intersection);

			center = closestPointOnLine(a, b, reference_point);
		}



		/*glm::vec3 intersection_vec = center - reference_point;
		float len = glm::length(intersection_vec);
		bool collided = len < capsule.radius;
		glm::vec3 penetration_normal = intersection_vec / len;
		float penetration_depth = capsule.radius - len;

		//return { collided, penetration_normal, penetration_depth};*/

		return SphereVsTriangle({ center, capsule.radius }, tri);
	}
};