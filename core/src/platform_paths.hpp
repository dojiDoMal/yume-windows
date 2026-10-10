#ifndef PLATFORM_PATHS_HPP
#define PLATFORM_PATHS_HPP

#include <string>

namespace Yume {

/**
 * @brief Prefixo do sistema de arquivos onde os assets de runtime vivem.
 *
 * Nos consoles (Switch e 3DS) os assets são empacotados no RomFS da aplicação,
 * montado em "romfs:/" por romfsInit(). No desktop/web os assets ficam ao lado
 * do executável (o diretório de trabalho), então o prefixo é vazio.
 *
 * Isto substitui o antigo chdir("romfs:/") do Switch: o 3DS/libctru não tem um
 * chdir confiável (ver devkitPro/libctru#138), então em vez de mudar o diretório
 * de trabalho, todo caminho de asset é resolvido explicitamente contra esta raiz.
 */
inline const char* assetRoot() {
#if defined(__SWITCH__) || defined(__3DS__)
    return "romfs:/";
#else
    return "";
#endif
}

/**
 * @brief Resolve um caminho de asset relativo contra a raiz da plataforma.
 *
 * No desktop/web devolve @p relativePath inalterado; nos consoles prefixa
 * "romfs:/". Deve ser aplicado uma única vez, no ponto em que um caminho vindo
 * da cena/config vira um caminho de abertura de arquivo — nunca dentro das
 * folhas compartilhadas (compiladores de shader, loadTexture, etc.), para não
 * prefixar duas vezes.
 *
 * @param relativePath Caminho relativo ao diretório de assets (ex.: "cube.obj").
 * @return O caminho pronto para abrir no sistema de arquivos da plataforma.
 */
inline std::string resolveAssetPath(const std::string& relativePath) {
    return std::string(assetRoot()) + relativePath;
}

} // namespace Yume

#endif // PLATFORM_PATHS_HPP
