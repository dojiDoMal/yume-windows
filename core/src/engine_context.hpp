#ifndef ENGINE_CONTEXT_HPP
#define ENGINE_CONTEXT_HPP

#include "input/i_input.hpp"

namespace Yume {
class Context {
  public:
    Context(IInput* input);
    ~Context();
    IInput* input;

    IInput& getInputSystem();
};
} // namespace Yume

#endif