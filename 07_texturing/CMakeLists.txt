set(NAME 07_texturing)
add_executable(${NAME}
    07_texturing.cpp

    grid.cpp
    shader_tooling.cpp
)

target_include_directories(${NAME}
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(${NAME}
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shader(${NAME} triangle_in.vert SPV_triangle_in_vert)
add_shader(${NAME} triangle_in.frag SPV_triangle_in_frag)

add_shader(${NAME} grid.vert SPV_grid_vert)
add_shader(${NAME} grid.frag SPV_grid_frag)
