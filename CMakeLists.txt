cmake_minimum_required(VERSION 3.20)

project(PacmanOpenGL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Note: Platform-specific policies will be handled per-dependency as needed

# Platform-specific compiler options
if(MSVC)
    # Add /FS flag directly to compile options to fix PDB file locking
    # Disable unsafe function warnings for third-party libraries (cgltf)
    add_compile_options(/W4 /permissive- /FS /D_CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
    # macOS-specific compiler options
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(UNIX)
    # Linux/Unix compiler options
    add_compile_options(-Wall -Wextra -Wpedantic)
else()
    # Default for other platforms
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

# Workaround for GLAD's old CMake requirements
# GLAD's CMakeLists.txt uses deprecated cmake_minimum_required(VERSION < 3.5) syntax
# We need to set CMAKE_POLICY_VERSION_MINIMUM before FetchContent processes GLAD
# This allows CMake to accept the old syntax from dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version" FORCE)
FetchContent_MakeAvailable(glad)
# Reset to our project's minimum version after GLAD is configured
set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_MINIMUM_REQUIRED_VERSION} CACHE STRING "Minimum CMake policy version" FORCE)

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
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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

# SDL_mixer for audio (supports MP3, OGG, WAV)
find_package(SDL2_mixer QUIET)
if(NOT SDL2_mixer_FOUND)
    # Try to find SDL2_mixer using pkg-config (Linux/macOS)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_MIXER QUIET SDL2_mixer)
        if(SDL2_MIXER_FOUND)
            set(SDL2_mixer_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIRS})
            set(SDL2_mixer_LIBRARIES ${SDL2_MIXER_LIBRARIES})
            set(SDL2_mixer_FOUND TRUE)
        endif()
    endif()
endif()

# If still not found, provide helpful message
if(NOT SDL2_mixer_FOUND)
    message(WARNING "SDL2_mixer not found. Audio features will be disabled.")
    message(STATUS "On macOS: brew install sdl2_mixer")
    message(STATUS "On Linux: sudo apt-get install libsdl2-mixer-dev")
    message(STATUS "On Windows: Download SDL2_mixer development libraries")
    set(ENABLE_AUDIO OFF)
else()
    set(ENABLE_AUDIO ON)
    message(STATUS "SDL2_mixer found")
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    
    # Core modules
    src/core/camera.cpp
    src/core/window.cpp
    src/core/audio_manager.cpp
    
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
    src/rendering/animation_player.cpp
    
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

# Cross-platform path handling for assets
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
# Use CMAKE_CFG_INTDIR for multi-config generators (Visual Studio), CONFIG for single-config
if(CMAKE_CONFIGURATION_TYPES)
    set(BINARY_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders)
    set(BINARY_DICE_MODEL ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/dice.glb)
    set(BINARY_FONT_FILE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/pixel-game.regular.otf)
else()
    set(BINARY_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    set(BINARY_DICE_MODEL ${CMAKE_CURRENT_BINARY_DIR}/dice.glb)
    set(BINARY_FONT_FILE ${CMAKE_CURRENT_BINARY_DIR}/pixel-game.regular.otf)
endif()

set(DICE_MODEL ${CMAKE_CURRENT_SOURCE_DIR}/dice.glb)
set(FONT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/pixel-game.regular.otf)

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

# Add GLM include directory (header-only library)
# GLM is header-only, so we need to add its source directory to includes
# Always add GLM include directory - it's critical for IntelliSense
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/glm-src")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/glm-src
    )
    message(STATUS "GLM include directory: ${CMAKE_BINARY_DIR}/_deps/glm-src")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/glm-src")
    # Fallback for when build directory is different
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/glm-src
    )
    message(STATUS "GLM include directory (fallback): ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/glm-src")
endif()
# Also try to get from target if available
if(TARGET glm::glm)
    get_target_property(GLM_INCLUDE_DIR glm::glm INTERFACE_INCLUDE_DIRECTORIES)
    if(GLM_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})
        message(STATUS "GLM include directory from target: ${GLM_INCLUDE_DIR}")
    endif()
endif()
# Force add GLM include - ensure it's always available
if(TARGET glm::glm-header-only)
    get_target_property(GLM_HEADER_INCLUDE glm::glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
    if(GLM_HEADER_INCLUDE)
        target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_HEADER_INCLUDE})
        message(STATUS "GLM include from header-only target: ${GLM_HEADER_INCLUDE}")
    endif()
endif()

# Add Assimp include directories
# When using FetchContent, assimp_SOURCE_DIR is set automatically
# Always try FetchContent location first (most reliable)
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/assimp-src/include
    )
    message(STATUS "Assimp include directory: ${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
endif()
# Also try assimp_SOURCE_DIR if defined
if(DEFINED assimp_SOURCE_DIR AND EXISTS "${assimp_SOURCE_DIR}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${assimp_SOURCE_DIR}/include
    )
    message(STATUS "Assimp include directory (source): ${assimp_SOURCE_DIR}/include")
endif()
# Try to get from target if available
if(TARGET assimp)
    get_target_property(ASSIMP_INCLUDE_DIR assimp INTERFACE_INCLUDE_DIRECTORIES)
    if(ASSIMP_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${ASSIMP_INCLUDE_DIR})
        message(STATUS "Assimp include directory from target: ${ASSIMP_INCLUDE_DIR}")
    endif()
endif()
# If using pre-installed Assimp
if(assimp_FOUND)
    message(STATUS "Using pre-installed Assimp include directories")
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

# Audio linking and definitions (before platform-specific)
if(ENABLE_AUDIO AND SDL2_mixer_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_mixer_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_AUDIO)
    # SDL2_mixer depends on SDL2 - link both
    # Use pkg-config to get both libraries directly
    find_package(PkgConfig)
    pkg_check_modules(SDL2_MIXER_PKG sdl2_mixer)
    pkg_check_modules(SDL2_PKG sdl2)
    if(SDL2_MIXER_PKG_FOUND AND SDL2_PKG_FOUND)
        if(SDL2_MIXER_PKG_LIBRARY_DIRS)
            target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_PKG_LIBRARY_DIRS})
        endif()
        if(SDL2_PKG_LIBRARY_DIRS)
            target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_PKG_LIBRARY_DIRS})
        endif()
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_PKG_LIBRARIES} ${SDL2_PKG_LIBRARIES})
    else()
        # Fallback: link directly
        if(APPLE)
            target_link_libraries(${PROJECT_NAME} PRIVATE /opt/homebrew/lib/libSDL2_mixer.dylib /opt/homebrew/lib/libSDL2.dylib)
        else()
            target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer SDL2)
        endif()
    endif()
    message(STATUS "Audio enabled: linking SDL2_mixer")
else()
    message(STATUS "Audio disabled: SDL2_mixer not found or ENABLE_AUDIO is OFF")
endif()

# Platform-specific linking and definitions
if(APPLE)
    # macOS requires Cocoa, IOKit, and CoreVideo frameworks for GLFW
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
    # macOS-specific compile definitions if needed
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        GL_SILENCE_DEPRECATION
    )
endif()

if(WIN32)
    # Windows-specific compile definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    # Windows may need additional libraries
    # GLFW will handle most of this, but we can add if needed
endif()

if(UNIX AND NOT APPLE)
    # Linux-specific settings
    # GLFW will handle X11 dependencies automatically
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _GNU_SOURCE
    )
endif()

install(TARGETS ${PROJECT_NAME})

