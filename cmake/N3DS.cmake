# =============================================================================
# N3DS.cmake - Nintendo 3DS (devkitPro / devkitARM / libctru) platform config
#
# Included from the root CMakeLists.txt when building with the devkitARM 3DS
# toolchain (CMAKE_SYSTEM_NAME STREQUAL "Horizon").
#
# Owns the 3DS runtime graphics dependencies and the link line for the Citro3D
# backend. Unlike the Switch/desktop paths, the 3DS does NOT use SDL2/OpenGL: it
# talks to the PICA200 GPU through citro3d and to the system through libctru.
#
# NOTE: The 3DS platform is identified by `__3DS__`, which the devkitARM
# toolchain already defines in its CFLAGS, so there is no need to add it here.
#
# MAX_WORLD_OBJECTS is a global define in the root CMakeLists.txt so it is shared
# by both the engine/app (yume_core) and scene_compiler (the .scnb layout must
# match).
# =============================================================================

if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "DEVKITPRO is not set. export DEVKITPRO=<path to>/devkitpro")
endif()

# devkitARM portlibs for the 3DS (citro3d, tex3ds, and their headers) live under
# portlibs/3ds; libctru lives under libctru. Put both on the include/link paths.
set(N3DS_PORTLIBS "$ENV{DEVKITPRO}/portlibs/3ds")
set(N3DS_LIBCTRU "$ENV{DEVKITPRO}/libctru")

target_include_directories(yume_core PUBLIC
    "${N3DS_PORTLIBS}/include"
    "${N3DS_LIBCTRU}/include")
target_link_directories(yume_core PUBLIC
    "${N3DS_PORTLIBS}/lib"
    "${N3DS_LIBCTRU}/lib")

# -----------------------------------------------------------------------------
# Link line.
#
# ORDER MATTERS (GNU linker resolves left-to-right): citro3d/tex3ds (high level,
# consume the GPU/texture entry points) come before libctru (ctru), and the math
# lib (m) goes last as the lowest-level provider. Mirrors the reference 3DS
# Makefile's `LIBS := -lcitro3d -lctru -lm` (plus tex3ds for .t3x textures).
# -----------------------------------------------------------------------------
target_link_libraries(yume_core PUBLIC
    citro3d
    ctru
    m
)

# NOTE: tex3ds (textura .t3x) foi deixado de fora do link por enquanto -- o
# backend citro3d ainda não usa texturas (ver TODOs em loadTexture/TexEnv). Nem
# toda instalação do devkitPro traz uma lib `tex3ds` linkável (o tex3ds costuma
# vir só como ferramenta de host). Readicionar `-ltex3ds` aqui quando o suporte
# a Tex3DS_* entrar no backend.

# -----------------------------------------------------------------------------
# Shader pipeline (PICA200).
#
# The 3DS does not compile shaders at runtime: PICA200 assembly (*.v.pica) is
# assembled at build time by `picasso` ($DEVKITPRO/tools/bin/picasso) into a
# .shbin, which the engine loads via DVLB_ParseFile (Citro3DShaderCompiler).
# This step lives in the PROJECT build, not here: cmake/YumeProject.cmake defines
# yume_compile_pica_shaders() and yume_add_project() runs it on the 3DS instead
# of the HLSL->SPIR-V->GLSL pipeline, then packs the resulting .shbin into the
# app RomFS via 3dsxtool. There is no fragment shader on the PICA200 (the
# fragment stage is TexEnv, configured by the backend), so only *.v.pica exists.
# -----------------------------------------------------------------------------
