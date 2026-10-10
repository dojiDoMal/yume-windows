#define CLASS_NAME "Citro3DShaderProgram"
#include "citro3d_shader_program.hpp"
#include "assets/shader_asset.hpp"
#include "log_macros.hpp"
#include <cstring>

Citro3DShaderProgram::~Citro3DShaderProgram() {
    if (initialized) {
        shaderProgramFree(&program);
        initialized = false;
    }
    // O DVLB em si pertence ao ShaderAsset/Citro3DShaderCompiler (foi criado lá
    // e é liberado por destroy()); aqui só soltamos o shaderProgram_s.
}

bool Citro3DShaderProgram::attachShader(const ShaderAsset& shader) {
    // O PICA200 só tem shader de vértice (e, opcionalmente, geometry). A engine
    // chama attachShader para o vertex E para o fragment, mas no 3DS a etapa de
    // fragmento é configurada por TexEnv no backend, não por um shader. Então
    // ignoramos qualquer coisa que não seja o vertex shader.
    if (shader.getType() != ShaderType::VERTEX) {
        // TODO: quando houver geometry shader, anexá-lo aqui via
        //       shaderProgramSetGsh. Fragment não se aplica ao PICA200.
        return true;
    }

    auto* handle = static_cast<Citro3DShaderHandle*>(shader.getHandle());
    if (!handle || !handle->dvlb) {
        LOG_ERROR("Vertex shader handle is null or has no DVLB");
        return false;
    }

    vshDvlb = handle->dvlb;
    return true;
}

bool Citro3DShaderProgram::link() {
    if (!vshDvlb) {
        LOG_ERROR("Can not link shader program without a vertex shader (DVLB)");
        return false;
    }

    shaderProgramInit(&program);
    // DVLE[0] é o primeiro (e normalmente único) executável do DVLB do vertex
    // shader montado pelo picasso.
    Result rc = shaderProgramSetVsh(&program, &vshDvlb->DVLE[0]);
    if (R_FAILED(rc)) {
        LOG_ERROR("shaderProgramSetVsh failed");
        shaderProgramFree(&program);
        return false;
    }

    initialized = true;

    // Resolve os uniforms do vertex shader por nome. São opcionais: um shader
    // pode usar só "modelView"/"projection" ou uma MVP única. shaderInstance...
    // devolve -1 quando o nome não existe, o que tratamos como "ausente".
    uLocProjection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
    uLocModelView = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
    uLocModel = shaderInstanceGetUniformLocation(program.vertexShader, "model");

    return true;
}

void Citro3DShaderProgram::use() {
    if (initialized)
        C3D_BindProgram(&program);
}

void Citro3DShaderProgram::setUniformBuffer(const char* name, const void* data, size_t size) {
    // No OpenGL cada "name" é um Uniform Buffer Object. O PICA200 não tem UBOs:
    // os valores vão para registradores de uniform do vertex shader. Aqui
    // traduzimos os blocos conhecidos da engine para escritas de uniform.
    //
    // As matrizes model/view/projection são enviadas pelo backend
    // (setBufferDataImpl) já combinadas; este caminho fica para quando o
    // ShaderProgram receber matrizes diretamente. A engine é column-major
    // (compatível com glm); o PICA200 espera row-major, então o backend é quem
    // transpõe antes de chamar C3D_FVUnifMtx4x4. Por isso, para matrizes,
    // preferimos as escritas feitas no backend.
    if (!initialized)
        return;

    (void)data;
    (void)size;

    // TODO (iluminação/material): quando o shader PICA200 de produção existir,
    // mapear "MaterialData" (ColorRGBA) e "LightData" (direção/cor/intensidade)
    // para uniforms float via C3D_FVUnifSet nos locais correspondentes. Hoje o
    // shader mínimo do cubo não consome esses blocos, então são ignorados sem
    // erro para não poluir o log a cada frame.
    if (std::strcmp(name, "MaterialData") != 0 && std::strcmp(name, "LightData") != 0 &&
        std::strcmp(name, "ModelViewProjection") != 0) {
        LOG_WARN("Unknown uniform block for Citro3D program: " + std::string(name));
    }
}
