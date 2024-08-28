
add_executable(01_window
    01_window.cpp
)

target_link_libraries(01_window
    PRIVATE glfw Vulkan::Vulkan imgui
)
