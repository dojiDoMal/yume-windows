#ifndef SPRITE_RENDERER_HPP
#define SPRITE_RENDERER_HPP

#include "components/component.hpp"
#include "assets/material.hpp"
#include <memory>

/**
 * @brief Componente que torna um objeto renderizável como sprite 2D.
 *
 * Análogo ao MeshRenderer, porém para sprites: guarda o Material usado para
 * desenhar o quad texturizado do objeto.
 *
 * @see Component, Material, MeshRenderer
 */
class SpriteRenderer : public Component {
private:
    std::unique_ptr<Material> material; ///< Material usado no desenho do sprite.

public:
    SpriteRenderer() = default;
    /** @brief Define o material deste renderer (assume a posse). */
    void setMaterial(std::unique_ptr<Material> m) { material = std::move(m); }
    /** @brief Retorna o material, ou @c nullptr. */
    Material* getMaterial() { return material.get(); }
    /** @brief Versão const de getMaterial(). */
    const Material* getMaterial() const { return material.get(); }
    /** @brief Indica se há material definido. */
    bool hasMaterial() const { return material != nullptr; }
};

#endif
