cmake_minimum_required(VERSION 3.16)

project(mini-minecraft VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable additional compiler warnings.
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets Widgets)

# Python is only used at build time to preprocess shader files.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Download the glm library.
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863 # Tag 1.0.1
)
FetchContent_MakeAvailable(glm)

set(PROJECT_SOURCES
    resources/static_resources.qrc
    src/main.cpp
    src/aligned_box_3d.h
    src/aligned_box_3d.cpp
    src/array_texture_2d.h
    src/array_texture_2d.cpp
    src/block_face_generation_task.h
    src/block_face_generation_task.cpp
    src/block_type.h
    src/camera.h
    src/camera.cpp
    src/camera_controls_window.h
    src/camera_controls_window.cpp
    src/direction.h
    src/entity.h
    src/entity.cpp
    src/geometry_framebuffer.h
    src/geometry_framebuffer.cpp
    src/instanced_renderer.h
    src/ivec2_hash.h
    src/main_window.h
    src/main_window.cpp
    src/movement_mode.h
    src/opengl_context.h
    src/opengl_context.cpp
    src/opengl_object.h
    src/opengl_widget.h
    src/opengl_widget.cpp
    src/player_controller.h
    src/player_controller.cpp
    src/player_info_display_data.h
    src/player_info_window.h
    src/player_info_window.cpp
    src/pose.h
    src/scene.h
    src/scope_guard.h
    src/shader_program.h
    src/shader_program.cpp
    src/shadow_map_camera.h
    src/shadow_map_camera.cpp
    src/shadow_map_framebuffer.h
    src/shadow_map_framebuffer.cpp
    src/terrain.h
    src/terrain.cpp
    src/terrain_chunk.h
    src/terrain_chunk.cpp
    src/terrain_chunk_generation_task.h
    src/terrain_chunk_generation_task.cpp
    src/terrain_streamer.h
    src/terrain_streamer.cpp
    src/vertex_attribute.h
    src/water_wave.h
    src/water_wave.cpp
)

# Collect all shader files in the source directory.
set(ALL_SHADER_SOURCES "")
set(ENTRY_POINT_SHADER_SOURCES "")
foreach(SHADER_SOURCE
    block_face.glsl
    block_type.glsl
    geometry.frag.glsl
    geometry.vert.glsl
    lighting.frag.glsl
    quad.vert.glsl
    shadow_depth.frag.glsl
    shadow_depth.vert.glsl
    water_wave.glsl
)
    set(SHADER_SOURCE_FULL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/${SHADER_SOURCE}")
    list(APPEND ALL_SHADER_SOURCES "${SHADER_SOURCE_FULL_PATH}")
    if (SHADER_SOURCE MATCHES "\\.(vert|frag)\\.glsl$")
        list(APPEND ENTRY_POINT_SHADER_SOURCES "${SHADER_SOURCE_FULL_PATH}")
    endif()
endforeach()

# Generate shaders.qrc based on the collected shader files.
set(SHADERS_QRC_CONTENT "")
string(APPEND SHADERS_QRC_CONTENT "<RCC>\n")
string(APPEND SHADERS_QRC_CONTENT "    <qresource prefix=\"/shaders\">\n")

# Preprocess shader files using Python.
set(PREPROCESSED_SHADER_SOURCES "")
foreach(SHADER_SOURCE ${ENTRY_POINT_SHADER_SOURCES})
    get_filename_component(SHADER_SOURCE_NAME "${SHADER_SOURCE}" NAME)
    set(PREPROCESSED_SHADER_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_SOURCE_NAME}")
    # Shader dependency tracking is intentionally simplified. Since these files are lightweight, we
    # choose to reprocess all shader files whenever any of them change.
    add_custom_command(
        OUTPUT "${PREPROCESSED_SHADER_SOURCE}"
        COMMAND
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/preprocess_shader.py"
            "${SHADER_SOURCE}"
            "${PREPROCESSED_SHADER_SOURCE}"
        DEPENDS ${ALL_SHADER_SOURCES}
        COMMENT "Preprocessing ${SHADER_SOURCE}"
    )
    list(APPEND PREPROCESSED_SHADER_SOURCES "${PREPROCESSED_SHADER_SOURCE}")

    # Add the preprocessed shader file to the qrc file.
    string(APPEND SHADERS_QRC_CONTENT "        <file>${SHADER_SOURCE_NAME}</file>\n")
endforeach()

# Write the qrc file to the build directory.
string(APPEND SHADERS_QRC_CONTENT "    </qresource>\n")
string(APPEND SHADERS_QRC_CONTENT "</RCC>\n")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(SHADERS_QRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/shaders.qrc")
file(WRITE "${SHADERS_QRC_FILE}" "${SHADERS_QRC_CONTENT}")

qt_add_executable(mini-minecraft MANUAL_FINALIZATION ${PROJECT_SOURCES} "${SHADERS_QRC_FILE}")

# Add the preprocessed shader files as dependencies of the executable target.
add_custom_target(preprocess-shaders ALL DEPENDS ${PREPROCESSED_SHADER_SOURCES})
add_dependencies(mini-minecraft preprocess-shaders)

option(MINECRAFT_NO_GL_ERROR_CHECK "Disable OpenGLContext::checkError()" OFF)
if (MINECRAFT_NO_GL_ERROR_CHECK)
    target_compile_definitions(mini-minecraft PRIVATE MINECRAFT_NO_GL_ERROR_CHECK)
endif()

target_link_libraries(mini-minecraft PRIVATE glm::glm Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Widgets)

set_target_properties(mini-minecraft PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS mini-minecraft
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_target(mini-minecraft)
