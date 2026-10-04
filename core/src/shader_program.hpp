#ifndef SHADER_PROGRAM_HPP
#define SHADER_PROGRAM_HPP

#include <cstddef>

class ShaderAsset;

/**
 * @brief Programa de shader "linkado", pronto para renderizar.
 *
 * Enquanto o ShaderAsset representa um único estágio compilado (vertex,
 * fragment, etc.), o ShaderProgram combina vários estágios em um pipeline
 * utilizável: anexam-se os shaders, chama-se link() e, a cada frame, use()
 * ativa o programa antes das chamadas de desenho.
 *
 * Cada backend gráfico fornece sua implementação; instancie via
 * ShaderProgramFactory.
 *
 * @code
 * auto program = ShaderProgramFactory::create(api, context);
 * program->attachShader(vertexAsset);
 * program->attachShader(fragmentAsset);
 * program->link();
 * // por frame:
 * program->use();
 * @endcode
 *
 * @see ShaderProgramFactory, ShaderAsset
 */
class ShaderProgram {
  public:
    virtual ~ShaderProgram() = default;

    /**
     * @brief Anexa um estágio de shader já compilado ao programa.
     * @param shader Asset de shader a incluir no pipeline.
     * @return @c true se o shader foi anexado com sucesso.
     */
    virtual bool attachShader(const ShaderAsset& shader) = 0;

    /**
     * @brief Linka os shaders anexados em um programa executável.
     * @return @c true se a linkagem teve sucesso.
     */
    virtual bool link() = 0;

    /** @brief Ativa este programa para as próximas chamadas de desenho. */
    virtual void use() = 0;

    /**
     * @brief Envia dados para um uniform buffer do programa.
     * @param name Nome do bloco de uniform no shader.
     * @param data Ponteiro para os bytes a enviar.
     * @param size Tamanho, em bytes, do bloco de dados.
     */
    virtual void setUniformBuffer(const char* name, const void* data, size_t size) = 0;

    /** @brief Retorna o handle nativo do programa (uso específico de backend). */
    virtual void* getHandle() const = 0;

    /** @brief Indica se o programa foi linkado com sucesso e pode ser usado. */
    virtual bool isValid() const = 0;
};

#endif // SHADERPROGRAM_HPP
