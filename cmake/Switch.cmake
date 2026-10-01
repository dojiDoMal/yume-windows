# =============================================================================
# Switch.cmake - Nintendo Switch (devkitPro / libnx) platform configuration
#
# Included from the root CMakeLists.txt when building with the devkitPro
# toolchain (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch").
#
# Owns the Switch runtime graphics dependencies and the link line for the
# libnx/EGL/OpenGL backend, plus the shader flavor (GLSL ES 3.20 -> .nxs, set
# in the root script). The GL loader on the Switch is `glad` (a devkitPro
# portlib), NOT GLEW -- GLEW is a desktop-only dependency handled in PC.cmake.
#
#     cmake --build build/switch --target Shaders   # shaders only
#     cmake --build build/switch                     # full app (links yume_core)
# =============================================================================

# NOTE: The Switch platform is identified by `__SWITCH__`, which the libnx
# toolchain already defines (`-D__SWITCH__` in its CFLAGS), so there is no need
# to add it here manually.
#
# MAX_WORLD_OBJECTS is a global define in the root CMakeLists.txt so it is
# shared by both the engine/app (yume_core) and scene_compiler (the .scnb
# layout must match).

# -----------------------------------------------------------------------------
# Dependency search roots.
#
# The reference libnx Makefile uses `LIBDIRS := $(PORTLIBS) $(LIBNX)`, i.e. it
# pulls headers from <root>/include and libs from <root>/lib for both the Switch
# portlibs tree and libnx itself. The devkitPro CMake toolchain already adds
# libnx to the search paths, but glad/EGL/glapi/drm_nouveau live in portlibs, so
# make sure that root is on the include/link search path too.
# -----------------------------------------------------------------------------
if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "DEVKITPRO is not set. export DEVKITPRO=<path to>/devkitpro")
endif()

set(SWITCH_PORTLIBS "$ENV{DEVKITPRO}/portlibs/switch")

target_include_directories(yume_core PUBLIC "${SWITCH_PORTLIBS}/include")
target_link_directories(yume_core PUBLIC "${SWITCH_PORTLIBS}/lib")

# -----------------------------------------------------------------------------
# SDL2 (devkitPro portlib). The engine drives SDL itself (SDL_SetMainReady +
# SDL_Init in Application::boot) and does not use SDL's own main shim, so define
# SDL_MAIN_HANDLED here just like PC.cmake. SDL2's own compile/link flags on the
# Switch (it depends on a chain of system libs) come from pkg-config, which the
# reference Makefile also uses (`pkg-config --cflags/--libs sdl2`).
# -----------------------------------------------------------------------------
target_compile_definitions(yume_core PUBLIC SDL_MAIN_HANDLED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# -----------------------------------------------------------------------------
# Link line.
#
# ORDER MATTERS: the GNU linker resolves symbols left-to-right. SDL2 is highest
# level and comes first; glad (which consumes the EGL/GL entry points) must come
# before EGL/glapi/drm_nouveau; nx / m go last as the lowest-level providers.
# Mirrors the reference Makefile's
#     LIBS := `pkg-config --libs sdl2` -lglad -lEGL -lglapi -ldrm_nouveau -lnx -lm
# -----------------------------------------------------------------------------
target_link_libraries(yume_core PUBLIC
    PkgConfig::SDL2
    glad
    EGL
    glapi
    drm_nouveau
    nx
    m
)
