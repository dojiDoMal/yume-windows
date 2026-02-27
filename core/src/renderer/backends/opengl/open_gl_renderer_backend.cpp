#define CLASS_NAME "OpenGLRendererBackend"
#include "../../../log_macros.hpp"

#include "../../../color.hpp"
#include "../../../game_object.hpp"
#include "../../../material.hpp"
#include "../../../mesh_renderer.hpp"
#include "../../../stb_image.h"
#include "mesh_buffer_factory.hpp"
#include "open_gl_renderer_backend.hpp"
#include "shader_compiler_factory.hpp"
#include "shader_program_factory.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GraphicsAPI OpenGLRendererBackend::getGraphicsAPI() const { return GraphicsAPI::OPENGL; }

std::string OpenGLRendererBackend::getShaderExtension() const { return ".glsl"; }

OpenGLRendererBackend::~OpenGLRendererBackend() {
    if (matricesUBO)
        glDeleteBuffers(1, &matricesUBO);
    if (materialDataUBO)
        glDeleteBuffers(1, &materialDataUBO);
    if (lightDataUBO)
        glDeleteBuffers(1, &lightDataUBO);
}

unsigned int OpenGLRendererBackend::getRequiredWindowFlags() const { return SDL_WINDOW_OPENGL; };

std::unique_ptr<ShaderProgram> OpenGLRendererBackend::createShaderProgram() {
    return ShaderProgramFactory::create(getGraphicsAPI());
}

std::unique_ptr<MeshBuffer> OpenGLRendererBackend::createMeshBuffer() {
    return MeshBufferFactory::create(getGraphicsAPI());
}

std::unique_ptr<ShaderCompiler> OpenGLRendererBackend::createShaderCompiler() {
    return ShaderCompilerFactory::create(getGraphicsAPI());
}

bool OpenGLRendererBackend::init(SDL_Window* window) {
    if (!window) {
        LOG_ERROR("Window is null!");
        return false;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        LOG_ERROR("Failed to create OpenGL context!");
        return false;
    }

    return init();
};

bool OpenGLRendererBackend::init() {
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::string glewErr = reinterpret_cast<const char*>(glewGetErrorString(err));
        LOG_ERROR("GLEW initialization failed: " + glewErr);
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenBuffers(1, &matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);

    glGenBuffers(1, &materialDataUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, materialDataUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, materialDataUBO);

    glGenBuffers(1, &lightDataUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, lightDataUBO);
    glBufferData(GL_UNIFORM_BUFFER, 256, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, lightDataUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    uniformBindings["ModelViewProjection"] = matricesUBO;
    uniformBindings["MaterialData"] = materialDataUBO;
    uniformBindings["LightData"] = lightDataUBO;

    initSpriteQuad();

    return true;
}

void OpenGLRendererBackend::onCameraSet() {}

void OpenGLRendererBackend::clear(Camera* camera) {
    ColorRGBA bgColor = camera ? camera->getBackgroundColor() : COLOR::BLACK;

    glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool OpenGLRendererBackend::initWindowContext() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    return true;
}

void OpenGLRendererBackend::draw(const Mesh& mesh) {
    auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh.getMeshBufferHandle()));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.getVertices().size() / 3);
    glBindVertexArray(0);
}

void OpenGLRendererBackend::setUniforms(ShaderProgram* shaderProgram) {
    if (!shaderProgram || !shaderProgram->isValid())
        return;

    shaderProgram->use();
}

void OpenGLRendererBackend::bindCamera(Camera* camera) {
    if (!camera) {
        LOG_ERROR("Camera is null");
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);

    auto& camPos = camera->getPosition();
    glm::mat4 view = glm::lookAt({camPos.x, camPos.y, camPos.z}, glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection;
    if (camera->isOrthographic()) {
        float orthoSize = camera->getOrthoSize();
        float aspect = camera->getAspectRatio();
        LOG_INFO("Ortho projection - size: " + std::to_string(orthoSize) +
                 " aspect: " + std::to_string(aspect));
        projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize,
                                camera->getNearDistance(), camera->getFarDistance());
    } else {
        projection = glm::perspective(glm::radians(camera->getFov()), camera->getAspectRatio(),
                                      camera->getNearDistance(), camera->getFarDistance());
    }

    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(model));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4),
                    glm::value_ptr(projection));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLRendererBackend::setBufferDataImpl(const std::string& name, const void* data,
                                              size_t size) {
    auto it = uniformBindings.find(name);
    if (it != uniformBindings.end()) {
        glBindBuffer(GL_UNIFORM_BUFFER, it->second);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void OpenGLRendererBackend::applyMaterial(Material* material) {
    auto program = material->getShaderProgram();
    if (!program || !program->isValid()) {
        return;
    }

    setUniforms(program);
}

void OpenGLRendererBackend::renderGameObjects(std::vector<GameObject*>* gameObjects,
                                              std::vector<Light>* lights) {
    for (const auto go : *gameObjects) {

        glm::mat4 model = glm::mat4(1.0f);
        if (go->getTransform()) {
            model = go->getTransform()->getModelMatrix();
        }

        glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(model));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        if (go->hasSprite() && go->hasSpriteRenderer()) {
            auto sprite = go->getSprite();
            auto spriteRenderer = go->getSpriteRenderer();
            auto mat = spriteRenderer->getMaterial();

            if (mat) {
                mat->use();
                applyMaterial(mat);
                drawSprite(*sprite);
            }
        } else if (go->hasMesh() && go->hasMeshRenderer()) {
            auto mesh = go->getMesh();
            auto meshRenderer = go->getMeshRenderer();
            auto mat = meshRenderer->getMaterial();

            if (mat) {
                mat->use();
                applyMaterial(mat);
                if (lights && !lights->empty()) {
                    mat->applyLight((*lights)[0]);
                }
                draw(*mesh);
            }
        }
    }
}

unsigned int OpenGLRendererBackend::createCubemapTexture(const std::vector<std::string>& faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format,
                         GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            LOG_WARN("Cubemap texture failed to load at path: " + faces[i].c_str());
            stbi_image_free(data);
            return 0;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void OpenGLRendererBackend::deleteCubemapTexture(unsigned int textureID) {
    glDeleteTextures(1, &textureID);
}

void OpenGLRendererBackend::renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                                         unsigned int textureID) {
    if (!mainCamera)
        return;

    glDepthFunc(GL_LEQUAL);

    auto& camPos = mainCamera->getPosition();
    glm::mat4 camView = glm::lookAt({camPos.x, camPos.y, camPos.z}, glm::vec3(0.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 view = glm::mat4(glm::mat3(camView));
    glm::mat4 projection =
        glm::perspective(glm::radians(mainCamera->getFov()), mainCamera->getAspectRatio(),
                         mainCamera->getNearDistance(), mainCamera->getFarDistance());

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE,
                       glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderProgram, "skybox"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh.getMeshBufferHandle()));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

void OpenGLRendererBackend::present(SDL_Window* window) { SDL_GL_SwapWindow(window); }

void OpenGLRendererBackend::initSpriteQuad() {
    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                        0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
                        0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f};

    glGenVertexArrays(1, &spriteVAO);
    glGenBuffers(1, &spriteVBO);

    glBindVertexArray(spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, spriteVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

unsigned int OpenGLRendererBackend::loadTexture(const std::string& path, uint8_t filterType) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        LOG_INFO("Texture loaded: " + path + " (" + std::to_string(width) + "x" +
                 std::to_string(height) + ", " + std::to_string(nrChannels) + " channels)");

        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLenum filter = (filterType == 1) ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

        stbi_image_free(data);
    } else {
        LOG_ERROR("Failed to load texture: " + path);
    }

    return textureID;
}

void OpenGLRendererBackend::drawSprite(const Sprite& sprite) {
    LOG_INFO("Drawing sprite - TextureID: " + std::to_string(sprite.getTexture()) + " Width: " +
             std::to_string(sprite.getWidth()) + " Height: " + std::to_string(sprite.getHeight()));

    // Não sobrescrever a matriz model, apenas aplicar a escala do sprite
    // A matriz model já foi configurada em renderGameObjects com o Transform
    glm::mat4 spriteScale =
        glm::scale(glm::mat4(1.0f), glm::vec3(sprite.getWidth(), sprite.getHeight(), 1.0f));

    glm::mat4 currentModel;
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(currentModel));

    // Multiplicar: transform * escala do sprite
    glm::mat4 finalModel = currentModel * spriteScale;

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(finalModel));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite.getTexture());

    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint texLoc =
        glGetUniformLocation(currentProgram, "SPIRV_Cross_CombinedspriteTexturespriteSampler");
    LOG_INFO("Texture uniform location: " + std::to_string(texLoc));
    if (texLoc != -1) {
        glUniform1i(texLoc, 0);
    }

    glBindVertexArray(spriteVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERROR("OpenGL error in drawSprite: " + std::to_string(err));
    }
}
