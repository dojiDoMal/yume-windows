#include "asset_manager.hpp"

void AssetManager::unloadAsset(const std::string& path) {
    assets.erase(std::remove_if(assets.begin(), assets.end(),
                                [&path](const std::unique_ptr<Asset>& asset) {
                                    return asset->getPath() == path;
                                }),
                 assets.end());
}

Asset* AssetManager::getAsset(const std::string& path) {
    auto it =
        std::find_if(assets.begin(), assets.end(), [&path](const std::unique_ptr<Asset>& asset) {
            return asset->getPath() == path;
        });

    return (it != assets.end()) ? it->get() : nullptr;
}

void AssetManager::unloadAll() { assets.clear(); }