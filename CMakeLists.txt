# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)
project ("AmazEngine")

include(cmake/CPM.cmake)

find_package(Vulkan)
message("glsl validator " ${GLSL_VALIDATOR})

function(add_shaders target_name)
    foreach(GLSL ${ARGN})
    
      set(GLSL "${CMAKE_CURRENT_SOURCE_DIR}/${GLSL}")
      message(STATUS "BUILDING SHADER")
      get_filename_component(FILE_NAME ${GLSL} NAME)
      set(SPIRV "${SHADERS_OUTPUT_DIRECTORY}/${FILE_NAME}.spv")
      message(STATUS ${GLSL})
      message(STATUS ${SPIRV})
      ##execute glslang command to compile that specific shader
      add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
      list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(
        ${target_name}
        DEPENDS ${SPIRV_BINARY_FILES}
        )
endfunction(add_shaders target_name)

if (${CMAKE_SYSTEM_NAME} MATCHES Windows)
message(STATUS "Building AmazEngine for Windows")
set(COPY_COMMAND copy)
elseif(${CMAKE_SYSTEM_NAME} MATCHES Linux)
message(STATUS "Building AmazEngine for Linux")
set(COPY_COMMAND cp)
else()
message(STATUS "Not a target OS")
endif()


function(add_assets target_name)
    foreach(FILE ${ARGN})

        set(IN "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}")

        cmake_path(NATIVE_PATH IN IN)

        set(OUT "${ASSETS_OUTPUT_DIRECTORY}/${FILE}")

        cmake_path(NATIVE_PATH OUT OUT)
   
        add_custom_command(
        OUTPUT ${OUT}
        COMMAND ${COPY_COMMAND} \"${IN}\" \"${OUT}\"
        DEPENDS ${IN})
        list(APPEND ASSET_FILES ${OUT})
    endforeach(FILE)

    add_custom_target(
        ${target_name}
        DEPENDS ${ASSET_FILES}
        )
endfunction(add_assets target_name)



set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(SHADERS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
set(ASSETS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/assets)


add_subdirectory("third_party")

file(MAKE_DIRECTORY ${SHADERS_OUTPUT_DIRECTORY})

# Include sub-projects.
add_subdirectory ("src")

add_subdirectory("shaders")

add_subdirectory("assets")

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

message(STATUS ${GLSL_VALIDATOR})



