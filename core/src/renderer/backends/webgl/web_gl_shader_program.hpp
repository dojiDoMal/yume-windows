#ifndef WEB_GL_SHADER_PROGRAM_HPP
#define WEB_GL_SHADER_PROGRAM_HPP

#include "shader_program.hpp"
#include <GLES3/gl3.h>

class WebGLShaderProgram : public ShaderProgram {
private:
    GLuint programID = 0;

public:
    ~WebGLShaderProgram() override;
    bool attachShader(const ShaderAsset& shader) override;
    bool link() override;
    void use() override;
    void* getHandle() const override { return reinterpret_cast<void*>(programID); }
    bool isValid() const override { return programID != 0; }
};

#endif 
