function(add_shader SHADER_TARGET SHADER_FILE SHADER_VAR_NAME)
    set(SHADER_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE})
    set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}_include.h)

    # Create command which compiles the shader
    add_custom_command(OUTPUT ${SHADER_OUTPUT}
        DEPENDS ${SHADER_INPUT}
        COMMAND glslangValidator
                -V
                --variable-name ${SHADER_VAR_NAME}
                -I${CMAKE_CURRENT_SOURCE_DIR}
                ${SHADER_INPUT}
                -o ${SHADER_OUTPUT}
    )
    add_custom_target(${SHADER_TARGET}-spv-${SHADER_FILE} DEPENDS ${SHADER_OUTPUT})
    add_dependencies(${SHADER_TARGET} ${SHADER_TARGET}-spv-${SHADER_FILE})
    target_include_directories(${SHADER_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_shader)
