// Single translation unit that compiles the stb_image implementation into
// yume_core. All backends include "stb_image.h" for the declarations only;
// the implementation (stbi_load, stbi_image_free, ...) must live in exactly
// one .cpp so the symbols are defined once in the static library.
#define STB_IMAGE_IMPLEMENTATION
#include "assets/stb_image.h"
