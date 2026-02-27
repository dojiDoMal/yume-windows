#ifndef WEB_GL_RENDERER_BACKEND_HPP
#define WEB_GL_RENDERER_BACKEND_HPP

#include <GLES3/gl3.h>
#include "graphics_api.hpp"
#include "../../renderer_backend.hpp"
#include "mesh.hpp"
#include <vector>
#include <string>

class WebGLRendererBackend : public RendererBackend {
private: 
    GLuint matricesUBO = 0;

public:
    ~WebGLRendererBackend();
    bool init() override;
    bool initWindowContext() override;
    void clear() override;  
    void draw(const Mesh&) override;
    void setUniforms(unsigned int shaderProgram) override;
    void onCameraSet() override;

    // Skybox management
    unsigned int createCubemapTexture(const std::vector<std::string>& faces) override;
    GraphicsAPI getGraphicsAPI() const override;
    void deleteCubemapTexture(unsigned int textureID);
    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram, unsigned int textureID) override;
};

#endif
