cmake_minimum_required(VERSION 3.20)
project(ObradexEngine VERSION 0.1.0 LANGUAGES CXX)

# ============================================================
# Project Configuration
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================
# Find System Dependencies
# ============================================================
find_package(OpenGL REQUIRED)

# GLFW - Windowing & Input
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.8
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# GLM - Math Library (header-only)
find_package(glm QUIET)
if(NOT glm_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()

# ============================================================
# Collect Source Files
# ============================================================
set(ENGINE_SOURCES
    src/main.cpp
    src/engine/Engine.cpp
    src/engine/Window.cpp
    src/engine/Input.cpp
    src/engine/Timer.cpp
    src/renderer/Renderer.cpp
    src/renderer/Shader.cpp
    src/renderer/Mesh.cpp
    src/renderer/Camera.cpp
    src/renderer/PostProcess.cpp
    src/world/World.cpp
    src/gameplay/Player.cpp
    src/gameplay/Interaction.cpp
    src/ui/EditorUI.cpp
    # GLAD (OpenGL loader) - bundled
    third_party/glad/src/glad.c
    # Dear ImGui - bundled
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

# ============================================================
# Build Target
# ============================================================
add_executable(ObradexEngine ${ENGINE_SOURCES})

target_include_directories(ObradexEngine PRIVATE
    src/
    third_party/glad/include
    third_party/imgui
    third_party/imgui/backends
    third_party/stb          # stb_image (header-only)
)

target_link_libraries(ObradexEngine PRIVATE
    OpenGL::GL
    glfw
    glm::glm
)

# Platform-specific linker flags
if(UNIX AND NOT APPLE)
    target_link_libraries(ObradexEngine PRIVATE dl)
endif()

# ============================================================
# Copy Assets to Build Directory
# ============================================================
add_custom_command(TARGET ObradexEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMENT "Copying assets to build directory..."
)

# ============================================================
# Compiler Warnings & Options
# ============================================================
if(MSVC)
    target_compile_options(ObradexEngine PRIVATE /W4)
else()
    target_compile_options(ObradexEngine PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter   # ImGui generates these
    )
endif()

# Debug vs Release
target_compile_definitions(ObradexEngine PRIVATE
    $<$<CONFIG:Debug>:OBRADEX_DEBUG>
    $<$<CONFIG:Release>:OBRADEX_RELEASE>
)

message(STATUS "ObradexEngine configured — build type: ${CMAKE_BUILD_TYPE}")
