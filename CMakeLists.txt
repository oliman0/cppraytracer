cmake_minimum_required(VERSION 4.0)

SET(SHADERC ${CMAKE_SOURCE_DIR}/util/dxc/dxc.exe)

function(add_shaders TARGET_NAME)
    set(SHADER_SOURCE_FILES ${ARGN})

    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "Cannot create a shaders target without any source files")
    endif()

    set(SHADER_COMMANDS)
    set(SHADER_PRODUCTS)

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE STEM LAST_ONLY SHADER_NAME)
        cmake_path(GET SHADER_SOURCE EXTENSION SHADER_EXT)

        list(APPEND SHADER_COMMANDS COMMAND)
        list(APPEND SHADER_COMMANDS ${SHADERC})

        if(SHADER_EXT STREQUAL ".comp.hlsl")
            list(APPEND SHADER_COMMANDS -T cs_6_0)
        elseif(SHADER_EXT STREQUAL ".frag.hlsl")
            list(APPEND SHADER_COMMANDS -T ps_6_0)
        elseif(SHADER_EXT STREQUAL ".vert.hlsl")
            list(APPEND SHADER_COMMANDS -T vs_6_0)
        endif()

        list(APPEND SHADER_COMMANDS -E main -spirv)
        list(APPEND SHADER_COMMANDS ${SHADER_SOURCE})
        list(APPEND SHADER_COMMANDS -Fo ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv)

        list(APPEND SHADER_PRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")

    endforeach()

    add_custom_target(${TARGET_NAME}
        ${SHADER_COMMANDS}
        COMMENT "Compiling Shaders [${TARGET_NAME}]"
        SOURCES ${SHADER_SOURCE_FILES}
        BYPRODUCTS ${SHADER_PRODUCTS}
    )
endfunction()

project(raytracer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_shaders(shader src/shader/screenShader.frag.hlsl src/shader/screenShader.vert.hlsl src/shader/raytracing.comp.hlsl)

add_executable(raytracer
        src/core/main.cpp
        src/core/App.cpp
        src/core/Camera.cpp
)
target_link_libraries(raytracer PUBLIC "${CMAKE_SOURCE_DIR}/extern/lib/SDL3.lib")
add_dependencies(raytracer shader)

include_directories(extern/include)

add_custom_command(TARGET raytracer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/extern/lib/SDL3.dll" $<TARGET_FILE_DIR:raytracer>)