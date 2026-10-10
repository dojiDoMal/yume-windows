#include "citro3d_mesh_buffer.hpp"
#include <cstring>

Citro3DMeshBuffer::~Citro3DMeshBuffer() { destroy(); }

void* Citro3DMeshBuffer::getHandle() const {
    // No OpenGL isto era o VAO empacotado num void*; aqui o PICA200 não tem um
    // objeto equivalente, então devolvemos o próprio buffer. O backend trata o
    // handle de malha como "ponteiro para o Citro3DMeshBuffer" e lê os dados de
    // vértice/contagem na hora de montar o C3D_BufInfo e chamar C3D_DrawArrays.
    return const_cast<Citro3DMeshBuffer*>(this);
}

bool Citro3DMeshBuffer::createBuffers(const std::vector<float>& vertices,
                                      const std::vector<float>& normals) {
    destroy();

    if (vertices.empty() || vertices.size() % 3 != 0)
        return false;

    vertexCount = static_cast<int>(vertices.size() / 3);
    hasNormals = (normals.size() == vertices.size());

    // A GPU do 3DS só consegue ler atributos de memória linear/VRAM. Intercalamos
    // posição e normal num único bloco com stride fixo (kFloatsPerVertex) para
    // casar com os dois loaders de atributo configurados no backend. Quando não
    // há normais, preenchemos com zero mantendo o mesmo layout — assim o backend
    // não precisa de dois caminhos de desenho distintos.
    const size_t bytes = static_cast<size_t>(vertexCount) * kFloatsPerVertex * sizeof(float);
    vboData = linearAlloc(bytes);
    if (!vboData)
        return false;

    auto* dst = static_cast<float*>(vboData);
    for (int i = 0; i < vertexCount; ++i) {
        const int src = i * 3;
        const int out = i * kFloatsPerVertex;

        dst[out + 0] = vertices[src + 0];
        dst[out + 1] = vertices[src + 1];
        dst[out + 2] = vertices[src + 2];

        if (hasNormals) {
            dst[out + 3] = normals[src + 0];
            dst[out + 4] = normals[src + 1];
            dst[out + 5] = normals[src + 2];
        } else {
            dst[out + 3] = 0.0f;
            dst[out + 4] = 0.0f;
            dst[out + 5] = 0.0f;
        }
    }

    return true;
}

void Citro3DMeshBuffer::bind() {
    // O vínculo de buffers no PICA200 (C3D_BufInfo) é global e configurado pelo
    // backend imediatamente antes de C3D_DrawArrays, por isso não há estado para
    // "bind" aqui. Mantido para cumprir a interface MeshBuffer.
}

void Citro3DMeshBuffer::unbind() {}

void Citro3DMeshBuffer::destroy() {
    if (vboData) {
        linearFree(vboData);
        vboData = nullptr;
    }
    vertexCount = 0;
    hasNormals = false;
}
