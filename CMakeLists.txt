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

# Suppress CMake deprecation warnings from external dependencies
# These warnings are from third-party libraries (freetype, glad) and don't affect functionality
# They use older cmake_minimum_required syntax which is deprecated in newer CMake versions
set(CMAKE_WARN_DEPRECATED OFF)

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

# Disable HarfBuzz to avoid warning (it's optional for freetype)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "Disable HarfBuzz support" FORCE)

FetchContent_MakeAvailable(freetype)

# Assimp for FBX support
# Try to find pre-installed Assimp first
find_package(assimp QUIET)

if(NOT assimp_FOUND)
    # If not found, use FetchContent with URL download instead of Git
    message(STATUS "Assimp not found, downloading from source...")
    
    FetchContent_Declare(
        assimp
        URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.2.zip
    )
    
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "Disable Assimp install" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable Assimp tools" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "Build zlib with Assimp" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable Assimp export" FORCE)
    
    FetchContent_MakeAvailable(assimp)
    
    # After FetchContent, the target is just 'assimp'
    set(ASSIMP_TARGET assimp)
else()
    message(STATUS "Using pre-installed Assimp")
    set(ASSIMP_TARGET assimp::assimp)
endif()

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
    src/rendering/fbx_loader.cpp
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
    src/game/menu/menu_renderer.cpp
    src/game/win/win_renderer.cpp
    src/game/minigame/minigame_menu_renderer.cpp
)

# /FS flag is already set via add_compile_options above

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(BINARY_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders)
set(DICE_MODEL ${CMAKE_CURRENT_SOURCE_DIR}/dice.glb)
set(BINARY_DICE_MODEL ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/dice.glb)
set(FONT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/pixel-game.regular.otf)
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

# Add Assimp include directories
# When using FetchContent, assimp_SOURCE_DIR is set automatically
if(DEFINED assimp_SOURCE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${assimp_SOURCE_DIR}/include
    )
    message(STATUS "Assimp include directory: ${assimp_SOURCE_DIR}/include")
elseif(TARGET assimp)
    # If assimp target exists, try to get include dirs from target
    get_target_property(ASSIMP_INCLUDE_DIR assimp INTERFACE_INCLUDE_DIRECTORIES)
    if(ASSIMP_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${ASSIMP_INCLUDE_DIR})
        message(STATUS "Assimp include directory from target: ${ASSIMP_INCLUDE_DIR}")
    endif()
    # Also try FetchContent location
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
        target_include_directories(${PROJECT_NAME} PRIVATE
            ${CMAKE_BINARY_DIR}/_deps/assimp-src/include
        )
        message(STATUS "Assimp include directory (FetchContent): ${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
    endif()
elseif(assimp_FOUND)
    message(STATUS "Using pre-installed Assimp include directories")
else()
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
        target_include_directories(${PROJECT_NAME} PRIVATE
            ${CMAKE_BINARY_DIR}/_deps/assimp-src/include
        )
        message(STATUS "Assimp include directory (fallback): ${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
    endif()
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glad::glad
        glfw
        OpenGL::GL
        glm::glm
        freetype
        ${ASSIMP_TARGET}
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

