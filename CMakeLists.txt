cmake_minimum_required(VERSION 3.20)

project(PacmanOpenGL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Add /FS flag directly to compile options to fix PDB file locking
    add_compile_options(/W4 /permissive- /FS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Disable GLFW install" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)

set(GLAD_PROFILE "core" CACHE STRING "GLAD OpenGL profile" FORCE)
set(GLAD_API "gl=4.1" CACHE STRING "GLAD OpenGL version" FORCE)
set(GLAD_GENERATOR "c" CACHE STRING "GLAD generator language" FORCE)
set(GLAD_SPEC "gl" CACHE STRING "GLAD specification" FORCE)
set(GLAD_INSTALL OFF CACHE BOOL "Disable GLAD install target" FORCE)

FetchContent_MakeAvailable(glad)

add_library(glad::glad ALIAS glad)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2
)
FetchContent_MakeAvailable(freetype)

find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    
    # Core modules
    src/core/camera.cpp
    src/core/window.cpp
    
    # Utilities
    src/utils/bounds_utils.cpp
    src/utils/file_utils.cpp
    
    # Rendering
    src/rendering/gltf_loader.cpp
    src/rendering/obj_loader.cpp
    src/rendering/mesh.cpp
    src/rendering/primitives.cpp
    src/rendering/shader.cpp
    src/rendering/texture_loader.cpp
    src/rendering/text_renderer.cpp
    
    # Game modules
    src/game/game_state.cpp
    src/game/game_loop.cpp
    src/game/renderer.cpp
    src/game/map/board.cpp
    src/game/map/map_generator.cpp
    src/game/map/map_manager.cpp
    src/game/minigame/qte_minigame.cpp
    src/game/minigame/tile_memory_minigame.cpp
    src/game/minigame/reaction_minigame.cpp
    src/game/minigame/math_minigame.cpp
    src/game/minigame/pattern_minigame.cpp
    src/game/player/player.cpp
    src/game/player/dice/dice.cpp
)

# /FS flag is already set via add_compile_options above

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(BINARY_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders)
set(DICE_MODEL ${CMAKE_CURRENT_SOURCE_DIR}/dice.glb)
set(BINARY_DICE_MODEL ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/dice.glb)
set(FONT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pixel-game.regular.otf)
set(BINARY_FONT_FILE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/pixel-game.regular.otf)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} ${BINARY_SHADER_DIR}
    COMMENT "Copying shaders"
)

# Copy dice model only if it exists
if(EXISTS ${DICE_MODEL})
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DICE_MODEL} ${BINARY_DICE_MODEL}
        COMMENT "Copying dice model"
    )
    add_dependencies(${PROJECT_NAME} copy_shaders copy_assets)
else()
    message(STATUS "dice.glb not found in source directory - dice visualization will be disabled at runtime")
    add_dependencies(${PROJECT_NAME} copy_shaders)
endif()

if(EXISTS ${FONT_FILE})
    add_custom_target(copy_font ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FONT_FILE} ${BINARY_FONT_FILE}
        COMMENT "Copying UI font"
    )
    add_dependencies(${PROJECT_NAME} copy_font)
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cgltf
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${glad_BINARY_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glad::glad
        glfw
        OpenGL::GL
        glm::glm
        freetype
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

install(TARGETS ${PROJECT_NAME})

