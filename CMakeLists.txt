cmake_minimum_required(VERSION 3.15)
project(V_Project VERSION 1.0.0 LANGUAGES CXX C)

# ============================================
# C++ STANDARD & OPTIONS
# ============================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(FORCE_FETCH_DEPS "Force downloading dependencies even if installed on system" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================
# AUTO-COLLECT SOURCE FILES
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
# FIND OPENGL & FETCHCONTENT SETUP
# ============================================
find_package(OpenGL REQUIRED)
include(FetchContent)

# ============================================
# GLFW (System Find with FetchContent Fallback)
# ============================================
if(NOT FORCE_FETCH_DEPS)
    find_package(glfw3 3.3 QUIET)
endif()

if(glfw3_FOUND)
    message(STATUS "GLFW: Using System Package")
    set(GLFW_LIBRARY glfw)
else()
    message(STATUS "GLFW: Fetching from source...")
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
endif()

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
set(STB_IMAGE_PATH "${CMAKE_SOURCE_DIR}/include/stb_image.h")
if(NOT EXISTS ${STB_IMAGE_PATH})
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" ${STB_IMAGE_PATH}
        SHOW_PROGRESS
    )
endif()

# ============================================
# ASSIMP (System Find with FetchContent Fallback)
# ============================================
if(NOT FORCE_FETCH_DEPS)
    find_package(assimp 5.0 QUIET)
endif()

if(assimp_FOUND)
    message(STATUS "Assimp: Using System Package")
    set(ASSIMP_TARGET assimp::assimp)
else()
    message(STATUS "Assimp: Fetching and building static target...")

    # Fix 1: Build Assimp's bundled zlib to avoid unzip.h missing errors
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

    # Fix 2: Disable broken MMD importer to save compilation time
    set(ASSIMP_BUILD_MMD_IMPORTER OFF CACHE BOOL "" FORCE)

    # Fix 3: Upgraded tag from v5.2.5 to stable v5.3.1
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.3.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(assimp)
    set(ASSIMP_TARGET assimp::assimp)
endif()

# ============================================
# DEAR IMGUI - FetchContent & Target Setup
# ============================================
message(STATUS "Fetching Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

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
# EXECUTABLE TARGET
# ============================================
add_executable(V_Project ${SOURCES})

foreach(SOURCE_FILE ${GLAD_SOURCES})
    set_source_files_properties(${SOURCE_FILE} PROPERTIES LANGUAGE C)
endforeach()

# ============================================
# INCLUDE DIRECTORIES & COMPILE OPTIONS
# ============================================
target_include_directories(V_Project PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/glad/include
    ${GLFW_INCLUDE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_compile_options(V_Project PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wno-nontrivial-memcall>
)

target_compile_definitions(V_Project PRIVATE
    GLAD_STATIC_API
    $<$<CONFIG:Debug>:DEBUG>
)

# ============================================
# LINK LIBRARIES & PLATFORM TARGETS
# ============================================
target_link_libraries(V_Project PRIVATE
    OpenGL::GL
    ${GLFW_LIBRARY}
    ${ASSIMP_TARGET}
    imgui
)

if(WIN32)
    target_compile_definitions(V_Project PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(V_Project PRIVATE opengl32.lib ws2_32.lib)
elseif(APPLE)
    target_link_libraries(V_Project PRIVATE "-framework Cocoa" "-framework IOKit")
elseif(UNIX)
    target_link_libraries(V_Project PRIVATE X11)
endif()

# ============================================
# POST-BUILD ASSET MANAGEMENT
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

add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to source root"
)
