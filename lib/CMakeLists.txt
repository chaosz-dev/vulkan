set(NAME vkcourse)
add_library(${NAME} STATIC
    buffer.cpp
    descriptors.cpp
    texture.cpp
)

target_include_directories(${NAME}
    PUBLIC ${Vulkan_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${NAME}
    PUBLIC Vulkan::Vulkan stb
)

