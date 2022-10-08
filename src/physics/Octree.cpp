#include "Octree.h"

#include "Collision.h"
#include <iostream>


constexpr int MAX_OCTREE_ELEMENTS = 8;

namespace amaz {

	std::shared_ptr<Octree> Octree::create(AABB aabb) {
		return std::shared_ptr<Octree>(new Octree(aabb));
	}

	Octree::Octree(AABB aabb) {
		nodes.emplace_back(aabb);
	}

	void Octree::addElement(size_t nodeId, size_t tri) {
		OcNode& node = nodes[nodeId];

		node.elements.push_back(tri);
		if (node.children || (node.elements.size() > MAX_OCTREE_ELEMENTS && getArea(nodeId) > 1.f)) {
			node.dirty = true;
		}
	}

	float Octree::getArea(size_t id) {
		OcNode& node = nodes[id];
		return amaz::calcArea(node.aabb);
	}

	std::deque<size_t> Octree::getElements(AABB aabb, size_t& count) {
		return getElements(0, aabb, count);
	}

	std::deque<size_t> Octree::getElements(size_t nodeId, AABB aabb, size_t& count) {

		std::deque<size_t> output;
		if (nodeId > nodes.size()) {
			std::cout << "NodeId passed to Octree:getElements is higher than nodes size: " << nodes.size() << "\n";
			std::cout << "nodeId is: " << nodeId << "\n";
			return output;
		}

		std::vector<size_t> nodeIds;
		nodeIds.push_back(nodeId);

		while (!nodeIds.empty()) {
			count++;
			nodeId = nodeIds.back();
			nodeIds.pop_back();
			if (!amaz::AABBvsAABB(aabb, nodes[nodeId].aabb)) continue;
			if (nodes[nodeId].dirty) {
				if (!nodes[nodeId].children) {
					genChildren(nodeId);
				}
				moveElementsToChildren(nodeId);
				nodes[nodeId].dirty = false;
			}

			if (nodes[nodeId].children) {
				for (int i = nodes[nodeId].children; i < nodes[nodeId].children + 8; i++) {
					if (i > nodes.size()) {
						std::cout << "Higher than nodes size: " << nodes.size() << "\n";
						std::cout << "Source node is: " << nodeId << "\n";
						std::cout << "nodes first child should be: " << nodes[nodeId].children << "\n";
						std::cout << "i is: " << i << "\n";
					}
					nodeIds.push_back(i);
				}
			} else {
				std::copy(nodes[nodeId].elements.begin(), nodes[nodeId].elements.end(), std::back_inserter(output));
			}
		}

		return output;
	}

	void Octree::genChildren(size_t nodeId) {
		auto& node = nodes[nodeId];
		nodes[nodeId].children = nodes.size();

		glm::vec3 midPoint = (nodes[nodeId].aabb.a + nodes[nodeId].aabb.b) / 2.f;

		for (int i = 0; i < 8; i++) {
			float x = i > 3 ? nodes[nodeId].aabb.a.x : nodes[nodeId].aabb.b.x;		// 0, 1, 2, 3	4, 5, 6, 7
			float y = i % 4 > 1 ? nodes[nodeId].aabb.a.y : nodes[nodeId].aabb.b.y;	// 0, 1, 4, 5	2, 3, 6, 7
			float z = (i % 2) ? nodes[nodeId].aabb.a.z : nodes[nodeId].aabb.b.z;	// 0, 2, 4, 6	1, 3, 5, 7
			nodes.emplace_back(AABB{ midPoint, {x, y, z} });
		}
	}

	void Octree::moveElementsToChildren(size_t nodeId) {
		auto& node = nodes[nodeId];
		for (auto tri : node.elements) {

			AABB triAABB = amaz::getAABBFromTriangle(tri);
			for (int i = node.children; i < node.children + 8; i++) {
				if (i > nodes.size()) {
					std::cout << "Higher than nodes size: " << nodes.size() << "\n";
					std::cout << "Source node is: " << nodeId << "\n";
					std::cout << "nodes children is: " << node.children << "\n";
					std::cout << "nodes children should be: " << nodes[nodeId].children << "\n";
					std::cout << "i is: " << i << "\n";
				}
				auto& child = nodes[i];
				if (amaz::AABBvsAABB(triAABB, child.aabb)) {
					child.elements.push_back(tri);
					if (child.children || (/*child.elements.size() > MAX_OCTREE_ELEMENTS && */getArea(i) > 1.f))
						child.dirty = true;
				}
			}
		}
		node.elements.clear();
	}


	OcNodeObject Octree::getRoot() {
		return { 0, getPtr() };
	}

	void Octree::genAll() {

		std::vector<size_t> nodeIds;
		nodeIds.push_back(0);

		size_t nodeId;

		while (!nodeIds.empty()) {
			nodeId = nodeIds.back();
			nodeIds.pop_back();
			if (nodes[nodeId].dirty) {
				if (!nodes[nodeId].children) {
					genChildren(nodeId);
				}
				moveElementsToChildren(nodeId);
				nodes[nodeId].dirty = false;
				for (size_t i = nodes[nodeId].children; i < nodes[nodeId].children + 8; i++) {
					nodeIds.push_back(i);
				}
			}
		}
	}

	//Node functions
	void OcNodeObject::addElement(size_t tri) {
		octree->addElement(this->id, tri);
	}

	float OcNodeObject::getArea() {
		return octree->getArea(this->id);
	}

	std::deque<size_t> OcNodeObject::getElements(AABB aabb, size_t& count) {
		return octree->getElements(this->id, aabb, count);
	}

}