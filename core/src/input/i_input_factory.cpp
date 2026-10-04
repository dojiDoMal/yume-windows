#include "input/i_input_factory.hpp"
#include "input/desktop_input.hpp"
#include "input/i_input.hpp"

Yume::IInput* Yume::IInputFactory::create() {
    return new DesktopInput;
}
