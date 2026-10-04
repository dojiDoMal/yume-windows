#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "assets/asset.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Carrega e armazena em cache recursos (Asset) por caminho.
 *
 * Garante que cada arquivo seja carregado apenas uma vez: loadAsset reaproveita
 * um asset já carregado com o mesmo caminho, ou cria, carrega e guarda um novo.
 * É dono de todos os assets e os libera em unloadAll().
 *
 * @code
 * auto* shader = assets.loadAsset<ShaderAsset>("shaders/basic.vert", ShaderType::VERTEX);
 * @endcode
 *
 * @see Asset
 */
class AssetManager {
  private:
    std::vector<std::unique_ptr<Asset>> assets; ///< Assets carregados (posse exclusiva).

  public:
    /**
     * @brief Carrega um asset do tipo @p T, reaproveitando-o se já estiver em cache.
     * @tparam T    Tipo concreto do asset (deve herdar de Asset).
     * @tparam Args Tipos dos argumentos extras passados ao construtor de @p T.
     * @param path  Caminho do recurso; também é a chave de cache.
     * @param args  Argumentos adicionais encaminhados ao construtor de @p T.
     * @return Ponteiro para o asset carregado, ou @c nullptr se o load() falhar.
     */
    template <typename T, typename... Args> T* loadAsset(const std::string& path, Args&&... args) {
        // Reaproveita o asset se já existir um com o mesmo caminho.
        auto it = std::find_if(
            assets.begin(), assets.end(),
            [&path](const std::unique_ptr<Asset>& asset) { return asset->getPath() == path; });

        if (it != assets.end()) {
            return static_cast<T*>(it->get());
        }

        // Cria um novo asset.
        auto asset = std::make_unique<T>(path, std::forward<Args>(args)...);
        if (!asset->load()) {
            return nullptr;
        }

        T* assetPtr = asset.get();
        assets.push_back(std::move(asset));
        return assetPtr;
    }

    /** @brief Descarrega e remove o asset com o caminho informado. */
    void unloadAsset(const std::string& path);
    /** @brief Retorna o asset com o caminho informado, ou @c nullptr. */
    Asset* getAsset(const std::string& path);
    /** @brief Descarrega e remove todos os assets. */
    void unloadAll();
};

#endif // ASSETMANAGER_HPP
