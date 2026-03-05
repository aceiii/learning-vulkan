function (add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "OUTPUT" "SOURCES" ${ARGN})
    set(SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
    set(ENTRY_POINTS -entry vertMain -entry fragMain)
    list(TRANSFORM SHADER_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
    set(OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_OUTPUT}")
    message("SHADER_OUTPUT: " ${SHADER_OUTPUT})
    add_custom_command(
        OUTPUT ${SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${OUTPUT_FILE}
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
        COMMENT "Compiling Slang Shaders"
        VERBATIM
    )
    add_custom_target(${TARGET} DEPENDS ${OUTPUT_FILE})
endfunction()
