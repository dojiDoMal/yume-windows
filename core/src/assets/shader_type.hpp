#ifndef SHADER_TYPE_HPP
#define SHADER_TYPE_HPP

/**
 * @brief Estágio do pipeline gráfico ao qual um shader pertence.
 *
 * Usado por ShaderCompiler::compile para informar ao backend como tratar o
 * código-fonte fornecido.
 */
enum class ShaderType {
    VERTEX,   ///< Processa vértices (posição, normais, UVs).
    FRAGMENT, ///< Calcula a cor de cada pixel/fragmento.
    GEOMETRY, ///< Gera primitivas adicionais a partir das existentes.
    COMPUTE   ///< Shader de propósito geral, fora do pipeline de rasterização.
};

#endif
