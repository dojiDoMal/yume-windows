---
inclusion: fileMatch
fileMatchPattern: ['core/src/assets/**', '**/*.vxs', '**/*.pxs']
---

# Guia do Módulo: Assets

Carregamento e cache de recursos (mesh, material, shader, textura, fonte), com as
peças dependentes de GPU criadas por **fábricas por `GraphicsAPI`**.

> Bibliotecas de terceiros neste módulo — **nunca editar**: `nlohmann/`,
> `tinyobjloader/`, `stb_image.h`, `stb_image_impl.cpp`.

## Peças

- `asset.hpp` — base `Asset`: `path`, `loaded`, ciclo `load()`/`unload()`.
- `asset_manager.{hpp,cpp}` — `AssetManager`: cache genérico por caminho. Template
  `loadAsset<T>(path, args...)` reaproveita se já carregado, senão constrói,
  chama `load()` e guarda. **Dono** dos assets (`vector<unique_ptr<Asset>>`).
- `mesh.{hpp,cpp}`, `mesh_buffer.hpp` — malha + buffer na GPU; `mesh_buffer`
  é interface, criada por `mesh_buffer_factory`.
- `material.{hpp,cpp}` — shaders + cor base + uniforms; `init()` trata o fragment
  shader como **opcional** (3DS/PICA200 não tem fragment programável).
- `shader_asset.{hpp,cpp}`, `shader_program.hpp`, `shader_compiler.hpp`,
  `shader_type.hpp` — pipeline de shader em runtime (interfaces).
- `shader_compiler_factory`, `shader_program_factory`, `mesh_buffer_factory` —
  as três fábricas por API.
- `font_atlas.hpp` — atlas MSDF para texto.

## Padrão de fábrica (repetido idêntico 3×)

As três fábricas têm a mesma forma:
`static std::unique_ptr<Base> create(GraphicsAPI api, void* context = nullptr)`.
Elas selecionam a implementação em **duas camadas**:

1. **Compile-time:** includes dos backends sob `#ifdef` de plataforma
   (`__3DS__` → citro3d; `__SWITCH__` → opengl; `PLATFORM_WEBGL` → webgl; senão
   desktop opengl/vulkan + `_WIN32` → directx12).
2. **Runtime:** `switch (api)` retornando a classe concreta, ou `nullptr` para
   APIs indisponíveis.

`void* context` carrega o estado opaco do backend quando preciso (Vulkan e D3D12
recebem o backend via `static_cast<XRendererBackend*>(context)`); backends sem
estado (OpenGL/WebGL/Citro3D) ignoram.

> Na prática quem chama as fábricas é o **RendererBackend** (via
> `createShaderCompiler()`/`createShaderProgram()`/`createMeshBuffer()`), não o
> SceneLoader diretamente. Ao adicionar um backend novo, registre nas três.

## Dois mecanismos de cache (atenção)

- `AssetManager` — cache genérico por path, para quem usa a API de assets direto.
- `SceneLoader` — mantém **caches próprios** (`meshCache`, `materialCache`,
  `fontAtlasCache`), chaveados por path (mesh/atlas) ou por combinação
  shaders+cor (material). São paralelos ao `AssetManager`; não os confunda.

## Pipeline de shaders (dois momentos)

- **Build-time (host):** a fonte canônica é **HLSL** (`*.vxs` vertex, `*.pxs`
  pixel). `cmake/YumeProject.cmake` roda dxc (HLSL → SPIR-V) e spirv-cross
  (SPIR-V → GLSL/`.nxs`), + DXIL `.cso` no Windows. O 3DS foge disso: PICA200 é
  assembly `*.v.pica` montado por `picasso` em `.shbin`.
- **Runtime (device):** não há tradução de HLSL. `ShaderAsset::load()` chama
  `ShaderCompiler::compile(...)` do backend, que transforma o artefato já no
  formato nativo (GLSL/CSO/.shbin) num objeto da API.
- **A ponte é a extensão:** o `.scnb` guarda o path base (ex.: `shaders/basic.vxs`)
  e o runtime anexa `getShaderExtension()` do backend (`.glsl`/`.nxs`/`.cso`).
  Se a extensão não casar com o que o build gerou, o material falha e a malha
  some. Mantenha os dois lados em sincronia.

## Convenções

Namespace **global**. Include guards `#ifndef` (atenção: alguns comentários de
`#endif` divergem do nome do guard — ex.: guard `ASSET_MANAGER_HPP`, comentário
`ASSETMANAGER_HPP`; preserve o guard). `unique_ptr` para posse, `shared_ptr` para
Mesh/Material compartilhados. Logging com `#define CLASS_NAME` + `log_macros.hpp`.
