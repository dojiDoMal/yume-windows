#ifndef ASSET_HPP
#define ASSET_HPP

#include <string>

/**
 * @brief Classe base para todo recurso carregável de disco (shaders, texturas,
 *        malhas, etc.).
 *
 * Define um ciclo de vida simples em duas fases: load() traz o recurso para a
 * memória e unload() o libera. Cada subclasse implementa esses métodos de
 * acordo com seu formato. O caminho do arquivo e o estado "carregado" ficam
 * disponíveis via getPath() e isLoaded().
 *
 * @see ShaderAsset
 */
class Asset {
  protected:
    std::string path;     ///< Caminho do arquivo de origem do recurso.
    bool loaded = false;  ///< @c true enquanto o recurso está carregado.

  public:
    /**
     * @brief Constrói o asset associando-o a um caminho de arquivo.
     * @param assetPath Caminho do recurso em disco.
     */
    Asset(const std::string& assetPath) : path(assetPath) {}
    virtual ~Asset() = default;

    /**
     * @brief Carrega o recurso para a memória.
     * @return @c true em caso de sucesso.
     */
    virtual bool load() = 0;

    /** @brief Libera o recurso da memória. */
    virtual void unload() = 0;

    /** @brief Indica se o recurso está atualmente carregado. */
    bool isLoaded() const { return loaded; }

    /** @brief Retorna o caminho de origem do recurso. */
    const std::string& getPath() const { return path; }
};

#endif // ASSET_HPP
