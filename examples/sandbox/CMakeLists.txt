# =============================================================================
# examples/sandbox - a Yume project (application) built on top of the engine.
#
# This is the "app" half of the engine/app split: it owns an entry point
# (src/main.cpp) and its runtime assets (scene.scn, shaders, meshes, fonts,
# project.conf), and links the reusable engine library `yume_core`.
#
# Two build paths are supported:
#   1. Standalone: configure THIS folder directly. We pull the engine in via
#      add_subdirectory() so `yume_core` and yume_add_project() exist.
#   2. From the engine root: the root CMakeLists already created `yume_core` and
#      included YumeProject.cmake, then add_subdirectory()'d us. We detect that
#      and skip re-adding the engine.
# =============================================================================
cmake_minimum_required(VERSION 3.10)
project(yume_sandbox LANGUAGES C CXX)

# Engine root is two levels up (examples/sandbox -> repo root).
get_filename_component(YUME_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# When configured standalone, bring in the engine. When included from the root,
# `yume_core` is already defined so we only need YumeProject.cmake's helpers
# (guarded by include_guard, so re-including is a no-op).
if(NOT TARGET yume_core)
    add_subdirectory(${YUME_ENGINE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/yume_engine)
endif()
include(${YUME_ENGINE_ROOT}/cmake/YumeProject.cmake)

# Create the executable, link yume_core, and wire the asset pipeline (shaders,
# scenes, and verbatim assets land next to the exe). PROJECT_DIR is this folder
# because the assets live here.
yume_add_project(sandbox
    PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
