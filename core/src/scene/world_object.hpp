#ifndef WORLD_OBJECT_HPP
#define WORLD_OBJECT_HPP

#include "assets/mesh.hpp"
#include "components/component.hpp"
#include "components/transform.hpp"
#include "scene/sprite.hpp"
#include <memory>
#include <typeinfo>
#include <vector>

/**
 * @brief Entidade de cena: um Transform mais uma lista de componentes.
 *
 * É a unidade básica do grafo de cena. Todo objeto tem um Transform e pode
 * receber comportamento adicional anexando componentes (câmera, luz, renderer
 * de malha, etc.). Os componentes são consultados por tipo em tempo de execução:
 *
 * @code
 * auto* obj = manager->createObject();
 * obj->addComponent(std::make_unique<Camera>());
 * if (Camera* cam = obj->getComponent<Camera>()) {
 *     // usar a câmera
 * }
 * @endcode
 *
 * @see Component, WorldObjectManager, Transform
 */
class WorldObject {
  private:
    Transform transform;                                ///< Transformação do objeto.
    std::vector<std::unique_ptr<Component>> components; ///< Componentes anexados (posse exclusiva).

    // TODO: Remover uso de mesh e sprite diretamente
    std::shared_ptr<Mesh> mesh;
    std::unique_ptr<Sprite> sprite;

  public:
    WorldObject() = default;

    // TODO: esse gettransform poderia por baixo dos panos chamar getComponent<Transform>
    // e o transform ficar dentro do vetor de componentes ?? só teria um transform mesmo...

    /** @brief Retorna a transformação do objeto. */
    Transform& getTransform();
    /** @brief Retorna a transformação do objeto (somente leitura). */
    const Transform& getTransform() const;

    /**
     * @brief Anexa um componente ao objeto, assumindo sua posse.
     * @tparam T   Tipo do componente (deve herdar de Component).
     * @param component Componente a anexar.
     * @return Ponteiro observador para o componente anexado.
     */
    template <typename T> T* addComponent(std::unique_ptr<T> component) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        T* ptr = component.get();
        ptr->setOwner(this);
        components.push_back(std::move(component));
        return ptr;
    }

    /**
     * @brief Busca o primeiro componente do tipo @p T.
     * @tparam T Tipo do componente procurado.
     * @return Ponteiro para o componente, ou @c nullptr se não houver.
     */
    template <typename T> T* getComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        for (auto& comp : components) {
            if (T* ptr = dynamic_cast<T*>(comp.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    /** @brief Versão const de getComponent(). */
    template <typename T> const T* getComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        for (auto& comp : components) {
            if (const T* ptr = dynamic_cast<const T*>(comp.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    /** @brief Indica se o objeto possui um componente do tipo @p T. */
    template <typename T> bool hasComponent() const { return getComponent<T>() != nullptr; }

    /**
     * @brief Chama Component::start() em todos os componentes do objeto.
     *
     * Executado uma vez pela Application depois que a cena termina de carregar,
     * quando o objeto já está completo (todos os componentes anexados).
     */
    void startComponents() {
        for (auto& comp : components)
            comp->start();
    }

    /**
     * @brief Chama Component::update() em todos os componentes do objeto.
     * @param deltaTime Tempo do frame, em segundos, repassado a cada componente.
     */
    void updateComponents(float deltaTime) {
        for (auto& comp : components)
            comp->update(deltaTime);
    }

    // TODO: remover suporte a legacy mesh/sprite

    /** @brief [Legado] Define a malha associada diretamente ao objeto. */
    void setMesh(std::shared_ptr<Mesh> m) { mesh = std::move(m); }
    /** @brief [Legado] Retorna a malha associada, ou @c nullptr. */
    Mesh* getMesh() { return mesh.get(); }
    /** @brief [Legado] Versão const de getMesh(). */
    const Mesh* getMesh() const { return mesh.get(); }
    /** @brief [Legado] Indica se há malha associada. */
    bool hasMesh() const { return mesh != nullptr; }

    /** @brief [Legado] Define o sprite associado diretamente ao objeto. */
    void setSprite(std::unique_ptr<Sprite> s) { sprite = std::move(s); }
    /** @brief [Legado] Retorna o sprite associado, ou @c nullptr. */
    Sprite* getSprite() { return sprite.get(); }
    /** @brief [Legado] Versão const de getSprite(). */
    const Sprite* getSprite() const { return sprite.get(); }
    /** @brief [Legado] Indica se há sprite associado. */
    bool hasSprite() const { return sprite != nullptr; }
};

#endif
