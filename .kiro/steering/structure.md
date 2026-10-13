# Yume Engine — Estrutura e Mapa de Módulos

Todo o código da engine vive em `core/src/`, exposto como include root
(`target_include_directories(yume_core PUBLIC core/src)`). Por isso os includes
são sempre relativos a `core/src` (ex.: `#include "renderer/renderer.hpp"`,
`#include "math/matrix4.hpp"`), nunca relativos ao arquivo atual.

## Layout de alto nível

```
core/src/
  application.{hpp,cpp}      Framework de app: boot/mainLoop/shutdown + hooks onInit/onUpdate
  engine_context.{hpp,cpp}  Yume::Context — agrega input/áudio/display da MultimediaLayer
  graphics_api.hpp          enum GraphicsAPI (CITRO3D, OPENGL, WEBGL, VULKAN, DIRECTX12)
  renderer_config.{hpp,cpp} Leitura de project.conf (api/srgb/vsync + aliases de input)
  logger.{hpp,cpp}          Logger estático (arquivo no desktop, debug nos consoles)
  log_macros.hpp            LOG_INFO/WARN/ERROR (requer #define CLASS_NAME antes)
  timer.{hpp,cpp}           Delta time / FPS, relógio por plataforma
  platform_paths.hpp        Yume::resolveAssetPath (romfs:/ nos consoles, no-op no resto)
  text_renderer.{hpp,cpp}   Renderização de texto MSDF

  assets/        Recursos carregáveis: mesh, material, shader, textura, fonte + fábricas
  components/    Componentes anexáveis a WorldObject (câmera, luz, renderers, script)
  input/         IInput + implementações desktop/3DS + tradução de nomes de tecla
  math/          Vector3/4, Matrix3/4 e Yume::Math (funções de álgebra linear)
  renderer/      Fachada Renderer + RendererBackend + fábrica + backends/ por API
  scene/         Formato de cena, compiler, loader, WorldObject, hierarquia
  window/        DisplayManager + mml/ (camada de multimídia: display/input/áudio)
  yumescript/    Linguagem de script .ys (lexer/parser/AST/interpreter) + testes

  nlohmann/, tinyobjloader/, stb_image*  Bibliotecas de terceiros (NÃO editar)
```

## Responsabilidade de cada módulo

- **assets/** — Carrega e cacheia recursos. `AssetManager` é um cache genérico
  por caminho (`loadAsset<T>(path, ...)`). Shaders, mesh buffers e shader
  programs são criados por **fábricas por `GraphicsAPI`** (`shader_compiler_factory`,
  `mesh_buffer_factory`, `shader_program_factory`).
- **components/** — Dados/comportamento anexados a um `WorldObject`. Base
  `Component` com ganchos `start()`/`update(float)`. Inclui `Transform` (embutido
  no WorldObject, não é um Component), `Camera`, `Light`, `MeshRenderer`,
  `SpriteRenderer`, `TextRendererComponent`, `LodGroup`, `ScriptComponent`.
- **input/** — Interface `Yume::IInput` + `DesktopInput` (SDL) / `N3DSInput`
  (libctru), escolhidos por `IInputFactory`. `key_names` traduz nomes lógicos
  estilo JS para o `KeyCode` nativo da plataforma.
- **math/** — Tipos `Vector3/4`, `Matrix3/4` (column-major, compatíveis com glm)
  e a classe utilitária `Yume::Math`. É a **folha** da árvore de dependências:
  não depende de nenhum outro módulo da engine.
- **renderer/** — Fachada `Renderer` (orquestra a frame) + interface
  `RendererBackend` + `RendererFactory` + um diretório por backend em
  `backends/{opengl,vulkan,directx12,citro3d,webgl}`.
- **scene/** — Pipeline de cena: `scene_format.hpp` (contrato POD),
  `scene_compiler` (`.scn` → `.scnb`, ferramenta de host), `scene_loader`
  (runtime), `WorldObject`/`WorldObjectManager` (grafo de cena), `SceneManager`.
- **window/** — `DisplayManager` orquestra renderer + janela. `mml/` (MultiMedia
  Layer) é o ponto de contato com SDL/libctru: `MultimediaLayer` agrega
  display+input+áudio; `DisplayBackend` é o único lugar que fala com a plataforma
  sobre a janela, expondo tudo por `void*` opaco.
- **yumescript/** — Linguagem embutida `.ys`. `Lexer → Parser → AST → Interpreter`
  (tree-walking). Integra na engine via `ScriptComponent`.

## Fluxo de dependências (direção das setas)

```
math  ◄──  components  ◄──  scene  ──►  renderer (backends concretos)
  ▲            ▲              │             ▲
  └── renderer ┘              └── assets ───┘ (via interfaces abstratas)

application ──► scene_manager, display_manager, multimedia_layer, context
yumescript ──► (isolado) ; ScriptComponent liga yumescript a components/scene/input
scene_compiler (host tool) ──► só depende de scene_format.hpp + nlohmann/stb
```

Regras práticas:
- **`math` não importa nada da engine.** Mantenha assim.
- **Backends concretos nunca vazam.** Headers de `renderer/backends/*` só são
  incluídos nos `.cpp` das fábricas, sob `#ifdef` de plataforma.
- **O renderer é agnóstico de SDL.** Qualquer coisa de janela/plataforma passa
  por `DisplayBackend` como `void*`.
- **`scene_format.hpp` é contrato compartilhado** entre a ferramenta de build
  (`scene_compiler`) e o runtime (`scene_loader`). Mudar o layout exige recompilar
  as cenas e, se mudar o `sizeof`, bump do byte de versão em `SCENE_MAGIC`.

## Projetos consumidores

A engine é uma biblioteca. Um jogo/exemplo vive **fora** deste repositório (ex.:
`../yume-examples/rotating-cube`), linka `yume_core` e chama `yume_add_project()`.
Ao raciocinar sobre "como alguém usa a feature X", pense do ponto de vista desse
projeto consumidor: ele só enxerga os headers públicos em `core/src` e os
formatos de autoria (`.scn`, `project.conf`, shaders, `.ys`).
