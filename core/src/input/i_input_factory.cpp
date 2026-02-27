#include "i_input_factory.hpp"
#include "desktop_input.hpp"
#include "i_input.hpp"

Yume::IInput* Yume::IInputFactory::create() {
    return new DesktopInput;
}
