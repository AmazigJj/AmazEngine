#pragma once

#include <glm/glm.hpp>
#include <SDL.h>

struct Input {
	glm::vec3 camPos = { 0.f,6.f,10.f };

	glm::vec3 camDir = { 0.f,0.f,-1.f };

	float yaw = -90.f;
	float pitch = 0.f;

	bool moveForward = false;
	bool moveBack = false;
	bool moveLeft = false;
	bool moveRight = false;
	bool moveDown = false;
	bool moveUp = false;
	bool jump = false;
	bool isGrounded = true;
	bool flying = true;

	bool isQuitting = false;

	bool thirdPerson = false;

	float sens = 10.f;

	glm::vec3 velVector = glm::vec3(0.f);

	uint32_t winWidth = 1600;
	uint32_t winHeight = 900;
	uint32_t fullScreenWidth = 1920;
	uint32_t fullScreenHeight = 1080;
	bool fullScreen = false;
	bool winChanged = false;
};

void handleEvents(Input& input, SDL_Window* window);