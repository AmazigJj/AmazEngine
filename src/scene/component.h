#include <glm/vec3.hpp>

namespace amaz {
	enum class Components {
		TRANSFORM = 1,
		MOVEMENT = 2
	};

	struct BaseComponent {

	};

	struct tranformComponent : BaseComponent {
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};
}