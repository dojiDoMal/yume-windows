#include "web_gl_renderer_backend.hpp"
#include "color.hpp"
#include "graphics_api.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

GraphicsAPI WebGLRendererBackend::getGraphicsAPI() const { return GraphicsAPI::WEBGL; }

WebGLRendererBackend::~WebGLRendererBackend() {
    if (matricesUBO)
        glDeleteBuffers(1, &matricesUBO);
}

void WebGLRendererBackend::onCameraSet() {
    if (mainCamera) {
        ColorRGBA& bgColor = mainCamera->getBackgroundColor();
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    }
}

bool WebGLRendererBackend::init() {
    glEnable(GL_DEPTH_TEST);

    glGenBuffers(1, &matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return true;
}

void WebGLRendererBackend::clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

bool WebGLRendererBackend::initWindowContext() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    return true;
}

void WebGLRendererBackend::draw(const Mesh& mesh) {
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh.getMeshBufferHandle()));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.getVertices().size() / 3);
    glBindVertexArray(0);
}

void WebGLRendererBackend::setUniforms(unsigned int shaderProgram) {

    if (!mainCamera) {
        //printf("Error: mainCamera is null\n");
        return;
    }

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f));
    model = glm::rotate(model, (float)(emscripten_get_now() / 1000.0), glm::vec3(0.5f, 1.0f, 0.0f));

    auto& camPos = mainCamera->getPosition();
    //printf("Camera pos: %f, %f, %f\n", camPos.x, camPos.y, camPos.z);
    glm::mat4 view = glm::lookAt({camPos.x, camPos.y, camPos.z}, glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection =
        glm::perspective(glm::radians(mainCamera->getFov()), mainCamera->getAspectRatio(),
                         mainCamera->getNearDistance(), mainCamera->getFarDistance());

    GLint loc = glGetUniformLocation(shaderProgram, "model");
    //if (loc == -1)
        //printf("Uniform 'model' not found\n");

    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(model));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    
}

unsigned int WebGLRendererBackend::createCubemapTexture(const std::vector<std::string>& faces) {
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
            //printf("Cubemap texture failed to load at path: %s\n", faces[i].c_str());
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

void WebGLRendererBackend::deleteCubemapTexture(unsigned int textureID) {
    glDeleteTextures(1, &textureID);
}

void WebGLRendererBackend::renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                                        unsigned int textureID) {
    if (!mainCamera)
        return;

    glDepthFunc(GL_LEQUAL);
    glUseProgram(shaderProgram);

    // Validate program
    GLint isValid;
    glValidateProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &isValid);
    if (!isValid) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        //printf("Shader validation failed: %s\n", infoLog);
        return;
    }

    auto& camPos = mainCamera->getPosition();
    glm::mat4 camView = glm::lookAt({camPos.x, camPos.y, camPos.z}, glm::vec3(0.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 view = glm::mat4(glm::mat3(camView));
    glm::mat4 projection =
        glm::perspective(glm::radians(mainCamera->getFov()), mainCamera->getAspectRatio(),
                         mainCamera->getNearDistance(), mainCamera->getFarDistance());

    GLuint blockIndex = glGetUniformBlockIndex(shaderProgram, "type_Matrices");
    if (blockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(shaderProgram, blockIndex, 0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    auto vao = static_cast<GLuint>(reinterpret_cast<uintptr_t>(mesh.getMeshBufferHandle()));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}
