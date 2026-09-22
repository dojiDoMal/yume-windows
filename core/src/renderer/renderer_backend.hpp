#ifndef RENDERER_BACKEND_HPP
#define RENDERER_BACKEND_HPP

#include "../color.hpp"
#include "../components/camera.hpp"
#include "../components/light.hpp"
#include "../font_atlas.hpp"
#include "../graphics_api.hpp"
#include "../mesh.hpp"
#include "../shader_program.hpp"
#include "../sprite.hpp"
#include "../world_object.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct SDL_Window;

class RendererBackend {
  protected:
    Camera* mainCamera = nullptr;
    std::vector<Light*> lights;

    // Per-frame draw statistics: what was actually submitted to the GPU this
    // frame after culling. Reset at the start of renderWorldObjects and
    // incremented at each draw call.
    int drawnObjects = 0;
    int drawnVerts = 0;
    int drawnTris = 0;

    // Objects rejected by frustum culling this frame (set by Renderer::render).
    int frustumCulledObjects = 0;

    bool frustumCullingEnabled = true;

    // Presentation config, set from project.conf before init(window). Backends
    // translate these into their native formats/present modes. Defaults match
    // the historical behavior (linear/UNORM output, vsync on).
    bool srgbEnabled = false;
    bool vsyncEnabled = true;

  public:
    virtual ~RendererBackend() = default;

    // Set by the host (from RendererConfig) prior to init(window). Kept as
    // plain state so each backend reads it during its own initialization.
    void setSrgbEnabled(bool enabled) { srgbEnabled = enabled; }
    bool isSrgbEnabled() const { return srgbEnabled; }
    void setVsyncEnabled(bool enabled) { vsyncEnabled = enabled; }
    bool isVsyncEnabled() const { return vsyncEnabled; }

    // Per-frame drawn stats accessors.
    int getDrawnObjects() const { return drawnObjects; }
    int getDrawnVerts() const { return drawnVerts; }
    int getDrawnTris() const { return drawnTris; }
    int getFrustumCulledObjects() const { return frustumCulledObjects; }
    void setFrustumCulledObjects(int n) { frustumCulledObjects = n; }

    bool isFrustumCullingEnabled() const { return frustumCullingEnabled; }
    void setFrustumCullingEnabled(bool enabled) { frustumCullingEnabled = enabled; }

    virtual unsigned int loadTexture(const std::string& path, uint8_t filterType = 0) = 0;
    virtual void drawSprite(const Sprite& sprite) = 0;
    virtual bool init() = 0;
    virtual bool init(SDL_Window* window) = 0;
    virtual void present(SDL_Window* window) = 0;
    virtual bool initWindowContext() = 0;
    virtual void bindCamera(Camera* camera) = 0;
    virtual void applyMaterial(Material* material) = 0;
    virtual void clear(Camera* camera) = 0;
    virtual void draw(const Mesh&) = 0;
    virtual GraphicsAPI getGraphicsAPI() const = 0;
    virtual std::string getShaderExtension() const = 0;
    virtual unsigned int createCubemapTexture(const std::vector<std::string>& faces) = 0;
    virtual std::unique_ptr<ShaderProgram> createShaderProgram() = 0;
    virtual std::unique_ptr<ShaderCompiler> createShaderCompiler() = 0;
    virtual std::unique_ptr<MeshBuffer> createMeshBuffer() = 0;
    virtual void onCameraSet() = 0;
    virtual void setUniforms(ShaderProgram* shaderProgram) = 0;
    virtual unsigned int getRequiredWindowFlags() const = 0;

    virtual void renderWorldObjects(const std::vector<WorldObject*>& objects,
                                    const std::vector<Light*>& lights) = 0;

    virtual void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                              unsigned int textureID) = 0;

    virtual void setBufferDataImpl(const std::string& name, const void* data, size_t size) = 0;

    virtual bool initText(const FontAtlas& atlas, unsigned int textureID,
                          const std::string& vertPath, const std::string& fragPath) {
        return false;
    }

    virtual void drawText(const std::string& text, float x, float y, float scale, ColorRGBA color,
                          int screenWidth, int screenHeight) {}

    template <typename T> void setBufferData(const std::string& name, const T* data) {
        setBufferDataImpl(name, static_cast<const void*>(data), sizeof(T));
    }

    Camera* getCamera() { return mainCamera; }

    void setCamera(Camera* camera) {
        mainCamera = camera;
        onCameraSet();
    }

    void setLights(const std::vector<Light*>& sceneLights) { lights = sceneLights; }
    const std::vector<Light*>& getLights() const { return lights; }
};

#endif
