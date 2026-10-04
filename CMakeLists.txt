cmake_minimum_required(VERSION 3.10)
project(yume LANGUAGES C CXX)

# This CMakeLists can be the top-level project (standalone engine build) OR be
# pulled in via add_subdirectory() from a project/example. Use
# CMAKE_CURRENT_SOURCE_DIR (this folder) rather than CMAKE_SOURCE_DIR (the
# top-level), so engine paths resolve correctly in both cases.
set(YUME_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# The devkitPro Switch toolchain sets CMAKE_SYSTEM_NAME to "NintendoSwitch".
# Use it as the single source of truth for the Switch build path.
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(IS_SWITCH TRUE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to a Debug build when the caller doesn't specify one, so standalone
# engine configures still behave like before. Projects pass -DCMAKE_BUILD_TYPE.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# -----------------------------------------------------------------------------
# YUME_DEBUG: compile-time switch for debug-only code (verbose logging, asserts).
# Defined for Debug builds and absent for Release, so code can gate on
# #ifdef YUME_DEBUG. Applied as a PUBLIC define on yume_core below so it reaches
# both the engine and anything that links it (the project executables).
# -----------------------------------------------------------------------------

# MAX_WORLD_OBJECTS sizes the CompiledScene struct (WorldObjectData[N]). The
# scene_compiler writes sizeof(CompiledScene) bytes and the engine reads exactly
# that many back, so BOTH binaries must be built with the SAME value. Keep it a
# global define so it reaches yume_core AND scene_compiler.
if(EMSCRIPTEN)
    add_compile_definitions(MAX_WORLD_OBJECTS=64)
else()
    add_compile_definitions(MAX_WORLD_OBJECTS=10240)
endif()

# -----------------------------------------------------------------------------
# Reusable build machinery (shader/scene/asset pipeline + yume_add_project).
# Defines DXC_EXECUTABLE / SPIRV_CROSS_EXECUTABLE and the per-platform shader
# flavor. Included here so the engine and the examples share one source of truth.
# -----------------------------------------------------------------------------
include(${YUME_ENGINE_ROOT}/cmake/YumeProject.cmake)

# -----------------------------------------------------------------------------
# Scene compiler (host tool: .scn -> .scnb). For cross builds (Emscripten,
# Switch) it must run on the build machine, so reuse a prebuilt binary from
# ${YUME_ENGINE_ROOT}/tools; for native builds compile it here. The resulting
# command/deps are published for yume_compile_scenes() via YUME_SCENE_COMPILER_*.
# -----------------------------------------------------------------------------
if(EMSCRIPTEN OR IS_SWITCH)
    find_program(SCENE_COMPILER_EXE scene_compiler PATHS ${YUME_ENGINE_ROOT}/tools REQUIRED)
    set(YUME_SCENE_COMPILER_CMD ${SCENE_COMPILER_EXE} CACHE STRING "scene_compiler command" FORCE)
    set(YUME_SCENE_COMPILER_DEPS "" CACHE STRING "scene_compiler deps" FORCE)
else()
    add_executable(scene_compiler ${YUME_ENGINE_ROOT}/core/src/scene/scene_compiler.cpp)
    # scene_compiler is a standalone host tool (it does not link yume_core), so
    # it needs the engine source dir on its include path to find bundled headers
    # like <nlohmann/json.hpp> and "stb_image.h".
    target_include_directories(scene_compiler PRIVATE ${YUME_ENGINE_ROOT}/core/src)
    set_target_properties(scene_compiler PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${YUME_ENGINE_ROOT}/tools)
    set(YUME_SCENE_COMPILER_CMD scene_compiler CACHE STRING "scene_compiler command" FORCE)
    set(YUME_SCENE_COMPILER_DEPS scene_compiler CACHE STRING "scene_compiler deps" FORCE)
endif()

# -----------------------------------------------------------------------------
# yume_core: the engine as a static library.
#
# Everything under core/src except scene_compiler.cpp, which is a separate
# host-tool executable (built above). The engine no longer contains an entry
# point: each project ships its own main.cpp (see ../yume-examples/rotating-cube)
# and links yume_core.
#
# Backends are filtered per platform exactly as before (one backend dir per API;
# keep only what this build can use).
# -----------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES "${YUME_ENGINE_ROOT}/core/src/*.cpp")
list(REMOVE_ITEM SOURCES "${YUME_ENGINE_ROOT}/core/src/scene/scene_compiler.cpp")

set(BACKENDS_DIR "${YUME_ENGINE_ROOT}/core/src/renderer/backends")
if(EMSCRIPTEN)
    foreach(BACKEND opengl vulkan directx12)
        file(GLOB _files "${BACKENDS_DIR}/${BACKEND}/*")
        list(REMOVE_ITEM SOURCES ${_files})
    endforeach()
elseif(IS_SWITCH)
    # Switch uses the unified opengl backend, which selects the glad loader and
    # OpenGL ES bits via `#ifdef __SWITCH__`. Only webgl/vulkan/directx12 are
    # dropped here; the opengl backend is kept.
    foreach(BACKEND webgl vulkan directx12)
        file(GLOB _files "${BACKENDS_DIR}/${BACKEND}/*")
        list(REMOVE_ITEM SOURCES ${_files})
    endforeach()
else()
    foreach(BACKEND webgl)
        file(GLOB _files "${BACKENDS_DIR}/${BACKEND}/*")
        list(REMOVE_ITEM SOURCES ${_files})
    endforeach()
endif()

add_library(yume_core STATIC ${SOURCES})

# Consumers (project executables) need the engine headers and the same
# MAX_WORLD_OBJECTS. Expose include dir publicly; YUME_DEBUG is public so project
# code can gate on it too.
target_include_directories(yume_core PUBLIC ${YUME_ENGINE_ROOT}/core/src)
target_compile_definitions(yume_core PUBLIC $<$<CONFIG:Debug>:YUME_DEBUG>)

# -----------------------------------------------------------------------------
# Platform-specific dependencies and link line for yume_core. Each file owns one
# platform and attaches its graphics deps (SDL2/GLEW/Vulkan/d3d12, libnx, etc.)
# to yume_core as PUBLIC so the final executable inherits them.
# -----------------------------------------------------------------------------
if(EMSCRIPTEN)
    include(${YUME_ENGINE_ROOT}/cmake/Web.cmake)
elseif(IS_SWITCH)
    include(${YUME_ENGINE_ROOT}/cmake/Switch.cmake)
else()
    include(${YUME_ENGINE_ROOT}/cmake/PC.cmake)
endif()

# -----------------------------------------------------------------------------
# No default application.
#
# A standalone engine build (this CMakeLists as the top-level project, e.g.
# build.bat) produces only the engine library `yume_core` and the host tools
# (scene_compiler) -- there is no built-in executable. The engine is a library;
# a runnable app is always owned by a consuming project that links yume_core and
# calls yume_add_project() (see ../yume-examples/rotating-cube for a complete
# example). The eventual in-repo executable is the editor, not a sample app.
# -----------------------------------------------------------------------------
