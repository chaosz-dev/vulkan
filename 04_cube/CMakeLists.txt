add_executable(04_cube
    04_cube.cpp
)

target_include_directories(04_cube
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(04_cube
    PRIVATE glfw Vulkan::Vulkan imgui
)

add_shader(04_cube triangle_in.vert SPV_triangle_in_vert)
add_shader(04_cube triangle_in.frag SPV_triangle_in_frag)


add_executable(04_cube_indexed
    04_cube_indexed.cpp
)

target_include_directories(04_cube_indexed
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(04_cube_indexed
    PRIVATE glfw Vulkan::Vulkan imgui
)
