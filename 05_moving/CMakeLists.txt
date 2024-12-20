add_executable(05_moving
    05_moving.cpp

    grid.cpp
    shader_tooling.cpp
)

target_include_directories(05_moving
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(05_moving
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shader(05_moving triangle_in.vert SPV_triangle_in_vert)
add_shader(05_moving triangle_in.frag SPV_triangle_in_frag)

add_shader(05_moving grid.vert SPV_grid_vert)
add_shader(05_moving grid.frag SPV_grid_frag)
