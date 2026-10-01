# =============================================================================
# PC.cmake - Native desktop (Windows/Linux) platform configuration
#
# Included from the root CMakeLists.txt for the native build. Owns the desktop
# graphics dependencies and the link line for the DirectX12/Vulkan/OpenGL
# backends, attaching them to the engine library `yume_core` as PUBLIC so the
# project executable that links yume_core inherits them.
#
# The HLSL -> DXIL (.cso) shader step now lives in cmake/YumeProject.cmake and
# runs per-project (next to each example's executable), not here.
# =============================================================================

set(OpenGL_GL_PREFERENCE GLVND)

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

# The engine owns the entry point and initializes SDL itself (SDL_SetMainReady
# + SDL_Init in Application::boot), so we do NOT link SDL2main and we define
# SDL_MAIN_HANDLED. This keeps the project's plain `int main()` as the real
# entry point: without it, SDL's <SDL.h> redefines main to SDL_main and the
# SDL2main shim fails to link (undefined reference to SDL_main) because the
# project's main is wrapped by the Application class, not named SDL_main.
# PUBLIC so the define reaches every TU that includes <SDL2/SDL.h>, engine and
# project alike.
target_compile_definitions(yume_core PUBLIC SDL_MAIN_HANDLED)

target_link_libraries(yume_core PUBLIC
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    GLEW::GLEW
    OpenGL::GL
    Vulkan::Vulkan
    d3d12
    dxgi
    d3dcompiler
)
