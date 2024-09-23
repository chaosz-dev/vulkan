cmake_minimum_required(VERSION 3.20)
project(vkbasics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# glfw can not be installed as a package on windows so this find_package would fail
IF(NOT WIN32)
  find_package(glfw3 3.3 REQUIRED)
ENDIF()

find_package(Vulkan 1.1 REQUIRED)

# msvc does not know these, remove on windows build
IF(NOT WIN32)
  add_compile_options(-Wall -Wextra -Werror)
ENDIF()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(external)
#add_subdir(lib)
add_subdirectory(01_window)
