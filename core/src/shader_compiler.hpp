#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include "shader_type.hpp"
#include <string>

/**
 * @brief Interface de compilação de shaders independente de backend.
 *
 * Cada API gráfica (OpenGL, Vulkan, DirectX 12, EGL, WebGL) fornece sua própria
 * implementação concreta. Em vez de instanciá-la diretamente, use a
 * ShaderCompilerFactory, que seleciona a classe certa conforme a plataforma.
 *
 * O compilador trabalha com @c void* opacos ("handles") que representam o
 * objeto nativo do shader. O chamador é responsável por liberar cada handle
 * com destroy() quando ele não for mais necessário.
 *
 * Exemplo de uso:
 * @code
 * auto compiler = ShaderCompilerFactory::create(GraphicsAPI::OPENGL);
 * void* handle = nullptr;
 * if (compiler->compile(source, ShaderType::VERTEX, &handle)) {
 *     // ... usar o shader ...
 *     compiler->destroy(handle);
 * }
 * @endcode
 *
 * @see ShaderCompilerFactory, ShaderType
 */
class ShaderCompiler {
  public:
    virtual ~ShaderCompiler() = default;

    /**
     * @brief Compila o código-fonte de um shader para um objeto nativo.
     *
     * @param source    Código-fonte do shader (ex.: GLSL, HLSL, SPIR-V textual).
     * @param type      Estágio do pipeline ao qual o shader pertence.
     * @param outHandle Saída: recebe o ponteiro para o shader nativo compilado.
     *                  Só é válido quando a função retorna @c true.
     * @return @c true se a compilação teve sucesso; @c false caso contrário.
     */
    virtual bool compile(const std::string& source, ShaderType type, void** outHandle) = 0;

    /**
     * @brief Libera o shader nativo associado ao handle.
     * @param handle Handle previamente retornado por compile().
     */
    virtual void destroy(void* handle) = 0;

    /**
     * @brief Indica se um handle aponta para um shader válido e utilizável.
     * @param handle Handle a verificar.
     * @return @c true se o handle for válido.
     */
    virtual bool isValid(void* handle) = 0;
};

#endif // SHADERCOMPILER_HPP
