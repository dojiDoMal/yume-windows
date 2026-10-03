#ifndef FONT_ATLAS_HPP
#define FONT_ATLAS_HPP

#include "nlohmann/json.hpp"
#include <fstream>
#include <string>
#include <unordered_map>

struct GlyphInfo {
    float planeLeft, planeBottom, planeRight, planeTop;
    float atlasLeft, atlasBottom, atlasRight, atlasTop;
    float advance;
    bool hasGeometry = false;
};

struct FontAtlas {
    int atlasWidth = 0;
    int atlasHeight = 0;
    float lineHeight = 0.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
    float distanceRange = 2.0f;
    float atlasSize = 32.0f;
    std::unordered_map<uint32_t, GlyphInfo> glyphs;
    std::unordered_map<uint64_t, float> kerning;

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

    float getKerning(uint32_t u1, uint32_t u2) const {
        uint64_t key = ((uint64_t)u1 << 32) | u2;
        auto it = kerning.find(key);
        return it != kerning.end() ? it->second : 0.0f;
    }
};

#endif
