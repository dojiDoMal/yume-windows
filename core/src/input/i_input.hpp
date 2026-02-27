#ifndef I_INPUT_HPP
#define I_INPUT_HPP

#include "input_key.hpp"
#include <SDL2/SDL.h>
#include <functional>

namespace Yume {
class IInput {
  public:
    virtual ~IInput() = default;

    virtual void processEvents() = 0;
    virtual void bindKey(KeyCode key, std::function<void()> callback) = 0;
    virtual bool getQuitEvent() = 0;
    virtual void requestQuit() = 0;
};
} // namespace Yume

#endif