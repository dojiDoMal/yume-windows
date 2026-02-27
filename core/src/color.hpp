#ifndef COLOR_HPP
#define COLOR_HPP

struct ColorRGBA {
    union {
        struct {
            float r, g, b, a;
        };
        float v[4];
    };
};

namespace COLOR {
inline constexpr ColorRGBA RED = {1.0f, 0.0f, 0.0f, 1.0f};
inline constexpr ColorRGBA GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
inline constexpr ColorRGBA BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
inline constexpr ColorRGBA WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr ColorRGBA BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr ColorRGBA YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};
inline constexpr ColorRGBA CYAN = {0.0f, 1.0f, 1.0f, 1.0f};
inline constexpr ColorRGBA MAGENTA = {1.0f, 0.0f, 1.0f, 1.0f};
} // namespace COLOR

#endif // COLOR_HPP
