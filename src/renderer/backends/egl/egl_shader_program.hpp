#ifndef EGL_SHADER_PROGRAM_HPP
#define EGL_SHADER_PROGRAM_HPP

#include "shader_program.hpp"
#include <glad/glad.h>
#include <string>
#include <unordered_map>


class EGLShaderProgram : public ShaderProgram {
  private:
    GLuint programID = 0;
    std::unordered_map<std::string, int> uniformBindings;
    std::unordered_map<std::string, GLuint> uniformBuffers;

  public:
    ~EGLShaderProgram() override;
    bool attachShader(const ShaderAsset& shader) override;
    bool link() override;
    void use() override;
    void setUniformBuffer(const char* name, const void* data, size_t size) override;
    void* getHandle() const override { return reinterpret_cast<void*>(programID); }
    bool isValid() const override { return programID != 0; }
};

#endif // OPENGLSHADERPROGRAM_HPP
