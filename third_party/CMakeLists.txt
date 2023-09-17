cmake_minimum_required (VERSION 3.8)

# find_package(Vulkan REQUIRED)

# message("Vulkan found? " ${VULKAN_FOUND})
# message("Alright, where is it? " ${Vulkan_LIBRARY})
# message("And I can include it? " ${Vulkan_INCLUDE_DIR})

CPMAddPackage("gh:zeux/volk#1.3.250")

CPMAddPackage("gh:charles-lunarg/vk-bootstrap@0.7")

CPMAddPackage("gh:g-truc/glm#0.9.9.8")

set(SDL_VIDEO ON)
set(SDL_VULKAN ON)
# Unable to update because ninja complains "ninja: error: build.ninja:4288: multiple rules generate lib/SDL2.lib" (tried moving to 2.24.1 and 2.24.0)
CPMAddPackage("gh:libsdl-org/SDL#release-2.0.22")

CPMAddPackage("gh:tinyobjloader/tinyobjloader@2.0.0rc9")

CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#master")


set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
CPMAddPackage("gh:nlohmann/json@3.11.2")

CPMAddPackage("gh:ocornut/imgui@1.88")
if(imgui_ADDED)

	# add_compile_definitions(IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
	set(imgui_sources ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
		${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp)
	add_library(imgui STATIC ${imgui_sources})
	target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
	target_link_libraries(imgui PRIVATE volk::volk SDL2::SDL2)
endif()

CPMAddPackage("gh:wolfpld/tracy@0.8.2.1")

#set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
CPMAddPackage("gh:syoyo/tinygltf@2.6.3")

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

