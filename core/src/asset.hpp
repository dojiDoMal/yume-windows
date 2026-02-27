#ifndef ASSET_HPP
#define ASSET_HPP

#include <string>

class Asset {
  protected:
    std::string path;
    bool loaded = false;

  public:
    Asset(const std::string& assetPath) : path(assetPath) {}
    virtual ~Asset() = default;

    virtual bool load() = 0;
    virtual void unload() = 0;

    bool isLoaded() const { return loaded; }
    const std::string& getPath() const { return path; }
};

#endif // ASSET_HPP