#include "sdl2_multimedia_layer.hpp"
#define CLASS_NAME "SDL2MultimediaLayer"
#include "log_macros.hpp"

#include "input/i_input_factory.hpp"
#include "window/mml/audio/sdl2/sdl2_audio_backend.hpp"
#include "window/mml/sdl2/sdl2_display_backend.hpp"
#include "window/mml/sdl2/sdl2_gl_display_backend.hpp"
// Vulkan/D3D12 display backends only exist on platforms that ship those APIs;
// the CMake build drops their .cpp elsewhere, so guard the includes to match.
#if !defined(__SWITCH__) && !defined(PLATFORM_WEBGL)
#include "window/mml/sdl2/sdl2_vulkan_display_backend.hpp"
#ifdef _WIN32
#include "window/mml/sdl2/sdl2_d3d12_display_backend.hpp"
#endif
#endif

#ifdef __SWITCH__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

namespace {
// Pick the SDL2 display backend that matches the active graphics API. Each
// per-API backend is the single place that touches the SDL calls specific to
// that API (SDL_GL_*, SDL_Vulkan_*, SDL_syswm), so a platform without an API
// never even compiles its backend.
DisplayBackend* createDisplayBackendFor(const GraphicsAPI& api) {
    switch (api) {
    case GraphicsAPI::OPENGL:
    case GraphicsAPI::WEBGL:
        return new SDL2GLDisplayBackend();
    case GraphicsAPI::VULKAN:
#if !defined(__SWITCH__) && !defined(PLATFORM_WEBGL)
        return new SDL2VulkanDisplayBackend();
#else
        return nullptr;
#endif
    case GraphicsAPI::DIRECTX12:
#if defined(_WIN32) && !defined(__SWITCH__) && !defined(PLATFORM_WEBGL)
        return new SDL2D3D12DisplayBackend();
#else
        return nullptr;
#endif
    default:
        return nullptr;
    }
}
} // namespace

SDL2MultimediaLayer::SDL2MultimediaLayer(const GraphicsAPI& api) : graphicsApi(api) {}

SDL2MultimediaLayer::~SDL2MultimediaLayer() { end(); }

bool SDL2MultimediaLayer::init() {
    if (initialized) {
        return true;
    }

    // The engine owns the SDL lifecycle. SDL_MAIN_HANDLED is defined so SDL
    // does NOT hijack main() into SDL_main and SDL2main is not linked. Bring up
    // video AND audio here, once, so every sub-backend (display/input/audio)
    // shares the same initialized SDL instead of each poking SDL_Init on its
    // own.
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    displayBackend.reset(createDisplayBackendFor(graphicsApi));
    if (!displayBackend) {
        LOG_ERROR("No SDL2 display backend for the selected graphics API");
        SDL_Quit();
        return false;
    }
    inputBackend.reset(Yume::IInputFactory::create());
    audioBackend = std::make_unique<SDL2AudioBackend>();

    if (!audioBackend->open()) {
        // Audio is non-fatal: a missing/held device shouldn't stop the engine.
        LOG_WARN("Audio device could not be opened; continuing without audio");
    }

    initialized = true;
    return true;
}

void SDL2MultimediaLayer::end() {
    if (!initialized) {
        return;
    }

    if (audioBackend) {
        audioBackend->close();
    }

    // Drop the sub-backends before finalizing SDL so any SDL resources they
    // hold are released while SDL is still up.
    audioBackend.reset();
    inputBackend.reset();
    displayBackend.reset();

    SDL_Quit();
    initialized = false;
}

DisplayBackend& SDL2MultimediaLayer::display() { return *displayBackend; }

Yume::IInput& SDL2MultimediaLayer::input() { return *inputBackend; }

AudioBackend& SDL2MultimediaLayer::audio() { return *audioBackend; }
