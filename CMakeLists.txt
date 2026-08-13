cmake_minimum_required(VERSION 3.15)
project(V_Project VERSION 1.0.0 LANGUAGES CXX C)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ============================================
# AUTO-COLLECT ALL SOURCE FILES
# ============================================
file(GLOB_RECURSE CPP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE GLAD_SOURCES
    "${CMAKE_SOURCE_DIR}/include/glad/src/*.c"
)

list(APPEND SOURCES ${CPP_SOURCES} ${GLAD_SOURCES})

# ============================================
# FIND REQUIRED PACKAGES
# ============================================
find_package(OpenGL REQUIRED)

# FetchContent Module for downloading external dependencies
include(FetchContent)

# ============================================
# GLFW - Auto-download and build via FetchContent
# ============================================
message(STATUS "Fetching GLFW...")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

set(GLFW_LIBRARY glfw)
set(GLFW_INCLUDE_DIR ${glfw_SOURCE_DIR}/include)

# ============================================
# GLM - Auto-download if missing
# ============================================
set(GLM_URL "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip")
set(GLM_PATH "${CMAKE_SOURCE_DIR}/include/glm/glm.hpp")

if(NOT EXISTS ${GLM_PATH})
    message(STATUS "Downloading GLM...")
    FetchContent_Declare(
        glm
        URL ${GLM_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(glm)

    add_custom_command(TARGET V_Project PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${glm_SOURCE_DIR}/glm
            ${CMAKE_SOURCE_DIR}/include/glm
        COMMENT "Copying GLM to include directory"
    )
endif()

# ============================================
# STB_IMAGE - Auto-download if missing
# ============================================
set(STB_IMAGE_URL "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h")
set(STB_IMAGE_PATH "${CMAKE_SOURCE_DIR}/include/stb_image.h")

if(NOT EXISTS ${STB_IMAGE_PATH})
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD ${STB_IMAGE_URL} ${STB_IMAGE_PATH}
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(WARNING "Failed to download stb_image.h.")
    else()
        message(STATUS "stb_image.h downloaded successfully!")
    endif()
endif()

# ============================================
# ASSIMP - Auto-download and build via FetchContent
# ============================================
message(STATUS "Fetching Assimp (this may take a few minutes on first build)...")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.2.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(assimp)

# ============================================
# DEAR IMGUI - Auto-download & Static Target Setup
# ============================================
message(STATUS "Fetching Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# Compile ImGui and GLFW/OpenGL3 backends into a target static library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${GLFW_INCLUDE_DIR}
)

target_link_libraries(imgui PRIVATE ${GLFW_LIBRARY})

# ============================================
# CREATE EXECUTABLE
# ============================================
add_executable(V_Project ${SOURCES})

# Force C language property for GLAD sources
foreach(SOURCE_FILE ${GLAD_SOURCES})
    set_source_files_properties(${SOURCE_FILE} PROPERTIES LANGUAGE C)
endforeach()

# ============================================
# INCLUDE DIRECTORIES
# ============================================
target_include_directories(V_Project PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/glad/include
    ${GLFW_INCLUDE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# ============================================
# COMPILER OPTIONS AND DEFINES
# ============================================
target_compile_options(V_Project PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(MSVC)
    set(CMAKE_LINKER "link.exe")
endif()

target_compile_definitions(V_Project PRIVATE
    GLAD_STATIC_API
    $<$<CONFIG:Debug>:DEBUG>
)

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(V_Project PRIVATE
    OpenGL::GL
    ${GLFW_LIBRARY}
    assimp::assimp
    imgui
)

# ============================================
# PLATFORM-SPECIFIC TARGETS
# ============================================
if(WIN32)
    target_compile_definitions(V_Project PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(V_Project PRIVATE opengl32.lib)
elseif(APPLE)
    target_link_libraries(V_Project PRIVATE "-framework Cocoa" "-framework IOKit")
elseif(UNIX)
    target_link_libraries(V_Project PRIVATE X11)
endif()

# ============================================
# COPY ASSETS, SHADERS, AND COMPILE_COMMANDS
# ============================================
add_custom_command(TARGET V_Project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory"
)

add_custom_command(TARGET V_Project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Copying shaders to build directory"
)

# Safe copy of compilation database for Clangd / IDEs
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to source root"
)

# ============================================
# PRINT SUMMARY
# ============================================
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "GLFW library: ${GLFW_LIBRARY}")
message(STATUS "GLFW include: ${GLFW_INCLUDE_DIR}")
message(STATUS "Assimp: FETCHED and BUILT (STATIC)")
message(STATUS "Dear ImGui: FETCHED and BUILT (STATIC)")
message(STATUS "========================================")
