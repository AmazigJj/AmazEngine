#pragma once

#include "Objects.h"
#include "Collision.h"
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>

namespace amaz {

	class OcNodeObject;
	struct OcNode;

	class Octree : std::enable_shared_from_this<Octree> {
	public:

		//TODO: make octree dynamically resize maybe?
		[[nodiscard]] static std::shared_ptr<Octree> create(AABB maxSize);
		
		OcNodeObject getRoot();

		void addElement(size_t nodeId, size_t tri);
		void addElement(size_t tri) {
			addElement(0, tri);
		}

		float getArea(size_t id);

		std::deque<size_t> getElements(size_t nodeId, AABB aabb, size_t& count);
		std::deque<size_t> getElements(AABB aabb, size_t& count);

		void genChildren(size_t nodeId);
		void moveElementsToChildren(size_t nodeId);

		auto getPtr() {
			return shared_from_this();
		}

		void genAll();


	private:
		Octree(AABB aabb);
		std::vector<OcNode> nodes;
	};

	struct OcNode {
		OcNode(AABB aabb) {
			this->aabb = aabb;
		}
		AABB aabb;
		size_t children = 0;
		std::deque<size_t> elements;
		bool dirty = false;
	};

	class OcNodeObject {
	public:
		void addElement(size_t tri);
		float getArea();
		std::deque<size_t> getElements(AABB aabb, size_t& count);

		size_t id;
		std::shared_ptr<Octree> octree;
	};
}