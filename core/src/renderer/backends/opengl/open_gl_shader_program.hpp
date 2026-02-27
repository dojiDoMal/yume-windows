#ifndef OPEN_GL_SHADER_PROGRAM_HPP
#define OPEN_GL_SHADER_PROGRAM_HPP

#include "shader_program.hpp"
#include <GL/glew.h>
#include <unordered_map>
#include <string>

class OpenGLShaderProgram : public ShaderProgram {
private:
    GLuint programID = 0;
    std::unordered_map<std::string, int> uniformBindings;
    std::unordered_map<std::string, GLuint> uniformBuffers;

public:
    ~OpenGLShaderProgram() override;
    bool attachShader(const ShaderAsset& shader) override;
    bool link() override;
    void use() override;
    void setUniformBuffer(const char* name, const void* data, size_t size) override;
    void* getHandle() const override { return reinterpret_cast<void*>(programID); }
    bool isValid() const override { return programID != 0; }
};

#endif // OPENGLSHADERPROGRAM_HPP
