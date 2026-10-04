#ifndef CAMERA_COMPONENT_HPP
#define CAMERA_COMPONENT_HPP

#include "color.hpp"
#include "scene/skybox.hpp"
#include "components/component.hpp"

/**
 * @brief Componente de câmera: define o ponto de vista da cena.
 *
 * Controla a projeção (perspectiva ou ortográfica), os parâmetros associados
 * (campo de visão, planos near/far, dimensões da viewport), a cor de fundo e um
 * skybox opcional. A posição/orientação vêm do Transform do objeto dono; este
 * componente guarda apenas as propriedades de projeção e apresentação.
 *
 * @see Component, Skybox, Scene::getCamera
 */
class Camera : public Component {
  private:
    ColorRGBA backgroundColor = {0.2f, 0.3f, 0.3f, 1.0f}; ///< Cor de limpeza do fundo.
    std::unique_ptr<Skybox> skybox;    ///< Skybox opcional desenhado ao fundo.
    float fov = 45.0f;                 ///< Campo de visão vertical, em graus (perspectiva).
    float nearDistance = 0.1f;         ///< Plano de corte próximo.
    float farDistance = 100.0f;        ///< Plano de corte distante.
    float width = 800.0f;              ///< Largura da viewport.
    float height = 600.0f;             ///< Altura da viewport.
    bool orthographic = false;         ///< @c true usa projeção ortográfica.
    float orthoSize = 1.0f;            ///< Tamanho da projeção ortográfica.

  public:
    Camera() = default;
    ~Camera() = default;

    /** @brief Retorna a cor de fundo (referência modificável). */
    ColorRGBA& getBackgroundColor();
    /** @brief Define a cor de fundo. */
    void setBackgroundColor(const ColorRGBA& color);
    /** @brief Define o campo de visão vertical, em graus. */
    void setFov(float fov);
    /** @brief Retorna o campo de visão vertical, em graus. */
    float getFov() const;
    /** @brief Define a distância do plano de corte próximo. */
    void setNearDistance(float nearDistance);
    /** @brief Retorna a distância do plano de corte próximo. */
    float getNearDistance() const;
    /** @brief Define a distância do plano de corte distante. */
    void setFarDistance(float farDistance);
    /** @brief Retorna a distância do plano de corte distante. */
    float getFarDistance() const;
    /** @brief Define a largura da viewport. */
    void setWidth(float width);
    /** @brief Retorna a largura da viewport. */
    float getWidth() const;
    /** @brief Define a altura da viewport. */
    void setHeight(float height);
    /** @brief Retorna a altura da viewport. */
    float getHeight() const;
    /** @brief Retorna a razão de aspecto (largura/altura). */
    float getAspectRatio() const;
    /** @brief Define o tamanho da projeção ortográfica. */
    void setOrthoSize(float size);
    /** @brief Retorna o tamanho da projeção ortográfica. */
    float getOrthoSize() const;
    /** @brief Define o skybox da câmera (assume a posse). */
    void setSkybox(std::unique_ptr<Skybox> skybox);
    /** @brief Retorna o skybox da câmera, ou @c nullptr. */
    Skybox* getSkybox() const;
    /** @brief Alterna entre projeção ortográfica (@c true) e perspectiva. */
    void setOrthographic(bool ortho);
    /** @brief Indica se a projeção é ortográfica. */
    bool isOrthographic() const;
    /** @brief Define largura e altura da viewport de uma vez. */
    void setViewRect(float width, float height);
};

#endif // CAMERA_HPP
