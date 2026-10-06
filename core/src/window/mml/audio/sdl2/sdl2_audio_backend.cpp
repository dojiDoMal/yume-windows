#include "sdl2_audio_backend.hpp"
#define CLASS_NAME "SDL2AudioBackend"
#include "log_macros.hpp"

// Stub: no real device is opened yet. SDL_INIT_AUDIO is brought up once by the
// SDL2MultimediaLayer; actual playback (SDL_OpenAudioDevice / SDL_mixer) lands
// on top of this interface later.
bool SDL2AudioBackend::open() {
    if (opened) {
        return true;
    }
    opened = true;
    LOG_INFO("SDL2AudioBackend: audio device opened (stub, no playback yet)");
    return true;
}

void SDL2AudioBackend::close() {
    if (!opened) {
        return;
    }
    opened = false;
}

void SDL2AudioBackend::setMasterVolume(float volume) {
    if (volume < 0.0f) {
        volume = 0.0f;
    } else if (volume > 1.0f) {
        volume = 1.0f;
    }
    masterVolume = volume;
}

float SDL2AudioBackend::getMasterVolume() const { return masterVolume; }
