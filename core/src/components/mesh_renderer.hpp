#ifndef MESH_RENDERER_HPP
#define MESH_RENDERER_HPP

#include "../material.hpp"
#include "component.hpp"
#include <memory>

/**
 * @brief Componente que torna um objeto renderizável como malha 3D.
 *
 * Guarda o Material usado para desenhar a malha do objeto. O renderer da cena
 * seleciona objetos com este componente (ver Scene::getRenderableObjects).
 *
 * @see Component, Material
 */
class MeshRenderer : public Component {
  private:
    std::shared_ptr<Material> material; ///< Material usado no desenho da malha.

  public:
    MeshRenderer() = default;
    /** @brief Define o material deste renderer (compartilhado). */
    void setMaterial(std::shared_ptr<Material> m) { material = std::move(m); };
    /** @brief Retorna o material, ou @c nullptr. */
    Material* getMaterial() { return material.get(); }
    /** @brief Versão const de getMaterial(). */
    const Material* getMaterial() const { return material.get(); }
    /** @brief Indica se há material definido. */
    bool hasMaterial() const { return material != nullptr; }
};

#endif // MESH_RENDERER_HPP
