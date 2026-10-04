#ifndef COLOR_HPP
#define COLOR_HPP

/**
 * @brief Cor RGBA com componentes em ponto flutuante no intervalo [0, 1].
 *
 * r, g, b são as componentes de cor e a é o alfa (opacidade). Como nos vetores,
 * os valores podem ser acessados por nome ou pelo array @c v (mesma memória).
 * Cores nomeadas prontas estão no namespace ::COLOR (ex.: COLOR::RED).
 */
struct ColorRGBA {
    union {
        struct {
            float r, g, b, a;
        };
        float v[4]; ///< Acesso por índice (compartilha memória com r,g,b,a).
    };
};

/// @brief Paleta de cores comuns já definidas.
namespace COLOR {
inline constexpr ColorRGBA RED = {1.0f, 0.0f, 0.0f, 1.0f};     ///< Vermelho opaco.
inline constexpr ColorRGBA GREEN = {0.0f, 1.0f, 0.0f, 1.0f};   ///< Verde opaco.
inline constexpr ColorRGBA BLUE = {0.0f, 0.0f, 1.0f, 1.0f};    ///< Azul opaco.
inline constexpr ColorRGBA WHITE = {1.0f, 1.0f, 1.0f, 1.0f};   ///< Branco opaco.
inline constexpr ColorRGBA BLACK = {0.0f, 0.0f, 0.0f, 1.0f};   ///< Preto opaco.
inline constexpr ColorRGBA YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};  ///< Amarelo opaco.
inline constexpr ColorRGBA CYAN = {0.0f, 1.0f, 1.0f, 1.0f};    ///< Ciano opaco.
inline constexpr ColorRGBA MAGENTA = {1.0f, 0.0f, 1.0f, 1.0f}; ///< Magenta opaco.
} // namespace COLOR

#endif // COLOR_HPP
