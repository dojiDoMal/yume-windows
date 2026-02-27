#ifndef IINPUT_FACTORY_HPP
#define IINPUT_FACTORY_HPP

#include "i_input.hpp"

namespace Yume {
class IInputFactory {
  public:
    static IInput* create();
};
} // namespace Yume

#endif