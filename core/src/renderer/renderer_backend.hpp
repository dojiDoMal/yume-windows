#ifndef RENDERER_BACKEND_HPP
#define RENDERER_BACKEND_HPP

#include "../camera.hpp"
#include "../game_object.hpp"
#include "../graphics_api.hpp"
#include "../light.hpp"
#include "../mesh.hpp"
#include "../shader_program.hpp"
#include "../sprite.hpp"
#include <memory>
#include <vector>

struct SDL_Window;

class RendererBackend {
  protected:
    Camera* mainCamera = nullptr;
    std::vector<Light> lights;

  public:
    virtual ~RendererBackend() = default;

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
    
    virtual void renderGameObjects(std::vector<GameObject*>* gameObjects,
                                   std::vector<Light>* lights) = 0;

    virtual void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                              unsigned int textureID) = 0;

    virtual void setBufferDataImpl(const std::string& name, const void* data, size_t size) = 0;
    template <typename T> void setBufferData(const std::string& name, const T* data) {
        setBufferDataImpl(name, static_cast<const void*>(data), sizeof(T));
    }

    Camera* getCamera() { return mainCamera; }

    void setCamera(Camera* camera) {
        mainCamera = camera;
        onCameraSet();
    }

    void setLights(const std::vector<Light>& sceneLights) { lights = sceneLights; }
    const std::vector<Light>& getLights() const { return lights; }
};

#endif // RENDERERBACKEND_HPP
