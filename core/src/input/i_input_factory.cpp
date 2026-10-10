#include "input/i_input_factory.hpp"
#include "input/i_input.hpp"

#ifdef __3DS__
#include "input/n3ds_input.hpp"
#else
#include "input/desktop_input.hpp"
#endif

Yume::IInput* Yume::IInputFactory::create() {
#ifdef __3DS__
    return new N3DSInput;
#else
    return new DesktopInput;
#endif
}
