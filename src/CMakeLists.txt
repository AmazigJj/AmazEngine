cmake_minimum_required (VERSION 3.20)

add_executable (AmazEngine "AmazEngine.cpp" "renderer/vk_initializers.cpp" "renderer/vk_mesh.cpp" "physics/Collision.cpp" "physics/Octree.cpp" "renderer/Renderer.cpp" "input/Input.cpp" "physics/Physics.cpp" "scene/scene.cpp" "renderer/vk_descriptor.cpp" "renderer/vk_pipeline.cpp" "renderer/backend/VkBackendRenderer.cpp")

set_target_properties(AmazEngine PROPERTIES CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
message("VULKAN VERSION: " ${Vulkan_VERSION})

target_link_libraries(AmazEngine PUBLIC Vulkan::Vulkan vk-bootstrap::vk-bootstrap glm::glm SDL2::SDL2 SDL2::SDL2main stb_image tinyobjloader VulkanMemoryAllocator imgui nlohmann_json::nlohmann_json tinygltf)

add_dependencies(AmazEngine Shaders AmazEngineAssets)


# VMA has tons of nullability-completeness warnings, unable to use SYSTEM on target_link_libararies for some reason
target_compile_options(AmazEngine PRIVATE -Wno-nullability-completeness)