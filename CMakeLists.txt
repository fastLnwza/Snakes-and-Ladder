cmake_minimum_required(VERSION 3.20)

# Check if vcpkg toolchain is being used
if(CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
    if(DEFINED ENV{VCPKG_ROOT})
        message(STATUS "VCPKG_ROOT environment variable: $ENV{VCPKG_ROOT}")
    endif()
endif()

project(SnakesAndLadder VERSION 0.1.0 LANGUAGES CXX)

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

# Set options before declaring to avoid unnecessary reconfiguration
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs")
set(GLFW_INSTALL OFF CACHE BOOL "Disable GLFW install")

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glfw)

# Set GLAD options before declaring to avoid unnecessary reconfiguration
set(GLAD_PROFILE "core" CACHE STRING "GLAD OpenGL profile")
set(GLAD_API "gl=4.1" CACHE STRING "GLAD OpenGL version")
set(GLAD_GENERATOR "c" CACHE STRING "GLAD generator language")
set(GLAD_SPEC "gl" CACHE STRING "GLAD specification")
set(GLAD_INSTALL OFF CACHE BOOL "Disable GLAD install target")

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
    OVERRIDE_FIND_PACKAGE
)

# Workaround for GLAD's old CMake requirements
# GLAD's CMakeLists.txt uses deprecated cmake_minimum_required(VERSION < 3.5) syntax
# We need to set CMAKE_POLICY_VERSION_MINIMUM before FetchContent processes GLAD
# This allows CMake to accept the old syntax from dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version")
FetchContent_MakeAvailable(glad)
# Reset to our project's minimum version after GLAD is configured
set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_MINIMUM_REQUIRED_VERSION} CACHE STRING "Minimum CMake policy version")

add_library(glad::glad ALIAS glad)

FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glm)

# Disable HarfBuzz to avoid warning (it's optional for freetype)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "Disable HarfBuzz support")

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(freetype)

# Assimp for FBX support
# Try to find pre-installed Assimp first
find_package(assimp QUIET)

if(NOT assimp_FOUND)
    # If not found, use FetchContent with URL download instead of Git
    message(STATUS "Assimp not found, downloading from source...")
    
    # Set Assimp options before declaring to avoid unnecessary reconfiguration
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests")
    set(ASSIMP_INSTALL OFF CACHE BOOL "Disable Assimp install")
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable Assimp tools")
    # On macOS, use system zlib to avoid macro conflicts with macOS SDK
    # On other platforms, build zlib with Assimp
    if(APPLE)
        set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "Use system zlib on macOS")
        # macOS has zlib built-in at /usr/lib/libz.dylib
        find_package(ZLIB REQUIRED)
        # Disable warnings that cause build failures on macOS with newer compilers
        set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Disable warnings as errors")
    else()
        set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "Build zlib with Assimp")
    endif()
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable Assimp export")
    
    FetchContent_Declare(
        assimp
        URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        OVERRIDE_FIND_PACKAGE
    )
    
    # Disable problematic warnings for Assimp on macOS
    if(APPLE)
        # Add compiler flags to disable deprecated-copy warnings
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy")
    endif()
    
    FetchContent_MakeAvailable(assimp)
    
    # After Assimp is configured, disable warnings for its build
    if(APPLE AND TARGET assimp)
        target_compile_options(assimp PRIVATE -Wno-deprecated-copy -Wno-error=deprecated-copy)
    endif()
    
    # After FetchContent, the target is just 'assimp'
    set(ASSIMP_TARGET assimp)
else()
    message(STATUS "Using pre-installed Assimp")
    set(ASSIMP_TARGET assimp::assimp)
endif()

find_package(OpenGL REQUIRED)

# SDL_mixer for audio (supports MP3, OGG, WAV)
# Cache the result to avoid re-searching on every configure
if(NOT SDL2_MIXER_SEARCH_DONE)
    set(SDL2_MIXER_SEARCH_DONE TRUE CACHE BOOL "SDL2_mixer search completed" FORCE)
    
    # Try to find SDL2_mixer - vcpkg toolchain file should handle this on Windows
    find_package(SDL2_mixer QUIET)
    
    # Check if found via find_package but INCLUDE_DIRS not set (Config mode with targets)
    if(SDL2_mixer_FOUND AND (NOT SDL2_mixer_INCLUDE_DIRS OR SDL2_mixer_INCLUDE_DIRS STREQUAL ""))
        if(TARGET SDL2_mixer::SDL2_mixer)
            # Modern CMake Config mode with targets - we'll use the target directly
            set(SDL2_MIXER_USE_TARGET TRUE CACHE BOOL "Use SDL2_mixer target" FORCE)
        else()
            # Found but no target and no include dirs - likely a broken Find module
            # Reset to trigger manual search
            set(SDL2_mixer_FOUND FALSE)
        endif()
    endif()
    
    if(NOT SDL2_mixer_FOUND)
        # Try to find SDL2_mixer using pkg-config (Linux/macOS)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2_MIXER QUIET SDL2_mixer)
            if(SDL2_MIXER_FOUND)
                set(SDL2_mixer_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIRS} CACHE PATH "SDL2_mixer include directories")
                set(SDL2_mixer_LIBRARIES ${SDL2_MIXER_LIBRARIES} CACHE STRING "SDL2_mixer libraries")
                set(SDL2_mixer_FOUND TRUE CACHE BOOL "SDL2_mixer found" FORCE)
            endif()
        endif()
    endif()
    
    # macOS: Try to find SDL2_mixer from Homebrew if not found
    if(APPLE AND NOT SDL2_mixer_FOUND)
        # Common Homebrew paths - check both opt subdirectory and main include
        set(HOMEBREW_PREFIXES "/opt/homebrew" "/usr/local")
        foreach(HOMEBREW_PREFIX ${HOMEBREW_PREFIXES})
            # Check in opt subdirectory first (newer Homebrew structure)
            set(SDL_MIXER_OPT_PATH "${HOMEBREW_PREFIX}/opt/sdl2_mixer/include/SDL2/SDL_mixer.h")
            if(EXISTS "${SDL_MIXER_OPT_PATH}")
                set(SDL2_mixer_INCLUDE_DIRS "${HOMEBREW_PREFIX}/opt/sdl2_mixer/include" CACHE PATH "SDL2_mixer include directories")
                find_library(SDL2_MIXER_LIB "${HOMEBREW_PREFIX}/opt/sdl2_mixer/lib" NAMES SDL2_mixer)
                if(SDL2_MIXER_LIB)
                    set(SDL2_mixer_LIBRARIES ${SDL2_MIXER_LIB} CACHE STRING "SDL2_mixer libraries")
                    # Also find SDL2
                    find_library(SDL2_LIB "${HOMEBREW_PREFIX}/opt/sdl2/lib" NAMES SDL2)
                    if(NOT SDL2_LIB)
                        find_library(SDL2_LIB "${HOMEBREW_PREFIX}/lib" NAMES SDL2)
                    endif()
                    if(SDL2_LIB)
                        list(APPEND SDL2_mixer_LIBRARIES ${SDL2_LIB})
                        set(SDL2_mixer_LIBRARIES ${SDL2_mixer_LIBRARIES} CACHE STRING "SDL2_mixer libraries" FORCE)
                    endif()
                    set(SDL2_mixer_FOUND TRUE CACHE BOOL "SDL2_mixer found" FORCE)
                    break()
                endif()
            # Check in main include directory (older Homebrew structure)
            elseif(EXISTS "${HOMEBREW_PREFIX}/include/SDL2/SDL_mixer.h")
                set(SDL2_mixer_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include" CACHE PATH "SDL2_mixer include directories")
                find_library(SDL2_MIXER_LIB "${HOMEBREW_PREFIX}/lib" NAMES SDL2_mixer)
                if(SDL2_MIXER_LIB)
                    set(SDL2_mixer_LIBRARIES ${SDL2_MIXER_LIB} CACHE STRING "SDL2_mixer libraries")
                    # Also find SDL2
                    find_library(SDL2_LIB "${HOMEBREW_PREFIX}/lib" NAMES SDL2)
                    if(SDL2_LIB)
                        list(APPEND SDL2_mixer_LIBRARIES ${SDL2_LIB})
                        set(SDL2_mixer_LIBRARIES ${SDL2_mixer_LIBRARIES} CACHE STRING "SDL2_mixer libraries" FORCE)
                    endif()
                    set(SDL2_mixer_FOUND TRUE CACHE BOOL "SDL2_mixer found" FORCE)
                    break()
                endif()
            endif()
        endforeach()
    endif()
    
    # Windows: Try to find SDL2_mixer from vcpkg if not found
    if(WIN32 AND NOT SDL2_mixer_FOUND)
        # Try vcpkg paths - check multiple possible locations
        set(VCPKG_ROOT "")
        if(DEFINED ENV{VCPKG_ROOT})
            set(VCPKG_ROOT $ENV{VCPKG_ROOT})
        elseif(EXISTS "C:/vcpkg")
            set(VCPKG_ROOT "C:/vcpkg")
        elseif(EXISTS "D:/vcpkg")
            set(VCPKG_ROOT "D:/vcpkg")
        endif()
        
        if(VCPKG_ROOT)
            # Check for SDL2_mixer in vcpkg installed directory
            set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-windows")
            if(EXISTS "${VCPKG_INSTALLED_DIR}/include/SDL2/SDL_mixer.h")
                set(SDL2_mixer_INCLUDE_DIRS "${VCPKG_INSTALLED_DIR}/include" CACHE PATH "SDL2_mixer include directories")
                
                # Find libraries
                find_library(SDL2_MIXER_LIB 
                    PATHS "${VCPKG_INSTALLED_DIR}/lib"
                    NAMES SDL2_mixer SDL2_mixer.lib
                    NO_DEFAULT_PATH
                )
                if(SDL2_MIXER_LIB)
                    set(SDL2_mixer_LIBRARIES ${SDL2_MIXER_LIB} CACHE STRING "SDL2_mixer libraries")
                    
                    # Also find SDL2
                    find_library(SDL2_LIB 
                        PATHS "${VCPKG_INSTALLED_DIR}/lib"
                        NAMES SDL2 SDL2.lib
                        NO_DEFAULT_PATH
                    )
                    if(SDL2_LIB)
                        list(APPEND SDL2_mixer_LIBRARIES ${SDL2_LIB})
                        set(SDL2_mixer_LIBRARIES ${SDL2_mixer_LIBRARIES} CACHE STRING "SDL2_mixer libraries" FORCE)
                    endif()
                    
                    set(SDL2_mixer_FOUND TRUE CACHE BOOL "SDL2_mixer found" FORCE)
                endif()
            endif()
        endif()
    endif()
endif()

# Use cached values
if(DEFINED SDL2_MIXER_USE_TARGET AND SDL2_MIXER_USE_TARGET)
    set(SDL2_mixer_FOUND TRUE)
endif()

# If still not found, provide helpful message
if(NOT SDL2_mixer_FOUND)
    message(WARNING "SDL2_mixer not found. Audio features will be disabled.")
    message(STATUS "Install: macOS: brew install sdl2_mixer | Linux: sudo apt-get install libsdl2-mixer-dev | Windows: vcpkg install sdl2-mixer")
    set(ENABLE_AUDIO OFF CACHE BOOL "Enable audio support")
else()
    set(ENABLE_AUDIO ON CACHE BOOL "Enable audio support" FORCE)
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
# Use target properties first (fastest, no file system checks)
if(TARGET glm::glm-header-only)
    get_target_property(GLM_INCLUDE_DIR glm::glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
    if(GLM_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})
    endif()
elseif(TARGET glm::glm)
    get_target_property(GLM_INCLUDE_DIR glm::glm INTERFACE_INCLUDE_DIRECTORIES)
    if(GLM_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})
    endif()
elseif(DEFINED glm_SOURCE_DIR)
    # Use FetchContent-provided source directory
    target_include_directories(${PROJECT_NAME} PRIVATE ${glm_SOURCE_DIR})
elseif(EXISTS "${CMAKE_BINARY_DIR}/_deps/glm-src")
    # Fallback: check build directory
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/glm-src)
endif()

# Add Assimp include directories
# Use target properties first (fastest, no file system checks)
if(TARGET assimp)
    get_target_property(ASSIMP_INCLUDE_DIR assimp INTERFACE_INCLUDE_DIRECTORIES)
    if(ASSIMP_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${ASSIMP_INCLUDE_DIR})
    endif()
elseif(DEFINED assimp_SOURCE_DIR)
    # Use FetchContent-provided source directory
    target_include_directories(${PROJECT_NAME} PRIVATE ${assimp_SOURCE_DIR}/include)
elseif(EXISTS "${CMAKE_BINARY_DIR}/_deps/assimp-src/include")
    # Fallback: check build directory
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/assimp-src/include)
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
# CRITICAL: Only enable audio if SDL2_mixer is actually found
if(SDL2_mixer_FOUND)
    set(ENABLE_AUDIO ON)
    message(STATUS "=== Enabling Audio Support ===")
    
    # Handle target-based linking (modern CMake Config mode)
    if(DEFINED SDL2_MIXER_USE_TARGET AND SDL2_MIXER_USE_TARGET AND TARGET SDL2_mixer::SDL2_mixer)
        # Get include directories from target if available
        get_target_property(SDL2_MIXER_INC_DIR SDL2_mixer::SDL2_mixer INTERFACE_INCLUDE_DIRECTORIES)
        if(SDL2_MIXER_INC_DIR)
            target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_INC_DIR})
        endif()
        
        # Find SDL2_mixer library using full path (more reliable than target on macOS)
        find_library(SDL2_MIXER_LIB 
            NAMES SDL2_mixer 
            PATHS /opt/homebrew/lib /usr/local/lib
            NO_DEFAULT_PATH
        )
        find_library(SDL2_LIB 
            NAMES SDL2 
            PATHS /opt/homebrew/lib /usr/local/lib
            NO_DEFAULT_PATH
        )
        
        if(SDL2_MIXER_LIB AND SDL2_LIB)
            # Use full library paths - most reliable on macOS with Homebrew
            target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIB} ${SDL2_LIB})
        elseif(SDL2_MIXER_LIB)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIB} SDL2)
        else()
            # Fallback: try target and add link directories
            if(APPLE)
                foreach(HOMEBREW_PREFIX "/opt/homebrew" "/usr/local")
                    if(EXISTS "${HOMEBREW_PREFIX}/lib")
                        target_link_directories(${PROJECT_NAME} PRIVATE "${HOMEBREW_PREFIX}/lib")
                        break()
                    endif()
                endforeach()
            endif()
            target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer::SDL2_mixer)
            if(SDL2_LIB)
                target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIB})
            elseif(TARGET SDL2::SDL2)
                target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
            else()
                target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
            endif()
        endif()
    elseif(SDL2_mixer_INCLUDE_DIRS)
        # Use include directories and libraries
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_mixer_INCLUDE_DIRS})
        
        # SDL2_mixer depends on SDL2 - link both
        # Use pkg-config to get both libraries directly
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2_MIXER_PKG QUIET sdl2_mixer)
            pkg_check_modules(SDL2_PKG QUIET sdl2)
            if(SDL2_MIXER_PKG_FOUND AND SDL2_PKG_FOUND)
                if(SDL2_MIXER_PKG_LIBRARY_DIRS)
                    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_PKG_LIBRARY_DIRS})
                endif()
                if(SDL2_PKG_LIBRARY_DIRS)
                    target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_PKG_LIBRARY_DIRS})
                endif()
                target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_PKG_LIBRARIES} ${SDL2_PKG_LIBRARIES})
            else()
                # Fallback: find libraries using full paths (most reliable)
                find_library(SDL2_MIXER_LIB_FULL 
                    NAMES SDL2_mixer 
                    PATHS /opt/homebrew/lib /usr/local/lib
                    NO_DEFAULT_PATH
                )
                find_library(SDL2_LIB_FULL 
                    NAMES SDL2 
                    PATHS /opt/homebrew/lib /usr/local/lib
                    NO_DEFAULT_PATH
                )
                if(SDL2_MIXER_LIB_FULL AND SDL2_LIB_FULL)
                    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIB_FULL} ${SDL2_LIB_FULL})
                elseif(SDL2_mixer_LIBRARIES)
                    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_mixer_LIBRARIES})
                    if(NOT "${SDL2_mixer_LIBRARIES}" MATCHES "SDL2")
                        if(SDL2_LIB_FULL)
                            target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIB_FULL})
                        else()
                            target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
                        endif()
                    endif()
                else()
                    # Last resort: try common paths with link directories
                    if(APPLE)
                        foreach(HOMEBREW_PREFIX "/opt/homebrew" "/usr/local")
                            if(EXISTS "${HOMEBREW_PREFIX}/lib")
                                target_link_directories(${PROJECT_NAME} PRIVATE "${HOMEBREW_PREFIX}/lib")
                                break()
                            endif()
                        endforeach()
                    endif()
                    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer SDL2)
                endif()
            endif()
        else()
            # Fallback: find libraries using full paths (most reliable)
            find_library(SDL2_MIXER_LIB_FULL 
                NAMES SDL2_mixer 
                PATHS /opt/homebrew/lib /usr/local/lib
                NO_DEFAULT_PATH
            )
            find_library(SDL2_LIB_FULL 
                NAMES SDL2 
                PATHS /opt/homebrew/lib /usr/local/lib
                NO_DEFAULT_PATH
            )
            if(SDL2_MIXER_LIB_FULL AND SDL2_LIB_FULL)
                target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIB_FULL} ${SDL2_LIB_FULL})
            elseif(SDL2_mixer_LIBRARIES)
                target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_mixer_LIBRARIES})
                # Ensure SDL2 is always linked - check if SDL2 is already in the list
                if(NOT "${SDL2_mixer_LIBRARIES}" MATCHES "SDL2")
                    if(SDL2_LIB_FULL)
                        target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIB_FULL})
                    else()
                        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
                    endif()
                endif()
            else()
                # Last resort: try common paths with link directories
                if(APPLE)
                    foreach(HOMEBREW_PREFIX "/opt/homebrew" "/usr/local")
                        if(EXISTS "${HOMEBREW_PREFIX}/lib")
                            target_link_directories(${PROJECT_NAME} PRIVATE "${HOMEBREW_PREFIX}/lib")
                            break()
                        endif()
                    endforeach()
                endif()
                target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer SDL2)
            endif()
        endif()
    else()
        # Should not happen, but handle gracefully
        message(WARNING "SDL2_mixer found but no include dirs or target available - trying fallback link")
        # Try to find full paths first
        find_library(SDL2_MIXER_LIB_FULL 
            NAMES SDL2_mixer 
            PATHS /opt/homebrew/lib /usr/local/lib
            NO_DEFAULT_PATH
        )
        find_library(SDL2_LIB_FULL 
            NAMES SDL2 
            PATHS /opt/homebrew/lib /usr/local/lib
            NO_DEFAULT_PATH
        )
        if(SDL2_MIXER_LIB_FULL AND SDL2_LIB_FULL)
            # Use full paths - most reliable
            target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIB_FULL} ${SDL2_LIB_FULL})
        elseif(TARGET SDL2_mixer::SDL2_mixer)
            target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer::SDL2_mixer)
            if(SDL2_LIB_FULL)
                target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIB_FULL})
            else()
                target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
            endif()
        else()
            # Last resort: use library names with link directories
            if(APPLE)
                foreach(HOMEBREW_PREFIX "/opt/homebrew" "/usr/local")
                    if(EXISTS "${HOMEBREW_PREFIX}/lib")
                        target_link_directories(${PROJECT_NAME} PRIVATE "${HOMEBREW_PREFIX}/lib")
                        break()
                    endif()
                endforeach()
            endif()
            target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer SDL2)
        endif()
    endif()
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_AUDIO)
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

