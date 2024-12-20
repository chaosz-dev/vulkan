add_executable(03_triangle_vertex
    03_triangle_vertex.cpp
)

target_include_directories(03_triangle_vertex
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS}
)

target_link_libraries(03_triangle_vertex
    PRIVATE glfw Vulkan::Vulkan imgui
)

add_shader(03_triangle_vertex triangle_in.vert SPV_triangle_in_vert)
add_shader(03_triangle_vertex triangle_in.frag SPV_triangle_in_frag)
