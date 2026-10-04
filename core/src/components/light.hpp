#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include "../color.hpp"
#include "../vector3.hpp"
#include "component.hpp"
#include <cstdint>

/// @brief Modelo de iluminação de uma luz.
enum class LightType : uint8_t {
    DIRECTIONAL = 0, ///< Luz direcional (ex.: sol): só direção, sem posição.
    POINT = 1,       ///< Luz pontual: irradia a partir de um ponto.
    SPOT = 2         ///< Holofote: cone de luz a partir de um ponto.
};

/**
 * @brief Componente de fonte de luz da cena.
 *
 * Define tipo, direção, cor e intensidade. O Material consome esses valores em
 * applyLight() ao desenhar superfícies iluminadas.
 *
 * @see Component, Material::applyLight
 */
class Light : public Component {
  private:
    LightType type = LightType::DIRECTIONAL;    ///< Tipo da luz.
    Vector3 direction = {0.0f, -1.0f, 0.0f};    ///< Direção (usada por direcional/spot).
    ColorRGBA color = {1.0f, 1.0f, 1.0f, 1.0f}; ///< Cor da luz.
    float intensity = 1.0f;                     ///< Intensidade (multiplicador).

  public:
    Light() = default;

    /** @brief Define o tipo da luz. */
    void setType(LightType t);
    /** @brief Retorna o tipo da luz. */
    LightType getType() const;
    /** @brief Define a direção da luz. */
    void setDirection(const Vector3& dir);
    /** @brief Retorna a direção da luz. */
    const Vector3& getDirection() const;
    /** @brief Define a cor da luz. */
    void setColor(const ColorRGBA& c);
    /** @brief Retorna a cor da luz. */
    const ColorRGBA& getColor() const;
    /** @brief Define a intensidade da luz. */
    void setIntensity(float i);
    /** @brief Retorna a intensidade da luz. */
    float getIntensity() const;
};

#endif
