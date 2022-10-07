#include <vector>
#include <bitset>
#include <glm/glm.hpp>
#include <functional>

namespace amaz::eng {

enum class Components {
	TRANSFORM = 1
};

struct TransformComponent {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

struct Entity {
	size_t ID;
	std::bitset<64> bitmask;
};

struct System {
	std::bitset<64> mask;
	std::function<void(void)> func;
};

template<typename T>
struct ComponentArray {
	std::vector<T> components;
	std::vector<size_t> empty;
};

struct Scene {
	std::vector<Entity> _entities;
	std::vector<TransformComponent> transformComponents;

	std::vector<System> systems;

	size_t addEntity(){
		_entities.push_back({_entities.size(), 0});
		return _entities.back().ID;
	}

	void addComponent(size_t entityId, size_t componentId) {
		Entity& entity = _entities[entityId];

		entity.bitmask.set(componentId);
	}

	void addSystem(std::bitset<64> mask, std::function<void(void)> func) {
		systems.push_back({mask, func});
	}

	void removeComponent(size_t entityId, size_t componentId);
};

}