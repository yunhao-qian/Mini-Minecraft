cmake_minimum_required(VERSION 3.16)

project(mini-minecraft VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Additional warnings
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui OpenGL OpenGLWidgets Widgets)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863 # Tag 1.0.1
)
FetchContent_MakeAvailable(glm)

set(PROJECT_SOURCES
    src/main.cpp
    src/camera_controls_window.h
    src/camera_controls_window.cpp
    src/gl_context.h
    src/gl_context.cpp
    src/gl_widget.h
    src/gl_widget.cpp
    src/main_window.h
    src/main_window.cpp
    src/player_info_display_data.h
    src/player_info_window.h
    src/player_info_window.cpp
    src/shader_program.h
    src/shader_program.cpp
)

qt_add_executable(mini-minecraft
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    resources.qrc
)

target_link_libraries(mini-minecraft
    PRIVATE glm::glm Qt6::Gui Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Widgets
)

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
