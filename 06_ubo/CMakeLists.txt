add_executable(06_ubo
    06_ubo.cpp

    grid.cpp
    shader_tooling.cpp
)

target_include_directories(06_ubo
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(06_ubo
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shader(06_ubo triangle_in.vert SPV_triangle_in_vert)
add_shader(06_ubo triangle_in.frag SPV_triangle_in_frag)

add_shader(06_ubo grid.vert SPV_grid_vert)
add_shader(06_ubo grid.frag SPV_grid_frag)
