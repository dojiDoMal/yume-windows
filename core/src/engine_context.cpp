#include "engine_context.hpp"

Yume::Context::Context(IInput* input) : input(input) {}

Yume::Context::~Context() {
    if (input) {
        delete input;
    }
}

Yume::IInput& Yume::Context::getInputSystem() { 
    return *input; 
}