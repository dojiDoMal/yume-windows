# =============================================================================
# Web.cmake - Emscripten / WebGL platform configuration
#
# Included from the root CMakeLists.txt when EMSCRIPTEN is set. Attaches the
# engine-level WebGL bits (includes, PLATFORM_WEBGL define, exception flag) to
# the engine library `yume_core`.
#
# STATUS: the Web target is on hiatus and has NOT been re-validated against the
# per-project (Caminho 1) layout. The asset preloading + Emscripten link flags
# are inherently per-executable (not per-library), so they now belong in the
# project's CMakeLists via yume_web_configure() below, which the example must
# call for its target. Revisit when WebGL comes off hiatus.
# =============================================================================

target_include_directories(yume_core PUBLIC ${YUME_ENGINE_ROOT}/core/libs)
target_compile_definitions(yume_core PUBLIC PLATFORM_WEBGL)
# NOTE: MAX_WORLD_OBJECTS is a global define in the root CMakeLists.txt so it is
# shared by both yume_core and scene_compiler (the .scnb layout must match).
# NOTE: CMAKE_EXECUTABLE_SUFFIX (".html") is set in the root CMakeLists.txt
# before add_executable, since the suffix is consumed when the target is created.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_DISABLE_EXCEPTION_CATCHING")

# -----------------------------------------------------------------------------
# yume_web_configure(<target> ASSET_DIR <dir>)
#
# Preloads the compiled runtime assets found in ASSET_DIR (where the project's
# asset pipeline wrote them) into the Emscripten VFS and applies the WebGL
# linker flags to <target>. Call this from the project's CMakeLists after
# yume_add_project(), passing the same output directory.
# -----------------------------------------------------------------------------
function(yume_web_configure TARGET)
    cmake_parse_arguments(ARG "" "ASSET_DIR" "" ${ARGN})

    set(_preload)
    file(GLOB _assets
        "${ARG_ASSET_DIR}/*.scnb"
        "${ARG_ASSET_DIR}/*.glsl"
        "${ARG_ASSET_DIR}/*.obj"
        "${ARG_ASSET_DIR}/*.png")
    foreach(F ${_assets})
        get_filename_component(FN ${F} NAME)
        list(APPEND _preload "--preload-file ${F}@${FN}")
    endforeach()
    string(REPLACE ";" " " _preload_str "${_preload}")

    set_target_properties(${TARGET} PROPERTIES
        LINK_FLAGS
        "-s USE_SDL=2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 ${_preload_str}")
endfunction()
