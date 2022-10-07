#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_cross_product.hpp>
#include <algorithm>
#include <iostream>
#include <ranges>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <array>
#include "Objects.h"
#include "Octree.h"

#include "Collision.h"
#include <deque>
#include <chrono>
#include "../input/Input.h"

namespace amaz {
	struct CollisionObject {
		glm::vec3 min;
		glm::vec3 max;
	};

	struct CapsuleEndPoints {
		glm::vec3 a, b;
	};

	glm::mat4 calcTransformMatrix(glm::vec3 position, float scale, glm::vec3 rotation);

	struct ColMesh {
		std::vector<Triangle> tris;
		bool loadObj(std::string filename, float scale = 0);
		void moveObj(glm::vec3 position, float scale, glm::vec3 rotation);
	};

	class Physics {
	public:
		Physics();
		void initCollision();
		void stepLogic(Input& input, float seconds);
		void loadMesh(std::string name, std::string filename, glm::vec3 position, float scale, glm::vec3 rotation);

		//move these to collision?
		bool newResolveCollisions(glm::vec3 pos, glm::vec3& moveVector);
		void detectCollision(glm::vec3 pos, glm::vec3 movementVec);

		// def move to collision
		float distBetweenPoints(glm::vec3 a, glm::vec3 b);
		float squaredDistBetweenPoints(glm::vec3 a, glm::vec3 b);
		bool SphereVsSphere(Sphere a, Sphere b);
		glm::vec3 closestPointOnLine(glm::vec3 a, glm::vec3 b, glm::vec3 point);
		bool SphereVsLine(Sphere sphere, glm::vec3 a, glm::vec3 b, glm::vec3& point);
		glm::vec3 trianglePlaneNormal(Triangle tri);

		struct SphereCollisionResults {
			bool collided;
			glm::vec3 penetration_normal;
			float penetration_depth;
		};
		SphereCollisionResults SphereVsTriangle(Sphere sphere, Triangle tri);

		CapsuleEndPoints calcCapsuleEndpoints(Capsule capsule);
		bool CapsuleVsSphere(Capsule capsule, Sphere sphere);
		bool CapsuleVsCapsule(Capsule a, Capsule b);
		glm::vec3 closestPointOnTriangle(Triangle tri, glm::vec3 point);
		SphereCollisionResults CapsuleVsTriangle(Capsule capsule, Triangle tri);

	private:
		std::vector<CollisionObject> _collisionObjects; // old
		std::vector<ColMesh> _meshes;
		std::unordered_map<std::string, int> _meshIDs;
		std::shared_ptr<amaz::Octree> octree;

		Capsule player;
		Sphere playerSphere;

		float yVelocity = 0.f;
		bool jumped = false;
		bool isGrounded = false;
		float jumpTime = 0;
	};

}