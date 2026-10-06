#include "engine_context.hpp"
#include "window/mml/multimedia_layer.hpp"

Yume::Context::Context(MultimediaLayer* multimedia) : multimedia(multimedia) {}

// Non-owning: the MultimediaLayer is owned by the Application. Nothing to free.
Yume::Context::~Context() = default;

Yume::IInput& Yume::Context::getInputSystem() { return multimedia->input(); }

AudioBackend& Yume::Context::getAudioSystem() { return multimedia->audio(); }

DisplayBackend& Yume::Context::getDisplaySystem() { return multimedia->display(); }
