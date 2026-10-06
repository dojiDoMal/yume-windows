# =============================================================================
# YumeProject.cmake - reusable build machinery for Yume projects/examples.
#
# A project (e.g. examples/rotating-cube) builds its executable by linking the
# engine library `yume_core` and turning its source assets into the runtime
# artifacts the engine loads at startup:
#
#   *.vxs / *.pxs  (HLSL)  -> SPIR-V -> GLSL/.nxs  (+ .cso DXIL on Windows)
#   *.scn          (JSON)  -> *.scnb (binary scene)
#   *.obj, *.ys, project.conf -> copied verbatim
#
# All artifacts land next to the executable in the project's build/<cfg> dir,
# which the engine uses as its working directory, so every bare relative path
# in a .scn / project.conf resolves correctly without an asset-root concept.
#
# This file is meant to be included from BOTH the engine root CMakeLists.txt and
# each project's CMakeLists.txt. It only defines variables/functions; it does
# not create targets by itself.
#
# Public entry point:
#   yume_add_project(<target>
#       PROJECT_DIR <dir>          # folder holding the source assets
#       [SOURCES <files...>]       # project .cpp (e.g. src/main.cpp)
#   )
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Host shader tools (dxc / spirv-cross). These run on the BUILD machine and
# turn HLSL into GLSL/SPIR-V text; they are never built for or run on target.
#
# On the Switch, devkitPro redirects find_program() into the cross sysroot where
# these don't exist, so take their paths from cache vars HOST_DXC /
# HOST_SPIRV_CROSS (settable via -D... on the configure line), falling back to
# the matching environment variables.
# -----------------------------------------------------------------------------
# CACHE (not a plain set) for the same reason the shader-flavor vars below are
# cached: this file has include_guard(GLOBAL). When a project pulls the engine
# in via add_subdirectory() and THEN include()s this file again, the second
# include is a no-op, so a plain set() here would only live in the engine's
# subdirectory scope -- leaving YUME_IS_SWITCH UNDEFINED in the project scope
# where yume_add_project() actually runs. That made the Switch-only branch
# (romfs staging + the main_nro / elf2nro packing target) silently not fire, so
# `cmake --build ... --target main_nro` failed with "No rule to make target".
# Caching makes the value survive across scopes and the include guard.
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(YUME_IS_SWITCH TRUE CACHE BOOL "Building for Nintendo Switch" FORCE)
else()
    set(YUME_IS_SWITCH FALSE CACHE BOOL "Building for Nintendo Switch" FORCE)
endif()

if(YUME_IS_SWITCH)
    if(NOT HOST_DXC AND DEFINED ENV{HOST_DXC})
        set(HOST_DXC "$ENV{HOST_DXC}")
    endif()
    if(NOT HOST_SPIRV_CROSS AND DEFINED ENV{HOST_SPIRV_CROSS})
        set(HOST_SPIRV_CROSS "$ENV{HOST_SPIRV_CROSS}")
    endif()
    if(NOT HOST_DXC OR NOT HOST_SPIRV_CROSS)
        message(FATAL_ERROR
            "Switch build needs host shader tools. Pass -DHOST_DXC=<path to dxc.exe> "
            "and -DHOST_SPIRV_CROSS=<path to spirv-cross.exe> (or set the HOST_DXC / "
            "HOST_SPIRV_CROSS environment variables).")
    endif()
    set(DXC_EXECUTABLE         "${HOST_DXC}"         CACHE FILEPATH "Host dxc for shader gen" FORCE)
    set(SPIRV_CROSS_EXECUTABLE "${HOST_SPIRV_CROSS}" CACHE FILEPATH "Host spirv-cross for shader gen" FORCE)
else()
    find_program(DXC_EXECUTABLE dxc REQUIRED)
    find_program(SPIRV_CROSS_EXECUTABLE spirv-cross REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Per-platform output flavor for spirv-cross (GLSL dialect + file extension),
# matching the runtime getShaderExtension() of each backend.
#   - WebGL (Emscripten): GLSL ES 3.00, ".glsl"
#   - Nintendo Switch:    GLSL ES 3.20, ".nxs"
#   - Desktop:            GLSL 4.30 core, ".glsl"
# -----------------------------------------------------------------------------
# These must be CACHE variables, not plain ones. This file has include_guard
# (GLOBAL): when a project pulls the engine in via add_subdirectory() and THEN
# include()s this file again, the second include is a no-op. A plain set() here
# would only live in the engine subdirectory's scope, leaving these UNDEFINED
# in the project scope where yume_add_project()/yume_compile_shaders() actually
# run. An empty YUME_GLSL_OUTPUT_EXT makes spirv-cross write "flat.vxs" instead
# of "flat.vxs.glsl", so the OpenGL backend can't find its shaders at runtime
# (material->init fails, the mesh is dropped, only the clear color shows).
# Caching makes them survive across scopes and the include guard.
if(EMSCRIPTEN)
    set(YUME_SPIRV_CROSS_ARGS "--es;--version;300" CACHE STRING "spirv-cross args" FORCE)
    set(YUME_GLSL_OUTPUT_EXT ".glsl" CACHE STRING "GLSL output extension" FORCE)
elseif(YUME_IS_SWITCH)
    set(YUME_SPIRV_CROSS_ARGS "--es;--version;320" CACHE STRING "spirv-cross args" FORCE)
    set(YUME_GLSL_OUTPUT_EXT ".nxs" CACHE STRING "GLSL output extension" FORCE)
else()
    set(YUME_SPIRV_CROSS_ARGS "--no-es;--version;430;--separate-shader-objects" CACHE STRING "spirv-cross args" FORCE)
    set(YUME_GLSL_OUTPUT_EXT ".glsl" CACHE STRING "GLSL output extension" FORCE)
endif()

# -----------------------------------------------------------------------------
# yume_compile_shaders(<out_var> PROJECT_DIR <dir> OUT_DIR <dir>)
#
# Emits custom commands turning every <dir>/*.vxs and *.pxs into GLSL (+ DXIL
# .cso on Windows), written into OUT_DIR, and returns the full list of output
# files in <out_var> so the caller can hang a target off them.
# -----------------------------------------------------------------------------
function(yume_compile_shaders OUT_VAR)
    cmake_parse_arguments(ARG "" "PROJECT_DIR;OUT_DIR" "" ${ARGN})

    file(GLOB VXS_SHADERS "${ARG_PROJECT_DIR}/*.vxs")
    file(GLOB PXS_SHADERS "${ARG_PROJECT_DIR}/*.pxs")

    set(_outputs)

    foreach(SHADER_FILE ${VXS_SHADERS})
        get_filename_component(NAME_WE ${SHADER_FILE} NAME_WE)
        set(SPIRV_FILE "${ARG_OUT_DIR}/${NAME_WE}.vxs.spv")
        set(GLSL_FILE  "${ARG_OUT_DIR}/${NAME_WE}.vxs${YUME_GLSL_OUTPUT_EXT}")

        add_custom_command(
            OUTPUT ${SPIRV_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}
            COMMAND ${DXC_EXECUTABLE} -spirv -T vs_6_0 -E main -fvk-t-shift 3 0 ${SHADER_FILE} -Fo ${SPIRV_FILE}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_FILE} -> ${SPIRV_FILE}")

        if(EMSCRIPTEN)
            add_custom_command(
                OUTPUT ${GLSL_FILE}
                COMMAND ${SPIRV_CROSS_EXECUTABLE} ${YUME_SPIRV_CROSS_ARGS} ${SPIRV_FILE} --output ${GLSL_FILE}
                COMMAND sed -i "s/^out vec3 out_var_/out highp vec3 out_var_/g" ${GLSL_FILE}
                DEPENDS ${SPIRV_FILE}
                COMMENT "Converting ${SPIRV_FILE} -> ${GLSL_FILE}")
        else()
            add_custom_command(
                OUTPUT ${GLSL_FILE}
                COMMAND ${SPIRV_CROSS_EXECUTABLE} ${YUME_SPIRV_CROSS_ARGS} ${SPIRV_FILE} --output ${GLSL_FILE}
                DEPENDS ${SPIRV_FILE}
                COMMENT "Converting ${SPIRV_FILE} -> ${GLSL_FILE}")
        endif()

        list(APPEND _outputs ${GLSL_FILE})

        # DXIL (.cso) for the D3D12 backend. Windows-only, desktop-only.
        if(WIN32 AND NOT EMSCRIPTEN AND NOT YUME_IS_SWITCH)
            set(CSO_FILE "${ARG_OUT_DIR}/${NAME_WE}.vxs.cso")
            add_custom_command(
                OUTPUT ${CSO_FILE}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}
                COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E main ${SHADER_FILE} -Fo ${CSO_FILE}
                DEPENDS ${SHADER_FILE}
                COMMENT "Compiling ${SHADER_FILE} -> ${CSO_FILE} (DXIL)")
            list(APPEND _outputs ${CSO_FILE})
        endif()
    endforeach()

    foreach(SHADER_FILE ${PXS_SHADERS})
        get_filename_component(NAME_WE ${SHADER_FILE} NAME_WE)
        set(SPIRV_FILE "${ARG_OUT_DIR}/${NAME_WE}.pxs.spv")
        set(GLSL_FILE  "${ARG_OUT_DIR}/${NAME_WE}.pxs${YUME_GLSL_OUTPUT_EXT}")

        # -fvk-s-shift 1 0: keep a separate image (t0) and its sampler (s0) from
        # colliding on the same Vulkan binding. Harmless for sampler-less shaders.
        add_custom_command(
            OUTPUT ${SPIRV_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}
            COMMAND ${DXC_EXECUTABLE} -spirv -T ps_6_0 -E main -fvk-s-shift 1 0 ${SHADER_FILE} -Fo ${SPIRV_FILE}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_FILE} -> ${SPIRV_FILE}")

        add_custom_command(
            OUTPUT ${GLSL_FILE}
            COMMAND ${SPIRV_CROSS_EXECUTABLE} ${YUME_SPIRV_CROSS_ARGS} ${SPIRV_FILE} --output ${GLSL_FILE}
            DEPENDS ${SPIRV_FILE}
            COMMENT "Converting ${SPIRV_FILE} -> ${GLSL_FILE}")

        list(APPEND _outputs ${GLSL_FILE})

        if(WIN32 AND NOT EMSCRIPTEN AND NOT YUME_IS_SWITCH)
            set(CSO_FILE "${ARG_OUT_DIR}/${NAME_WE}.pxs.cso")
            add_custom_command(
                OUTPUT ${CSO_FILE}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}
                COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E main ${SHADER_FILE} -Fo ${CSO_FILE}
                DEPENDS ${SHADER_FILE}
                COMMENT "Compiling ${SHADER_FILE} -> ${CSO_FILE} (DXIL)")
            list(APPEND _outputs ${CSO_FILE})
        endif()
    endforeach()

    set(${OUT_VAR} ${_outputs} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# yume_compile_scenes(<out_var> PROJECT_DIR <dir> OUT_DIR <dir>)
#
# Turns every <dir>/*.scn into <OUT_DIR>/*.scnb using the scene_compiler host
# tool, and returns the output file list. Requires the `scene_compiler` target
# (engine build) or a prebuilt binary found via YUME_SCENE_COMPILER.
# -----------------------------------------------------------------------------
function(yume_compile_scenes OUT_VAR)
    cmake_parse_arguments(ARG "" "PROJECT_DIR;OUT_DIR" "" ${ARGN})

    file(GLOB SCENE_FILES "${ARG_PROJECT_DIR}/*.scn")
    set(_outputs)

    foreach(SCENE_FILE ${SCENE_FILES})
        get_filename_component(NAME_WE ${SCENE_FILE} NAME_WE)
        set(OUTPUT_FILE "${ARG_OUT_DIR}/${NAME_WE}.scnb")
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}
            COMMAND ${YUME_SCENE_COMPILER_CMD} ${SCENE_FILE} ${OUTPUT_FILE}
            DEPENDS ${YUME_SCENE_COMPILER_DEPS} ${SCENE_FILE}
            COMMENT "Compiling ${NAME_WE}.scn -> ${NAME_WE}.scnb")
        list(APPEND _outputs ${OUTPUT_FILE})
    endforeach()

    set(${OUT_VAR} ${_outputs} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# yume_copy_assets(<out_var> PROJECT_DIR <dir> OUT_DIR <dir>)
#
# Copies non-compiled runtime assets (*.obj, project.conf, *.png) from the
# project dir into OUT_DIR so everything the engine opens by relative path sits
# next to the executable. Returns the list of copied destination files.
# -----------------------------------------------------------------------------
function(yume_copy_assets OUT_VAR)
    cmake_parse_arguments(ARG "" "PROJECT_DIR;OUT_DIR" "" ${ARGN})

    set(_sources)
    file(GLOB _objs "${ARG_PROJECT_DIR}/*.obj")
    file(GLOB _pngs "${ARG_PROJECT_DIR}/*.png")
    # YumeScript sources are copied verbatim (Phase 1: parsed at runtime by the
    # ScriptComponent). The .scn only stores the .ys path, so the file must sit
    # next to the executable like .obj / project.conf.
    file(GLOB _ys "${ARG_PROJECT_DIR}/*.ys")
    list(APPEND _sources ${_objs} ${_pngs} ${_ys})
    if(EXISTS "${ARG_PROJECT_DIR}/project.conf")
        list(APPEND _sources "${ARG_PROJECT_DIR}/project.conf")
    endif()

    set(_outputs)
    foreach(SRC ${_sources})
        get_filename_component(FNAME ${SRC} NAME)
        set(DST "${ARG_OUT_DIR}/${FNAME}")
        add_custom_command(
            OUTPUT ${DST}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
            DEPENDS ${SRC}
            COMMENT "Copying asset ${FNAME}")
        list(APPEND _outputs ${DST})
    endforeach()

    set(${OUT_VAR} ${_outputs} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# yume_add_project(<target> PROJECT_DIR <dir> [SOURCES <files...>])
#
# High-level helper: creates the executable, links yume_core, and wires the
# asset pipeline so that building <target> also produces every runtime artifact
# in the same directory as the executable.
# -----------------------------------------------------------------------------
function(yume_add_project TARGET)
    cmake_parse_arguments(ARG "" "PROJECT_DIR" "SOURCES" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "yume_add_project(${TARGET}): no SOURCES given")
    endif()

    add_executable(${TARGET} ${ARG_SOURCES})
    target_link_libraries(${TARGET} PRIVATE yume_core)

    # Where the generated runtime artifacts (compiled shaders/scenes + copied
    # assets) are written.
    #   - Desktop/Web: next to the executable, which is also the working dir, so
    #     the engine resolves relative paths directly. Honor
    #     CMAKE_RUNTIME_OUTPUT_DIRECTORY if the project set one.
    #   - Switch: there is no "folder next to the exe" at runtime; assets must be
    #     packed into the .nro's RomFS. So we write them into a romfs/ staging
    #     dir and feed it to elf2nro below. The engine mounts it as romfs:/.
    if(YUME_IS_SWITCH)
        set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/romfs")
    elseif(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(_out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else()
        set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}")
        set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_out_dir})
    endif()

    yume_compile_shaders(_shader_outputs PROJECT_DIR ${ARG_PROJECT_DIR} OUT_DIR ${_out_dir})
    yume_compile_scenes(_scene_outputs   PROJECT_DIR ${ARG_PROJECT_DIR} OUT_DIR ${_out_dir})
    yume_copy_assets(_asset_outputs      PROJECT_DIR ${ARG_PROJECT_DIR} OUT_DIR ${_out_dir})

    add_custom_target(${TARGET}_assets ALL
        DEPENDS ${_shader_outputs} ${_scene_outputs} ${_asset_outputs})
    add_dependencies(${TARGET}_assets ${TARGET})

    # -------------------------------------------------------------------------
    # Windows desktop: deploy the runtime DLLs next to the executable.
    #
    # The graphics deps (SDL2, GLEW, Vulkan) are linked PUBLIC onto yume_core,
    # but yume_core is a STATIC lib, so vcpkg's app-local deploy
    # (VCPKG_APPLOCAL_DEPS) can't reliably trace the transitive DLLs through it
    # and copies them inconsistently (only vulkan-1.dll shows up). Without
    # SDL2.dll / glew32.dll beside main.exe the program fails to start.
    #
    # Copy every DLL from the vcpkg triplet's bin dir next to the executable.
    # This also brings dxcompiler.dll / dxil.dll along, which the D3D12 backend
    # needs. Desktop + Windows only; Web/Switch don't use vcpkg DLLs.
    if(WIN32 AND NOT EMSCRIPTEN AND NOT YUME_IS_SWITCH)
        if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
            # Pick the DLL flavor that matches the build config. A Debug build
            # links the debug import libs, which load the debug-suffixed DLLs
            # (SDL2d.dll, glew32d.dll) living in <triplet>/debug/bin; Release
            # uses the plain names in <triplet>/bin. Copying the wrong flavor
            # leaves the loader hunting for SDL2d.dll/glew32d.dll it can't find.
            #
            # Don't trust CMAKE_BUILD_TYPE alone: when this project is the
            # top-level CMake build, build.bat may leave it empty in the cache
            # (the engine's default only applies in the engine's own scope). So
            # treat "anything that isn't an explicit release config" as debug,
            # and fall back to whichever bin dir actually exists.
            set(_vcpkg_root "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
            string(TOLOWER "${CMAKE_BUILD_TYPE}" _cfg_lower)
            if(_cfg_lower STREQUAL "release"
                    OR _cfg_lower STREQUAL "relwithdebinfo"
                    OR _cfg_lower STREQUAL "minsizerel")
                set(_vcpkg_bin "${_vcpkg_root}/bin")
            else()
                # Debug (or unset): prefer the debug DLLs if vcpkg built them.
                if(EXISTS "${_vcpkg_root}/debug/bin")
                    set(_vcpkg_bin "${_vcpkg_root}/debug/bin")
                else()
                    set(_vcpkg_bin "${_vcpkg_root}/bin")
                endif()
            endif()
            if(EXISTS "${_vcpkg_bin}")
                file(GLOB _runtime_dlls "${_vcpkg_bin}/*.dll")
                add_custom_command(TARGET ${TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_runtime_dlls} "${_out_dir}"
                    COMMAND_EXPAND_LISTS
                    COMMENT "Deploying vcpkg runtime DLLs (${CMAKE_BUILD_TYPE}) next to ${TARGET}")
            else()
                message(WARNING
                    "yume_add_project(${TARGET}): vcpkg bin dir '${_vcpkg_bin}' not found; "
                    "runtime DLLs (SDL2/GLEW/...) will not be deployed next to the executable.")
            endif()
        else()
            message(WARNING
                "yume_add_project(${TARGET}): VCPKG_INSTALLED_DIR / VCPKG_TARGET_TRIPLET "
                "unset; cannot deploy runtime DLLs next to the executable.")
        endif()
    endif()

    # -------------------------------------------------------------------------
    # Switch: turn the linked ELF + the romfs staging dir into a runnable .nro.
    #
    # The devkitPro toolchain links an .elf (CMAKE_EXECUTABLE_SUFFIX is .elf).
    # elf2nro embeds the RomFS; nacptool writes the control metadata (.nacp)
    # that gives the homebrew its title/author/version. Both tools live in
    # $DEVKITPRO/tools/bin. The .nro lands in the project build dir.
    # -------------------------------------------------------------------------
    if(YUME_IS_SWITCH)
        set(_dkp "$ENV{DEVKITPRO}")
        set(_elf2nro  "${_dkp}/tools/bin/elf2nro")
        set(_nacptool "${_dkp}/tools/bin/nacptool")
        set(_default_icon "${_dkp}/libnx/default_icon.jpg")

        set(_nacp "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.nacp")
        set(_nro  "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.nro")

        # Metadata shown on the Switch home menu. Overridable per project via
        # -DYUME_APP_TITLE / _AUTHOR / _VERSION.
        if(NOT YUME_APP_TITLE)
            set(YUME_APP_TITLE "${TARGET}")
        endif()
        if(NOT YUME_APP_AUTHOR)
            set(YUME_APP_AUTHOR "Yume")
        endif()
        if(NOT YUME_APP_VERSION)
            set(YUME_APP_VERSION "1.0.0")
        endif()

        add_custom_command(
            OUTPUT ${_nro}
            # The assets target wrote everything into _out_dir (the romfs dir);
            # depend on it so the romfs is fully populated before packing.
            COMMAND ${_nacptool} --create "${YUME_APP_TITLE}" "${YUME_APP_AUTHOR}" "${YUME_APP_VERSION}" ${_nacp}
            COMMAND ${_elf2nro} $<TARGET_FILE:${TARGET}> ${_nro} --icon=${_default_icon} --nacp=${_nacp} --romfsdir=${_out_dir}
            DEPENDS ${TARGET} ${TARGET}_assets
            COMMENT "Packing ${TARGET}.nro (elf2nro + romfs)")

        add_custom_target(${TARGET}_nro ALL DEPENDS ${_nro})
    endif()
endfunction()
