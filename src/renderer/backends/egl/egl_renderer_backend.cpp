#define CLASS_NAME "EGLRendererBackend"
#include "../../../log_macros.hpp"

#include "../../../color.hpp"
#include "../../../components/mesh_renderer.hpp"
#include "../../../components/sprite_renderer.hpp"
#include "../../../material.hpp"
#include "../../../math.hpp"
#include "../../../stb_image.h"
#include "egl_renderer_backend.hpp"
#include "mesh_buffer_factory.hpp"
#include "shader_compiler_factory.hpp"
#include "shader_program_factory.hpp"
#include <SDL.h>
#include <fstream>
#include <glad/glad.h>
#include <sstream>

GraphicsAPI EGLRendererBackend::getGraphicsAPI() const { return GraphicsAPI::EGL; }

std::string EGLRendererBackend::getShaderExtension() const { return ".nxs"; }

EGLRendererBackend::~EGLRendererBackend() {
    if (instanceSSBO)
        glDeleteBuffers(1, &instanceSSBO);
    if (matricesUBO)
        glDeleteBuffers(1, &matricesUBO);
    if (materialDataUBO)
        glDeleteBuffers(1, &materialDataUBO);
    if (lightDataUBO)
        glDeleteBuffers(1, &lightDataUBO);
    if (textShaderProgram)
        glDeleteProgram(textShaderProgram);
    if (textVAO)
        glDeleteVertexArrays(1, &textVAO);
    if (textVBO)
        glDeleteBuffers(1, &textVBO);
    if (textUBOProjection)
        glDeleteBuffers(1, &textUBOProjection);
    if (textUBOColor)
        glDeleteBuffers(1, &textUBOColor);
}

unsigned int EGLRendererBackend::getRequiredWindowFlags() const { return SDL_WINDOW_OPENGL; };

std::unique_ptr<ShaderProgram> EGLRendererBackend::createShaderProgram() {
    return ShaderProgramFactory::create(getGraphicsAPI());
}

std::unique_ptr<MeshBuffer> EGLRendererBackend::createMeshBuffer() {
    return MeshBufferFactory::create(getGraphicsAPI());
}

std::unique_ptr<ShaderCompiler> EGLRendererBackend::createShaderCompiler() {
    return ShaderCompilerFactory::create(getGraphicsAPI());
}

bool EGLRendererBackend::init(SDL_Window* window) {
    if (!window) {
        LOG_INFO("Window is null!");
        return false;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        LOG_INFO("Failed to create OpenGL context for SDL Window: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    // Load OpenGL routines using glad. SDL exposes the
    // EGL loader through SDL_GL_GetProcAddress.
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        LOG_INFO("Failed to load OpenGL routines using glad: %s\n", SDL_GetError());
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        return false;
    }

    // Vsync from project.conf: 1 = cap to refresh rate, 0 = uncapped.
    if (SDL_GL_SetSwapInterval(vsyncEnabled ? 1 : 0) != 0) {
        LOG_INFO("Failed to set Swap Interval for SDL Window: %s\n", SDL_GetError());
    }

    return init();
};

bool EGLRendererBackend::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // sRGB output from project.conf. When enabled, the default framebuffer
    // applies a linear->sRGB conversion on write; when disabled (default),
    // colors are written as-is (historical behavior). Requires an
    // sRGB-capable default framebuffer, which SDL provides by default.
    if (srgbEnabled)
        glEnable(GL_FRAMEBUFFER_SRGB);
    else
        glDisable(GL_FRAMEBUFFER_SRGB);

    glGenBuffers(1, &matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(Matrix4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);

    glGenBuffers(1, &materialDataUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, materialDataUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Vector4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, materialDataUBO);

    glGenBuffers(1, &lightDataUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, lightDataUBO);
    glBufferData(GL_UNIFORM_BUFFER, 256, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, lightDataUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    uniformBindings["ModelViewProjection"] = matricesUBO;
    uniformBindings["MaterialData"] = materialDataUBO;
    uniformBindings["LightData"] = lightDataUBO;

    glGenBuffers(1, &instanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, instanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    initSpriteQuad();

    return true;
}

void EGLRendererBackend::onCameraSet() {}

void EGLRendererBackend::clear(Camera* camera) {
    ColorRGBA bgColor = camera ? camera->getBackgroundColor() : COLOR::BLACK;

    // TODO: glClearColor(0x68 / 255.0f, 0xB0 / 255.0f, 0xD8 / 255.0f, 1.0f);
    //  Por que hexa, e por que dividir por 255
    //  Cores costumam ser especificadas no formato de 8 bits por canal (0–255),
    //  que é como você vê em CSS, editores de imagem, color pickers etc. O hexa
    //  é só a forma compacta de escrever esses valores 0–255:
    //  0x68 = 104
    //  0xB0 = 176
    //  0xD8 = 216
    //  Como o OpenGL quer 0.0–1.0 e não 0–255, divide-se cada um por 255:
    //  R: 0x68 / 255.0f = 104 / 255 ≈ 0.408
    //  G: 0xB0 / 255.0f = 176 / 255 ≈ 0.690
    //  B: 0xD8 / 255.0f = 216 / 255 ≈ 0.847
    //  A: 1.0f = opaco (alfa já vem direto em 0.0–1.0, por isso não é dividido)
    //  O resultado é um azul claro acinzentado. Em notação web, essa cor é #68B0D8.
    glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool EGLRendererBackend::initWindowContext() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    return true;
}

void EGLRendererBackend::draw(const Mesh& mesh) {
    auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh.getMeshBufferHandle()));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.getVertices().size() / 3);
    glBindVertexArray(0);
}

void EGLRendererBackend::setUniforms(ShaderProgram* shaderProgram) {
    if (!shaderProgram || !shaderProgram->isValid())
        return;

    shaderProgram->use();
}

void EGLRendererBackend::bindCamera(Camera* camera) {
    if (!camera) {
        LOG_INFO("Camera is null!");
        return;
    }

    WorldObject* cameraObj = camera->getOwner();
    if (!cameraObj) {
        LOG_INFO("Camera has no owner WorldObject!");
        return;
    }

    Matrix4 model = Matrix4(1.0f);

    const auto camPos = cameraObj->getTransform().getPosition();
    const auto camRot = cameraObj->getTransform().getRotation();

    float yawRad = Yume::Math::radians(camRot.y);
    float pitchRad = Yume::Math::radians(camRot.x);

    Vector3 forward;
    forward.x = cos(pitchRad) * sin(yawRad);
    forward.y = sin(pitchRad);
    forward.z = cos(pitchRad) * cos(yawRad);
    forward = Yume::Math::normalize(forward);
    forward = forward * -1.0f;

    Vector3 camPosVec{camPos.x, camPos.y, camPos.z};
    Vector3 target = camPosVec + forward;

    Matrix4 view = Yume::Math::lookAt(camPosVec, target, {0.0f, 1.0f, 0.0f});

    Matrix4 projection;
    if (camera->isOrthographic()) {
        float orthoSize = camera->getOrthoSize();
        float aspect = camera->getAspectRatio();
        projection =
            Yume::Math::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize,
                              camera->getNearDistance(), camera->getFarDistance());
    } else {
        projection =
            Yume::Math::perspective(Yume::Math::radians(camera->getFov()), camera->getAspectRatio(),
                                    camera->getNearDistance(), camera->getFarDistance());
    }

    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), model.data());
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), view.data());
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(Matrix4), sizeof(Matrix4), projection.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void EGLRendererBackend::setBufferDataImpl(const std::string& name, const void* data, size_t size) {
    auto it = uniformBindings.find(name);
    if (it != uniformBindings.end()) {
        glBindBuffer(GL_UNIFORM_BUFFER, it->second);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void EGLRendererBackend::applyMaterial(Material* material) {
    auto program = material->getShaderProgram();
    if (!program || !program->isValid()) {
        return;
    }

    setUniforms(program);
}

void EGLRendererBackend::renderWorldObjects(const std::vector<WorldObject*>& objects,
                                            const std::vector<Light*>& lights) {

    // Reset per-frame draw statistics. (frustumCulledObjects is set by
    // Renderer::render before this call, so it is intentionally not reset here.)
    drawnObjects = 0;
    drawnVerts = 0;
    drawnTris = 0;

    nonInstancedObjects.clear();
    for (auto& [vao, group] : instanceGroups)
        group.models.clear();

    for (auto* obj : objects) {
        if (!obj->hasMesh())
            continue;
        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->getMaterial())
            continue;

        // printf("instancing: %d\n",
        //        meshRenderer->getMaterial()->isInstancingEnabled()); // adiciona isso

        if (!meshRenderer->getMaterial()->isInstancingEnabled()) {
            nonInstancedObjects.push_back(obj);
            continue;
        }

        auto* mesh = obj->getMesh();
        auto* mat = meshRenderer->getMaterial();
        auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh->getMeshBufferHandle()));
        auto shader =
            static_cast<GLuint>(reinterpret_cast<uintptr_t>(mat->getShaderProgram()->getHandle()));
        RenderKey key{vao, shader};

        auto& group = instanceGroups[key];
        if (!group.mesh) {
            group.mesh = mesh;
            group.material = mat;
        }
        group.models.push_back(obj->getTransform().getModelMatrix());
    }

    static bool printed = false;
    if (!printed) {
        LOG_INFO("Groups: %zu\n", instanceGroups.size());
        for (auto& [key, group] : instanceGroups)
            LOG_INFO("  VAO %u shader %u: %zu instances\n", key.vao, key.shader,
                     group.models.size());
        printed = true;
    }

    // instanced
    for (auto& [key, group] : instanceGroups) {
        if (group.models.empty())
            continue;
        auto* mat = group.material;
        mat->use();
        applyMaterial(mat);
        if (!lights.empty())
            mat->applyLight(*lights[0]);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, group.models.size() * sizeof(Matrix4),
                     group.models.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, instanceSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glBindVertexArray(key.vao);
        const GLsizei vertsPerInstance = static_cast<GLsizei>(group.mesh->getVertices().size() / 3);
        const GLsizei instanceCount = static_cast<GLsizei>(group.models.size());
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertsPerInstance, instanceCount);
        glBindVertexArray(0);

        drawnObjects += instanceCount;
        drawnVerts += static_cast<int>(vertsPerInstance) * instanceCount;
        drawnTris += static_cast<int>(vertsPerInstance / 3) * instanceCount;
    }

    // non-instanced — matrix individual via UBO
    for (auto* obj : nonInstancedObjects) {
        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        auto* mat = meshRenderer->getMaterial();
        auto* mesh = obj->getMesh();
        auto* program = mat->getShaderProgramSingle();

        Matrix4 model = obj->getTransform().getModelMatrix();
        glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), model.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        program->use();
        if (!lights.empty())
            mat->applyLight(*lights[0]);

        auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh->getMeshBufferHandle()));
        const GLsizei vertCount = static_cast<GLsizei>(mesh->getVertices().size() / 3);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
        glBindVertexArray(0);

        drawnObjects++;
        drawnVerts += static_cast<int>(vertCount);
        drawnTris += static_cast<int>(vertCount / 3);
    }

    for (auto* obj : objects) {
        if (!obj->hasSprite())
            continue;
        auto* spriteRenderer = obj->getComponent<SpriteRenderer>();
        if (!spriteRenderer || !spriteRenderer->getMaterial())
            continue;

        Matrix4 model = obj->getTransform().getModelMatrix();
        glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), model.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        auto* mat = spriteRenderer->getMaterial();
        mat->use();
        applyMaterial(mat);
        drawSprite(*obj->getSprite());
    }
}

unsigned int EGLRendererBackend::createCubemapTexture(const std::vector<std::string>& faces) {
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
            LOG_INFO("Cubemap texture failed to load at path: " + faces[i].c_str());
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

void EGLRendererBackend::deleteCubemapTexture(unsigned int textureID) {
    glDeleteTextures(1, &textureID);
}

void EGLRendererBackend::renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                                      unsigned int textureID) {
    if (!mainCamera)
        return;

    WorldObject* cameraObj = mainCamera->getOwner();
    if (!cameraObj)
        return;

    glDepthFunc(GL_LEQUAL);

    const auto camPos = cameraObj->getTransform().getPosition();
    Matrix4 camView =
        Yume::Math::lookAt({camPos.x, camPos.y, camPos.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    Matrix4 view = Matrix4(camView.toMatrix3());
    Matrix4 projection = Yume::Math::perspective(
        Yume::Math::radians(mainCamera->getFov()), mainCamera->getAspectRatio(),
        mainCamera->getNearDistance(), mainCamera->getFarDistance());

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.data());
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                       projection.data());
    glUniform1i(glGetUniformLocation(shaderProgram, "skybox"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh.getMeshBufferHandle()));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

void EGLRendererBackend::present(SDL_Window* window) { SDL_GL_SwapWindow(window); }

void EGLRendererBackend::initSpriteQuad() {
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

unsigned int EGLRendererBackend::loadTexture(const std::string& path, uint8_t filterType) {
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
        LOG_INFO("Failed to load texture: " + path);
    }

    return textureID;
}

void EGLRendererBackend::drawSprite(const Sprite& sprite) {
    LOG_INFO("Drawing sprite - TextureID: " + std::to_string(sprite.getTexture()) + " Width: " +
             std::to_string(sprite.getWidth()) + " Height: " + std::to_string(sprite.getHeight()));

    // Não sobrescrever a matriz model, apenas aplicar a escala do sprite
    // A matriz model já foi configurada em renderGameObjects com o Transform
    Matrix4 spriteScale =
        Yume::Math::scale(Matrix4(1.0f), {sprite.getWidth(), sprite.getHeight(), 1.0f});

    Matrix4 currentModel;
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), currentModel.data());

    // Multiplicar: transform * escala do sprite
    Matrix4 finalModel = currentModel * spriteScale;

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), finalModel.data());
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
        LOG_INFO("OpenGL error in drawSprite: " + std::to_string(err));
    }
}

GLuint EGLRendererBackend::compileTextShader(const std::string& path, GLenum type) {
    std::ifstream file(path);
    if (!file.is_open())
        return 0;
    std::stringstream ss;
    ss << file.rdbuf();
    std::string src = ss.str();
    const char* srcPtr = src.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        LOG_INFO("TextRenderer shader error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool EGLRendererBackend::initText(const FontAtlas& atlas, unsigned int texID,
                                  const std::string& vertPath, const std::string& fragPath) {
    textAtlas = &atlas;
    textTextureID = texID;

    GLuint vert = compileTextShader(vertPath, GL_VERTEX_SHADER);
    GLuint frag = compileTextShader(fragPath, GL_FRAGMENT_SHADER);
    if (!vert || !frag)
        return false;

    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vert);
    glAttachShader(textShaderProgram, frag);
    glLinkProgram(textShaderProgram);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok;
    glGetProgramiv(textShaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(textShaderProgram, 512, nullptr, log);
        LOG_INFO("TextRenderer link error: %s\n", log);
        return false;
    }

    GLuint projBlock = glGetUniformBlockIndex(textShaderProgram, "type_TextUniforms");
    if (projBlock != GL_INVALID_INDEX)
        glUniformBlockBinding(textShaderProgram, projBlock, 4);

    GLuint colorBlock = glGetUniformBlockIndex(textShaderProgram, "type_TextColor");
    if (colorBlock != GL_INVALID_INDEX)
        glUniformBlockBinding(textShaderProgram, colorBlock, 5);

    struct ColorBlock {
        ColorRGBA color;
        float distRange;
        float pad[3];
    };

    glGenBuffers(1, &textUBOProjection);
    glBindBuffer(GL_UNIFORM_BUFFER, textUBOProjection);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Matrix4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, textUBOProjection);

    glGenBuffers(1, &textUBOColor);
    glBindBuffer(GL_UNIFORM_BUFFER, textUBOColor);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ColorBlock), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 5, textUBOColor);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * 512, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, textTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void EGLRendererBackend::drawText(const std::string& text, float x, float y, float scale,
                                  ColorRGBA color, int screenWidth, int screenHeight) {
    if (!textAtlas || !textShaderProgram)
        return;

    struct ColorBlock {
        ColorRGBA color;
        float distRange;
        float pad[3];
    };

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glUseProgram(textShaderProgram);

    Matrix4 proj = Yume::Math::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);
    glBindBuffer(GL_UNIFORM_BUFFER, textUBOProjection);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), proj.data());

    float screenPxRange = textAtlas->distanceRange * (scale / textAtlas->atlasSize);
    ColorBlock colorData{color, screenPxRange, {}};
    glBindBuffer(GL_UNIFORM_BUFFER, textUBOColor);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ColorBlock), &colorData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    GLint texLoc =
        glGetUniformLocation(textShaderProgram, "SPIRV_Cross_CombinedmsdfTexturemsdfSampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textTextureID);
    if (texLoc != -1)
        glUniform1i(texLoc, 0);

    glBindVertexArray(textVAO);

    std::vector<float> vertices;
    vertices.reserve(text.size() * 6 * 4);

    float cursorX = x;
    uint32_t prevChar = 0;
    for (char c : text) {
        uint32_t unicode = (uint32_t)(unsigned char)c;
        auto it = textAtlas->glyphs.find(unicode);
        if (it == textAtlas->glyphs.end()) {
            prevChar = unicode;
            continue;
        }

        const GlyphInfo& g = it->second;
        cursorX += textAtlas->getKerning(prevChar, unicode) * scale;

        if (g.hasGeometry) {
            float x0 = cursorX + g.planeLeft * scale, x1 = cursorX + g.planeRight * scale;
            float y0 = y - g.planeTop * scale, y1 = y - g.planeBottom * scale;
            float u0 = g.atlasLeft / textAtlas->atlasWidth,
                  u1 = g.atlasRight / textAtlas->atlasWidth;
            float v0 = 1.0f - (g.atlasBottom / textAtlas->atlasHeight);
            float v1 = 1.0f - (g.atlasTop / textAtlas->atlasHeight);

            float quad[6][4] = {
                {x0, y0, u0, v1}, {x0, y1, u0, v0}, {x1, y1, u1, v0},
                {x0, y0, u0, v1}, {x1, y1, u1, v0}, {x1, y0, u1, v1},
            };
            for (auto& v : quad)
                vertices.insert(vertices.end(), v, v + 4);
        }
        cursorX += g.advance * scale;
        prevChar = unicode;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
                 GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(prevProgram);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
