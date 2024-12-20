include(FetchContent)

set(EXTERNALS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/ PARENT_SCOPE)

IF(BUILD_GLFW)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/glfw/
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()


FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1-light.zip
    #GIT_REPOSITORY  https://github.com/g-truc/glm.git
    #GIT_TAG     bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/glm/
    #DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(glm)

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

add_library(imgui STATIC ${IMGUI_SRC})
target_link_libraries(imgui
    PRIVATE glfw Vulkan::Vulkan)

target_include_directories(imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS}
)

add_library(stb STATIC
    stb/stb_wrapper.cpp
    stb/stb_image.h
)
target_include_directories(stb PUBLIC stb/)
