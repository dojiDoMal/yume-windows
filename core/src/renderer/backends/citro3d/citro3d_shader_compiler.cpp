#define CLASS_NAME "Citro3DShaderCompiler"
#include "log_macros.hpp"

#include "citro3d_shader_compiler.hpp"
#include "citro3d_shader_program.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

bool Citro3DShaderCompiler::compile(const std::string& source, ShaderType type, void** outHandle) {
    (void)type; // O DVLB carrega todos os DVLE; o ShaderProgram escolhe o estágio.

    // Lê o .shbin inteiro para memória. DVLB_ParseFile NÃO copia o bytecode: ele
    // aponta para dentro deste buffer, que portanto precisa viver tanto quanto o
    // DVLB. Guardamos ambos no Citro3DShaderHandle, destruído em destroy().
    std::ifstream file(source, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open shader binary (.shbin): " + source);
        return false;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size <= 0) {
        LOG_ERROR("Shader binary is empty: " + source);
        return false;
    }

    // O bytecode do DVLB é lido como palavras de 32 bits (u32*), então alinhamos
    // a alocação a 4 bytes.
    const size_t wordCount = (static_cast<size_t>(size) + 3) / 4;
    auto* code = static_cast<u32*>(std::malloc(wordCount * sizeof(u32)));
    if (!code) {
        LOG_ERROR("Out of memory loading shader binary: " + source);
        return false;
    }
    std::memset(code, 0, wordCount * sizeof(u32));

    if (!file.read(reinterpret_cast<char*>(code), size)) {
        LOG_ERROR("Failed to read shader binary: " + source);
        std::free(code);
        return false;
    }

    DVLB_s* dvlb = DVLB_ParseFile(code, static_cast<u32>(size));
    if (!dvlb) {
        LOG_ERROR("DVLB_ParseFile failed for: " + source);
        std::free(code);
        return false;
    }

    auto* handle = new Citro3DShaderHandle{dvlb, code};
    *outHandle = handle;
    return true;
}

void Citro3DShaderCompiler::destroy(void* handle) {
    if (!handle)
        return;

    auto* h = static_cast<Citro3DShaderHandle*>(handle);
    if (h->dvlb)
        DVLB_Free(h->dvlb);
    if (h->code)
        std::free(h->code);
    delete h;
}

bool Citro3DShaderCompiler::isValid(void* handle) {
    auto* h = static_cast<Citro3DShaderHandle*>(handle);
    return h && h->dvlb;
}
