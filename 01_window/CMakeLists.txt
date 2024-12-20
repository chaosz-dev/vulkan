
add_executable(01_window
    01_window.cpp
)

target_include_directories(01_window
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS}
)

target_link_libraries(01_window
    PRIVATE glfw Vulkan::Vulkan imgui
)
