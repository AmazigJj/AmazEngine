#include "Input.h"
#include "SDL_events.h"

#include <SDL.h>
#include <glm/gtx/transform.hpp>
#include <backends/imgui_impl_sdl.h>
#include <iostream>

void handleEvents(Input& input, SDL_Window* window) {
	SDL_Event e;

	static bool mouseMode = 0;

	bool winUnfullscreened = false;

	while (SDL_PollEvent(&e) != 0)
	{
		ImGui_ImplSDL2_ProcessEvent(&e);

		//close the window when user alt-f4s or clicks the X button			
		if (e.type == SDL_QUIT) input.isQuitting = true;

		if (e.type == SDL_KEYDOWN) {

			switch (e.key.keysym.scancode) {
			case SDL_SCANCODE_SPACE:
				input.jump = true;
				
			break;case SDL_SCANCODE_W:
				input.moveForward = true;
			break;case SDL_SCANCODE_S:
				input.moveBack = true;

			break;case SDL_SCANCODE_A:
				input.moveLeft = true;
			break;case SDL_SCANCODE_D:
				input.moveRight = true;

			break;case SDL_SCANCODE_Q:
				input.moveDown = true;
			break;case SDL_SCANCODE_E:
				input.moveUp = true;
				

			case SDL_SCANCODE_ESCAPE:
				mouseMode = !mouseMode;
				SDL_SetRelativeMouseMode((SDL_bool)mouseMode);

			break;case SDL_SCANCODE_F5:
				input.thirdPerson = !input.thirdPerson;

			break;case SDL_SCANCODE_RETURN:
				if (!(e.key.keysym.mod & KMOD_ALT)) break;
				input.fullScreen = !input.fullScreen;
				SDL_SetWindowFullscreen(window, (input.fullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
				input.winChanged = true;
				winUnfullscreened = true;

			break;default:break;
			}
		}

		if (e.type == SDL_KEYUP) {
			switch (e.key.keysym.scancode) {
			case SDL_SCANCODE_SPACE:
				input.jump = false;

			break;case SDL_SCANCODE_W:
				input.moveForward = false;
			break;case SDL_SCANCODE_S:
				input.moveBack = false;

			break;case SDL_SCANCODE_A:
				input.moveLeft = false;
			break;case SDL_SCANCODE_D:
				input.moveRight = false;

			break;case SDL_SCANCODE_Q:
				input.moveDown = false;
			break;case SDL_SCANCODE_E:
				input.moveUp = false;
				
			break;default:break;
			}
		}

		if (e.type == SDL_MOUSEMOTION) {
			if (!mouseMode)
				continue;
			input.yaw += e.motion.xrel * input.sens * 0.01f;
			input.pitch -= e.motion.yrel * input.sens * 0.01f;
			input.pitch = std::min(std::max(-89.f, input.pitch), 89.f);

			glm::vec3 dir;
			dir.x = cos(glm::radians(input.yaw)) * cos(glm::radians(input.pitch));
			dir.y = sin(glm::radians(input.pitch));
			dir.z = sin(glm::radians(input.yaw)) * cos(glm::radians(input.pitch));
			input.camDir = glm::normalize(dir);

		}

		if (e.type == SDL_WINDOWEVENT) {
			switch (e.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					if (e.window.data1 == input.fullScreenWidth && e.window.data2 == input.fullScreenHeight) break;
					input.winChanged = true;
					input.winWidth = e.window.data1;
					input.winHeight = e.window.data2;
					std::cout << "WINDOW RESIZED: " << input.winWidth << "x" << input.winHeight << "\n";
			}
		}

	}
}