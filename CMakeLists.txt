cmake_minimum_required(VERSION 3.20)

project(PacmanOpenGL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
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

find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(BINARY_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} ${BINARY_SHADER_DIR}
    COMMENT "Copying runtime assets"
)

add_dependencies(${PROJECT_NAME} copy_shaders)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cgltf
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glad::glad
        glfw
        OpenGL::GL
        glm::glm
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

