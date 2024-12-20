set(NAME 10_postprocess)
add_executable(${NAME}
    10_postprocess.cpp

    shadow_map.cpp
    lightning_pass.cpp

    post_process.cpp

    grid.cpp
    shader_tooling.cpp
)

target_include_directories(${NAME}
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(${NAME}
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shader(${NAME} shadow_map.vert SPV_shadow_map_vert)
add_shader(${NAME} shadow_map.frag SPV_shadow_map_frag)

add_shader(${NAME} lightning_simple.vert SPV_lightning_simple_vert)
add_shader(${NAME} lightning_simple.frag SPV_lightning_simple_frag)
add_shader(${NAME} lightning_no.frag SPV_lightning_no_frag)

add_shader(${NAME} lightning_shadowmap.vert SPV_lightning_shadowmap_vert)
add_shader(${NAME} lightning_shadowmap.frag SPV_lightning_shadowmap_frag)

add_shader(${NAME} post_process.vert SPV_post_process_vert)
add_shader(${NAME} post_process.frag SPV_post_process_frag)
