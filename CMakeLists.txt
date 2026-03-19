cmake_minimum_required(VERSION 3.24)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)  # Allow older CMakeLists in FetchContent deps
project(xr_ar_space VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC settings
if(MSVC)
    add_compile_options(/W4 /utf-8 /Zc:__cplusplus /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

# ── Dependencies via FetchContent ──────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(hidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi.git
    GIT_TAG        hidapi-0.14.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glfw glm hidapi)

# ── Vendored GLAD ─────────────────────────────────────────────────────────────
add_library(glad STATIC
    extern/glad/src/gl.c
)
target_include_directories(glad PUBLIC extern/glad/include)

# ── stb (header-only) ─────────────────────────────────────────────────────────
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE extern/stb)

# ── Main executable ───────────────────────────────────────────────────────────
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    glm::glm
    stb
    opengl32
    # Windows system libs for future phases
    dwmapi
    d3d11
    dxgi
    hidapi::winapi
    setupapi
)

# Phase 4+ will need:
# windowsapp  (C++/WinRT)

# Copy shaders next to executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copying shaders to output directory"
)
