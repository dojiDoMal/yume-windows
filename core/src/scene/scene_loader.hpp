#ifndef SCENE_LOADER_HPP
#define SCENE_LOADER_HPP

#include "assets/font_atlas.hpp"
#include "assets/material.hpp"
#include "assets/mesh.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene/scene_format.hpp"
#include "scene/world_object.hpp"
#include "scene/world_object_manager.hpp"
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief Lê uma cena compilada (.scnb) e monta os WorldObject em memória.
 *
 * Faz o inverso do compilador de cena: valida e carrega o arquivo binário para
 * um CompiledScene (POD) e depois instancia os objetos e seus componentes num
 * WorldObjectManager. Recursos pesados (malhas, materiais, atlas de fonte) são
 * mantidos em caches internos para serem reaproveitados entre objetos.
 *
 * @see SceneManager, CompiledScene, scene_format.hpp
 */
class SceneLoader {
  private:
    RendererBackend* rendererBackend = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Mesh>>
        meshCache; ///< Cache de malhas por caminho.
    std::unordered_map<std::string, std::shared_ptr<Material>>
        materialCache; ///< Cache de materiais.
    std::unordered_map<std::string, std::shared_ptr<FontAtlas>>
        fontAtlasCache; ///< Cache de atlas de fonte.

    std::shared_ptr<Mesh> loadObjMesh(const std::string& filepath, bool shadeSmooth);

    /**
     * @brief Cria e inicializa um Material a partir dos dados de disco.
     *
     * Centraliza o passo repetido de montar os shaders (vértice/fragmento) com
     * seus compiladores, criar o Material, ligar o programa de shader, aplicar a
     * cor base e chamar init(). A extensão dos shaders vem do backend.
     *
     * @param matData Caminhos dos shaders e cor base.
     * @return O material pronto para uso, ou @c nullptr se init() falhar.
     */
    std::unique_ptr<Material> createMaterial(const MaterialData& matData);

    void loadMeshRendererComponent(WorldObject* obj, const ComponentData& comp);
    void loadSpriteRendererComponent(WorldObject* obj, const ComponentData& comp);
    void loadCameraComponent(WorldObject* obj, const ComponentData& comp);
    void loadLightComponent(WorldObject* obj, const ComponentData& comp);
    void loadTextRendererComponent(WorldObject* obj, const ComponentData& comp);
    void loadLodGroupComponent(WorldObject* obj, const ComponentData& comp);
    void loadScriptComponent(WorldObject* obj, const ComponentData& comp);

  public:
    SceneLoader();

    /** @brief Define o backend usado para criar recursos gráficos dos objetos. */
    void setRendererBackend(RendererBackend&);

    /**
     * @brief Verifica se o arquivo é uma cena válida (magic/estrutura).
     * @param filepath Caminho do arquivo .scnb.
     * @return @c true se o arquivo parece válido.
     */
    bool validateSceneFile(const std::string& filepath);

    /**
     * @brief Lê o arquivo binário e devolve a cena em forma de dados (POD).
     * @param filepath Caminho do arquivo .scnb.
     * @return A cena compilada, ou @c nullptr em caso de falha.
     */
    std::unique_ptr<CompiledScene> loadCompiledScene(const std::string& filepath);

    /**
     * @brief Instancia os WorldObject da cena no gerenciador fornecido.
     * @param manager Destino dos objetos criados.
     * @param scene   Dados da cena previamente carregados.
     */
    void loadWorldObjects(WorldObjectManager* manager, const CompiledScene* scene);
};

#endif
