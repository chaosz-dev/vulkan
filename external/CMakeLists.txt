
set(IMGUI_SRC
    imgui/imconfig.h
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui.h
    imgui/imgui_internal.h
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h

    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_glfw.h

    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_vulkan.h
)

# On windows remember to run `git submodule init`
IF(WIN32)
  add_subdirectory(glfw)
ENDIF()

add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

target_link_libraries(imgui Vulkan::Vulkan glfw)
