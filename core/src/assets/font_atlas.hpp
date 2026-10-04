#ifndef FONT_ATLAS_HPP
#define FONT_ATLAS_HPP

#include "nlohmann/json.hpp"
#include <fstream>
#include <string>
#include <unordered_map>

/**
 * @brief Posição e métricas de um glifo dentro do atlas de fonte.
 *
 * Os campos @c plane* descrevem a caixa do glifo no espaço do texto (onde
 * desenhá-lo em relação ao cursor) e os @c atlas* descrevem a região
 * correspondente na textura do atlas (de onde amostrar). @c advance é o quanto
 * o cursor avança após o glifo.
 */
struct GlyphInfo {
    float planeLeft, planeBottom, planeRight, planeTop; ///< Caixa do glifo no espaço do texto.
    float atlasLeft, atlasBottom, atlasRight, atlasTop; ///< Região do glifo na textura do atlas.
    float advance;                                      ///< Avanço do cursor após o glifo.
    bool hasGeometry = false;                           ///< @c false para glifos sem desenho (ex.: espaço).
};

/**
 * @brief Atlas de fonte MSDF carregado de JSON: métricas e tabela de glifos.
 *
 * Guarda as dimensões do atlas, as métricas da fonte (altura de linha,
 * ascender, descender) e um mapa de glifos indexado por código Unicode, além da
 * tabela de kerning entre pares de caracteres. É consumido pelo TextRenderer
 * para posicionar e amostrar cada glifo.
 *
 * @see TextRenderer, GlyphInfo
 */
struct FontAtlas {
    int atlasWidth = 0;          ///< Largura da textura do atlas, em pixels.
    int atlasHeight = 0;         ///< Altura da textura do atlas, em pixels.
    float lineHeight = 0.0f;     ///< Altura de uma linha de texto.
    float ascender = 0.0f;       ///< Distância acima da linha de base.
    float descender = 0.0f;      ///< Distância abaixo da linha de base.
    float distanceRange = 2.0f;  ///< Alcance do campo de distância (MSDF).
    float atlasSize = 32.0f;     ///< Tamanho de referência da fonte no atlas.
    std::unordered_map<uint32_t, GlyphInfo> glyphs; ///< Glifos indexados por código Unicode.
    std::unordered_map<uint64_t, float> kerning;    ///< Kerning por par (u1<<32 | u2).

    /**
     * @brief Carrega o atlas de um arquivo JSON (formato msdf-atlas-gen).
     * @param jsonPath Caminho do JSON de descrição do atlas.
     * @return @c true se o arquivo foi aberto e lido com sucesso.
     */
    bool load(const std::string& jsonPath) {
        std::ifstream f(jsonPath);
        if (!f.is_open())
            return false;

        nlohmann::json j;
        f >> j;

        atlasWidth = j["atlas"]["width"];
        atlasHeight = j["atlas"]["height"];
        distanceRange = j["atlas"]["distanceRange"];
        lineHeight = j["metrics"]["lineHeight"];
        ascender = j["metrics"]["ascender"];
        descender = j["metrics"]["descender"];
        atlasSize = j["atlas"]["size"];

        for (auto& g : j["glyphs"]) {
            GlyphInfo info;
            info.advance = g["advance"];
            if (g.contains("planeBounds")) {
                info.planeLeft = g["planeBounds"]["left"];
                info.planeBottom = g["planeBounds"]["bottom"];
                info.planeRight = g["planeBounds"]["right"];
                info.planeTop = g["planeBounds"]["top"];
                info.atlasLeft = g["atlasBounds"]["left"];
                info.atlasBottom = g["atlasBounds"]["bottom"];
                info.atlasRight = g["atlasBounds"]["right"];
                info.atlasTop = g["atlasBounds"]["top"];
                info.hasGeometry = true;
            }
            glyphs[g["unicode"]] = info;
        }

        for (auto& k : j["kerning"]) {
            uint32_t u1 = k["unicode1"];
            uint32_t u2 = k["unicode2"];
            uint64_t key = ((uint64_t)u1 << 32) | u2;
            kerning[key] = k["advance"];
        }

        return true;
    }

    /**
     * @brief Retorna o ajuste de kerning entre dois caracteres.
     * @param u1 Código Unicode do caractere anterior.
     * @param u2 Código Unicode do caractere seguinte.
     * @return O avanço de kerning, ou 0 se não houver entrada para o par.
     */
    float getKerning(uint32_t u1, uint32_t u2) const {
        uint64_t key = ((uint64_t)u1 << 32) | u2;
        auto it = kerning.find(key);
        return it != kerning.end() ? it->second : 0.0f;
    }
};

#endif
