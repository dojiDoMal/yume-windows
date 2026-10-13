---
inclusion: fileMatch
fileMatchPattern: ['core/src/renderer/**', 'core/src/renderer_config.*', 'core/src/graphics_api.hpp', 'core/src/text_renderer.*']
---

# Guia do Módulo: Renderer

Renderização multiplataforma por **interface abstrata + fábrica**, com backend
escolhido em duas camadas: compile-time (macros de plataforma) e runtime (enum
`GraphicsAPI`).

## Peças

- `graphics_api.hpp` — `enum class GraphicsAPI { CITRO3D, OPENGL, WEBGL, VULKAN, DIRECTX12 }`.
  `OPENGL` cobre desktop (GLEW) **e** Switch (OpenGL ES via EGL/glad), com o mesmo
  `OpenGLRendererBackend`.
- `renderer/renderer_backend.hpp` — a interface `RendererBackend`. ~20 métodos
  virtuais puros + estado compartilhado (câmera, luzes, estatísticas por frame,
  flags `srgb`/`vsync`/`frustumCulling`). Usa NVI: `setBufferData<T>()` (template
  não-virtual) encaminha para `setBufferDataImpl(name, void*, size)` (virtual puro).
- `renderer/renderer.{hpp,cpp}` — fachada `Renderer`. **Possui** o backend
  (`delete` no destrutor). `render(scene)` orquestra a frame; `present(window)`
  faz o swap.
- `renderer/renderer_factory.cpp` — `RendererFactory::create(GraphicsAPI)`.
- `renderer/frustum.hpp` — `Frustum` (Gribb-Hartmann), 6 planos, `intersectsSphere`.
- `renderer_config.{hpp,cpp}` — lê `project.conf`; `parseApi()` mapeia string →
  `GraphicsAPI` (com aliases, fallback para OPENGL, nunca aborta).

## Fluxo de uma frame (`Renderer::render`)

1. Guard: sem backend → `LOG_ERROR`; sem câmera na cena → `LOG_WARN`; retorna.
2. `backend->bindCamera(camera)` e `backend->clear(camera)`.
3. Coleta luzes: `getComponent<Light>()` dos objetos de luz da cena.
4. Atualiza LOD: para cada objeto com `LodGroup`, estima raio e chama
   `lodGroup->update(...)`, trocando a mesh ativa.
5. Frustum culling na CPU: reconstrói view/projection (`Yume::Math::lookAt` +
   `perspective`/`ortho`), monta o `Frustum` e descarta esferas fora do volume.
   Sprites e objetos sem mesh são sempre mantidos.
6. `backend->setFrustumCulledObjects(n)` e `backend->renderWorldObjects(objs, lights)`.
7. O host chama `Renderer::present(window)` em seguida.

## Backends existentes

`renderer/backends/{opengl,vulkan,directx12,citro3d,webgl}`. Cada um tem 4 pares
de arquivos com prefixo da API: `*_renderer_backend`, `*_mesh_buffer`,
`*_shader_compiler`, `*_shader_program`.

## Como adicionar um novo backend (checklist completo)

1. `graphics_api.hpp`: novo valor no `enum class GraphicsAPI`.
2. `renderer_config.cpp` → `parseApi()`: mapear a string do `project.conf`.
3. Criar `renderer/backends/<api>/` com os 4 pares (use `opengl` como gabarito):
   - `<api>_renderer_backend.{hpp,cpp}` — `class XRendererBackend : public RendererBackend`
     implementando **todos** os virtuais puros, incluindo `createShaderProgram()`,
     `createShaderCompiler()`, `createMeshBuffer()` e `getShaderExtension()`.
   - `<api>_mesh_buffer`, `<api>_shader_compiler`, `<api>_shader_program`.
4. Registrar nas **três fábricas** (mesmo padrão `#ifdef` include + `switch` case):
   - `renderer/renderer_factory.cpp`
   - `assets/mesh_buffer_factory.cpp`
   - `assets/shader_compiler_factory.cpp`
   (Se precisar do estado do backend, siga o padrão Vulkan/D3D12 de receber
   `void* context` e `static_cast<XRendererBackend*>(context)`.)
5. `CMakeLists.txt`: o `.cpp` novo compila via glob, mas **adicione o backend à
   poda** `foreach(BACKEND ...)` dos ramos de plataforma onde ele NÃO deve existir
   (espelhe `vulkan`/`directx12`).
6. `cmake/<plataforma>.cmake`: linkar as libs do backend em `yume_core` (PUBLIC).
7. `cmake/YumeProject.cmake`: garantir que o build gera shaders na extensão que o
   `getShaderExtension()` do backend espera.
8. Se usar SDL para a janela, criar o display backend por API em
   `window/mml/sdl2/` (ver `window.md`).

## Regras do módulo

- **Nunca inclua headers de `backends/*` fora dos `.cpp` das fábricas.** O resto
  da engine só conhece a interface `RendererBackend`.
- **Nada de SDL aqui.** Serviços de janela chegam via `DisplayBackend` como `void*`.
- Namespace **global** (sem `Yume`). Include guards `#ifndef`. `#define CLASS_NAME`
  + `log_macros.hpp` para logar. Config nunca é fatal.

## Dependências

`assets` (Mesh/Material/ShaderProgram/MeshBuffer/FontAtlas), `scene`
(`Scene`/`WorldObject`/`Sprite`), `components` (`Camera`/`Light`/`LodGroup`),
`math` (view/projection + frustum), `color.hpp`, e `DisplayBackend` (forward-decl).
