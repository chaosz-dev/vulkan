cmake_minimum_required(VERSION 3.20)
project(vkbasics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan 1.1 REQUIRED)

option(BUILD_GLFW "If enabled download and build glfw lib also" OFF)

if(NOT BUILD_GLFW)
    find_package(glfw3 3.3)
endif()

if(NOT glfw3_FOUND)
    message("-- Forcing GLFW3 download and build")
    set(BUILD_GLFW ON)
endif()



set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(external)

include(shader.cmake)

add_compile_options(-Wall)
if(NOT WIN32)
	add_compile_options(-Wextra -Werror)
endif()

add_subdirectory(lib)

#add_subdir(lib)
add_subdirectory(01_window)
add_subdirectory(02_triangle)
add_subdirectory(03_triangle_vertex)
add_subdirectory(04_cube)
add_subdirectory(05_moving)
add_subdirectory(06_ubo)
add_subdirectory(07_texturing)
add_subdirectory(08_light)
add_subdirectory(09_shadow)
add_subdirectory(10_postprocess)
add_subdirectory(11_msaa)
