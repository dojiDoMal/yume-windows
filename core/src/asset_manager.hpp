#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "asset.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>


class AssetManager {
  private:
    std::vector<std::unique_ptr<Asset>> assets;

  public:
    template <typename T, typename... Args> T* loadAsset(const std::string& path, Args&&... args) {
        // Check if asset already exists
        auto it = std::find_if(
            assets.begin(), assets.end(),
            [&path](const std::unique_ptr<Asset>& asset) { return asset->getPath() == path; });

        if (it != assets.end()) {
            return static_cast<T*>(it->get());
        }

        // Create new asset
        auto asset = std::make_unique<T>(path, std::forward<Args>(args)...);
        if (!asset->load()) {
            return nullptr;
        }

        T* assetPtr = asset.get();
        assets.push_back(std::move(asset));
        return assetPtr;
    }

    void unloadAsset(const std::string& path);
    Asset* getAsset(const std::string& path);
    void unloadAll();
};

#endif // ASSETMANAGER_HPP
