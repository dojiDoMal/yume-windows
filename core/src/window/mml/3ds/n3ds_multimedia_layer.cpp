#define CLASS_NAME "N3DSMultimediaLayer"
#include "n3ds_multimedia_layer.hpp"
#include "log_macros.hpp"

#include "input/n3ds_input.hpp"
#include "window/mml/3ds/n3ds_display_backend.hpp"
#include "window/mml/audio/3ds/n3ds_audio_backend.hpp"

#include <3ds.h>

N3DSMultimediaLayer::N3DSMultimediaLayer() = default;

N3DSMultimediaLayer::~N3DSMultimediaLayer() { end(); }

bool N3DSMultimediaLayer::init() {
    if (initialized)
        return true;

    // A camada de multimídia é dona do ciclo de vida do subsistema de vídeo do
    // 3DS. gfxInitDefault liga as duas telas; o C3D_Init e a criação do render
    // target ficam no N3DSDisplayBackend::createMainDisplay (análogo a criar a
    // janela no SDL).
    gfxInitDefault();

    displayBackend = std::make_unique<N3DSDisplayBackend>();
    inputBackend = std::make_unique<Yume::N3DSInput>();
    audioBackend = std::make_unique<N3DSAudioBackend>();

    if (!audioBackend->open()) {
        // Áudio é não-fatal: sua ausência não deve impedir o engine de subir.
        LOG_WARN("Audio device could not be opened; continuing without audio");
    }

    initialized = true;
    return true;
}

void N3DSMultimediaLayer::end() {
    if (!initialized)
        return;

    // Solta os sub-backends antes de finalizar o vídeo, para que o
    // N3DSDisplayBackend libere o render target / C3D enquanto a GPU ainda está
    // ativa (seu destrutor chama C3D_Fini).
    if (audioBackend)
        audioBackend->close();

    audioBackend.reset();
    inputBackend.reset();
    displayBackend.reset();

    gfxExit();
    initialized = false;
}

DisplayBackend& N3DSMultimediaLayer::display() { return *displayBackend; }

Yume::IInput& N3DSMultimediaLayer::input() { return *inputBackend; }

AudioBackend& N3DSMultimediaLayer::audio() { return *audioBackend; }
