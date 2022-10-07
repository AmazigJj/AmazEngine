// AmazEngine.cpp : Defines the entry point for the application.
//
#include <chrono>

#include <iostream>
#include <cctype>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <functional>
#include "renderer/Renderer.h"
#include "input/Input.h"
#include "physics/Physics.h"
#include "glm/glm.hpp"
#include <variant>


using json = nlohmann::json;

const int SIM_RATE = 60;

uint32_t FRAME_LIMIT = 0;

using std::string;

const string ASSETS_PATH = "../assets/";

void loadScene(string name, Renderer& renderer, amaz::Physics& physics);

template <uint64_t T>
using frames = std::chrono::duration<double, std::ratio<1, T>>;

glm::vec3 interpolate(glm::vec3 oldPos, glm::vec3 newPos, float time) {

	if (time <= 0) {
		return oldPos;
	}

	if (time > 1) {
		time = 1;
	}

	glm::vec3 diff = (newPos - oldPos) * time;
	
	return oldPos + diff;
}



int main(int argc, char* argv[]) {

	Renderer renderer(1600, 900);

	amaz::Physics physics;

	loadScene("test", renderer, physics);

	bool running = true;

	bool unlimitedFps = false;
	auto nextFrame = std::chrono::high_resolution_clock::now() + frames<SIM_RATE>{ 0 };
	auto nextRenderFrame = std::chrono::high_resolution_clock::now();/* + frames<1>{ 0 };*/

	auto lastFrameTime = std::chrono::high_resolution_clock::now();

	Input inputs;
	glm::vec3 lastFramePos;

	float fps;
	int frametime = 0;

	const std::string main_thread_name = "main";
	while (running) {
		handleEvents(inputs, renderer.getWindow());
		if (inputs.isQuitting) {
			running = false;
		}

		auto time = std::chrono::high_resolution_clock::now();
		while (time > nextFrame) {
			//FrameMarkNamed(main_thread_name.c_str());
			lastFramePos = inputs.camPos;
			physics.stepLogic(inputs, 1.f/SIM_RATE);
			nextFrame += frames<SIM_RATE>{ 1 };

			// glm::vec3 playerPos = inputs.camPos;
			// renderer.setPlayerPos(playerPos);
			// renderer.setThirdPerson(inputs.thirdPerson);
			// renderer.draw(inputs.camDir, &inputs);

			//FrameMarkEnd(main_thread_name.c_str());

		}
		
		if (FRAME_LIMIT <= 0 || time > nextRenderFrame) {

			if (inputs.winChanged) {
				if (inputs.fullScreen) 
					renderer.recreateFrameBuffers(inputs.fullScreenWidth, inputs.fullScreenHeight);
				else
					renderer.recreateFrameBuffers(inputs.winWidth, inputs.winHeight);
				inputs.winChanged = false;
			}

			frames<SIM_RATE> timeToNextFrame = nextFrame - time;
			glm::vec3 playerPos = interpolate(lastFramePos, inputs.camPos, 1.f - timeToNextFrame.count());
			//glm::vec3 playerPos = inputs.camPos;
			renderer.setPlayerPos(playerPos);
			renderer.setThirdPerson(inputs.thirdPerson);
			renderer.frametime = frametime/1000.f; // convert to milliseconds
			renderer.draw(inputs.camDir, &inputs);

			auto thisFrameTime = std::chrono::high_resolution_clock::now();

			//frametime = (std::chrono::duration_cast<std::chrono::microseconds>(thisFrameTime - lastFrameTime).count() / 1000.0);

			frametime = std::chrono::duration_cast<std::chrono::microseconds>(thisFrameTime - lastFrameTime).count();

			fps = 1000000.f/frametime;

			// std::cout << "Frame time: " << frametime << "ms\n";
			// std::cout << "FPS: " << fps << "\n";
			lastFrameTime = thisFrameTime;

			//nextRenderFrame += frames<1>{ 1 };

			nextRenderFrame += std::chrono::microseconds(FRAME_LIMIT ? 1000000/FRAME_LIMIT : frametime);
		}
	}

	return 0;
}

void loadScene(string name, Renderer& renderer, amaz::Physics& physics) {

	std::cout << "Loading scene: " << name << "\n";
	string path = ASSETS_PATH + name + ".json";
	std::ifstream f(path);

	json data = json::parse(f);
	if (data.contains("version")) {
		std::cout << "Version: " << data["version"] << "\n";
	}

	if (data.contains("materials")) {
		for (auto& material : data["materials"]) {
			if (!material.contains("name")) {
				std::cout << "Material without name found, unable to load material";
				continue;
			}

			string matName = material["name"].get<string>();
			bool hasDiffuse = false;
			bool hasSpecular = false;

			if (material.contains("diffuseMap")) {
				hasDiffuse = true;
				renderer.loadImage(ASSETS_PATH + material["diffuseMap"].get<string>(), matName + "_diffuse");
			}
			
			if (material.contains("specularMap")) {
				hasSpecular = true;
				renderer.loadImage(ASSETS_PATH + material["specularMap"].get<string>(), matName + "_specular");
			}

			if (hasDiffuse) {
				if (hasSpecular) {
					renderer.registerMaterial("texturedmesh", matName, matName + "_diffuse", matName + "_specular");
				} else {
					renderer.registerMaterial("texturedmesh", matName, matName + "_diffuse");
				}
			} else {
				renderer.registerMaterial("texturedmesh", matName);
			}
		}
	}

	if (data.contains("scene")) {
		auto scene = data["scene"];
		if (scene.contains("meshes")) {
			auto meshes = scene["meshes"];
			for (auto& mesh : meshes) {
				if (!mesh.contains("name")) {
					std::cout << "Scene contains mesh without name, unable to load mesh\n";
					continue;
				}

				string meshName = mesh["name"].get<string>();

				glm::vec3 offset;
				float scale;
				glm::vec3 rotate;

				if (mesh.contains("offset")) {
					std::cout << "Offset: ";
					std::array<float, 3> offsets;
					int i = 0;
					for (auto offset : mesh["offset"]) {
						std::cout << offset.get<float>() << " ";
						offsets[i++] = offset.get<float>();
					}
					std::cout << "\n";
					offset = { offsets[0], offsets[1], offsets[2] };
				} else offset = { 0.f, 0.f, 0.f };

				if (mesh.contains("scale")) {
					scale = mesh["scale"].get<float>();
				} else scale = 1.f;

				if (mesh.contains("rotate")) {
					std::array<float, 3> rotations;
					int i = 0;
					for (auto rotate : mesh["rotate"]) {
						rotations[i++] = rotate.get<float>();
					}
					rotate = { rotations[0], rotations[1], rotations[2] };
				} else rotate = { 0.f, 0.f, 0.f };

				string material;
				if (mesh.contains("material")) {
					material = mesh["material"].get<string>();
				} else material = "defaultmesh";

				if (mesh.contains("renderMesh")) {
					std::cout << "Render mesh: " << meshName << " File name: " << mesh["renderMesh"] << "\n";
					renderer.loadMesh(meshName, ASSETS_PATH + mesh["renderMesh"].get<string>());
					renderer.registerRenderObject(meshName, material, offset, scale, rotate);
				}

				if (mesh.contains("collisonMesh")) {
					std::cout << mesh["collisonMesh"] << "\n";
					physics.loadMesh(meshName, ASSETS_PATH + mesh["collisonMesh"].get<string>(), offset, scale, rotate);
				}
			}
		}

		if (scene.contains("lights")) {
			auto lights = scene["lights"];

			for (auto light : lights) {

				glm::vec3 pos;
				glm::vec3 rgb;
				float constant = 1.0f;
				float linear = 0.14f;
				float quadratic = 0.07f;
				float radius = 1.f;

				if (light.contains("pos")) {
					std::array<float, 3> positions;
					int i = 0;
					for (auto pos : light["pos"]) {
						std::cout << pos.get<float>() << " ";
						positions[i++] = pos.get<float>();
					}
					pos = { positions[0], positions[1], positions[2] };
				} else pos = { 0, 0, 0 };

				if (light.contains("rgb")) {
					std::array<float, 3> rgbValues;
					int i = 0;
					for (auto rgbValue : light["rgb"]) {
						std::cout << rgbValue.get<float>() << " ";
						rgbValues[i++] = rgbValue.get<float>() / 255;
					}
					rgb = { rgbValues[0], rgbValues[1], rgbValues[2] };
				} else rgb = { 0, 0, 0 };

				if (light.contains("light")) {
					auto lightLevels = light["light"];

					if (lightLevels.contains("Constant")) {
						constant = lightLevels["Constant"].get<float>();
					}

					if (lightLevels.contains("Linear")) {
						linear = lightLevels["Linear"].get<float>();
					}

					if (lightLevels.contains("Quadratic")) {
						quadratic = lightLevels["Quadratic"].get<float>();
					}

					if (lightLevels.contains("radius")) {
						radius = lightLevels["radius"].get<float>();
					}

				}


				renderer.loadLight(pos, rgb, radius);

			}

		}
	}

}