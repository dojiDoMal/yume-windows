#ifndef RENDERER_CONFIG_HPP
#define RENDERER_CONFIG_HPP

#include "graphics_api.hpp"
#include <string>

/**
 * @brief Configuração do renderer no nível do projeto, lida uma vez no boot.
 *
 * Carregada de um arquivo JSON (project.conf), centraliza decisões de
 * apresentação que pertencem ao engine/aplicação, não a uma cena específica:
 * qual backend criar, espaço de cor de saída, vsync, cena inicial e
 * propriedades da janela.
 *
 * Sobre @ref srgb: @c false (padrão) usa swapchain/framebuffer linear (UNORM),
 * escrevendo as cores como estão; @c true usa sRGB, fazendo a GPU converter
 * linear->sRGB na escrita. Atenção: isso só muda o espaço de cor de SAÍDA; não
 * reinterpreta as cores de entrada (da cena ou de texturas) como sRGB — o
 * tratamento gamma-correct da entrada é uma etapa separada ainda não
 * implementada.
 *
 * @see loadRendererConfig
 */
struct RendererConfig {
    GraphicsAPI api = GraphicsAPI::OPENGL; ///< Backend gráfico a ser criado.
    bool srgb = false;                     ///< Espaço de cor de saída (ver descrição da struct).
    bool vsync = true;                     ///< @c true limita à taxa de atualização do monitor.

    /**
     * Cena inicial carregada no boot, como nome de arquivo de cena compilada
     * resolvido relativo ao diretório de trabalho (ex.: "scene.scnb"). Cada
     * projeto declara sua cena de entrada em project.conf. Vazio significa
     * "nenhuma cena configurada" e o engine registra um aviso em vez de adivinhar.
     */
    std::string scene = "scene.scnb";

    std::string windowTitle = "Engine"; ///< Título da janela.
    int windowWidth = 1280;             ///< Largura da janela, em pixels.
    int windowHeight = 720;             ///< Altura da janela, em pixels.
};

/**
 * @brief Carrega a configuração do renderer a partir de um arquivo.
 *
 * Arquivo ausente ou erro de parse não são fatais: a configuração retornada
 * mantém os padrões seguros e um aviso é registrado. Assim o engine sempre
 * inicia, mesmo sem o arquivo.
 *
 * @param path Caminho do arquivo de configuração (padrão "project.conf").
 * @return A configuração carregada, ou os padrões em caso de falha.
 */
RendererConfig loadRendererConfig(const std::string& path = "project.conf");

#endif // RENDERER_CONFIG_HPP
